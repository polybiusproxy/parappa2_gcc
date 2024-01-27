/* Opcode table for the DVP
   Copyright (c) 1998 Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ansidecl.h"
#include "libiberty.h"
#include "sysdep.h"
#include "dis-asm.h"
#include "opcode/dvp.h"
#include "opintl.h"

#include <ctype.h>

#ifndef NULL
#define NULL 0
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if defined (__STDC__) || defined (ALMOST_STDC)
#define XCONCAT2(a,b)	a##b
#else
#define XCONCAT2(a,b)	a/**/b
#endif
#define CONCAT2(a,b)	XCONCAT2(a,b)

typedef struct {
  int value;
  const char *name;
} keyword;

static int lookup_keyword_value PARAMS ((const keyword *, const char *, int));
static const char *lookup_keyword_name PARAMS ((const keyword *table, int));

static char *scan_symbol PARAMS ((char *));

/* Return non-zero if CH is a character that may appear in a symbol.  */
/* FIXME: This will need revisiting.  */
#define issymchar(ch) (isalnum ((unsigned char) ch) || ch == '_')

#define SKIP_BLANKS(var) while (isspace ((unsigned char) *(var))) ++(var)

/* ??? One can argue it's preferable to have the PARSE_FN support in tc-dvp.c
   and the PRINT_FN support in dvp-dis.c.  For this project I like having
   them all in one place.  */

#define PARSE_FN(fn) \
static long CONCAT2 (parse_,fn) \
     PARAMS ((const dvp_opcode *, const dvp_operand *, int, char **, \
	      const char **));
#define INSERT_FN(fn) \
static void CONCAT2 (insert_,fn) \
     PARAMS ((const dvp_opcode *, const dvp_operand *, int, DVP_INSN *, \
	      long, const char **))
#define EXTRACT_FN(fn) \
static long CONCAT2 (extract_,fn) \
     PARAMS ((const dvp_opcode *, const dvp_operand *, int, DVP_INSN *, \
	      int *));
#define PRINT_FN(fn) \
static void CONCAT2 (print_,fn) \
     PARAMS ((const dvp_opcode *, const dvp_operand *, int, DVP_INSN *, \
	      disassemble_info *, long));

PARSE_FN (dotdest);
INSERT_FN (dotdest);
EXTRACT_FN (dotdest);
PRINT_FN (dotdest);

PARSE_FN (dotdest1);
PARSE_FN (dest1);
PRINT_FN (dest1);

PARSE_FN (uflags);
PRINT_FN (uflags);

PARSE_FN (bc);
EXTRACT_FN (bc);
PRINT_FN (sdest);

PARSE_FN (vfreg);
PRINT_FN (vfreg);

PARSE_FN (bcftreg);
PRINT_FN (bcftreg);

PARSE_FN (accdest);
PRINT_FN (accdest);

INSERT_FN (xyz);
INSERT_FN (w);

PARSE_FN (ireg);
PRINT_FN (ireg);

PARSE_FN (freg);
PRINT_FN (freg);

PARSE_FN (ffstreg);
INSERT_FN (ffstreg);
EXTRACT_FN (ffstreg);
PRINT_FN (ffstreg);

PARSE_FN (vi01);
PRINT_FN (vi01);

INSERT_FN (luimm12);
EXTRACT_FN (luimm12);

INSERT_FN (luimm12up6);

INSERT_FN (luimm15);
EXTRACT_FN (luimm15);

/* Various types of DVP operands, including insn suffixes.

   Fields are:

   BITS SHIFT FLAGS PARSE_FN INSERT_FN EXTRACT_FN PRINT_FN

   Operand values are 128 + table index.  This allows ASCII chars to be
   included in the syntax spec.  */

const dvp_operand vu_operands[] =
{
  /* place holder (??? not sure if needed) */
#define UNUSED 128
  { 0 },

  /* Operands that exist in the same place for essentially the same purpose
     in both upper and lower instructions.  These don't have a U or L prefix.
     Operands specific to the upper or lower instruction are so prefixed.  */

  /* Destination indicator attached to mnemonic, with leading '.' or '/'.
     After parsing this, the value is stored in global `state_vu_mnemonic_dest'
     so that the register parser can verify the same choice of xyzw is
     used.  */
#define DOTDEST (UNUSED + 1)
  { 4, VU_SHIFT_DEST, 0, DVP_OPERAND_SUFFIX,
      parse_dotdest, insert_dotdest, extract_dotdest, print_dotdest },

  /* ft reg, with vector specification same as DOTDEST */
#define VFTREG (DOTDEST + 1)
  { 5, VU_SHIFT_TREG, 0, 0, parse_vfreg, 0, 0, print_vfreg },

  /* fs reg, with vector specification same as DOTDEST */
#define VFSREG (VFTREG + 1)
  { 5, VU_SHIFT_SREG, 0, 0, parse_vfreg, 0, 0, print_vfreg },

  /* fd reg, with vector specification same as DOTDEST */
#define VFDREG (VFSREG + 1)
  { 5, VU_SHIFT_DREG, 0, 0, parse_vfreg, 0, 0, print_vfreg },

  /* Upper word operands.  */

  /* flag bits */
#define UFLAGS (VFDREG + 1)
  { 5, 27, 0, DVP_OPERAND_SUFFIX, parse_uflags, 0, 0, print_uflags },

  /* broadcast */
#define UBC (UFLAGS + 1)
  { 2, 0, 0, DVP_OPERAND_SUFFIX, parse_bc, 0, extract_bc, print_sdest },

  /* ftreg in broadcast case */
#define UBCFTREG (UBC + 1)
  { 5, VU_SHIFT_TREG, 0, 0, parse_bcftreg, 0, 0, print_bcftreg },

  /* accumulator dest */
#define UACCDEST (UBCFTREG + 1)
  { 0, 0, 0, 0, parse_accdest, 0, 0, print_accdest },

  /* The XYZ operand is a fake one that is used to ensure only "xyz" is
     specified.  It simplifies the opmula and opmsub entries.  */
#define UXYZ (UACCDEST + 1)
  { 0, 0, 0, DVP_OPERAND_FAKE, 0, insert_xyz, 0, 0 },

  /* The W operand is a fake one that is used to ensure only "w" is
     specified.  It simplifies the clipw entry.  */
#define UW (UXYZ + 1)
  { 0, 0, 0, DVP_OPERAND_FAKE, 0, insert_w, 0, 0 },

  /* Lower word operands.  */

  /* 5 bit signed immediate.  */
#define LIMM5 (UW + 1)
  { 5, 6, 0, DVP_OPERAND_SIGNED, 0, 0, 0, 0 },

  /* 11 bit signed immediate.  */
#define LIMM11 (LIMM5 + 1)
  { 11, 0, 0, DVP_OPERAND_SIGNED, 0, 0, 0, 0 },
  /* WAS: { 11, 0, 0, DVP_OPERAND_RELOC_11_S4, 0, 0, 0, 0 },*/

  /* 15 bit unsigned immediate.  */
#define LUIMM15 (LIMM11 + 1)
  { 15, 0, 0, DVP_OPERAND_RELOC_U15_S3, 0, insert_luimm15, extract_luimm15, 0 },

  /* ID register.  */
#define LIDREG (LUIMM15 + 1)
  { 5, 6, 0, 0, parse_ireg, 0, 0, print_ireg },

  /* IS register.  */
#define LISREG (LIDREG + 1)
  { 5, 11, 0, 0, parse_ireg, 0, 0, print_ireg },

  /* IT register.  */
#define LITREG (LISREG + 1)
  { 5, 16, 0, 0, parse_ireg, 0, 0, print_ireg },

  /* FS reg, with FSF field selector.  */
#define LFSFFSREG (LITREG + 1)
  { 5, 11, 0, 0, parse_ffstreg, insert_ffstreg, extract_ffstreg, print_ffstreg },

  /* FS reg, no selector (choice of x,y,z,w is provided by opcode).  */
#define LFSREG (LFSFFSREG + 1)
  { 5, 11, 0, 0, parse_freg, 0, 0, print_freg },

  /* FT reg, with FTF field selector.  */
#define LFTFFTREG (LFSREG + 1)
  { 5, 16, 0, 0, parse_ffstreg, insert_ffstreg, extract_ffstreg, print_ffstreg },

  /* VI01 register.  */
#define LVI01 (LFTFFTREG + 1)
  { 0, 0, 0, 0, parse_vi01, 0, 0, print_vi01 },

  /* 24 bit unsigned immediate.  */
#define LUIMM24 (LVI01 + 1)
  { 24, 0, 0, 0, 0, 0, 0, 0 },

  /* 12 bit unsigned immediate, split into 1 and 11 bit pieces.  */
#define LUIMM12 (LUIMM24 + 1)
  { 12, 0, 0, 0, 0, insert_luimm12, extract_luimm12, 0 },

  /* upper 6 bits of 12 bit unsigned immediate */
#define LUIMM12UP6 (LUIMM12 + 1)
  { 12, 0, 0, 0, 0, insert_luimm12up6, extract_luimm12, 0 },

  /* 11 bit pc-relative signed immediate.  */
#define LPCREL11 (LUIMM12UP6 + 1)
  { 11, 0, 0, DVP_OPERAND_SIGNED + DVP_OPERAND_RELATIVE_BRANCH, 0, 0, 0, 0 },

  /* Destination indicator, single letter only, with leading '.' or '/'.  */
#define LDOTDEST1 (LPCREL11 + 1)
  { 4, VU_SHIFT_DEST, 0, DVP_OPERAND_SUFFIX,
      /* Note that we borrow the insert/extract/print functions from the
	 vector case.  */
      parse_dotdest1, insert_dotdest, extract_dotdest, print_dotdest },

  /* Destination indicator, single letter only, no leading '.'.  */
  /* ??? Making this FAKE is a workaround to a limitation in the parser.
     It can't handle operands with a legitimate value of "".  */
#define LDEST1 (LDOTDEST1 + 1)
  { 0, 0, 0, DVP_OPERAND_FAKE, parse_dest1, 0, 0, print_dest1 },

  /* 32 bit floating point immediate.  */
#define LFIMM32 (LDEST1 + 1)
  { 32, 0, 0, DVP_OPERAND_FLOAT, 0, 0, 0, 0 },
/* end of list place holder */
  { 0 }
};

/* Macros to put a field's value into the right place.  */
/* ??? If assembler needs these, move to opcode/dvp.h.  */

/* value X, B bits, shift S */
#define V(x,b,s) (((x) & ((1 << (b)) - 1)) << (s))

/* Field value macros for both upper and lower instructions.
   These shift a value into the right place in the instruction.  */

/* [FI] T reg field (remember it's V for value, not vector, here).  */
#define VT(x) V ((x), 5, VU_SHIFT_TREG)
/* [FI] S reg field.  */
#define VS(x) V ((x), 5, VU_SHIFT_SREG)
/* [FI] D reg field.  */
#define VD(x) V ((x), 5, VU_SHIFT_DREG)
/* DEST field.  */
#define VDEST(x) V ((x), 4, 21)

/* Masks for fields in both upper and lower instructions.
   These mask out all bits but the ones for the field in the instruction.  */

#define MT VT (~0)
#define MS VS (~0)
#define MD VD (~0)
#define MDEST VDEST (~0)

/* Upper instruction Value macros.  */

/* Upper Flag bits.  */
#define VUF(x) V ((x), 5, 27)
/* Upper REServed two bits next to flag bits.  */
#define VURES(x) V ((x), 2, 25)
/* 4 bit opcode field.  */
#define VUOP4(x) V ((x), 4, 2)
/* 6 bit opcode field.  */
#define VUOP6(x) V ((x), 6, 0)
/* 9 bit opcode field.  */
#define VUOP9(x) V ((x), 9, 2)
/* 11 bit opcode field.  */
#define VUOP11(x) V ((x), 11, 0)
/* BroadCast field.  */
#define VUBC(x) V ((x), 2, 0)

/* Upper instruction field masks.  */
#define MURES VURES (~0)
#define MUOP4 VUOP4 (~0)
#define MUOP6 VUOP6 (~0)
#define MUOP9 VUOP9 (~0)
#define MUOP11 VUOP11 (~0)

/* A space, separates instruction name (mnemonic + mnemonic operands) from
   operands.  */
#define SP ' '
/* Commas separate operands.  */
#define C ','
/* Special I,P,Q,R operands.  */
#define I 'i'
#define P 'p'
#define Q 'q'
#define R 'r'

/* VU instructions.
   [??? some of these comments are left over from the ARC port from which
   this code is borrowed, delete in time]

   Longer versions of insns must appear before shorter ones (if gas sees
   "lsr r2,r3,1" when it's parsing "lsr %a,%b" it will think the ",1" is
   junk).  This isn't necessary for `ld' because of the trailing ']'.

   Instructions that are really macros based on other insns must appear
   before the real insn so they're chosen when disassembling.  Eg: The `mov'
   insn is really the `and' insn.

   This table is best viewed on a wide screen (161 columns).  I'd prefer to
   keep it this way.  The rest of the file, however, should be viewable on an
   80 column terminal.  */

