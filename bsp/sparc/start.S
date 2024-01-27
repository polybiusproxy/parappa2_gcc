/*
 * start.S -- SPARC BSP startup.
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

	.file "start.S"

	.extern	SYM_NAME(_bsp_init_stack)
	.extern	SYM_NAME(_start)

	.text

	/*
	 * The hardware reset vector jumps to here.
	 */
FUNC_START _start
	wr	%g0,PSR_INIT,%psr
	wr	%g0,1,%wim
	wr	%g0,0,%tbr

#if defined(SINGLE_VECTOR_TRAP)
#error SINGLE_VECTOR_TRAP not yet implemented.
	wr	%g0, 1, %asr17
#endif
	/*
	 * Minimally initialize cpu and board.
	 * Establish the initial stack.
	 */
	call	SYM_NAME(_bsp_init_stack)
	nop
	mov	%sp,%fp
	
	/*
	 * Move	initialized data from ROM to RAM
	 */
	set	SYM_NAME(__rom_data_start),%g1
	set	SYM_NAME(__ram_data_start),%g2
	cmp	%g1,%g2
	beq	2f
	 nop
	set	SYM_NAME(__ram_data_end),%g3
	cmp	%g2,%g3
	beq	2f
	 nop
    1:
	ldd	[%g1],%g4
	std	%g4,[%g2]
	add	%g2,8,%g2
	cmp	%g2,%g3
	blt	1b
	 add	%g1,8,%g1
    2:
	/*
	 * Zero the bss section.
	 */
	set	SYM_NAME(__bss_start),%g2
	set	SYM_NAME(_end),%g3
	cmp	%g2,%g3
	be	2f
	 mov	%g0,%g1
    1:
	std	%g0,[%g2]
	add	%g2,8,%g2
	cmp	%g2,%g3
	blt	1b
	 nop
    2:
	/*
	 * Copy pointer to board-specific info from stack to variable.
	 */
	ld	[%sp],%g1
	sethi	%hi(SYM_NAME(_bsp_ram_info_ptr)),%g2
	st	%g1,[%g2 + %lo(SYM_NAME(_bsp_ram_info_ptr))]

#if 0	
	call	SYM_NAME(_init)
	 nop
	
	set	SYM_NAME(_fini),%o0
	call	SYM_NAME(atexit)
	 nop
#endif

	call	SYM_NAME(_bsp_init)
	 nop

	/* enable interrupts */
	rd	%psr,%l0
	orn	%g0,0xf00,%l1
	and	%l0,%l1,%l0
	wr	%l0,%psr
	
	mov	%g0,%o0	
	mov	%g0,%o1
	call	SYM_NAME(main)
	 nop

	mov	%o0,%l1
    1:
	call	SYM_NAME(exit)
	 mov	%l1,%o0
	ba	1b
	 nop
	
FUNC_END _start

		
	