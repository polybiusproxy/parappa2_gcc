/*
 * bsp-start.c -- C Startup code for the Cygnus BSP.
 *                This is called by the BSP boot code itself, so it sets
 *                up more than just the C runtime stuff.  It also initializes
 *                the bsp.
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

#include <bsp_start.h>

#if defined(SIZE_MEM)
unsigned long *maxmem;
#endif /* defined(SIZE_MEM) */

void c_start(void)
{
#ifdef SIZE_MEM
    /*
     * Find out how big the RAM is
     */
    register unsigned long *ptr=(unsigned long *)0xC0000000, *end = (unsigned long *)0xE0000000;
    register unsigned long data1=0xCAFEBABE, data2=0xDEADBEEF;

    /*
     * Fill all of suspected memory w/ 0xCAFEBABE and 0xDEADBEEF
     */
    while (ptr < end)
    {
        *(ptr+0) = data1;
        *(ptr+1) = data2;
        ptr = (unsigned long *)((unsigned long)ptr + SZ_4K);
    }

    /*
     * Now test all of suspected memory for expected pattern
     */
    ptr = (unsigned long *)0xC0000000;

    while (ptr < end)
    {
        /*
         * Check for 0xCAFEBABE
         */
        if (*(ptr+0) != data1) break;
        *(ptr+0) = 0;

        /*
         * Check for 0xDEADBEEF
         */
        if (*(ptr+1) != data2) break;
        *(ptr+1) = 0;

        ptr = (unsigned long *)((unsigned long)ptr + SZ_4K);
    }
#endif /* SIZE_MEM */

    /*
     * Do the C Runtime startup stuff first part
     * This routine is shared by both the app and the bsp
     */
    c_crt0_begin();

    /*
     * Do the bsp specific startup stuff.
     */
    c_crt0_bsp_specific();

#ifdef SIZE_MEM
    maxmem = ptr;
#endif /* SIZE_MEM */

    /*
     * Do the C Runtime startup stuff end part
     * This routine is shared by both the app and the bsp
     */
    c_crt0_end();
}
