/*
 * cpu.c -- M68K specific support for Cygnus BSP
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
#include <stdlib.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include "bsp_if.h"
#include "gdb.h"
#include "syscall.h"

#define USE_SAMPLE_BUS_ADDR_HANDLERS 0

extern void addr_error_asm(void);
extern void bus_error_asm(void);
extern void m68k_ill_instruction_asm(void);
extern void m68k_div_zero_asm(void);
extern void m68k_chk_asm(void);
extern void m68k_ftrap_asm(void);
extern void m68k_priv_violation_asm(void);
extern void m68k_trace_asm(void);
extern void m68k_line_1010_asm(void);
extern void m68k_line_1111_asm(void);
extern void m68k_coproc_protocol_violation_asm(void);
extern void m68k_format_error_asm(void);
extern void m68k_uninitialized_interrupt_asm(void);
extern void m68k_spurious_int_asm(void);
extern void m68k_level_1_auto_asm(void);
extern void m68k_level_2_auto_asm(void);
extern void m68k_level_3_auto_asm(void);
extern void m68k_level_4_auto_asm(void);
extern void m68k_level_5_auto_asm(void);
extern void m68k_level_6_auto_asm(void);
extern void m68k_level_7_auto_asm(void);
extern void m68k_trap_0_asm(void);
extern void m68k_trap_1_asm(void);
extern void m68k_trap_2_asm(void);
extern void m68k_trap_3_asm(void);
extern void m68k_trap_4_asm(void);
extern void m68k_trap_5_asm(void);
extern void m68k_trap_6_asm(void);
extern void m68k_trap_7_asm(void);
extern void m68k_trap_8_asm(void);
extern void m68k_trap_9_asm(void);
extern void m68k_trap_10_asm(void);
extern void m68k_trap_11_asm(void);
extern void m68k_trap_12_asm(void);
extern void m68k_trap_13_asm(void);
extern void m68k_trap_14_asm(void);
extern void m68k_trap_15_asm(void);
extern void m68k_fp_unordered_cond_asm(void);
extern void m68k_fp_inexact_asm(void);
extern void m68k_fp_div_zero_asm(void);
extern void m68k_fp_underflow_asm(void);
extern void m68k_fp_operand_error_asm(void);
extern void m68k_fp_overflow_asm(void);
extern void m68k_fp_nan_asm(void);
extern void m68k_fp_unimp_data_type_asm(void);
extern void m68k_mmu_config_error_asm(void);
extern void m68k_mmu_ill_operation_asm(void);
extern void m68k_mmu_access_violation_asm(void);

/*
 * Initialize some vectors
 */
