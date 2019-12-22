/*************************************************************************
 * This source is derived from the Linux wine project. see www.winehq.com.
 * Bug fixes and patches to this file should be made in the original and then 
 * reflected into Twine.  Please see WINE in this distribution for more 
 * information.  This file is from wine-981129/debugger/hash.c
 ************************************************************************/

/*
 * File hash.c - generate hash tables for Wine debugger symbols
 *
 * Copyright (C) 1993, Eric Youngdale.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include "debugger.h"
#include "module.h"
#include "selectors.h"
#include "toolhelp.h"

#define NR_NAME_HASH 16384
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

static char * reg_name[] =
{
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};


struct name_hash
{
    struct name_hash * next;		/* Used to look up within name hash */
    char *             name;
    char *             sourcefile;

    int		       n_locals;
    int		       locals_alloc;
    WineLocals       * local_vars;
  
    int		       n_lines;
    int		       lines_alloc;
    WineLineNo       * linetab;

    DBG_ADDR           addr;
    unsigned short     flags;
    unsigned short     breakpoint_offset;
    unsigned int       symbol_size;
};


static BOOL32 DEBUG_GetStackSymbolValue( const char * name, DBG_ADDR *addr );
static int sortlist_valid = FALSE;

static int sorttab_nsym;
static struct name_hash ** addr_sorttab = NULL;

static struct name_hash * name_hash_table[NR_NAME_HASH];

static unsigned int name_hash( const char * name )
{
    unsigned int hash = 0;
    unsigned int tmp;
    const char * p;

    p = name;

    while (*p) 
      {
	hash = (hash << 4) | *p++;

	if( (tmp = (hash & 0xf0000000)) )
	 {
	    hash ^= tmp >> 24;
	  }
	hash &= ~tmp;
      }
    //hash = (hash << 5) | strlen(name);
    return hash % NR_NAME_HASH;
}

int
DEBUG_cmp_sym(const void * p1, const void * p2)
{
  struct name_hash ** name1 = (struct name_hash **) p1;
  struct name_hash ** name2 = (struct name_hash **) p2;

  if( ((*name1)->flags & SYM_INVALID) != 0 )
    {
      return -1;
    }

  if( ((*name2)->flags & SYM_INVALID) != 0 )
    {
      return 1;
    }

  if( (*name1)->addr.seg > (*name2)->addr.seg )
    {
      return 1;
    }

  if( (*name1)->addr.seg < (*name2)->addr.seg )
    {
      return -1;
    }

  if( (*name1)->addr.off > (*name2)->addr.off )
    {
      return 1;
    }

  if( (*name1)->addr.off < (*name2)->addr.off )
    {
      return -1;
    }

  return 0;
}

/***********************************************************************
 *           DEBUG_ResortSymbols
 *
 * Rebuild sorted list of symbols.
 */
static
void
DEBUG_ResortSymbols()
{
    struct name_hash *nh;
    int		nsym = 0;
    int		i;

    for(i=0; i<NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
	  {
	    nsym++;
	  }
    }

    sorttab_nsym = nsym;
    if( nsym == 0 )
      {
	return;
      }

    addr_sorttab = (struct name_hash **) xrealloc(addr_sorttab, 
					 nsym * sizeof(struct name_hash *));

    nsym = 0;
    for(i=0; i<NR_NAME_HASH; i++)
    {
        for (nh = name_hash_table[i]; nh; nh = nh->next)
	  {
	    addr_sorttab[nsym++] = nh;
	  }
    }

    qsort(addr_sorttab, nsym,
	  sizeof(struct name_hash *), DEBUG_cmp_sym);
    sortlist_valid = TRUE;

}

/***********************************************************************
 *           DEBUG_AddSymbol
 *
 * Add a symbol to the table.
 */
struct name_hash *
DEBUG_AddSymbol( const char * name, const DBG_ADDR *addr, const char * source,
		 int flags)
{
    struct name_hash  * new;
    struct name_hash *nh;
    static char  prev_source[PATH_MAX] = {'\0', };
    static char * prev_duped_source = NULL;
    char * c;
    int hash;

    if(strlen(name) == 0)
	return 0;

    hash = name_hash(name);

    for (nh = name_hash_table[hash]; nh; nh = nh->next)
    {
	if(strcmp(nh->name,name) == 0) {
	    if(source == 0) {
		nh->addr.off = addr->off;
		nh->addr.seg = addr->seg;
 	    }
	}
    }

    for (nh = name_hash_table[hash]; nh; nh = nh->next)
    {
 	if( ((nh->flags & SYM_INVALID) != 0) && strcmp(name, nh->name) == 0 )
        {
 	    nh->addr.off = addr->off;
 	    nh->addr.seg = addr->seg;
 	    if( nh->addr.type == NULL && addr->type != NULL )
            {
 		nh->addr.type = addr->type;
            }
 	    nh->flags &= ~SYM_INVALID;
 	    return nh;
        }
 	if (nh->addr.seg == addr->seg &&
 	    nh->addr.off == addr->off &&
 	    strcmp(name, nh->name) == 0 )
        {
            return nh;
        }
    }

    /*
     * First see if we already have an entry for this symbol.  If so
     * return it, so we don't end up with duplicates.
     */
    
    new = (struct name_hash *) xmalloc(sizeof(struct name_hash));
    new->addr = *addr;
    new->name = xstrdup(name);

    if( source != NULL )
      {
	/*
	 * This is an enhancement to reduce memory consumption.  The idea
	 * is that we duplicate a given string only once.  This is a big
	 * win if there are lots of symbols defined in a given source file.
	 */
	if( strcmp(source, prev_source) == 0 )
	  {
	    new->sourcefile = prev_duped_source;
	  }
	else
	  {
	    strcpy(prev_source, source);
	    prev_duped_source = new->sourcefile = xstrdup(source);
	  }
      }
    else
      {
	new->sourcefile = NULL;
      }

    new->n_lines	= 0;
    new->lines_alloc	= 0;
    new->linetab	= NULL;

    new->n_locals	= 0;
    new->locals_alloc	= 0;
    new->local_vars	= NULL;

    new->flags		= flags;
    new->next		= NULL;

    /* Now insert into the hash table */
    new->next = name_hash_table[hash];
    name_hash_table[hash] = new;

    /*
     * Check some heuristics based upon the file name to see whether
     * we want to step through this guy or not.  These are machine generated
     * assembly files that are used to translate between the MS way of
     * calling things and the GCC way of calling things.  In general we
     * always want to step through.
     */
    if( source != NULL )
      {
	c = strrchr(source, '.');
	if( c != NULL && strcmp(c, ".s") == 0 )
	  {
	    c = strrchr(source, '/');
	    if( c != NULL )
	      {
		c++;
		if(    (strcmp(c, "callfrom16.s") == 0)
		    || (strcmp(c, "callto16.s") == 0)
		    || (strcmp(c, "call32.s") == 0) )
		  {
		    new->flags |= SYM_TRAMPOLINE;
		  }
	      }
	  }
      }

    sortlist_valid = FALSE;
    return new;
}

BOOL32 DEBUG_Normalize(struct name_hash * nh )
{

  /*
   * We aren't adding any more locals or linenumbers to this function.
   * Free any spare memory that we might have allocated.
   */
  if( nh == NULL )
    {
      return TRUE;
    }

  if( nh->n_locals != nh->locals_alloc )
    {
      nh->locals_alloc = nh->n_locals;
      nh->local_vars = xrealloc(nh->local_vars,
				  nh->locals_alloc * sizeof(WineLocals));
    }

  if( nh->n_lines != nh->lines_alloc )
    {
      nh->lines_alloc = nh->n_lines;
      nh->linetab = xrealloc(nh->linetab,
			      nh->lines_alloc * sizeof(WineLineNo));
    }

  return TRUE;
}

/***********************************************************************
 *           DEBUG_GetSymbolValue
 *
 * Get the address of a named symbol.
 */
BOOL32 DEBUG_GetSymbolValue( const char * name, const int lineno, 
			     DBG_ADDR *addr, int bp_flag )
{
    char buffer[256];
    struct name_hash *nh;
    struct name_hash *fh = 0;
    int cnt = 0;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
      {
	if( (nh->flags & SYM_INVALID) != 0 )
	  {
	    continue;
	  }

        if (!strcmp(nh->name, name)) {
	        cnt++;
		fh = nh;
	}
      }

    nh = fh;
    cnt = 0;
    fh  = 0;
    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
	  {
	    if( (nh->flags & SYM_INVALID) != 0 )
	      {
		continue;
	      }
            if (!strcmp(nh->name, buffer)) {
	        cnt++;
		fh = nh;
	    }
	  }
    }

    /*
     * If we don't have anything here, then try and see if this
     * is a local symbol to the current stack frame.  No matter
     * what, we have nothing more to do, so we let that function
     * decide what we ultimately return.
     */
    if (!nh) 
      {
	return DEBUG_GetStackSymbolValue(name, addr);
      }

    return DEBUG_GetLineNumberAddr( nh, lineno, addr, bp_flag );
}

