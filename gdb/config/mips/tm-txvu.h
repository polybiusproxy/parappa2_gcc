/* Definitions for the Toshiba R5900/Vector Unit target
   Copyright 1998, Free Software Foundation, Inc.
   MIPs definitions contributed by Per Bothner (bothner@cs.wisc.edu)
   and by Alessandro Forin (af@cs.cmu.edu) at CMU..

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

#ifndef TM_TXVU_H
#define TM_TXVU_H 1

#define TARGET_BYTE_ORDER_DEFAULT LITTLE_ENDIAN

#define MIPS_DEFAULT_FPU_TYPE MIPS_FPU_SINGLE

#define MIPS_EABI 1

/* Even though the r5900 is a MIPS III chip and uses the EABI, it only
   has a 32bit address space.  */
#define TARGET_PTR_BIT       32

#include "mips/tm-mips64.h"

/* The current CPU context. Processor specific operations are
   directed at the current CPU. TXVU_CPU_AUTO defaults the focus
   to the last signal source, eg. the CPU that hit the last break
   point. */

enum txvu_cpu_context
{
  TXVU_CPU_AUTO		= -1,	/* context-sensitive context */
  TXVU_CPU_MASTER	= 0,	/* R5900 core */
  TXVU_CPU_VU0		= 1,	/* Vector units */
  TXVU_CPU_VU1		= 2,
  TXVU_CPU_VIF0		= 3,	/* FIFO's */
  TXVU_CPU_VIF1		= 4,
  TXVU_CPU_LAST
};

#define TXVU_CPU_NUM	6 /* Count of context types */

extern enum txvu_cpu_context txvu_cpu;

#define TXVU_SET_CPU_HELP_STRING	\
		   "Set CPU context.\n\
Processor specific commands (such as register display) will be relative\n\
to this context. Legal selections are:\n\
    auto: context is relative to last signal source (eg. breakpoint).\n\
    master: context is relative to the main processor.\n\
    vu0,vu1 : context is relative to vector unit 0 or 1.\n\
    vif0,vif1 : context is relative to VIF unit 0 or 1.\n"

/* memory segment for communication between GDB and SIM */
#define GDB_COMM_AREA	0xb9810000
#define GDB_COMM_SIZE	0x4000

/* Memory address containing last device to execute */
#define LAST_DEVICE	GDB_COMM_AREA

/* The FIFO breakpoint count and table */
#define FIFO_BPT_CNT	(GDB_COMM_AREA + 4)
#define FIFO_BPT_TBL	(GDB_COMM_AREA + 8)

/* Each element of the breakpoint table is three four-byte integers. */
#define BPT_ELEM_SZ	4*3

/* Read the PC (context-sensitive) */
CORE_ADDR txvu_read_pc_pid PARAMS((int pid));
#define TARGET_READ_PC(PID) txvu_read_pc_pid (pid)

/* Read/write the SP/FP (context-sensitive) */
#undef TARGET_READ_SP
#define TARGET_READ_SP() txvu_read_sp ()
CORE_ADDR txvu_read_sp PARAMS((void));

void txvu_write_sp PARAMS((CORE_ADDR));
#define TARGET_WRITE_SP(VAL) txvu_write_sp (VAL)

/* The VUs and VIFs don't have frame pointers */
#define TARGET_READ_FP() \
    ((txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) ? \
        read_register (FP_REGNUM) : 0)

#define TARGET_WRITE_FP(VAL) { \
    if (txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) \
      write_register (FP_REGNUM, VAL); \
  }

extern int txvu_step_skips_delay PARAMS ((CORE_ADDR));
#undef STEP_SKIPS_DELAY
#define STEP_SKIPS_DELAY(pc) txvu_step_skips_delay (pc)

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#undef SAVED_PC_AFTER_CALL
#define SAVED_PC_AFTER_CALL(frame) txvu_call_return_address ()
extern CORE_ADDR txvu_call_return_address PARAMS ((void));

#undef SKIP_PROLOGUE
#define SKIP_PROLOGUE(pc) {	\
      if (txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) \
	pc = mips_skip_prologue (pc, 0); \
    }

#define TXVU_VU_BRK_MASK 0x02	/* Breakpoint bit is #57 for VU insns */
#define TXVU_VIF_BRK_MASK 0x80	/* Use interrupt bit for VIF insns */

