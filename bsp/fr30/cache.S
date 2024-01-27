/*
 * cache.S -- FR30 I-cache ops.
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
	.file "cache.S"

	.text	

#define ENAB	1
#define FLSH	2
#define ELKR	4
#define EOLK	8
#define ALFL   16
#define GBLK   32

	.globl	__icache_init
__icache_init:
	ldi:20	#0x3e7,r4
	ldi:8	#FLSH+ELKR,r5
	stb	r5,@r4
	ret

	.globl	__icache_enable
__icache_enable:
	ldi:20	#0x3e7,r4
	ldi:8	#ENAB,r5
	stb	r5,@r4
	nop
	nop
	nop
	ret

	.globl	__icache_disable
__icache_disable:
	ldi:20	#0x3e7,r4
	bandl	#0xe,@r4
	nop
	nop
	nop
	ret

	.globl	__icache_flush
__icache_flush:
	ldi:20	#0x3e7,r4
	borl	#FLSH,@r4
	nop
	nop
	nop
	ret

	


	