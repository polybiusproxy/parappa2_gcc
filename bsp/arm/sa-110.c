/*
 * sa-110.c -- Intel StrongArm SA-110 specific support for Cygnus BSP
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

#ifdef CPU_SA110

#include <stdio.h>
#include <bsp/bsp.h>
#include <bsp/cpu.h>
#include "bsp_if.h"
#include "queue.h"
#include BOARD_HEADER

/*
 * Enable/Disable separate debug functions
 */
#define DEBUG_UART_CODE      0
#define DEBUG_UART_FUNCTIONS 0
#define DEBUG_LOCAL_ECHO     0

static void	sa110_ictrl_init(const struct bsp_irq_controller *ic);
static int	sa110_ictrl_disable(const struct bsp_irq_controller *ic, int irq_nr);
static void	sa110_ictrl_enable(const struct bsp_irq_controller *ic, int irq_nr);

static bsp_vec_t *sa110_irq_vec_tbl[NUM_SA110_INTERRUPTS];

static const struct bsp_irq_controller sa110_irq_controller = {
    SA110_IRQ_MIN, SA110_IRQ_MAX,
    &sa110_irq_vec_tbl[0],
    sa110_ictrl_init,
    sa110_ictrl_disable,
    sa110_ictrl_enable
};


/*
 * Dispatch code for sa110 interrupts. Called by exception dispatch code.
 */
static int
sa110_irq_dispatch(int ex_nr, ex_regs_t *regs)
{
    bsp_vec_t *vec = NULL;
    int done = 0;
    int irq = 0;

#if DEBUG_UART_CODE
    bsp_printf("sa110_irq_dispatch(%d, 0x%08lx)\n", ex_nr, regs);
    BSP_ASSERT((ex_nr == BSP_CORE_EXC_IRQ) || (ex_nr == BSP_CORE_EXC_FIQ));

    bsp_printf("*SA110_IRQCONT_IRQSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQENABLE = 0x%04lx\n", *SA110_IRQCONT_IRQENABLE);
    bsp_printf("*SA110_IRQCONT_FIQSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQENABLE = 0x%04lx\n", *SA110_IRQCONT_FIQENABLE);
#endif /* DEBUG_UART_CODE */

    if (ex_nr == BSP_CORE_EXC_FIQ)
    {
        /*
         * Some FIQ occurred.
         * Figure out which one.
         */
        unsigned long int_status = *SA110_IRQCONT_FIQSTATUS;

        /*
         * We will prioritize them by the low-order bit that is
         * set in the status register
         */
        while (((int_status & 0x1) == 0x0) && (irq < NUM_SA110_INTERRUPTS))
        {
            irq++;
            int_status >>= 1;
        }
    } else {
        /*
         * Some IRQ occurred.
         * Figure out which one.
         */
        unsigned long int_status = *SA110_IRQCONT_IRQSTATUS;

        /*
         * We will prioritize them by the low-order bit that is
         * set in the status register
         */
        while (((int_status & 0x1) == 0x0) && (irq < NUM_SA110_INTERRUPTS))
        {
            irq++;
            int_status >>= 1;
        }
    }

#if DEBUG_UART_CODE
    bsp_printf("irq == %d\n", irq);
#endif /* DEBUG_UART_CODE */

    if (irq < NUM_SA110_INTERRUPTS)
        vec = sa110_irq_vec_tbl[irq];

    while (!done && vec && vec->handler != NULL) {
#if DEBUG_UART_CODE
        bsp_printf("vec->handler = 0x%08lx\n", vec->handler);
        bsp_printf("vec->next = 0x%08lx\n", vec->next);
#endif /* DEBUG_UART_CODE */
        done = vec->handler(ex_nr, regs);
        vec = vec->next;
    }

    return done;
}


/*
 *  Initialize SA110 interrupt controller.
 */
