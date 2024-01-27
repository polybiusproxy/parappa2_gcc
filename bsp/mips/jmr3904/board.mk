#
# jmr3904/board.mk -- Makefile fragment for JMR tx3904 PCI board
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
BOARD_VPATH=@top_srcdir@/@archdir@/jmr3904:@top_srcdir@/@archdir@/jmr3904/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/jmr3904/include
BOARD_HEADERS=
BOARD_LIBBSP=libjmr3904.a
BOARD_OBJS=init_jmr.o jmr.o
BOARD_STARTUP=jmr3904-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BOARD_SPECS=jmr3904-rom.specs
BOARD_RAM_SPECS=jmr3904.specs
BOARD_LDSCRIPT=bsp3k.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/jmr3904/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/jmr3904/$(BOARD_RAM_SPECS)
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
jmr3904-start.o: start.S bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_jmr.o: init_jmr.S bsp/cpu.h $(ALL_DEPENDS)
jmr.o: jmr.c bsp/cpu.h bsp/bsp.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)