/* Number of machine registers */
#define NUM_VU_REGS 160

#define NUM_VU_INTEGER_REGS 16
#define FIRST_VEC_REG 32

#define NUM_VIF_REGS 26

#define NUM_CORE_REGS 150

#define FIRST_COP0_REG 128
#define NUM_COP0_REGS 22

#undef NUM_REGS
#define NUM_REGS (NUM_CORE_REGS + 2*(NUM_VU_REGS) + 2*(NUM_VIF_REGS))

#define VU0_PC_REGNUM	(NUM_CORE_REGS + NUM_VU_INTEGER_REGS)
#define VU1_PC_REGNUM	(NUM_CORE_REGS + NUM_VU_INTEGER_REGS + NUM_VU_REGS)
#define VIF0_PC_REGNUM  (NUM_CORE_REGS + 2*NUM_VU_REGS + NUM_VIF_REGS - 2)
#define VIF1_PC_REGNUM  (NUM_CORE_REGS + 2*NUM_VU_REGS + 2*NUM_VIF_REGS - 2)

#define VU0_SP_REGNUM	(NUM_CORE_REGS + 14)
#define VU1_SP_REGNUM	(NUM_CORE_REGS + NUM_VU_REGS + 14)

#define VU0_RA_REGNUM	(NUM_CORE_REGS + 15)
#define VU1_RA_REGNUM	(NUM_CORE_REGS + NUM_VU_REGS + 15)

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

