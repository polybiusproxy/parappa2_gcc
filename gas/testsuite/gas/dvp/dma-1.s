; DMA testcases, set 1

	.dmapackvif 0

test.cnt.1:
	dmacnt 2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.cnt.2:
	dmacnt *
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.next.3:
	dmanext 2,next1
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata
test.next.4:
next1:
	dmanext *,next2
	vifnop
	vifnop
	vifnop
	vifnop
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

test.refs.7:
	dmarefs 2, ref1
test.refs.8:
	dmarefs *, ref2

test.refe.9:
	dmarefe 2, ref1
test.refe.10:
	dmarefe *, ref2

test.call.11:
	dmacall 2, next2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.call.12:
	dmacall *, next2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.ret.13:
	dmaret 2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.ret.14:
	dmaret *
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.end.15:
	dmaend 2
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata

test.end.16:
	dmaend *
	vifnop
	vifnop
	vifnop
	vifnop
	.enddmadata
