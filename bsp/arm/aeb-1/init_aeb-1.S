/*
 * init_aeb-1.S -- Initialization for ARM AEB-1 Eval board.
 *
 * Copyright (c) 1998 Cygnus Support
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
	.file "init_aeb-1.S"
        .title "Init Code for ARM AEB-1 Eval Board"

#include <bsp/cpu.h>
#include <bsp/aeb-1.h>
        
#define DEBUG_INIT_STACK 0
	
	.text

/*
 * Chip Select Registers for Bank 0 (ROM/FLASH)
 */
SYM_NAME(ROM_START):       .word LH77790A_MEM_SEG_START0
SYM_NAME(ROM_STOP):        .word LH77790A_MEM_SEG_STOP0
SYM_NAME(ROM_SDR):         .word LH77790A_MEM_SEG_SDR0
SYM_NAME(ROM_BCR):         .word LH77790A_BANK_CONFIG_BCR0

SYM_NAME(ROM_START_VALUE): .word LH77790A_MEM_SEG_START_ADDRESS(256*1024)
SYM_NAME(ROM_STOP_VALUE):  .word LH77790A_MEM_SEG_STOP_ADDRESS(512*1024)
SYM_NAME(ROM_SDR_VALUE):   .word LH77790A_MEM_SEG_SYS_PRIVILEGE_RO  | \
                                 LH77790A_MEM_SEG_USER_PRIVILEGE_RO | \
                                 LH77790A_MEM_SEG_NONCACHEABLE      | \
                                 LH77790A_MEM_SEG_32BIT_MODE        | \
                                 LH77790A_MEM_SEG_SELECT_BANK_0
SYM_NAME(ROM_BCR_VALUE):   .word LH77790A_BANK_SRAM_BUS_SIZE_8_BITS | \
                                 LH77790A_BANK_SRAM_WAIT_STATES_7   | \
                                 LH77790A_BANK_SRAM_ECE_CE0_HIGH    | \
                                 LH77790A_BANK_SRAM_ECE_CE0_LOW

/*
 * Chip Select Registers for Bank 1 (RAM)
 */
SYM_NAME(RAM_START):       .word LH77790A_MEM_SEG_START2
SYM_NAME(RAM_STOP):        .word LH77790A_MEM_SEG_STOP2
SYM_NAME(RAM_SDR):         .word LH77790A_MEM_SEG_SDR2
SYM_NAME(RAM_BCR):         .word LH77790A_BANK_CONFIG_BCR2
                              
SYM_NAME(RAM_START_VALUE): .word LH77790A_MEM_SEG_START_ADDRESS(0)
SYM_NAME(RAM_STOP_VALUE):  .word LH77790A_MEM_SEG_STOP_ADDRESS(128*1024)
SYM_NAME(RAM_SDR_VALUE):   .word LH77790A_MEM_SEG_SYS_PRIVILEGE_RW  | \
                                 LH77790A_MEM_SEG_USER_PRIVILEGE_RW | \
                                 LH77790A_MEM_SEG_NONCACHEABLE      | \
                                 LH77790A_MEM_SEG_32BIT_MODE        | \
                                 LH77790A_MEM_SEG_SELECT_BANK_2
SYM_NAME(RAM_BCR_VALUE):   .word LH77790A_BANK_SRAM_BUS_SIZE_16_BITS | \
                                 LH77790A_BANK_SRAM_WAIT_STATES_1    | \
                                 LH77790A_BANK_SRAM_ECE_CE1_HIGH     | \
                                 LH77790A_BANK_SRAM_ECE_CE1_LOW
                              
SYM_NAME(ROM_ADDR):     .word ROM_ADDR_LABEL

        .align     4
FUNC_START(_bsp_init_stack)
        /*
         * Setup the ROM/FLASH
         */
        ldr     r0, SYM_NAME(ROM_START)
        ldr     r1, SYM_NAME(ROM_START_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(ROM_STOP)
        ldr     r1, SYM_NAME(ROM_STOP_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(ROM_SDR)
        ldr     r1, SYM_NAME(ROM_SDR_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(ROM_BCR)
        ldr     r1, SYM_NAME(ROM_BCR_VALUE)
        str     r1, [r0]

        /*
         * Now we have ROM.
         *
         * Load the PC directly with the address of the next instruction
         * so that we start executing from ROM.
         */
        ldr     pc, SYM_NAME(ROM_ADDR)

ROM_ADDR_LABEL:
        /*
         * Setup the RAM
         */
        ldr     r0, SYM_NAME(RAM_START)
        ldr     r1, SYM_NAME(RAM_START_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(RAM_STOP)
        ldr     r1, SYM_NAME(RAM_STOP_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(RAM_SDR)
        ldr     r1, SYM_NAME(RAM_SDR_VALUE)
        str     r1, [r0]

        ldr     r0, SYM_NAME(RAM_BCR)
        ldr     r1, SYM_NAME(RAM_BCR_VALUE)
        str     r1, [r0]

        /*
         * Now we have RAM.
         */
        mov     pc, lr
FUNC_END(_bsp_init_stack)