/* ??? This table also includes macros: asl, lsl, and mov.  The ppc port has
   a more general facility for dealing with macros which could be used if
   we need to.  */

/* These tables can't be `const' because members `next_asm' and `next_dis' are
   computed at run-time.  We could split this into two, as that would put the
   constant stuff into a readonly section.  */

struct dvp_opcode vu_upper_opcodes[] =
{
  /* Macros appear first, so the disassembler will try them first.  */
  /* ??? Any aliases?  */

  /* The rest of these needn't be sorted, but it helps to find them if they are.  */
  { "abs",    { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x1fd) },
  { "add",    { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x28) },
  { "addi",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x22) },
  { "addq",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, Q },             MURES + MT + MUOP6,  VUOP6 (0x20) },
  { "add",    { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + VUOP4 (~0),  VUOP4 (0) },
  { "adda",   { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG },      MURES + MUOP11,      VUOP11 (0x2bc) },
  { "addai",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, I },           MURES + MT + MUOP11, VUOP11 (0x23e) },
  { "addaq",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, Q },           MURES + MT + MUOP11, VUOP11 (0x23c) },
  { "adda",   { UBC, UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, UBCFTREG }, MURES + MUOP9,     VUOP9 (0xf) },
  { "clip",   { UBC, UFLAGS, DOTDEST, SP, VFSREG, C, UBCFTREG, UW },        MURES + MDEST + MUOP11, VDEST (0xe) + VUOP11 (0x1ff) },
  { "ftoi0",  { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x17c) },
  { "ftoi4",  { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x17d) },
  { "ftoi12", { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x17e) },
  { "ftoi15", { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x17f) },
  { "itof0",  { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x13c) },
  { "itof4",  { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x13d) },
  { "itof12", { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x13e) },
  { "itof15", { UFLAGS, DOTDEST, SP, VFTREG, C, VFSREG },                   MURES + MUOP11,      VUOP11 (0x13f) },
  { "madd",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x29) },
  { "maddi",  { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x23) },
  { "maddq",  { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, Q },             MURES + MT + MUOP6,  VUOP6 (0x21) },
  { "madd",   { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + MUOP4,       VUOP4 (0x2) },
  { "madda",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG },      MURES + MUOP11,      VUOP11 (0x2bd) },
  { "maddai", { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, I },           MURES + MT + MUOP11, VUOP11 (0x23f) },
  { "maddaq", { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, Q },           MURES + MT + MUOP11, VUOP11 (0x23d) },
  { "madda",  { UBC, UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, UBCFTREG }, MURES + MUOP9,     VUOP9 (0x2f) },
  { "max",    { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x2b) },
  { "maxi",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x1d) },
  { "max",    { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + MUOP4,       VUOP4 (0x4) },
  { "mini",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x2f) },
  { "minii",  { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x1f) },
  { "mini",   { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + MUOP4,       VUOP4 (0x5) },
  { "msub",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x2d) },
  { "msubi",  { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x27) },
  { "msubq",  { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, Q },             MURES + MT + MUOP6,  VUOP6 (0x25) },
  { "msub",   { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + MUOP4,       VUOP4 (0x3) },
  { "msuba",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG },      MURES + MUOP11,      VUOP11 (0x2fd) },
  { "msubai", { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, I },           MURES + MT + MUOP11, VUOP11 (0x27f) },
  { "msubaq", { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, Q },           MURES + MT + MUOP11, VUOP11 (0x27d) },
  { "msuba",  { UBC, UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, UBCFTREG }, MURES + MUOP9,     VUOP9 (0x3f) },
  { "mul",    { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x2a) },
  { "muli",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x1e) },
  { "mulq",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, Q },             MURES + MT + MUOP6,  VUOP6 (0x1c) },
  { "mul",    { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + VUOP4 (~0),  VUOP4 (6) },
  { "mula",   { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG },      MURES + MUOP11,      VUOP11 (0x2be) },
  { "mulai",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, I },           MURES + MT + MUOP11, VUOP11 (0x1fe) },
  { "mulaq",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, Q },           MURES + MT + MUOP11, VUOP11 (0x1fc) },
  { "mula",   { UBC, UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, UBCFTREG }, MURES + MUOP9,     VUOP9 (0x6f) },
  { "nop",    { UFLAGS },                                                   MURES + MDEST + MT + MS + MUOP11, VUOP11 (0x2ff) },
  { "opmula", { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG, UXYZ }, MURES + MUOP11,     VUOP11 (0x2fe) },
  { "opmsub", { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG, UXYZ },  MURES + MUOP6,       VUOP6 (0x2e) },
  { "sub",    { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, VFTREG },        MURES + MUOP6,       VUOP6 (0x2c) },
  { "subi",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, I },             MURES + MT + MUOP6,  VUOP6 (0x26) },
  { "subq",   { UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, Q },             MURES + MT + MUOP6,  VUOP6 (0x24) },
  { "sub",    { UBC, UFLAGS, DOTDEST, SP, VFDREG, C, VFSREG, C, UBCFTREG }, MURES + VUOP4 (~0),  VUOP4 (1) },
  { "suba",   { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, VFTREG },      MURES + MUOP11,      VUOP11 (0x2fc) },
  { "subai",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, I },           MURES + MT + MUOP11, VUOP11 (0x27e) },
  { "subaq",  { UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, Q },           MURES + MT + MUOP11, VUOP11 (0x27c) },
  { "suba",   { UBC, UFLAGS, DOTDEST, SP, UACCDEST, C, VFSREG, C, UBCFTREG }, MURES + MUOP9,     VUOP9 (0x1f) }
};
const int vu_upper_opcodes_count = sizeof (vu_upper_opcodes) / sizeof (vu_upper_opcodes[0]);

/* Lower instruction Value macros.  */

/* 6 bit opcode.  */
#define VLOP6(x) V ((x), 6, 0)
/* 7 bit opcode.  */
#define VLOP7(x) V ((x), 7, 25)
/* 11 bit opcode.  */
#define VLOP11(x) V ((x), 11, 0)
/* 11 bit immediate.  */
#define VLIMM11(x) V ((x), 11, 0)
/* FTF field.  */
#define VLFTF(x) V ((x), 2, 23)
/* FSF field.  */
#define VLFSF(x) V ((x), 2, 21)
/* Upper bit of 12 bit unsigned immediate.  */
#define VLUIMM12TOP(x) V ((x), 1, 21)
/* Upper 4 bits of 15 bit unsigned immediate.  */
#define VLUIMM15TOP(x) VDEST (x)

/* Lower instruction field masks.  */
#define MLOP6 VLOP6 (~0)
#define MLOP7 VLOP7 (~0)
#define MLOP11 VLOP11 (~0)
#define MLIMM11 VLIMM11 (~0)
#define MLB24 V (1, 1, 24)
#define MLUIMM12TOP VLUIMM12TOP (~0)
/* 12 bit unsigned immediates are split into two parts, 1 bit and 11 bits.
   The upper 1 bit is part of the `dest' field.  This mask is for the
   other 3 bits of the dest field.  */
#define MLUIMM12UNUSED V (7, 3, 22)
#define MLUIMM15TOP MDEST

struct dvp_opcode vu_lower_opcodes[] =
{
  /* Macros appear first, so the disassembler will try them first.  */

  /* There isn't an explicit nop.  Apparently it's "move vf0,vf0".  */
  { "nop", { 0 }, 0xffffffff, VLOP7 (0x40) + VLIMM11 (0x33c) },

  /* The rest of these needn't be sorted, but it helps to find them if they are.  */
  { "b",       { SP, LPCREL11 },                      MLOP7 + MDEST + MT + MS,          VLOP7 (0x20) },
  { "bal",     { SP, LITREG, C, LPCREL11 },           MLOP7 + MDEST + MS,               VLOP7 (0x21) },
  { "div",     { SP, Q, C, LFSFFSREG, C, LFTFFTREG }, MLOP7 + MLOP11,                   VLOP7 (0x40) + VLOP11 (0x3bc) },
  { "eatan",   { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7fd) },
  { "eatanxy", { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xc) + VLOP11 (0x77c) },
  { "eatanxz", { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xa) + VLOP11 (0x77d) },
  { "eexp",    { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7fe) },
  { "eleng",   { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xe) + VLOP11 (0x73e) },
  { "ercpr",   { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7be) },
  { "erleng",  { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xe) + VLOP11 (0x73f) },
  { "ersadd",  { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xe) + VLOP11 (0x73d) },
  { "ersqrt",  { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7bd) },
  { "esadd",   { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xe) + VLOP11 (0x73c) },
  { "esin",    { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7fc) },
  { "esqrt",   { SP, P, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLOP11, VLOP7 (0x40) + VLOP11 (0x7bc) },
  { "esum",    { SP, P, C, LFSREG },                  MLOP7 + MDEST + MT + MLOP11,      VLOP7 (0x40) + VDEST (0xf) + VLOP11 (0x77e) },
  { "fcand",   { SP, LVI01, C, LUIMM24 },             MLOP7 + MLB24,                    VLOP7 (0x12) },
  { "fceq",    { SP, LVI01, C, LUIMM24 },             MLOP7 + MLB24,                    VLOP7 (0x10) },
  { "fcget",   { SP, LITREG },                        MLOP7 + MDEST + MS + MLIMM11,     VLOP7 (0x1c) },
  { "fcor",    { SP, LVI01, C, LUIMM24 },             MLOP7 + MLB24,                    VLOP7 (0x13) },
  { "fcset",   { SP, LUIMM24 },                       MLOP7 + MLB24,                    VLOP7 (0x11) },
  { "fmand",   { SP, LITREG, C, LISREG },             MLOP7 + MDEST + MLIMM11,          VLOP7 (0x1a) },
  { "fmeq",    { SP, LITREG, C, LISREG },             MLOP7 + MDEST + MLIMM11,          VLOP7 (0x18) },
  { "fmor",    { SP, LITREG, C, LISREG },             MLOP7 + MDEST + MLIMM11,          VLOP7 (0x1b) },
  { "fsand",   { SP, LITREG, C, LUIMM12 },            MLOP7 + MLUIMM12UNUSED + MS,      VLOP7 (0x16) },
  { "fseq",    { SP, LITREG, C, LUIMM12 },            MLOP7 + MLUIMM12UNUSED + MS,      VLOP7 (0x14) },
  { "fsor",    { SP, LITREG, C, LUIMM12 },            MLOP7 + MLUIMM12UNUSED + MS,      VLOP7 (0x17) },
  { "fsset",   { SP, LUIMM12UP6 },                    MLOP7 + MLUIMM12UNUSED + V (~0, 6, 0) + MS + MT, VLOP7 (0x15) },
  { "iadd",    { SP, LIDREG, C, LISREG, C, LITREG },  MLOP7 + MDEST + MLOP6,            VLOP7 (0x40) + VLOP6 (0x30) },
  { "iaddi",   { SP, LITREG, C, LISREG, C, LIMM5 },   MLOP7 + MDEST + MLOP6,            VLOP7 (0x40) + VLOP6 (0x32) },
  { "iaddiu",  { SP, LITREG, C, LISREG, C, LUIMM15 }, MLOP7,                            VLOP7 (0x08) },
  { "iand",    { SP, LIDREG, C, LISREG, C, LITREG },  MLOP7 + MDEST + MLOP6,            VLOP7 (0x40) + VLOP6 (0x34) },
  { "ibeq",    { SP, LITREG, C, LISREG, C, LPCREL11 }, MLOP7 + MDEST,                   VLOP7 (0x28) },
  { "ibgez",   { SP, LISREG, C, LPCREL11 },           MLOP7 + MDEST + MT,               VLOP7 (0x2f) },
  { "ibgtz",   { SP, LISREG, C, LPCREL11 },           MLOP7 + MDEST + MT,               VLOP7 (0x2d) },
  { "iblez",   { SP, LISREG, C, LPCREL11 },           MLOP7 + MDEST + MT,               VLOP7 (0x2e) },
  { "ibltz",   { SP, LISREG, C, LPCREL11 },           MLOP7 + MDEST + MT,               VLOP7 (0x2c) },
  { "ibne",    { SP, LITREG, C, LISREG, C, LPCREL11 }, MLOP7 + MDEST,                   VLOP7 (0x29) },
  { "ilw",     { LDOTDEST1, SP, LITREG, C, LIMM11, '(', LISREG, ')', LDEST1 }, MLOP7,   VLOP7 (0x04) },
  { "ilwr",    { LDOTDEST1, SP, LITREG, C, '(', LISREG, ')', LDEST1 }, MLOP7 + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x3fe) },
  { "ior",     { SP, LIDREG, C, LISREG, C, LITREG },  MLOP7 + MDEST + MLOP6,            VLOP7 (0x40) + VLOP6 (0x35) },
  { "isub",    { SP, LIDREG, C, LISREG, C, LITREG },  MLOP7 + MDEST + MLOP6,            VLOP7 (0x40) + VLOP6 (0x31) },
  { "isubiu",  { SP, LITREG, C, LISREG, C, LUIMM15 }, MLOP7,                            VLOP7 (0x09) },
  { "isw",     { DOTDEST, SP, LITREG, C, LIMM11, '(', LISREG, ')', LDEST1 }, MLOP7,     VLOP7 (0x05) },
  { "iswr",    { DOTDEST, SP, LITREG, C, '(', LISREG, ')', LDEST1 }, MLOP7 + MLIMM11,   VLOP7 (0x40) + VLIMM11 (0x3ff) },
  { "jalr",    { SP, LITREG, C, LISREG },             MLOP7 + MDEST + MLIMM11,          VLOP7 (0x25) },
  { "jr",      { SP, LISREG },                        MLOP7 + MDEST + MT + MLIMM11,     VLOP7 (0x24) },
  { "loi",     { SP, LFIMM32 },                       0,                                0 },
  { "lq",      { DOTDEST, SP, VFTREG, C, LIMM11, '(', LISREG, ')' }, MLOP7,             VLOP7 (0x00) },
  { "lqd",     { DOTDEST, SP, VFTREG, C, '(', '-', '-', LISREG, ')' }, MLOP7 + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x37e) },
  { "lqi",     { DOTDEST, SP, VFTREG, C, '(', LISREG, '+', '+', ')' }, MLOP7 + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x37c) },
  /* Only a single VF reg is allowed here.  We can use VFTREG because LDOTDEST1
     handles verifying only a single choice of xyzw is present.  */
  { "mfir",    { DOTDEST, SP, VFTREG, C, LISREG },    MLOP7 + MLIMM11,                  VLOP7 (0x40) + VLIMM11 (0x3fd) },
  { "mfp",     { DOTDEST, SP, VFTREG, C, P },         MLOP7 + MS + MLIMM11,             VLOP7 (0x40) + VLIMM11 (0x67c) },
  { "move",    { DOTDEST, SP, VFTREG, C, VFSREG },    MLOP7 + MLIMM11,                  VLOP7 (0x40) + VLIMM11 (0x33c) },
  { "mr32",    { DOTDEST, SP, VFTREG, C, VFSREG },    MLOP7 + MLIMM11,                  VLOP7 (0x40) + VLIMM11 (0x33d) },
  { "mtir",    { SP, LITREG, C, LFSFFSREG },          MLOP7 + VLFTF (~0) + MLOP11,      VLOP7 (0x40) + VLOP11 (0x3fc) },
  { "rget",    { DOTDEST, SP, VFTREG, C, R },         MLOP7 + MS + MLIMM11,             VLOP7 (0x40) + VLIMM11 (0x43d) },
  { "rinit",   { SP, R, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x43e) },
  { "rnext",   { DOTDEST, SP, VFTREG, C, R },         MLOP7 + MS + MLIMM11,             VLOP7 (0x40) + VLIMM11 (0x43c) },
  { "rsqrt",   { SP, Q, C, LFSFFSREG, C, LFTFFTREG }, MLOP7 + MLIMM11,                  VLOP7 (0x40) + VLIMM11 (0x3be) },
  { "rxor",    { SP, R, C, LFSFFSREG },               MLOP7 + VLFTF (~0) + MT + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x43f) },
  { "sq",      { DOTDEST, SP, VFSREG, C, LIMM11, '(', LITREG, ')' }, MLOP7,             VLOP7 (0x01) },
  { "sqd",     { DOTDEST, SP, VFSREG, C, '(', '-', '-', LITREG, ')' }, MLOP7 + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x37f) },
  { "sqi",     { DOTDEST, SP, VFSREG, C, '(', LITREG, '+', '+', ')' }, MLOP7 + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x37d) },
  { "sqrt",    { SP, Q, C, LFTFFTREG },               MLOP7 + VLFSF (~0) + MS + MLIMM11, VLOP7 (0x40) + VLIMM11 (0x3bd) },
  { "waitp",   { 0 },                                 0xffffffff,                       VLOP7 (0x40) + VLIMM11 (0x7bf) },
  { "waitq",   { 0 },                                 0xffffffff,                       VLOP7 (0x40) + VLIMM11 (0x3bf) },
  { "xgkick",  { SP, LISREG },                        MLOP7 + MDEST + MT + MLIMM11,     VLOP7 (0x40) + VLIMM11 (0x6fc) },
  { "xitop",   { SP, LITREG },                        MLOP7 + MDEST + MS + MLIMM11,     VLOP7 (0x40) + VLIMM11 (0x6bd) },
  { "xtop",    { SP, LITREG },                        MLOP7 + MDEST + MS + MLIMM11,     VLOP7 (0x40) + VLIMM11 (0x6bc) }
};
const int vu_lower_opcodes_count = sizeof (vu_lower_opcodes) / sizeof (vu_lower_opcodes[0]);

