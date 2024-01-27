/*
 * cpu.h -- CPU specific header for Cygnus BSP.
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
#ifndef __CPU_H__
#define __CPU_H__ 1

#ifdef __ASSEMBLER__
/* Easy to read register names. */
/* Standard MIPS register names: */
#define zero	$0
#define at	$1
#define v0	$2
#define v1	$3
#define a0	$4
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24
#define t9	$25
#define k0	$26	/* kernel private register 0 */
#define k1	$27	/* kernel private register 1 */
#define gp	$28	/* global data pointer */
#define sp 	$29	/* stack-pointer */
#define fp	$30	/* frame-pointer */
#define ra	$31	/* return address */

#define pc	$pc	/* pc, used on mips16 */

#define fp0	$f0
#define fp1	$f1
#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__
/*
 *  Register numbers. These are assumed to match the
 *  register numbers used by GDB.
 */
enum __regnames {
    REG_ZERO,   REG_AT,     REG_V0,     REG_V1,
    REG_A0,     REG_A1,     REG_A2,     REG_A3,
    REG_T0,     REG_T1,     REG_T2,     REG_T3,
    REG_T4,     REG_T5,     REG_T6,     REG_T7,
    REG_S0,     REG_S1,     REG_S2,     REG_S3,
    REG_S4,     REG_S5,     REG_S6,     REG_S7,
    REG_T8,     REG_T9,     REG_K0,     REG_K1,
    REG_GP,     REG_SP,     REG_S8,     REG_RA,
    REG_SR,     REG_LO,     REG_HI,     REG_BAD,
    REG_CAUSE,  REG_PC,
    REG_F0,     REG_F1,     REG_F2,     REG_F3,
    REG_F4,     REG_F5,     REG_F6,     REG_F7,
    REG_F8,     REG_F9,     REG_F10,    REG_F11,
    REG_F12,    REG_F13,    REG_F14,    REG_F15,
    REG_F16,    REG_F17,    REG_F18,    REG_F19,
    REG_F20,    REG_F21,    REG_F22,    REG_F23,
    REG_F24,    REG_F25,    REG_F26,    REG_F27,
    REG_F28,    REG_F29,    REG_F30,    REG_F31,
    REG_FSR,    REG_FIR,    REG_FP,
#if defined(_R3900)
    REG_CONFIG = 84,    REG_CACHE,  REG_DEBUG,  REG_DEPC,   REG_EPC,
#endif
    REG_LAST
};
#endif


/* Useful memory constants: */
/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG                   0x00000000
#define KSEG0                   0x80000000
#define KSEG1                   0xa0000000
#define KSEG2                   0xc0000000
#define KSEG3                   0xe0000000

/*
 * Returns the kernel segment base of a given address
 */
#define KSEGX(a)                (((unsigned long)(a)) & 0xe0000000)

/*
 * Returns the physical address of a KSEG0/KSEG1 address
 */
#define PHYSADDR(a)		(((unsigned long)(a)) & 0x1fffffff)

/*
 * Map an address to a certain kernel segment
 */
#define KSEG0ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG0))
#define KSEG1ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG1))
#define KSEG2ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG2))
#define KSEG3ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG3))

/*
 * Memory segments (64bit kernel mode addresses)
 */
#define XKUSEG                  0x0000000000000000
#define XKSSEG                  0x4000000000000000
#define XKPHYS                  0x8000000000000000
#define XKSEG                   0xc000000000000000
#define CKSEG0                  0xffffffff80000000
#define CKSEG1                  0xffffffffa0000000
#define CKSSEG                  0xffffffffc0000000
#define CKSEG3                  0xffffffffe0000000

