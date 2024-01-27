/*
 * Linker script for ARM Pid Eval Board
 */

OUTPUT_ARCH("arm")

/*
 * Initalize some symbols to be zero so we can reference them in the
 * crt0 without core dumping. These functions are all optional, but
 * we do this so we can have our crt0 always use them if they exist. 
 * This is so BSPs work better when using the crt0 installed with gcc.
 */
PROVIDE(__hardware_init_hook = 0);
PROVIDE(__software_init_hook = 0);
PROVIDE(_hardware_init_hook = 0);
PROVIDE(_software_init_hook = 0);

SECTIONS
{	
  .text : {
    . = ALIGN(8);
    _ftext = . ;
    PROVIDE(__ftext = .) ;
    PROVIDE(_ftext = .) ;

    *(.text*)
    *(.gnu.warning)
    *(.gnu.linkonce*)
    *(.init)
    *(.rdata)

    _etext = .;
    PROVIDE(__etext = .);
    PROVIDE(_etext = .);
  }

  .fini             : { *(.fini)             }
  .rodata1          : { *(.rodata1)          }
  .rodata           : { *(.rodata*)          }
  .fixup            : { *(.fixup)            }
  .gcc_except_table : { *(.gcc_except_table) }

  . = ALIGN(8);
  _rom_data_start = .;
  PROVIDE(__rom_data_start = .) ;
  PROVIDE(_rom_data_start = .) ;
    
  .data : AT( _rom_data_start ) {
    _ram_data_start = . ;
    PROVIDE(__ram_data_start = .) ;
    PROVIDE(_ram_data_start = .) ;

    *(.vectors)
    *(.data*)
    *(.glue_7t)
    *(.glue_7)

    __GOT1_START__ = .;
    *(.got1) 
    __GOT1_END__ = .;

    /*
     * Put .ctors and .dtors next to the .got2 section, so that the pointers
     * get relocated with -mrelocatable.
     */
    . = ALIGN(8);
    ___CTOR_LIST__ = .;
    KEEP(*(SORT(.ctors*)))
    ___CTOR_END__ = .;
    ___DTOR_LIST__ = .;
    KEEP(*(SORT(.dtors*)))
    ___DTOR_END__ = .;

    __GOT2_START__ = .;
    *(.got2)
    __GOT2_END__ = .;
    __GOT_START__ = .;
    _SDA_BASE_ = .;
    *(.got.plt)
    *(.got)
    __GOT_END__ = .;
    *(.dynamic)
    /* We want the small data sections together, so single-instruction offsets
       can access them all, and initialized data all before uninitialized, so
       we can shorten the on-disk segment size.  */
    __SDATA_START__ = .;
    *(.sdata)
    __SDATA2_START__ = .;
    *(.sdata2)

    _ram_data_end = .;
    PROVIDE(__ram_data_end = .);
    PROVIDE(_ram_data_end = .);
  }

  .bss : {
    . = ALIGN(8);
    _bss_start = . ;
    PROVIDE(__bss_start = .) ;
    PROVIDE(_bss_start = .) ;

    *(.bss)
    *(.dynbss)
    *(COMMON)
    *(.sbss*)
    *(.scommon)

    . = ALIGN(0x4000);   /* Make sure we are on a 16K boundary */
    *(.page_ta*)

    _bss_end = . ;
    PROVIDE(__bss_end = .) ;
    PROVIDE(_bss_end = .) ;
    PROVIDE(__end = .) ;
    PROVIDE(_end = .) ;
    PROVIDE(end = .) ;
  }
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
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
  .stack 0x80000 : { _stack = .; *(.stack) }
  /* These must appear regardless of  .  */
}

PROVIDE(__initial_stack = 0x00080000);
PROVIDE(_initial_stack = 0x00080000);
