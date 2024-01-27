/*
 * start.S -- PowerPC BSP startup.
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
#include <bsp/bsp.h>
#include <bsp/cpu.h>

	.file "start.S"

	.extern	SYM_NAME(_bsp_init_stack)
	.extern	SYM_NAME(_bsp_init)
	.extern	SYM_NAME(__eabi)

	.text

	.macro default_vec
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__exception_handler)@h
	ori	r3,r3,SYM_NAME(__exception_handler)@l
	mtlr	r3
	blrl
	.endm
		
  SYM_NAME(__vectors_start):
	/*
	 *  Reserved vector 0
	 */
	nop

	
	.p2align 8
	/* Offset 0x100 */
#if defined(CPU_PPC4XX)
	/*
	 * Critical Interrupt
	 */
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__ci_handler)@h
	ori	r3,r3,SYM_NAME(__ci_handler)@l
	mtlr	r3
	blrl
#else
	/*
	 *  Reset vector
	 */
	lis	r3,SYM_NAME(_start)@h
	ori	r3,r3,SYM_NAME(_start)@l
	mtlr	r3
	blr
#endif

	.p2align 8
	/* Offset 0x200 */
#if defined(CPU_PPC4XX)
	/*
	 * Machine Check
	 */
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__ci_handler)@h
	ori	r3,r3,SYM_NAME(__ci_handler)@l
	mtlr	r3
	blrl
#else
	default_vec
#endif
	
	.p2align 8
	/* Offset 0x300 */
	default_vec
	
	.p2align 8
	/* Offset 0x400 */
	default_vec
	
	.p2align 8
	/* Offset 0x500 */
	default_vec
	
	.p2align 8
	/* Offset 0x600 */
	default_vec
	
	.p2align 8
	/* Offset 0x700 */
	default_vec
	
	.p2align 8
	/* Offset 0x800 */
	default_vec
	
	.p2align 8
	/* Offset 0x900 */
	default_vec
	
	.p2align 8
	/* Offset 0xA00 */
	default_vec
	
	.p2align 8
	/* Offset 0xB00 */
	default_vec
	
	.p2align 8
	/* Offset 0xC00 */
	default_vec
	
	.p2align 8
	/* Offset 0xD00 */
	default_vec
	
	.p2align 8
	/* Offset 0xE00 */
	default_vec
	
	.p2align 8
	/* Offset 0xF00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1000 */
#if defined(CPU_PPC4XX)
	/* pit */
	b	1f
	nop
	nop
	nop
	/* fit */
	b	2f
	nop
	nop
	nop
	/* watchdog */
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__wdt_handler)@h
	ori	r3,r3,SYM_NAME(__wdt_handler)@l
	mtlr	r3
	blr
    1:
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__pit_handler)@h
	ori	r3,r3,SYM_NAME(__pit_handler)@l
	mtlr	r3
	blr
    2:
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__fit_handler)@h
	ori	r3,r3,SYM_NAME(__fit_handler)@l
	mtlr	r3
	blr
#else
	default_vec
#endif
	
	.p2align 8
	/* Offset 0x1100 */
	default_vec
	
	.p2align 8
	/* Offset 0x1200 */
	default_vec
	
	.p2align 8
	/* Offset 0x1300 */
	default_vec
	
	.p2align 8
	/* Offset 0x1400 */
	default_vec
	
	.p2align 8
	/* Offset 0x1500 */
	default_vec
	
	.p2align 8
	/* Offset 0x1600 */
	default_vec
	
	.p2align 8
	/* Offset 0x1700 */
	default_vec
	
	.p2align 8
	/* Offset 0x1800 */
	default_vec
	
	.p2align 8
	/* Offset 0x1900 */
	default_vec
	
	.p2align 8
	/* Offset 0x1A00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1B00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1C00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1D00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1E00 */
	default_vec
	
	.p2align 8
	/* Offset 0x1F00 */
	default_vec
	
	.p2align 8
	/* Offset 0x2000 */
#if defined(CPU_PPC4XX)
	/*
	 * Debug Exception
	 */
	mtspr	sprg0,r3
	mflr	r3
	mtspr	sprg1,r3
	lis	r3,SYM_NAME(__ci_handler)@h
	ori	r3,r3,SYM_NAME(__ci_handler)@l
	mtlr	r3
	blrl
#else
	default_vec
#endif
	
	/*
	 *  The remaining vectors use the same indirection scheme
	 *  so that even vectors in ROM can be overridden.
	 */
	.rept 31
	.p2align 8
	default_vec
	.endr

	.p2align 8
  SYM_NAME(__vectors_end):


	
	
