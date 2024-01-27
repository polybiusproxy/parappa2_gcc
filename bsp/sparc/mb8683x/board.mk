#
# mb8683x/board.mk -- Makefile fragment for Fujitsu MB8683x Eval Board
#
# Copyright (c) 1999 Cygnus Solutions
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
BOARD_VPATH=@top_srcdir@/@archdir@/mb8683x
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/mb8683x
BOARD_HEADERS=
BOARD_LIBBSP=libmb86x.a
BOARD_OBJS=init_86x.o mb.o cache-sparcl.o mb86964.o $(COMMON_TCP_OBJS)
BOARD_STARTUP=mb86x-vec.o mb86x-start.o
BOARD_CFLAGS=-DBOARD_MB8683X
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=-DGDB_THREAD_SUPPORT

RAM_STARTUP=crt0-86x.o

BOARD_SPECS=mb86x-rom.specs
BOARD_RAM_SPECS=mb86x.specs
BOARD_LDSCRIPT=bsp86x.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/mb8683x/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/mb8683x/$(BOARD_RAM_SPECS)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/$(BOARD_LDSCRIPT)

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for board objects.
#
mb86x-start.o: start.S bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

mb86x-vec.o: vectors.S bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

crt0-86x.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_86x.o: init_86x.S bsp/cpu.h $(ALL_DEPENDS)
mb.o: mb.c bsp/cpu.h bsp/bsp.h bsp_if.h gdb.h queue.h \
	mb86940.h mb8683x.h $(ALL_DEPENDS)
mb86964.o: mb86964.c mb86964.h bsp/bsp.h bsp/cpu.h bsp_if.h \
	net.h $(ALL_DEPENDS)