/***********************************************************************
 *           DEBUG_GetLineNumberAddr
 *
 * Get the address of a named symbol.
 */
BOOL32 DEBUG_GetLineNumberAddr( struct name_hash * nh, const int lineno, 
				DBG_ADDR *addr, int bp_flag )
{
    int i;

    if( lineno == -1 )
      {
	*addr = nh->addr;
	if( bp_flag )
	  {
	    addr->off += nh->breakpoint_offset;
	  }
      }
    else
      {
	/*
	 * Search for the specific line number.  If we don't find it,
	 * then return FALSE.
	 */
	if( nh->linetab == NULL )
	  {
	    return FALSE;
	  }

	for(i=0; i < nh->n_lines; i++ )
	  {
	    if( nh->linetab[i].line_number == lineno )
	      {
		*addr = nh->linetab[i].pc_offset;
		return TRUE;
	      }
	  }

	/*
	 * This specific line number not found.
	 */
	return FALSE;
      }

    return TRUE;
}


/***********************************************************************
 *           DEBUG_SetSymbolValue
 *
 * Set the address of a named symbol.
 */
BOOL32 DEBUG_SetSymbolValue( const char * name, const DBG_ADDR *addr )
{
    char buffer[256];
    struct name_hash *nh;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
        if (!strcmp(nh->name, name)) break;

    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
            if (!strcmp(nh->name, buffer)) break;
    }

    if (!nh) return FALSE;
    nh->addr = *addr;
    nh->flags &= SYM_INVALID;
    DBG_FIX_ADDR_SEG( &nh->addr, DS_reg(&DEBUG_context) );
    return TRUE;
}


/***********************************************************************
 *           DEBUG_FindNearestSymbol
 *
 * Find the symbol nearest to a given address.
 * If ebp is specified as non-zero, it means we should dump the argument
 * list into the string we return as well.
 */
const char * DEBUG_FindNearestSymbol( const DBG_ADDR *addr, int flag,
				      struct name_hash ** rtn,
				      unsigned int ebp,
				      struct list_id * source)
{
    static char name_buffer[MAX_PATH + 256];
    static char arglist[1024];
    static char argtmp[256];
    struct name_hash * nearest = NULL;
    int mid, high, low;
    unsigned	int * ptr;
    int lineno;
    char * lineinfo, *sourcefile;
    int i;
    char linebuff[16];

    if( rtn != NULL )
      {
	*rtn = NULL;
      }

    if( source != NULL )
      {
 	source->sourcefile = NULL;
	source->line = -1;
      }

    if( sortlist_valid == FALSE )
      {
	DEBUG_ResortSymbols();
      }

    if( sortlist_valid == FALSE )
      {
	return NULL;
      }

    /*
     * FIXME  - use the binary search that we added to
     * the function DEBUG_CheckLinenoStatus.  Better yet, we should
     * probably keep some notion of the current function so we don't
     * have to search every time.
     */
    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = sorttab_nsym;
    if( addr_sorttab[0]->addr.seg > addr->seg
	|| (   addr_sorttab[0]->addr.seg == addr->seg 
	    && addr_sorttab[0]->addr.off > addr->off) )
      {
	nearest = NULL;
      }
    else if( addr_sorttab[high - 1]->addr.seg < addr->seg
	|| (   addr_sorttab[high - 1]->addr.seg == addr->seg 
	    && addr_sorttab[high - 1]->addr.off < addr->off) )
      {
	nearest = addr_sorttab[high - 1];
      }
    else
      {
	while(1==1)
	  {
	    mid = (high + low)/2;
	    if( mid == low )
	      {
		/* 
		 * See if there are any other entries that might also
		 * have the same address, and would also have a line
		 * number table.  
		 */
		if( mid > 0 && addr_sorttab[mid]->linetab == NULL )
		  {
		    if(    (addr_sorttab[mid - 1]->addr.seg ==
			    addr_sorttab[mid]->addr.seg)
			&& (addr_sorttab[mid - 1]->addr.off ==
			    addr_sorttab[mid]->addr.off)
		        && (addr_sorttab[mid - 1]->linetab != NULL) )
		      {
			mid--;
		      }
		  }

		if(    (mid < sorttab_nsym - 1)
		    && (addr_sorttab[mid]->linetab == NULL) )
		  {
		    if(    (addr_sorttab[mid + 1]->addr.seg ==
			    addr_sorttab[mid]->addr.seg)
			&& (addr_sorttab[mid + 1]->addr.off ==
			    addr_sorttab[mid]->addr.off)
		        && (addr_sorttab[mid + 1]->linetab != NULL) )
		      {
			mid++;
		      }
		  }
		nearest = addr_sorttab[mid];
#if 0
		fprintf(stderr, "Found %x:%x when looking for %x:%x %x %s\n",
			addr_sorttab[mid ]->addr.seg,
			addr_sorttab[mid ]->addr.off,
			addr->seg, addr->off,
			addr_sorttab[mid ]->linetab,
			addr_sorttab[mid ]->name);
#endif
		break;
	      }
	    if(    (addr_sorttab[mid]->addr.seg < addr->seg)
		|| (   addr_sorttab[mid]->addr.seg == addr->seg 
		    && addr_sorttab[mid]->addr.off <= addr->off) )
	      {
		low = mid;
	      }
	    else
	      {
		high = mid;
	      }
	  }
      }

