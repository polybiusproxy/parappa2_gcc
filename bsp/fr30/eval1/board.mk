#
# eval1/board.mk -- Makefile fragment for FR30 Eval board.
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
BOARD_DIR=eval1
BOARD_VPATH=@top_srcdir@/@archdir@/eval1:@top_srcdir@/@archdir@/$(BOARD_DIR)/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/$(BOARD_DIR)/include
BOARD_HEADERS=
BOARD_LIBBSP=libeval1.a
BOARD_OBJS=init_eval.o eval.o mb86964.o $(COMMON_TCP_OBJS)
BOARD_STARTUP=eval1-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib/libc
BOARD_DEFINES=

BOARD_SPECS=eval1-rom.specs
BOARD_RAM_SPECS=eval1.specs
BOARD_LDSCRIPT=fr30.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/$(BOARD_DIR)/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/$(BOARD_DIR)/$(BOARD_RAM_SPECS)
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
eval1-start.o: start.S bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_eval.o: init_eval.S bsp/cpu.h $(ALL_DEPENDS)
eval.o: eval.c bsp/cpu.h bsp/bsp.h bsp_if.h queue.h $(ALL_DEPENDS)
mb86964.o: mb86964.c mb86964.h bsp/bsp.h bsp/cpu.h bsp_if.h net.h $(ALL_DEPENDS)


