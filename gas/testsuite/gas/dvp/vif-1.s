; VIF tests

foo:
	vifnop
	stcycl 1, 2
	offset 42
	base[i] 42
	itop 42
	stmod direct
	stmod add
	stmod addrow
	mskpath3 disable
	mskpath3 enable
	mark 0x1234
	flushe
	flush
	flusha[i]
	mscal 0
	mscnt
	mscalf 0

	stmask 0x12345678

	strow 1, 2, 3, 4
	stcol 5, 6, 7, 8

; For the filename tests, we assume the driver script will pass the
; right -I argument to find the file.

	mpg 0, "nop.vu"

	mpg *, 2
	nop nop
	nop nop
	.endmpg

	mpg *, *
	nop nop
	nop nop
	.endmpg

#direct "filename"

	direct 2
	gifpacked regs={a_d}, nloop=1, eop 
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif
	.enddirect

	direct *
	gifpacked regs={a_d}, nloop=1, eop 
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif
	.enddirect

#directhl "filename"

	directhl 2
	gifpacked regs={a_d}, nloop=1, eop 
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif
	.enddirect

	directhl *
	gifpacked regs={a_d}, nloop=1, eop 
	.int 0x000a0000, 0x00000000, 0x4c, 0x00
	.endgif
	.enddirect

# The stcycl value must be set correctly before we can use unpack.

	stcycl 1, 1

#unpack       S_32,  0, "filename"
#unpack[i]    S_16,  *, "filename"

# No, it's not intended that the data here be meaningful.

	unpack       S_32,  0, *
	.int 0x12345678
	.endunpack

	unpack[i]    S_16,  1, *
	.short 0x1234
	.endunpack

	unpack[m]    S_8,   2, *
	.byte 0,1,2,3
	.endunpack

	unpack[r]    V2_32, 3, *
	.int 0,1,2,3
	.endunpack

	unpack[imr]  V2_16, 4, *
	.short 0,1,2,3
	.endunpack

	unpack[imru] V2_8,  5, *
	.byte 0,1,2,3
	.endunpack

	unpack       V3_32, 6, *
	.int 0,1,2,3,4,5
	.endunpack

	unpack       V3_16, 7, *
	.short 0,1,2,3,4,5
	.endunpack

	unpack       V3_8,  8, *
	.byte 0,1,2
	.endunpack

	unpack       V4_32, 9, *
	.int 0,1,2,3
	.endunpack

	unpack       V4_16, 10, *
	.short 0,1,2,3
	.endunpack

	unpack       V4_8,  11, *
	.byte 0,1,2,3
	.endunpack

	unpack       V4_5,  12, *
	.short 0,1,2,3
	.endunpack
