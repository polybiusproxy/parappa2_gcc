/*
 * bsp-crt0.S -- SPARC startup file for RAM-based apps.
 *
 * Copyright (c) 1999 Cygnus Support
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
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include "syscall.h"

	.file "bsp-crt0.S"

	.text

	/*
	 * The BSP/Monitor in ROM jumps to here.
	 */
FUNC_START _start

	/* zero the bss section */
	set	SYM_NAME(__bss_start),%g2
	set	SYM_NAME(_end),%g3
	mov	%g0,%g1
    1:
	std	%g0,[%g2]
	add	%g2,8,%g2
	cmp	%g2,%g3
	bleu,a	1b
	 nop

#ifdef BOARD_MB8683X
	set	0x04000000,%l0
#endif
	set	BSP_VEC_BSPDATA,%l1
	sll	%l1,2,%l1
	ld	[%l0 + %l1],%l2
	set	SYM_NAME(bsp_shared_data),%l0
	st	%l2,[%l0]

#if 0	
	call	SYM_NAME(__main)
	 nop
#endif

	mov	%g0,%o0	
	mov	%g0,%o1
	call	SYM_NAME(main)
	 nop
	mov	%o0,%l0
    1:
	mov	%l0,%o0
	call	SYM_NAME(exit)
	 nop
	ba	1b
	 nop
	
FUNC_END _start
	