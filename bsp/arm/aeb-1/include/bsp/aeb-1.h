/*
 * aeb-1.h -- Header for Arm AEB-1 Eval Board
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
#ifndef __AEB_1_H__
#define __AEB_1_H__ 1

#include <bsp/lh77790a.h>

#define AEB_1_LED_D1                       LH77790A_PORT_C_BIT_7
#define AEB_1_LED_D2                       LH77790A_PORT_C_BIT_6
#define AEB_1_LED_D3                       LH77790A_PORT_C_BIT_5
#define AEB_1_LED_D4                       LH77790A_PORT_C_BIT_4

#ifdef __ASSEMBLER__
/*
 * Use the following code for debugging by toggling a port pin
 */
.macro PORT_TOGGLE_DEBUG
        /*
         * Make sure the Port C is in Output mode
         */
        ldr     r0, 7f
        mov     r1, IMM(LH77790A_PORT_CTL_MODE_SELECTION     | \
                        LH77790A_PORT_C_4_7_DIRECTION_OUTPUT | \
                        LH77790A_PORT_C_0_3_DIRECTION_OUTPUT)
        strb    r1, [r0]

        /*
         * Write to the PORT C register
         */
        ldr     r0, 9f

        /*
         * Turn on the LED's
         */
0:      mov     r1, IMM(0xF0)
        strb    r1, [r0]

        /*
         * Delay a while
         */
        ldr     r2, 8f
1:      subs    r2, r2, IMM(1)
        bgt     1b

        /*
         * Turn off the LED's
         */
        mov     r1, IMM(0x00)
        strb    r1, [r0]

        /*
         * Delay a while
         */
        ldr     r2, 8f
1:      subs    r2, r2, IMM(1)
        bgt     1b

        b       0b

7:      .word LH77790A_PORT_CONTROL_REGISTER    /* PCR                */
8:      .word 0xFFFF                            /* LED_DELAY_CONSTANT */
9:      .word LH77790A_PORT_C                   /* PC                 */
.endm
#else

#define PORT_TOGGLE_DEBUG()  asm volatile ("                            \n\
        /*                                                              \n\
         * Make sure the Port C is in Output mode                       \n\
         */                                                             \n\
        ldr     %r0, 7f                                                 \n\
        mov     %r1, #0x80                                              \n\
        strb    %r1, [%r0]                                              \n\
                                                                        \n\
        /*                                                              \n\
         * Write to the PORT C register                                 \n\
         */                                                             \n\
        ldr     %r0, 9f                                                 \n\
                                                                        \n\
        /*                                                              \n\
         * Turn on the LED's                                            \n\
         */                                                             \n\
0:      mov     %r1, #0xF0                                              \n\
        strb    %r1, [%r0]                                              \n\
                                                                        \n\
        /*                                                              \n\
         * Delay a while                                                \n\
         */                                                             \n\
        ldr     %r2, 8f                                                 \n\
1:      subs    %r2, %r2, #1                                            \n\
        bgt     1b                                                      \n\
                                                                        \n\
        /*                                                              \n\
         * Turn off the LED's                                           \n\
         */                                                             \n\
        mov     %r1, #0x00                                              \n\
        strb    %r1, [%r0]                                              \n\
                                                                        \n\
        /*                                                              \n\
         * Delay a while                                                \n\
         */                                                             \n\
        ldr     %r2, 8f                                                 \n\
1:      subs    %r2, %r2, #1                                            \n\
        bgt     1b                                                      \n\
                                                                        \n\
        b       0b                                                      \n\
                                                                        \n\
7:      .word 0xFFFF1C0C                     /* PCR                */   \n\
8:      .word 0xFFFF                         /* LED_DELAY_CONSTANT */   \n\
9:      .word 0xFFFF1C08                     /* PC                 */   \n\
");
#endif /* __ASSEMBLER__ */

#define UART_BASE_0 LH77790A_UART_BASE_1
#define UART_BASE_1 LH77790A_UART_BASE_0

#endif /* __AEB_1_H__ */
