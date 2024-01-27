/*
 * Linker script for Dragonball ADS BSP
 */

OUTPUT_ARCH("m68k")
ENTRY(_start)

/*
 * Initalize some symbols to be zero so we can reference them in the
 * crt0 without core dumping. These functions are all optional, but
 * we do this so we can have our crt0 always use them if they exist. 
 * This is so BSPs work better when using the crt0 installed with gcc.
 */
PROVIDE (_hardware_init_hook = 0);
PROVIDE (_software_init_hook = 0);

/*
 * +-----------------------+ <- 0x00008000 (32K arbitrary limit for BSP RAM)
 * | .text                 |
 * |    _ftext             |
 * |      reset vector     |
 * |      text code        |
 * |      ctor list        |
 * |      dtor list        |
 * |      rodata           |
 * |      gcc_except_table |
 * |      __INIT_SECTION__ |
 * |      __FINI_SECTION__ |
 * |      lit              |
 * |    _etext             |
 * +-----------------------+
 * | .data                 |
 * |    _ram_data_start    |
 * |      global data      | initialized data goes here
 * |    _ram_data_end      |
 * +-----------------------+
 * | .bss                  |
 * |    _bss_start         | start of bss, cleared by crt0
 * |      global data      | uninitialized data here
 * |    _bss_end           | start of heap, used by sbrk()
 * |      heap             |
 * +-----------------------+
 * |                       |
 * .                       .
 * .                       .
 * .                       .
 * |                       |
 * |                       |
 * |        __stack        | top of stack (Initialized by BSP)
 * +-----------------------+ <- high memory
 */
SECTIONS
{	
  .text : {
    . = ALIGN(0x2);
    ftext = . ;
    _ftext = . ;
    *(.text)

    . = ALIGN(0x2);
    _runtime_reloc_start = .;
    *(.rel.sdata)
    _runtime_reloc_stop = .;

    . = ALIGN(0x2);
    __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;

    . = ALIGN(0x2);
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    *(.dtors)
    LONG(0)
    __DTOR_END__ = .;

    . = ALIGN(0x2);
    *(.rodata)
    *(.gcc_except_table) 

    . = ALIGN(0x2);
    __INIT_SECTION__ = . ;
    LONG (0x4e560000)	/* linkw %fp,#0 */
    *(.init)
    SHORT (0x4e5e)	/* unlk %fp */
    SHORT (0x4e75)	/* rts */

    . = ALIGN(0x2);
    __FINI_SECTION__ = . ;
    LONG (0x4e560000)	/* linkw %fp,#0 */
    *(.fini)
    SHORT (0x4e5e)	/* unlk %fp */
    SHORT (0x4e75)	/* rts */

    . = ALIGN(0x2);
    *(.lit)

    . = ALIGN(0x2);

    etext = .;
    _etext = .;
  }

  . = ALIGN(0x4);
  _rom_data_start = .;
    
  .data : AT( _rom_data_start ) {
    fdata  =  .;
    _fdata  =  .;
    _ram_data_start = . ;
    *(.shdata)
    *(.data)
    *(.eh_fram)
    *(.eh_frame)
    . = ALIGN(0x2);
    edata  =  .;
    _edata  =  .;
    _ram_data_end = . ;
  }

  .bss BLOCK (0x2) : {
    fbss = .;
    _fbss = .;
    _bss_start = . ;
    *(.shbss)
    *(.bss)
    *(COMMON)
    . = ALIGN(0x2);
    _bss_end = . ;
    end = .;
    _end = .;
    ebss = .;
    _ebss = .;
  }

  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }

  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to 
     the beginning of the section so we begin them at 0.  */

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
}

/*
 * allocate the stack to be at the top of memory, since the stack
 * grows down
 */
PROVIDE (_superstack = 0x200000);
PROVIDE (_userstack  = 0x180000);
