/* Opcode table for the DVP.
   Copyright 1998 Free Software Foundation, Inc.

This file is part of GAS, the GNU Assembler, GDB, the GNU debugger, and
the GNU Binutils.

GAS/GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GAS/GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GAS or GDB; see the file COPYING.	If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Enum describing each processing component.
   In the case where one wants to specify DVP_VU, use DVP_VUUP.  */
typedef enum {
  DVP_UNKNOWN, DVP_DMA, DVP_VIF, DVP_GIF, DVP_VUUP, DVP_VULO
} dvp_cpu;

/* Type to denote a DVP instruction (at least a 32 bit unsigned int).  */
typedef unsigned int DVP_INSN;

/* Maximum number of operands and syntax chars an instruction can have.  */
#define DVP_MAX_OPERANDS 16

typedef struct dvp_opcode {
  char *mnemonic;
  /* The value stored is 128 + operand number.
     This allows ASCII chars to go here as well.  */
  unsigned char syntax[DVP_MAX_OPERANDS];
  DVP_INSN mask, value;	/* recognize insn if (op&mask)==value */
  unsigned opcode_word;	/* opcode word to contain contain "value"; usually 0 */
		      	/* (definition of a word is target specific) */
  int flags;		/* various flag bits */

/* Values for `flags'.  */

/* This insn is a conditional branch.  */
#define DVP_OPCODE_COND_BRANCH 1
/* Ignore this insn during disassembly.  */
#define DVP_OPCODE_IGNORE_DIS 2
/* CPU specific values begin at 0x10.  */

  /* These values are used to optimize assembly and disassembly.  Each insn is
     on a list of related insns (same first letter for assembly, same insn code
     for disassembly).  */
  /* FIXME: May wish to move this to separate table.  */
  struct dvp_opcode *next_asm;	/* Next instruction to try during assembly.  */
  struct dvp_opcode *next_dis;	/* Next instruction to try during disassembly.  */

  /* Macros to create the hash values for the lists.  */
#define DVP_HASH_UPPER_OPCODE(string) \
  (tolower ((string)[0]) >= 'a' && tolower ((string)[0]) <= 'z' \
   ? tolower ((string)[0]) - 'a' : 26)
#define DVP_HASH_LOWER_OPCODE(string) \
  (tolower ((string)[0]) >= 'a' && tolower ((string)[0]) <= 'z' \
   ? tolower ((string)[0]) - 'a' : 26)
/* ??? The icode hashing is very simplistic.
   upper: bits 0x3c, can't use lower two bits because of bc field
   lower: upper 6 bits  */
#define DVP_ICODE_HASH_SIZE 6 /* bits */
#define DVP_HASH_UPPER_ICODE(insn) \
  ((insn) & 0x3c)
#define DVP_HASH_LOWER_ICODE(insn) \
  ((((insn) & 0xfc) >> 26) & 0x3f)

  /* Macros to access `next_asm', `next_dis' so users needn't care about the
     underlying mechanism.  */
#define DVP_OPCODE_NEXT_ASM(op) ((op)->next_asm)
#define DVP_OPCODE_NEXT_DIS(op) ((op)->next_dis)
} dvp_opcode;

/* The operand table.  */