#ifdef CPU_PPC4XX
#define INIT_MSR  (MSR_ME)
#else
#ifdef HAVE_FPU
#define INIT_MSR  (MSR_FP|MSR_ME|MSR_FE0|MSR_FE1|MSR_IP|MSR_RI)
#else
#define INIT_MSR  (MSR_ME|MSR_IP|MSR_RI)
#endif
#endif

FUNC_START(_start)
#if !defined(CPU_PPC4XX)
	/* Set up MSR */
	lis	r3,INIT_MSR@h
	ori	r3,r3,INIT_MSR@l
	mtmsr	r3
        mtspr	srr1,r3
#endif
	/* set up time base register to zero */
	xor	r3,r3,r3
	mtspr	tblw,r3
	xor	r4,r4,r4
	mtspr	tbuw,r4
	
#if 0	
	/* Disable caches for now. */
	bl	dcache_disable
	bl	icache_disable
#endif	
	
#if defined(CPU_MPC8XX)
#define CACHE_UNLOCKALL		0x0A00
#define CACHE_DISABLE		0X0400
#define CACHE_INVALIDATEALL	0X0C00
#define CACHE_ENABLE		0X0200
#define CACHE_ENABLEBIT		0X8000

	mfspr	r3,dc_cst		/* clear error bits */
        lis     r3,CACHE_UNLOCKALL
	sync
        mtspr   dc_cst,r3		/* unlock all lines */
	
        lis     r3,CACHE_INVALIDATEALL
	sync
        mtspr   dc_cst,r3		/* invalidate all lines */

	lis	r3,CACHE_DISABLE
	sync
	mtspr	dc_cst,r3               /* disable */
	
	
	mfspr	r3,ic_cst		/* clear error bits */
        lis     r3,CACHE_UNLOCKALL
        mtspr   ic_cst,r3		/* unlock all lines */
	isync
        lis     r3,CACHE_INVALIDATEALL
        mtspr   ic_cst,r3		/* invalidate all lines */
        isync
	lis	r3,CACHE_DISABLE
	mtspr	ic_cst,r3               /* disable */
	isync
	
	sync
	
	/*
	 * Debug related registers.
	 */
        lwi	r3,0x00000007
        mtspr	ictrl,r3

        lwi	r3,0x00000000
        mtspr	der,r3
#endif
	
	/*
	 * Call _bsp_init_stack to do just enough initialization to
	 * set up clocks, chip selects, and give us a stack.
	 */
	bl	SYM_NAME(_bsp_init_stack)

	/***************************************************
	 * RAM Vector Initialization
	 *
	 *    The user exeception vector will be initialized
	 *    and located at 0 so that user can claim 
	 *    every exception except trace for the application.
	 *
	 *****************************************************/
#ifndef USE_MONITOR
        lis     r5,0x0000
        lis     r4,SYM_NAME(__vectors_start)@h
        ori     r4,r4,SYM_NAME(__vectors_start)@l
        li      r3,0x1000
    1:
        addic.  r3,r3,-1
        lwz     r6,0(r4)
        addi    r4,r4,4
        stw     r6,0(r5)
        addi    r5,r5,4
        bne     1b
#endif

	/* Change exception prefix to 0 so that CPU will
	   jump to vector table at 0 instead of FFF00000 */
#if defined(CPU_PPC4XX)
	xor	r0,r0,r0
	mtspr	evpr,r0
#else
        mfmsr   r3
        andi.   r3,r3,0xffbf
        mtmsr   r3
#endif

	/* Copy data from ROM to ram */
	lwi	r3,SYM_NAME(__rom_data_start)
	lwi	r4,SYM_NAME(__ram_data_start)
	lwi	r5,SYM_NAME(__ram_data_end)
	cmplw	r4,r5
	beq	2f
1:
	lwz	r0,0(r3)
	stw	r0,0(r4)
	addi	r3,r3,4
	addi	r4,r4,4
	cmplw	r4,r5
	blt	1b
2:
	/* clear BSS */
	lwi	r3,_edata
	lwi	r4,_end
	li	r0,0
	cmplw	r3,r4
	beq	2f
1:
	stw	r0,0(r3)
	addi	r3,r3,4
	cmplw	r3,r4
	blt	1b
