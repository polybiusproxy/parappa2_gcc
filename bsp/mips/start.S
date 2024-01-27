/*
 * start.S -- MIPS BSP startup.
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
#include "syscall.h"

	.file 1 "start.S"

	.extern	SYM_NAME(_bsp_init_stack)
	.extern	SYM_NAME(_bsp_init)
	.extern	SYM_NAME(_start)


	.section ".vectors","ax"
	
	.set noreorder
	.set noat

	/*
	 *  Reset, Soft Reset, NMI Vector.
	 *
	 *  This vector is always in ROM.
	 *
	 *  Jump to actual startup code using an uncached address.
	 *  
	 */
	la	k1,SYM_NAME(_start)
	sll	k1,k1,3		/* strip off address space bits */
	srl	k1,k1,3
	lui	k0,0xa000
	or	k0,k0,k1	/* force to uncached space */
	jr	k0
	 nop

	/*
	 *  For R3000 class processors, the next ROM vector is at
	 *  0xbfc00100. For R4000 class processors, 0xbfc00200.
	 */
#ifdef _R3000
	.p2align 8
#else
	.p2align 9
#endif

__vec_start:
	/*
	 *  TLB Refill Exception.
	 */
	j	SYM_NAME(__exception_handler)
	 li	k0,BSP_EXC_TLB
	
#ifndef _R3000	
	/*
	 *  XTLB Refill Exception.
	 */
	.p2align 7
	j	SYM_NAME(__exception_handler)
	 li	k0,BSP_EXC_XTLB

	/*
	 *  Cache Error Exception.
	 */
	.p2align 7
	j	SYM_NAME(__exception_handler)
	 li	k0,BSP_EXC_CACHE
#endif

	/*
	 *  General Exception Handler.
	 */
	.p2align 7

	la	k1,SYM_NAME(__exception_handler)
	sll	k1,k1,3		/* strip off address space bits */
	srl	k1,k1,3
	lui	k0,0xa000
	or	k1,k1,k0	/* force to uncached space */
	
	mfc0	k0,C0_CAUSE
	 nop
	srl	k0,k0,2
	andi	k0,k0,0x1f
	jr	k1
	 addi	k0,k0,BSP_EXC_INT

__vec_end:

	.text
	.p2align 2
	
#if defined(_R3900)
#define INIT_SR	SR_BEV
#endif

FUNC_START(_start)
	
#if defined(_R3900)
	/* check for NMI */
	mfc0	k0,C0_STATUS
	lui	k1,0x10
	and	k1,k1,k0
	beqz	k1,1f
	 nop
	mtc0	k0,C0_STATUS
	nop
	j	SYM_NAME(__exception_handler)
	 li	k0,BSP_EXC_NMI
    1:
#endif
	mtc0	zero,C0_CAUSE	/* reset SW interrupt bits */
	
	li	k0,INIT_SR
	mtc0	k0,C0_STATUS	/* set initial SR */
	
	la	k1,SYM_NAME(_bsp_init_stack)
	sll	k1,k1,3		/* strip off address space bits */
	srl	k1,k1,3
	lui	k0,0xa000
	or	k0,k0,k1	/* force to uncached space */
	jalr	k0		/* call _bsp_init_stack */
	 nop

	
	/*
	 *  _bsp_init_stack should have initialized the caches,
	 *  so let's run cached if linked to do so.
	 */
	j	1f
	 nop
    1:
	
	/*
	 *  Move vectors into RAM using uncached addresses.
	 */	
        la      a0,__vec_start
        lui     a1,0xa000
        la      a2,__vec_end
    1:
	lw	t1,0(a0)
	addiu	a0,a0,4
	sw	t1,0(a1)
	bne	a0,a2,1b
	 addiu	a1,a1,4

	/*
	 *  Clear SR_BEV so we use the RAM vectors.
	 */
	mfc0	t0,C0_STATUS
	li	t1,SR_BEV
	or	t0,t0,t1
	xor	t0,t0,t1
	mtc0	t0,C0_STATUS

	/*
	 *  Copy data from ROM to ram
	 */
	la	a0,SYM_NAME(__rom_data_start)
	la	a1,SYM_NAME(__ram_data_start)
	la	a2,SYM_NAME(__ram_data_end)
	beq	a1,a2,2f
	 nop
    1:
	lw	t0,0(a0)
	nop
	sw	t0,0(a1)
	addiu	a1,a1,4
	bne	a1,a2,1b
	 addiu	a0,a0,4
    2:

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

	/*
	 * Copy pointer to board-specific RAM info from stack to variable.
	 */
	lw	t0,0(sp)
	lui	a0,%hi(SYM_NAME(_bsp_ram_info_ptr))
	sw	t0,%lo(SYM_NAME(_bsp_ram_info_ptr))(a0)

	/* set the global pointer */
	la	gp,SYM_NAME(_gp)

	/*
	 * Call _bsp_init to do all remaining setup actions which
	 * need to be done after .data and .bss have been initialized.
	 */
	jal	SYM_NAME(_bsp_init)
	 nop

	/*
	 * Enable interrupts.
	 */
	mfc0	a0,C0_STATUS
	nop
	ori	a0,a0,0xff01
	mtc0	a0,C0_STATUS
	
	li	a0,0
	li	a1,0
	jal	SYM_NAME(main)
	 addiu	sp,sp,-32
	addiu	sp,sp,24	/* leave room for exit arg */
	sw	v0,0(sp)
    9:
	jal	exit
	 lw	a0,0(sp)	/* main's return value */
	b	9b
FUNC_END(_start)



/*
 *  The real work of BSP exception handling is done here.
 *  On entry, k0 contains the BSP exception number.
 */
FUNC_START(__exception_handler)
	/*
	 *  Make sure to use exception stack.
	 *  Since, CP0 is always usable in kernel mode, we can
	 *  use the CU0 bit of the status register to indicate
	 *  that we are using the exception stack.
	 */
	mfc0	k1,C0_STATUS
	nop					/* wait for reg data */
	sll	k1,k1,3
	bltz	k1,1f
	 move	k1,sp
	lui	sp,%hi(_ex_stack)
	addi	sp,sp,%lo(_ex_stack)
    1:
	sw	k1,FR_REG29 - EX_STACK_SIZE(sp)	/* save original stack */
	li	k1,EX_STACK_SIZE
	subu	sp,sp,k1
	mfc0	k1,C0_STATUS
	sw	$0,FR_REG0(sp)
	sw	$1,FR_REG1(sp)
	sw	k1,FR_SR(sp)
	li	$1,SR_CU0
	or	k1,k1,$1
	mtc0	k1,C0_STATUS
	nop
	mfc0	k1,C0_BADVA
	sw	$2,FR_REG2(sp)
	sw	k1,FR_BAD(sp)
	sw	$3,FR_REG3(sp)
	sw	$4,FR_REG4(sp)
	sw	$5,FR_REG5(sp)
	sw	$6,FR_REG6(sp)
	sw	$7,FR_REG7(sp)
	sw	$8,FR_REG8(sp)
	sw	$9,FR_REG9(sp)
	sw	$10,FR_REG10(sp)
	sw	$11,FR_REG11(sp)
	sw	$12,FR_REG12(sp)
	sw	$13,FR_REG13(sp)
	sw	$14,FR_REG14(sp)
	sw	$15,FR_REG15(sp)
	sw	$16,FR_REG16(sp)
	sw	$17,FR_REG17(sp)
	sw	$18,FR_REG18(sp)
	sw	$19,FR_REG19(sp)
	sw	$20,FR_REG20(sp)
	sw	$21,FR_REG21(sp)
	sw	$22,FR_REG22(sp)
	sw	$23,FR_REG23(sp)
	sw	$24,FR_REG24(sp)
	sw	$25,FR_REG25(sp)
	/* no need to save k0/k1 */
	sw	$28,FR_REG28(sp)
	/* already saved sp */
	sw	$30,FR_REG30(sp)
	sw	$31,FR_REG31(sp)
	mfhi	k1
	sw	k1,FR_HI(sp)
	mflo	k1
	sw	k1,FR_LO(sp)
	mfc0	k1,C0_EPC
	nop
	sw	k1,FR_EPC(sp)
	