typedef struct dvp_operand {
  /* The number of bits in the operand (may be unused for a modifier).  */
  unsigned char bits;

  /* How far the operand is left shifted in the instruction, or
     the modifier's flag bit (may be unused for a modifier).  */
  unsigned char shift;

  /* An index to the instruction word which will contain the operand value.
     Usually 0.  */
  unsigned char word;

  /* Various flag bits.  */
  int flags;

/* Values for `flags'.  */

/* This operand is a suffix to the opcode.  */
#define DVP_OPERAND_SUFFIX 1

/* This operand is a relative branch displacement.  The disassembler
   prints these symbolically if possible.  */
#define DVP_OPERAND_RELATIVE_BRANCH 2

/* This operand is an absolute branch address.  The disassembler
   prints these symbolically if possible.  */
#define DVP_OPERAND_ABSOLUTE_BRANCH 4

/* This operand is a mips address.  The disassembler
   prints these symbolically if possible.  */
#define DVP_OPERAND_MIPS_ADDRESS 8

/* This operand is a vu address.  The disassembler
   prints these symbolically if possible.  */
#define DVP_OPERAND_VU_ADDRESS 0x10

/* This operand takes signed values (default is unsigned).
   The default was chosen to be unsigned as most fields are unsigned
   (e.g. registers).  */
#define DVP_OPERAND_SIGNED 0x20

/* This operand takes signed values, but also accepts a full positive
   range of values.  That is, if bits is 16, it takes any value from
   -0x8000 to 0xffff.  */
#define DVP_OPERAND_SIGNOPT 0x40

/* This operand should be regarded as a negative number for the
   purposes of overflow checking (i.e., the normal most negative
   number is disallowed and one more than the normal most positive
   number is allowed).  This flag will only be set for a signed
   operand.  */
#define DVP_OPERAND_NEGATIVE 0x80

/* This operand doesn't really exist.  The program uses these operands
   in special ways by creating insertion or extraction functions to have
   arbitrary processing performed during assembly/disassemble.
   Parse and print routines are ignored for FAKE operands.  */
#define DVP_OPERAND_FAKE 0x100

/* This operand is the address of the `unpack' insn.  */
#define DVP_OPERAND_UNPACK_ADDRESS 0x200

/* Inline data.  */
#define DVP_OPERAND_DMA_INLINE 0x10000

/* Pointer to the data.  */
#define DVP_OPERAND_DMA_ADDR 0x20000

/* Pointer to the data.  */
#define DVP_OPERAND_DMA_NEXT 0x40000

/* The actual count operand.  */
#define DVP_OPERAND_DMA_COUNT 0x80000

/* A 32 bit floating point immediate.  */
#define DVP_OPERAND_FLOAT 0x100000

/* An 11-bit immediate operand.  May be a label.  */
#define DVP_OPERAND_RELOC_11_S4 0x200000

/* An 15-bit unsigned immediate operand.  May be a label.  */
#define DVP_OPERAND_RELOC_U15_S3 0x400000

/* Modifier values.  */

/* A dot is required before a suffix.  e.g. .le  */
/* ??? Not currently used.  */
#define DVP_MOD_DOT 0x1000000

/* The count operand was an asterisk.  */
#define DVP_OPERAND_AUTOCOUNT 0x2000000

/* Ignore the word part of any shift and operate on the "first" word
   of the instruction.  */
#define DVP_MOD_THIS_WORD 0x4000000

/* Sum of all DVP_MOD_XXX bits.  */
#define DVP_MOD_BITS 0xff000000

/* Non-zero if the operand type is really a modifier.  */
#define DVP_MOD_P(X) ((X) & DVP_MOD_BITS)

  /* Parse function.  This is used by the assembler.
     MODS is a list of modifiers preceding the operand in the syntax string.
     If the operand cannot be parsed an error message is stored in ERRMSG,
     otherwise ERRMSG is unchanged.  */
  long (*parse) PARAMS ((const struct dvp_opcode *opcode,
			 const struct dvp_operand *operand,
			 int mods, char **str, const char **errmsg));

  /* Insertion function.  This is used by the assembler.  To insert an
     operand value into an instruction, check this field.

     If it is NULL, execute
         i |= (p & ((1 << o->bits) - 1)) << o->shift;
     (I is the instruction which we are filling in, O is a pointer to
     this structure, and OP is the opcode value; this assumes twos
     complement arithmetic).

     If this field is not NULL, then simply call it with the
     instruction and the operand value.  It will overwrite the appropriate
     bits of the instruction with the operand's value.
     MODS is a list of modifiers preceding the operand in the syntax string.
     If the ERRMSG argument is not NULL, then if the operand value is illegal,
     *ERRMSG will be set to a warning string (the operand will be inserted in
     any case).  If the operand value is legal, *ERRMSG will be unchanged.
     OPCODE may be NULL, in which case the value isn't known.  This happens
     when applying fixups.  */

  void (*insert) PARAMS ((const struct dvp_opcode *opcode,
			  const struct dvp_operand *operand,
			  int mods, DVP_INSN *insn,
			  long value, const char **errmsg));

  /* Extraction function.  This is used by the disassembler.  To
     extract this operand type from an instruction, check this field.

     If it is NULL, compute
         op = ((i) >> o->shift) & ((1 << o->bits) - 1);
	 if ((o->flags & DVP_OPERAND_SIGNED) != 0
	     && (op & (1 << (o->bits - 1))) != 0)
	   op -= 1 << o->bits;
     (I is the instruction, O is a pointer to this structure, and OP
     is the result; this assumes twos complement arithmetic).

     If this field is not NULL, then simply call it with the
     instruction value.  It will return the value of the operand.  If
     the INVALID argument is not NULL, *INVALID will be set to
     non-zero if this operand type can not actually be extracted from
     this operand (i.e., the instruction does not match).  If the
     operand is valid, *INVALID will not be changed.
     MODS is a list of modifiers preceding the operand in the syntax string.

     INSN is a pointer to one or two `DVP_INSN's.  The first element is
     the insn, the second is an immediate constant if present.
     FIXME: just thrown in here for now.
     */

  long (*extract) PARAMS ((const struct dvp_opcode *opcode,
			   const struct dvp_operand *operand,
			   int mods, DVP_INSN *insn, int *pinvalid));

  /* Print function.  This is used by the disassembler.  */
  void (*print) PARAMS ((const struct dvp_opcode *opcode,
			 const struct dvp_operand *operand,
			 int mods, DVP_INSN *insn,
			 disassemble_info *info, long value));
} dvp_operand;

