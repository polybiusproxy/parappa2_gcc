/*
 * bsp_if.c -- BSP Interfaces.
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
#include "bsp_if.h"

bsp_shared_t *bsp_shared_data;

/*
 * Install a debug handler.
 * Returns old handler being replaced.
 */
bsp_handler_t
bsp_install_dbg_handler(bsp_handler_t new_handler)
{
    bsp_handler_t old_handler;

    old_handler = *bsp_shared_data->__dbg_vector;
    *bsp_shared_data->__dbg_vector = new_handler;

    bsp_flush_dcache(bsp_shared_data->__dbg_vector, 8);

    return old_handler;
}


/*
 * Sometimes it is desireable to call the debug handler directly. This routine
 * accomplishes that. It is the responsibility of the caller to insure that
 * interrupts are disabled before calling this routine.
 */
void
bsp_invoke_dbg_handler(int exc_nr, void *regs)
{
    (*bsp_shared_data->__dbg_vector)(exc_nr, regs);
}

/*
 * Install a 'kill' handler.
 * Returns old handler being replaced.
 */
bsp_handler_t
bsp_install_kill_handler(bsp_handler_t new_handler)
{
    bsp_handler_t old_handler;

    old_handler = bsp_shared_data->__kill_vector;
    bsp_shared_data->__kill_vector = new_handler;

    bsp_flush_dcache(&bsp_shared_data->__kill_vector, 8);

    return old_handler;
}


void
bsp_console_write(const char *p, int len)
{
    struct bsp_comm_procs *com;

    if ((com = bsp_shared_data->__console_procs) != NULL)
	com->__write(com->ch_data, p, len);
    else
	bsp_debug_write(p, len);
}


static unsigned char __console_ungetc;
static unsigned char __debug_ungetc;

int
bsp_console_read(char *p, int len)
{
    struct bsp_comm_procs *com;

    if (len <= 0)
	return 0;

    if ((com = bsp_shared_data->__console_procs) != NULL) {
	if (__console_ungetc) {
	    *p = __console_ungetc;
	    __console_ungetc = 0;
	    return 1;
	}
	return com->__read(com->ch_data, p, len);
    } else
	return bsp_debug_read(p, len);
}

/*#define PRINTABLE_ONLY*/
#ifdef PRINTABLE_ONLY
#include <ctype.h>
#endif /* PRINTABLE_ONLY */

void
bsp_console_putc(char ch)
{
    struct bsp_comm_procs *com;

#ifdef PRINTABLE_ONLY
    if ((!isprint(ch)) && (!isspace(ch)))
        ch = '.';
#endif /* PRINTABLE_ONLY */

    if ((com = bsp_shared_data->__console_procs) != NULL)
	com->__putc(com->ch_data, ch);
    else
	bsp_debug_putc(ch);
}

int
bsp_console_getc(void)
{
    struct bsp_comm_procs *com;
    int    ch;

    if ((com = bsp_shared_data->__console_procs) != NULL) {
	if (__console_ungetc) {
	    ch = __console_ungetc;
	    __console_ungetc = 0;
	    return ch;
	}
	return com->__getc(com->ch_data);
    } else
	return bsp_debug_getc();
}


void
bsp_console_ungetc(char ch)
{
    if (bsp_shared_data->__console_procs != NULL)
	__console_ungetc = (unsigned char)ch;
    else
	__debug_ungetc = (unsigned char)ch;
}


void
bsp_debug_write(const char *p, int len)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;

    com->__write(com->ch_data, p, len);
}


int
bsp_debug_read(char *p, int len)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;

    if (len <= 0)
	return 0;

    if (__debug_ungetc) {
	*p = __debug_ungetc;
	__debug_ungetc = 0;
	return 1;
    }

    return com->__read(com->ch_data, p, len);
}


void
bsp_debug_putc(char ch)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;

    com->__putc(com->ch_data, ch);
}

int
bsp_debug_getc(void)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;
    int  ch;

    if (__debug_ungetc) {
	ch = __debug_ungetc;
	__debug_ungetc = 0;
    } else
	ch = com->__getc(com->ch_data);

    return ch;
}


void
bsp_debug_ungetc(char ch)
{
    __debug_ungetc = (unsigned char)ch;
}


int
bsp_debug_irq_disable(void)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;

    return com->__control(com->ch_data, COMMCTL_IRQ_DISABLE);
}


void
bsp_debug_irq_enable(void)
{
    struct bsp_comm_procs *com = bsp_shared_data->__debug_procs;

    com->__control(com->ch_data, COMMCTL_IRQ_ENABLE);
}


void
bsp_flush_dcache(void *p, int nbytes)
{
    bsp_shared_data->__flush_dcache(p, nbytes);
}


void
bsp_flush_icache(void *p, int nbytes)
{
    bsp_shared_data->__flush_icache(p, nbytes);
}


void *
bsp_cpu_data(void)
{
  return bsp_shared_data->__cpu_data;
}


void *
bsp_board_data(void)
{
    return bsp_shared_data->__board_data;
}


int
bsp_sysinfo(enum bsp_info_id id, ...)
{
    int     retval;
    va_list ap;

    va_start (ap, id);
    retval = bsp_shared_data->__sysinfo(id, ap);
    va_end(ap);
    return retval;
}


int
bsp_set_debug_comm(int id)
{
    return bsp_shared_data->__set_debug_comm(id);
}

int
bsp_set_console_comm(int id)
{
    return bsp_shared_data->__set_console_comm(id);
}

int
bsp_set_serial_baud(int id, int baud)
{
    return bsp_shared_data->__set_serial_baud(id, baud);
}
