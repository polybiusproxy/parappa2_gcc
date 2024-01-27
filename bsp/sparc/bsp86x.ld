
ENTRY(_start)
OUTPUT_ARCH(sparc)
SEARCH_DIR(.)
__DYNAMIC  =  0;

/*
 * Base address of the on-CPU peripherals. This is for compatability
 * with the simulator.
 */
_ERC32_MEC = 0x0;

/*
 * All the symbols that might be accessed from C code need to be
 * listed twice, once with an additional underscore. aout format needs
 * an extra underscore, whereas coff & elf doesn't. This is to work
 * with both.
 */
SECTIONS
{
  . = 0x04010000;

  .text : {
    stext = .;
    _stext = .;
    *(.vectors)  /* ROM vectors */
    CREATE_OBJECT_SYMBOLS
    *(.bsp_init)  /* board-specific init code */
    *(.text)

     __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    *(.dtors)
     LONG(0)
    __DTOR_END__ = .;

    _etext  =  .;

    *(.init)
    *(.lit)
    *(.rodata)
    *(.shdata)
    *(.eh_frame)
    *(.gnu.linkonce.t*)
    *(.gnu.linkonce.r*)
    *(.gcc_except_table)
    *(.fini)

  }
  .talias : { }
  . = ALIGN(8);
  __rom_data_start = . ;
  .data  : AT( __rom_data_start ) {
    __ram_data_start = . ;
    *(.vsr_table)          /* secondary vector table in RAM */
    sdata  =  .;
    _sdata  =  .;
    *(.data)
    edata  =  .;
    _edata  =  .;
  }
  . = ALIGN(8);
   __ram_data_end = . ;
  .bss : {
   sbss = . ;
   _sbss = . ;
   __bss_start = .;
   *(.bss)
   *(COMMON)
     end = ALIGN(0x8);
     _end = ALIGN(0x8);
     __end = ALIGN(0x8);
     ebss = .;
     _ebss = .;
  }
  .mstack  : { }
  .rstack  : { }
  .stab  0 (NOLOAD) : {
    [ .stab ]
  }
  .stabstr  0 (NOLOAD) :
  {
    [ .stabstr ]
  }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* These must appear regardless of  .  */
}
