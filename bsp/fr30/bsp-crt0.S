/*
 * bsp-crt0.S -- startup file for FR30.
 *
 * Copyright (c) 1998, 1999 Cygnus Support
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

	.text
	.p2align 2

/* Without the following nop, GDB thinks _start is a data variable.
 * This is probably a bug in GDB in handling a symbol that is at the
 * start of the .text section.
 */
	nop

	.globl	_start
_start:
	/*
	 *  Zero BSS
	 */
	ldi:32	#SYM_NAME(_edata),r0
	ldi:32	#SYM_NAME(_end),r1
	cmp	r0,r1
	beq	2f
	ldi:8	#0,r2
    1:
	st	r2,@r0
	add	#4,r0
	cmp	r1,r0
	blt	1b
    2:
	
	ldi:32	#SYM_NAME(_init),r0
	call	@r0

	ldi:32	#SYM_NAME(atexit),r0
	ldi:32	#SYM_NAME(_fini),r4
	call	@r0


	/*
	 * get linkage to shared data
	 */
	ldi:20	#BSP_GET_SHARED,r4
	ldi:32	#bsp_shared_data,r5
	int	#10
	
	ldi:8	#0,r4
	ldi:8	#0,r5
	ldi:8	#0,r6
	ldi:32	#SYM_NAME(main),r0
	call	@r0

	/* return value from main is argument to exit */
1:
	ldi:32	#SYM_NAME(exit),r0
	call	@r0
	bra	1b
	
	.end _start