    if (!nearest) return NULL;

    if( rtn != NULL )
      {
	*rtn = nearest;
      }

    /*
     * Fill in the relevant bits to the structure so that we can
     * locate the source and line for this bit of code.
     */
    if( source != NULL )
      {
	source->sourcefile = nearest->sourcefile;
	if( nearest->linetab == NULL )
	  {
	    source->line = -1;
	  }
	else
	  {
	    source->line = nearest->linetab[0].line_number;
	  }
      }

    lineinfo = "";
    lineno = -1;

    /*
     * Prepare to display the argument list.  If ebp is specified, it is
     * the framepointer for the function in question.  If not specified,
     * we don't want the arglist.
     */
    memset(arglist, '\0', sizeof(arglist));
    if( ebp != 0 )
      {
	for(i=0; i < nearest->n_locals; i++ )
	  {
	    /*
	     * If this is a register (offset == 0) or a local
	     * variable, we don't want to know about it.
	     */
	    if( nearest->local_vars[i].offset <= 0 )
	      {
		continue;
	      }

	    ptr = (unsigned int *) (ebp + nearest->local_vars[i].offset);
	    if( arglist[0] == '\0' )
	      {
		arglist[0] = '(';
	      }
	    else
	      {
		strcat(arglist, ", ");
	      }

	    sprintf(argtmp, "%s=0x%x", nearest->local_vars[i].name,
		    *ptr);
	    strcat(arglist, argtmp);
	  }
	if( arglist[0] == '(' )
	  {
	    strcat(arglist, ")");
	  }
      }

    if( (nearest->sourcefile != NULL) && (flag == TRUE)
	&& (addr->off - nearest->addr.off < 0x100000) )
      {

	/*
	 * Try and find the nearest line number to the current offset.
	 */
	if( nearest->linetab != NULL )
        {
            low = 0;
            high = nearest->n_lines;
            while ((high - low) > 1)
            {
                mid = (high + low) / 2;
                if (addr->off < nearest->linetab[mid].pc_offset.off)
                    high = mid;
                else
                    low = mid;
            }
            lineno = nearest->linetab[low].line_number;
        }

	if( lineno != -1 )
	  {
	    sprintf(linebuff, ":%d", lineno);
	    lineinfo = linebuff;
	    if( source != NULL )
	      {
		source->line = lineno;
	      }
	  }

        /* Remove the path from the file name */
        sourcefile = strrchr( nearest->sourcefile, '/' );
        if (!sourcefile) sourcefile = nearest->sourcefile;
        else sourcefile++;

	if (addr->off == nearest->addr.off)
	  sprintf( name_buffer, "%s%s [%s%s]", nearest->name, 
		   arglist, sourcefile, lineinfo);
	else
	  sprintf( name_buffer, "%s+0x%lx%s [%s%s]", nearest->name,
		   addr->off - nearest->addr.off, 
		   arglist, sourcefile, lineinfo );
      }
    else
      {
	if (addr->off == nearest->addr.off)
	  sprintf( name_buffer, "%s%s", nearest->name, arglist);
	else {
	  if (addr->seg && (nearest->addr.seg!=addr->seg))
	      return NULL;
	  else
	      sprintf( name_buffer, "%s+0x%lx%s", nearest->name,
		       addr->off - nearest->addr.off, arglist);
 	}
      }
    return name_buffer;
}


/***********************************************************************
 *           DEBUG_ReadSymbolTable
 *
 * Read a symbol file into the hash table.
 */
