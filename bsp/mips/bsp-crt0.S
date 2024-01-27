/*
 * bsp-crt0.S -- startup file for MIPS.
 *
 * Copyright (c) 1995, 1996, 1997, 1998 Cygnus Support
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

#ifdef __mips16
/* This file contains 32 bit assembly code.  */
	.set nomips16
#endif

#include <bsp/cpu.h>
#include "bsp_if.h"

/*
 * Set up some room for a stack. We just grab a chunk of memory.
 */
	.text
	.p2align 2

/* Without the following nop, GDB thinks _start is a data variable.
 * This is probably a bug in GDB in handling a symbol that is at the
 * start of the .text section.
 */
	nop

	.globl	_start
	.ent	_start
_start:
	.set	noreorder

	/*
	 *  Zero BSS
	 */
	la	a0,SYM_NAME(_edata)
	la	a1,SYM_NAME(_end)
	beq	a0,a1,2f
	 nop
    1:
	sw	zero,0(a0)
	addiu	a0,a0,4
	bne	a0,a1,1b
	 nop
    2:

	la	gp,_gp				# set the global data pointer

	/*
	 * get linkage to shared data
	 */
	li	a0,BSP_GET_SHARED
	la	a1,bsp_shared_data
	syscall
	nop

	la	a0,__do_global_dtors
	jal	atexit
	nop

#ifdef GCRT0
	.globl	_ftext
	.globl	_extext
	la	a0,_ftext
	la	a1,_etext
	jal	monstartup
	nop
#endif

	li	a0,0
	li	a1,0
	li	a2,0
	jal	main			# call the program start function
	 addiu	sp,sp,-32
	addiu	sp,sp,32

	move	s0,v0			# save exit code
#ifdef GCRT0
	jal	_mcleanup
	nop	 
#endif
	/* return value from main is argument to exit */
1:
	jal	exit
	 move	a0,s0
	b	1b
	
	.end _start