/* Value of DEST in use.
   Each of the registers must specify the same value as the opcode.  */
static int state_vu_mnemonic_dest;

/* Value of BC to use.
   The register specified for the ftreg must match the broadcast register
   specified in the opcode.  */
static int state_vu_mnemonic_bc;

#define INVALID_DEST 		_("invalid `dest'")
#define MISSING_DEST 		_("missing `dest'")
#define MISSING_DOT  		_("missing `.'")
#define UNKNOWN_REGISTER 	_("unknown register")
#define INVALID_REGISTER_NUMBER _("invalid register number")

/* Multiple destination choice support.
   The "dest" string selects any combination of x,y,z,w.
   [The letters are ordered that way to follow the manual's style.]  */

/* Utility to parse a `dest' spec.
   Return the found value or zero if not present.
   *PSTR is set to the character that terminated the parsing.
   It is up to the caller to do any error checking.  */

static long
u_parse_dest (pstr)
     char **pstr;
{
  long dest = 0;

  while (**pstr)
    {
      switch (**pstr)
	{
	case 'x' : case 'X' : dest |= VU_DEST_X; break;
	case 'y' : case 'Y' : dest |= VU_DEST_Y; break;
	case 'z' : case 'Z' : dest |= VU_DEST_Z; break;
	case 'w' : case 'W' : dest |= VU_DEST_W; break;
	default : return dest;
	}
      ++*pstr;
    }

  return dest;
}

static long
parse_dotdest (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  long dest;

  /* If we're at a space, the dest isn't present so use the default: xyzw.  */
  if (**pstr == ' ')
    return VU_DEST_X | VU_DEST_Y | VU_DEST_Z | VU_DEST_W;

  if (**pstr != '.' && **pstr != '/')
    {
      *errmsg = MISSING_DOT;
      return 0;
    }

  ++*pstr;
  dest = u_parse_dest (pstr);
  if (dest == 0 || isalnum ((unsigned char) **pstr))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }

  return dest;
}

/* Parse a `dest' spec where only a single letter is allowed,
   but the encoding handles all four.  */

static long
parse_dotdest1 (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char c;
  long dest;

  if (**pstr != '.' && **pstr != '/')
    {
      *errmsg = MISSING_DOT;
      return 0;
    }

  ++*pstr;
  switch (**pstr)
    {
    case 'x' : case 'X' : dest = VU_DEST_X; break;
    case 'y' : case 'Y' : dest = VU_DEST_Y; break;
    case 'z' : case 'Z' : dest = VU_DEST_Z; break;
    case 'w' : case 'W' : dest = VU_DEST_W; break;
    default : *errmsg = INVALID_DEST; return 0;
    }
  ++*pstr;
  c = tolower (**pstr);
  if (c == 'x' || c == 'y' || c == 'z' || c == 'w')
    {
      *errmsg = _("only one of x,y,z,w can be specified");
      return 0;
    }
  if (isalnum ((unsigned char) **pstr))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }

  return dest;
}

/* Parse a `dest' spec with no leading '.', where only a single letter is
   allowed, but the encoding handles all four.  The value, if specified,
   must match that recorded in `dest'.  */

static long
parse_dest1 (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  long dest;

  dest = u_parse_dest (pstr);
  if (dest == 0)
    ; /* not specified, nothing to do */
  else if (dest != VU_DEST_X
      && dest != VU_DEST_Y
      && dest != VU_DEST_Z
      && dest != VU_DEST_W)
    {
      *errmsg = _("expecting one of x,y,z,w");
      return 0;
    }
  else if (dest != state_vu_mnemonic_dest)
    {
      *errmsg = _("`dest' suffix does not match instruction `dest'");
      return 0;
    }

  return dest;
}

static void
insert_dotdest (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  /* Record the DEST value in use so the register parser can use it.  */
  state_vu_mnemonic_dest = value;
  *insn |= value << operand->shift;
}

static long
extract_dotdest (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  /* Record the DEST value in use so the register printer can use it.  */
  state_vu_mnemonic_dest = (*insn >> operand->shift) & ((1 << operand->bits) - 1);
  return state_vu_mnemonic_dest;
}

/* Utility to print a multiple dest spec.  */

static void
u_print_dest (info, insn, value)
     disassemble_info *info;
     DVP_INSN *insn;
     long value;
{
  if (value & VU_DEST_X)
    (*info->fprintf_func) (info->stream, "x");
  if (value & VU_DEST_Y)
    (*info->fprintf_func) (info->stream, "y");
  if (value & VU_DEST_Z)
    (*info->fprintf_func) (info->stream, "z");
  if (value & VU_DEST_W)
    (*info->fprintf_func) (info->stream, "w");
}

static void
print_dotdest (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, ".");
  u_print_dest (info, insn, value);
}

static void
print_dest1 (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  u_print_dest (info, insn, state_vu_mnemonic_dest);
}

/* Utilities for single destination choice handling.  */

/* Parse a single dest spec.
   Return one of VU_SDEST_[XYZW] or -1 if not present.  */

static long
u_parse_sdest (pstr, errmsg)
     char **pstr;
     const char **errmsg;
{
  char c;
  long dest = 0;

  switch (**pstr)
    {
    case 'x' : case 'X' : dest = VU_SDEST_X; break;
    case 'y' : case 'Y' : dest = VU_SDEST_Y; break;
    case 'z' : case 'Z' : dest = VU_SDEST_Z; break;
    case 'w' : case 'W' : dest = VU_SDEST_W; break;
    default : return -1;
    }
  ++*pstr;
  c = tolower (**pstr);
  if (c == 'x' || c == 'y' || c == 'z' || c == 'w')
    {
      *errmsg = _("only one of x,y,z,w can be specified");
      return 0;
    }
  if (isalnum ((unsigned char) **pstr))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }

  return dest;
}

static void
print_sdest (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  char c;

  switch (value)
    {
    case VU_SDEST_X : c = 'x'; break;
    case VU_SDEST_Y : c = 'y'; break;
    case VU_SDEST_Z : c = 'z'; break;
    case VU_SDEST_W : c = 'w'; break;
    default: abort (); return;
    }

  (*info->fprintf_func) (info->stream, "%c", c);
}

/* The upper word flags bits.  */

static long
parse_uflags (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  long value = 0;
  char *str = *pstr;

  if (*str != '[')
    return 0;
  ++str;
  while (*str && *str != ']')
    {
      switch (tolower (*str))
	{
	case 'i' : value |= VU_FLAG_I; break;
	case 'e' : value |= VU_FLAG_E; break;
	case 'm' : value |= VU_FLAG_M; break;
	case 'd' : value |= VU_FLAG_D; break;
	case 't' : value |= VU_FLAG_T; break;
	default : *errmsg = _("invalid flag character present"); return 0;
	}
      ++str;
    }
  if (*str != ']')
    {
      *errmsg = _("syntax error in flag spec");
      return 0;
    }
  *pstr = str + 1;
  return value;
}

static void
print_uflags (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value)
    {
      (*info->fprintf_func) (info->stream, "[");
      if (value & VU_FLAG_I)
	(*info->fprintf_func) (info->stream, "i");
      if (value & VU_FLAG_E)
	(*info->fprintf_func) (info->stream, "e");
      if (value & VU_FLAG_M)
	(*info->fprintf_func) (info->stream, "m");
      if (value & VU_FLAG_D)
	(*info->fprintf_func) (info->stream, "d");
      if (value & VU_FLAG_T)
	(*info->fprintf_func) (info->stream, "t");
      (*info->fprintf_func) (info->stream, "]");
    }
}

/* Broadcase field.  */

static long
parse_bc (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  long value = u_parse_sdest (pstr, errmsg);

  if (*errmsg)
    return 0;
  if (value == -1)
    {
      *errmsg = MISSING_DEST;
      return 0;
    }
  if (isalnum ((unsigned char) **pstr))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }
  /* Save value for later verification in register parsing.  */
  state_vu_mnemonic_bc = value;
  return value;
}

/* During the extraction process, save the bc field for use in
   printing the bc register.  */

