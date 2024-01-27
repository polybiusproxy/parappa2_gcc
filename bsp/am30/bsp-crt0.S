/*
 * bsp-crt0.S -- AM30 (MN10300x) RAM-based startup (BSP in ROM).
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
	
	.file	"bsp-crt0.S"

#include <bsp/cpu.h>
#include "bsp_if.h"
	
	.text

	.extern	SYM_NAME(__main)
	.extern	SYM_NAME(main)
	.extern	SYM_NAME(exit)

	.globl	SYM_NAME(_start)
SYM_NAME(_start):
	
	/*
	 * Clear BSS
	 */
	mov	SYM_NAME(_bss_start),a0
	mov	SYM_NAME(end),a1

	cmp	a0,a1
	beq	8f
	clr	d0
    1:
	movbu	d0,(a0)
	inc	a0
	cmp	a0,a1
	bne	1b
    8:
	/*
	 * get linkage to shared data
	 */
	mov	BSP_GET_SHARED,d0
	mov	SYM_NAME(bsp_shared_data),d1
	syscall
	nop

	/*
	 * Call constructors
	 */
	call	SYM_NAME(__main),[],0

	/*
	 * Call main
	 */
	clr	d0
	clr	d1
	mov	d0,(4,sp)
	call	SYM_NAME(main),[],0

	/* return value from main is argument to exit */
    1:
	call	SYM_NAME(exit),[],0
	bra	1b