/* Standard Co-Processor 0 register numbers: */
#define C0_INDEX	$0
#define C0_RANDOM	$1
#define C0_ENTRYLO_0	$2
#if defined(_R3900)
#define C0_CONFIG	$3
#else
#define C0_ENTRYLO_1	$3
#endif
#define C0_CONTEXT	$4
#define C0_PAGEMASK	$5
#define C0_WIRED	$6
#if defined(_R3900)
#define C0_CACHE	$7
#endif
#define C0_BADVA	$8
#define C0_COUNT	$9		/* Count Register */
#define C0_ENTRYHI	$10
#define C0_COMPARE	$11
#define C0_STATUS	$12		/* Status Register */
#define C0_CAUSE	$13		/* last exception description */
#define C0_EPC		$14		/* Exception error address */
#define C0_PRID		$15		/* Processor ID */
#if defined(_R3900)
#define C0_DEBUG        $16
#define C0_DEPC         $17
#else
#define C0_CONFIG	$16		/* CPU configuration */
#define C0_LLADDR	$17		/* Load Linked Address */
#endif
#define C0_WATCHLO	$18		/* Watch address low bits */
#define C0_WATCHHI	$19		/* Watch address high bits */
#define C0_XCONTEXT	$20		/* 64-bit addressing context */
#define C0_ECC		$26		/* L2 cache ECC, L1 parity */
#define C0_CACHE_ERR	$27		/* Cache error and status */
#define C0_TAGLO	$28		/* Cache tag register low bits */
#define C0_TAGHI	$29		/* Cache tag register high bits */
#define C0_ERRORPC	$30		/* Error EPC */

/* Standard Status Register bitmasks: */
#define SR_CU0		0x10000000	/* Mark CP1 as usable */
#define SR_CU1		0x20000000	/* Mark CP1 as usable */
#define SR_FR		0x04000000	/* Enable MIPS III FP registers */
#define SR_BEV		0x00400000	/* Controls location of exception vectors */
#define SR_PE		0x00100000	/* Mark soft reset (clear parity error) */

#define SR_KX		0x00000080	/* Kernel extended addressing enabled */
#define SR_SX		0x00000040	/* Supervisor extended addressing enabled */
#define SR_UX		0x00000020	/* User extended addressing enabled */

#define SR_TS		0x00200000	/* TLB Shutdown */

#if defined(_R3900)
#define SR_NMI          0x00100000      /* Non-Maskable Interrupt status bit */
#endif

/* Standard (R4000) cache operations. Taken from "MIPS R4000
   Microprocessor User's Manual" 2nd edition: */

#define CACHE_I		(0)	/* primary instruction */
#define CACHE_D		(1)	/* primary data */
#define CACHE_SI	(2)	/* secondary instruction */
#define CACHE_SD	(3)	/* secondary data (or combined instruction/data) */

#define INDEX_INVALIDATE		(0)	/* also encodes WRITEBACK if CACHE_D or CACHE_SD */
#define INDEX_LOAD_TAG			(1)
#define INDEX_STORE_TAG			(2)
#define CREATE_DIRTY_EXCLUSIVE		(3)	/* CACHE_D and CACHE_SD only */
#define HIT_INVALIDATE			(4)
#define CACHE_FILL			(5)	/* CACHE_I only */
#define HIT_WRITEBACK_INVALIDATE	(5)	/* CACHE_D and CACHE_SD only */
#define HIT_WRITEBACK			(6)	/* CACHE_I, CACHE_D and CACHE_SD only */
#define HIT_SET_VIRTUAL			(7)	/* CACHE_SI and CACHE_SD only */

#define BUILD_CACHE_OP(o,c)		(((o) << 2) | (c))

/* Individual cache operations: */
#define INDEX_INVALIDATE_I		BUILD_CACHE_OP(INDEX_INVALIDATE,CACHE_I)
#define INDEX_WRITEBACK_INVALIDATE_D	BUILD_CACHE_OP(INDEX_INVALIDATE,CACHE_D)
#define INDEX_INVALIDATE_SI             BUILD_CACHE_OP(INDEX_INVALIDATE,CACHE_SI)
#define INDEX_WRITEBACK_INVALIDATE_SD	BUILD_CACHE_OP(INDEX_INVALIDATE,CACHE_SD)

#define INDEX_LOAD_TAG_I		BUILD_CACHE_OP(INDEX_LOAD_TAG,CACHE_I)
#define INDEX_LOAD_TAG_D                BUILD_CACHE_OP(INDEX_LOAD_TAG,CACHE_D)
#define INDEX_LOAD_TAG_SI               BUILD_CACHE_OP(INDEX_LOAD_TAG,CACHE_SI)
#define INDEX_LOAD_TAG_SD               BUILD_CACHE_OP(INDEX_LOAD_TAG,CACHE_SD)

