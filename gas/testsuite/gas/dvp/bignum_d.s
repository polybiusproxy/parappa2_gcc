; Test Tokyo-bignum errors
;
; { dg-do assemble { target dvp-*-* } }

foo:
	.octa	0x333_0_12345678	; { dg-error "A bignum with underscores must have exactly 4 words." }
	.octa	0x333_0_12345678_1_2	; { dg-error "A bignum with underscores must have exactly 4 words." } 
					; { dg-warning "Bignum truncated to 16 bytes" "Comment" { target *-*-* } 7 }
