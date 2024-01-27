; GIFpacked tests

; The examples aren't intended to be useful or even coherent.

foo:
	gifpacked regs={prim}, nloop=0
	.endgif

	gifpacked regs={rgbaq}, nloop=0
	.endgif

	gifpacked regs={st}, nloop=0
	.endgif

	gifpacked regs={uv}, nloop=0
	.endgif

	gifpacked regs={xyzf2}, nloop=0
	.endgif

	gifpacked regs={xyz2}, nloop=0
	.endgif

	gifpacked regs={tex0_1}, nloop=0
	.endgif

	gifpacked regs={tex0_2}, nloop=0
	.endgif

	gifpacked regs={clamp_1}, nloop=0
	.endgif

	gifpacked regs={clamp_2}, nloop=0
	.endgif

	gifpacked regs={xyzf}, nloop=0
	.endgif

	gifpacked regs={xyzf3}, nloop=0
	.endgif

	gifpacked regs={xyz3}, nloop=0
	.endgif

	gifpacked regs={a_d}, nloop=0
	.endgif

	gifpacked regs={nop}, nloop=0
	.endgif

; Random gifpacked examples.

	gifpacked regs={xyzf,xyzf3,xyz3}, nloop=0
	.endgif

	gifpacked regs={xyzf,xyzf3,xyz3}, nloop=3
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked prim=0x42,regs={a_d},nloop=1
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked prim=0x42,regs={a_d},nloop=1,eop
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked prim=0x42,regs={a_d},eop
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked prim=0x42,regs={a_d}
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked regs={a_d}
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif

	gifpacked regs={}, nloop=0, eop
	.endgif

	gifpacked regs={}, eop
	.endgif
