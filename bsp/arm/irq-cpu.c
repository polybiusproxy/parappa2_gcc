/*
 * irq-cpu.c -- Interrupt support for M68K.
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
#include "bsp_if.h"
#include <insn.h>

#define DEBUG_INT_CODE 0

#if DEBUG_INT_CODE
#include BOARD_HEADER

/*
 * Print out a stack dump
 */
void stack_dump(unsigned short *sp)
{
    int i, j;
    
    for (j = 0; j < 8; j++)
    {
        bsp_printf("\t0x%08lx: ", sp);
        for (i = 0; i < 8; i++)
        {
            unsigned short sp_value;

            if (bsp_memory_read((void *)sp, 0, sizeof(*sp) * 8, 1, &sp_value) != 0)
                bsp_printf("%04x ", sp_value);
            else
                bsp_printf(".... ");

            sp++;
        }
        bsp_printf("\n");
    }
}

/*
 * Print some diagnostic info about the exception
 */
static void exceptionPrint(ex_regs_t *regs, unsigned long vect_num)
{
    bsp_printf("Exception #0x%x\n", vect_num);
    if ((vect_num != BSP_CORE_EXC_PREFETCH_ABORT) &&
        (vect_num != BSP_CORE_EXC_DATA_ABORT))
    {
        /*
         * Only do a stack dump if we are not processing
         * an abort exception already.  If we do a stack
         * dump in an exception, that will corrupt the 
         * jmpbuf already in use in generic_mem.c
         *
         * Besides, we really don't want to produce a
         * double-abort situation
         */
        bsp_printf("Stack Frame:\n");
        stack_dump((unsigned short *)(regs->_sp));
    }
    bsp_printf("Register Dump (0x%08lx):\n", (unsigned long)regs);
    _bsp_dump_regs(regs);
}
#endif /* DEBUG_INT_CODE */

/*
 *  Architecture specific routine to 'install' interrupt controllers.
 */
void
_bsp_install_cpu_irq_controllers(void)
{
#if DEBUG_INT_CODE
    bsp_printf("_bsp_install_cpu_irq_controllers()\n");
#endif /* DEBUG_INT_CODE */

#ifdef CPU_SA1100
    {
        extern void _bsp_install_sa1100_irq_controllers(void);
        _bsp_install_sa1100_irq_controllers();
    }
#endif /* CPU_SA1100 */

#ifdef CPU_SA110
    {
        extern void _bsp_install_sa110_irq_controllers(void);
        _bsp_install_sa110_irq_controllers();
    }
#endif /* CPU_SA110 */
}

/*
 * Temporarily switch to a different mode so the R8
 * through R12 registers for that mode will be visible
 * and update the regs struct with their values.
 *
 * Make sure you don't use any stack based storage 
 * here since the stack will be temporarily invalid.
 * ie: it must be inline
 */
static inline void get_r8_r12_from_mode(ex_regs_t *regs, unsigned mode)
{
    register union arm_psr orig_cpsr asm ("r0");
    register union arm_psr new_cpsr asm ("r1");
    register int new_r8  asm ("r2");
    register int new_r9  asm ("r3");
    register int new_r10 asm ("r4");
    register int new_r11 asm ("r5");
    register int new_r12 asm ("r6");

    new_cpsr.word = orig_cpsr.word = __get_cpsr();
    new_cpsr.psr.mode = mode;

    __set_cpsr(new_cpsr.word);
    new_r8  = __get_r8();
    new_r9  = __get_r9();
    new_r10 = __get_r10();
    new_r11 = __get_r11();
    new_r12 = __get_r12();
    __set_cpsr(orig_cpsr.word);

    regs->_r8  = new_r8;
    regs->_r9  = new_r9;
    regs->_r10 = new_r10;
    regs->_r11 = new_r11;
    regs->_r12 = new_r12;
}

/*
 * Temporarily switch to a different mode so the R8
 * through R12 registers for that mode will be visible
 * and update them from the regs struct.
 *
 * Make sure you don't use any stack based storage 
 * here since the stack will be temporarily invalid.
 * ie: it must be inline
 */
static inline void set_r8_r12_in_mode(ex_regs_t *regs, unsigned new_mode)
{
    register union arm_psr orig_cpsr asm ("r0");
    register union arm_psr new_cpsr asm ("r1");
    register int new_r8  asm ("r2") = regs->_r8;
    register int new_r9  asm ("r3") = regs->_r9;
    register int new_r10 asm ("r4") = regs->_r10;
    register int new_r11 asm ("r5") = regs->_r11;
    register int new_r12 asm ("r6") = regs->_r12;

    new_cpsr.word = orig_cpsr.word = __get_cpsr();
    new_cpsr.psr.mode = new_mode;

    __set_cpsr(new_cpsr.word);
    __set_r8(new_r8);
    __set_r9(new_r9);
    __set_r10(new_r10);
    __set_r11(new_r11);
    __set_r12(new_r12);
    __set_cpsr(orig_cpsr.word);

    regs->_r8  = new_r8;
    regs->_r9  = new_r9;
    regs->_r10 = new_r10;
    regs->_r11 = new_r11;
    regs->_r12 = new_r12;
}

