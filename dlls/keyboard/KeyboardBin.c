
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define NOAPIPROTO
#include "windows.h"

#include "WinDefs.h"
#include "kerndef.h"
#include "Endian.h"
#include "Log.h"
#include "Kernel.h"
#include "Resources.h"
#include "DPMI.h"
#include "BinTypes.h"
#include "ModTable.h"

extern long int Trap();
extern long int AnsiToOem();
extern long int AnsiToOemBuff();
extern long int OemToAnsi();
extern long int OemToAnsiBuff();
extern long int GetKeyboardType();
extern long int VkKeyScan();
extern long int OemKeyScan();
extern long int MapVirtualKey();
extern long int GetKeyNameText();
extern long int ToAscii();
extern long int GetKBCodePage();

#if defined(TWIN_BIN16)
extern long int IT_1I();
extern long int IT_V();
extern long int IT_2LP();
extern long int IT_2LP1UI();
extern long int IT_1UI();
extern long int IT_2UI();
extern long int	IT_1D1LP1I();
extern long int	IT_TOASCII();

 /* Interface Segment Image KEYBOARD: */

static long int (*seg_image_KEYBOARD_0[])() =
{	/* nil */	0, 0,
	/* 001 */	Trap, 0,
	/* 002 */	Trap, 0,
	/* 003 */	Trap, 0,
	/* 004 */	IT_TOASCII, ToAscii,
	/* 005 */	IT_2LP,  AnsiToOem,
	/* 006 */	IT_2LP,  OemToAnsi,
	/* 007 */	Trap, 0,
	/* 008 */	Trap, 0,
	/* 009 */	Trap, 0,
	/* 00a */	Trap, 0,
	/* 00b */	Trap, 0,
	/* 00c */	Trap, 0,
	/* 00d */	Trap, 0,
	/* 00e */	Trap, 0,
	/* 00f */	Trap, 0,
	/* 010 */	Trap, 0,
	/* 011 */	Trap, 0,
	/* 012 */	Trap, 0,
	/* 013 */	Trap, 0,
	/* 014 */	Trap, 0,
	/* 015 */	Trap, 0,
	/* 016 */	Trap, 0,
	/* 017 */	Trap, 0,
	/* 018 */	Trap, 0,
	/* 019 */	Trap, 0,
	/* 01a */	Trap, 0,
	/* 01b */	Trap, 0,
	/* 01c */	Trap, 0,
	/* 01d */	Trap, 0,
	/* 01e */	Trap, 0,
	/* 01f */	Trap, 0,
	/* 020 */	Trap, 0,
	/* 021 */	Trap, 0,
	/* 022 */	Trap, 0,
	/* 023 */	Trap, 0,
	/* 024 */	Trap, 0,
	/* 025 */	Trap, 0,
	/* 026 */	Trap, 0,
	/* 027 */	Trap, 0,
	/* 028 */	Trap, 0,
	/* 029 */	Trap, 0,
	/* 02a */	Trap, 0,
	/* 02b */	Trap, 0,
	/* 02c */	Trap, 0,
	/* 02d */	Trap, 0,
	/* 02e */	Trap, 0,
	/* 02f */	Trap, 0,
	/* 030 */	Trap, 0,
	/* 031 */	Trap, 0,
	/* 032 */	Trap, 0,
	/* 033 */	Trap, 0,
	/* 034 */	Trap, 0,
	/* 035 */	Trap, 0,
	/* 036 */	Trap, 0,
	/* 037 */	Trap, 0,
	/* 038 */	Trap, 0,
	/* 039 */	Trap, 0,
	/* 03a */	Trap, 0,
	/* 03b */	Trap, 0,
	/* 03c */	Trap, 0,
	/* 03d */	Trap, 0,
	/* 03e */	Trap, 0,
	/* 03f */	Trap, 0,
	/* 040 */	Trap, 0,
	/* 041 */	Trap, 0,
	/* 042 */	Trap, 0,
	/* 043 */	Trap, 0,
	/* 044 */	Trap, 0,
	/* 045 */	Trap, 0,
	/* 046 */	Trap, 0,
	/* 047 */	Trap, 0,
	/* 048 */	Trap, 0,
	/* 049 */	Trap, 0,
	/* 04a */	Trap, 0,
	/* 04b */	Trap, 0,
	/* 04c */	Trap, 0,
	/* 04d */	Trap, 0,
	/* 04e */	Trap, 0,
	/* 04f */	Trap, 0,
	/* 050 */	Trap, 0,
	/* 051 */	Trap, 0,
	/* 052 */	Trap, 0,
	/* 053 */	Trap, 0,
	/* 054 */	Trap, 0,
	/* 055 */	Trap, 0,
	/* 056 */	Trap, 0,
	/* 057 */	Trap, 0,
	/* 058 */	Trap, 0,
	/* 059 */	Trap, 0,
	/* 05a */	Trap, 0,
	/* 05b */	Trap, 0,
	/* 05c */	Trap, 0,
	/* 05d */	Trap, 0,
	/* 05e */	Trap, 0,
	/* 05f */	Trap, 0,
	/* 060 */	Trap, 0,
	/* 061 */	Trap, 0,
	/* 062 */	Trap, 0,
	/* 063 */	Trap, 0,
	/* 064 */	Trap, 0,
	/* 065 */	Trap, 0,
	/* 066 */	Trap, 0,
	/* 067 */	Trap, 0,
	/* 068 */	Trap, 0,
	/* 069 */	Trap, 0,
	/* 06a */	Trap, 0,
	/* 06b */	Trap, 0,
	/* 06c */	Trap, 0,
	/* 06d */	Trap, 0,
	/* 06e */	Trap, 0,
	/* 06f */	Trap, 0,
	/* 070 */	Trap, 0,
	/* 071 */	Trap, 0,
	/* 072 */	Trap, 0,
	/* 073 */	Trap, 0,
	/* 074 */	Trap, 0,
	/* 075 */	Trap, 0,
	/* 076 */	Trap, 0,
	/* 077 */	Trap, 0,
	/* 078 */	Trap, 0,
	/* 079 */	Trap, 0,
	/* 07a */	Trap, 0,
	/* 07b */	Trap, 0,
	/* 07c */	Trap, 0,
	/* 07d */	Trap, 0,
	/* 07e */	Trap, 0,
	/* 07f */	Trap, 0,
	/* 080 */	IT_1UI, OemKeyScan,
	/* 081 */	IT_1UI, VkKeyScan,
	/* 082 */	IT_1I, GetKeyboardType,
	/* 083 */	IT_2UI, MapVirtualKey,
	/* 084 */	IT_V, GetKBCodePage,
	/* 085 */	IT_1D1LP1I, GetKeyNameText,
	/* 086 */	IT_2LP1UI, AnsiToOemBuff,
	/* 087 */	IT_2LP1UI, OemToAnsiBuff,
	0,0
};
#endif


 /* Segment Table KEYBOARD: */

SEGTAB SegmentTableKEYBOARD[] =
{	
#if defined(TWIN_BIN16)
	{ (char *) seg_image_KEYBOARD_0, 1080, TRANSFER_CALLBACK, 1080, 0, 0 },
#endif
	/* end */	{ 0, 0, 0, 0, 0, 0 }
};

