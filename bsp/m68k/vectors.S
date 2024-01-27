/*
 * vectors.S -- Exception vector code for M68K BSP
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
/*
 * Macro Definition for an EXCEPTION vector
 *
 * This will simply push the vector number and jump to the
 * generic exception handler.
 */
.macro EXCEPTION_VECTOR label, vect_num
FUNC_START(\label)
        pea   \vect_num
        bra   SYM_NAME(generic_except)
FUNC_END(\label)
.endm


/*
 * Setup the exception vectors for all the 68k exceptions
 */
        EXCEPTION_VECTOR addr_error_asm, BSP_CORE_EXC_ADDR_ERROR
        EXCEPTION_VECTOR bus_error_asm,  BSP_CORE_EXC_BUS_ERROR
        EXCEPTION_VECTOR m68k_ill_instruction_asm, BSP_CORE_EXC_ILL_INSTRUCTION
        EXCEPTION_VECTOR m68k_div_zero_asm, BSP_CORE_EXC_DIV_ZERO
        EXCEPTION_VECTOR m68k_chk_asm, BSP_CORE_EXC_CHK
        EXCEPTION_VECTOR m68k_ftrap_asm, BSP_CORE_EXC_TRAP
        EXCEPTION_VECTOR m68k_priv_violation_asm, BSP_CORE_EXC_PRIV_VIOLATION
        EXCEPTION_VECTOR m68k_trace_asm, BSP_CORE_EXC_TRACE
        EXCEPTION_VECTOR m68k_line_1010_asm, BSP_CORE_EXC_LINE_1010
        EXCEPTION_VECTOR m68k_line_1111_asm, BSP_CORE_EXC_LINE_1111
        EXCEPTION_VECTOR m68k_coproc_protocol_violation_asm, BSP_CORE_EXC_COPROC_PROTOCOL_VIOLATION
        EXCEPTION_VECTOR m68k_format_error_asm, BSP_CORE_EXC_FORMAT_ERROR
        EXCEPTION_VECTOR m68k_uninitialized_interrupt_asm, BSP_CORE_EXC_UNINITIALIZED_INTERRUPT
        EXCEPTION_VECTOR m68k_spurious_int_asm, BSP_CORE_EXC_SPURIOUS_INT
        EXCEPTION_VECTOR m68k_level_1_auto_asm, BSP_CORE_EXC_LEVEL_1_AUTO
        EXCEPTION_VECTOR m68k_level_2_auto_asm, BSP_CORE_EXC_LEVEL_2_AUTO
        EXCEPTION_VECTOR m68k_level_3_auto_asm, BSP_CORE_EXC_LEVEL_3_AUTO
        EXCEPTION_VECTOR m68k_level_4_auto_asm, BSP_CORE_EXC_LEVEL_4_AUTO
        EXCEPTION_VECTOR m68k_level_5_auto_asm, BSP_CORE_EXC_LEVEL_5_AUTO
        EXCEPTION_VECTOR m68k_level_6_auto_asm, BSP_CORE_EXC_LEVEL_6_AUTO
        EXCEPTION_VECTOR m68k_level_7_auto_asm, BSP_CORE_EXC_LEVEL_7_AUTO
        EXCEPTION_VECTOR m68k_trap_0_asm, BSP_CORE_EXC_TRAP_0
        EXCEPTION_VECTOR m68k_trap_1_asm, BSP_CORE_EXC_TRAP_1
        EXCEPTION_VECTOR m68k_trap_2_asm, BSP_CORE_EXC_TRAP_2
        EXCEPTION_VECTOR m68k_trap_3_asm, BSP_CORE_EXC_TRAP_3
        EXCEPTION_VECTOR m68k_trap_4_asm, BSP_CORE_EXC_TRAP_4
        EXCEPTION_VECTOR m68k_trap_5_asm, BSP_CORE_EXC_TRAP_5
        EXCEPTION_VECTOR m68k_trap_6_asm, BSP_CORE_EXC_TRAP_6
        EXCEPTION_VECTOR m68k_trap_7_asm, BSP_CORE_EXC_TRAP_7
        EXCEPTION_VECTOR m68k_trap_8_asm, BSP_CORE_EXC_TRAP_8
        EXCEPTION_VECTOR m68k_trap_9_asm, BSP_CORE_EXC_TRAP_9
        EXCEPTION_VECTOR m68k_trap_10_asm, BSP_CORE_EXC_TRAP_10
        EXCEPTION_VECTOR m68k_trap_11_asm, BSP_CORE_EXC_TRAP_11
        EXCEPTION_VECTOR m68k_trap_12_asm, BSP_CORE_EXC_TRAP_12
        EXCEPTION_VECTOR m68k_trap_13_asm, BSP_CORE_EXC_TRAP_13
        EXCEPTION_VECTOR m68k_trap_14_asm, BSP_CORE_EXC_TRAP_14
        EXCEPTION_VECTOR m68k_trap_15_asm, BSP_CORE_EXC_TRAP_15
        EXCEPTION_VECTOR m68k_fp_unordered_cond_asm, BSP_CORE_EXC_FP_UNORDERED_COND
        EXCEPTION_VECTOR m68k_fp_inexact_asm, BSP_CORE_EXC_FP_INEXACT
        EXCEPTION_VECTOR m68k_fp_div_zero_asm, BSP_CORE_EXC_FP_DIV_ZERO
        EXCEPTION_VECTOR m68k_fp_underflow_asm, BSP_CORE_EXC_FP_UNDERFLOW
        EXCEPTION_VECTOR m68k_fp_operand_error_asm, BSP_CORE_EXC_FP_OPERAND_ERROR
        EXCEPTION_VECTOR m68k_fp_overflow_asm, BSP_CORE_EXC_FP_OVERFLOW
        EXCEPTION_VECTOR m68k_fp_nan_asm, BSP_CORE_EXC_FP_NAN
        EXCEPTION_VECTOR m68k_fp_unimp_data_type_asm, BSP_CORE_EXC_FP_UNIMP_DATA_TYPE
        EXCEPTION_VECTOR m68k_mmu_config_error_asm, BSP_CORE_EXC_MMU_CONFIG_ERROR
        EXCEPTION_VECTOR m68k_mmu_ill_operation_asm, BSP_CORE_EXC_MMU_ILL_OPERATION
        EXCEPTION_VECTOR m68k_mmu_access_violation_asm, BSP_CORE_EXC_MMU_ACCESS_VIOLATION