static long
extract_bc (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  state_vu_mnemonic_bc = *insn & 3;
  return state_vu_mnemonic_bc;
}

static long
parse_vfreg (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;
  int reg_dest;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'f')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg < 0 || reg > 31)
    {
      *errmsg = INVALID_REGISTER_NUMBER;
      return 0;
    }
  reg_dest = u_parse_dest (&str);
  if (isalnum ((unsigned char) *str))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }
  if (reg_dest == 0)
    ; /* not specified, nothing to do */
  else if (reg_dest != state_vu_mnemonic_dest)
    {
      *errmsg = _("register `dest' does not match instruction `dest'");
      return 0;
    }
  *pstr = str;
  return reg;
}

static void
print_vfreg (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     disassemble_info *info;
     DVP_INSN *insn;
     long value;
{
  (*info->fprintf_func) (info->stream, "vf%02ld", value);
  u_print_dest (info, insn, state_vu_mnemonic_dest);
}

/* FT register in broadcast case.  */

static long
parse_bcftreg (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;
  int reg_bc;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'f')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg < 0 || reg > 31)
    {
      *errmsg = _("invalid register number");
      return 0;
    }
  reg_bc = u_parse_sdest (&str, errmsg);
  if (*errmsg)
    return 0;
  if (reg_bc == -1)
    ; /* not specified, nothing to do */
  else if (reg_bc != state_vu_mnemonic_bc)
    {
      *errmsg = _("register `bc' does not match instruction `bc'");
      return 0;
    }
  *pstr = str;
  return reg;
}

static void
print_bcftreg (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "vf%02ld", value);
  /* The assembler syntax has been modified to not require the dest spec
     on the register.  Unlike the normal dest spec, for bc we print the
     letter because it makes the output more readable without having to
     remember which register is the bc one.  */
  print_sdest (opcode, operand, mods, insn, info, state_vu_mnemonic_bc);
}

/* ACC handling.  */

static long
parse_accdest (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  long acc_dest = 0;

  if (strncasecmp (str, "acc", 3) != 0)
    {
      *errmsg = _("expecting `acc'");
      return 0;
    }
  str += 3;
  acc_dest = u_parse_dest (&str);
  if (isalnum ((unsigned char) *str))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }
  if (acc_dest == 0)
    ; /* not specified, nothing to do */
  else if (acc_dest != state_vu_mnemonic_dest)
    {
      *errmsg = _("acc `dest' does not match instruction `dest'");
      return 0;
    }
  *pstr = str;
  /* Value isn't used, but we must return something.  */
  return 0;
}

static void
print_accdest (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     disassemble_info *info;
     DVP_INSN *insn;
     long value;
{
  (*info->fprintf_func) (info->stream, "acc");
  u_print_dest (info, insn, state_vu_mnemonic_dest);
}

/* XYZ operand handling.
   This simplifies the opmula,opmsub entries by keeping them equivalent to
   the others.  */

static void
insert_xyz (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  if (state_vu_mnemonic_dest != (VU_DEST_X | VU_DEST_Y | VU_DEST_Z))
    *errmsg = _("expecting `xyz' for `dest' value");
}


/* W operand handling.
   This simplifies the clip entry. */

static void
insert_w (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  if (state_vu_mnemonic_bc != VU_SDEST_W)
    *errmsg = _("expecting `w' for `bc' value");
}


/* F[ST] register using selector in F[ST]F field.
   Internally, the value is encoded in 7 bits: the 2 bit xyzw indicator
   followed by the 5 bit register number.  */

static long
parse_ffstreg (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  int reg, xyzw;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'f')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg < 0 || reg > 31)
    {
      *errmsg = _("invalid register number");
      return 0;
    }
  xyzw = u_parse_sdest (&str, errmsg);
  if (*errmsg)
    return 0;
  if (xyzw == -1)
    {
      *errmsg = MISSING_DEST;
      return 0;
    }
  if (isalnum ((unsigned char) *str))
    {
      *errmsg = INVALID_DEST;
      return 0;
    }
  *pstr = str;
  return reg | (xyzw << 5);
}

static void
print_ffstreg (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "vf%02ld", value & VU_MASK_REG);
  print_sdest (opcode, operand, mods, insn, info, (value >> 5) & 3);
}

static void
insert_ffstreg (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  if (operand->shift == VU_SHIFT_SREG)
    *insn |= VLFSF (value >> 5) | VS (value);
  else
    *insn |= VLFTF (value >> 5) | VT (value);
}

static long
extract_ffstreg (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  if (operand->shift == VU_SHIFT_SREG)
    return (((*insn & VLFSF (~0)) >> 21) << 5) | (((*insn) & MS) >> VU_SHIFT_SREG);
  else
    return (((*insn & VLFTF (~0)) >> 21) << 5) | (((*insn) & MS) >> VU_SHIFT_TREG);
}

/* F register.  */

static long
parse_freg (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'f')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg < 0 || reg > 31)
    {
      *errmsg = INVALID_REGISTER_NUMBER;
      return 0;
    }
  *pstr = str;
  return reg;
}

static void
print_freg (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "vf%02ld", value);
}

/* I register.  */

static long
parse_ireg (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'i')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg < 0 || reg > 31)
    {
      *errmsg = INVALID_REGISTER_NUMBER;
      return 0;
    }
  *pstr = str;
  return reg;
}

static void
print_ireg (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "vi%02ld", value);
}

/* VI01 register.  */

static long
parse_vi01 (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long reg;

  if (tolower (str[0]) != 'v'
      || tolower (str[1]) != 'i')
    {
      *errmsg = UNKNOWN_REGISTER;
      return 0;
    }

  start = str = str + 2;
  reg = strtol (start, &str, 10);
  if (reg != 1)
    {
      *errmsg = _("vi01 required here");
      return 0;
    }
  *pstr = str;
  return reg;
}

static void
print_vi01 (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "vi01");
}

/* Lower instruction 12 bit unsigned immediate.  */

static void
insert_luimm12 (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  *insn |= VLUIMM12TOP ((value & (1 << 11)) != 0) | VLIMM11 (value);
}

static long
extract_luimm12 (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  return (((*insn & MLUIMM12TOP) != 0) << 11) | VLIMM11 (*insn);
}

/* Lower instruction 12 bit unsigned immediate, upper 6 bits.  */

static void
insert_luimm12up6 (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  *insn |= VLUIMM12TOP ((value & (1 << 11)) != 0) | (value & 0x7c0);
}

/* Lower instruction 15 bit unsigned immediate.  */

static void
insert_luimm15 (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  *insn |= VLUIMM15TOP (value >> 11) | VLIMM11 (value);
}

static long
extract_luimm15 (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  return (((*insn & MLUIMM15TOP) >> 21) << 11) | VLIMM11 (*insn);
}

/* VIF support.  */

PARSE_FN (vif_ibit);
PRINT_FN (vif_ibit);

INSERT_FN (vif_wlcl);
EXTRACT_FN (vif_wlcl);

PARSE_FN (vif_mode);
PRINT_FN (vif_mode);

PARSE_FN (vif_ability);
PRINT_FN (vif_ability);

PARSE_FN (vif_mpgloc);
INSERT_FN (vif_mpgloc);

PARSE_FN (vif_datalen_special);
INSERT_FN (vif_datalen);
EXTRACT_FN (vif_datalen);

PARSE_FN (vif_imrubits);
INSERT_FN (vif_imrubits);
EXTRACT_FN (vif_imrubits);
PRINT_FN (vif_imrubits);

PARSE_FN (vif_unpacktype);
PRINT_FN (vif_unpacktype);

const dvp_operand vif_operands[] =
{
  /* place holder (??? not sure if needed) */
#define VIF_UNUSED 128
  { 0 },

  /* The I bit.  */
#define VIF_IBIT (VIF_UNUSED + 1)
  { 1, 31, 0, DVP_OPERAND_SUFFIX, parse_vif_ibit, 0, 0, print_vif_ibit },

  /* WL value, an 8 bit unsigned immediate, stored in upper 8 bits of
     immed field.  */
#define VIF_WL (VIF_IBIT + 1)
  { 8, 8, 0, 0, 0, insert_vif_wlcl, extract_vif_wlcl, 0 },

  /* CL value, an 8 bit unsigned immediate, stored in lower 8 bits of
     immed field.  */
#define VIF_CL (VIF_WL + 1)
  { 8, 0, 0, 0, 0, insert_vif_wlcl, extract_vif_wlcl, 0 },

  /* An 16 bit unsigned immediate, stored in lower 8 bits of immed field.  */
#define VIF_UIMM16 (VIF_CL + 1)
  { 16, 0, 0, 0, 0, 0, 0, 0 },

  /* The mode operand of `stmod'.  */
#define VIF_MODE (VIF_UIMM16 + 1)
  { 2, 0, 0, 0, parse_vif_mode, 0, 0, print_vif_mode },

  /* The ability operand of `mskpath3'.  */
#define VIF_ABILITY (VIF_MODE + 1)
  { 1, 15, 0, 0, parse_vif_ability, 0, 0, print_vif_ability },

  /* A VU address.  */
#define VIF_VUADDR (VIF_ABILITY + 1)
  { 16, 0, 0, 0, 0, 0, 0, 0 },

  /* A 32 bit immediate, appearing in 2nd word.  */
#define VIF_UIMM32_2 (VIF_VUADDR + 1)
  { 32, 0, 1, 0, 0, 0, 0, 0 },

  /* A 32 bit immediate, appearing in 3rd word.  */
#define VIF_UIMM32_3 (VIF_UIMM32_2 + 1)
  { 32, 0, 2, 0, 0, 0, 0, 0 },

  /* A 32 bit immediate, appearing in 4th word.  */
#define VIF_UIMM32_4 (VIF_UIMM32_3 + 1)
  { 32, 0, 3, 0, 0, 0, 0, 0 },

  /* A 32 bit immediate, appearing in 5th word.  */
#define VIF_UIMM32_5 (VIF_UIMM32_4 + 1)
  { 32, 0, 4, 0, 0, 0, 0, 0 },

  /* VU address used by mpg insn.  */
#define VIF_MPGLOC (VIF_UIMM32_5 + 1)
  { 16, 0, 0, DVP_OPERAND_VU_ADDRESS, parse_vif_mpgloc, insert_vif_mpgloc, 0, 0 },

  /* A variable length data specifier as an expression.  */
#define VIF_DATALEN (VIF_MPGLOC + 1)
  { 0, 0, 0, 0, 0, insert_vif_datalen, extract_vif_datalen, 0 },

  /* A variable length data specifier as a file name or '*'.
     The operand is marked as pc-relative as the length is calculated by
     emitting a label at the end of the data.  */
#define VIF_DATALEN_SPECIAL (VIF_DATALEN + 1)
  { 0, 0, 0, DVP_OPERAND_RELATIVE_BRANCH, parse_vif_datalen_special, insert_vif_datalen, 0, 0 },

  /* The IMRU bits of the unpack insn.  */
#define VIF_IMRUBITS (VIF_DATALEN_SPECIAL + 1)
  { 0, 0, 0, DVP_OPERAND_SUFFIX,
    parse_vif_imrubits, insert_vif_imrubits, extract_vif_imrubits, print_vif_imrubits },

  /* The type of the unpack insn.  */
#define VIF_UNPACKTYPE (VIF_IMRUBITS + 1)
  { 4, 24, 0, 0, parse_vif_unpacktype, 0, 0, print_vif_unpacktype },

  /* VU address used by unpack insn.  */
#define VIF_UNPACKLOC (VIF_UNPACKTYPE + 1)
  { 10, 0, 0, DVP_OPERAND_UNPACK_ADDRESS, 0, 0, 0, 0 },

/* end of list place holder */
  { 0 }
};

/* Some useful operand numbers.  */
const int vif_operand_mpgloc = DVP_OPERAND_INDEX (VIF_MPGLOC);
const int vif_operand_datalen_special = DVP_OPERAND_INDEX (VIF_DATALEN_SPECIAL);

/* Field mask values.  */
#define MVIFCMD 0x7f000000
#define MVIFUNPACK 0x60000000

/* Field values.  */
#define VVIFCMD(x) V ((x), 7, 24)
#define VVIFUNPACK V (0x60, 8, 24)

