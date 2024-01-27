/*
 * init_armpid.S -- Initialization for ARM Pid Eval board.
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
	.file "init_pid.S"
        .title "Init Code for ARM Pid Eval Board"

#include <bsp/cpu.h>
#include <bsp/armpid.h>
        
#define DEBUG_INIT_STACK 0
	
	.text

        .align     4
SYM_NAME(CLEAR_RESET_MAP):      .word ARMPID_REMAP_CLEAR_RST_MAP
SYM_NAME(ROM_ADDR):             .word ROM_ADDR_LABEL
FUNC_START(_bsp_init_stack)
        /*
         *
         * Load the PC directly with the address of the next instruction
         * so that we start executing from ROM.
         */
        ldr     pc, SYM_NAME(ROM_ADDR)

ROM_ADDR_LABEL:
        /*
         * Write to the CLEAR_RESET_MAP register and redo the memory
         * map.  RAM will now be located at 0.
         */
        ldr     r11, SYM_NAME(CLEAR_RESET_MAP)
        mov     r10, IMM(0)
        strb    r10, [r11]

        mov     pc, lr
FUNC_END(_bsp_init_stack)
