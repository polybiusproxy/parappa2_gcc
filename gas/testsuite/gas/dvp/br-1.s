; Branch tests

	.vu
foo:
	nop	b foo1
	nop	nop
foo1:
	nop	bal vi01, foo
	nop	nop

; branch to undefined

	nop	b bar1
	nop	nop