static void
sa110_ictrl_init(const struct bsp_irq_controller *ic)
{
    static bsp_vec_t irq_vec, fiq_vec;
    int i;

#if DEBUG_UART_CODE
    bsp_printf("sa110_ictrl_init(0x%08lx)\n", ic);

    bsp_printf("*SA110_IRQCONT_IRQSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQENABLE = 0x%04lx\n", *SA110_IRQCONT_IRQENABLE);
    bsp_printf("*SA110_IRQCONT_FIQSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQENABLE = 0x%04lx\n", *SA110_IRQCONT_FIQENABLE);
#endif /* DEBUG_UART_CODE */

    /*
     * Clear all pending interrupts
     */
    *SA110_IRQCONT_IRQENABLECLEAR = ~0;
    *SA110_IRQCONT_FIQENABLECLEAR = ~0;

    for (i = ic->first; i < ic->last; i++)
    {
        ic->vec_list[i - ic->first] = NULL;
    }

    /*
     * Setup exception handlers for the ARM exceptions
     * corresponding to IRQ's and FIQ's
     */
    irq_vec.handler = (void *)sa110_irq_dispatch;
    irq_vec.next = NULL;
    bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_IRQ, BSP_VEC_CHAIN_FIRST, &irq_vec);

    fiq_vec.handler = (void *)sa110_irq_dispatch;
    fiq_vec.next = NULL;
    bsp_install_vec(BSP_VEC_EXCEPTION, BSP_CORE_EXC_FIQ, BSP_VEC_CHAIN_FIRST, &fiq_vec);
}


/*
 *  Disable SA110 interrupts.
 */
static int
sa110_ictrl_disable(const struct bsp_irq_controller *ic, int irq_nr)
{
#if DEBUG_UART_CODE
    BSP_ASSERT((irq_nr >= 0) && (irq_nr <= NUM_SA110_INTERRUPTS));
    bsp_printf("sa110_ictrl_disable(0x%08lx, %d)\n", ic, irq_nr);

    bsp_printf("*SA110_IRQCONT_IRQSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQENABLE = 0x%04lx\n", *SA110_IRQCONT_IRQENABLE);
    bsp_printf("*SA110_IRQCONT_FIQSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQENABLE = 0x%04lx\n", *SA110_IRQCONT_FIQENABLE);
#endif /* DEBUG_UART_CODE */

    *SA110_IRQCONT_IRQENABLECLEAR = SA110_IRQ_INTSRC_MASK(irq_nr);
    return 0;
}


/*
 *  Enable SA110 interrupts.
 */
static void
sa110_ictrl_enable(const struct bsp_irq_controller *ic, int irq_nr)
{
#if DEBUG_UART_CODE
    bsp_printf("sa110_ictrl_enable(0x%08lx, %d)\n", ic, irq_nr);
    BSP_ASSERT((irq_nr >= 0) && (irq_nr <= NUM_SA110_INTERRUPTS));

    bsp_printf("*SA110_IRQCONT_IRQSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_IRQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_IRQENABLE = 0x%04lx\n", *SA110_IRQCONT_IRQENABLE);
    bsp_printf("*SA110_IRQCONT_FIQSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQRAWSTATUS = 0x%04lx\n", *SA110_IRQCONT_FIQRAWSTATUS);
    bsp_printf("*SA110_IRQCONT_FIQENABLE = 0x%04lx\n", *SA110_IRQCONT_FIQENABLE);
#endif /* DEBUG_UART_CODE */

    *SA110_IRQCONT_IRQENABLESET = SA110_IRQ_INTSRC_MASK(irq_nr);
}


/*
 * SA110 specific routine to 'install' interrupt controllers.
 */
void
_bsp_install_sa110_irq_controllers(void)
{
#if DEBUG_UART_CODE
    bsp_printf("_bsp_install_sa110_irq_controllers()\n");
#endif /* DEBUG_UART_CODE */

    _bsp_install_irq_controller(&sa110_irq_controller);
}

/*
 * Function prototypes
 */
static void uart_write   (void*, const char*, int);
static int  uart_read    (void*, char*, int);
static void uart_putchar (void*, char);
static int  uart_getchar (void*);
static int  uart_control (void*, int, ...);
#if DEBUG_UART_FUNCTIONS
void uart_putnibble(void*, unsigned char);
void uart_putbyte(void*, unsigned char);
void uart_putword(void*, unsigned short);
void uart_putlong(void*, unsigned long);
void uart_putstring(void *, char *);
void uart_putcharacter(void *, char);
#endif /* DEBUG_UART_FUNCTIONS */