#define INDEX_STORE_TAG_I              	BUILD_CACHE_OP(INDEX_STORE_TAG,CACHE_I)
#define INDEX_STORE_TAG_D               BUILD_CACHE_OP(INDEX_STORE_TAG,CACHE_D)
#define INDEX_STORE_TAG_SI              BUILD_CACHE_OP(INDEX_STORE_TAG,CACHE_SI)
#define INDEX_STORE_TAG_SD              BUILD_CACHE_OP(INDEX_STORE_TAG,CACHE_SD)

#define CREATE_DIRTY_EXCLUSIVE_D        BUILD_CACHE_OP(CREATE_DIRTY_EXCLUSIVE,CACHE_D)
#define CREATE_DIRTY_EXCLUSIVE_SD       BUILD_CACHE_OP(CREATE_DIRTY_EXCLUSIVE,CACHE_SD)

#define HIT_INVALIDATE_I                BUILD_CACHE_OP(HIT_INVALIDATE,CACHE_I)
#define HIT_INVALIDATE_D                BUILD_CACHE_OP(HIT_INVALIDATE,CACHE_D)
#define HIT_INVALIDATE_SI               BUILD_CACHE_OP(HIT_INVALIDATE,CACHE_SI)
#define HIT_INVALIDATE_SD               BUILD_CACHE_OP(HIT_INVALIDATE,CACHE_SD)

#define CACHE_FILL_I                    BUILD_CACHE_OP(CACHE_FILL,CACHE_I)
#define HIT_WRITEBACK_INVALIDATE_D      BUILD_CACHE_OP(HIT_WRITEBACK_INVALIDATE,CACHE_D)
#define HIT_WRITEBACK_INVALIDATE_SD     BUILD_CACHE_OP(HIT_WRITEBACK_INVALIDATE,CACHE_SD)

#define HIT_WRITEBACK_I                 BUILD_CACHE_OP(HIT_WRITEBACK,CACHE_I)
#define HIT_WRITEBACK_D                 BUILD_CACHE_OP(HIT_WRITEBACK,CACHE_D)
#define HIT_WRITEBACK_SD                BUILD_CACHE_OP(HIT_WRITEBACK,CACHE_SD)

#define HIT_SET_VIRTUAL_SI		BUILD_CACHE_OP(HIT_SET_VIRTUAL,CACHE_SI)
#define HIT_SET_VIRTUAL_SD              BUILD_CACHE_OP(HIT_SET_VIRTUAL,CACHE_SD)


/*
 * Some R3904 Internal registers.
 */
#if defined(_R3904)
#define R3904_ISR     (*(unsigned int *)0xffffc000)
#define R3904_IMR     (*(unsigned int *)0xffffc004)
#define R3904_ILRBASE ((unsigned int *)0xffffc010)

#define R3904_COCR    (*(unsigned int *)0xffffe000)
#define R3904_IACK    (*(unsigned char *)0xffffe001)
#define R3904_ICFG    (*(unsigned short *)0xffffe002)
#endif


#ifdef __ASSEMBLER__
/*
 * Macros to glue together two tokens.
 */
#ifdef __STDC__
#define XGLUE(a,b) a##b
#else
#define XGLUE(a,b) a/**/b
#endif

#define GLUE(a,b) XGLUE(a,b)

#ifdef NEED_UNDERSCORE
#define SYM_NAME(name) GLUE(_,name)
#define FUNC_START(name) \
	.type GLUE(_,name),@function; \
	.globl GLUE(_,name); \
	.ent   GLUE(_,name); \
GLUE(_,name):

#define FUNC_END(name) \
	.end GLUE(_,name)
#else
#define SYM_NAME(name) name
#define FUNC_START(name) \
	.type name,@function; \
	.globl name; \
	.ent   name; \
name:

#define FUNC_END(name) \
	.end name
#endif
#endif /* __ASSEMBLER__ */

/*
 *  breakpoint opcode.
 */
#define BREAKPOINT_OPCODE	0x0005000d

/*
 *  inline asm statement to cause breakpoint.
 */
#define BREAKPOINT()	asm volatile (".long 0x0005000d\n")


/*
 * Core Exception vectors.
 */
