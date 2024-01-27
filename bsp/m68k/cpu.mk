#
# m68k/cpu.mk -- Makefile fragment for M68K family.
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
CPU_VPATH=@top_srcdir@/@archdir@:@top_srcdir@/@archdir@/include
CPU_INCLUDES=-I@top_srcdir@/@archdir@/include
CPU_HEADERS=@top_srcdir@/@archdir@/include/bsp/cpu.h
CPU_OBJS=cpu.o gdb-cpu.o singlestep.o irq-cpu.o trap.o vectors.o \
         generic-mem.o generic-reg.o
CPU_CFLAGS=
CPU_LDFLAGS=
CPU_DEFINES=-DMISSING_SYSCALL_NAMES

CPU_SPECS=
CPU_RAM_SPECS=

CPU_SPECS_SRC=
CPU_RAM_SPECS_SRC=

CPU_DEPENDS=

#
# Indicate that 'all' is the default target
#
all: $(CPU_SPECS) $(CPU_RAM_SPECS)

#
# CPU specific installation stuff
#
cpu-install: board-install

#
# Dependencies for common M68K objects.
#
vectors.o: vectors.S bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
cpu.o: cpu.c bsp/cpu.h bsp/bsp.h bsp_if.h syscall.h gdb.h gdb-cpu.h $(ALL_DEPENDS)
gdb-cpu.o: gdb-cpu.c gdb.h gdb-cpu.h bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
irq-cpu.o: irq-cpu.c bsp/cpu.h bsp/bsp.h bsp_if.h gdb.h gdb-cpu.h $(ALL_DEPENDS)
singlestep.o: singlestep.c gdb.h gdb-cpu.h bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
trap.o: trap.S bsp/cpu.h $(ALL_DEPENDS)

#
# Dependencies for implementation-dependent cpu objects.
#