/*
 * Setup the bsp_comm_channel data structure
 */
#define BSP_NUM_COMMS        1
struct bsp_comm_channel _bsp_comm_list[BSP_NUM_COMMS] = 
{
    {
	{ "Serial Port A", BSP_COMM_SERIAL, BSP_PROTO_NONE },
	{ (void*)UART_BASE_0, uart_write, uart_read,
	  uart_putchar, uart_getchar, uart_control }
    }
};
int _bsp_num_comms = sizeof(_bsp_comm_list)/sizeof(_bsp_comm_list[0]);

/*
 * Baud rate selection stuff
 */
static int uart_baud[BSP_NUM_COMMS] = {-1};
struct _baud {
    int baud;
    unsigned short divisor_high, divisor_low;
};

const static struct _baud bauds[] = {
    /* { 50, ,},  Too Small */            /* 16524 = 0x408C */
    /* { 75, ,},  Too Small */            /* 10416 = 0x28B0 */
    /* {110, ,},  Too Small */            /* 7101  = 0x1BBD */
    /* {134, ,},  Too Small */            /* 5808  = 0x16B0 */
    /* {150, ,},  Too Small */            /* 5207  = 0x1457 */
    {   300, 0xA, 0x2B},                  /* 2603  = 0x0A2B */
    {   600, 0x5, 0x15},                  /* 1301  = 0x0515 */
    {  1200, 0x2, 0x8A},                  /* 650   = 0x028A */
    {  1800, 0x1, 0xB1},                  /* 433   = 0x01B1 */
    {  2000, 0x1, 0x86},                  /* 390   = 0x0186 */
    {  2400, 0x1, 0x45},                  /* 325   = 0x0145 */
    {  3600, 0x0, 0xD8},                  /* 216   = 0x00D8 */
    {  4800, 0x0, 0xA2},                  /* 162   = 0x00A2 */
    {  7200, 0x0, 0x6C},                  /* 108   = 0x006C */
    {  9600, 0x0, 0x50},                  /* 80    = 0x0050 */
    { 19200, 0x0, 0x28},                  /* 40    = 0x0028 */
    { 38400, 0x0, 0x13},                  /* 19    = 0x0013 */
    { 56000, 0x0, 0x0D},                  /* 13    = 0x000D */
    {128000, 0x0, 0x05},                  /* 5     = 0x0005 */
};

/*
 * UART queuing stuff
 */
#define UART_BUFSIZE 256
static bsp_queue_t uart_queue[BSP_NUM_COMMS];
static char uart_buffer[BSP_NUM_COMMS][UART_BUFSIZE];

/*
 * The currently selected debug port
 */
static volatile unsigned char *uart_dbg_base = NULL;

/*
 * Array of memory region descriptors. We just list RAM and FLASH.
 */
struct bsp_mem_info _bsp_memory_list[] = BOARD_MEMORY_LIST;

/*
 * Number of memory region descriptors.
 */
int _bsp_num_mem_regions = sizeof(_bsp_memory_list)/sizeof(_bsp_memory_list[0]);

static void
uart_irq_enable(void *base)
{
    BSP_ASSERT(base == UART_BASE_0);

    *SA110_IRQCONT_IRQENABLESET |= (SA110_IRQ_CONSOLE_RX | SA110_IRQ_CONSOLE_TX);
}


static void
uart_irq_disable(void *base)
{
    BSP_ASSERT(base == UART_BASE_0);

    *SA110_IRQCONT_IRQENABLECLEAR |= (SA110_IRQ_CONSOLE_RX | SA110_IRQ_CONSOLE_TX);
}

static void uart_putchar(void *base, char c)
{
    BSP_ASSERT(base == UART_BASE_0);

    while ((*SA110_UART_FLAG_REGISTER & SA110_TX_FIFO_STATUS_MASK) == SA110_TX_FIFO_BUSY)
        ;
    *SA110_UART_DATA_REGISTER = c;
}


