OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc",
	      "elf32-powerpc")
OUTPUT_ARCH(powerpc)
ENTRY(_start)

SECTIONS
{
  . = 0x10000 ;

  /* Read-only sections, merged into text segment: */
  .interp   : { *(.interp) }
  .hash		  : { *(.hash)		}
  .dynsym	  : { *(.dynsym)		}
  .dynstr	  : { *(.dynstr)		}
  .rela.text     :
    { *(.rela.text) *(.rela.gnu.linkonce.t*) }
  .rela.data     :
    { *(.rela.data) *(.rela.gnu.linkonce.d*) }
  .rela.rodata   :
    { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
  .rela.got	  : { *(.rela.got)	}
  .rela.got1	  : { *(.rela.got1)	}
  .rela.got2	  : { *(.rela.got2)	}
  .rela.ctors	  : { *(.rela.ctors)	}
  .rela.dtors	  : { *(.rela.dtors)	}
  .rela.init	  : { *(.rela.init)	}
  .rela.fini	  : { *(.rela.fini)	}
  .rela.bss	  : { *(.rela.bss)	}
  .rela.plt	  : { *(.rela.plt)	}
  .rela.sdata	  : { *(.rela.sdata2)	}
  .rela.sbss	  : { *(.rela.sbss2)	}
  .rela.sdata2	  : { *(.rela.sdata2)	}
  .rela.sbss2	  : { *(.rela.sbss2)	}
  .plt   : { *(.plt) }
  .text      :
  {
    *(.text)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0
  .init		  : { *(.init)		} =0
  .fini		  : { *(.fini)		} =0
  .rodata	  : { *(.rodata) *(.gnu.linkonce.r*) }
  .rodata1	  : { *(.rodata1) }
  .gcc_except_table	: { *(.gcc_except_table)		}
  _etext = .;
  PROVIDE (etext = .);
  .sdata2   : { *(.sdata2) }
  .sbss2   : { *(.sbss2) }
  . = ALIGN(8);
  __rom_data_start = . ;
  .data :
  AT( __rom_data_start )
  {
    __ram_data_start = .;
    *(.data)
    *(.gnu.linkonce.d*)
    *(.ctors)
    *(.dtors)
    CONSTRUCTORS
    *(.data1)
    *(.got1)
    *(.dynamic)
    *(.got2)
    *(.ctors)
    *(.dtors)
    *(.fixup)
    *(.got)
    *(.got.plt)
    *(.sdata)
  }
/*  .data1   : { *(.data1) }
  .got1		  : { *(.got1) }
  .dynamic	  : { *(.dynamic) }*/
  /* Put .ctors and .dtors next to the .got2 section, so that the pointers
     get relocated with -mrelocatable. Also put in the .fixup pointers.
     The current compiler no longer needs this, but keep it around for 2.7.2  */
/*  .got2		  : { *(.got2) }
  .ctors	  : { *(.ctors) }
  .dtors	  : { *(.dtors) }
  .fixup	  : { *(.fixup) }
  .got		  : { *(.got) }
  .got.plt	  : { *(.got.plt) }*/
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
/*  .sdata	  : { *(.sdata) }*/
  PROVIDE( __ram_data_end = .);
  . = ALIGN(4);
  _edata  =  .;
  PROVIDE (edata = .);
  .sbss      :
  {
    PROVIDE (__sbss_start = .);
    *(.sbss)
    *(.scommon)
    PROVIDE (__sbss_end = .);
  }
  .bss       :
  {
   PROVIDE (__bss_start = .);
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  . = ALIGN(4);
  _end = . ;
  PROVIDE (end = .);


  . = 0xfffffe00 ;
  .init_403 :
  {
    *(.init_403)
  }

  . = 0xfffffffc ;
  .reset_403 :
  {
    *(.reset_403)
  }


  /* These are needed for ELF backends which have not yet been
     converted to the new style linker.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
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