struct dvp_opcode vif_opcodes[] =
{
  { "vifnop",   { VIF_IBIT },                                  0x7fffffff, 0 },
  { "stcycle",  { VIF_IBIT, SP, VIF_WL, C, VIF_CL },           MVIFCMD, VVIFCMD (1), 0, DVP_OPCODE_IGNORE_DIS },
  { "stcycl",   { VIF_IBIT, SP, VIF_WL, C, VIF_CL },           MVIFCMD, VVIFCMD (1) },
  { "offset",   { VIF_IBIT, SP, VIF_UIMM16 },                  MVIFCMD, VVIFCMD (2) },
  { "base",     { VIF_IBIT, SP, VIF_UIMM16 },                  MVIFCMD, VVIFCMD (3) },
  { "itop",     { VIF_IBIT, SP, VIF_UIMM16 },                  MVIFCMD, VVIFCMD (4) },
  { "stmod",    { VIF_IBIT, SP, VIF_MODE },                    MVIFCMD + V (~0, 14, 2), VVIFCMD (5) },
  { "mskpath3", { VIF_IBIT, SP, VIF_ABILITY },                 MVIFCMD + V (~0, 15, 0), VVIFCMD (6) },
  { "mark",     { VIF_IBIT, SP, VIF_UIMM16 },                  MVIFCMD, VVIFCMD (7) },
  { "flushe",   { VIF_IBIT },                                  MVIFCMD, VVIFCMD (16) },
  { "flusha",   { VIF_IBIT },                                  MVIFCMD, VVIFCMD (19) },
  /* "flush" must appear after the previous two, longer ones first remember */
  { "flush",    { VIF_IBIT },                                  MVIFCMD, VVIFCMD (17) },
  { "mscalf",   { VIF_IBIT, SP, VIF_VUADDR },                  MVIFCMD, VVIFCMD (21) },
  { "mscal",    { VIF_IBIT, SP, VIF_VUADDR },                  MVIFCMD, VVIFCMD (20) },
  /* "mscal" must appear after the previous one. */
  { "mscnt",    { VIF_IBIT },                                  MVIFCMD, VVIFCMD (23) },

  /* 2 word instructions */
  { "stmask",   { VIF_IBIT, SP, VIF_UIMM32_2 },                MVIFCMD, VVIFCMD (32), 0, VIF_OPCODE_LEN2 },

  /* 5 word instructions */
  { "strow",    { VIF_IBIT, SP, VIF_UIMM32_2, C, VIF_UIMM32_3, C, VIF_UIMM32_4, C, VIF_UIMM32_5 }, MVIFCMD, VVIFCMD (48), 0, VIF_OPCODE_LEN5 },
  { "stcol",    { VIF_IBIT, SP, VIF_UIMM32_2, C, VIF_UIMM32_3, C, VIF_UIMM32_4, C, VIF_UIMM32_5 }, MVIFCMD, VVIFCMD (49), 0, VIF_OPCODE_LEN5 },

  /* variable length instructions */
  { "mpg",      { VIF_IBIT, SP, VIF_MPGLOC, C, VIF_DATALEN_SPECIAL }, MVIFCMD, VVIFCMD (0x4a), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_MPG + DVP_OPCODE_IGNORE_DIS },
  { "mpg",      { VIF_IBIT, SP, VIF_MPGLOC, C, VIF_DATALEN },         MVIFCMD, VVIFCMD (0x4a), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_MPG },

  /* `directhl' must appear before `direct', longer ones first */
  { "directhl", { VIF_IBIT, SP, VIF_DATALEN_SPECIAL },  MVIFCMD, VVIFCMD (0x51), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_DIRECT + DVP_OPCODE_IGNORE_DIS },
  { "directhl", { VIF_IBIT, SP, VIF_DATALEN },          MVIFCMD, VVIFCMD (0x51), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_DIRECT },

  { "direct",   { VIF_IBIT, SP, VIF_DATALEN_SPECIAL },  MVIFCMD, VVIFCMD (0x50), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_DIRECT + DVP_OPCODE_IGNORE_DIS },
  { "direct",   { VIF_IBIT, SP, VIF_DATALEN },          MVIFCMD, VVIFCMD (0x50), 0, VIF_OPCODE_LENVAR + VIF_OPCODE_DIRECT },

  { "unpack",   { VIF_IMRUBITS, SP, VIF_UNPACKTYPE, C, VIF_UNPACKLOC, C, VIF_DATALEN_SPECIAL }, MVIFUNPACK, VVIFUNPACK, 0, VIF_OPCODE_LENVAR + VIF_OPCODE_UNPACK + DVP_OPCODE_IGNORE_DIS },
  { "unpack",   { VIF_IMRUBITS, SP, VIF_UNPACKTYPE, C, VIF_UNPACKLOC, C, VIF_DATALEN },         MVIFUNPACK, VVIFUNPACK, 0, VIF_OPCODE_LENVAR + VIF_OPCODE_UNPACK },
};
const int vif_opcodes_count = sizeof (vif_opcodes) / sizeof (vif_opcodes[0]);

/* unpack/stcycl macro */

const struct dvp_macro vif_macros[] = {
  { "unpack${imrubits} ${wl},${cl},${unpacktype},${unpackloc},${datalen}", "stcycl %1,%2\nunpack%0 %3,%4,%5" }
};
const int vif_macro_count = sizeof (vif_macros) / sizeof (vif_macros[0]);

/* Length of parsed insn, in 32 bit words, or 0 if unknown.  */
static int state_vif_len;

/* The value for mpgloc seen.  */
static int state_vif_mpgloc;
/* Non-zero if '*' was seen for mpgloc.  */
static int state_vif_mpgloc_star_p;

/* The most recent WL,CL args to stcycl.
   HACK WARNING: This is a real kludge because when processing an `unpack'
   we don't necessarily know what stcycl insn goes with it.
   For now we punt and assume the last one we assembled.
   Note that this requires that there be at least one stcycl in the
   file before any unpack (if not we assume wl <= cl).  */
static int state_vif_wl = -1, state_vif_cl = -1;

/* The file argument to mpg,direct,directhl,unpack is stored here
   (in a malloc'd buffer).  */
static char *state_vif_data_file;
/* A numeric length for mpg,direct,directhl,unpack is stored here.  */
static int state_vif_data_len;

/* VIF parse,insert,extract,print helper fns.  */

static long
parse_vif_ibit (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  int flags = 0;

  if (*str != '[')
    return 0;

  for (str = str + 1; *str != ']'; ++str)
    {
      switch (tolower (*str))
	{
	case 'i' : flags = 1; break;
	default : *errmsg = _("unknown flag"); return 0;
	}
    }

  *pstr = str + 1;
  return flags;
}

static void
print_vif_ibit (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value)
    (*info->fprintf_func) (info->stream, "[i]");
}

/* Insert the wl,cl value into the insn.
   This is used to record the wl,cl values for subsequent use by an unpack
   insn.  */

static void
insert_vif_wlcl (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  *insn |= (value & ((1 << operand->bits) - 1)) << operand->shift;
  if (operand->shift == 8)
    state_vif_wl = value;
  else
    state_vif_cl = value;
}

/* Extract the wl,cl value from the insn.
   This is used to record the wl,cl values for subsequent use by an unpack
   insn.  */

static long
extract_vif_wlcl (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  long value = (*insn >> operand->shift) & ((1 << operand->bits) - 1);
  if (operand->shift == 8)
    state_vif_wl = value;
  else
    state_vif_cl = value;
  return value;
}

static const keyword stmod_modes[] = {
  { VIF_MODE_DIRECT, "direct" },
  { VIF_MODE_ADD,    "add" },
  { VIF_MODE_ADDROW, "addrow" },
  { 0, 0 }
};

static long
parse_vif_mode (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  int mode;
  char *str = *pstr;
  char *start;
  char c;

  start = str;
  if (isdigit ((unsigned char) *str))
    {
      mode = strtol (start, &str, 0);
    }
  else
    {
      str = scan_symbol (str);
      c = *str;
      *str = 0;
      mode = lookup_keyword_value (stmod_modes, start, 0);
      *str = c;
    }
  if (mode >= 0 && mode <= 2)
    {
      *pstr = str;
      return mode;
    }
  *errmsg = _("invalid mode");
  return 0;
}

static void
print_vif_mode (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     disassemble_info *info;
     DVP_INSN *insn;
     long value;
{
  const char *name = lookup_keyword_name (stmod_modes, value);

  if (name)
    (*info->fprintf_func) (info->stream, name);
  else
    (*info->fprintf_func) (info->stream, "???");
}

static long
parse_vif_ability (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;

  if (strncasecmp (str, "disable", 7) == 0)
    {
      *pstr += 7;
      return 1;
    }
  else if (strncasecmp (str, "enable", 6) == 0)
    {
      *pstr += 6;
      return 0;
    }
  *errmsg = _("invalid ability");
  return 1;
}

static void
print_vif_ability (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (! value)
    (*info->fprintf_func) (info->stream, "enable");
  else
    (*info->fprintf_func) (info->stream, "disable");
}

/* Parse the mpgloc field.
   This doesn't do any actual parsing.
   It exists to record the fact that '*' was seen.  */

static long
parse_vif_mpgloc (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  if (**pstr == '*')
    state_vif_mpgloc_star_p = 1;
  /* Since we don't advance *pstr, the normal expression parser will
     be called.  */
  return 0;
}

static void
insert_vif_mpgloc (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  *insn |= (value & ((1 << operand->bits) - 1)) << operand->shift;
  /* If we saw a '*', don't record VALUE - it's meaningless.  */
  if (! state_vif_mpgloc_star_p)
    state_vif_mpgloc = value;
}

/* Parse a file name or '*'.
   If a file name, the name is stored in state_vif_data_file and result is 0.
   If a *, result is -1.  */

static long
parse_vif_datalen_special (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  int len;

  if (*str == '*')
    {
      ++*pstr;
      return -1;
    }

  if (*str == '"')
    {
      start = ++str;
      ++str;
      while (*str && *str != '"')
	{
	  /* FIXME: \ parsing? */
	  ++str;
	}
      if (*str == 0)
	{
	  *errmsg = _("file name missing terminating `\"'");
	  return 0;
	}
      len = str - start;
      state_vif_data_file = xmalloc (len + 1);
      memcpy (state_vif_data_file, start, len);
      state_vif_data_file[len] = 0;
      *pstr = str + 1;
      return 0;
    }

  *errmsg = _("invalid data length");
  return 0;
}

/* This routine is used for both the mpg and unpack insns which store
   their length value in the `num' field and the direct/directhl insns
   which store their length value in the `immediate' field.  */

static void
insert_vif_datalen (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  /* FIXME: The actual insertion of the value in the insn is currently done
     in tc-dvp.c.  We just record the value.  */
  /* Perform the max+1 -> 0 conversion.  */
  if ((opcode->flags & VIF_OPCODE_MPG) != 0
      && value == 256)
    value = 0;
  else if ((opcode->flags & VIF_OPCODE_DIRECT) != 0
	   && value == 65536)
    value = 0;
  else if ((opcode->flags & VIF_OPCODE_UNPACK) != 0
	   && value == 256)
    value = 0;
  state_vif_data_len = value;
}

/* This routine is used for both the mpg and unpack insns which store
   their length value in the `num' field and the direct/directhl insns
   which store their length value in the `immediate' field.  */

static long
extract_vif_datalen (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  int len;

  switch ((*insn >> 24) & 0x70)
    {
    case 0x40 : /* mpg */
      len = (*insn >> 16) & 0xff;
      return len ? len : 256;
    case 0x50 : /* direct,directhl */
      len = *insn & 0xffff;
      return len ? len : 65536;
    case 0x60 : /* unpack */
    case 0x70 :
      len = (*insn >> 16) & 0xff;
      return len ? len : 256;
    default :
      return 0;
    }
}

static long
parse_vif_imrubits (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  int flags = 0;

  if (*str != '[')
    return 0;

  for (str = str + 1; *str != ']'; ++str)
    {
      switch (tolower (*str))
	{
	case 'i' : flags |= VIF_FLAG_I; break;
	case 'm' : flags |= VIF_FLAG_M; break;
	case 'r' : flags |= VIF_FLAG_R; break;
	case 'u' : flags |= VIF_FLAG_U; break;
	default : *errmsg = _("unknown vif flag"); return 0;
	}
    }

  *pstr = str + 1;
  return flags;
}

static void
insert_vif_imrubits (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  if (value & VIF_FLAG_I)
    *insn |= 0x80000000;
  if (value & VIF_FLAG_M)
    *insn |= 0x10000000;
  if (value & VIF_FLAG_R)
    *insn |= 0x8000;
  if (value & VIF_FLAG_U)
    *insn |= 0x4000;
}

static long
extract_vif_imrubits (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  long value = 0;
  if (*insn & 0x80000000)
    value |= VIF_FLAG_I;
  if (*insn & 0x10000000)
    value |= VIF_FLAG_M;
  if (*insn & 0x8000)
    value |= VIF_FLAG_R;
  if (*insn & 0x4000)
    value |= VIF_FLAG_U;
  return value;
}

static void
print_vif_imrubits (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value)
    {
      (*info->fprintf_func) (info->stream, "[");
      if (value & VIF_FLAG_I)
	(*info->fprintf_func) (info->stream, "i");
      if (value & VIF_FLAG_M)
	(*info->fprintf_func) (info->stream, "m");
      if (value & VIF_FLAG_R)
	(*info->fprintf_func) (info->stream, "r");
      if (value & VIF_FLAG_U)
	(*info->fprintf_func) (info->stream, "u");
      (*info->fprintf_func) (info->stream, "]");
    }
}