/* for 5900, add a bunch of 64-bit registers that are really the upper
   64 bits of the 128-bit registers */

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
	"vu0_vi00", "vu0_vi01", "vu0_vi02", "vu0_vi03", \
	"vu0_vi04", "vu0_vi05", "vu0_vi06", "vu0_vi07", \
	"vu0_vi08", "vu0_vi09", "vu0_vi10", "vu0_vi11", \
	"vu0_vi12", "vu0_vi13", "vu0_vi14", "vu0_vi15", \
	"vu0_pc",   "vu0_tpc",  "vu0_stat", "vu0_sflag", \
        "vu0_mac",  "vu0_clip",	"vu0_cmsar","vu0_fbrst", \
	"vu0_r",    "",         "vu0_i",    "vu0_q", \
	"vu0_accx", "vu0_accy", "vu0_accz", "vu0_accw", \
	"vu0_vf00x", "vu0_vf00y", "vu0_vf00z", "vu0_vf00w", \
	"vu0_vf01x", "vu0_vf01y", "vu0_vf01z", "vu0_vf01w", \
	"vu0_vf02x", "vu0_vf02y", "vu0_vf02z", "vu0_vf02w", \
	"vu0_vf03x", "vu0_vf03y", "vu0_vf03z", "vu0_vf03w", \
	"vu0_vf04x", "vu0_vf04y", "vu0_vf04z", "vu0_vf04w", \
	"vu0_vf05x", "vu0_vf05y", "vu0_vf05z", "vu0_vf05w", \
	"vu0_vf06x", "vu0_vf06y", "vu0_vf06z", "vu0_vf06w", \
	"vu0_vf07x", "vu0_vf07y", "vu0_vf07z", "vu0_vf07w", \
	"vu0_vf08x", "vu0_vf08y", "vu0_vf08z", "vu0_vf08w", \
	"vu0_vf09x", "vu0_vf09y", "vu0_vf09z", "vu0_vf09w", \
	"vu0_vf10x", "vu0_vf10y", "vu0_vf10z", "vu0_vf10w", \
	"vu0_vf11x", "vu0_vf11y", "vu0_vf11z", "vu0_vf11w", \
	"vu0_vf12x", "vu0_vf12y", "vu0_vf12z", "vu0_vf12w", \
	"vu0_vf13x", "vu0_vf13y", "vu0_vf13z", "vu0_vf13w", \
	"vu0_vf14x", "vu0_vf14y", "vu0_vf14z", "vu0_vf14w", \
	"vu0_vf15x", "vu0_vf15y", "vu0_vf15z", "vu0_vf15w", \
	"vu0_vf16x", "vu0_vf16y", "vu0_vf16z", "vu0_vf16w", \
	"vu0_vf17x", "vu0_vf17y", "vu0_vf17z", "vu0_vf17w", \
	"vu0_vf18x", "vu0_vf18y", "vu0_vf18z", "vu0_vf18w", \
	"vu0_vf19x", "vu0_vf19y", "vu0_vf19z", "vu0_vf19w", \
	"vu0_vf20x", "vu0_vf20y", "vu0_vf20z", "vu0_vf20w", \
	"vu0_vf21x", "vu0_vf21y", "vu0_vf21z", "vu0_vf21w", \
	"vu0_vf22x", "vu0_vf22y", "vu0_vf22z", "vu0_vf22w", \
	"vu0_vf23x", "vu0_vf23y", "vu0_vf23z", "vu0_vf23w", \
	"vu0_vf24x", "vu0_vf24y", "vu0_vf24z", "vu0_vf24w", \
	"vu0_vf25x", "vu0_vf25y", "vu0_vf25z", "vu0_vf25w", \
	"vu0_vf26x", "vu0_vf26y", "vu0_vf26z", "vu0_vf26w", \
	"vu0_vf27x", "vu0_vf27y", "vu0_vf27z", "vu0_vf27w", \
	"vu0_vf28x", "vu0_vf28y", "vu0_vf28z", "vu0_vf28w", \
	"vu0_vf29x", "vu0_vf29y", "vu0_vf29z", "vu0_vf29w", \
	"vu0_vf30x", "vu0_vf30y", "vu0_vf30z", "vu0_vf30w", \
	"vu0_vf31x", "vu0_vf31y", "vu0_vf31z", "vu0_vf31w", \
	"vu1_vi00", "vu1_vi01", "vu1_vi02", "vu1_vi03", \
	"vu1_vi04", "vu1_vi05", "vu1_vi06", "vu1_vi07", \
	"vu1_vi08", "vu1_vi09", "vu1_vi10", "vu1_vi11", \
	"vu1_vi12", "vu1_vi13", "vu1_vi14", "vu1_vi15", \
	"vu1_pc",   "vu1_tpc",  "vu1_stat", "vu1_sflag", \
        "vu1_mac",  "vu1_clip", "vu1_cmsar","", \
	"vu1_r",    "vu1_p",    "vu1_i",    "vu1_q", \
	"vu1_accx", "vu1_accy", "vu1_accz", "vu1_accw", \
	"vu1_vf00x", "vu1_vf00y", "vu1_vf00z", "vu1_vf00w", \
	"vu1_vf01x", "vu1_vf01y", "vu1_vf01z", "vu1_vf01w", \
	"vu1_vf02x", "vu1_vf02y", "vu1_vf02z", "vu1_vf02w", \
	"vu1_vf03x", "vu1_vf03y", "vu1_vf03z", "vu1_vf03w", \
	"vu1_vf04x", "vu1_vf04y", "vu1_vf04z", "vu1_vf04w", \
	"vu1_vf05x", "vu1_vf05y", "vu1_vf05z", "vu1_vf05w", \
	"vu1_vf06x", "vu1_vf06y", "vu1_vf06z", "vu1_vf06w", \
	"vu1_vf07x", "vu1_vf07y", "vu1_vf07z", "vu1_vf07w", \
	"vu1_vf08x", "vu1_vf08y", "vu1_vf08z", "vu1_vf08w", \
	"vu1_vf09x", "vu1_vf09y", "vu1_vf09z", "vu1_vf09w", \
	"vu1_vf10x", "vu1_vf10y", "vu1_vf10z", "vu1_vf10w", \
	"vu1_vf11x", "vu1_vf11y", "vu1_vf11z", "vu1_vf11w", \
	"vu1_vf12x", "vu1_vf12y", "vu1_vf12z", "vu1_vf12w", \
	"vu1_vf13x", "vu1_vf13y", "vu1_vf13z", "vu1_vf13w", \
	"vu1_vf14x", "vu1_vf14y", "vu1_vf14z", "vu1_vf14w", \
	"vu1_vf15x", "vu1_vf15y", "vu1_vf15z", "vu1_vf15w", \
	"vu1_vf16x", "vu1_vf16y", "vu1_vf16z", "vu1_vf16w", \
	"vu1_vf17x", "vu1_vf17y", "vu1_vf17z", "vu1_vf17w", \
	"vu1_vf18x", "vu1_vf18y", "vu1_vf18z", "vu1_vf18w", \
	"vu1_vf19x", "vu1_vf19y", "vu1_vf19z", "vu1_vf19w", \
	"vu1_vf20x", "vu1_vf20y", "vu1_vf20z", "vu1_vf20w", \
	"vu1_vf21x", "vu1_vf21y", "vu1_vf21z", "vu1_vf21w", \
	"vu1_vf22x", "vu1_vf22y", "vu1_vf22z", "vu1_vf22w", \
	"vu1_vf23x", "vu1_vf23y", "vu1_vf23z", "vu1_vf23w", \
	"vu1_vf24x", "vu1_vf24y", "vu1_vf24z", "vu1_vf24w", \
	"vu1_vf25x", "vu1_vf25y", "vu1_vf25z", "vu1_vf25w", \
	"vu1_vf26x", "vu1_vf26y", "vu1_vf26z", "vu1_vf26w", \
	"vu1_vf27x", "vu1_vf27y", "vu1_vf27z", "vu1_vf27w", \
	"vu1_vf28x", "vu1_vf28y", "vu1_vf28z", "vu1_vf28w", \
	"vu1_vf29x", "vu1_vf29y", "vu1_vf29z", "vu1_vf29w", \
	"vu1_vf30x", "vu1_vf30y", "vu1_vf30z", "vu1_vf30w", \
	"vu1_vf31x", "vu1_vf31y", "vu1_vf31z", "vu1_vf31w", \
	"vif0_stat", "vif0_fbrst", "vif0_err", "vif0_mark", \
	"vif0_cycle", "vif0_mode", "vif0_num", "vif0_mask", \
	"vif0_code", "vif0_itops", "", "", \
	"", "vif0_itop", "", "", \
	"vif0_r0", "vif0_r1", "vif0_r2", "vif0_r3", \
	"vif0_c0", "vif0_c1", "vif0_c2", "vif0_c3", \
	"vif0_pc", "vif0_pcx", \
	"vif1_stat", "vif1_fbrst", "vif1_err", "vif1_mark", \
	"vif1_cycle", "vif1_mode", "vif1_num", "vif1_mask", \
	"vif1_code", "vif1_itops", "vif1_base", "vif1_ofst", \
	"vif1_tops", "vif1_itop", "vif1_top", "vif1_dbf", \
	"vif1_r0", "vif1_r1", "vif1_r2", "vif1_r3", \
	"vif1_c0", "vif1_c1", "vif1_c2", "vif1_c3", \
	"vif1_pc", "vif1_pcx" \
    }

