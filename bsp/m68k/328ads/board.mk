#
# 328ads/board.mk -- Makefile fragment for Motorola Dragonball(68328) ADS board
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
BOARD_VPATH=@top_srcdir@/@archdir@/328ads:@top_srcdir@/@archdir@/328ads/include
BOARD_INCLUDES=-I@top_srcdir@/@archdir@/328ads/include
BOARD_HEADERS=@top_srcdir@/@archdir@/328ads/include/bsp/328ads.h
BOARD_LIBBSP=lib328ads.a
BOARD_OBJS=init_328ads.o 328ads.o 328ads-start.o 328ads-bsp-crt0.o
BOARD_STARTUP=
BOARD_CFLAGS=
BOARD_LDFLAGS=-L../../newlib
BOARD_DEFINES=

BIN_FORMAT=@host_os@

BOARD_SPECS=328ads-rom.specs
BOARD_RAM_SPECS=328ads.specs
BOARD_LDSCRIPT=328ads.ld

BOARD_SPECS_SRC=@top_srcdir@/@archdir@/328ads/$(BOARD_SPECS).$(BIN_FORMAT)
BOARD_RAM_SPECS_SRC=@top_srcdir@/@archdir@/328ads/$(BOARD_RAM_SPECS).$(BIN_FORMAT)
BOARD_LDSCRIPT_SRC=@top_srcdir@/@archdir@/328ads/$(BOARD_LDSCRIPT)

BOARD_DEPENDS=

ROM_STARTUP=328ads-start.o
RAM_STARTUP=328ads-bsp-crt0.o

#
# Indicate that 'all' is the default target
#
all: $(BOARD_SPECS) $(BOARD_RAM_SPECS)

#
# Board specific installation stuff
#
board-install:

#
# Dependencies for 328ads objects.
#
init_328ads.o: init_328ads.S bsp/cpu.h bsp/328ads.h $(ALL_DEPENDS)
328ads.o: 328ads.c bsp/cpu.h bsp/bsp.h bsp/328ads.h bsp_if.h gdb.h queue.h $(ALL_DEPENDS)
328ads-start.o: start.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

328ads-bsp-crt0.o: bsp-crt0.S bsp/cpu.h bsp/bsp.h bsp_if.h bsp_start.h $(ALL_DEPENDS)
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<