static const keyword unpack_types[] = {
  { VIF_UNPACK_S_32, "s_32" },
  { VIF_UNPACK_S_16, "s_16" },
  { VIF_UNPACK_S_8, "s_8" },
  { VIF_UNPACK_V2_32, "v2_32" },
  { VIF_UNPACK_V2_16, "v2_16" },
  { VIF_UNPACK_V2_8, "v2_8" },
  { VIF_UNPACK_V3_32, "v3_32" },
  { VIF_UNPACK_V3_16, "v3_16" },
  { VIF_UNPACK_V3_8, "v3_8" },
  { VIF_UNPACK_V4_32, "v4_32" },
  { VIF_UNPACK_V4_16, "v4_16" },
  { VIF_UNPACK_V4_8, "v4_8" },
  { VIF_UNPACK_V4_5, "v4_5" },
  { 0, 0 }
};

static long
parse_vif_unpacktype (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  int type;
  char *str = *pstr;
  char *start;
  char c;

  start = str;
  str = scan_symbol (str);
  c = *str;
  *str = 0;
  type = lookup_keyword_value (unpack_types, start, 0);
  *str = c;
  if (type != -1)
    {
      *pstr = str;
      return type;
    }
  *errmsg = _("invalid unpack type");
  return 0;
}

static void
print_vif_unpacktype (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  const char *name = lookup_keyword_name (unpack_types, value);

  if (name)
    (*info->fprintf_func) (info->stream, name);
  else
    (*info->fprintf_func) (info->stream, "???");
}

/* External VIF supporting routines.  */

/* Return length, in 32 bit words, of just parsed vif insn,
   or 0 if unknown.  */

int
vif_len ()
{
  /* This shouldn't be called unless a vif insn was parsed.
     Also, we want to catch errors in parsing that don't set this.  */
  if (state_vif_len == -1)
    abort ();

  return state_vif_len;
}

/* Return the length value to use for an unpack instruction, type TYPE,
   whose data length in bytes is LEN.
   We do not perform the max + 1 -> 0 transformation.  That's up to the caller.
   WL,CL are the values to use for those registers.  A value of -1 means to
   use the internally recorded values.  If the values are still -1, we
   assume wl <= cl.
   This means that it is assumed that unpack always requires a preceeding
   stcycl sufficiently recently that the value we have recorded is correct.
   FIXME: We assume len % 4 == 0.  */

int
vif_unpack_len_value (type, wl, cl, len)
     unpack_type type;
     int wl,cl;
     int len;
{
  int vn = (type >> 2) & 3;
  int vl = type & 3;
  int n, num;

  if (wl < 0)
    wl = state_vif_wl;
  if (cl < 0)
    cl = state_vif_cl;
  if (wl == -1 || cl == -1)
    wl = cl = 1;

  if (wl <= cl)
    {
      /* data length (in words) len = ((((32 >> vl) * (vn + 1)) * num) / 32)
	 Thus num = (len / 4) * 32 / ((32 >> vl) * (vn + 1)) */
      num = (len * 32 / 4) / ((32 >> vl) * (vn + 1));
    }
  else /* wl > cl */
    {
      /* data length (in words) len = ((((32 >> vl) * (vn + 1)) * n) / 32)
	 n = CL * (num / WL) + min (num % WL, CL)
	 Thus n = (len / 4) * 32 / ((32 >> vl) * (vn + 1))
	 num = (n - delta) * wl / cl;
	 where `delta' is defined to be "just right".
	 I'm too bloody tired to do this more cleverly.  */
      n = (len * 32 / 4) / ((32 >> vl) * (vn + 1));
      /* Avoid divide by zero.  */
      if (cl == 0)
	return 0;
      /* Note that wl cannot be zero here.
	 We know that cl > 0 and thus if wl == 0 we would have taken
	 the `then' part of this if().  */
      num = n * wl / cl;
      while (num > 0
	     && n != (cl * num / wl) + MIN (num % wl, cl))
	--num;
      if (num == 0)
	{
	  fprintf (stderr, _("internal error in unpack length calculation"));
	  abort ();
	}
    }
  return num;
}

/* Return the data length, in bytes, of an unpack insn of type TYPE,
   whose `num' field in NUM.
   If WL,CL are unknown, we assume wl <= cl.  */

int
vif_unpack_len (type, num)
     unpack_type type;
     int num;
{
  int vn = (type >> 2) & 3;
  int vl = type & 3;
  int wl = state_vif_wl;
  int cl = state_vif_cl;
  int n, len;

  if (wl == -1 || cl == -1)
    wl = cl = 1;

  /* Perform 0 -> max+1 conversion.  */
  if (num == 0)
    num = 256;
  if (wl == 0)
    wl = 256;

  if (wl <= cl)
    {
      /* data length (in words) len = ((((32 >> vl) * (vn + 1)) * num) / 32) */
      n = num;
    }
  else /* wl > cl */
    {
      /* data length (in words) len = ((((32 >> vl) * (vn + 1)) * n) / 32)
	 n = CL * (num / WL) + min (num % WL, CL) */
      n = cl * num / wl + MIN (num % wl, cl);
    }
  /* +31: round up to next word boundary */
  len = ((((32 >> vl) * (vn + 1)) * n) + 31) / 32;
  return len * 4;
}

/* Return the length, in 32 bit words, of a VIF insn.
   INSN is the first word.
   The cpu type of the following data is stored in PCPU.  */

int
vif_insn_len (insn, pcpu)
     DVP_INSN insn;
     dvp_cpu *pcpu;
{
  unsigned char cmd;

  *pcpu = DVP_VIF;

  /* strip off `i' bit */
  insn &= 0x7fffffff;

  /* get top byte */
  cmd = insn >> 24;

  /* see page 11 of cpu2 spec 2.1 for further info */
  if ((cmd & 0x60) == 0)
    return 1;
  if ((cmd & 0x70) == 0x20)
    return 2;
  if ((cmd & 0x70) == 0x30)
    return 5;
  if ((cmd & 0x70) == 0x40)
    {
      /* mpg */
      int len = (insn >> 16) & 0xff;
      *pcpu = DVP_VUUP;
      return 1 + (len == 0 ? 256 : len) * 2;
    }
  if ((cmd & 0x70) == 0x50)
    {
      /* direct,directhl */
      int len = insn & 0xffff;
      *pcpu = DVP_GIF;
      return 1 + (len == 0 ? 65536 : len) * 4;
    }
  if ((cmd & 0x60) == 0x60)
    {
      /* unpack */
      int len = vif_unpack_len (cmd & 15, (insn >> 16) & 0xff);
      if (len == -1)
	len = 4; /* FIXME: revisit */
      return 1 + len / 4;
    }

  /* unknown insn */
  return 1;
}

/* Get the value of mpgloc seen.  */

int
vif_get_mpgloc ()
{
  return state_vif_mpgloc;
}

/* Return recorded variable data length indicator.
   This is either a file name or a numeric length.
   A length of -1 means the caller must compute it.  */

void
vif_get_var_data (file, len)
     const char **file;
     int *len;
{
  *file = state_vif_data_file;
  *len = state_vif_data_len;
}

/* Return the specified values for wl,cl.  */

void
vif_get_wl_cl (wlp, clp)
     int *wlp, *clp;
{
  *wlp = state_vif_wl;
  *clp = state_vif_cl;
}

/* DMA support.  */

PARSE_FN (dma_flags);
INSERT_FN (dma_flags);
EXTRACT_FN (dma_flags);
PRINT_FN (dma_flags);

INSERT_FN (dma_count);
EXTRACT_FN (dma_count);
PRINT_FN (dma_count);

PARSE_FN (dma_data2);

PARSE_FN (dma_addr);
INSERT_FN (dma_addr);
EXTRACT_FN (dma_addr);
PRINT_FN (dma_addr);

const dvp_operand dma_operands[] =
{
    /* place holder (??? not sure if needed) */
#define DMA_UNUSED 128
    { 0 },

    /* dma tag flag bits */
#define DMA_FLAGS (DMA_UNUSED + 1)
    { 0, 0, 0, DVP_OPERAND_SUFFIX,
      parse_dma_flags, insert_dma_flags, extract_dma_flags, print_dma_flags },

    /* dma data spec */
#define DMA_COUNT (DMA_FLAGS + 1)
    { 16, 0, 0, DVP_OPERAND_DMA_COUNT,
      0, 0 /*insert_dma_count*/,
      0 /*extract_dma_count*/, 0 /*print_dma_count*/ },

    /* dma autocount modifier */
#define DMA_AUTOCOUNT (DMA_COUNT + 1)
    { 0, 0, 0, DVP_OPERAND_AUTOCOUNT, 0, 0, 0, 0 },

    /* dma in-line-data */
#define DMA_INLINE (DMA_AUTOCOUNT + 1)
    { 0, 0, 0, DVP_OPERAND_FAKE + DVP_OPERAND_DMA_INLINE, 0, 0, 0, 0 },

    /* dma ref data address */
#define DMA_ADDR (DMA_INLINE + 1)
    { 27, 4, 1, DVP_OPERAND_DMA_ADDR + DVP_OPERAND_MIPS_ADDRESS,
      0 /*parse_dma_addr*/, insert_dma_addr,
      extract_dma_addr, 0 /*print_dma_addr*/ },

    /* dma next tag spec */
#define DMA_NEXT (DMA_ADDR + 1)
    { 27, 4, 1, DVP_OPERAND_DMA_NEXT + DVP_OPERAND_MIPS_ADDRESS,
      0, insert_dma_addr, extract_dma_addr, 0 /*print_dma_addr*/ },

/* end of list place holder */
  { 0 }
};

/* Some useful operand numbers.  */
const int dma_operand_count = DVP_OPERAND_INDEX (DMA_COUNT);
const int dma_operand_addr = DVP_OPERAND_INDEX (DMA_ADDR);

