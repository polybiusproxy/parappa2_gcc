/*
 * crt0.S -- startup file for PowerPC systems.
 *
 * Copyright (c) 1995, 1999 Cygnus Support
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

#include <bsp/cpu.h>
#include "bsp_if.h"


	.file	"crt0.S"
		
	.globl	_start
	.type	_start,@function
_start:
	/* clear bss and sbss */
	lwi	r6,_edata
	lwi	r7,_end
	cmplw	r6,r7
	beq	.Ldone1

	subf	r8,r6,r7		/* number of bytes to zero */
	srwi	r9,r8,2			/* number of words to zero */
	mtctr	r9
	li	r0,0			/* zero to clear memory */
	addi	r6,r6,-4		/* adjust so we can use stwu */
.Lloop:
	stwu	r0,4(r6)		/* zero bss */
	bdnz	.Lloop
.Ldone1:

	/* get linkage to BSP shared data */
	lis	r3,BSP_GET_SHARED@h
	ori	r3,r3,BSP_GET_SHARED@l
	lis	r4,SYM_NAME(bsp_shared_data)@h
	ori	r4,r4,SYM_NAME(bsp_shared_data)@l
	sc
	nop

.Lnostack:
	/* set up initial stack frame */
	addi	sp,sp,-64		/* make sure we don't overwrite debug mem */
	lis	r0,0
	stw	r0,0(sp)		/* clear back chain */
	stwu	sp,-64(sp)		/* push another stack frame */

	/* Let her rip */
	bl	SYM_NAME(main)

	/* return value from main is argument to exit */
	bl	SYM_NAME(exit)
	trap
.Lstart:
	.size	_start,.Lstart-_start

	.extern	SYM_NAME(atexit)
	.globl	SYM_NAME(__atexit)
	.section ".sdata","aw"
	.align	2
SYM_NAME(__atexit):			/* tell C's eabi-ctor's we have an atexit function */
	.long	SYM_NAME(atexit)@fixup	/* and that it is to register __do_global_dtors */

	.section ".fixup","aw"
	.align	2
	.long	SYM_NAME(__atexit)
