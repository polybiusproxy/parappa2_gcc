/*
 * gdb-cpu.c -- CPU specific support for GDB stub.
 *
 * Copyright (c) 1998 Cygnus Support
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */
#include <stdlib.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include "gdb.h"
#include "insn.h"

/*
 * Return byte offset within the saved register area of the
 * given register.
 */
int
bsp_regbyte(int regno)
{
    if (regno < 32)
	return regno * sizeof(unsigned long);
    
#ifndef __mips_soft_float
    if (regno >= REG_F0 && regno < (REG_F0 + 32))
	return (int)&((ex_regs_t *)0)->_fpr[regno - REG_F0];
#endif

    switch (regno) {
      case REG_SR:
	return (int)&((ex_regs_t *)0)->_sr;
      case REG_LO:
	return (int)&((ex_regs_t *)0)->_lo;
      case REG_HI:
	return (int)&((ex_regs_t *)0)->_hi;
      case REG_BAD:
	return (int)&((ex_regs_t *)0)->_bad;
      case REG_CAUSE:
	return (int)&((ex_regs_t *)0)->_cause;
      case REG_PC:
	return (int)&((ex_regs_t *)0)->_epc;
#ifndef __mips_soft_float
      case REG_FSR:
	return (int)&((ex_regs_t *)0)->_fsr;
      case REG_FIP:
	return (int)&((ex_regs_t *)0)->_fip;
      case REG_FP:
	return (int)&((ex_regs_t *)0)->_fp;
#endif
#if defined(_R3900)
      case REG_CONFIG:
	return (int)&((ex_regs_t *)0)->_config;
      case REG_CACHE:
	return (int)&((ex_regs_t *)0)->_cache;
      case REG_DEBUG:
	return (int)&((ex_regs_t *)0)->_debug;
      case REG_DEPC:
	return (int)&((ex_regs_t *)0)->_depc;
      case REG_EPC:
	return (int)&((ex_regs_t *)0)->_xpc;
#endif
    }
    return 0;
}


/*
 * Return size in bytes of given register.
 */
int
bsp_regsize(int regno)
{
    if (regno >= REG_F0 && regno < (REG_F0 + 32)) {
#ifdef __mips64
	return sizeof(double);
#else
	return sizeof(float);
#endif
    }

    switch (regno) {
      case REG_SR:
      case REG_CAUSE:
      case REG_FSR:
      case REG_FIR:
	return 4;

#if defined(_R3900)
      case REG_CONFIG:
      case REG_CACHE:
      case REG_DEBUG:
      case REG_DEPC:
      case REG_EPC:
	return 4;
#endif
    }
    return sizeof(long);
}


/*
 *  Given an exception number and a pointer to saved registers,
 *  return a GDB signal value.
 */
int
bsp_get_signal(int exc_nr, void *saved_regs)
{
    int sig = TARGET_SIGNAL_TRAP;

    switch (exc_nr) {
      case BSP_EXC_TLB:
      case BSP_EXC_XTLB:
      case BSP_EXC_TLBMOD:
      case BSP_EXC_TLBL:
      case BSP_EXC_TLBS:
      case BSP_EXC_VCEI:
      case BSP_EXC_VCED:
	sig = TARGET_SIGNAL_SEGV;
	break;

      case BSP_EXC_ADEL:
      case BSP_EXC_ADES:
      case BSP_EXC_IBE:
      case BSP_EXC_DBE:
      case BSP_EXC_CACHE:
	sig = TARGET_SIGNAL_BUS;
	break;

      case BSP_EXC_ILL:
      case BSP_EXC_CPU:
	sig = TARGET_SIGNAL_ILL;
	break;

      case BSP_EXC_FPE:
      case BSP_EXC_OV:
	sig = TARGET_SIGNAL_FPE;
	break;

      case BSP_EXC_INT:
	sig = TARGET_SIGNAL_INT;
	break;
    }

    return sig;
}


/*
 * Set the PC value in the saved registers.
 */
void
bsp_set_pc(unsigned long pc, void *saved_regs)
{
    ((ex_regs_t *)saved_regs)->_epc = pc;
}


/*
 * Get the PC value from the saved registers.
 */
unsigned long
bsp_get_pc(void *saved_regs)
{
    return ((ex_regs_t *)saved_regs)->_epc;
}