void _bsp_initvectors(void)
{
    *BSP_CORE_EXC(BSP_CORE_EXC_ADDR_ERROR)                = (unsigned long)addr_error_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_BUS_ERROR)                 = (unsigned long)bus_error_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_ILL_INSTRUCTION)           = (unsigned long)m68k_ill_instruction_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_DIV_ZERO)                  = (unsigned long)m68k_div_zero_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_CHK)                       = (unsigned long)m68k_chk_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP)                      = (unsigned long)m68k_ftrap_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_PRIV_VIOLATION)            = (unsigned long)m68k_priv_violation_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRACE)                     = (unsigned long)m68k_trace_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LINE_1010)                 = (unsigned long)m68k_line_1010_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LINE_1111)                 = (unsigned long)m68k_line_1111_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_COPROC_PROTOCOL_VIOLATION) = (unsigned long)m68k_coproc_protocol_violation_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FORMAT_ERROR)              = (unsigned long)m68k_format_error_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_UNINITIALIZED_INTERRUPT)   = (unsigned long)m68k_uninitialized_interrupt_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_SPURIOUS_INT)              = (unsigned long)m68k_spurious_int_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_1_AUTO)              = (unsigned long)m68k_level_1_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_2_AUTO)              = (unsigned long)m68k_level_2_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_3_AUTO)              = (unsigned long)m68k_level_3_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_4_AUTO)              = (unsigned long)m68k_level_4_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_5_AUTO)              = (unsigned long)m68k_level_5_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_6_AUTO)              = (unsigned long)m68k_level_6_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_LEVEL_7_AUTO)              = (unsigned long)m68k_level_7_auto_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_0)                    = (unsigned long)m68k_trap_0_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_1)                    = (unsigned long)m68k_trap_1_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_2)                    = (unsigned long)m68k_trap_2_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_3)                    = (unsigned long)m68k_trap_3_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_4)                    = (unsigned long)m68k_trap_4_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_5)                    = (unsigned long)m68k_trap_5_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_6)                    = (unsigned long)m68k_trap_6_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_7)                    = (unsigned long)m68k_trap_7_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_8)                    = (unsigned long)m68k_trap_8_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_9)                    = (unsigned long)m68k_trap_9_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_10)                   = (unsigned long)m68k_trap_10_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_11)                   = (unsigned long)m68k_trap_11_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_12)                   = (unsigned long)m68k_trap_12_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_13)                   = (unsigned long)m68k_trap_13_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_14)                   = (unsigned long)m68k_trap_14_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_TRAP_15)                   = (unsigned long)m68k_trap_15_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_UNORDERED_COND)         = (unsigned long)m68k_fp_unordered_cond_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_INEXACT)                = (unsigned long)m68k_fp_inexact_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_DIV_ZERO)               = (unsigned long)m68k_fp_div_zero_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_UNDERFLOW)              = (unsigned long)m68k_fp_underflow_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_OPERAND_ERROR)          = (unsigned long)m68k_fp_operand_error_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_OVERFLOW)               = (unsigned long)m68k_fp_overflow_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_NAN)                    = (unsigned long)m68k_fp_nan_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_FP_UNIMP_DATA_TYPE)        = (unsigned long)m68k_fp_unimp_data_type_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_MMU_CONFIG_ERROR)          = (unsigned long)m68k_mmu_config_error_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_MMU_ILL_OPERATION)         = (unsigned long)m68k_mmu_ill_operation_asm;
    *BSP_CORE_EXC(BSP_CORE_EXC_MMU_ACCESS_VIOLATION)      = (unsigned long)m68k_mmu_access_violation_asm;
}

void
_bsp_dump_regs(void *regs)
{
    ex_regs_t *p = regs;

    bsp_printf(" d0[0x%08lx]  d1[0x%08lx]  d2[0x%08lx]  d3[0x%08lx]\n", 
               p->_d0, p->_d1, p->_d2, p->_d3);
    bsp_printf(" d4[0x%08lx]  d5[0x%08lx]  d6[0x%08lx]  d7[0x%08lx]\n",
               p->_d4, p->_d5, p->_d6, p->_d7);
    bsp_printf(" a0[0x%08lx]  a1[0x%08lx]  a2[0x%08lx]  a3[0x%08lx]\n", 
               p->_a0, p->_a1, p->_a2, p->_a3);
    bsp_printf(" a4[0x%08lx]  a5[0x%08lx]  a6[0x%08lx]  a7[0x%08lx]\n",
               p->_a4, p->_a5, p->_a6, p->_a7);
    bsp_printf(" sr[0x%04lx]      pc[0x%08lx]\n",
               p->_sr, p->_pc);
}

#define DEBUG_SYSCALL 0

static int
syscall_handler(int exc_nr, void *regs_ptr)
{
    ex_regs_t *regs = regs_ptr;
    int func, arg1, arg2, arg3, arg4;
    int err;

    func = *((unsigned int *)(regs->_sp + 4));
    arg1 = *((unsigned int *)(regs->_sp + 8));
    arg2 = *((unsigned int *)(regs->_sp + 12));
    arg3 = *((unsigned int *)(regs->_sp + 16));
    arg4 = *((unsigned int *)(regs->_sp + 20));

#if DEBUG_SYSCALL
    bsp_printf("regs_ptr = 0x%08lx\n", regs_ptr);
    bsp_printf("regs->_sp = 0x%08lx\n", regs->_sp);
    bsp_printf("handle_syscall: func<%d> args<0x%x, 0x%x, 0x%x, 0x%x>...\n",
               func, arg1, arg2, arg3, arg4);
#endif

    if (func == SYS_exit) {
        /*
         *  We want to stop in exit so that the user may poke around
         *  to see why his app exited.
         */
        (*bsp_shared_data->__dbg_vector)(exc_nr, regs);
        return 1;
    }

    if (_bsp_do_syscall(func, arg1, arg2, arg3, arg4, &err)) {
        regs->_d0 = err;
#if DEBUG_SYSCALL
        bsp_printf("returned: %d\n",err);
#endif
        return 1;
    }

#if DEBUG_SYSCALL
    bsp_printf("not handled.\n");
#endif
    return 0;
}