#define SR_REGNUM 32

/* Disable the use of floating point registers for function arguments.  */
#undef  MIPS_LAST_FP_ARG_REGNUM
#define MIPS_LAST_FP_ARG_REGNUM (FPA0_REGNUM-1)

/* Define DO_REGISTERS_INFO() to do machine-specific formatting
   of register dumps. */
#undef DO_REGISTERS_INFO
#define DO_REGISTERS_INFO(_regnum, fp) txvu_do_registers_info(_regnum, fp)
extern void txvu_do_registers_info PARAMS ((int, int));

/* Define REGISTER_NAME_ALIAS_HOOK to allow context sensitive mapping
   of register names to indices. */

#define REGISTER_NAME_ALIAS_HOOK(_str,_len) txvu_register_name_alias(_str,_len)
extern int txvu_register_name_alias PARAMS ((char*, int));

#define REGISTER_VIRTUAL_SIZE(N) TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (N))

#define REGISTER_RAW_SIZE(N) REGISTER_VIRTUAL_SIZE(N)

/* Select the proper disassembler */

#undef TM_PRINT_INSN_MACH
#define TM_PRINT_INSN_MACH bfd_mach_mips5900

/* The r5900 has 128bit wide GPRs; the compiler will use 128bit
   stores/loads in the prologue/epilogue.  However, MIPS_REGSIZE
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

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

extern struct type *txvu_register_type PARAMS ((int));

#undef REGISTER_VIRTUAL_TYPE
#define REGISTER_VIRTUAL_TYPE(N) \
(((N) >= FP0_REGNUM && (N) < FP0_REGNUM + 32) ? builtin_type_float \
 : ((N) == SR_REGNUM) ? builtin_type_uint32 \
 : ((N) >= 70 && (N) <= 89) ? builtin_type_uint32 \
 : ((N) >= FIRST_COP0_REG && (N) < (FIRST_COP0_REG + NUM_COP0_REGS)) ? builtin_type_uint32 \
 : ((N) < NUM_CORE_REGS) ? builtin_type_int64 \
 : txvu_register_type (N) )


/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer. */
#undef FRAME_CHAIN

