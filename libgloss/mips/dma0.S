	.file	1 "dma0.c"
gcc2_compiled.:
__gnu_compiled_c:
	.rdata
	.align	3
$LC0:
	.ascii	"stopped - Reserved Instruction Error\n\000"
	.text
	.align	2
	.globl	main
	.ent	main
main:
	.frame	$fp,64,$31		# vars= 16, regs= 2/0, args= 32, extra= 0
	.mask	0xc0000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,64
	sd	$31,56($sp)
	sd	$fp,48($sp)
	move	$fp,$sp
###	jal	__main			### Commented out of compiler output.
	la	$2,__eovutext
	la	$3,__bovutext
	subu	$2,$2,$3
	sw	$2,32($fp)
	li	$2,1			# 0x1
	#.set	volatile
	sw	$2,-1878990848
	#.set	novolatile
	li	$2,-2147352576			# 0x80020000
	#.set	volatile
	sw	$2,-1878990816
	#.set	novolatile
	la	$2,__bovutext
	#.set	volatile
	sw	$2,-1879011312
	#.set	novolatile
	lw	$3,32($fp)
	srl	$2,$3,4
	#.set	volatile
	sw	$2,-1879011296
	#.set	novolatile
	li	$2,257			# 0x101
	#.set	volatile
	sw	$2,-1879011328
	#.set	novolatile
$L2:
	#.set	volatile
	lw	$2,-1879032832
	#.set	novolatile
	sw	$2,36($fp)
	#.set	volatile
	lw	$2,-1862241328
	#.set	novolatile
	sw	$2,40($fp)
	lw	$3,36($fp)
	andi	$2,$3,0x2000
	beq	$2,$0,$L4
	li	$4,1
	la	$5,$LC0
	li	$6,37
	jal	write
	j	$L3
$L5:
$L4:
	lw	$3,36($fp)
	andi	$2,$3,0x3
	bne	$2,$0,$L6
	lw	$2,36($fp)
	li	$3,520093696			# 0x1f000000
	and	$2,$2,$3
	bne	$2,$0,$L6
	lw	$3,40($fp)
	andi	$2,$3,0x100
	bne	$2,$0,$L6
	j	$L3
$L6:
	j	$L2
$L3:
	.set	noreorder
	nop
	.set	reorder
	sw	$0,44($fp)
$L7:
	lw	$2,44($fp)
	li	$3,196608			# 0x30000
	ori	$3,$3,0xd3f
	slt	$2,$3,$2
	beq	$2,$0,$L9
	j	$L8
$L10:
$L9:
	lw	$2,44($fp)
	addu	$3,$2,1
	sw	$3,44($fp)
	j	$L7
$L8:
	move	$2,$0
	j	$L1
$L1:
	move	$sp,$fp
	ld	$31,56($sp)
	ld	$fp,48($sp)
	addu	$sp,$sp,64
	j	$31
	.end	main

#################################
#
# All following code was manually added to the compiler output.
# It supplies entry points referenced by crt0.S but not needed.
#

	.globl	__do_global_dtors
	.ent	__do_global_dtors
__do_global_dtors:
	move	$2,$0
	j	$31
	.end	__do_global_dtors


	.globl	atexit
	.ent	atexit
atexit:
	move	$2,$0
	j	$31
	.end	atexit


	.globl	exit
	.ent	exit
exit:
	move	$2,$0
	j	$31
	.end	exit