#if USE_SAMPLE_BUS_ADDR_HANDLERS
static int addr_err_handler(int ex_nr, ex_regs_t *regs);
static int bus_err_handler(int ex_nr, ex_regs_t *regs);
#endif /* USE_SAMPLE_BUS_ADDR_HANDLERS */

/*
 *  Architecture specific BSP initialization.
 */
void
_bsp_cpu_init(void)
{
    static bsp_vec_t sc_vec;

    /*
     * Install syscall handler.
     */
    sc_vec.handler = syscall_handler;
    bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_TRAP_15,
                    BSP_VEC_REPLACE, &sc_vec);

#if USE_SAMPLE_BUS_ADDR_HANDLERS
    /*
     * Setup default handlers for address and bus errors
     */
    {
        static bsp_vec_t addr_irq_vec;
        static bsp_vec_t bus_irq_vec;

        addr_irq_vec.handler = (void *)addr_err_handler;
        addr_irq_vec.next = NULL;
        bus_irq_vec.handler = (void *)bus_err_handler;
        bus_irq_vec.next = NULL;

        bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_ADDR_ERROR, BSP_VEC_CHAIN_LAST, &addr_irq_vec);
        bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_BUS_ERROR, BSP_VEC_CHAIN_LAST, &bus_irq_vec);
    }
#endif
}

#if USE_SAMPLE_BUS_ADDR_HANDLERS

static int
addr_err_handler(int ex_nr, ex_regs_t *regs)
{
    /*
     * The stack frame is calculated based on
     * the stack that is setup by the code in
     * start.S.  See that file for a stack
     * diagram.
   */
    m68k_bus_addr_stkfrm *frame = (m68k_bus_addr_stkfrm *)((unsigned long)regs
                                                           + (unsigned long)sizeof(*regs)
                                                           + (unsigned long)sizeof(ex_nr));

    bsp_printf("Address Error\n");
    bsp_printf("Register Dump (0x%08lx):\n", (unsigned long)regs);
    _bsp_dump_regs(regs);
    bsp_printf("Stack frame (0x%08lx):\n", frame);
    bsp_printf("\tcode = 0x%04x\taccess address = 0x%04x%04x\n", 
               frame->code, frame->access_h, frame->access_l);
    bsp_printf("\tir   = 0x%04x\tsr = 0x%04x\n", frame->ir, frame->sr);
    bsp_printf("\tpc   = 0x%04x%04x\n", frame->pc_h, frame->pc_l);

    while (1) ;
    return 1;
}

static int
bus_err_handler(int ex_nr, ex_regs_t *regs)
{
    /*
     * The stack frame is calculated based on
     * the stack that is setup by the code in
     * start.S.  See that file for a stack
     * diagram.
   */
    m68k_bus_addr_stkfrm *frame = (m68k_bus_addr_stkfrm *)((unsigned long)regs
                                                           + (unsigned long)sizeof(*regs)
                                                           + (unsigned long)sizeof(ex_nr));

    bsp_printf("Bus Error\n");
    bsp_printf("Register Dump (0x%08lx):\n", (unsigned long)regs);
    _bsp_dump_regs(regs);
    bsp_printf("Stack frame (0x%08lx):\n", frame);
    bsp_printf("\tcode = 0x%04x\taccess address = 0x%04x%04x\n", 
               frame->code, frame->access_h, frame->access_l);
    bsp_printf("\tir   = 0x%04x\tsr = 0x%04x\n", frame->ir, frame->sr);
    bsp_printf("\tpc   = 0x%04x%04x\n", frame->pc_h, frame->pc_l);

    while (1) ;
    return 1;
}


int generate_bus_addr_error(void)
{
#if 0
    /*
     * Force Address Error
     */
    *(unsigned long*)0x3 = 4;
#else
    /*
     * Force Bus Error
     */
    *(unsigned long*)0x600000 = 4;
#endif 

    return 0;
}
#endif /* USE_SAMPLE_BUS_ADDR_HANDLERS */