static void
uart_write(void *base, const char *buf, int len)
{
    BSP_ASSERT(base == UART_BASE_0);

    while (len-- > 0)
        uart_putchar(base, *buf++);
}

static int
uart_rcvchar(void *base)
{
    BSP_ASSERT(base == UART_BASE_0);

    while ((*SA110_UART_FLAG_REGISTER & SA110_RX_FIFO_STATUS_MASK) == SA110_RX_FIFO_EMPTY)
        ;
    return(*SA110_UART_DATA_REGISTER & 0xFFFF);
}

static int
uart_getchar(void *base)
{
    int c;

    BSP_ASSERT(base == UART_BASE_0);

    uart_irq_disable(base);
    if ((c = _bsp_dequeue(&uart_queue[0])) < 0)
        c = uart_rcvchar(base);
    uart_irq_enable(base);

#if DEBUG_LOCAL_ECHO
    uart_putchar(base, c);
#endif
    return c;
}

static int
uart_read(void *base, char *buf, int len)
{
    int ch;

    BSP_ASSERT(base == UART_BASE_0);

    if (len <= 0)
        return 0;

    ch = uart_getchar(base);
    *buf = ch;
    return 1;
}

static int
uart_irq_handler(int irq_nr, void *regs)
{
    unsigned char ch;
    int return_code = 0;
    unsigned long intstatus = *SA110_IRQCONT_IRQSTATUS;

#if DEBUG_UART_CODE
    bsp_printf("In uart_irq_handler(%d, 0x%08lx)\n", irq_nr, regs);
    _bsp_dump_regs(regs);
#endif /* DEBUG_UART_CODE */

    if (intstatus & SA110_IRQ_INTSRC_MASK(SA110_IRQ_CONSOLE_RX))
    {
        BSP_ASSERT((void*)uart_dbg_base == (void*)UART_BASE_0);

        ch = *SA110_UART_DATA_REGISTER & 0xFFFF;

        /*
         * If we read a Ctrl-C then we need to let
         * GDB handle it.  Return 0 to have the GDB
         * exception processor run.
         */
        if (ch == '\003')
        {
            return_code = 0;
        } else {
            _bsp_enqueue(&uart_queue[0], ch);
#if DEBUG_LOCAL_ECHO
            uart_putchar(uart_dbg_base, ch);
#endif
            return_code = 1;
        }
    }
  
    return return_code;
}

static int
uart_setbaud(void *base, int bps)
{
    int   i;
    const struct _baud *bp;

    BSP_ASSERT(base == UART_BASE_0);
    if (uart_baud[0] != bps)
    {
        for (i = 0, bp = bauds; i < (sizeof(bauds)/sizeof(bauds[0])); i++, bp++)
        {
            if (bp->baud == bps)
            {
                /*
                 * Setup the UART Baud Rate Divisor
                 *
                 * Note that the ordering of these writes is critical,
                 * and the writes to the H_BAUD_CONTROL and CONTROL_REGISTER
                 * are necessary to force the UART to update its register
                 * contents.
                 */
                *SA110_UART_L_BAUD_CONTROL   = bp->divisor_low;
                *SA110_UART_M_BAUD_CONTROL   = bp->divisor_high;
                *SA110_UART_H_BAUD_CONTROL = SA110_UART_BREAK_DISABLED    |
                                             SA110_UART_PARITY_DISABLED   |
                                             SA110_UART_STOP_BITS_ONE     |
                                             SA110_UART_FIFO_ENABLED      |
                                             SA110_UART_DATA_LENGTH_8_BITS;
                *SA110_UART_CONTROL_REGISTER = SA110_UART_ENABLED | 
                                               SA110_SIR_DISABLED;


                /*
                 * Store the state
                 */
                uart_baud[0] = bps;
                return 0;
            }
        }
    }

    return -1;
}

