/*
 * ebsa.h -- Header for Intel StrongArm EBSA285 Eval Board
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
#ifndef __EBSA_H__
#define __EBSA_H__ 1

#include <bsp/sa-110.h>

/*
 * This define is a static initializer construct for
 * the bsp_mem_info structure
 */
#define BOARD_MEMORY_LIST                                                    \
{                                                                            \
}

/*
 * EBSA Soft I/O Register
 */
#define EBSA285_SOFT_IO_REGISTER           REG32_PTR(0x40012000)

/*
 * EBSA Soft I/O Register Bit Field definitions
 */
#define EBSA285_SOFT_IO_TOGGLE             0x80
#define EBSA285_SOFT_IO_RED_LED            0x04
#define EBSA285_SOFT_IO_GREEN_LED          0x02
#define EBSA285_SOFT_IO_AMBER_LED          0x01
#define EBSA285_SOFT_IO_J9_9_10_MASK       0x40
#define EBSA285_SOFT_IO_J9_11_12_MASK      0x20
#define EBSA285_SOFT_IO_J9_13_14_MASK      0x10
#define EBSA285_SOFT_IO_SWITCH_L_MASK      0x0F

#ifdef __ASSEMBLER__
/*
 * Use the following code for debugging by toggling a port pin
 */
.macro PORT_TOGGLE_DEBUG
        /*
         * Leave the following values in registers:
         *
         *    r0 = Soft IO Register
         *    r1 = LED_ON value
         *    r2 = LED_OFF value
         *    r3 = LED_DELAY value
         */
        ldr     r0, 7f
        ldr     r1, 8f
        mvn     r2, r1
        ldr     r3, 9f

        /*
         * Turn on the LED
         */
0:      str     r1, [r0]

        /*
         * Delay a while
         */
        mov     r4, r3
1:      subs    r4, r4, IMM(1)
        bne     1b

        /*
         * Turn off the LED
         */
        str     r2, [r0]

        /*
         * Delay a while
         */
        mov     r4, r3
1:      subs    r4, r4, IMM(1)
        bne     1b

        /*
         * Repeat
         */
        b       0b

7:      .word   EBSA285_SOFT_IO_REGISTER
8:      .word   EBSA285_SOFT_IO_RED_LED | EBSA285_SOFT_IO_GREEN_LED | EBSA285_SOFT_IO_AMBER_LED
9:      .word   0xFFFF
.endm
#else
#define PORT_TOGGLE_DEBUG()    asm volatile ("                          \n\
        /*                                                              \n\
         * Leave the following values in registers:                     \n\
         *                                                              \n\
         *    r0 = Soft IO Register                                     \n\
         *    r1 = LED_ON value                                         \n\
         *    r2 = LED_OFF value                                        \n\
         *    r3 = LED_DELAY value                                      \n\
         */                                                             \n\
        ldr     r0, 7f                                                  \n\
        ldr     r1, 8f                                                  \n\
        mvn     r2, r1                                                  \n\
        ldr     r3, 9f                                                  \n\
                                                                        \n\
        /*                                                              \n\
         * Turn on the LED                                              \n\
         */                                                             \n\
0:      str     r1, [r0]                                                \n\
                                                                        \n\
        /*                                                              \n\
         * Delay a while                                                \n\
         */                                                             \n\
        mov     r4, r3                                                  \n\
1:      subs    r4, r4, #1                                              \n\
        bne     1b                                                      \n\
                                                                        \n\
        /*                                                              \n\
         * Turn off the LED                                             \n\
         */                                                             \n\
        str     r2, [r0]                                                \n\
                                                                        \n\
        /*                                                              \n\
         * Delay a while                                                \n\
         */                                                             \n\
        mov     r4, r3                                                  \n\
1:      subs    r4, r4, #1                                              \n\
        bne     1b                                                      \n\
                                                                        \n\
        /*                                                              \n\
         * Repeat                                                       \n\
         */                                                             \n\
        b       0b                                                      \n\
                                                                        \n\
7:      .word   0x40012000                                              \n\
8:      .word   0x07                                                    \n\
9:      .word   0xFFFF");
#endif /* __ASSEMBLER__ */

#endif /* __EBSA_H__ */
