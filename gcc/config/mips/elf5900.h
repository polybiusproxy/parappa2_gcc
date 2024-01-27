/* CYGNUS LOCAL: Entire file.  */

/* Definitions of target machine for GNU compiler.
   Toshiba r5900 little-endian
   Copyright (c) 1995 Cygnus Support Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#define MIPS_CPU_STRING_DEFAULT "R5900"

/* We use the MIPS EABI by default.  */
#define MIPS_ABI_DEFAULT ABI_EABI

/* The r5900 only has a 32bit address space.  */
#define Pmode SImode
#define POINTER_SIZE 32

/* We can't do integer arithmetic in anything wider than DImode.  */
#define MAX_INTEGER_COMPUTATION_MODE DImode

/* Define default alignments for assorted jump targets.  We're slightly more
   concerned about performance than code space on the r5900, so we increase
   the default alignment of loops and functions.  Note the user can override
   these defaults on the command line.  */
#define DEFAULT_LOOP_ALIGN 3
#define DEFAULT_LOOP_MAX_SKIP 0
#define DEFAULT_JUMP_ALIGN 2
#define DEFAULT_JUMP_MAX_SKIP 0
#define DEFAULT_FUNCTION_ALIGN 3
#define DEFAULT_FUNCTION_MAX_SKIP 0

#include "mips/elfl64.h"
#include "mips/abi64.h"

/* Override default STACK_BOUNDARY and MIPS_STACK_ALIGN definitions
   for 128bit lq/sq support.  */
#undef STACK_BOUNDARY
#define STACK_BOUNDARY 					\
  ((mips_abi == ABI_32 					\
    || mips_abi == ABI_O64 				\
    || (mips_abi == ABI_EABI && !TARGET_MIPS5900))	\
   ? 64 : 128)

#undef MIPS_STACK_ALIGN
#define MIPS_STACK_ALIGN(LOC) \
  ((mips_abi == ABI_32 					\
    || mips_abi == ABI_O64				\
    || (mips_abi == ABI_EABI && !TARGET_MIPS5900))	\
   ? ((LOC) + 7) & ~7					\
   : ((LOC) + 15) & ~15)

/* We must override BIGGEST_ALIGNMENT since we support TImode loads/stores
   which must be properly aligned.  */
#undef BIGGEST_ALIGNMENT
#define BIGGEST_ALIGNMENT (TARGET_MIPS5900 ? 128 : 64)

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dmips -DMIPSEL -DR5900 -D_mips -D_MIPSEL -D_R5900"

#undef SUBTARGET_CPP_SIZE_SPEC
#define SUBTARGET_CPP_SIZE_SPEC "\
-D__SIZE_TYPE__=\\ unsigned\\ int -D__PTRDIFF_TYPE__=\\ int"

#undef SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC "\
%{!mips1:%{!mips2:-U__mips -D__mips=3 -D__mips64}}\
%{!mabi=32: %{!mabi=n32: %{!mabi=64: -D__mips_eabi}}}\
%{!msoft-float: %{!mdouble-float : -D__mips_single_float}}"

#undef MIPS_ISA_DEFAULT
#define MIPS_ISA_DEFAULT 3

/* How to output a quadword for the r5900.  */
#define ASM_OUTPUT_QUADRUPLE_INT(STREAM,VALUE)				\
do {									\
  if (TARGET_64BIT)							\
    {									\
      fprintf (STREAM, "\t.octa\t");					\
      if (HOST_BITS_PER_WIDE_INT < 64 || GET_CODE (VALUE) != CONST_INT)	\
	/* We can't use 'X' for negative numbers, because then we won't	\
	   get the right value for the upper 32 bits.  */		\
        output_addr_const (STREAM, VALUE);				\
      else								\
	/* We must use 'X', because otherwise LONG_MIN will print as	\
	   a number that the Irix 6 assembler won't accept.  */		\
        print_operand (STREAM, VALUE, 'X');				\
      fprintf (STREAM, "\n");						\
    }									\
  else									\
    {									\
      assemble_integer (operand_subword ((VALUE), 0, 0, TImode),	\
			UNITS_PER_WORD, 1);				\
      assemble_integer (operand_subword ((VALUE), 1, 0, TImode),	\
			UNITS_PER_WORD, 1);				\
    }									\
} while (0)

/* We want to include alignment directives for the r5900 to ensure that
   TImode values are properly aligned.  It would be best to do this in
   mips.h, but it's unclear if all mips assemblers can handle alignments
   for local common/bss objects.  */
#undef ASM_OUTPUT_LOCAL

/* This says how to output an assembler line to define a global common symbol
   with size SIZE (in bytes) and alignment ALIGN (in bits).  */
#define ASM_OUTPUT_ALIGNED_COMMON(STREAM, NAME, SIZE, ALIGN)		\
do									\
{									\
  mips_declare_object (STREAM, NAME, "\n\t.comm\t", ",%u,", (SIZE));	\
  fprintf ((STREAM), "%d\n", ((ALIGN) / BITS_PER_UNIT));  	\
}									\
while (0)

#define POPSECTION_ASM_OP       "\t.previous"
#define BSS_SECTION_ASM_OP      "\t.section\t.bss"
#define SBSS_SECTION_ASM_OP     "\t.section\t.sbss"
#define ASM_OUTPUT_ALIGNED_LOCAL(STREAM, NAME, SIZE, ALIGN)		\
do									\
  {									\
    if ((SIZE) > 0 && (SIZE) <= mips_section_threshold)			\
      fprintf (STREAM, "%s\n", SBSS_SECTION_ASM_OP);			\
    else				       				\
      fprintf (STREAM, "%s\n", BSS_SECTION_ASM_OP);			\
    mips_declare_object (STREAM, NAME, "", ":\n", 0);		   	\
    ASM_OUTPUT_ALIGN (STREAM, floor_log2 (ALIGN / BITS_PER_UNIT));	\
    ASM_OUTPUT_SKIP (STREAM, SIZE);					\
    fprintf (STREAM, "%s\n", POPSECTION_ASM_OP);			\
  }									\
while (0)

/* Avoid returning unaligned structures > 64bits, but <= 128bits wide
   in registers.

   This avoids a bad interaction between the code to return BLKmode
   structures in registers and the wider than word_mode registers
   found on the r5900.

   This does not effect returning an aligned 128bit value in a register,
   which should work.  */
#undef RETURN_IN_MEMORY
#define RETURN_IN_MEMORY(TYPE)						\
  ((mips_abi == ABI_32 || mips_abi == ABI_O64)				\
   ? TYPE_MODE (TYPE) == BLKmode					\
   : (mips_abi == ABI_EABI && TYPE_MODE (TYPE) == BLKmode		\
      ? (int_size_in_bytes (TYPE) > UNITS_PER_WORD)			\
      : (int_size_in_bytes (TYPE)					\
	 > (mips_abi == ABI_EABI ? 2 * UNITS_PER_WORD : 16))))
