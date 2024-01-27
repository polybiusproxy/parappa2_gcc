#
# eval1/board.mk -- Makefile fragment for MN103002 eval board.
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
BOARD_VPATH=@top_srcdir@/@archdir@/eval1
BOARD_INCLUDES=
BOARD_HEADERS=
BOARD_LIBBSP=libeval1.a
BOARD_OBJS=init_eval1.o eval1.o
BOARD_STARTUP=eval1-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BOARD_SPECS=eval1-rom.specs
BOARD_RAM_SPECS=eval1.specs
BOARD_LDSCRIPT=eval1.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/eval1/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/eval1/$(BOARD_RAM_SPECS)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/eval1/$(BOARD_LDSCRIPT)

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
eval1-start.o: start.S bsp/cpu.h gdb.h gdb-cpu.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_eval1.o: init_eval1.S bsp/cpu.h $(ALL_DEPENDS)
eval1.o: eval1.c bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)