/* Given an operand entry, return the table index.  */
#define DVP_OPERAND_INDEX(op) ((op) - 128)

/* Macro support.  */

typedef struct dvp_macro {
  const char *template;
  const char *result;
} dvp_macro;

/* Expand an instruction if it is a macro, else NULL.  */
extern char * dvp_expand_macro PARAMS ((const dvp_macro *, int, char *));

/* VU support.  */

/* Flag values.
   The actual value stored in the insn is left shifted by 27.  */
#define VU_FLAG_I 16
#define VU_FLAG_E 8
#define VU_FLAG_M 4
#define VU_FLAG_D 2
#define VU_FLAG_T 1

/* Positions, masks, and values of various fields used in multiple places
   (the opcode table, the disassembler, GAS).  */
#define VU_SHIFT_DEST 21
#define VU_SHIFT_TREG 16
#define VU_SHIFT_SREG 11
#define VU_SHIFT_DREG 6
#define VU_MASK_REG 31
/* Bits for multiple dest choices.  */
#define VU_DEST_X 8
#define VU_DEST_Y 4
#define VU_DEST_Z 2
#define VU_DEST_W 1
/* Values for a single dest choice.  */
#define VU_SDEST_X 0
#define VU_SDEST_Y 1
#define VU_SDEST_Z 2
#define VU_SDEST_W 3

extern const dvp_operand vu_operands[];
extern /*const*/ dvp_opcode vu_upper_opcodes[];
extern /*const*/ dvp_opcode vu_lower_opcodes[];
extern const int vu_upper_opcodes_count;
extern const int vu_lower_opcodes_count;

const dvp_opcode *vu_upper_opcode_lookup_asm PARAMS ((const char *));
const dvp_opcode *vu_lower_opcode_lookup_asm PARAMS ((const char *));
const dvp_opcode *vu_upper_opcode_lookup_dis PARAMS ((unsigned int));
const dvp_opcode *vu_lower_opcode_lookup_dis PARAMS ((unsigned int));

/* VIF support.  */

/* VIF opcode flags.
   The usage here is a bit wasteful of bits, but there's enough bits
   and we can always make better usage later.
   We begin at 0x10 because the lower 4 bits are reserved for
   general opcode flags.  */

/* 2 word instruction */
#define VIF_OPCODE_LEN2 0x10
/* 5 word instruction */
#define VIF_OPCODE_LEN5 0x20
/* variable length instruction */
#define VIF_OPCODE_LENVAR 0x40
/* the mpg instruction */
#define VIF_OPCODE_MPG 0x80
/* the direct instruction */
#define VIF_OPCODE_DIRECT 0x100
/* the directhl instruction */
#define VIF_OPCODE_DIRECTHL 0x200
/* the unpack instruction */
#define VIF_OPCODE_UNPACK 0x400

/* Instruction flag bits.  M,R,U are only applicable to `unpack'.
   These aren't the actual bit numbers.  They're for internal use.
   The insert/extract handlers do the appropriate conversions.  */
#define VIF_FLAG_I 1
#define VIF_FLAG_M 2
#define VIF_FLAG_R 4
#define VIF_FLAG_U 8

/* The "mode" operand of the "stmod" insn.  */
#define VIF_MODE_DIRECT 0
#define VIF_MODE_ADD 1
#define VIF_MODE_ADDROW 2