#ifndef __mips_soft_float
	mfc0	k1,C0_STATUS
	sll	t0,k1,2         /* use CU1 bit as sign bit */
	bgez	t0,3f		/* branch if FPU disabled */
	 nop
#ifdef __mips64
	sll	t0,k1,5		/* use FR bit as sign bit */
	bgez	t0,1f		/* branch if FPU in 32-bit mode */
	 nop
	sdc1	$0,FR_FPR0(sp)
	sdc1	$1,FR_FPR1(sp)
	sdc1	$2,FR_FPR2(sp)
	sdc1	$3,FR_FPR3(sp)
	sdc1	$4,FR_FPR4(sp)
	sdc1	$5,FR_FPR5(sp)
	sdc1	$6,FR_FPR6(sp)
	sdc1	$7,FR_FPR7(sp)
	sdc1	$8,FR_FPR8(sp)
	sdc1	$9,FR_FPR9(sp)
	sdc1	$10,FR_FPR10(sp)
	sdc1	$11,FR_FPR11(sp)
	sdc1	$12,FR_FPR12(sp)
	sdc1	$13,FR_FPR13(sp)
	sdc1	$14,FR_FPR14(sp)
	sdc1	$15,FR_FPR15(sp)
	sdc1	$16,FR_FPR16(sp)
	sdc1	$17,FR_FPR17(sp)
	sdc1	$18,FR_FPR18(sp)
	sdc1	$19,FR_FPR19(sp)
	sdc1	$20,FR_FPR20(sp)
	sdc1	$21,FR_FPR21(sp)
	sdc1	$22,FR_FPR22(sp)
	sdc1	$23,FR_FPR23(sp)
	sdc1	$24,FR_FPR24(sp)
	sdc1	$25,FR_FPR25(sp)
	sdc1	$26,FR_FPR26(sp)
	sdc1	$27,FR_FPR27(sp)
	sdc1	$28,FR_FPR28(sp)
	sdc1	$29,FR_FPR29(sp)
	sdc1	$30,FR_FPR30(sp)
	sdc1	$31,FR_FPR31(sp)
	b	2f
	 nop
    1:
#endif
	swc1	$0,FR_FPR0(sp)
	swc1	$1,FR_FPR1(sp)
	swc1	$2,FR_FPR2(sp)
	swc1	$3,FR_FPR3(sp)
	swc1	$4,FR_FPR4(sp)
	swc1	$5,FR_FPR5(sp)
	swc1	$6,FR_FPR6(sp)
	swc1	$7,FR_FPR7(sp)
	swc1	$8,FR_FPR8(sp)
	swc1	$9,FR_FPR9(sp)
	swc1	$10,FR_FPR10(sp)
	swc1	$11,FR_FPR11(sp)
	swc1	$12,FR_FPR12(sp)
	swc1	$13,FR_FPR13(sp)
	swc1	$14,FR_FPR14(sp)
	swc1	$15,FR_FPR15(sp)
	swc1	$16,FR_FPR16(sp)
	swc1	$17,FR_FPR17(sp)
	swc1	$18,FR_FPR18(sp)
	swc1	$19,FR_FPR19(sp)
	swc1	$20,FR_FPR20(sp)
	swc1	$21,FR_FPR21(sp)
	swc1	$22,FR_FPR22(sp)
	swc1	$23,FR_FPR23(sp)
	swc1	$24,FR_FPR24(sp)
	swc1	$25,FR_FPR25(sp)
	swc1	$26,FR_FPR26(sp)
	swc1	$27,FR_FPR27(sp)
	swc1	$28,FR_FPR28(sp)
	swc1	$29,FR_FPR29(sp)
	swc1	$30,FR_FPR30(sp)
	swc1	$31,FR_FPR31(sp)
    2:	
	mfc1	k1,CP1_STATUS
	sw	k1,FR_FSR(sp)
	mfc1	k1,CP1_REVISION
	sw	k1,FR_FIR(sp)
    3:
