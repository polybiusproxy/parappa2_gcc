/*
 * vectors.S -- Exception vector code for ARM BSP
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

	.file	"vectors.S"

        .text
/*
 * Save all relevant registers into
 * an ex_regs_t struct on the stack
 */
.macro SAVE_REGS
        sub     sp, sp, IMM(ARM_EX_REGS_T_SIZE)  /* Move sp to bottom of reg image     */
        stmia   sp, {r0-r12,sp,lr,pc}            /* Save all the current registers     */

        mov     r0, sp                           /* SP at time of exception            */
        add     r0, r0, IMM(ARM_EX_REGS_T_SIZE)  /*    in r0                           */
        str     r0, [sp, IMM(sp_o)]              /* Save SP in the register image      */

        mrs     r0, spsr                         /* SPSR in r0                         */
        str     r0, [sp, IMM(spsr_o)]            /* Save spsr in the register image    */

        mrs     r0, cpsr                         /* CPSR in r0                         */
        str     r0, [sp, IMM(cpsr_o)]            /* Save cpsr in the register image    */
.endm


/*
 * Restore all relevant registers from
 * an ex_regs_t struct on the stack
 */
.macro RESTORE_REGS_AND_RTE
        UPDATE_REGS_FOR_EXCEPTION_RETURN

        ldr     lr, [sp, IMM(lr_o)]              /* Restore lr from register image     */

        ldr     r1, [sp, IMM(spsr_o)]            /* Restore spsr from register image   */
        msr     spsr, r1

        ldmia   sp, {r0-r12,sp}                  /* Restore r0 - r12 and sp,           */
                                                 /*   popping the rest of the parms    */

        movs    pc, lr                           /* return & move spsr_svc into cpsr   */
.endm

/*
 * Change into SVC mode safely (ie don't clobber any regs)
 * And build up a register image on the stack containing
 * the register set that was valid on exception entry
 */
.macro CHANGE_MODE_AND_BUILD_REG_IMAGE cur_mode pc_decrement
   /*
    *  If cur_mode is not SVC, then do a safe change of mode
    */ 
   .if \cur_mode <> ARM_PSR_MODE_SVC
        stmfd   sp!, {r3-r7}                     /* Save regs on (cur_mode) stack      */

        mov     r3, lr                           /* Saved LR_cur_mode in r3            */
        mov     r4, sp                           /* Saved SP_cur_mode in r4            */
        mrs     r5, spsr                         /* Saved SPSR_cur_mode in r5          */

        mrs     r6, cpsr                         /* Switch                             */
        and     r7, r6, IMM(~ARM_PSR_MODE_MASK)  /* to                                 */
        orr     r7, r7, IMM(ARM_PSR_MODE_SVC)    /* SVC                                */
        msr     cpsr, r7                         /* mode                               */
   .endif

   /*
    * Create the register image
    */
   SAVE_REGS

   /*
    * If cur_mode is not SVC, update the regs image
    * with the regs saved before the mode change
    */
   .if \cur_mode <> ARM_PSR_MODE_SVC
        str     r3, [sp, IMM(lr_o)]              /* Put the saved cur_mode registers   */
        str     r5, [sp, IMM(spsr_o)]            /*    into the register image         */
        str     r6, [sp, IMM(cpsr_o)]

        ldmfd   r4, {r3-r7}                      /* Restore working registers from     */
                                                 /*    (cur_mode) stack                */

        str     r3, [sp, IMM(r3_o)]              /* Put the restored working registers */
        str     r4, [sp, IMM(r4_o)]              /*    into the register image         */
        str     r5, [sp, IMM(r5_o)]
        str     r6, [sp, IMM(r6_o)]
        str     r7, [sp, IMM(r7_o)]
   .endif

   /*
    * Modify the regs image to the preexception values.
    * Also, decrement the appropriate amount from the
    * pc based on the exception.
    */
   UPDATE_REGS_TO_PREEXCEPTION_VALUE \pc_decrement

.endm


/*
 * Update the regs struct to reflect the pre-exception
 * condition of the registers
 *
 * Copy lr to pc in the register image
 *    and subract the pc_decrement value to
 *    point the pc at the next "to-be-executed"
 *    instruction
 * Copy spsr to cpsr in the register image
 */
.macro UPDATE_REGS_TO_PREEXCEPTION_VALUE pc_decrement
        ldr     r1, [sp, IMM(lr_o)]
.if \pc_decrement <> 0
        sub     r1, r1, IMM(\pc_decrement)
.endif
        str     r1, [sp, IMM(pc_o)]
        ldr     r1, [sp, IMM(spsr_o)]
        str     r1, [sp, IMM(cpsr_o)]
.endm


/*
 * Update the regs struct to reflect the exception return
 * condition of the registers
 *
 * ie: pc gets copied to lr
 *     cpsr gets copied to spsr
 *
 * On exception return these values will get put into the
 * right places.
 */
.macro UPDATE_REGS_FOR_EXCEPTION_RETURN
        ldr     r1, [sp, IMM(pc_o)]
        str     r1, [sp, IMM(lr_o)]
        ldr     r1, [sp, IMM(cpsr_o)]
        str     r1, [sp, IMM(spsr_o)]
.endm


/*
 * This is the Undefined instruction interrupt handler
 */