2:
	
	/*
	 * Copy pointer to board-specific RAM info from stack to variable.
	 */
	lwz	r0,0(sp)
	lis	r3,SYM_NAME(_bsp_ram_info_ptr)@h
	ori	r3,r3,SYM_NAME(_bsp_ram_info_ptr)@l
	stw     r0,0(r3)

	/*
	 *  Call __eabi to set up runtime environment. This
	 *  is normally called from the prologue in main(),
	 *  but we do it here so that _bsp_late_init can be
	 *  written in C or C++.
	 */
	stwu	sp,-UNDERHEAD_SIZE(sp)
/*	bl	SYM_NAME(__eabi)*/

	/*
	 * Call _bsp_init to do all remaining setup actions which
	 * need to be done after .data and .bss have been initialized.
	 */
	bl	SYM_NAME(_bsp_init)

	li	r0,0
	li	r3,SYM_NAME(__atexit)@h
	ori	r3,r3,SYM_NAME(__atexit)@l
	stw	r0,0(r3)

	/* set up stack to call main */
	li	r0,0
	stw	r0,0(sp) /* zero backlink */
	mr	r3,0
	mr	r4,0
9:	
	bl	SYM_NAME(main)
	b	9b
FUNC_END(_start)


/*
 * Common (mostly) exception handler.
 * On entry:
 *     sprg0 = original R3
 *     sprg1 = original LR
 *     LR    = return address of vector so vec number
 *	       can be calculated.
 */	
FUNC_START(__exception_handler)
	mtspr	sprg2,sp
	mfcr	r3
	mtspr	sprg3,r3
	lis	r3,SYM_NAME(_ex_stack)@h
	ori	r3,r3,SYM_NAME(_ex_stack)@l
	cmplw	sp,r3
	blt	1f
	mr	sp,r3
    1:
	subi	sp,sp,EX_STACK_SIZE
	stw	r0,GPR0(sp)
	mfspr	r0,sprg2
	stw	r0,GPR1(sp)
	mfspr	r3,sprg0
	stmw	r2,GPR2(sp)
        mfspr   r3,sprg1
	stw	r3,LR(sp)
	mfspr	r3,srr0
	stw	r3,NIP(sp)
	mfctr	r3
	stw	r3,CTR(sp)
	mfspr	r3,sprg3
	stw	r3,CCR(sp)
	mfxer	r3
	stw	r3,XER(sp)
	mfspr	r3,srr1
	stw	r3,MSR(sp)
	li	r3,0
	stw	r3,MQ(sp)
	mflr	r3
	rlwinm	r3,r3,24,25,31
	stw	r3,TRAP(sp)
	mr	r4,sp
	bl	SYM_NAME(_bsp_exc_dispatch)

	/*
	 * need to call debug stub if exception not handled
	 * by application code.
	 */
	cmpwi	r3,0
	bne	1f
	lwz	r3,TRAP(sp)
	mr	r4,sp
	lis	r5,SYM_NAME(_bsp_dbg_vector)@h
	ori	r5,r5,SYM_NAME(_bsp_dbg_vector)@l
	lwz	r5,0(r5)
	mtlr	r5
	blrl
    1:
	lwz	r3,MSR(sp)
	mtspr   srr1,r3
	lwz	r3,NIP(sp)
	mtspr   srr0,r3
	lwz	r3,XER(sp)
	mtspr   xer,r3
	lwz	r3,CCR(sp)
	mtcr    r3
	lwz	r3,CTR(sp)
	mtctr   r3
	lwz	r3,LR(sp)
	mtlr	r3
	lmw	r2,GPR2(sp)
	lwz	r0,GPR0(sp)
	lwz	sp,GPR1(sp)
	isync
	sync
	rfi
FUNC_END(__exception_handler)
	

	
#if defined(CPU_PPC4XX)
/*
 * PPC 403 systems have some vector overloading which we have
 * handle differently.
 */

/*
 * Critical interrupt uses srr2 and srr3.
 */
