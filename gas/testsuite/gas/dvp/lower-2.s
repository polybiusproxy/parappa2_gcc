	.vu

	.text
; Test the R_MIPS_DVP_U15_S3 relocation.
; Ensure iaddiu_label is at non-zero offset in .vutext (helps catch bugs).
	nop	nop
; iaddiu with a label arg
iaddiu_label:
	nop	iaddiu  vi01,vi00,iaddiu_label

; Test the R_MIPS_DVP_11_S4 relocation.
	.data
; Ensure somevar is at a non-zero offset in .vudata (helps catch bugs).
	.int 0
	.p2align 4
somevar:
	.quad 0
	.text
ilw_label:
	nop	ilw.x vi01,somevar-.vudata(vi00)
	nop	isw.y vi01,somevar-.vudata(vi00)