void DEBUG_ReadSymbolTable( const char * filename )
{
    FILE * symbolfile;
    DBG_ADDR addr = { 0, 0 };
    int nargs;
    char type;
    char * cpnt;
    char buffer[256];
    char name[256];

    if (!(symbolfile = fopen(filename, "r")))
    {
        fprintf( stderr, "Unable to open symbol table %s\n", filename );
        return;
    }

    fprintf( stderr, "Reading symbols from file %s\n", filename );

    while (1)
    {
        fgets( buffer, sizeof(buffer), symbolfile );
        if (feof(symbolfile)) break;
		
        /* Strip any text after a # sign (i.e. comments) */
        cpnt = buffer;
        while (*cpnt)
            if(*cpnt++ == '#') { *cpnt = 0; break; }
		
        /* Quietly ignore any lines that have just whitespace */
        cpnt = buffer;
        while(*cpnt)
        {
            if(*cpnt != ' ' && *cpnt != '\t') break;
            cpnt++;
        }
        if (!(*cpnt) || *cpnt == '\n') continue;
		
        nargs = sscanf(buffer, "%lx %c %s", &addr.off, &type, name);
        DEBUG_AddSymbol( name, &addr, NULL, SYM_WINE );
    }
    fclose(symbolfile);
    DEBUG_ResortSymbols();
}

#if 0
/***********************************************************************
 *           DEBUG_LoadEntryPoints16
 *
 * Load the entry points of a Win16 module into the hash table.
 */
static void DEBUG_LoadEntryPoints16( HMODULE16 hModule, NE_MODULE *pModule,
                                     const char *name )
{
    DBG_ADDR addr;
    char buffer[256];
    FARPROC16 address;

    /* First search the resident names */

    unsigned char *cpnt = (unsigned char *)pModule + pModule->name_table;
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        sprintf( buffer, "%s.%.*s", name, *cpnt, cpnt + 1 );
        if ((address = NE_GetEntryPoint(hModule, *(WORD *)(cpnt + *cpnt + 1))))
        {
            addr.seg = HIWORD(address);
            addr.off = LOWORD(address);
            addr.type = NULL;
            DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );
        }
    }

    /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return;  /* No non-resident table */
    cpnt = (char *)GlobalLock16( pModule->nrname_handle );
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        sprintf( buffer, "%s.%.*s", name, *cpnt, cpnt + 1 );
        if ((address = NE_GetEntryPoint(hModule, *(WORD *)(cpnt + *cpnt + 1))))
        {
            addr.seg = HIWORD(address);
            addr.off = LOWORD(address);
            addr.type = NULL;
            DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );
        }
    }
}


/***********************************************************************
 *           DEBUG_LoadEntryPoints32
 *
 * Load the entry points of a Win32 module into the hash table.
 */
static void DEBUG_LoadEntryPoints32( HMODULE32 hModule, const char *name )
{
#define RVA(x) (hModule+(DWORD)(x))

    DBG_ADDR addr;
    char buffer[256];
    int i, j;
    IMAGE_SECTION_HEADER *pe_seg;
    IMAGE_EXPORT_DIRECTORY *exports;
    IMAGE_DATA_DIRECTORY *dir;
    WORD *ordinals;
    void **functions;
    const char **names;

    addr.seg = 0;
    addr.type = NULL;

    /* Add start of DLL */

    addr.off = hModule;
    DEBUG_AddSymbol( name, &addr, NULL, SYM_WIN32 | SYM_FUNC );

    /* Add entry point */

    sprintf( buffer, "%s.EntryPoint", name );
    addr.off = (DWORD)RVA_PTR( hModule, OptionalHeader.AddressOfEntryPoint );
    DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );

    /* Add start of sections */

    pe_seg = PE_SECTIONS(hModule);
    for (i = 0; i < PE_HEADER(hModule)->FileHeader.NumberOfSections; i++)
    {
        sprintf( buffer, "%s.%s", name, pe_seg->Name );
        addr.off = RVA(pe_seg->VirtualAddress );
        DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );
        pe_seg++;
    }

    /* Add exported functions */

    dir = &PE_HEADER(hModule)->OptionalHeader.
                                   DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir->Size) return;
    exports   = (IMAGE_EXPORT_DIRECTORY *)RVA( dir->VirtualAddress );
    ordinals  = (WORD *)RVA( exports->AddressOfNameOrdinals );
    names     = (const char **)RVA( exports->AddressOfNames );
    functions = (void **)RVA( exports->AddressOfFunctions );

    for (i = 0; i < exports->NumberOfNames; i++)
    {
        if (!names[i]) continue;
        sprintf( buffer, "%s.%s", name, (char *)RVA(names[i]) );
        addr.off = RVA( functions[ordinals[i]] );
        DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );
    }

    for (i = 0; i < exports->NumberOfFunctions; i++)
    {
        if (!functions[i]) continue;
        /* Check if we already added it with a name */
        for (j = 0; j < exports->NumberOfNames; j++)
            if ((ordinals[j] == i) && names[j]) break;
        if (j < exports->NumberOfNames) continue;
        sprintf( buffer, "%s.%ld", name, i + exports->Base );
        addr.off = (DWORD)RVA( functions[i] );
        DEBUG_AddSymbol( buffer, &addr, NULL, SYM_WIN32 | SYM_FUNC );
    }

    dir = &PE_HEADER(hModule)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (dir->Size)
        DEBUG_RegisterDebugInfo(hModule, name, dir->VirtualAddress, dir->Size);
