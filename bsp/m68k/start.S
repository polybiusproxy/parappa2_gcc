/*
 * start.S -- ROM-based startup for M68K
 *
 * Copyright (c) 1998, 1999 Cygnus Support
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */
#include <bsp/cpu.h>
#include "gdb.h"

#define DEBUG_START_CODE 0

#if DEBUG_START_CODE
#include BOARD_HEADER
#endif /* DEBUG_START_CODE */

	.file	"start.S"

       	.text

	/* Reset Supervisor Stack Pointer */
        .long SYM_NAME(_superstack)

        /* Reset Initial Program Counter */
        .long SYM_NAME(_start)

#if DEBUG_START_CODE
        .align     2
SYM_NAME(zeroing): .string "Zeroing "
        .align     2
SYM_NAME(copying): .string "Copying "
        .align     2
SYM_NAME(bytes):   .string " bytes from "
        .align     2
SYM_NAME(to):      .string " to "
        .align     2
SYM_NAME(f_str):   .string "__FINI_SECTION__ = 0x"
        .align     2
SYM_NAME(i_str):   .string "__INIT_SECTION__ = 0x"
        .align     2
SYM_NAME(eoln):    .byte 0xd
                   .byte 0xa
                   .byte 0
#endif /* DEBUG_START_CODE */

FUNC_START(_start)
        /*
         * Disable all interrupts
         */
        movew   IMM(0x2700), sr

#ifdef CPU_MC68328
        /*
         * Dragonball Register base will be in a5
         */
        moveal  IMM(MC68328_REG_BASE), a5

        /*
         * Enable bus error timeouts
         */
        bsetb   IMM(4),a5@(MC68328_SCR_o)
        
        /*
         * Disable the watchdog timer.
         */
        movew   IMM(0), a5@(MC68328_WDOG_CTL_o)
#endif /* CPU_MC68328 */
        
	/*
	 *  Jump to _bsp_init_stack with return address in a0.
	 *  We call _bsp_init_stack this way because there is
	 *  no stack, yet.
	 */
	movel	IMM(1f),a0
        jmp	SYM_NAME(_bsp_init_stack)

1:
#if DEBUG_START_CODE
        movel   IMM(38400), sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     init_uart
        lea     sp@(8), sp
#endif /* DEBUG_START_CODE */

        /*
	 * Copy data from ROM to RAM, if neccessary.
	 */
	movel	IMM(SYM_NAME(_ram_data_start)),d3
	movel	IMM(SYM_NAME(_ram_data_end)),d2
        movel   IMM(SYM_NAME(_rom_data_start)),d1
	cmp	d3, d2
	jeq	8f                                 /* If equal, no data to copy                */
	cmp	d3, d1
	jeq	8f                                 /* If src and dest are equal, no copy req.  */

        movel   d1, a3                             /* Copy from (a3)                           */
        movel   d3, a4                             /* Copy to (a4)                             */
        subl    d3, d2                             /* Amount to copy                           */

#if DEBUG_START_CODE
        pea     copying
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putstring
        lea     sp@(8), sp
         
        movel   d2, sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     bytes
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

        movel   a3, sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     to
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

        movel   a4, sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     eoln
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp
#endif /* DEBUG_START_CODE */

        asrl    IMM(2), d2                         /* Adjust to number of longwords to copy    */
        subq    IMM(1), d2                         /* Adjust for -1 stopping condition of dbra */

0:      movel   a3@+, a4@+                         /* Copy a long                              */
        dbra    d2, 0b

8:	
	/*
	 *  Clear BSS, if any.
	 */
	movel	IMM(SYM_NAME(_bss_start)),d3
	movel	IMM(SYM_NAME(_bss_end)),d2
	cmp	d2,d3
	jeq	8f

        movel   d3, a4                        /* Zero to (a4)                             */
        subl    d3, d2                        /* Amount to zero                           */
        
#if DEBUG_START_CODE
        pea     zeroing
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp
         
        movel   d2, sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     bytes
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

        movel   a4, sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     eoln
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp
#endif /* DEBUG_START_CODE */

        asrl    IMM(2), d2                    /* Adjust to number of longwords to zero    */
        subq    IMM(1), d2                    /* Adjust for -1 stopping condition of dbra */

0:      clrl    (a4)+
        dbra    d2, 0b

8:	
        /*
         * Finish setup of RAM description. Put a
         * physical pointer to the top of RAM in _bsp_ram_info_ptr.
         */
        lea     SYM_NAME(_bsp_ram_info_ptr), a0
        movel   SYM_NAME(_superstack), a0@

        /*
         * initialize target specific stuff. Only execute these
         * functions it they exist.
         */
	lea	SYM_NAME(_hardware_init_hook), a0
	cmpl	IMM(0),a0
	jbeq	4f
	jsr     (a0)
4:

	lea	SYM_NAME(_software_init_hook), a0
	cmpl	IMM(0),a0
	jbeq	5f
	jsr     (a0)
5:
	bsr	SYM_NAME(_bsp_init)

#ifdef ADD_DTORS
	/* put __do_global_dtors in the atexit list so the destructors get run */
	movel	IMM (SYM_NAME(__do_global_dtors)),sp@-
	jsr	SYM_NAME (atexit)
        lea     sp@(4), sp
#endif /* ADD_DTORS */
#if DEBUG_START_CODE
        pea     f_str
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

	movel	IMM(__FINI_SECTION__), sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     eoln
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

        pea     i_str
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp

	movel	IMM(__INIT_SECTION__), sp@-
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-       
        jsr     uart_putlong
        lea     sp@(8), sp

        pea     eoln
        movel   IMM(MC68681_REG_BASE + MC68681_MR1A_o), sp@-
        jsr     uart_putstring
        lea     sp@(8), sp
#endif /* DEBUG_START_CODE */

        movel	IMM (__FINI_SECTION__),sp@-
	jsr	SYM_NAME (atexit)
        lea     sp@(4), sp

        jsr	SYM_NAME(__INIT_SECTION__)

        jsr     SYM_NAME(_bsp_initvectors)

        /*
         * Enable interrupts
         */
        andiw   IMM(~0x0700),sr

	/*
	 *  Call main.
	 */
        pea     SYM_NAME(nullstring)
        pea     SYM_NAME(nullstring)
        pea     0

        bsr     SYM_NAME(main)
        lea     sp@(12), sp    /* Pop the parameters */

	/*
	 *  Call exit if main() returns.
	 */
        movel   d0, sp@-       /* With the return code from main */
	bsr	SYM_NAME(exit)

	/*
	 * Never reached, but just in case...
	 */
  1:
	bra	1b
FUNC_END(_start)

/*
 * Define an empty environment.
 */
SYM_NAME(nullstring):                     .long 0