#define BSP_EXC_NMI         0
#define BSP_EXC_TLB         1
#define BSP_EXC_XTLB	    2
#define BSP_EXC_CACHE	    3
#define BSP_EXC_INT	    4
#define BSP_EXC_TLBMOD	    5
#define BSP_EXC_TLBL	    6
#define BSP_EXC_TLBS	    7
#define BSP_EXC_ADEL	    8
#define BSP_EXC_ADES	    9
#define BSP_EXC_IBE        10
#define BSP_EXC_DBE        11
#define BSP_EXC_SYSCALL    12
#define BSP_EXC_BREAK      13
#define BSP_EXC_ILL        14
#define BSP_EXC_CPU        15
#define BSP_EXC_OV         16
#define BSP_EXC_TRAP       17
#define BSP_EXC_VCEI       18
#define BSP_EXC_FPE        19
#define BSP_EXC_RSV16      20
#define BSP_EXC_RSV17      21
#define BSP_EXC_RSV18      22
#define BSP_EXC_RSV19      23
#define BSP_EXC_RSV20      24
#define BSP_EXC_RSV21      25
#define BSP_EXC_RSV22      26
#define BSP_EXC_WATCH      27
#define BSP_EXC_RSV24      28
#define BSP_EXC_RSV25      29
#define BSP_EXC_RSV26      30
#define BSP_EXC_RSV27      31
#define BSP_EXC_RSV28      32
#define BSP_EXC_RSV29      33
#define BSP_EXC_RSV30      34
#define BSP_EXC_VCED       35

#define BSP_MAX_EXCEPTIONS 36

/*
 *  Interrupts.
 */
#define BSP_IRQ_SW0	0
#define BSP_IRQ_SW1	1
#define BSP_IRQ_HW0	2
#define BSP_IRQ_HW1	3
#define BSP_IRQ_HW2	4
#define BSP_IRQ_HW3	5
#define BSP_IRQ_HW4	6
#define BSP_IRQ_HW5	7

#if defined(_R3904)
#define BSP_IRQ_HW6	8
#define BSP_IRQ_HW7	9
#define BSP_IRQ_DMA3   10
#define BSP_IRQ_DMA2   11
#define BSP_IRQ_DMA1   12
#define BSP_IRQ_DMA0   13
#define BSP_IRQ_SIO0   14
#define BSP_IRQ_SIO1   15
#define BSP_IRQ_TMR0   16
#define BSP_IRQ_TMR1   17
#define BSP_IRQ_TMR2   18
#endif


#ifdef __mips64
#define GPR_SIZE 8
#define FPR_SIZE 8
#else
#define GPR_SIZE 4
#define FPR_SIZE 4
#endif

#define FR_REG0	  0
#define FR_REG1	  (FR_REG0 + GPR_SIZE)
#define FR_REG2	  (FR_REG1 + GPR_SIZE)
#define FR_REG3	  (FR_REG2 + GPR_SIZE)
#define FR_REG4	  (FR_REG3 + GPR_SIZE)
#define FR_REG5	  (FR_REG4 + GPR_SIZE)
#define FR_REG6	  (FR_REG5 + GPR_SIZE)
#define FR_REG7	  (FR_REG6 + GPR_SIZE)
#define FR_REG8	  (FR_REG7 + GPR_SIZE)
#define FR_REG9	  (FR_REG8 + GPR_SIZE)
#define FR_REG10  (FR_REG9 + GPR_SIZE)
#define FR_REG11  (FR_REG10 + GPR_SIZE)
#define FR_REG12  (FR_REG11 + GPR_SIZE)
#define FR_REG13  (FR_REG12 + GPR_SIZE)
#define FR_REG14  (FR_REG13 + GPR_SIZE)
#define FR_REG15  (FR_REG14 + GPR_SIZE)
#define FR_REG16  (FR_REG15 + GPR_SIZE)
#define FR_REG17  (FR_REG16 + GPR_SIZE)
#define FR_REG18  (FR_REG17 + GPR_SIZE)
#define FR_REG19  (FR_REG18 + GPR_SIZE)
#define FR_REG20  (FR_REG19 + GPR_SIZE)
#define FR_REG21  (FR_REG20 + GPR_SIZE)
#define FR_REG22  (FR_REG21 + GPR_SIZE)
#define FR_REG23  (FR_REG22 + GPR_SIZE)
#define FR_REG24  (FR_REG23 + GPR_SIZE)
#define FR_REG25  (FR_REG24 + GPR_SIZE)
#define FR_REG26  (FR_REG25 + GPR_SIZE)
#define FR_REG27  (FR_REG26 + GPR_SIZE)
#define FR_REG28  (FR_REG27 + GPR_SIZE)
#define FR_REG29  (FR_REG28 + GPR_SIZE)
#define FR_REG30  (FR_REG29 + GPR_SIZE)
#define FR_REG31  (FR_REG30 + GPR_SIZE)

