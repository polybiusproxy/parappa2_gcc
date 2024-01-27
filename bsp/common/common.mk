# Common makefile fragment for Cygnus BSP
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
vpath %c $(BOARD_VPATH):$(CPU_VPATH):@top_srcdir@/common:@top_srcdir@/net:@top_srcdir@/include
vpath %S $(BOARD_VPATH):$(CPU_VPATH):@top_srcdir@/common:@top_srcdir@/include
vpath %h $(BOARD_VPATH):$(CPU_VPATH):@top_srcdir@/common:@top_srcdir@/include

AR = @AR@
ARFLAGS=rcs
OBJCOPY = @OBJCOPY@
OBJDUMP = @OBJDUMP@
CFLAGS = -g -O2 -Wall -fno-exceptions
ALL_CFLAGS = $(INCLUDES) $(CPU_CFLAGS) $(BOARD_CFLAGS) $(CFLAGS) -specs=`pwd`/$(BOARD_SPECS)
#
# remove the bss section because COFF assumes there is contents
# here and the section must be written when doing a binary dump
# This causes problems if the base address of this section
# is large.
#
# Ditto for .page_table
#
OBJCOPY_FLAGS=--gap-fill=255 --remove-section=.bss --remove-section=.page_ta
DEFINES = @SYSCALL_UNDERSCORE@ @NEED_UNDERSCORE@ $(CPU_DEFINES) $(BOARD_DEFINES)
LDFLAGS=-Wl,-Map,bsp.map -L. $(CPU_LDFLAGS) $(BOARD_LDFLAGS) $(CPU_CFLAGS) $(BOARD_CFLAGS) $(CFLAGS) -specs=`pwd`/$(BOARD_SPECS)
INCLUDES=-I. -I@top_srcdir@/include $(CPU_INCLUDES) $(BOARD_INCLUDES)
COMMON_TCP_OBJS=arp.o bootp.o cksum.o enet.o icmp.o ip.o net.o pktbuf.o \
                socket.o tcp.o timers.o udp.o
COMMON_OBJS=gdb.o bsp.o bsp_if.o breakpoint.o irq.o irq-rom.o syscall.o \
	    printf.o main.o open.o close.o exit.o lseek.o print.o read.o \
	    write.o sbrk.o getpid.o fstat.o isatty.o kill.o queue.o \
            sysinfo.o unlink.o raise.o gettimeofday.o times.o
COMMON_THREAD_OBJS=gdb-threads.o threads-syscall.o
LIBBSP=$(BOARD_LIBBSP)
LIBS=-lc -lgcc
objdir = .
INSTALL = @INSTALL@
INSTALL_PROGRAM = @INSTALL_PROGRAM@
INSTALL_DATA = @INSTALL_DATA@
BSP_HEADERS=@top_srcdir@/include/bsp/bsp.h
MKINSTALLDIRS = @top_srcdir@/mkinstalldirs
ALL_DEPENDS = $(BOARD_SPECS) $(CPU_SPECS) Makefile $(CPU_DEPENDS) $(BOARD_DEPENDS)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

%.o: %.S
	$(CC) -c $(CPPFLAGS) $(DEFINES) $(ALL_CFLAGS) -o $@ $<

all: bsp.rom bsp.srec bsp.bin bsp.lst $(ROM_STARTUP) $(RAM_STARTUP) $(BOARD_STARTUP)

bsp.rom: $(ROM_STARTUP) $(BOARD_STARTUP) $(LIBBSP) $(BOARD_LDSCRIPT) $(ALL_DEPENDS)
	$(CC) $(LDFLAGS) -o bsp.rom

bsp.lst: bsp.rom
	$(OBJDUMP) $(OBJDUMP_FLAGS) -D bsp.rom > bsp.lst

bsp.srec: bsp.rom
	$(OBJCOPY) $(OBJCOPY_FLAGS) -O srec bsp.rom bsp.srec

bsp.bin: bsp.rom
	$(OBJCOPY) $(OBJCOPY_FLAGS) -O binary bsp.rom bsp.bin

$(LIBBSP): $(COMMON_OBJS) $(CPU_OBJS) $(BOARD_OBJS) $(COMMON_THREAD_OBJS)
	$(AR) $(ARFLAGS) $(LIBBSP) $(COMMON_OBJS) $(CPU_OBJS) $(BOARD_OBJS) \
                         $(COMMON_THREAD_OBJS)

$(BOARD_SPECS): $(BOARD_SPECS_SRC)
	$(INSTALL_DATA) $(BOARD_SPECS_SRC) `basename $@`

$(BOARD_RAM_SPECS): $(BOARD_RAM_SPECS_SRC)
	$(INSTALL_DATA) $(BOARD_RAM_SPECS_SRC) `basename $@`

$(BOARD_LDSCRIPT): $(BOARD_LDSCRIPT_SRC)
	$(INSTALL_DATA) $(BOARD_LDSCRIPT_SRC) `basename $@`

$(CPU_SPECS): $(CPU_SPECS_SRC)
	$(INSTALL_DATA) $(CPU_SPECS_SRC) `basename $@`