#undef RVA
}


/***********************************************************************
 *           DEBUG_LoadEntryPoints
 *
 * Load the entry points of all the modules into the hash table.
 */
void DEBUG_LoadEntryPoints(void)
{
    MODULEENTRY entry;
    NE_MODULE *pModule;
    BOOL32 ok;
    WINE_MODREF	*wm;

    for (ok = ModuleFirst(&entry); ok; ok = ModuleNext(&entry))
    {
        if (!(pModule = NE_GetPtr( entry.hModule ))) continue;
        fprintf( stderr, " %s", entry.szModule );

        if (!(pModule->flags & NE_FFLAGS_WIN32))  /* NE module */
            DEBUG_LoadEntryPoints16( entry.hModule, pModule, entry.szModule );
    }
    for (wm=PROCESS_Current()->modref_list;wm;wm=wm->next) {
        fprintf( stderr, " %s", wm->modname );
	DEBUG_LoadEntryPoints32( wm->module, wm->modname );
    }
}
#endif


void
DEBUG_AddLineNumber( struct name_hash * func, int line_num, 
		     unsigned long offset )
{
  if( func == NULL )
    {
      return;
    }

  if( func->n_lines + 1 >= func->lines_alloc )
    {
      func->lines_alloc += 64;
      func->linetab = xrealloc(func->linetab,
			      func->lines_alloc * sizeof(WineLineNo));
    }

  func->linetab[func->n_lines].line_number = line_num;
  func->linetab[func->n_lines].pc_offset.seg = func->addr.seg;
  func->linetab[func->n_lines].pc_offset.off = func->addr.off + offset;
  func->linetab[func->n_lines].pc_offset.type = NULL;
  func->n_lines++;
}


struct wine_locals *
DEBUG_AddLocal( struct name_hash * func, int regno, 
		int offset,
		int pc_start,
		int pc_end,
		char * name)
{
  if( func == NULL )
    {
      return NULL;
    }

  if( func->n_locals + 1 >= func->locals_alloc )
    {
      func->locals_alloc += 32;
      func->local_vars = xrealloc(func->local_vars,
			      func->locals_alloc * sizeof(WineLocals));
    }

  func->local_vars[func->n_locals].regno = regno;
  func->local_vars[func->n_locals].offset = offset;
  func->local_vars[func->n_locals].pc_start = pc_start;
  func->local_vars[func->n_locals].pc_end = pc_end;
  func->local_vars[func->n_locals].name = xstrdup(name);
  func->local_vars[func->n_locals].type = NULL;
  func->n_locals++;

  return &func->local_vars[func->n_locals - 1];
}

void
DEBUG_DumpHashInfo()
{
  int i;
  int depth;
  struct name_hash *nh;

  /*
   * Utility function to dump stats about the hash table.
   */
    for(i=0; i<NR_NAME_HASH; i++)
    {
      depth = 0;
      for (nh = name_hash_table[i]; nh; nh = nh->next)
	{
	  depth++;
	}
      fprintf(stderr, "Bucket %d: %d\n", i, depth);
    }
}

/***********************************************************************
 *           DEBUG_CheckLinenoStatus
 *
 * Find the symbol nearest to a given address.
 * If ebp is specified as non-zero, it means we should dump the argument
 * list into the string we return as well.
 */
