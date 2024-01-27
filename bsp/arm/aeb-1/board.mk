#
# aeb-1/board.mk -- Makefile fragment for ARM AEB-1 Eval Board
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
BOARD_VPATH=@top_srcdir@/@archdir@/aeb-1:@top_srcdir@/@archdir@/aeb-1/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/aeb-1/include
BOARD_HEADERS=@top_srcdir@/@archdir@/aeb-1/include/bsp/aeb-1.h
BOARD_LIBBSP=libaeb-1.a
BOARD_OBJS=init_aeb-1.o aeb-1.o aeb-1-start.o aeb-1-c_start.o aeb-1-bsp-crt0.o aeb-1-c_bsp-crt0.o
BOARD_STARTUP=
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BIN_FORMAT=@host_os@

BOARD_SPECS=aeb-1-rom.specs
BOARD_RAM_SPECS=aeb-1.specs
BOARD_LDSCRIPT=aeb-1.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/aeb-1/$(BOARD_SPECS).$(BIN_FORMAT)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/aeb-1/$(BOARD_RAM_SPECS).$(BIN_FORMAT)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/aeb-1/$(BOARD_LDSCRIPT)

BOARD_DEPENDS=bsp/lh77790a.h

ROM_STARTUP=aeb-1-start.o aeb-1-c_start.o
RAM_STARTUP=aeb-1-bsp-crt0.o aeb-1-c_bsp-crt0.o

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for aeb-1 objects.
#
init_aeb-1.o: init_aeb-1.S bsp/cpu.h bsp/aeb-1.h $(ALL_DEPENDS) bsp/lh77790a.h
aeb-1.o: aeb-1.c bsp/cpu.h bsp/bsp.h bsp/aeb-1.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS) bsp/lh77790a.h

aeb-1-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

aeb-1-c_start.o: c_start.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

aeb-1-bsp-crt0.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

aeb-1-c_bsp-crt0.o: c_bsp-crt0.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<
