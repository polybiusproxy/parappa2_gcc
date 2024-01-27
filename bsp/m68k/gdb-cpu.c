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

/*
 * Return byte offset within the saved register area of the
 * given register.
 */
int
bsp_regbyte(int regno)
{
  switch (regno) {
  case REG_SR:
    return (int)&((ex_regs_t *)0)->_sr;
  case REG_PC:
    return (int)&((ex_regs_t *)0)->_pc;
  default:
    if (regno <= REG_A7)
      return regno * sizeof(unsigned long);
  }

  return 0;
}


/*
 * Return size in bytes of given register.
 */
int
bsp_regsize(int regno)
{
  if (regno == REG_SR)
    return sizeof(((ex_regs_t *)0)->_sr);
  else if (regno <= REG_MAX)
    return sizeof(((ex_regs_t *)0)->_pc);
  else
    return 0;
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
  case BSP_CORE_EXC_BUS_ERROR:
  case BSP_CORE_EXC_ADDR_ERROR:
  /*
   * Coprocessor protocol violation. Using a standard MMU or FPU this
   * cannot be triggered by software. Call it a SIGBUS.
   */
  case BSP_CORE_EXC_COPROC_PROTOCOL_VIOLATION:
    sig = TARGET_SIGNAL_BUS;
    break;

  case BSP_CORE_EXC_ILL_INSTRUCTION:
    sig = TARGET_SIGNAL_ILL;
    break;

  case BSP_CORE_EXC_DIV_ZERO:
  case BSP_CORE_EXC_CHK:
  case BSP_CORE_EXC_TRAP:
  /*
   * This is a trap #8 instruction. Apparently it is someone's software
   * convention for some sort of SIGFPE condition.  Whose?  How many
   * people are being screwed by having this code the way it is?
   * Is there a clean solution?
   */
  case BSP_CORE_EXC_TRAP_8:
  case BSP_CORE_EXC_FP_UNORDERED_COND:
  case BSP_CORE_EXC_FP_INEXACT:
  case BSP_CORE_EXC_FP_DIV_ZERO:
  case BSP_CORE_EXC_FP_UNDERFLOW:
  case BSP_CORE_EXC_FP_OPERAND_ERROR:
  case BSP_CORE_EXC_FP_OVERFLOW:
  case BSP_CORE_EXC_FP_NAN:
    sig = TARGET_SIGNAL_FPE;
    break;

  case BSP_CORE_EXC_PRIV_VIOLATION:
    sig = TARGET_SIGNAL_SEGV;
    break;

  case BSP_CORE_EXC_TRACE:
  case GDB_BREAKPOINT_VECTOR:
    sig = TARGET_SIGNAL_TRAP;
    break;

  case BSP_CORE_EXC_LINE_1010:
  case BSP_CORE_EXC_LINE_1111:
    sig = TARGET_SIGNAL_ILL;
    break;

  /*
   * Interrupt
   */
  case BSP_CORE_EXC_LEVEL_7_AUTO:
    sig = TARGET_SIGNAL_INT;
    break;

  /*
   * This is the exit condition
   */
  case BSP_CORE_EXC_TRAP_15:
    sig = 0;
    break;

  default:
    sig = TARGET_SIGNAL_TRAP;
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
  ((ex_regs_t *)saved_regs)->_pc = pc;
}


/*
 * Get the PC value from the saved registers.
 */
unsigned long
bsp_get_pc(void *saved_regs)
{
  return ((ex_regs_t *)saved_regs)->_pc;
}


  
