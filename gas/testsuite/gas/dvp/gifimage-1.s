; GIFpacked tests

; The examples aren't intended to be useful or even coherent.

foo:
	gifimage nloop=0, eop
	.endgif

	gifimage eop
	.endgif

	gifimage nloop=0
	.endgif

	gifimage
	.int 1,2,3,4
	.int 5,6,7,8
	.endgif