FUNC_START(__ci_handler)
	mtspr	sprg2,sp
	mfcr	r3
	mtspr	sprg3,r3
	lis	r3,SYM_NAME(_ex_stack)@h
	ori	r3,r3,SYM_NAME(_ex_stack)@l
	cmplw	sp,r3
	blt	1f
	mr	sp,r3
    1:
	subi	sp,sp,EX_STACK_SIZE
	stw	r0,GPR0(sp)
	mfspr	r0,sprg2
	stw	r0,GPR1(sp)
	mfspr	r3,sprg0
	stmw	r2,GPR2(sp)
        mfspr   r3,sprg1
	stw	r3,LR(sp)
	mfspr	r3,srr2
	stw	r3,NIP(sp)
	mfctr	r3
	stw	r3,CTR(sp)
	mfspr	r3,sprg3
	stw	r3,CCR(sp)
	mfxer	r3
	stw	r3,XER(sp)
	mfspr	r3,srr3
	stw	r3,MSR(sp)
	li	r3,0
	stw	r3,MQ(sp)
	mflr	r3
	rlwinm	r3,r3,24,25,31
	stw	r3,TRAP(sp)
	mr	r4,sp
	bl	SYM_NAME(_bsp_exc_dispatch)

	/*
	 * need to call debug stub if exception not handled
	 * by application code.
	 */
	cmpwi	r3,0
	bne	1f
	lwz	r3,TRAP(sp)
	mr	r4,sp
	lis	r5,SYM_NAME(_bsp_dbg_vector)@h
	ori	r5,r5,SYM_NAME(_bsp_dbg_vector)@l
	lwz	r5,0(r5)
	mtlr	r5
	blrl
    1:
	lwz	r3,MSR(sp)
	mtspr   srr3,r3
	lwz	r3,NIP(sp)
	mtspr   srr2,r3
	lwz	r3,XER(sp)
	mtspr   xer,r3
	lwz	r3,CCR(sp)
	mtcr    r3
	lwz	r3,CTR(sp)
	mtctr   r3
	lwz	r3,LR(sp)
	mtlr	r3
	lmw	r2,GPR2(sp)
	lwz	r0,GPR0(sp)
	lwz	sp,GPR1(sp)
	isync
	sync
	rfci
FUNC_END(__ci_handler)


/*
 * Programable interval timer interrupt uses srr2 and srr3.
 */
FUNC_START(__pit_handler)
	mtspr	sprg2,sp
	mfcr	r3
	mtspr	sprg3,r3
	lis	r3,SYM_NAME(_ex_stack)@h
	ori	r3,r3,SYM_NAME(_ex_stack)@l
	cmplw	sp,r3
	blt	1f
	mr	sp,r3
    1:
	subi	sp,sp,EX_STACK_SIZE
	stw	r0,GPR0(sp)
	mfspr	r0,sprg2
	stw	r0,GPR1(sp)
	mfspr	r3,sprg0
	stmw	r2,GPR2(sp)
        mfspr   r3,sprg1
	stw	r3,LR(sp)
	mfspr	r3,srr0
	stw	r3,NIP(sp)
	mfctr	r3
	stw	r3,CTR(sp)
	mfspr	r3,sprg3
	stw	r3,CCR(sp)
	mfxer	r3
	stw	r3,XER(sp)
	mfspr	r3,srr1
	stw	r3,MSR(sp)
	li	r3,0
	stw	r3,MQ(sp)
	li	r3,BSP_EXC_PIT
	stw	r4,TRAP(sp)
	mr	r4,sp
	bl	SYM_NAME(_bsp_exc_dispatch)

	/*
	 * need to call debug stub if exception not handled
	 * by application code.
	 */
	cmpwi	r3,0
	bne	1f
	lwz	r3,TRAP(sp)
	mr	r4,sp
	lis	r5,SYM_NAME(_bsp_dbg_vector)@h
	ori	r5,r5,SYM_NAME(_bsp_dbg_vector)@l
	lwz	r5,0(r5)
	mtlr	r5
	blrl
    1:
	lwz	r3,MSR(sp)
	mtspr   srr1,r3
	lwz	r3,NIP(sp)
	mtspr   srr0,r3
	lwz	r3,XER(sp)
	mtspr   xer,r3
	lwz	r3,CCR(sp)
	mtcr    r3
	lwz	r3,CTR(sp)
	mtctr   r3
	lwz	r3,LR(sp)
	mtlr	r3
	lmw	r2,GPR2(sp)
	lwz	r0,GPR0(sp)
	lwz	sp,GPR1(sp)
	isync
	sync
	rfi
FUNC_END(__pit_handler)


/*
 * Fixed interval timer interrupt uses srr2 and srr3.
 */
