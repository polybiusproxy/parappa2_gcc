/* Test automagic alignment.  */

	.text

	.p2align 4
	.byte 1
dmalab:
	dmacnt *
	mscal 0
	.enddmadata
	dmaend

	.p2align 4
	.byte 1
giflab:
	gifreglist regs={rgbaq}, nloop=0
	.endgif
endgif:
	.p2align 4
	.byte 1
	.vu
vulab:
	abs.xyz vf01xyz, vf00xyz	nop