static int
uart_control(void *base, int func, ...)
{
    int                    i, retval = 0;
    va_list                ap;
    static bsp_vec_t       uart_vec;

    BSP_ASSERT(base == UART_BASE_0);

    va_start (ap, func);

    switch (func) {
    case COMMCTL_SETBAUD:
        i = va_arg(ap, int);
        retval = uart_setbaud((void *)base, i);
        break;

    case COMMCTL_GETBAUD:
        retval = uart_baud[0];
        break;

    case COMMCTL_INSTALL_DBG_ISR:
        if (uart_dbg_base != base) {
            if (uart_dbg_base == NULL) {
                uart_vec.handler = uart_irq_handler;
                uart_vec.next = NULL;
                bsp_install_vec(BSP_VEC_INTERRUPT, SA110_IRQ_CONSOLE_RX,
                                BSP_VEC_REPLACE, &uart_vec);
                bsp_enable_irq(SA110_IRQ_CONSOLE_RX);
                uart_irq_enable(base);
            }
            uart_dbg_base = base;
        }
        break;
	
    case COMMCTL_REMOVE_DBG_ISR:
        uart_irq_disable(base);
        bsp_disable_irq(SA110_IRQ_CONSOLE_RX);
        bsp_remove_vec(BSP_VEC_INTERRUPT, SA110_IRQ_CONSOLE_RX, &uart_vec);
        uart_dbg_base = NULL;
        break;

    default:
        retval = -1;
        break;
    }

    va_end(ap);

    return retval;
}


#if DEBUG_UART_FUNCTIONS
#else
static
#endif /* DEBUG_UART_FUNCTIONS */
void
init_uart(void *base, int bps)
{
    unsigned long dummy;

    BSP_ASSERT(base == UART_BASE_0);

    /*
     * Make sure everything is off
     */
    *SA110_UART_CONTROL_REGISTER = SA110_UART_DISABLED | SA110_SIR_DISABLED;

    /*
     * Read the RXStat to drain the fifo
     */
    dummy = *SA110_UART_RXSTAT;

    /*
     * Set the baud rate - this also turns the uart on.
     */
    uart_setbaud((void *)base, bps);
}

/*
 * Early initialization of comm channels. Must not rely
 * on interrupts, yet. Interrupt operation can be enabled
 * in _bsp_board_init().
 */
void
_bsp_init_sa110_comm(void)
{
    _bsp_queue_init(&uart_queue[0], uart_buffer[0], sizeof(uart_buffer[0]));

    init_uart((void *)UART_BASE_0, 38400);
}

#if DEBUG_UART_FUNCTIONS
/*
 * Print out a single Hex digit
 */
void uart_putnibble(void *base, unsigned char printval)
{
    if (printval <= 9)
        uart_putchar(base, printval + '0');
    else if ((printval >= 0xA) && (printval <= 0xF))
        uart_putchar(base, printval - 0xA + 'A');
    else
        uart_putchar(base, '.');
}

/*
 * Print out a byte value
 */
void uart_putbyte(void *base, unsigned char printval)
{
    uart_putnibble(base, (unsigned char)((printval >> 4)  & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 0)  & 0x0000000F));
}

/*
 * Print out a word value
 */
void uart_putword(void *base, unsigned short printval)
{
    uart_putnibble(base, (unsigned char)((printval >> 12) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 8)  & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 4)  & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 0)  & 0x0000000F));
}

/*
 * Print out a long value
 */
void uart_putlong(void *base, unsigned long printval)
{
    uart_putnibble(base, (unsigned char)((printval >> 28) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 24) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 20) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 16) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 12) & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 8)  & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 4)  & 0x0000000F));
    uart_putnibble(base, (unsigned char)((printval >> 0)  & 0x0000000F));
}


/*
 * Print out a NULL terminated string
 */
void uart_putstring(void *base, char *string)
{
    if (string != NULL)
    {
        while (*string != '\0')
        {
            uart_putchar(base, *string);
            string++;
        }
    }
}

/*
 * Print out a single character
 */
void uart_putcharacter(void *base, char c)
{
    uart_putchar(base, c);
}
#endif /* DEBUG_UART_FUNCTIONS */

#endif /* CPU_SA110 */
