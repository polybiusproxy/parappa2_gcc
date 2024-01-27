/*
 * cpu.c -- ARM specific support for Cygnus BSP
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
#include <stdlib.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include <insn.h>
#include "bsp_if.h"
#include "gdb.h"
#include "syscall.h"

/*
 * CPU specific data for ARM boards.
 */
#ifdef CPU_SHARP_LH77790A
#  include <lh77790a.h>
   static arm_cpu_data cpu_data = { 0 };
#endif /* CPU_SHARP_LH77790A */

extern unsigned char ROM_VECTOR_START;
extern unsigned char ROM_VECTOR_END;

/*
 * Initialize some vectors
 */
void _bsp_initvectors(void)
{
}

void
_bsp_dump_regs(void *regs)
{
    ex_regs_t *p = regs;

    bsp_printf(" r0[0x%08lx]  r1[0x%08lx]    r2[0x%08lx]   r3[0x%08lx]\n",
               p->_r0, p->_r1, p->_r2, p->_r3);
    bsp_printf(" r4[0x%08lx]  r5[0x%08lx]    r6[0x%08lx]   r7[0x%08lx]\n",
               p->_r4, p->_r5, p->_r6, p->_r7);
    bsp_printf(" r8[0x%08lx]  r9[0x%08lx]    r10[0x%08lx]  r11[0x%08lx]\n",
               p->_r8, p->_r9, p->_r10, p->_r11);
    bsp_printf(" r12[0x%08lx] sp[0x%08lx]    lr[0x%08lx]\n",
               p->_r12, p->_sp, p->_lr);
    bsp_printf(" pc[0x%08lx]  cpsr[0x%08lx]  spsr[0x%08lx]\n",
               p->_pc, p->_cpsr, p->_spsr);
}

#define DEBUG_SYSCALL 0

static int
syscall_handler(int exc_nr, void *regs_ptr)
{
    ex_regs_t *regs = regs_ptr;
    int func, arg1, arg2, arg3, arg4;
    int err;
    union arm_insn inst;

    /*
     * What is the instruction we were executing
     */
    inst.word = *(unsigned long *)(regs->_pc - ARM_INST_SIZE);

    if ((inst.swi.rsv1 != SWI_RSV1_VALUE) || (inst.swi.swi_number != SYSCALL_SWI))
    {
        /*
         * Not a syscall.  Don't handle it
         */
        return 0;
    }

    func = regs->_r0;
    arg1 = regs->_r1;
    arg2 = regs->_r2;
    arg3 = regs->_r3;
    arg4 = *((unsigned int *)(regs->_sp + 4));

#if DEBUG_SYSCALL
    bsp_printf("handle_syscall: func<%d> args<0x%x, 0x%x, 0x%x, 0x%x>...\n",
               func, arg1, arg2, arg3, arg4);
    bsp_printf("Register Dump (0x%08lx):\n", (unsigned long)regs);
    _bsp_dump_regs(regs);
#endif

    if (func == SYS_exit) {
        /*
         * Update the _pc so we will reexecute the swi instruction
         * if the user continues or steps, and we will simply
         * return here;
         */
        regs->_pc -= ARM_INST_SIZE;

        /*
         *  We want to stop in exit so that the user may poke around
         *  to see why his app exited.
         */
        (*bsp_shared_data->__dbg_vector)(exc_nr, regs);

        return 1;
    }

    if (_bsp_do_syscall(func, arg1, arg2, arg3, arg4, &err)) {
        regs->_r0 = err;
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

#ifdef MMU
#include BOARD_HEADER
/*
 * Initialize and enable the mmu if it exists
 */
void
_bsp_mmu_init(void)
{
#if 0
    unsigned long ttb_val;

    __mrc(ARM_CACHE_COPROCESSOR_NUM,
          ARM_COPROCESSOR_OPCODE_DONT_CARE,
          ttb_val,
          ARM_TRANSLATION_TABLE_BASE_REGISTER,
          ARM_COPROCESSOR_RM_DONT_CARE,
          ARM_COPROCESSOR_OPCODE_DONT_CARE);

    /*
     * First clear all TT entries - ie Set them to Faulting
     */
    ldr    r0, =0xC0000000		       /* Base of RAM Bank for TTBs  */
    ldr    r3, =(0x01000000 - (2 * 16384))     /* Top of RAM - 2 Page Tables */
    add    r3, r3, r0                          /* Real address of Level1 TTB */
    ldr    r2, =0
    ldr    r0, =(0x1000 + 0x200)               /* Level1_Table_Size + Level2_Table_Size */

1:
    str	   r2, [r3], #4
    subs   r0, r0, #1	/* decrement loop count */
    bne    1b

    bsp_printf("ttb_val = 0x%08lx\n", ttb_val);
#endif
}
#endif /* MMU */

/*
 *  Architecture specific BSP initialization.
 */
void
_bsp_cpu_init(void)
{
    static bsp_vec_t sc_vec;

#ifdef CPU_SHARP_LH77790A
    /*
     * Install cpu specific data
     */
    bsp_shared_data->__cpu_data = &cpu_data;
#endif /* CPU_SHARP_LH77790A */

#ifdef CPU_SA1100
    {
        extern void __dcache_flush(void *__p, int __nbytes);
        extern void __icache_flush(void *__p, int __nbytes);

        _bsp_platform_info.cpu = "StrongArm SA1100";

        _bsp_dcache_info.size = SA1100_DCACHE_SIZE;
        _bsp_dcache_info.linesize = SA1100_DCACHE_LINESIZE;
        _bsp_dcache_info.ways = SA1100_DCACHE_WAYS;

        _bsp_icache_info.size = SA1100_ICACHE_SIZE;
        _bsp_icache_info.linesize = SA1100_ICACHE_LINESIZE;
        _bsp_icache_info.ways = SA1100_ICACHE_WAYS;

        bsp_shared_data->__flush_icache = __icache_flush;
        bsp_shared_data->__flush_dcache = __dcache_flush;
    }
#endif

#ifdef CPU_SA110
    _bsp_platform_info.cpu = "StrongArm SA110";
#endif

#ifdef MMU
    _bsp_mmu_init();
#endif /* MMU */

    /*
     * Install syscall handler.
     */
    sc_vec.handler = syscall_handler;
    bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_SOFTWARE_INTERRUPT,
                    BSP_VEC_REPLACE, &sc_vec);
}