$(CPU_RAM_SPECS): $(CPU_RAM_SPECS_SRC)
	$(INSTALL_DATA) $(CPU_RAM_SPECS_SRC) `basename $@`

clean:
	rm -f *~ core *.o a.out bsp.rom bsp.srec bsp.bin bsp.lst bsp.map $(LIBBSP)
	rm -f $(BOARD_SPECS) $(BOARD_RAM_SPECS) $(BOARD_LDSCRIPT)
	rm -f $(CPU_SPECS) $(CPU_RAM_SPECS)

install: cpu-install
	$(SHELL) $(MKINSTALLDIRS) $(tooldir)/include/bsp $(tooldir)/lib
	set -e; for x in $(LIBBSP) $(BOARD_STARTUP) $(ROM_STARTUP) $(RAM_STARTUP) $(BOARD_LDSCRIPT) $(BOARD_SPECS) $(BOARD_RAM_SPECS) $(CPU_RAM_SPECS); do $(INSTALL_DATA) $$x $(tooldir)/lib/`basename $$x`; done
	set -e; for x in $(BSP_HEADERS) $(CPU_HEADERS) $(BOARD_HEADERS); do $(INSTALL_DATA) $$x $(tooldir)/include/bsp/`basename $$x`; done

version.c: force
	@echo 'char *build_date = "'"`date`"'";' > version.c

force:

#
# Dependencies for common objects.
#
gdb.o: gdb.c bsp/bsp.h bsp_if.h bsp/cpu.h gdb.h gdb-cpu.h $(ALL_DEPENDS)
bsp.o: bsp.c bsp/bsp.h bsp_if.h bsp/cpu.h gdb.h $(ALL_DEPENDS)
bsp_if.o: bsp.c bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
breakpoint.o: breakpoint.c bsp/bsp.h bsp/cpu.h $(ALL_DEPENDS)
printf.o: printf.c $(ALL_DEPENDS)
irq.o: irq.c bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
irq-rom.o: irq-rom.c bsp/cpu.h bsp/bsp.h $(ALL_DEPENDS)
syscall.o: syscall.c bsp/cpu.h bsp/bsp.h syscall.h $(ALL_DEPENDS)
main.o: main.c $(ALL_DEPENDS)
open.o: open.c syscall.h bsp-trap.h $(ALL_DEPENDS)
close.o: close.c syscall.h bsp-trap.h $(ALL_DEPENDS)
read.o: read.c syscall.h bsp-trap.h $(ALL_DEPENDS)
write.o: write.c syscall.h bsp-trap.h $(ALL_DEPENDS)
print.o: print.c syscall.h bsp-trap.h $(ALL_DEPENDS)
lseek.o: lseek.c syscall.h bsp-trap.h $(ALL_DEPENDS)
exit.o: exit.c syscall.h bsp-trap.h $(ALL_DEPENDS)
sbrk.o: sbrk.c bsp-trap.h $(ALL_DEPENDS)
raise.o: raise.c bsp-trap.h $(ALL_DEPENDS)
unlink.o: unlink.c bsp-trap.h $(ALL_DEPENDS)
gettimeofday.o: gettimeofday.c bsp-trap.h $(ALL_DEPENDS)
times.o: times.c bsp-trap.h $(ALL_DEPENDS)
fstat.o: fstat.c bsp-trap.h $(ALL_DEPENDS)
getpid.o: getpid.c bsp-trap.h $(ALL_DEPENDS)
isatty.o: isatty.c bsp-trap.h $(ALL_DEPENDS)
kill.o: kill.c bsp-trap.h $(ALL_DEPENDS)
queue.o: queue.c bsp/bsp.h bsp/cpu.h queue.h $(ALL_DEPENDS)
sysinfo.o: sysinfo.c bsp/bsp.h bsp/cpu.h bsp_if.h $(ALL_DEPENDS)
arp.o: arp.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
bootp.o: bootp.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
cksum.o: cksum.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
enet.o: enet.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
icmp.o: icmp.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
ip.o: ip.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
net.o: net.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
pktbuf.o: pktbuf.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
socket.o: socket.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
tcp.o: tcp.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
timers.o: timers.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
udp.o: udp.c bsp/bsp.h bsp_if.h net.h $(ALL_DEPENDS)
generic-mem.o: generic-mem.c bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)
generic-reg.o: generic-reg.c bsp/cpu.h bsp/bsp.h bsp_if.h $(ALL_DEPENDS)

gdb-threads.o: gdb-threads.c bsp/cpu.h bsp/bsp.h bsp/dbg-threads-api.h \
               gdb.h gdb-cpu.h gdb-threads.h
threads-syscall.o: threads-syscall.c bsp/cpu.h bsp/bsp.h threads-syscall.h \
                   bsp/dbg-threads-api.h gdb-cpu.h
bsplog.o: bsplog.c bsp/cpu.h bsp/bsp.h bsp_if.h bsplog.h $(ALL_DEPENDS)



