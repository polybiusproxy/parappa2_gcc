/*
 * start.S -- ROM-based startup for ARM
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

        .file   "start.S"

        .text
FUNC_START(start)

/*
 * Setup some vectors in ROM until we can remap RAM to address 0
 */
#if defined(BOARD_SA1100_MULTIMEDIA) || defined(BOARD_BRUTUS)
SYM_NAME(RESET_VECTOR):           ldr     pc, SYM_NAME(ROM_RESET_ISR)
SYM_NAME(UNDEFINED_VECTOR):       ldr     pc, SYM_NAME(ROM_UNDEFINED_ISR)
SYM_NAME(SWI_VECTOR):             ldr     pc, SYM_NAME(ROM_SWI_ISR)
SYM_NAME(PREFETCH_ABORT_VECTOR):  ldr     pc, SYM_NAME(ROM_PREFETCH_ABORT_ISR)
SYM_NAME(DATA_ABORT_VECTOR):      ldr     pc, SYM_NAME(ROM_DATA_ABORT_ISR)
SYM_NAME(ADDR_ERROR_VECTOR):      ldr     pc, SYM_NAME(ROM_ADDR_ERROR_ISR)
SYM_NAME(IRQ_VECTOR):             ldr     pc, SYM_NAME(ROM_IRQ_ISR)
SYM_NAME(FIQ_VECTOR):             ldr     pc, SYM_NAME(ROM_FIQ_ISR)

SYM_NAME(ROM_RESET_ISR):          .word   SYM_NAME(real_start)
SYM_NAME(ROM_UNDEFINED_ISR):      .word   SYM_NAME(undefined_isr_asm)
SYM_NAME(ROM_SWI_ISR):            .word   SYM_NAME(swi_isr_asm)
SYM_NAME(ROM_PREFETCH_ABORT_ISR): .word   SYM_NAME(prefetch_abort_isr_asm)
SYM_NAME(ROM_DATA_ABORT_ISR):     .word   SYM_NAME(data_abort_isr_asm)
SYM_NAME(ROM_ADDR_ERROR_ISR):     .word   SYM_NAME(address_err_isr_asm)
SYM_NAME(ROM_IRQ_ISR):            .word   SYM_NAME(irq_isr_asm)
SYM_NAME(ROM_FIQ_ISR):            .word   SYM_NAME(fiq_isr_asm)

FUNC_START(real_start)
#endif /* defined(BOARD_SA1100_MULTIMEDIA) || defined(BOARD_BRUTUS) */

        /*
         * Careful.  There is no memory yet.
         */

        /*
         * Setup the initial exception stack
         */
	mov	r0,IMM(ARM_PSR_IRQ | ARM_PSR_FIQ | ARM_PSR_MODE_SVC)
	msr	cpsr, r0
        ldr     sp, SYM_NAME(_ex_stack_addr)

        /*
         * Setup the initial user stack (normal processing in system mode)
         * and continue processing in System mode
         */
	mov	r0,IMM(ARM_PSR_IRQ | ARM_PSR_FIQ | ARM_PSR_MODE_SYSTEM)
	msr	cpsr, r0
        ldr     sp, SYM_NAME(init_stack)

#if 0
        /*
         * Test if the lr works in system mode.
         * It appeared not to work on the AEB-1
         */
        mov     lr, IMM(0x4)
        mov     r0, lr
        mov     r1, IMM(0x4)
        cmps    r0, r1
0:      bne     0b
        PORT_TOGGLE_DEBUG
#endif

        /*
         * We manually put the return address into the lr in case
         * _bsp_init_stack modifies the memory layout and invalidates
         * the current map
         */
        ldr     lr, SYM_NAME(bsp_init_stack_return_address)
        b       SYM_NAME(_bsp_init_stack)
FUNC_END(start)

SYM_NAME(bsp_init_stack_return_address): .word SYM_NAME(c_start)
SYM_NAME(init_stack):                    .word SYM_NAME(_initial_stack)
SYM_NAME(_ex_stack_addr):                .word SYM_NAME(_ex_stack)
