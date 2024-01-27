; Branch test #2.

foo:
	mpg 0,*
foo1:
	nop b foo2
	nop nop
	.endmpg

	mpg 0x100,*
foo2:
	nop b foo1
	nop nop
	.endmpg
