; Test unpack -> stcycl/unpack macro.

foo:
	unpack 1,1,s_32,0,*
	.int 0
	.endunpack

	unpack 2,2,s_32,1,1
	.int 1
	.endunpack

	UNPACK 1,1,S_32,2,1
	.int 2
	.endunpack

	UNPACK[MR] 1 , 1 , s_32 , 3 , 1
	.int 3
	.endunpack

	unpack[mr] 2 , 1 , s_32 , 4 , *
	.int 4
	.endunpack

	unpack[mr] 1 , 2 , s_32 , 5 , 1
	.int 5
	.endunpack