int DEBUG_CheckLinenoStatus( const DBG_ADDR *addr)
{
    struct name_hash * nearest = NULL;
    int mid, high, low;

    if( sortlist_valid == FALSE )
      {
	DEBUG_ResortSymbols();
      }

    /*
     * Binary search to find closest symbol.
     */
    low = 0;
    high = sorttab_nsym;
    if( addr_sorttab[0]->addr.seg > addr->seg
	|| (   addr_sorttab[0]->addr.seg == addr->seg 
	    && addr_sorttab[0]->addr.off > addr->off) )
      {
	nearest = NULL;
      }
    else if( addr_sorttab[high - 1]->addr.seg < addr->seg
	|| (   addr_sorttab[high - 1]->addr.seg == addr->seg 
	    && addr_sorttab[high - 1]->addr.off < addr->off) )
      {
	nearest = addr_sorttab[high - 1];
      }
    else
      {
	while(1==1)
	  {
	    mid = (high + low)/2;
	    if( mid == low )
	      {
		/* 
		 * See if there are any other entries that might also
		 * have the same address, and would also have a line
		 * number table.  
		 */
		if( mid > 0 && addr_sorttab[mid]->linetab == NULL )
		  {
		    if(    (addr_sorttab[mid - 1]->addr.seg ==
			    addr_sorttab[mid]->addr.seg)
			&& (addr_sorttab[mid - 1]->addr.off ==
			    addr_sorttab[mid]->addr.off)
		        && (addr_sorttab[mid - 1]->linetab != NULL) )
		      {
			mid--;
		      }
		  }

		if(    (mid < sorttab_nsym - 1)
		    && (addr_sorttab[mid]->linetab == NULL) )
		  {
		    if(    (addr_sorttab[mid + 1]->addr.seg ==
			    addr_sorttab[mid]->addr.seg)
			&& (addr_sorttab[mid + 1]->addr.off ==
			    addr_sorttab[mid]->addr.off)
		        && (addr_sorttab[mid + 1]->linetab != NULL) )
		      {
			mid++;
		      }
		  }
		nearest = addr_sorttab[mid];
#if 0
		fprintf(stderr, "Found %x:%x when looking for %x:%x %x %s\n",
			addr_sorttab[mid ]->addr.seg,
			addr_sorttab[mid ]->addr.off,
			addr->seg, addr->off,
			addr_sorttab[mid ]->linetab,
			addr_sorttab[mid ]->name);
#endif
		break;
	      }
	    if(    (addr_sorttab[mid]->addr.seg < addr->seg)
		|| (   addr_sorttab[mid]->addr.seg == addr->seg 
		    && addr_sorttab[mid]->addr.off <= addr->off) )
	      {
		low = mid;
	      }
	    else
	      {
		high = mid;
	      }
	  }
      }

    if (!nearest) return FUNC_HAS_NO_LINES;

    if( nearest->flags & SYM_STEP_THROUGH )
      {
	/*
	 * This will cause us to keep single stepping until
	 * we get to the other side somewhere.
	 */
	return NOT_ON_LINENUMBER;
      }

    if( (nearest->flags & SYM_TRAMPOLINE) )
      {
	/*
	 * This will cause us to keep single stepping until
	 * we get to the other side somewhere.
	 */
	return FUNC_IS_TRAMPOLINE;
      }

    if( nearest->linetab == NULL )
      {
	return FUNC_HAS_NO_LINES;
      }


    /*
     * We never want to stop on the first instruction of a function
     * even if it has it's own linenumber.  Let the thing keep running
     * until it gets past the function prologue.  We only do this if there
     * is more than one line number for the function, of course.
     */
    if( nearest->addr.off == addr->off && nearest->n_lines > 1 )
      {
	return NOT_ON_LINENUMBER;
      }

    if( (nearest->sourcefile != NULL)
	&& (addr->off - nearest->addr.off < 0x100000) )
      {
          low = 0;
          high = nearest->n_lines;
          while ((high - low) > 1)
          {
              mid = (high + low) / 2;
              if (addr->off < nearest->linetab[mid].pc_offset.off) high = mid;
              else low = mid;
          }
          if (addr->off == nearest->linetab[low].pc_offset.off)
              return AT_LINENUMBER;
          else
              return NOT_ON_LINENUMBER;
      }

    return FUNC_HAS_NO_LINES;
}

/***********************************************************************
 *           DEBUG_GetFuncInfo
 *
 * Find the symbol nearest to a given address.
 * Returns sourcefile name and line number in a format that the listing
 * handler can deal with.
 */