/*
 * Temporarily switch to a different mode so the LR
 * and SP registers for that mode will be visible
 * and update the regs struct with their values.
 *
 * Make sure you don't use any stack based storage 
 * here since the stack will be temporarily invalid.
 * ie: it must be inline
 */
static inline void get_sp_lr_from_mode(ex_regs_t *regs, unsigned mode)
{
    register union arm_psr orig_cpsr asm ("r0");
    register union arm_psr new_cpsr asm ("r1");
    register int new_lr asm ("r2");
    register int new_sp asm ("r3");

    new_cpsr.word = orig_cpsr.word = __get_cpsr();
    new_cpsr.psr.mode = mode;

    __set_cpsr(new_cpsr.word);
    new_sp = __get_sp();
    new_lr = __get_lr();
    __set_cpsr(orig_cpsr.word);

    regs->_sp = new_sp;
    regs->_lr = new_lr;
}

/*
 * Temporarily switch to a different mode so the LR
 * and SP registers for that mode will be visible
 * and update them from the regs struct.
 *
 * Make sure you don't use any stack based storage 
 * here since the stack will be temporarily invalid.
 * ie: it must be inline
 */
static inline void set_sp_lr_in_mode(ex_regs_t *regs, unsigned new_mode)
{
    register union arm_psr orig_cpsr asm ("r0");
    register union arm_psr new_cpsr asm ("r1");
    register int new_lr asm ("r2") = regs->_lr;
    register int new_sp asm ("r3") = regs->_sp;

    new_cpsr.word = orig_cpsr.word = __get_cpsr();
    new_cpsr.psr.mode = new_mode;

    __set_cpsr(new_cpsr.word);
    __set_sp(new_sp);
    __set_lr(new_lr);
    __set_cpsr(orig_cpsr.word);

    regs->_sp = new_sp;
    regs->_lr = new_lr;
}

/*
 * This is the generic exception handler that is called by all exceptions
 * after the regs structure is fixed up
 */
