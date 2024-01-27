ENTRY(__start)

SECTIONS
{
  /* Start of RAM (leaving room for stub data) */
  . = 0x48008000;

  .text      :
  {
    _stext = .;
    *(.text)
    *(.gnu.warning)
    *(.gnu.linkonce*)
    *(.init)

  }
  _etext = .;
  PROVIDE (etext = .);
  .fini      : { *(.fini)    }
  .rodata    : { *(.rodata)  }
  .rodata1   : { *(.rodata1) }
  .fixup     : { *(.fixup)   }
  .gcc_except_table : { *(.gcc_except_table) }

  __rom_data_start = ALIGN(4);
  .data  :  AT( __rom_data_start )
  {
    __ram_data_start = .;

    *(.data)
    *(.data1)
    _GOT1_START_ = .;
    *(.got1) 
    _GOT1_END_ = .;
    /* Put .ctors and .dtors next to the .got2 section, so that the pointers
       get relocated with -mrelocatable. */
    _GOT2_START_ = .;
    *(.got2)

    PROVIDE(___ctors = .);
    *(.ctors)
    PROVIDE(___ctors_end = .);

    PROVIDE(___dtors = .);
    *(.dtors)
    PROVIDE(___dtors_end = .);

    _GOT2_END_ = .;
    _GOT_START_ = .;
    _GLOBAL_OFFSET_TABLE_ = . + 32768;
    _SDA_BASE_ = .;
    *(.got.plt)
    *(.got)
    _GOT_END_ = .;
    *(.dynamic)
    /* We want the small data sections together, so single-instruction offsets
       can access them all, and initialized data all before uninitialized, so
       we can shorten the on-disk segment size.  */
    *(.sdata*)
   *(.sbss*)
  }
  __ram_data_end = .;
  PROVIDE( __ram_data_end = .);
  _edata  =  .;
  PROVIDE (edata = .);
  . = ALIGN(4);
  __bss_start = .;
  .bss       :
  {
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  . = ALIGN(4);
  __end = . ;
  PROVIDE (_end = .);
}
