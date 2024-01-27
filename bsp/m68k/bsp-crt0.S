/*
 * bsp-crt0.S -- startup file for M68K.
 *
 * Copyright (c) 1995, 1996, 1997, 1998 Cygnus Support
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
#include "bsp_if.h"

/*
 * Define an empty environment.
 */
        .data
        .align 2
SYM_NAME(environ):
        .long 0
        .long 0
        .long 0

	.text
	.align 4

FUNC_START(_start)

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
        asrl    IMM(2), d2                    /* Adjust to number of longwords to zero    */
        subq    IMM(1), d2                    /* Adjust for -1 stopping condition of dbra */

0:      clrl    (a4)+
        dbra    d2, 0b

8:	

	/*
	 * get linkage to shared data
	 */
	movel   IMM(bsp_shared_data), sp@-
	movel	IMM(BSP_GET_SHARED), sp@-
	jsr     _bsp_trap
        lea     sp@(8), sp

#ifdef ADD_DTORS
	/* put __do_global_dtors in the atexit list so the destructors get run */
	movel	IMM (SYM_NAME(__do_global_dtors)),sp@-
	jsr	SYM_NAME (atexit)
        lea     sp@(4), sp
#endif
	movel	IMM (__FINI_SECTION__),sp@-
	jsr	SYM_NAME (atexit)
        lea     sp@(4), sp

	jsr	__INIT_SECTION__


	/*
	 *  Call main.
	 */
        pea     0
        pea     SYM_NAME(environ)
        pea     sp@(4)
        pea     0
        bsr     SYM_NAME(main)
        lea     sp@(16), sp    /* Pop the parameters */

      	/* return value from main is argument to exit */
1:      movel   d0, sp@-

#ifdef GCRT0
	jsr	_mcleanup
#endif

        /*
         * Call exit w/ the return value from main as the first parameter
         */
	bsr	SYM_NAME(exit)

FUNC_END(_start)