void generic_exception_handler(ex_regs_t *regs, unsigned long vect_num)
{
    unsigned long old_mode = 0;
    unsigned long r8_save  = 0;
    unsigned long r9_save  = 0;
    unsigned long r10_save = 0;
    unsigned long r11_save = 0;
    unsigned long r12_save = 0;
    unsigned long sp_save  = 0;
    unsigned long lr_save  = 0;

#if DEBUG_INT_CODE && !defined(NDEBUG)
    {
        union arm_psr cur_psr;
        union arm_insn inst;

        /*
         * What is the current psr
         */
        cur_psr.word = __get_cpsr();

        BSP_ASSERT(cur_psr.psr.mode == ARM_PSR_MODE_SVC);
        BSP_ASSERT(regs->_sp == ((unsigned long)regs + sizeof(*regs)));
        switch (vect_num)
        {
        case BSP_CORE_EXC_PREFETCH_ABORT:
        case BSP_CORE_EXC_DATA_ABORT:
            /*
             * No assertions
             */
            break;
        case BSP_CORE_EXC_SOFTWARE_INTERRUPT:
            /*
             * Verify that we were executing a swi instruction.
             */
            if (bsp_memory_read((void *)(regs->_lr - ARM_INST_SIZE), 0, 
                                ARM_INST_SIZE * 8, 1, &(inst.word)) != 0)
            {
                BSP_ASSERT(inst.swi.rsv1 == SWI_RSV1_VALUE);
            }
            break;

        case BSP_CORE_EXC_UNDEFINED_INSTRUCTION:
            /*
             * No assertions
             */
            break;

        case BSP_CORE_EXC_ADDRESS_ERROR_26_BIT:
            /*
             * No assertions
             */
            break;

        case BSP_CORE_EXC_IRQ:
            /*
             * Verify that IRQ's are masked
             */
            BSP_ASSERT(cur_psr.psr.i_bit = 1);
            break;

        case BSP_CORE_EXC_FIQ:
            /*
             * Verify that IRQ's and FIQ's are masked
             */
            BSP_ASSERT(cur_psr.psr.i_bit = 1);
            BSP_ASSERT(cur_psr.psr.f_bit = 1);
            break;

        default:
            bsp_printf("Unknown exception %d\n", vect_num);
            break;
        }
    }
#endif /* defined(DEBUG_INT_CODE) && !defined(NDEBUG) */

    /*
     * If we were in user mode, let's pretend we were in
     * system mode so we can switch back and forth w/o
     * a swi instruction.
     */
    old_mode = ((union arm_psr)regs->_cpsr).psr.mode;
    if (old_mode == ARM_PSR_MODE_USER)
        old_mode = ARM_PSR_MODE_SYSTEM;

    /*
     * Now get the shadowed registers from their original mode
     */
    switch (old_mode)
    {
    case ARM_PSR_MODE_FIQ:
        /*
         * SVC-to-FIQ mode transition
         * Get all banked registers: r8-r12, sp, lr
         */

        /*
         * First, save off the old values
         */
        r8_save   = regs->_r8;
        r9_save   = regs->_r9;
        r10_save  = regs->_r10;
        r11_save  = regs->_r11;
        r12_save  = regs->_r12;
        sp_save   = regs->_sp;
        lr_save   = regs->_lr;

        get_r8_r12_from_mode(regs, old_mode);
        get_sp_lr_from_mode(regs, old_mode);
        break;

    case ARM_PSR_MODE_SVC:
        /*
         * We had an SVC-to-SVC mode transition.
         *
         * That means we are using the SVC stack and it is not now
         * what it was when the exception occurred.  We cannot get
         * it from the mode and we need to rely on the one in the
         * regs struct.
         *
         * It is impossible to know what the lr was when we hit the
         * exception since it has been overwritten by the exception
         * processing.
         */
        break;

    default:
        /*
         * SVC-to-other mode transition
         * Get some banked registers: sp, lr
         */

        /*
         * First, save off the old values
         */
        sp_save   = regs->_sp;
        lr_save   = regs->_lr;

        get_sp_lr_from_mode(regs, old_mode);
        break;
    }

#if DEBUG_INT_CODE
    bsp_printf("In generic_exception_handler()\n");
    exceptionPrint(regs, vect_num);
#endif /* DEBUG_INT_CODE */

    /*
     * Call the dispatch routine to see if anyone
     * has registered for this exception.
     */
    if (_bsp_exc_dispatch(vect_num, regs) == 0)
    {
        union arm_insn inst;

        /*
         * If this is an illegal instruction, we may need to adjust
         * the PC if it is a breakpoint that GDB inserted.
         */
        if ((vect_num == BSP_CORE_EXC_UNDEFINED_INSTRUCTION) &&
            (bsp_memory_read((void *)(regs->_pc - ARM_INST_SIZE), 0, 
                             ARM_INST_SIZE * 8, 1, &(inst.word)) != 0))
        {
            /*
             * We were able to read this address. It must be a valid address.
             */
            if (inst.word == BREAKPOINT_INSN)
            {
                /*
                 * Adjust the PC to point at the breakpoint instruction
                 * itself.
                 */
                regs->_pc -= ARM_INST_SIZE;
            }
        }
         
         /*
          * nobody handled this exception.
          * Let's let GDB take care of it
          */
         bsp_invoke_dbg_handler(vect_num, regs);
    }

    /*
     * Now restore the shadowed registers to their original mode
     */
    switch (old_mode)
    {
    case ARM_PSR_MODE_FIQ:
        /*
         * SVC-to-FIQ mode transition
         * Set all banked registers: r8-r12, sp, lr
         */
        set_r8_r12_in_mode(regs, old_mode);
        set_sp_lr_in_mode(regs, old_mode);

        /*
         * Now, restore the old values
         */
        regs->_r8  = r8_save;
        regs->_r9  = r9_save;
        regs->_r10 = r10_save;
        regs->_r11 = r11_save;
        regs->_r12 = r12_save;
        regs->_sp  = sp_save;
        regs->_lr  = lr_save;

        break;

    case ARM_PSR_MODE_SVC:
        /*
         * We had an SVC-to-SVC mode transition.
         *
         * That means we are using the SVC stack and it is not now
         * what it was when the exception occurred.  We cannot get
         * it from the mode and we need to rely on the one in the
         * regs struct.
         *
         * It is impossible to know what the lr was when we hit the
         * exception since it has been overwritten by the exception
         * processing.
         *
         * Since we don't know what to do with the registers
         * leave them alone.
         */
        break;

    default:
        /*
         * SVC-to-other mode transition
         * Set some banked registers: sp, lr
         */
        set_sp_lr_in_mode(regs, old_mode);

        /*
         * Now, restore the old values
         */
        regs->_sp  = sp_save;
        regs->_lr  = lr_save;

        break;
    }

#if DEBUG_INT_CODE
    bsp_printf("Returning from exception\n");
    _bsp_dump_regs(regs);

    /*
     * If we will switch modes on exception return,
     * make sure the stack will be in the right place.
     */
    BSP_ASSERT((old_mode == ARM_PSR_MODE_SVC) ||
               (regs->_sp == ((unsigned long)regs + sizeof(*regs))));
#endif /* DEBUG_INT_CODE */
}