#define FR_FREG0  (FR_REG30 + GPR_SIZE)
#define FR_FREG1  (FR_FREG0 + FPR_SIZE)
#define FR_FREG2  (FR_FREG1 + FPR_SIZE)
#define FR_FREG3  (FR_FREG2 + FPR_SIZE)
#define FR_FREG4  (FR_FREG3 + FPR_SIZE)
#define FR_FREG5  (FR_FREG4 + FPR_SIZE)
#define FR_FREG6  (FR_FREG5 + FPR_SIZE)
#define FR_FREG7  (FR_FREG6 + FPR_SIZE)
#define FR_FREG8  (FR_FREG7 + FPR_SIZE)
#define FR_FREG9  (FR_FREG8 + FPR_SIZE)
#define FR_FREG10 (FR_FREG9 + FPR_SIZE)
#define FR_FREG11 (FR_FREG10 + FPR_SIZE)
#define FR_FREG12 (FR_FREG11 + FPR_SIZE)
#define FR_FREG13 (FR_FREG12 + FPR_SIZE)
#define FR_FREG14 (FR_FREG13 + FPR_SIZE)
#define FR_FREG15 (FR_FREG14 + FPR_SIZE)
#define FR_FREG16 (FR_FREG15 + FPR_SIZE)
#define FR_FREG17 (FR_FREG16 + FPR_SIZE)
#define FR_FREG18 (FR_FREG17 + FPR_SIZE)
#define FR_FREG19 (FR_FREG18 + FPR_SIZE)
#define FR_FREG20 (FR_FREG19 + FPR_SIZE)
#define FR_FREG21 (FR_FREG20 + FPR_SIZE)
#define FR_FREG22 (FR_FREG21 + FPR_SIZE)
#define FR_FREG23 (FR_FREG22 + FPR_SIZE)
#define FR_FREG24 (FR_FREG23 + FPR_SIZE)
#define FR_FREG25 (FR_FREG24 + FPR_SIZE)
#define FR_FREG26 (FR_FREG25 + FPR_SIZE)
#define FR_FREG27 (FR_FREG26 + FPR_SIZE)
#define FR_FREG28 (FR_FREG27 + FPR_SIZE)
#define FR_FREG29 (FR_FREG28 + FPR_SIZE)
#define FR_FREG30 (FR_FREG29 + FPR_SIZE)
#define FR_FREG31 (FR_FREG30 + FPR_SIZE)

#ifdef __mips_soft_float
#define FR_EPC	  (FR_REG31 + GPR_SIZE)
#else
#define FR_EPC	  (FR_FREG31 + FPR_SIZE)
#endif

#define FR_BAD	  (FR_EPC    + GPR_SIZE)
#define FR_HI	  (FR_BAD    + GPR_SIZE)
#define FR_LO     (FR_HI     + GPR_SIZE)
#define FR_FSR    (FR_LO     + GPR_SIZE)
#define FR_FIR    (FR_FSR    + 4)
#define FR_SR	  (FR_FIR    + 4)
#define FR_CAUSE  (FR_SR     + 4)

#define EX_STACK_SIZE (FR_CAUSE + 4)

#if defined(_R3900)
#define FR_CONFIG  (FR_CAUSE + 4)
#define FR_CACHE   (FR_CONFIG + 4)
#define FR_DEBUG   (FR_CACHE + 4)
#define FR_DEPC    (FR_DEBUG + 4)
#define FR_XPC     (FR_DEPC  + 4)
#define FR_PAD     (FR_XPC + 4)
#undef  EX_STACK_SIZE
#define EX_STACK_SIZE (FR_PAD + 4)
#endif

