#
# mbx/board.mk -- Makefile fragment for MBX board.
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
BOARD_VPATH=@top_srcdir@/@archdir@/mbx
BOARD_INCLUDES=
BOARD_HEADERS=
BOARD_LIBBSP=libmbx.a
BOARD_OBJS=init_mbx.o mbx.o cache8xx.o
BOARD_STARTUP=mbx-start.o
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../nof/newlib
BOARD_DEFINES=

BOARD_SPECS=mbx_rom.specs
BOARD_RAM_SPECS=mbx.specs
BOARD_LDSCRIPT=bsp.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/mbx/$(BOARD_SPECS)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/mbx/$(BOARD_RAM_SPECS)
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
# Dependencies for MBX objects.
#
mbx-start.o: start.S bsp/cpu.h bsp/bsp.h bsp/ppc8xx.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

init_mbx.o: init_mbx.S bsp/cpu.h bsp/ppc8xx.h $(ALL_DEPENDS)
mbx.o: mbx.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp/ppc8xx.h $(ALL_DEPENDS)

