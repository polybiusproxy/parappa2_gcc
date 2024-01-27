#
# pid/board.mk -- Makefile fragment for ARM PID Eval Board
#
# Copyright (c) 1998, 1999 Cygnus Solutions
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.
#
BOARD_VPATH=@top_srcdir@/@archdir@/pid:@top_srcdir@/@archdir@/pid/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/pid/include
BOARD_HEADERS=@top_srcdir@/@archdir@/pid/include/bsp/armpid.h
BOARD_LIBBSP=libarmpid.a
BOARD_OBJS=init_armpid.o armpid.o armpid-start.o armpid-c_start.o armpid-bsp-crt0.o armpid-c_bsp-crt0.o
BOARD_STARTUP=
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BIN_FORMAT=@host_os@

BOARD_SPECS=armpid-rom.specs
BOARD_RAM_SPECS=armpid.specs
BOARD_LDSCRIPT=armpid.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/pid/$(BOARD_SPECS).$(BIN_FORMAT)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/pid/$(BOARD_RAM_SPECS).$(BIN_FORMAT)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/pid/$(BOARD_LDSCRIPT)

BOARD_DEPENDS=

ROM_STARTUP=armpid-start.o armpid-c_start.o
RAM_STARTUP=armpid-bsp-crt0.o armpid-c_bsp-crt0.o

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for armpid objects.
#
init_armpid.o: init_armpid.S bsp/cpu.h bsp/armpid.h $(ALL_DEPENDS)
armpid.o: armpid.c bsp/cpu.h bsp/bsp.h bsp/armpid.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)
armpid-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

armpid-c_start.o: c_start.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

armpid-bsp-crt0.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

armpid-c_bsp-crt0.o: c_bsp-crt0.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<
