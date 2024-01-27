	.text
	mpg 0x80,*
label1:
	nop nop
label2:
	nop nop
	.EndMpg
	mscal 0x80
	mpg label2,*
	nop nop
	.EndMpg
	mscal 0x80
	mpg label2,*
	nop nop
	.EndMpg
	mscal 0x80