/* Unpack types.  */
typedef enum {
  VIF_UNPACK_S_32 = 0,
  VIF_UNPACK_S_16 = 1,
  VIF_UNPACK_S_8 = 2,
  VIF_UNPACK_UNUSED3 = 3,
  VIF_UNPACK_V2_32 = 4,
  VIF_UNPACK_V2_16 = 5,
  VIF_UNPACK_V2_8 = 6,
  VIF_UNPACK_UNUSED7 = 7,
  VIF_UNPACK_V3_32 = 8,
  VIF_UNPACK_V3_16 = 9,
  VIF_UNPACK_V3_8 = 10,
  VIF_UNPACK_UNUSED11 = 11,
  VIF_UNPACK_V4_32 = 12,
  VIF_UNPACK_V4_16 = 13,
  VIF_UNPACK_V4_8 = 14,
  VIF_UNPACK_V4_5 = 15
} unpack_type;

extern const dvp_operand vif_operands[];
extern /*const*/ dvp_opcode vif_opcodes[];
extern const int vif_opcodes_count;
extern const dvp_macro vif_macros[];
extern const int vif_macro_count;
const dvp_opcode *vif_opcode_lookup_asm PARAMS ((const char *));
const dvp_opcode *vif_opcode_lookup_dis PARAMS ((unsigned int));

/* Return length, in 32 bit words, of just parsed vif insn,
   or 0 if unknown.  */
int vif_len PARAMS ((void));

/* Given the first word of a VIF insn, return its length.  */
int vif_insn_len PARAMS ((DVP_INSN, dvp_cpu *));

/* Return the length value to use for an unpack instruction.  */
int vif_unpack_len_value PARAMS ((unpack_type, int, int, int));

/* Return the length, in words, of an unpack insn.  */
int vif_unpack_len PARAMS ((unpack_type, int));

/* Fetch user data for variable length insns.  */
void vif_get_var_data PARAMS ((const char **, int *));

/* Fetch the current values of wl,cl.  */
void vif_get_wl_cl PARAMS ((int *, int *));

/* Various operand numbers.  */
extern const int vif_operand_mpgloc;
extern const int vif_operand_datalen_special;

/* DMA support.  */

/* DMA instruction flags.  */
#define DMA_FLAG_PCE0 1
#define DMA_FLAG_PCE1 2
#define DMA_FLAG_INT 4
#define DMA_FLAG_SPR 8

extern const dvp_operand dma_operands[];
extern /*const*/ dvp_opcode dma_opcodes[];
extern const int dma_opcodes_count;
const dvp_opcode *dma_opcode_lookup_asm PARAMS ((const char *));
const dvp_opcode *dma_opcode_lookup_dis PARAMS ((unsigned int));
int dvp_dma_operand_autocount PARAMS ((int));

/* Various operand numbers.  */
extern const int dma_operand_count;
extern const int dma_operand_addr;

/* GIF support.  */

/* Maximum value for nloop.  */
#define GIF_MAX_NLOOP 32767

/* The PRE bit in the appropriate word in a tag.  */
#define GIF_PRE (1 << 14)

/* The values here correspond to the values in the instruction.  */
typedef enum { GIF_PACKED = 0, GIF_REGLIST = 1, GIF_IMAGE = 2 } gif_type;

typedef enum {
  GIF_REG_PRIM = 0,
  GIF_REG_RGBAQ = 1,
  GIF_REG_ST = 2,
  GIF_REG_UV = 3,
  GIF_REG_XYZF2 = 4,
  GIF_REG_XYZ2 = 5,
  GIF_REG_TEX0_1 = 6,
  GIF_REG_TEX0_2 = 7,
  GIF_REG_CLAMP_1 = 8,
  GIF_REG_CLAMP_2 = 9,
  GIF_REG_XYZF = 10,
  GIF_REG_UNUSED11 = 11, /* 11 is unused */
  GIF_REG_XYZF3 = 12,
  GIF_REG_XYZ3 = 13,
  GIF_REG_A_D = 14,
  GIF_REG_NOP = 15
} gif_reg;

extern const dvp_operand gif_operands[];
extern /*const*/ dvp_opcode gif_opcodes[];
extern const int gif_opcodes_count;
const dvp_opcode *gif_opcode_lookup_asm PARAMS ((const char *));
const dvp_opcode *gif_opcode_lookup_dis PARAMS ((unsigned int));
extern int gif_nloop PARAMS ((void));
extern int gif_nregs PARAMS ((void));

/* Various operand numbers.  */
extern const int gif_operand_nloop;

/* Utility fns in dvp-opc.c.  */
void dvp_opcode_init_tables PARAMS ((int));
void dvp_opcode_init_parse PARAMS ((void));
void dvp_opcode_init_print PARAMS ((void));
