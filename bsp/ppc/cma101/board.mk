#
# cma101/board.mk -- Makefile fragment for Cogent CMA101 board.
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
BOARD_VPATH=@top_srcdir@/@archdir@/cma101
BOARD_INCLUDES=
BOARD_HEADERS=
BOARD_LIBBSP=libcma.a
BOARD_OBJS=init_cma286.o cma101.o cache8xx.o mb86964.o $(COMMON_TCP_OBJS)
BOARD_STARTUP=cma-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../nof/newlib
BOARD_DEFINES=

BOARD_SPECS=cma_rom.specs
BOARD_RAM_SPECS=cma.specs 
BOARD_LDSCRIPT=bsp.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/cma101/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/cma101/$(BOARD_RAM_SPECS)
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
cma-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_cma286.o: init_cma286.S bsp/cpu.h bsp/ppc8xx.h bsp_if.h $(ALL_DEPENDS)
cma101.o: cma101.c bsp/cpu.h bsp/bsp.h bsp/ppc8xx.h bsp_if.h $(ALL_DEPENDS)
mb86964.o: mb86964.c mb86964.h bsp/bsp.h bsp/cpu.h bsp_if.h net.h $(ALL_DEPENDS)
