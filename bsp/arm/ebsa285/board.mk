#
# ebsa285/board.mk -- Makefile fragment for the
#                     Intel StrongArm EBSA285 Eval Board
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
BOARD_VPATH=@top_srcdir@/@archdir@/ebsa285:@top_srcdir@/@archdir@/ebsa285/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/ebsa285/include
BOARD_HEADERS=@top_srcdir@/@archdir@/ebsa285/include/bsp/ebsa.h
BOARD_LIBBSP=libebsa.a
BOARD_OBJS=init_ebsa.o ebsa.o sa-110.o ebsa-start.o ebsa-c_start.o ebsa-bsp-crt0.o ebsa-c_bsp-crt0.o
BOARD_STARTUP=
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BIN_FORMAT=@host_os@

BOARD_SPECS=ebsa-rom.specs
BOARD_RAM_SPECS=ebsa.specs
BOARD_LDSCRIPT=ebsa.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/ebsa285/$(BOARD_SPECS).$(BIN_FORMAT)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/ebsa285/$(BOARD_RAM_SPECS).$(BIN_FORMAT)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/ebsa285/$(BOARD_LDSCRIPT)

BOARD_DEPENDS=bsp/sa-110.h

ROM_STARTUP=ebsa-start.o ebsa-c_start.o
RAM_STARTUP=ebsa-bsp-crt0.o ebsa-c_bsp-crt0.o

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for ebsa objects.
#
init_ebsa.o: sa-110.o init_ebsa.S bsp/cpu.h bsp/ebsa.h $(ALL_DEPENDS)
ebsa.o: sa-110.o ebsa.c bsp/cpu.h bsp/bsp.h bsp/ebsa.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)
sa-110.o: sa-110.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h bsp/ebsa.h $(ALL_DEPENDS)
ebsa-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebsa-c_start.o: c_start.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebsa-bsp-crt0.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebsa-c_bsp-crt0.o: c_bsp-crt0.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<
