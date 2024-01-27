#
# crt0b.S -- startup file for skyb
#
# Copyright (c) 1995, 1996, 1997, 1998 Cygnus Support
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
#

.ifdef __mips16
# This file contains 32 bit assembly code.
	.set nomips16
.endif

# Useful memory constants:
	.equ K0BASE,	0x80000000

# Standard Status Register bitmasks:
	.equ SR_CU1,	0x20000000	# Mark CP1 as usable
	.equ SR_FR,	0x04000000	# Enable MIPS III FP registers
	.equ SR_BEV,	0x00400000	# Controls location of exception vectors
	.equ SR_PE,	0x00100000	# Mark soft reset (clear parity error)

	.equ SR_KX,	0x00000080	# Kernel extended addressing enabled
	.equ SR_SX,	0x00000040	# Supervisor extended addressing enabled
	.equ SR_UX,	0x00000020	# User extended addressing enabled

#
# Set up some room for a stack. We just grab a chunk of memory.
#
	.equ STACK_SIZE,  0x4000
	.equ GLOBAL_SIZE, 0x2000

	.equ STARTUP_STACK_SIZE,	0x0100

	.comm	__memsize, 12
	.comm	__lstack, STARTUP_STACK_SIZE
	.comm   __stackbase,4

	.text
	.align	2

# Without the following nop, GDB thinks _start is a data variable.
# This is probably a bug in GDB in handling a symbol that is at the
# start of the .text section.
	nop

	.globl	_start
	.ent	_start
_start:
	.set	noreorder
	li	$2, SR_CU1|SR_PE|SR_FR|SR_KX|SR_SX|SR_UX
	mtc0	$2, $12
	mtc0	$0, $13

# Check for FPU presence
.ifndef __mips_soft_float
# This does not work if there is no FPU -- illegal instruction exceptions. 
	li	$10,0xAAAA5555
	mtc1	$10,$f0		# write to FPR 0
	mtc1	$0,$f1		# write to FPR 1
	mfc1	$8,$f0
	mfc1	$9,$f1
	nop
	bne	$8,$10,1f	# check for match
	nop
	bne	$9,$0,1f	# double check
	nop
	j	2f		# status register already correct 
	nop
.endif
1:
	li	$2, SR_PE|SR_FR|SR_KX|SR_SX|SR_UX
	mtc0	$2, $12
2:
	.end _start

# zero out the bss section.

	.globl	__memsize
	.globl	get_mem_info .text
	.globl	__stack
	.globl	__global
	.globl	zerobss
	.ent	zerobss
zerobss:
	la	$2, _fbss
	la	$3, _end
3:
	sw	$0,0($2)
	bltu	$2,$3,3b
	addiu	$2,$2,4				# executed in delay slot

	la	$8, __lstack			# make a small stack so we
	addiu	$29, $8, STARTUP_STACK_SIZE	# can run some C code
	la	$4, __memsize			# get the usable memory size
	jal	get_mem_info
	nop

	# setup the stack pointer
	la	$8,__stack			# is __stack set ?
	bne	$8,$0,4f
	nop

	# NOTE: $4[0] contains the amount of memory available, and
	#        not the last memory address.
	lw	$8,0($4)			# last address of memory available
	la	$9,K0BASE			# cached kernel memory
	addu	$8,$8,$9			# get the end of memory address
	# We must subtract 24 bytes for the 3 8 byte arguments to main, in
	# case main wants to write them back to the stack.  The caller is
	#  supposed to allocate stack space for parameters in registers in
	#  the old MIPS ABIs.  We must do this even though we are not passing
	#  arguments, because main might be declared to have them. 

	#  Some ports need a larger alignment for the stack, so we
	#  subtract 32, which satisifes the stack for the arguments and keeps 
	#  the stack pointer better aligned. 
	subu	$8,$8,32			# and generate a starting stack-pointer
4:
	move	$29,$8				# set stack pointer
	la	$28,_gp				# set the global data pointer
	sw	$29,__stackbase			# keep this for future ref
	.end	zerobss

# Initialize target specific stuff.
# Only execute these functions it they exist.

	.globl	hardware_init_hook .text
	.globl	software_init_hook .text
	.globl	__do_global_dtors .text
	.globl	atexit .text
	.globl	exit .text
	.globl	init
	.ent	init
init:
	la	$25,hardware_init_hook		# init the hardware if needed
	beq	$25,$0,6f
	nop
	jal	$25
	nop
6:
	la	$25,software_init_hook		# init the software if needed
	beq	$25,$0,7f
	nop
	jal	$25
	nop
7:
	la	$4,__do_global_dtors
	jal	atexit
	nop

.ifdef GCRT0
	.globl	_ftext
	.globl	_extext
	la	$4,_ftext
	la	$5,_etext
	jal	monstartup
	nop
.endif

	jal	main			# call the program start function
	nop

	# fall through to the "exit" routine
	jal	exit			# call libc exit to run the G++
					# destructors
	move	$4,$2			# pass through the exit code
	.end	init
	
#
# _exit -- Exit from the application. Normally we cause a user trap
#          to return to the ROM monitor for another run. NOTE: This is
#	    the only other routine we provide in the crt0.o object, since
#          it may be tied to the "_start" routine. It also allows
#          executables that contain a complete world to be linked with
#          just the crt0.o object.

	.globl	_exit
	.ent _exit
_exit:
7:
.ifdef GCRT0
	jal	_mcleanup
	nop
.endif
	# break instruction can cope with 0xfffff, but GAS limits the range:
	break	1023
	nop
	b	7b				# but loop back just in-case
	nop
	.end _exit

# EOF crt0.S
