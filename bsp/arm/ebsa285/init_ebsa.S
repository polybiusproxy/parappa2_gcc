/*
 * init_ebsa.S -- Initialization for Intel StrongArm EBSA285 Eval Board
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
	.file "init_ebsa.S"
        .title "Init Code for Intel StrongArm EBSA285 Eval Board"

#include <bsp/cpu.h>
#include <bsp/ebsa.h>
        
#define DEBUG_INIT_STACK 0
	
	.text

        .align     4

FUNC_START(_bsp_init_stack)
        /*
         * Force an all-banks recharge on all four SDRAM arrays
         *
         * This code came from the EBIX-DB ver 1.0 (3-16-98) spec pg 22
         *
         * You must access all four arrays regardless of whether there is
         * memory there because the 21285 counds the precharge accesses and
         * inhibits access to the SDRAM until all four have been done.
         *
         * An all banks rechargs is initiated by a read from any address
         * in the mode register space.
         */
        ldr     r0, SYM_NAME(ARRAY_0_MODE_REGISTER)
        ldr     r0, [r0]
        ldr     r0, SYM_NAME(ARRAY_1_MODE_REGISTER)
        ldr     r0, [r0]
        ldr     r0, SYM_NAME(ARRAY_2_MODE_REGISTER)
        ldr     r0, [r0]
        ldr     r0, SYM_NAME(ARRAY_3_MODE_REGISTER)
        ldr     r0, [r0]

        /*
         * Now we need to write to the SDRAM Mode Register.
         * The address is important, not the data.  The mode register
         * should be configured for a burst size of 4 with linear addressing
         */
        ldr     r0, SYM_NAME(ARRAY_0_MODE_REGISTER)
        str     r0, [r0, IMM(SA110_SDRAM_CAS_LATENCY_2_CYCLES)]
        ldr     r0, SYM_NAME(ARRAY_1_MODE_REGISTER)
        str     r0, [r0, IMM(SA110_SDRAM_CAS_LATENCY_2_CYCLES)]
        ldr     r0, SYM_NAME(ARRAY_2_MODE_REGISTER)
        str     r0, [r0, IMM(SA110_SDRAM_CAS_LATENCY_2_CYCLES)]
        ldr     r0, SYM_NAME(ARRAY_3_MODE_REGISTER)
        str     r0, [r0, IMM(SA110_SDRAM_CAS_LATENCY_2_CYCLES)]

        /*
         * Write to the SDRAM Timing Register in the 21285.  Set the
         * refresh interval to the minimum because we have to wait for
         * 8 refresh cycles to complete before we can rely on the SDRAMs
         * to be operating normally
         */
        ldr     r1, SYM_NAME(SDRAM_TIMING)
        ldr     r0, SYM_NAME(SDRAM_TIMING_VALUE_MIN)
        str     r0, [r1]

        /*
         * Wait for 8 refresh cycles to complete
         */
        mov     r0, IMM(0x100)
0:      subs    r0, r0, IMM(1)
        bgt     0b

        /*
         * Setup the SDRAM array and size registers
         */
        ldr     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_0_VALUE)
        str     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_0)
        ldr     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_1_VALUE)
        str     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_1)
        ldr     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_2_VALUE)
        str     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_2)
        ldr     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_3_VALUE)
        str     r0, SYM_NAME(ADDRESS_SIZE_ARRAY_3)

        /*
         * Now reset the Refresh interval to a sensible value
         * SDRAM_TIMING address should be in R1
         */
        ldr     r0, SYM_NAME(SDRAM_TIMING_VALUE_NORMAL)
        str     r0, [r1]

        mov     pc, lr
FUNC_END(_bsp_init_stack)

SYM_NAME(ARRAY_0_MODE_REGISTER):      .word (SA110_SDRAM_ARRAY_0_MODE_REGISTER_BASE + 8)
SYM_NAME(ARRAY_1_MODE_REGISTER):      .word (SA110_SDRAM_ARRAY_1_MODE_REGISTER_BASE + 8)
SYM_NAME(ARRAY_2_MODE_REGISTER):      .word (SA110_SDRAM_ARRAY_2_MODE_REGISTER_BASE + 8)
SYM_NAME(ARRAY_3_MODE_REGISTER):      .word (SA110_SDRAM_ARRAY_3_MODE_REGISTER_BASE + 8)
SYM_NAME(SDRAM_TIMING):               .word (SA110_SDRAM_TIMING)
SYM_NAME(SDRAM_TIMING_VALUE_MIN):     .word (SA110_SDRAM_ROW_PRECHARGE_2_CYCLES    | \
                                             SA110_SDRAM_LAST_DATA_IN_3_CYCLES     | \
                                             SA110_SDRAM_RAS_TO_CAS_DELAY_2_CYCLES | \
                                             SA110_SDRAM_CAS_LATENCY_2_CYCLES      | \
                                             SA110_SDRAM_ROW_CYCLE_TIME_6_CYCLES   | \
                                             SA110_SDRAM_COMMAND_DRIVE_1_CYCLE     | \
                                             SA110_SDRAM_REFRESH_INTERVAL_MIN)
SYM_NAME(SDRAM_TIMING_VALUE_NORMAL):  .word (SA110_SDRAM_ROW_PRECHARGE_2_CYCLES    | \
                                             SA110_SDRAM_LAST_DATA_IN_3_CYCLES     | \
                                             SA110_SDRAM_RAS_TO_CAS_DELAY_2_CYCLES | \
                                             SA110_SDRAM_CAS_LATENCY_2_CYCLES      | \
                                             SA110_SDRAM_ROW_CYCLE_TIME_6_CYCLES   | \
                                             SA110_SDRAM_COMMAND_DRIVE_1_CYCLE     | \
                                             SA110_SDRAM_REFRESH_INTERVAL_NORMAL)
SYM_NAME(ADDRESS_SIZE_ARRAY_0):       .word SA110_SDRAM_ADDRESS_SIZE_ARRAY_0
SYM_NAME(ADDRESS_SIZE_ARRAY_1):       .word SA110_SDRAM_ADDRESS_SIZE_ARRAY_1
SYM_NAME(ADDRESS_SIZE_ARRAY_2):       .word SA110_SDRAM_ADDRESS_SIZE_ARRAY_2
SYM_NAME(ADDRESS_SIZE_ARRAY_3):       .word SA110_SDRAM_ADDRESS_SIZE_ARRAY_3
SYM_NAME(ADDRESS_SIZE_ARRAY_0_VALUE): .word (0x00000000 | 0x10 | SA110_SDRAM_SIZE_8MB)
SYM_NAME(ADDRESS_SIZE_ARRAY_1_VALUE): .word (0x00800000 | 0x10 | SA110_SDRAM_SIZE_8MB)
SYM_NAME(ADDRESS_SIZE_ARRAY_2_VALUE): .word (0x01000000 | 0x10 | SA110_SDRAM_SIZE_8MB)
SYM_NAME(ADDRESS_SIZE_ARRAY_3_VALUE): .word (0x01C00000 | 0x10 | SA110_SDRAM_SIZE_8MB)
