; Relaxation test #1.

foo:
	dmacnt *
	direct *
	gifpacked regs={a_d}, nloop=1, eop
	.int	0x00000000, 0x0000004c, 0x00000000, 0x000a0000
	.endgif
	.enddirect
	.enddmadata

; .org affects relaxing (pr 16132)

dot_org_test:
	; This is disassembled with --disassemble-zeroes so don't make the
	; arg to .org too large.
	.org 0x40
	mpg 20,*
	nop nop
	.endmpg