struct dvp_opcode dma_opcodes[] =
{
  { "dmarefe", { DMA_FLAGS, SP, '*',       C, DMA_AUTOCOUNT, DMA_ADDR },             0x70000000, 0x00000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmarefe", { DMA_FLAGS, SP, DMA_COUNT, C, DMA_ADDR },                            0x70000000, 0x00000000, 0 },
  { "dmacnt",  { DMA_FLAGS, SP, '*',       DMA_AUTOCOUNT, DMA_INLINE },              0x70000000, 0x10000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmacnt",  { DMA_FLAGS, SP, DMA_COUNT, DMA_INLINE },                             0x70000000, 0x10000000, 0 },
  { "dmanext", { DMA_FLAGS, SP, '*',       C, DMA_AUTOCOUNT, DMA_INLINE, DMA_NEXT }, 0x70000000, 0x20000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmanext", { DMA_FLAGS, SP, DMA_COUNT, C, DMA_INLINE, DMA_NEXT },                0x70000000, 0x20000000, 0 },
  { "dmaref",  { DMA_FLAGS, SP, '*',       C, DMA_AUTOCOUNT, DMA_ADDR },             0x70000000, 0x30000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmaref",  { DMA_FLAGS, SP, DMA_COUNT, C, DMA_ADDR },                            0x70000000, 0x30000000, 0 },
  { "dmarefs", { DMA_FLAGS, SP, '*',       C, DMA_AUTOCOUNT, DMA_ADDR },             0x70000000, 0x40000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmarefs", { DMA_FLAGS, SP, DMA_COUNT, C, DMA_ADDR },                            0x70000000, 0x40000000, 0 },
  { "dmacall", { DMA_FLAGS, SP, '*',       C, DMA_AUTOCOUNT, DMA_INLINE, DMA_NEXT }, 0x70000000, 0x50000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmacall", { DMA_FLAGS, SP, DMA_COUNT, C, DMA_INLINE, DMA_NEXT },                0x70000000, 0x50000000, 0 },
  { "dmaret",  { DMA_FLAGS, SP, '*',       DMA_AUTOCOUNT, DMA_INLINE},               0x70000000, 0x60000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmaret",  { DMA_FLAGS, SP, DMA_COUNT, DMA_INLINE },                             0x70000000, 0x60000000, 0 },
  { "dmaend",  { DMA_FLAGS, SP, '*',       DMA_AUTOCOUNT, DMA_INLINE },              0x70000000, 0x70000000, 0, DVP_OPCODE_IGNORE_DIS },
  { "dmaend",  { DMA_FLAGS, SP, DMA_COUNT, DMA_INLINE },                             0x70000000, 0x70000000, 0 },
  { "dmaend",  { DMA_FLAGS },                                                        0x70000000, 0x70000000, 0 },
};
const int dma_opcodes_count = sizeof (dma_opcodes) / sizeof (dma_opcodes[0]);

/* DMA parse,insert,extract,print helper fns.  */

static long
parse_dma_flags (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  int flags = 0;

  if (*str != '[')
    return 0;

  for (str = str + 1; *str != ']'; ++str)
    {
      switch (tolower (*str))
	{
	case '0' : flags |= DMA_FLAG_PCE0; break;
	case '1' : flags |= DMA_FLAG_PCE1; break;
	case 'i' : flags |= DMA_FLAG_INT; break;
	case 's' : flags |= DMA_FLAG_SPR; break;
	default : *errmsg = _("unknown dma flag"); return 0;
	}
    }

  *pstr = str + 1;
  return flags;
}

static void
insert_dma_flags (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  if (value & DMA_FLAG_PCE0)
    insn[0] |= 2 << 26;
  else if (value & DMA_FLAG_PCE0)
    insn[0] |= 3 << 26;
  if (value & DMA_FLAG_INT)
    insn[0] |= (1 << 31);
  if (value & DMA_FLAG_SPR)
    insn[1] |= (1 << 31);
}

static long
extract_dma_flags (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  long value = 0;
  if (((insn[0] >> 26) & 3) == 2)
     value |= DMA_FLAG_PCE0;
  if (((insn[0] >> 26) & 3) == 3)
     value |= DMA_FLAG_PCE0;
  if (((insn[0] >> 31) & 1) == 1)
     value |= DMA_FLAG_INT;
  if (((insn[1] >> 31) & 1) == 1)
     value |= DMA_FLAG_SPR;
  return value;
}

static void
print_dma_flags (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value)
    {
      (*info->fprintf_func) (info->stream, "[");
      if (value & DMA_FLAG_PCE0)
	(*info->fprintf_func) (info->stream, "0");
      if (value & DMA_FLAG_PCE1)
	(*info->fprintf_func) (info->stream, "1");
      if (value & DMA_FLAG_INT)
	(*info->fprintf_func) (info->stream, "i");
      if (value & DMA_FLAG_SPR)
	(*info->fprintf_func) (info->stream, "s");
      (*info->fprintf_func) (info->stream, "]");
    }
}


static void
insert_dma_count (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
}

static long
extract_dma_count (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  return 0;
}

static void
print_dma_count (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "???");
}

static long
parse_dma_addr (opcode, operand, mods, pstr, errmsg)
    const dvp_opcode *opcode;
    const dvp_operand *operand;
    int mods;
    char **pstr;
    const char **errmsg;
{
    return 0;
}

static void
insert_dma_addr (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  int word;

  if (mods & DVP_MOD_THIS_WORD)
    word = 0;
  else if (operand->word)
    word = operand->word;
  else
    word = operand->shift / 32;

  /* The lower 4 bits are cut off and the value begins at bit 4.  */
  insn[word] |= value & 0x7ffffff0;
}

static long
extract_dma_addr (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  int word;

  if (mods & DVP_MOD_THIS_WORD)
    word = 0;
  else if (operand->word)
    word = operand->word;
  else
    word = operand->shift / 32;

  return insn[word] & 0x7ffffff0;
}

static void
print_dma_addr (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "???");
}

/* GIF support.  */

PARSE_FN (gif_prim);
INSERT_FN (gif_prim);
EXTRACT_FN (gif_prim);
PRINT_FN (gif_prim);

PARSE_FN (gif_regs);
INSERT_FN (gif_regs);
EXTRACT_FN (gif_regs);
PRINT_FN (gif_regs);

PARSE_FN (gif_nloop);
INSERT_FN (gif_nloop);
EXTRACT_FN (gif_nloop);
PRINT_FN (gif_nloop);

PARSE_FN (gif_eop);
EXTRACT_FN (gif_eop);
PRINT_FN (gif_eop);

/* Bit numbering:

    insn[0]    insn[1]     insn[2]     insn[3]
   31 ... 0 | 63 ... 32 | 95 ... 64 | 127 ... 96  */

const dvp_operand gif_operands[] =
{
  /* place holder (??? not sure if needed) */
#define GIF_UNUSED 128
  { 0 },

  /* PRIM=foo operand */
#define GIF_PRIM (GIF_UNUSED + 1)
  { 11, 47, 0, 0, parse_gif_prim, insert_gif_prim, extract_gif_prim, print_gif_prim },

  /* REGS=foo operand */
#define GIF_REGS (GIF_PRIM + 1)
  { 64, 0, 0, 0, parse_gif_regs, insert_gif_regs, extract_gif_regs, print_gif_regs },

  /* NLOOP=foo operand */
#define GIF_NLOOP (GIF_REGS + 1)
  { 15, 0, 0, 0, parse_gif_nloop, insert_gif_nloop, extract_gif_nloop, print_gif_nloop },

  /* EOP operand */
#define GIF_EOP (GIF_NLOOP + 1)
  { 1, 15, 0, 0, parse_gif_eop, 0, extract_gif_eop, print_gif_eop },

/* end of list place holder */
  { 0 }
};

/* Some useful operand numbers.  */
const int gif_operand_nloop = DVP_OPERAND_INDEX (GIF_NLOOP);

/* GIF opcode values.  */
#define VGIFOP(x) (((x) & 3) << 26)
#define VGIFNREGS(x) (((x) & 15) << 28)

/* GIF opcode masks.  */
#define MGIFOP VGIFOP (~0)
#define MGIFNREGS VGIFNREGS (~0)

struct dvp_opcode gif_opcodes[] =
{
  /* Some of these may take optional arguments.
     The way this is handled is to have multiple table entries, those with and
     those without the optional arguments.
     !!! The order here is important.  The code that scans this table assumes
     that if it reaches the end of a syntax string there is nothing more to
     parse.  This means that longer versions of instructions must appear before
     shorter ones.  Otherwise the text at the "end" of a longer one may be
     interpreted as junk when the parser is using a shorter version of the
     syntax string.  */

  { "gifpacked", { SP, GIF_PRIM, C, GIF_REGS, C, GIF_NLOOP, C, GIF_EOP }, MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_REGS, C, GIF_NLOOP, C, GIF_EOP },              MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_PRIM, C, GIF_REGS, C, GIF_EOP },               MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_PRIM, C, GIF_REGS, C, GIF_NLOOP },             MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_REGS, C, GIF_EOP },                            MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_REGS, C, GIF_NLOOP },                          MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_PRIM, C, GIF_REGS },                           MGIFOP, VGIFOP (0), 1 },
  { "gifpacked", { SP, GIF_REGS },                                        MGIFOP, VGIFOP (0), 1 },

  { "gifreglist", { SP, GIF_REGS, C, GIF_NLOOP, C, GIF_EOP }, MGIFOP, VGIFOP (1), 1 },
  { "gifreglist", { SP, GIF_REGS, C, GIF_EOP },               MGIFOP, VGIFOP (1), 1 },
  { "gifreglist", { SP, GIF_REGS, C, GIF_NLOOP },             MGIFOP, VGIFOP (1), 1 },
  { "gifreglist", { SP, GIF_REGS },                           MGIFOP, VGIFOP (1), 1 },

  { "gifimage", { SP, GIF_NLOOP, C, GIF_EOP }, MGIFOP, VGIFOP (2), 1 },
  { "gifimage", { SP, GIF_EOP },               MGIFOP, VGIFOP (2), 1 },
  { "gifimage", { SP, GIF_NLOOP },             MGIFOP, VGIFOP (2), 1 },
  { "gifimage", { 0 },                         MGIFOP, VGIFOP (2), 1 },
};
const int gif_opcodes_count = sizeof (gif_opcodes) / sizeof (gif_opcodes[0]);

/* GIF parsing/printing state.  */

static int state_gif_nregs;
static int state_gif_regs[16];
static int state_gif_nloop;

/* GIF parse,insert,extract,print helper fns.  */

static long
parse_gif_prim (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  long prim;

  if (strncasecmp (str, "prim=", 5) != 0)
    {
      *errmsg = _("missing PRIM spec");
      return 0;
    }
  str += 5;
  start = str;
  prim = strtol (start, &str, 0);
  if (str == start)
    {
      *errmsg = _("missing PRIM spec");
      return 0;
    }
  *pstr = str;
  return prim;
}

static void
insert_gif_prim (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  int word = operand->shift / 32;
  /* Chop off unwanted bits.  */
  insn[word] |= (value & ((1 << operand->bits) - 1)) << operand->shift % 32;
  /* Set the PRE bit.  */
  insn[word] |= GIF_PRE;
}

static long
extract_gif_prim (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  int word = operand->shift / 32;
  if (! (insn[word] & GIF_PRE))
    {
      /* The prim register isn't used.  Mark as invalid so this choice is
	 skipped as a possibility for disassembly.  */
      *pinvalid = 1;
      return -1;
    }
  return (insn[word] >> operand->shift % 32) & ((1 << operand->bits) - 1);
}

static void
print_gif_prim (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value != -1)
    (*info->fprintf_func) (info->stream, "prim=0x%lx", value);
}

static const keyword gif_regs[] = {
  { GIF_REG_PRIM,    "prim" },
  { GIF_REG_RGBAQ,   "rgbaq" },
  { GIF_REG_ST,      "st" },
  { GIF_REG_UV,      "uv" },
  { GIF_REG_XYZF2,   "xyzf2" },
  { GIF_REG_XYZ2,    "xyz2" },
  { GIF_REG_TEX0_1,  "tex0_1" },
  { GIF_REG_TEX0_2,  "tex0_2" },
  { GIF_REG_CLAMP_1, "clamp_1" },
  { GIF_REG_CLAMP_2, "clamp_2" },
  { GIF_REG_XYZF,    "xyzf" },
  /* 11 is unused.  Should it ever appear we want to disassemble it somehow
     so we give it a name anyway.  */
  { GIF_REG_UNUSED11, "unused11" },
  { GIF_REG_XYZF3,   "xyzf3" },
  { GIF_REG_XYZ3,    "xyz3" },
  { GIF_REG_A_D,     "a_d" },
  { GIF_REG_NOP,     "nop" },
  { 0, 0 }
};

/* Parse a REGS= spec.
   The result is the number of registers parsed.
   The selected registers are recorded internally in a static
   variable for use later by the insert routine.  */

static long
parse_gif_regs (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  char c;
  int reg,nregs;

  if (strncasecmp (str, "regs=", 5) != 0)
    {
      *errmsg = _("missing REGS spec");
      return 0;
    }
  str += 5;
  SKIP_BLANKS (str);
  if (*str != '{')
    {
      *errmsg = _("missing '{' in REGS spec");
      return 0;
    }
  ++str;

  nregs = 0;
  while (*str && *str != '}')
    {
      if (nregs == 16)
	{
	  *errmsg = _("too many registers");
	  return 0;
	}

      /* Pick out the register name.  */
      SKIP_BLANKS (str);
      start = str;
      str = scan_symbol (str);
      if (str == start)
	{
	  *errmsg = _("invalid REG");
	  return 0;
	}

      /* Look it up in the table.  */
      c = *str;
      *str = 0;
      reg = lookup_keyword_value (gif_regs, start, 0);
      *str = c;
      if (reg == -1)
	{
	  *errmsg = _("invalid REG");
	  return 0;
	}

      /* Tuck the register number away for later use.  */
      state_gif_regs[nregs++] = reg;

      /* Prepare for the next one.  */
      SKIP_BLANKS (str);
      if (*str == ',')
	++str;
      else if (*str != '}')
	break;
    }
  if (*str != '}')
    {
      *errmsg = _("missing '}' in REGS spec");
      return 0;
    }

  *pstr = str + 1;
  return nregs;
}

static void
insert_gif_regs (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  int i;
  DVP_INSN *p;

  state_gif_nregs = value;

  /* Registers are stored in word 2,3 (0-origin) in memory.
     Each word is processed from the lower bit numbers upwards,
     and the words are stored little endian.  We must record each word
     in host-endian form as the word will be swapped to target endianness
     when written out.  */

  p = insn + 2;
  for (i = 0; i < state_gif_nregs; ++i)
    {
      /* Move to next word?  */
      if (i == 8)
	++p;

      *p |= state_gif_regs[i] << (i * 4);
    }

  insn[1] |= VGIFNREGS (state_gif_nregs);
}

static long
extract_gif_regs (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  state_gif_nregs = (insn[1] & MGIFNREGS) >> 28;
  return state_gif_nregs;
}

static void
print_gif_regs (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  /* VALUE is the number of registers [returned by the extract handler].  */
  int i;
  DVP_INSN *p;

  /* See insert_gif_regs for an explanation of how the regs are stored.  */

  (*info->fprintf_func) (info->stream, "regs={");

  p = insn + 2;
  for (i = 0; i < value; ++i)
    {
      int reg;

      /* Move to next word?  */
      if (i == 8)
	++p;

      reg = (*p >> (i * 4)) & 15;

      (*info->fprintf_func) (info->stream, "%s",
			     lookup_keyword_name (gif_regs, reg));
      if (i + 1 != value)
	(*info->fprintf_func) (info->stream, ",");
    }

  (*info->fprintf_func) (info->stream, "}");
}