#endif

#if defined(_R3900)
	mfc0	t1,C0_CONFIG
	mfc0	t2,C0_CACHE
	sw	t1,FR_CONFIG(sp)
	mfc0	t1,C0_DEBUG
	sw	t2,FR_CACHE(sp)
	mfc0	t2,C0_DEPC
	sw	t1,FR_DEBUG(sp)
	sw	t2,FR_DEPC(sp)
#endif
	
	move	a0,k0		/* BSP exception number */
	mfc0	k0,C0_CAUSE
	move	a1,sp		/* saved regs pointer */
	sw	k0,FR_CAUSE(sp)
	jal	SYM_NAME(_bsp_exc_dispatch)
	 move	s0,a0

	/*
	 * enter the debugger stub if exception wasn't handled
	 * by application code.
	 */
	bnez	v0,1f
	 la	t0,SYM_NAME(_bsp_dbg_vector)
	lw	t0,0(t0)
	move	a1,sp
	jalr	t0
	 move	a0,s0
    1:
#if defined(_R3900)
	lw	t1,FR_CONFIG(sp)
	lw	t2,FR_CACHE(sp)
	mtc0	t1,C0_CONFIG
	lw	t1,FR_DEBUG(sp)
	mtc0	t2,C0_CACHE
	lw	t2,FR_DEPC(sp)
	mtc0	t1,C0_DEBUG
	mtc0	t2,C0_DEPC
#endif
	
#ifndef __mips_soft_float
	mfc0	k1,C0_STATUS
	sll	t0,k1,2         /* use CU1 bit as sign bit */
	bgez	t0,3f		/* branch if FPU disabled */
	 nop
#ifdef __mips64
	sll	t0,k1,5		/* use FR bit as sign bit */
	bgez	t0,1f		/* branch if FPU in 32-bit mode */
	 nop
	ldc1	$31,FR_FPR31(sp)
	ldc1	$30,FR_FPR30(sp)
	ldc1	$29,FR_FPR29(sp)
	ldc1	$28,FR_FPR28(sp)
	ldc1	$27,FR_FPR27(sp)
	ldc1	$26,FR_FPR26(sp)
	ldc1	$25,FR_FPR25(sp)
	ldc1	$24,FR_FPR24(sp)
	ldc1	$23,FR_FPR23(sp)
	ldc1	$22,FR_FPR22(sp)
	ldc1	$21,FR_FPR21(sp)
	ldc1	$20,FR_FPR20(sp)
	ldc1	$19,FR_FPR19(sp)
	ldc1	$18,FR_FPR18(sp)
	ldc1	$17,FR_FPR17(sp)
	ldc1	$16,FR_FPR16(sp)
	ldc1	$15,FR_FPR15(sp)
	ldc1	$14,FR_FPR14(sp)
	ldc1	$13,FR_FPR13(sp)
	ldc1	$12,FR_FPR12(sp)
	ldc1	$11,FR_FPR11(sp)
	ldc1	$10,FR_FPR10(sp)
	ldc1	$9,FR_FPR9(sp)
	ldc1	$8,FR_FPR8(sp)
	ldc1	$7,FR_FPR7(sp)
	ldc1	$6,FR_FPR6(sp)
	ldc1	$5,FR_FPR5(sp)
	ldc1	$4,FR_FPR4(sp)
	ldc1	$3,FR_FPR3(sp)
	ldc1	$2,FR_FPR2(sp)
	ldc1	$1,FR_FPR1(sp)
	ldc1	$0,FR_FPR0(sp)
	b	2f
	 nop
    1:
