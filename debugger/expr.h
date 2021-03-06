/*************************************************************************
 * This source is derived from the Linux wine project. see www.winehq.com.
 * Bug fixes and patches to this file should be made in the original and then 
 * reflected into Twine.  Please see WINE in this distribution for more 
 * information.  This file is from wine-981129/debugger/expr.h
 ************************************************************************/

#define EXP_OP_LOR		0x01
#define EXP_OP_LAND		0x02
#define EXP_OP_OR		0x03
#define EXP_OP_AND		0x04
#define EXP_OP_XOR		0x05
#define EXP_OP_EQ		0x06
#define EXP_OP_GT		0x07
#define EXP_OP_LT		0x08
#define EXP_OP_GE		0x09
#define EXP_OP_LE		0x0a
#define EXP_OP_NE		0x0b
#define EXP_OP_SHL		0x0c
#define EXP_OP_SHR		0x0d
#define EXP_OP_ADD		0x0e
#define EXP_OP_SUB		0x0f
#define EXP_OP_MUL		0x10
#define EXP_OP_DIV		0x11
#define EXP_OP_REM		0x12
#define EXP_OP_NEG		0x13
#define EXP_OP_NOT		0x24
#define EXP_OP_LNOT		0x25
#define EXP_OP_DEREF		0x26
#define EXP_OP_ADDR		0x27
#define EXP_OP_ARR		0x28
#define EXP_OP_SEG		0x29
#define EXP_OP_FORCE_DEREF	0x2a