static long
parse_gif_nloop (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  char *str = *pstr;
  char *start;
  int nloop;

  if (strncasecmp (str, "nloop=", 6) != 0)
    {
      *errmsg = _("missing NLOOP spec");
      return 0;
    }
  str += 6;
  SKIP_BLANKS (str);
  start = str;
  nloop = strtol (start, &str, 10);
  if (str == start)
    {
      *errmsg = _("invalid NLOOP spec");
      return 0;
    }
  *pstr = str;
  return nloop;
}

static void
insert_gif_nloop (opcode, operand, mods, insn, value, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     long value;
     const char **errmsg;
{
  int word = operand->shift / 32;
  insn[word] |= (value & ((1 << operand->bits) - 1)) << operand->shift % 32;
  /* Tuck the value away for later use.  */
  state_gif_nloop = value;
}

static long
extract_gif_nloop (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  long value;
  int word = operand->shift / 32;
  value = (insn[word] >> operand->shift % 32) & ((1 << operand->bits) - 1);
  /* Tuck the value away for later use.  */
  state_gif_nloop = value;
  return value;
}

static void
print_gif_nloop (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  (*info->fprintf_func) (info->stream, "nloop=%ld", value);
}

static long
parse_gif_eop (opcode, operand, mods, pstr, errmsg)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     char **pstr;
     const char **errmsg;
{
  if (strncasecmp (*pstr, "eop", 3) == 0)
    {
      *pstr += 3;
      return 1;
    }
  *errmsg = _("missing `EOP'");
  return 0;
}

static long
extract_gif_eop (opcode, operand, mods, insn, pinvalid)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     int *pinvalid;
{
  long value;
  int word = operand->shift / 32;
  value = (insn[word] >> operand->shift % 32) & ((1 << operand->bits) - 1);
  /* If EOP=0, mark this choice as invalid so it's not used for
     disassembly.  */
  if (! value)
    *pinvalid = 1;
  return value;
}

static void
print_gif_eop (opcode, operand, mods, insn, info, value)
     const dvp_opcode *opcode;
     const dvp_operand *operand;
     int mods;
     DVP_INSN *insn;
     disassemble_info *info;
     long value;
{
  if (value)
    (*info->fprintf_func) (info->stream, "eop");
}

/* External GIF support routines.  */

int
gif_nloop ()
{
  return state_gif_nloop;
}

int
gif_nregs ()
{
  return state_gif_nregs;
}

/* Init fns.
   These are called before doing each of the respective activities.  */

/* Called by the assembler before parsing an instruction.  */

void
dvp_opcode_init_parse ()
{
  state_vu_mnemonic_dest = -1;
  state_vu_mnemonic_bc = -1;
  state_vif_mpgloc = -1;
  state_vif_mpgloc_star_p = 0;
  state_vif_len = -1;
  state_vif_data_file = NULL;
  state_vif_data_len = 0;
  state_gif_nregs = -1;
  state_gif_nloop = -1;
}

/* Called by the disassembler before printing an instruction.  */

void
dvp_opcode_init_print ()
{
  state_vu_mnemonic_dest = -1;
  state_vu_mnemonic_bc = -1;
  state_vif_len = -1;
  state_gif_nregs = -1;
  state_gif_nloop = -1;
}

/* Indexed by first letter of opcode.  Points to chain of opcodes with same
   first letter.  */
/* ??? One can certainly use a better hash.  Later.  */
static dvp_opcode *upper_opcode_map[26 + 1];
static dvp_opcode *lower_opcode_map[26 + 1];

/* Indexed by insn code.  Points to chain of opcodes with same insn code.  */
static dvp_opcode *upper_icode_map[(1 << DVP_ICODE_HASH_SIZE) - 1];
static dvp_opcode *lower_icode_map[(1 << DVP_ICODE_HASH_SIZE) - 1];

/* Initialize any tables that need it.
   Must be called once at start up (or when first needed).

   FLAGS is currently unused but is intended to control initialization.  */

void
dvp_opcode_init_tables (flags)
     int flags;
{
  static int init_p = 0;

  /* We may be intentionally called more than once (for example gdb will call
     us each time the user switches cpu).  These tables only need to be init'd
     once though.  */
  /* ??? We can remove the need for dvp_opcode_supported by taking it into
     account here, but I'm not sure I want to do that yet (if ever).  */
  if (!init_p)
    {
      int i;

      /* Upper VU table.  */

      memset (upper_opcode_map, 0, sizeof (upper_opcode_map));
      memset (upper_icode_map, 0, sizeof (upper_icode_map));
      /* Scan the table backwards so macros appear at the front.  */
      for (i = vu_upper_opcodes_count - 1; i >= 0; --i)
	{
	  int opcode_hash = DVP_HASH_UPPER_OPCODE (vu_upper_opcodes[i].mnemonic);
	  int icode_hash = DVP_HASH_UPPER_ICODE (vu_upper_opcodes[i].value);

	  vu_upper_opcodes[i].next_asm = upper_opcode_map[opcode_hash];
	  upper_opcode_map[opcode_hash] = &vu_upper_opcodes[i];

	  vu_upper_opcodes[i].next_dis = upper_icode_map[icode_hash];
	  upper_icode_map[icode_hash] = &vu_upper_opcodes[i];
	}

      /* Lower VU table.  */

      memset (lower_opcode_map, 0, sizeof (lower_opcode_map));
      memset (lower_icode_map, 0, sizeof (lower_icode_map));
      /* Scan the table backwards so macros appear at the front.  */
      for (i = vu_lower_opcodes_count - 1; i >= 0; --i)
	{
	  int opcode_hash = DVP_HASH_LOWER_OPCODE (vu_lower_opcodes[i].mnemonic);
	  int icode_hash = DVP_HASH_LOWER_ICODE (vu_lower_opcodes[i].value);

	  vu_lower_opcodes[i].next_asm = lower_opcode_map[opcode_hash];
	  lower_opcode_map[opcode_hash] = &vu_lower_opcodes[i];

	  vu_lower_opcodes[i].next_dis = lower_icode_map[icode_hash];
	  lower_icode_map[icode_hash] = &vu_lower_opcodes[i];
	}

      /* FIXME: We just hash everything to the same value for the rest.
	 Quick hack while other things are worked on.  */

      /* VIF table.  */

      for (i = vif_opcodes_count - 2; i >= 0; --i)
	{
	  vif_opcodes[i].next_asm = & vif_opcodes[i+1];
	  vif_opcodes[i].next_dis = & vif_opcodes[i+1];
	}

      /* DMA table.  */

      for (i = dma_opcodes_count - 2; i >= 0; --i)
	{
	  dma_opcodes[i].next_asm = & dma_opcodes[i+1];
	  dma_opcodes[i].next_dis = & dma_opcodes[i+1];
	}

      /* GIF table.  */

      for (i = gif_opcodes_count - 2; i >= 0; --i)
	{
	  gif_opcodes[i].next_asm = & gif_opcodes[i+1];
	  gif_opcodes[i].next_dis = & gif_opcodes[i+1];
	}

      init_p = 1;
    }
}

/* Return the first insn in the chain for assembling upper INSN.  */

const dvp_opcode *
vu_upper_opcode_lookup_asm (insn)
     const char *insn;
{
  return upper_opcode_map[DVP_HASH_UPPER_OPCODE (insn)];
}

/* Return the first insn in the chain for disassembling upper INSN.  */

const dvp_opcode *
vu_upper_opcode_lookup_dis (insn)
     DVP_INSN insn;
{
  return upper_icode_map[DVP_HASH_UPPER_ICODE (insn)];
}

/* Return the first insn in the chain for assembling lower INSN.  */

const dvp_opcode *
vu_lower_opcode_lookup_asm (insn)
     const char *insn;
{
  return lower_opcode_map[DVP_HASH_LOWER_OPCODE (insn)];
}

/* Return the first insn in the chain for disassembling lower INSN.  */

const dvp_opcode *
vu_lower_opcode_lookup_dis (insn)
     DVP_INSN insn;
{
  return lower_icode_map[DVP_HASH_LOWER_ICODE (insn)];
}

/* Return the first insn in the chain for assembling lower INSN.  */

const dvp_opcode *
vif_opcode_lookup_asm (insn)
     const char *insn;
{
  return &vif_opcodes[0];
}

/* Return the first insn in the chain for disassembling lower INSN.  */

const dvp_opcode *
vif_opcode_lookup_dis (insn)
     DVP_INSN insn;
{
  return &vif_opcodes[0];
}

/* Return the first insn in the chain for assembling lower INSN.  */

const dvp_opcode *
dma_opcode_lookup_asm (insn)
     const char *insn;
{
  return &dma_opcodes[0];
}

/* Return the first insn in the chain for disassembling lower INSN.  */

const dvp_opcode *
dma_opcode_lookup_dis (insn)
     DVP_INSN insn;
{
  return &dma_opcodes[0];
}

/* Return the first insn in the chain for assembling lower INSN.  */

const dvp_opcode *
gif_opcode_lookup_asm (insn)
     const char *insn;
{
  return &gif_opcodes[0];
}

/* Return the first insn in the chain for disassembling lower INSN.  */

const dvp_opcode *
gif_opcode_lookup_dis (insn)
     DVP_INSN insn;
{
  return &gif_opcodes[0];
}

/* Misc. utilities.  */

/* Scan a symbol and return a pointer to one past the end.  */

static char *
scan_symbol (sym)
     char *sym;
{
  while (*sym && issymchar (*sym))
    ++sym;
  return sym;
}

/* Given a keyword, look up its value, or -1 if not found.  */

static int
lookup_keyword_value (table, name, case_sensitive_p)
     const keyword *table;
     const char *name;
     int case_sensitive_p;
{
  const keyword *p;

  if (case_sensitive_p)
    {
      for (p = table; p->name; ++p)
	if (strcmp (name, p->name) == 0)
	  return p->value;
    }
  else
    {
      for (p = table; p->name; ++p)
	if (strcasecmp (name, p->name) == 0)
	  return p->value;
    }

  return -1;
}

/* Given a keyword's value, look up its name, or NULL if not found.  */

static const char *
lookup_keyword_name (table, value)
     const keyword *table;
     int value;
{
  const keyword *p;

  for (p = table; p->name; ++p)
    if (value == p->value)
      return p->name;

  return NULL;
}

/* Macro insn support.  */

/* Given a string, see if it's a macro insn and return the expanded form.
   If not a macro insn, return NULL.
   The expansion is done in malloc'd space.
   It is up to the caller to free it.  */

char *
dvp_expand_macro (mactable, tabsize, insn)
     const dvp_macro * mactable;
     int tabsize;
     char *insn;
{
  const dvp_macro * m, *mend;
  char * operands[10];
  int oplens[10];
  int noperands = 0;

  for (m = mactable, mend = mactable + tabsize; m < mend; ++m)
    {
      const char * p = m->template;
      char * ip = insn;

      for (;;)
	{
	  while (*p && *p == *ip)
	    ++p, ++ip;

	  /* Did we find a complete match?  */
	  if (*p == 0 && *ip == 0)
	    {
	      int total_len = strlen (m->result);
	      int i;
	      char * result;

	      for (i = 0; i < noperands; ++i)
		total_len += strlen (operands[i]);
	      total_len += 1 + 10  /* slop */;
	      result = xmalloc (total_len);
	      for (ip = result, p = m->result; *p; )
		{
		  if (*p == '%')
		    {
		      /* Ok, we shouldn't assume max 10 operands.  */
		      int opnum = *++p - '0';
		      memcpy (ip, operands[opnum], oplens[opnum]);
		      ++p;
		      ip += oplens[opnum];
		    }
		  else
		    {
		      *ip++ = *p++;
		    }
		}
	      if (ip - result >= total_len)
		abort ();
	      *ip = 0;
	      return result;
	    }

	  /* Is this an operand?  */
	  if (*p == '$' && p[1] == '{')
	    {
	      if (strncmp (p + 2, "imrubits}", 9) == 0)
		{
		  if (*ip == '[')
		    {
		      char *q = ip;
		      while (*q && *q != ']')
			++q;
		      if (! *q)
			return NULL;
		      ++q;
		      operands[noperands] = ip;
		      oplens[noperands++] = q - ip;
		      ip = q;
		    }
		  else
		    {
		      operands[noperands] = "";
		      oplens[noperands++] = 0;
		    }
		}
	      else if (strncmp (p + 2, "wl}", 3) == 0
		       || strncmp (p + 2, "cl}", 3) == 0
		       || strncmp (p + 2, "unpacktype}", 11) == 0
		       || strncmp (p + 2, "unpackloc}", 10) == 0
		       || strncmp (p + 2, "datalen}", 8) == 0)
		{
		  char *q = ip;
		  while (*q && *q != ',')
		    ++q;
		  operands[noperands] = ip;
		  oplens[noperands++] = q - ip;
		  ip = q;
		}
	      else
		abort ();

	      /* Skip to end of operand in template.  */
	      p = strchr (p, '}');
	      if (! p)
		abort ();
	      ++p;
	    }
	  else
	    break;
	}
    }

  return NULL;
}