#endif
	lwc1	$31,FR_FPR31(sp)
	lwc1	$30,FR_FPR30(sp)
	lwc1	$29,FR_FPR29(sp)
	lwc1	$28,FR_FPR28(sp)
	lwc1	$27,FR_FPR27(sp)
	lwc1	$26,FR_FPR26(sp)
	lwc1	$25,FR_FPR25(sp)
	lwc1	$24,FR_FPR24(sp)
	lwc1	$23,FR_FPR23(sp)
	lwc1	$22,FR_FPR22(sp)
	lwc1	$21,FR_FPR21(sp)
	lwc1	$20,FR_FPR20(sp)
	lwc1	$19,FR_FPR19(sp)
	lwc1	$18,FR_FPR18(sp)
	lwc1	$17,FR_FPR17(sp)
	lwc1	$16,FR_FPR16(sp)
	lwc1	$15,FR_FPR15(sp)
	lwc1	$14,FR_FPR14(sp)
	lwc1	$13,FR_FPR13(sp)
	lwc1	$12,FR_FPR12(sp)
	lwc1	$11,FR_FPR11(sp)
	lwc1	$10,FR_FPR10(sp)
	lwc1	$9,FR_FPR9(sp)
	lwc1	$8,FR_FPR8(sp)
	lwc1	$7,FR_FPR7(sp)
	lwc1	$6,FR_FPR6(sp)
	lwc1	$5,FR_FPR5(sp)
	lwc1	$4,FR_FPR4(sp)
	lwc1	$3,FR_FPR3(sp)
	lwc1	$2,FR_FPR2(sp)
	lwc1	$1,FR_FPR1(sp)
	lwc1	$0,FR_FPR0(sp)
    2:	
	lw	k0,FR_FSR(sp)
	 nop
	mtc1	k0,CP1_STATUS
    3:
#endif

	lw	k1,FR_HI(sp)
	lw	k0,FR_LO(sp)
	mthi	k1
	lw	k1,FR_SR(sp)
	mtlo	k0
	mtc0	k1,C0_STATUS
	lw	k0,FR_EPC(sp)
	lw	$31,FR_REG31(sp)
#ifndef _R3000
	/* EPC is read-only on r3k */
	mtc0	k0,C0_EPC
#endif
	lw	$30,FR_REG30(sp)
	/* sp gets restored last */
	lw	$28,FR_REG28(sp)
	/* no need to restore k0/k1 */
	lw	$25,FR_REG25(sp)
	lw	$24,FR_REG24(sp)
	lw	$23,FR_REG23(sp)
	lw	$22,FR_REG22(sp)
	lw	$21,FR_REG21(sp)
	lw	$20,FR_REG20(sp)
	lw	$19,FR_REG19(sp)
	lw	$18,FR_REG18(sp)
	lw	$17,FR_REG17(sp)
	lw	$16,FR_REG16(sp)
	lw	$15,FR_REG15(sp)
	lw	$14,FR_REG14(sp)
	lw	$13,FR_REG13(sp)
	lw	$12,FR_REG12(sp)
	lw	$11,FR_REG11(sp)
	lw	$10,FR_REG10(sp)
	lw	$9,FR_REG9(sp)
	lw	$8,FR_REG8(sp)
	lw	$7,FR_REG7(sp)
	lw	$6,FR_REG6(sp)
	lw	$5,FR_REG5(sp)
	lw	$4,FR_REG4(sp)
	lw	$3,FR_REG3(sp)
	lw	$2,FR_REG2(sp)
	lw	$1,FR_REG1(sp)
	lw	sp,FR_REG29(sp)

#ifdef _R3000
	jr	k0
	 rfe
#else
	eret
#endif
	nop
FUNC_END(__exception_handler)


	/*
	 *  Provide a stack for use exceptions and interrupts.
	 *  Note that this stack is in .bss, so it will be below
	 *  the normal user stack which is placed in high memory.
	 */
	.section ".bss","a"
	.p2align 4
	.rept	4096
	.long	0
	.endr
  _ex_stack:	

