; DMA testcases, set 2

	.dmapackvif 1

test.cnt.1:
	dmacnt 2
	base 42
	offset 43
	.enddmadata

test.cnt.2:
	dmacnt *
	stcol 1,2,3,4
	.enddmadata

test.next.3:
	dmanext 2,next1
	stmask 0x12345678
	vifnop
	vifnop
	.enddmadata

test.next.4:
next1:
	dmanext *,next2
	vifnop
	strow 1,2,3,4
	.enddmadata
next2:

test.ref.5:
	dmaref 2, ref1
test.ref.6:
	dmaref *, ref2

ref1:
	vifnop
	vifnop
	vifnop
	vifnop
	.dmadata ref2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata
