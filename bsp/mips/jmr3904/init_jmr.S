/*
 * init_jmr.S -- Initialization for JMR R3904 PCI board.
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
	.file 1 "init_jmr.S"
	
#include <bsp/cpu.h>
	
	.text
        .align     2
	.set noreorder
	.set noat

	.macro port val
	li	k0,\val
	sb	k0,-2812(zero)
	 nop
	.endm

FUNC_START(_bsp_init_stack)
	/*
	 *  Initialize the DGA-001 Controller.
	 */
	# Initialize hardware
	# these mappings need to be setup before we
	# can use the stack and make calls to other
	# functions.

	# SCS0,1 base addr of ISA & PCI
	la	a0, 0xffffe010		# SCSAddr
	la	a1, 0x20201410		
	sw	a1, 0(a0)

	la	a0, 0xffffe014		# SCSMask
	la	a1, 0x0000fcfc
	sw	a1, 0(a0)

	# ROM Configuration , ROM Access condition
	la	a0, 0xffff9000		# RCCR0
	la	a1, 0x1fc00400
	sw	a1,0(a0)

	# SRAM config
	la	a0, 0xffff9100		# RCRR1 base address
	la	a1, 0x00000000		# waitcount
	sw	a1, 0(a0)

#if 0	/* Omitted because the sample provided omitted this */
	/* But, this is from the JMR board manual */
	# DRAM Configuration
	la	a0, 0xffff8000
	la	a1, 0x08024030
	sw	a1, 0(a0)
	
	# DBMR0
	la	a0, 0xffff8004
	la	a1, 0x00000000	
	sw	a1, 0(a0)
	
	# DWR0
	la	a0, 0xffff8008
	la	a1, 0x00000000
	sw	a1, 0(a0)
	
        # DREFC , (Depend on clock requency
	la	a0, 0xffff8800
	la	a1, 0x00000180
	sw	a1, 0(a0)
	
	# Interrupt Assignment
	# 0xffffe000 = 0x00001040		# CConR
	# in using int5, this should be 0x00001440
	la	a0, 0xffffe000
	la	a1, 0x00001040
	sw	a1, 0(a0)

	# ISA Controller
	# ISA Clock Frequency
	la	a0, 0xb2100000
	la	a1, 0x04		# 4 dividing
	sw	a1, 0(a0)
#endif	
	
	lui	t1,0xa000	/* RAM start */
	lui	t3,0xa010	/* RAM end   */
	lui	t2,0xa5a5
	ori	t2,t2,0xaa55	/* test pattern */
    1:	
	sw	zero,0(t1)
	sw	t2,4(t1)
	lw	a0,0(t1)
	nop
	bnez	a0,ram_test_done
	 nop
	sw	t2,0(t1)
	sw	zero,4(t1)
	lw	a0,0(t1)
	nop
	bne	a0,t2,ram_test_done
	 nop
	li	t0,4096
    2:
	sw	zero,0(t1)
	addiu	t0,t0,-4
	bnez	t0,2b
	 addiu	t1,t1,4
	bne	t1,t3,1b
	 nop
    ram_test_done:

	sll	s0,t1,3
	srl	s0,s0,3		/* physical address of top of memory */
	
	addiu	t1,t1,-4
	li	sp,-32
	and	sp,sp,t1

	sll	sp,sp,3
	srl	sp,sp,3
	lui	t1,0x8000
	or	sp,sp,t1

	/* enable ICache and Dcache */
	mfc0	k0,C0_CONFIG
	nop
	ori	k0,0x30
	mtc0	k0,C0_CONFIG

	j	ra
	 sw	s0,0(sp)	/* store pointer to be copied to
				   _bsp_ram_info_ptr, later */
FUNC_END(_bsp_init_stack)


/*
 *  Flush Icache. (Entire cache, for now).
 *
 *  void __icache_flush(void *va, int nbytes)
 */
FUNC_START(__icache_flush)
	mfc0	a2,C0_CONFIG
	nop
	ori	a3,a2,0x20
	xori	a3,a3,0x20
	mtc0	a3,C0_CONFIG
	b	1f
	nop
	nop
	nop
	nop
    1:
	lui	a0,0x8000
	ori	a1,a0,0x1000
	b	1f
	nop
	nop
	nop
	nop
    1:
	cache	INDEX_INVALIDATE_I,0(a0)
	nop
	addi	a0,a0,16
	bne	a0,a1,1b
	nop
	mtc0	a2,C0_CONFIG
	nop
	nop
	nop
	jr	ra
	 nop
FUNC_END(__icache_flush)


/*
 *  Flush entire Dcache.
 *
 *  void __dcache_flush(void *va, int nbytes)
 */
FUNC_START(__dcache_flush)
	beqz	a1,2f
	/*
	 * Must make sure its a cached address.
	 *
	 * For tx39, these addresses are uncached:
	 *   x111 1111 xxxx xxxx xxxx xxxx xxxx xxxx
	 *   101x xxxx xxxx xxxx xxxx xxxx xxxx xxxx
	 *
	 * Danger, Will Robinson! The following assumes
	 * the given address range is all cached or all
	 * uncached.
	 */
	srl	t2,a0,24
	andi	t3,t2,0x7f
	xori	t3,t3,0x7f
	andi	t2,t2,0xe0
	xori	t2,t2,0xa0
	or	t2,t2,t3
	beqz	t2,2f		/* branch on uncached addresses */
	 andi	t1,a0,3
	li	t2,-4
	and	a0,a0,t2	/* floor va to word address */
	add	a1,a1,t1	/* adjust count for flooring of address */
	addiu	a1,a1,3		/* add for partial word */
    1:
	cache	HIT_INVALIDATE_D,0(a0)
	addiu	a1,a1,-4
	bgez	a1,1b
	 addiu	a0,a0,4
    2:
	jr	ra
	 nop
FUNC_END(__dcache_flush)



		
