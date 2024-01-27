/* Copyright (C) 1997 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define TARGET_BYTE_ORDER_SELECTABLE_P 1
#define TARGET_MONITOR_PROMPT "<RISQ> "
#define MIPS_EABI 1
#define MIPS_DEFAULT_FPU_TYPE MIPS_FPU_SINGLE

/* Even though the r5900 is a MIPS III chip and uses the EABI, it only
   has a 32bit address space.  */
#define TARGET_PTR_BIT       32

#include "mips/tm-mips64.h"

/* for 5900, add a bunch of 64-bit registers that are really the upper */

/* Disable the use of floating point registers for function arguments.  */
#undef  MIPS_LAST_FP_ARG_REGNUM
#define MIPS_LAST_FP_ARG_REGNUM (FPA0_REGNUM-1)
/* 64 bits of the 128-bit registers */

#define FIRST_COP0_REG 128

#undef NUM_REGS
#define NUM_REGS 150 /* 5900 */

#undef REGISTER_NAMES
#define REGISTER_NAMES 	\
    {	/*  0*/	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3", \
	/*  8*/	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7", \
	/* 16*/	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7", \
	/* 24*/	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra", \
	/* 32*/	"sr",	"lo",	"hi",	"bad",	"cause","pc",    \
	/* 38*/	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7", \
	/* 46*/	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15", \
	/* 54*/	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23", \
	/* 62*/	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31", \
	/* 70*/	"fsr",  "fir",  "fp",	"", \
	/* 74*/	"",	"",	"",	"",	"",	"",	"",	"", \
	/* 82*/	"",	"",	"",	"",	"",	"",	"",	"", \
	/* 90*/	"zeroh","ath",	"v0h",	"v1h",	"a0h",	"a1h",	"a2h",	"a3h", \
	/* 98*/	"t0h",	"t1h",	"t2h",	"t3h",	"t4h",	"t5h",	"t6h",	"t7h", \
	/*106*/	"s0h",	"s1h",	"s2h",	"s3h",	"s4h",	"s5h",	"s6h",	"s7h", \
	/*114*/	"t8h",	"t9h",	"k0h",	"k1h",	"gph",	"sph",	"s8h",	"rah", \
	/*122*/ "loh",	"hih",	"sa",	"",	"",    "", "index", "random", \
	/*130*/ "entlo0", "entlo1", "context", "pgmsk", "wired", "", "badva", "count", \
	/*138*/ "enthi", "compare", "", "epc", "", "prid", "config", "debug", \
	/*146*/  "perf", "taglo", "taghi", "err_epc", \
    }


#define SR_REGNUM 32

#undef REGISTER_VIRTUAL_TYPE
#define REGISTER_VIRTUAL_TYPE(N) \
(((N) >= FP0_REGNUM && (N) < FP0_REGNUM + 32) ? builtin_type_float \
 : ((N) == SR_REGNUM) ? builtin_type_uint32 \
 : ((N) >= 70 && (N) <= 89) ? builtin_type_uint32 \
 : ((N) >= FIRST_COP0_REG) ? builtin_type_uint32 \
 : builtin_type_int64 )

#undef REGISTER_VIRTUAL_SIZE
#define REGISTER_VIRTUAL_SIZE(N) TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (N))

#undef REGISTER_RAW_SIZE
#define REGISTER_RAW_SIZE(N) REGISTER_VIRTUAL_SIZE(N)

/* Select the proper disassembler */

#undef TM_PRINT_INSN_MACH
#define TM_PRINT_INSN_MACH bfd_mach_mips5900

/* The r5900 has 128bit wide GPRs; the compiler will use 128bit
   stores/loads in the prologue/epilogue.  However, MIPS_REG_SIZE
   still specifies a 64bit wide register.  So we have to compensate
   for this difference.

   Long term we need to expose all 128bits of the GPRs to GDB in
   the normal way.

   Also note, this definition will cause massive problems if one
   tries to debug code which only saves 64bit wide GPRs in the
   prologue.  The way to handle this is to actually decode the
   instruction which saves the register and take appropriate
   action based on the size of the store instruction.  */
#define R5900_128BIT_GPR_HACK

