/* Copy of macros/semi.s except that our .text is actually .vutext.  */

        .macro semicolon
        .ascii "; "
        .endm

        .macro colon
        .ascii ": "
        .endm

        semicolon
        .ascii "; "
        colon
        .ascii ": "

	.p2align 5,0