void
DEBUG_GetFuncInfo( struct list_id * ret, const char * filename, 
		   const char * name)
{
    char buffer[256];
    char * pnt;
    struct name_hash *nh;

    for(nh = name_hash_table[name_hash(name)]; nh; nh = nh->next)
      {
	if( filename != NULL )
	  {

	    if( nh->sourcefile == NULL )
	      {
		continue;
	      }

	    pnt = strrchr(nh->sourcefile, '/');
	    if( strcmp(nh->sourcefile, filename) != 0
		&& (pnt == NULL || strcmp(pnt + 1, filename) != 0) )
	      {
		continue;
	      }
	  }
        if (!strcmp(nh->name, name)) break;
      }

    if (!nh && (name[0] != '_'))
    {
        buffer[0] = '_';
        strcpy(buffer+1, name);
        for(nh = name_hash_table[name_hash(buffer)]; nh; nh = nh->next)
	  {
	    if( filename != NULL )
	      {
		if( nh->sourcefile == NULL )
		  {
		    continue;
		  }

		pnt = strrchr(nh->sourcefile, '/');
		if( strcmp(nh->sourcefile, filename) != 0
		    && (pnt == NULL || strcmp(pnt + 1, filename) != 0) )
		  {
		    continue;
		  }
	      }
            if (!strcmp(nh->name, buffer)) break;
	  }
    }

    if( !nh )
      {
	if( filename != NULL )
	  {
	    fprintf(stderr, "No such function %s in %s\n", name, filename);
	  }
	else
	  {
	    fprintf(stderr, "No such function %s\n", name);
	  }
	ret->sourcefile = NULL;
	ret->line = -1;
	return;
      }

    ret->sourcefile = nh->sourcefile;

    /*
     * Search for the specific line number.  If we don't find it,
     * then return FALSE.
     */
    if( nh->linetab == NULL )
      {
	ret->line = -1;
      }
    else
      {
	ret->line = nh->linetab[0].line_number;
      }
}

/***********************************************************************
 *           DEBUG_GetStackSymbolValue
 *
 * Get the address of a named symbol from the current stack frame.
 */
static
BOOL32 DEBUG_GetStackSymbolValue( const char * name, DBG_ADDR *addr )
{
  struct name_hash * curr_func;
  unsigned int	     ebp;
  unsigned int	     eip;
  int		     i;

  if( DEBUG_GetCurrentFrame(&curr_func, &eip, &ebp) == FALSE )
    {
      return FALSE;
    }

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((eip - curr_func->addr.off) 
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((eip - curr_func->addr.off) 
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}

      if( strcmp(name, curr_func->local_vars[i].name) == 0 )
	{
	  /*
	   * OK, we found it.  Now figure out what to do with this.
	   */
	  if( curr_func->local_vars[i].regno != 0 )
	    {
	      /*
	       * Register variable.  We don't know how to treat
	       * this yet.
	       */
	      return FALSE;
	    }

	  addr->seg = 0;
	  addr->off = ebp + curr_func->local_vars[i].offset;
	  addr->type = curr_func->local_vars[i].type;

	  return TRUE;
	}
    }
  return FALSE;
}

int
DEBUG_InfoLocals()
{
  struct name_hash  * curr_func;
  unsigned int	      ebp;
  unsigned int	      eip;
  int		      i;
  unsigned int      * ptr;
  int		      rtn = FALSE;

  if( DEBUG_GetCurrentFrame(&curr_func, &eip, &ebp) == FALSE )
    {
      return FALSE;
    }

  for(i=0; i < curr_func->n_locals; i++ )
    {
      /*
       * Test the range of validity of the local variable.  This
       * comes up with RBRAC/LBRAC stabs in particular.
       */
      if(    (curr_func->local_vars[i].pc_start != 0)
	  && ((eip - curr_func->addr.off) 
	      < curr_func->local_vars[i].pc_start) )
	{
	  continue;
	}

      if(    (curr_func->local_vars[i].pc_end != 0)
	  && ((eip - curr_func->addr.off) 
	      > curr_func->local_vars[i].pc_end) )
	{
	  continue;
	}
      
      if( curr_func->local_vars[i].offset == 0 )
	{
	  fprintf(stderr, "%s:%s optimized into register $%s \n",
		  curr_func->name, curr_func->local_vars[i].name,
		  reg_name[curr_func->local_vars[i].regno]);
	}
      else
	{
	  ptr = (unsigned int *) (ebp + curr_func->local_vars[i].offset);
	  fprintf(stderr, "%s:%s == 0x%8.8x\n",
		  curr_func->name, curr_func->local_vars[i].name,
		  *ptr);
	}
    }

  rtn = TRUE;

  return (rtn);
}

int
DEBUG_SetSymbolSize(struct name_hash * sym, unsigned int len)
{
  sym->symbol_size = len;

  return TRUE;
}

int
DEBUG_SetSymbolBPOff(struct name_hash * sym, unsigned int off)
{
  sym->breakpoint_offset = off;

  return TRUE;
}

int
DEBUG_GetSymbolAddr(struct name_hash * sym, DBG_ADDR * addr)
{

  *addr = sym->addr;

  return TRUE;
}

int DEBUG_SetLocalSymbolType(struct wine_locals * sym, struct datatype * type)
{
  sym->type = type;

  return TRUE;
}
