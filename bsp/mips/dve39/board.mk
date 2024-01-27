#
# dve39/board.mk -- Makefile fragment for Densan tx3901 board
#
# Copyright (c) 1998 Cygnus Solutions
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
BOARD_VPATH=@top_srcdir@/@archdir@/dve39:@top_srcdir@/@archdir@/dve39/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/dve39/include
BOARD_HEADERS=@top_srcdir@/@archdir@/dve39/include/bsp/dve39.h
BOARD_LIBBSP=libdve39.a
BOARD_OBJS=init_dve.o dve.o i82596.o $(COMMON_TCP_OBJS)
BOARD_STARTUP=dve-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BOARD_SPECS=dve-rom.specs
BOARD_RAM_SPECS=dve.specs
BOARD_LDSCRIPT=bsp3k.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/dve39/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/dve39/$(BOARD_RAM_SPECS)
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
# Dependencies for CMA101 objects.
#
dve-start.o: start.S bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_dve.o: init_dve.S bsp/cpu.h bsp/dve39.h $(ALL_DEPENDS)
dve.o: dve.c bsp/cpu.h bsp/bsp.h bsp/dve39.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)
i82596.o: i82596.c bsp/bsp.h bsp/cpu.h i82596.h $(ALL_DEPENDS)