FUNC_START(__fit_handler)
	mtspr	sprg2,sp
	mfcr	r3
	mtspr	sprg3,r3
	lis	r3,SYM_NAME(_ex_stack)@h
	ori	r3,r3,SYM_NAME(_ex_stack)@l
	cmplw	sp,r3
	blt	1f
	mr	sp,r3
    1:
	subi	sp,sp,EX_STACK_SIZE
	stw	r0,GPR0(sp)
	mfspr	r0,sprg2
	stw	r0,GPR1(sp)
	mfspr	r3,sprg0
	stmw	r2,GPR2(sp)
        mfspr   r3,sprg1
	stw	r3,LR(sp)
	mfspr	r3,srr0
	stw	r3,NIP(sp)
	mfctr	r3
	stw	r3,CTR(sp)
	mfspr	r3,sprg3
	stw	r3,CCR(sp)
	mfxer	r3
	stw	r3,XER(sp)
	mfspr	r3,srr1
	stw	r3,MSR(sp)
	li	r3,0
	stw	r3,MQ(sp)
	li	r3,BSP_EXC_FIT
	stw	r4,TRAP(sp)
	mr	r4,sp
	bl	SYM_NAME(_bsp_exc_dispatch)

	/*
	 * need to call debug stub if exception not handled
	 * by application code.
	 */
	cmpwi	r3,0
	bne	1f
	lwz	r3,TRAP(sp)
	mr	r4,sp
	lis	r5,SYM_NAME(_bsp_dbg_vector)@h
	ori	r5,r5,SYM_NAME(_bsp_dbg_vector)@l
	lwz	r5,0(r5)
	mtlr	r5
	blrl
    1:
	lwz	r3,MSR(sp)
	mtspr   srr1,r3
	lwz	r3,NIP(sp)
	mtspr   srr0,r3
	lwz	r3,XER(sp)
	mtspr   xer,r3
	lwz	r3,CCR(sp)
	mtcr    r3
	lwz	r3,CTR(sp)
	mtctr   r3
	lwz	r3,LR(sp)
	mtlr	r3
	lmw	r2,GPR2(sp)
	lwz	r0,GPR0(sp)
	lwz	sp,GPR1(sp)
	isync
	sync
	rfi
FUNC_END(__fit_handler)

	
/*
 * Watchdog timer.
 */
FUNC_START(__wdt_handler)
	mtspr	sprg2,sp
	mfcr	r3
	mtspr	sprg3,r3
	lis	r3,SYM_NAME(_ex_stack)@h
	ori	r3,r3,SYM_NAME(_ex_stack)@l
	cmplw	sp,r3
	blt	1f
	mr	sp,r3
    1:
	subi	sp,sp,EX_STACK_SIZE
	stw	r0,GPR0(sp)
	mfspr	r0,sprg2
	stw	r0,GPR1(sp)
	mfspr	r3,sprg0
	stmw	r2,GPR2(sp)
        mfspr   r3,sprg1
	stw	r3,LR(sp)
	mfspr	r3,srr2
	stw	r3,NIP(sp)
	mfctr	r3
	stw	r3,CTR(sp)
	mfspr	r3,sprg3
	stw	r3,CCR(sp)
	mfxer	r3
	stw	r3,XER(sp)
	mfspr	r3,srr3
	stw	r3,MSR(sp)
	li	r3,0
	stw	r3,MQ(sp)
	li	r3,BSP_EXC_WATCHDOG
	stw	r3,TRAP(sp)
	mr	r4,sp
	bl	SYM_NAME(_bsp_exc_dispatch)

	/*
	 * need to call debug stub if exception not handled
	 * by application code.
	 */
	cmpwi	r3,0
	bne	1f
	lwz	r3,TRAP(sp)
	mr	r4,sp
	lis	r5,SYM_NAME(_bsp_dbg_vector)@h
	ori	r5,r5,SYM_NAME(_bsp_dbg_vector)@l
	lwz	r5,0(r5)
	mtlr	r5
	blrl
    1:
	lwz	r3,MSR(sp)
	mtspr   srr3,r3
	lwz	r3,NIP(sp)
	mtspr   srr2,r3
	lwz	r3,XER(sp)
	mtspr   xer,r3
	lwz	r3,CCR(sp)
	mtcr    r3
	lwz	r3,CTR(sp)
	mtctr   r3
	lwz	r3,LR(sp)
	mtlr	r3
	lmw	r2,GPR2(sp)
	lwz	r0,GPR0(sp)
	lwz	sp,GPR1(sp)
	isync
	sync
	rfci
FUNC_END(__wdt_handler)


#endif /* CPU_PPC4XX */	
		
	.data
	
	/*
	 *  Provide a stack for use by nmi exception handler.
	 *  Note that this stack is in .bss, so it will be below
	 *  the normal user stack which is placed in high memory.
	 *  We use this fact to detect whether or not we need to
	 *  switch stacks before saving regs.
	 */
	.section ".bss","a"
	.p2align 4
	.rept	4096
	.long	0
	.endr
_ex_stack:	
	.long	0

		
