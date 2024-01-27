/*
 * ebsa.c -- Support for Intel StrongArm EBSA285 Eval Board
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
#include <stdio.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include <bsp/ebsa.h>
#include "bsp_if.h"
#include "queue.h"
#include "gdb.h"

#if defined(HAVE_NET)
#error HAVE_NET Not Supported on EBSA
#endif

/*
 * Toggle LED for debugging purposes.
 */
void flash_led(int n, int which)
{
    int led;

    /*
     * Select the right LED
     */
    switch (which)
    {
    case 0: led = EBSA285_SOFT_IO_RED_LED;   break;
    case 1: led = EBSA285_SOFT_IO_GREEN_LED; break;
    case 2: led = EBSA285_SOFT_IO_AMBER_LED; break;
    default: return;
    }

    /*
     * Double n so we turn on and off for each count
     */
    n <<= 1;

    while (n--)
    {
        int i;

        /*
         * Toggle the LED
         */
        *EBSA285_SOFT_IO_REGISTER ^= led;

        i = 0xffff; while (--i);
    }
}

/*
 * Early initialization of comm channels. Must not rely
 * on interrupts, yet. Interrupt operation can be enabled
 * in _bsp_board_init().
 */
void
_bsp_init_board_comm(void)
{
    extern void _bsp_init_sa110_comm(void);
    _bsp_init_sa110_comm();
}


/*
 * Set any board specific debug traps.
 */
void
_bsp_install_board_debug_traps(void)
{
}


/*
 * Install any board specific interrupt controllers.
 */
void
_bsp_install_board_irq_controllers(void)
{
}

/*
 *  Board specific BSP initialization.
 */
void
_bsp_board_init(void)
{
    /*
     * Override default platform info.
     */
    _bsp_platform_info.board = "Intel StrongArm EBSA285";
}
