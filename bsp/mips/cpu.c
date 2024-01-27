/*
 * cpu.c -- MIPS specific support for Cygnus BSP
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


void
_bsp_dump_regs(void *regs)
{
    ex_regs_t *p = regs;

    bsp_printf(" r0[0x%08lx]  at[0x%08lx]  v0[0x%08lx]  v1[0x%08lx]\n",
	       p->_zero, p->_at, p->_v0, p->_v1);
    bsp_printf(" a0[0x%08lx]  a1[0x%08lx]  a2[0x%08lx]  a3[0x%08lx]\n",
	       p->_a0, p->_a1, p->_a2, p->_a3);
    bsp_printf(" t0[0x%08lx]  t1[0x%08lx]  t2[0x%08lx]  t3[0x%08lx]\n",
	       p->_t0, p->_t1, p->_t2, p->_t3);
    bsp_printf(" t4[0x%08lx]  t5[0x%08lx]  t6[0x%08lx]  t7[0x%08lx]\n",
	       p->_t4, p->_t5, p->_t6, p->_t7);
    bsp_printf(" s0[0x%08lx]  s1[0x%08lx]  s2[0x%08lx]  s3[0x%08lx]\n",
	       p->_s0, p->_s1, p->_s2, p->_s3);
    bsp_printf(" s4[0x%08lx]  s5[0x%08lx]  s6[0x%08lx]  s7[0x%08lx]\n",
	       p->_s4, p->_s5, p->_s6, p->_s7);
    bsp_printf(" t8[0x%08lx]  t9[0x%08lx]  k0[0x%08lx]  k1[0x%08lx]\n",
	       p->_t8, p->_t9, p->_k0, p->_k1);
    bsp_printf(" gp[0x%08lx]  sp[0x%08lx]  fp[0x%08lx]  ra[0x%08lx]\n",
	       p->_gp, p->_sp, p->_fp, p->_ra);
    bsp_printf(" pc[0x%08lx]  sr[0x%08x]  hi[0x%08lx]  lo[0x%08lx]\n",
	       p->_epc, p->_sr, p->_hi, p->_lo);
    bsp_printf(" cause[0x%08x]  badvaddr[0x%08lx]\n",
	       p->_cause, p->_bad);
}

#define DEBUG_SYSCALL 0

static int
syscall_handler(int exc_nr, void *regs_ptr)
{
    ex_regs_t *regs = regs_ptr;
    int err, func;
    int arg1, arg2, arg3, arg4;

    func = regs->_a0;
    arg1 = regs->_a1;
    arg2 = regs->_a2;
    arg3 = regs->_a3;
    arg4 = regs->_t0;

#if DEBUG_SYSCALL
    bsp_printf("handle_syscall: func<%d> args<0x%x, 0x%x, 0x%x, 0x%x>...",
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

    /*
     * skip past 'syscall' opcode.
     */
    regs->_epc += 4;

    if (_bsp_do_syscall(func, arg1, arg2, arg3, arg4, &err)) {
	regs->_v0 = err;
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


/*
 *  Architecture specific BSP initialization.
 */
void
_bsp_cpu_init(void)
{
    static bsp_vec_t sc_vec;

    bsp_shared_data->__flush_dcache = __dcache_flush;
    bsp_shared_data->__flush_icache = __icache_flush;

    /*
     * Install syscall handler.
     */
    sc_vec.handler = syscall_handler;
    sc_vec.next    = NULL;
    bsp_install_vec(BSP_VEC_EXCEPTION, BSP_EXC_SYSCALL,
		    BSP_VEC_REPLACE, &sc_vec);
}