FUNC_START(undefined_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_undefined_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_UNDEF 0

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_UNDEFINED_INSTRUCTION)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(undefined_isr_asm)


/*
 * This is the SWI interrupt handler
 */
FUNC_START(swi_isr_asm)
        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_SVC 0

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_SOFTWARE_INTERRUPT)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(swi_isr_asm)


/*
 * This is the Prefetch Abort interrupt handler
 */
FUNC_START(prefetch_abort_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_abort_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_ABORT 4

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_PREFETCH_ABORT)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(prefetch_abort_isr_asm)


/*
 * This is the Data Abort interrupt handler
 */
FUNC_START(data_abort_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_abort_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_ABORT 8

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_DATA_ABORT)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(data_abort_isr_asm)


/*
 * This is the 26-bit mode Address Error interrupt handler
 */
FUNC_START(address_err_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_address_err_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_UNDEF 0

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_ADDRESS_ERROR_26_BIT)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(address_err_isr_asm)

/*
 * This is the IRQ interrupt handler
 */
FUNC_START(irq_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_irq_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_IRQ 4

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_IRQ)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(irq_isr_asm)


/*
 * This is the FIQ interrupt handler
 */
FUNC_START(fiq_isr_asm)
        /*
         * Set up a temp stack to use
         */
        ldr     sp, SYM_NAME(_fiq_stack_addr)

        CHANGE_MODE_AND_BUILD_REG_IMAGE ARM_PSR_MODE_FIQ 4

        /*
         * void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
         */
        mov     r0, sp
        mov     r1, IMM(BSP_CORE_EXC_FIQ)
        bl      SYM_NAME(generic_exception_handler)

        RESTORE_REGS_AND_RTE
FUNC_END(fiq_isr_asm)


/*
 * Assembly representing the exception vectors.
 */
        .section .vectors

SYM_NAME(RESET_VECTOR):           b       SYM_NAME(RESET_VECTOR)
SYM_NAME(UNDEFINED_VECTOR):       ldr     pc, SYM_NAME(ROM_UNDEFINED_ISR)
SYM_NAME(SWI_VECTOR):             ldr     pc, SYM_NAME(ROM_SWI_ISR)
SYM_NAME(PREFETCH_ABORT_VECTOR):  ldr     pc, SYM_NAME(ROM_PREFETCH_ABORT_ISR)
SYM_NAME(DATA_ABORT_VECTOR):      ldr     pc, SYM_NAME(ROM_DATA_ABORT_ISR)
SYM_NAME(ADDR_ERROR_VECTOR):      ldr     pc, SYM_NAME(ROM_ADDR_ERROR_ISR)
SYM_NAME(IRQ_VECTOR):             ldr     pc, SYM_NAME(ROM_IRQ_ISR)
SYM_NAME(FIQ_VECTOR):             ldr     pc, SYM_NAME(ROM_FIQ_ISR)

SYM_NAME(ROM_UNDEFINED_ISR):      .word   SYM_NAME(undefined_isr_asm)
SYM_NAME(ROM_SWI_ISR):            .word   SYM_NAME(swi_isr_asm)
SYM_NAME(ROM_PREFETCH_ABORT_ISR): .word   SYM_NAME(prefetch_abort_isr_asm)
SYM_NAME(ROM_DATA_ABORT_ISR):     .word   SYM_NAME(data_abort_isr_asm)
SYM_NAME(ROM_ADDR_ERROR_ISR):     .word   SYM_NAME(address_err_isr_asm)
SYM_NAME(ROM_IRQ_ISR):            .word   SYM_NAME(irq_isr_asm)
SYM_NAME(ROM_FIQ_ISR):            .word   SYM_NAME(fiq_isr_asm)


#ifdef MMU
/*
 * This the the MMU page tables
 */
	.section .page_table
        .global SYM_NAME(page1)
SYM_NAME(page1): .rept ARM_FIRST_LEVEL_PAGE_SIZE
                 .byte 0
                 .endr
#endif /* MMU */

/*
 * Macro Definition for an EXCEPTION stack
 */
.macro EXCEPTION_STACK label, size
	.bss
        .global SYM_NAME(\label)
	.align 4
	.rept	\size
	.byte	0
	.endr
SYM_NAME(\label):
.endm


/*
 * Provide a stack for use by exception processing
 */
        EXCEPTION_STACK _ex_stack, 16384


/*
 * Provide a simple stack for use in each of the
 * exception modes.  These are pretty small since
 * all we do is immediately switch to the exception
 * stack
 */
        EXCEPTION_STACK _abort_stack, 20
        EXCEPTION_STACK _address_err_stack, 20
        EXCEPTION_STACK _undefined_stack, 20
        EXCEPTION_STACK _irq_stack, 20
        EXCEPTION_STACK _fiq_stack, 20


        .text
SYM_NAME(_abort_stack_addr):             .word SYM_NAME(_abort_stack)
SYM_NAME(_address_err_stack_addr):       .word SYM_NAME(_address_err_stack)
SYM_NAME(_undefined_stack_addr):         .word SYM_NAME(_undefined_stack)
SYM_NAME(_irq_stack_addr):               .word SYM_NAME(_irq_stack)
SYM_NAME(_fiq_stack_addr):               .word SYM_NAME(_fiq_stack)