/*
 * This is the generic exception handler.
 * All exceptions will be routed through this routine.
 *
 * Once we are here, the vector number is pushed onto the stack.
 * 
 * We will set up the parameters for a _bsp_exc_dispatch() call
 *
 */
FUNC_START(generic_except)
	/*
         * Save the address of the stack frame on the stack
         */
        pea     sp@(4)

        /*
         * See if we need to switch stacks
         */
      	cmp	IMM(SYM_NAME(_ex_stack)),sp
        ble	1f

        /*
	 * Need to switch stacks.
         */
        movel   a0, sp@-                        /* Save a0                          */

        /*
         * At this point, the main (old) stack will look as follows:
         *              .               <-- High Mem
         *              .
         *              .
         *       |              |
         *       |   Program    |
         *       |    Stack     |
         *       |              |
         *              .       
         *              .
         *              .
         *       |              |
         *       | Stack Frame  |
         *       +--------------+
         *       | vect_num (4) |
         *       +--------------+
         *       | &Stack Frame |
         *       +--------------+
         *       |   a0 saved   |
         *       +--------------+
         *
         *
         * Let's make the new stack (a0) look as follows:
         *
         *  ex_stack:   +--------------+
         *  (in bss)    | vect_num (4) |
         *              +--------------+
         *              | &Stack Frame |
         *              +--------------+
         *              |   a0 saved   |
         *              +--------------+
         */
        moveal  IMM(SYM_NAME(_ex_stack)), a0    /* a0 is the new stack              */
        movel   sp@(8), a0@-                    /* Copy vect # to new stack         */
        movel   sp@(4), a0@-                    /* Copy stkfrm address to new stack */
        movel   sp@, a0@-                       /* Copy saved a0 to new stack       */

        /*
         * Now we switch to the new stack
         * and restore a0
         */
        movel  a0, sp                           /* Start using the new stack        */
        moveal  sp@+, a0                        /* Restore a0                       */

        /*
         * Now we are using the right stack.
         */
