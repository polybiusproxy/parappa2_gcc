; GIFreglist tests

; The examples aren't intended to be useful or even coherent.

foo:
	gifreglist regs={rgbaq}, nloop=0
	.endgif

	gifreglist regs={rgbaq}, nloop=0, eop
	.endgif

	gifreglist regs={rgbaq}, eop
	.endgif

	gifreglist regs={tex0_1,tex0_2,clamp_1,clamp_2}, nloop=0
	.endgif

	gifreglist regs={xyzf,xyzf3,xyz3}, nloop=3
	.int 1,1,2,2,3,3
	.int 4,4,5,5,6,5
	.int 7,7,8,8,9,9
	.endgif

	gifreglist regs={xyzf,xyzf3}
	.int 1,2,3,4
	.endgif
