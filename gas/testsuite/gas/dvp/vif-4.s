; Miscellaneous unpack tests.

foo:
	STMASK 0x55555555
	STROW 0xffff, 0xffff, 0xffff, 0xffff
	STCYCL 0, 0
	unpack[m] V3_8, 0, 0
	.EndUnpack
	unpack[m] V3_8, 0, 256
	.EndUnpack

	STCYCL 64, 0
	unpack[m] V3_8, 0, 192
	.EndUnpack
