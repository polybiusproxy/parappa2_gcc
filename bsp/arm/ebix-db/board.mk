#
# ebix-db/board.mk -- Makefile fragment for the
#                     Intel StrongArm EBIX-DB Eval Board
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
BOARD_VPATH=@top_srcdir@/@archdir@/ebix-db:@top_srcdir@/@archdir@/ebix-db/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/ebix-db/include
BOARD_HEADERS=@top_srcdir@/@archdir@/ebix-db/include/bsp/ebix.h
BOARD_LIBBSP=libebix.a
BOARD_OBJS=init_ebix.o ebix.o sa-110.o ebix-start.o ebix-c_start.o ebix-bsp-crt0.o ebix-c_bsp-crt0.o
BOARD_STARTUP=
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BIN_FORMAT=@host_os@

BOARD_SPECS=ebix-rom.specs
BOARD_RAM_SPECS=ebix.specs
BOARD_LDSCRIPT=ebix.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/ebix-db/$(BOARD_SPECS).$(BIN_FORMAT)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/ebix-db/$(BOARD_RAM_SPECS).$(BIN_FORMAT)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/ebix-db/$(BOARD_LDSCRIPT)

BOARD_DEPENDS=bsp/sa-110.h

ROM_STARTUP=ebix-start.o ebix-c_start.o
RAM_STARTUP=ebix-bsp-crt0.o ebix-c_bsp-crt0.o

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for ebix objects.
#
init_ebix.o: sa-110.o init_ebix.S bsp/cpu.h bsp/ebix.h $(ALL_DEPENDS)
ebix.o: sa-110.o ebix.c bsp/cpu.h bsp/bsp.h bsp/ebix.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)
sa-110.o: sa-110.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h bsp/ebix.h $(ALL_DEPENDS)
ebix-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebix-c_start.o: c_start.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebix-bsp-crt0.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

ebix-c_bsp-crt0.o: c_bsp-crt0.c bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<