#ifndef __ASSEMBLER__
/*
 *  How registers are stored for exceptions.
 */
typedef struct
{
    unsigned long _zero;
    unsigned long _at;
    unsigned long _v0;
    unsigned long _v1;
    unsigned long _a0;
    unsigned long _a1;
    unsigned long _a2;
    unsigned long _a3;
    unsigned long _t0;
    unsigned long _t1;
    unsigned long _t2;
    unsigned long _t3;
    unsigned long _t4;
    unsigned long _t5;
    unsigned long _t6;
    unsigned long _t7;
    unsigned long _s0;
    unsigned long _s1;
    unsigned long _s2;
    unsigned long _s3;
    unsigned long _s4;
    unsigned long _s5;
    unsigned long _s6;
    unsigned long _s7;
    unsigned long _t8;
    unsigned long _t9;
    unsigned long _k0;
    unsigned long _k1;
    unsigned long _gp;
    unsigned long _sp;
    unsigned long _fp;
    unsigned long _ra;
#ifndef __mips_soft_float
#ifdef __mips64
    double	  _fpr[32];
#else
    float	  _fpr[32];
#endif
#endif /* __mips_soft_float */
    unsigned long _epc;
    unsigned long _bad;
    unsigned long _hi;
    unsigned long _lo;
    unsigned int  _fsr;
    unsigned int  _fir;
    unsigned int  _sr;
    unsigned int  _cause;
#if defined(_R3900)
    unsigned int  _config;
    unsigned int  _cache;
    unsigned int  _debug;
    unsigned int  _depc;
    unsigned int  _xpc;
    unsigned int  _pad;
#endif
} ex_regs_t;


extern void __dcache_flush(void *addr, int nbytes);
extern void __icache_flush(void *addr, int nbytes);
extern int  __dcache_disable(void);
extern void __dcache_enable(void);
extern void __icache_disable(void);
extern void __icache_enable(void);

static inline void __set_sr(unsigned val)
{
    asm volatile (
        "mtc0   %0,$12\n"
	: /* no outputs */
	: "r" (val)  );
}

static inline unsigned __get_sr(void)
{
    unsigned retval;

    asm volatile (
        "mfc0   %0,$12\nnop\n"
	: "=r" (retval)
	: /* no inputs */  );

    return retval;
}

static inline unsigned __get_badvaddr(void)
{
    unsigned retval;

    asm volatile (
        "mfc0   %0,$8\nnop\n"
	: "=r" (retval)
	: /* no inputs */  );

    return retval;
}

static inline unsigned __get_epc(void)
{
    unsigned retval;

    asm volatile (
        "mfc0   %0,$14\nnop\n"
	: "=r" (retval)
	: /* no inputs */  );

    return retval;
}

#define __cli()         \
 asm volatile(                    \
	".set noreorder\n\t"      \
	".set noat\n\t"           \
	"mfc0 $1,$12\n\t"         \
	"nop\n\t"                 \
	"ori  $1,$1,1\n\t"        \
	"xori $1,$1,1\n\t"        \
	"mtc0 $1,$12\n\t"         \
	"nop\n\t"                 \
	"nop\n\t"                 \
	"nop\n\t"                 \
	".set at\n\t"             \
	".set reorder"            \
	: /* no outputs */        \
	: /* no inputs */         \
	: )

#define __sti()         \
 asm volatile(                    \
	".set noreorder\n\t"      \
	".set noat\n\t"           \
	"mfc0 $1,$12\n\t"         \
	"nop\n\t"                 \
	"ori  $1,$1,1\n\t"        \
	"mtc0 $1,$12\n\t"         \
	"nop\n\t"                 \
	"nop\n\t"                 \
	"nop\n\t"                 \
	".set at\n\t"             \
	".set reorder"            \
	: /* no outputs */        \
	: /* no inputs */         \
	: )

#if defined(_R3900)
#define __wbflush()   \
  asm volatile(       \
    "nop\n"           \
    "sync\n"          \
    "nop\n"           \
    "1: bc0f 1b\n"    \
    "nop\n"           \
  )
#endif

#endif /* !__ASSEMBLER__ */


#endif /* __CPU_H__ */