extern CORE_ADDR mips_frame_chain PARAMS ((struct frame_info *));

#define FRAME_CHAIN(thisframe) \
    ((txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) ? \
        mips_frame_chain (thisframe) : 0)

/* Define other aspects of the stack frame.  */

/* Saved PC  */
#undef FRAME_SAVED_PC
#define FRAME_SAVED_PC(FRAME) \
    ((txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) ? \
     mips_frame_saved_pc (FRAME) : 0)
extern CORE_ADDR mips_frame_saved_pc PARAMS ((struct frame_info *));

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#undef FRAME_NUM_ARGS
#define FRAME_NUM_ARGS(num, fi)	\
    (num = (txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) ? \
     mips_frame_num_args(fi) : -1)
extern int mips_frame_num_args PARAMS ((struct frame_info *));


/* Things needed for making the inferior call functions.  */

/* There's a mess in stack frame creation.  See comments in blockframe.c
   near reference to INIT_FRAME_PC_FIRST.  */

#undef INIT_FRAME_PC_FIRST
#define INIT_FRAME_PC_FIRST(leaf, prev) txvu_init_frame_pc_first (leaf, prev)
extern void txvu_init_frame_pc_first PARAMS ((int, struct frame_info *));

#undef INIT_EXTRA_FRAME_INFO
#define INIT_EXTRA_FRAME_INFO(fromleaf, fci) { \
    if (txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) \
      init_extra_frame_info (fci); \
  }

#undef PRINT_EXTRA_FRAME_INFO
#define	PRINT_EXTRA_FRAME_INFO(fi) \
  { \
    if (txvu_cpu==TXVU_CPU_AUTO || txvu_cpu==TXVU_CPU_MASTER) \
      if (fi && fi->proc_desc && fi->proc_desc->pdr.framereg < NUM_REGS) \
        printf_filtered (" frame pointer is at %s+%d\n", \
                         REGISTER_NAME (fi->proc_desc->pdr.framereg), \
                         fi->proc_desc->pdr.frameoffset); \
  }

struct objfile; /* forward declaration for prototype */

extern void txvu_symfile_postread PARAMS ((struct objfile*));

#define TARGET_SYMFILE_POSTREAD(OFILE)	txvu_symfile_postread (OFILE)

#ifdef TARGET_KEEP_SECTION
#undef TARGET_KEEP_SECTION
#endif

extern int txvu_is_phantom_section PARAMS ((asection *));

#define TARGET_KEEP_SECTION(ASECT)	txvu_is_phantom_section (ASECT)

struct target_ops;
extern void txvu_redefine_default_ops PARAMS ((struct target_ops *));

#define TARGET_REDEFINE_DEFAULT_OPS(OPS) txvu_redefine_default_ops (OPS);

#endif	/* TM_TXVU_H */

/* For sky, use the info field to record the Unit identifier */
#undef ELF_MAKE_MSYMBOL_SPECIAL
#define ELF_MAKE_MSYMBOL_SPECIAL(sym,msym) \
 { \
   switch (((elf_symbol_type *)(sym))->internal_elf_sym.st_other) \
     { \
     case STO_MIPS16 : \
       MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym)) | 0x80000000); \
       SYMBOL_VALUE_ADDRESS (msym) |= 1; \
       break; \
     case STO_DVP_DMA: MSYMBOL_INFO(msym) = (char *) bfd_mach_dvp_dma; break; \
     case STO_DVP_VIF: MSYMBOL_INFO(msym) = (char *) bfd_mach_dvp_vif; break; \
     case STO_DVP_GIF: MSYMBOL_INFO(msym) = (char *) bfd_mach_dvp_gif; break; \
     case STO_DVP_VU : MSYMBOL_INFO(msym) = (char *) bfd_mach_dvp_vu; break; \
     default : break; \
     } \
 }
   
