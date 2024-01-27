OUTPUT_FORMAT("elf32-fr30", "elf32-fr30",
	      "elf32-fr30")
OUTPUT_ARCH(fr30)
ENTRY(_start)
SEARCH_DIR(.);
SECTIONS
{
  . = 0x01010000;

  .text		: {
    *(.text)
    *(.text.*)
    KEEP (*(.init))
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0
  _etext = .;
  PROVIDE (etext = .);

  .fini		: { KEEP (*(.fini))    } =0
  .rodata	: { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) }
  .rodata1	: { *(.rodata1) }
  .eh_frame     : { *(.eh_frame) }
  .gcc_except_table : { *(.gcc_except_table) }


   _fdata = ALIGN(16);
  __rom_data_start = . ;

  .data : AT( __rom_data_start ) {
    __ram_data_start = . ;
    *(.data)
    *(.data.*)
    *(.eh_frame)
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
    ___ctors = .;
    KEEP (*(.ctors))
    ___ctors_end = .;
    ___dtors = .;
    KEEP (*(.dtors))
    ___dtors_end = .;
  }
  .data1	: { *(.data1) }
  .got		: { *(.got.plt) *(.got) }
  _edata  = . ;
  PROVIDE (edata = _edata);
  __ram_data_end = . ;
  .bss       :
  {
	__bss_start = . ;
	*(.dynbss)
	*(.bss)
	*(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);

  . = 0x000ffffc ;
  .reset_vector :
  {
	*(.reset_vector)
  }


  /* Stabs debugging sections.  */
  .stab 0		: { *(.stab) }
  .stabstr 0		: { *(.stabstr) }
  .stab.excl 0		: { *(.stab.excl) }
  .stab.exclstr 0	: { *(.stab.exclstr) }
  .stab.index 0		: { *(.stab.index) }
  .stab.indexstr 0	: { *(.stab.indexstr) }
  .comment 0		: { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0	: { *(.debug) }
  .line           0	: { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0	: { *(.debug_srcinfo) }
  .debug_sfnames  0	: { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0	: { *(.debug_aranges) }
  .debug_pubnames 0	: { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0	: { *(.debug_info) }
  .debug_abbrev   0	: { *(.debug_abbrev) }
  .debug_line     0	: { *(.debug_line) }
  .debug_frame    0	: { *(.debug_frame) }
  .debug_str      0	: { *(.debug_str) }
  .debug_loc      0	: { *(.debug_loc) }
  .debug_macinfo  0	: { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0	: { *(.debug_weaknames) }
  .debug_funcnames 0	: { *(.debug_funcnames) }
  .debug_typenames 0	: { *(.debug_typenames) }
  .debug_varnames  0	: { *(.debug_varnames) }
  /* These must appear regardless of  .  */
}