1:
        lea    sp@(-12), sp            /* save space for SP/SR/PC in the register image        */
        moveml d0-d7/a0-a6,sp@-        /* save all the registers before C can modify them      */

        /*
         * Call into C code
         *     processException(uint16 *stkfrm, uint32 *registers, uint32 vect_num)
         *
         * At this point, we want the stack to look as follows:
         *              .
         *              .             <-- High Mem
         *              .
         *       |              |
         *       +--------------+
         *       | vect_num (4) |
         *       +--------------+
         *       | &Stk Frm(4)  |
         *       +--------------+
         *       |              |
         *       |              |   (16 data & address registers (4 bytes wide)
         *       | Reg Img (72) |    + PC (4 bytes wide)
         *       |              |    + SR (4 bytes wide))
         *       |              |
         *       +--------------+
         *       | vect_num (4) |   *(SP + 72 + 4)
         *       +--------------+
         *       | &Reg Img (4) |   (SP + 4)
         *       +--------------+
         *       | &StkFrm (4)  |   *(SP + 72 + 4 + 4)
         *       +--------------+
         */
        movel  sp@(72+4),sp@-             /* Last Parm is the vect_num                    */
        pea    sp@(4)                     /* Second Parm is the address of register image */
        movel  sp@(72+4+4),sp@-           /* First Parm is the stkfrm                     */
        bsr    SYM_NAME(processException) /* Go to C code                                 */
        movel  d0, a1                     /* &StkFrm in a1                                */
        movel  sp@(4), a6                 /* &regimage in a6                              */
        
        btstb  IMM(5), a1@(0)             /* Test the super/user state bit in saved SR    */
        beq    SYM_NAME(return_to_user)   /* If bit == 0;  return to user state           */
        bra    SYM_NAME(return_to_super)  /* else;         return to system state         */
FUNC_END(generic_except)

/*
 * Exception return code
 * Call this if returning to supervisor state
 *
 * Calling Sequence:
 *        return_to_super(unsigned short *stkfrm, ex_regs_t *registers)
 */
FUNC_START(return_to_super)
        /*
         * get new stack pointer
         */
        movel   a6@(sp_o), sp

        /*
         * Adjust the stkfrm pointer to point at the end of the struct,
         * so the copy by predecrement below will work.
         */
        addql   IMM(STKFRM_SIZE), a1

        /*
         * copy the stack frame
         */
        movew   a1@-, sp@-       /* copy the SR                       */
        movel   a1@-, sp@-       /* copy the PC                       */

        bra     SYM_NAME(return_to_any)
FUNC_END(return_to_super)

/*
 * Exception return code
 * Call this if returning to user state
 *
 * Calling Sequence:
 *        return_to_user(unsigned short *stkfrm, ex_regs_t *registers)
 */
FUNC_START(return_to_user)
        /*
         * get usp
         */
        movel   a6@(sp_o), a0
        movel   a0, usp              /* set usp                           */

        movel   a1, sp               /* get original superStack pointer   */

        bra     SYM_NAME(return_to_any)
FUNC_END(return_to_user)

/*
 * Exception return code.  Called by return_to_super and return_to_user
 *
 * Calling Sequence:
 *        return_to_any
 *              a1 = stkfrm
 *              a6 = registers
 */
FUNC_START(return_to_any)
        /*
         * It is critical that we don't touch the stack pointer
         * now, because it is setup as GDB requested and we don't
         * know the state of the data on either side of the SP
         */
        
        /*
         * Restore from the register image
         */
        moveml  a6@+, d0-d7/a0-a5
        moveal  a6@, a6

        rte                        /* return from exception             */
FUNC_END(return_to_any)

/*
 *  Provide a stack for use by the exception handler.
 *  Note that this stack is in .bss, so it will be below
 *  the normal user stack which is placed in high memory.
 *  We use this fact to detect whether or not we need to
 *  switch stacks before saving regs.
 */
	.bss
	.align 4
	.rept	16384
	.byte	0
	.endr
SYM_NAME(_ex_stack):
	.long	0
