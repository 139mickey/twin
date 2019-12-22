
typedef HANDLE HANDLE16;
typedef HANDLE HQUEU16;
typedef WORD UINT16;
typedef HINSTANCE HINSTANCE16;
typedef HMODULE HMODULE16;
typedef FARPROC FARPROC16;
typedef void *SEGPTR;

  /* signal proc typedef */
typedef void (CALLBACK *USERSIGNALPROC)(HANDLE16, UINT16, UINT16,HINSTANCE, HQUEUE16);

typedef struct
{
    WORD      int20;                   /* 00 int 20h instruction */
    WORD      nextParagraph;           /* 02 Segment of next paragraph */
    BYTE      reserved1;
    BYTE      dispatcher[5];           /* 05 Long call to DOS */
    FARPROC16 savedint22 ;  /* 0a Saved int 22h handler */
    FARPROC16 savedint23 ;  /* 0e Saved int 23h handler */
    FARPROC16 savedint24 ;  /* 12 Saved int 24h handler */
    WORD      parentPSP;               /* 16 Selector of parent PSP */
    BYTE      fileHandles[20];         /* 18 Open file handles */
    HANDLE16  environment;             /* 2c Selector of environment */
    WORD      reserved2[2];
    WORD      nbFiles;                 /* 32 Number of file handles */
    SEGPTR    fileHandlesPtr;          /* 34 Pointer to file handle table */
    HANDLE16  hFileHandles;            /* 38 Handle to fileHandlesPtr */
    WORD      reserved3[17];
    BYTE      fcb1[16];                /* 5c First FCB */
    BYTE      fcb2[20];                /* 6c Second FCB */
    BYTE      cmdLine[128];            /* 80 Command-line (first byte is len)*/
} PDB;


  /* Segment containing MakeProcInstance() thunks */
typedef struct
{
    WORD  next;       /* Selector of next segment */
    WORD  magic;      /* Thunks signature */
    WORD  unused;
    WORD  free;       /* Head of the free list */
    WORD  thunks[4];  /* Each thunk is 4 words long */
} THUNKS;

#define THUNK_MAGIC  ('P' | ('T' << 8))

  /* Task database. See 'Windows Internals' p. 226.
   * Note that 16-bit OLE 2 libs like to read it directly 
   * so we have to keep entry offsets as they are. 
   */
typedef struct _TDB
{
    HTASK16   hNext;                      /* 00 Selector of next TDB */
    DWORD     ss_sp ;          /* 02 Stack pointer of task */
    WORD      nEvents;                    /* 06 Events for this task */
    WORD     priority;                   /* 08 Task priority, -32..15 */
    WORD      unused1;                    /* 0a */
    HTASK16   hSelf;                      /* 0c Selector of this TDB */
    HANDLE16  hPrevInstance;              /* 0e Previous instance of module */
    DWORD     esp;                        /* 10 32-bit stack pointer */
    WORD      ctrlword8087;               /* 14 80x87 control word */
    WORD      flags;                      /* 16 Task flags */
    UINT16    error_mode;                 /* 18 Error mode (see SetErrorMode)*/
    WORD      version;                    /* 1a Expected Windows version */
    HANDLE16  hInstance;                  /* 1c Instance handle for task */
    HMODULE16 hModule;                    /* 1e Module handle */
    HQUEUE16  hQueue;                     /* 20 Selector of task queue */
    HTASK16   hParent;                    /* 22 Selector of TDB of parent */
    WORD      signal_flags;               /* 24 Flags for signal handler */
    FARPROC16 sighandler ;     /* 26 Signal handler */
    USERSIGNALPROC userhandler ; /* 2a USER signal handler */
    FARPROC16 discardhandler ; /* 2e Handler for GlobalNotify() */
    DWORD     int0 ;           /* 32 int 0 (divide by 0) handler */
    DWORD     int2 ;           /* 36 int 2 (NMI) handler */
    DWORD     int4 ;           /* 3a int 4 (INTO) handler */
    DWORD     int6 ;           /* 3e int 6 (invalid opc) handler */
    DWORD     int7 ;           /* 42 int 7 (coprocessor) handler */
    DWORD     int3e ;          /* 46 int 3e (80x87 emu) handler */
    DWORD     int75 ;          /* 4a int 75 (80x87 error) handler */
    DWORD     compat_flags ;   /* 4e Compatibility flags */
    BYTE      unused4[2];                 /* 52 */
    struct _THDB   *thdb;                 /* 54 Pointer to thread database */
    struct _WSINFO *pwsi;		  /* 58 Socket control struct */
    BYTE      unused5[4];                 /* 5B */
    HANDLE16  hPDB;                       /* 60 Selector of PDB (i.e. PSP) */
    SEGPTR    dta ;            		  /* 62 Current DTA */
    BYTE      curdrive;                   /* 66 Current drive */
    BYTE      curdir[65];                 /* 67 Current directory */
    WORD      nCmdShow;                   /* a8 cmdShow parameter to WinMain */
    HTASK16   hYieldTo;                   /* aa Next task to schedule */
    DWORD     dlls_to_init;               /* ac Ptr to DLLs to initialize */
    HANDLE16  hCSAlias;                   /* b0 Code segment for this TDB */
    THUNKS    thunks ;                    /* b2 Make proc instance thunks */
    WORD      more_thunks[6*4];           /* c2 Space for 6 more thunks */
    BYTE      module_name[8];             /* f2 Module name for task */
    WORD      magic;                      /* fa TDB signature */
    DWORD     unused7;                    /* fc */
    PDB       pdb;                        /* 100 PDB for this task */
} TDB;

#define TDB_MAGIC    ('T' | ('D' << 8))

  /* TDB flags */
#define TDBF_WINOLDAP   0x0001
#define TDBF_OS2APP     0x0008
#define TDBF_WIN32      0x0010

  /* USER signals */
#define USIG_TERMINATION	0x0020
#define USIG_DLL_LOAD		0x0040
#define USIG_DLL_UNLOAD		0x0080
#define USIG_GPF		0x0666

