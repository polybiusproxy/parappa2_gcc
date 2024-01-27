/* Instruction printing code for the DVP
   Copyright (C) 1998 Free Software Foundation, Inc. 

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "dis-asm.h"
#include "opcode/dvp.h"
#include "elf-bfd.h"
#include "elf/mips.h"
#include "opintl.h"

static int print_dma PARAMS ((bfd_vma, disassemble_info *));
static int print_vif PARAMS ((bfd_vma, disassemble_info *));
static int print_gif PARAMS ((bfd_vma, disassemble_info *));
static int print_vu PARAMS ((bfd_vma, disassemble_info *));
static void print_insn PARAMS ((dvp_cpu, const dvp_opcode *, const dvp_operand *,
				bfd_vma, disassemble_info *, DVP_INSN *));

static int read_word PARAMS ((bfd_vma, disassemble_info *, DVP_INSN *));

/* Return the dvp mach number to use or 0 if not at a dvp insn.
   The different machs are distinguished by marking the start of a group
   of related insns by a specially marked label.  */

int
dvp_info_mach_type (info)
     struct disassemble_info *info;
{
  if (info->flavour == bfd_target_elf_flavour
      && info->symbols != NULL)
    {
      asymbol **sym,**symend;

      for (sym = info->symbols, symend = sym + info->num_symbols;
	   sym < symend; ++sym)
	{
	  int sto = (*(elf_symbol_type **) sym)->internal_elf_sym.st_other;
	  switch (sto)
	    {
	    case STO_DVP_DMA : return bfd_mach_dvp_dma;
	    case STO_DVP_VIF : return bfd_mach_dvp_vif;
	    case STO_DVP_GIF : return bfd_mach_dvp_gif;
	    case STO_DVP_VU : return bfd_mach_dvp_vu;
	    default : break;
	    }
	}
    }

  return 0;
}

/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction.  */

int
print_insn_dvp (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  int mach;
  static int initialized = 0;

  if (!initialized)
    {
      initialized = 1;
      dvp_opcode_init_tables (0);
    }

  mach = dvp_info_mach_type (info);
  if (mach == bfd_mach_dvp_dma
      || info->mach == bfd_mach_dvp_dma)
    return print_dma (memaddr, info);
  if (mach == bfd_mach_dvp_vif
      || info->mach == bfd_mach_dvp_vif)
    return print_vif (memaddr, info);
  if (mach == bfd_mach_dvp_gif
      || info->mach == bfd_mach_dvp_gif)
    return print_gif (memaddr, info);
  if (mach == bfd_mach_dvp_vu
      || info->mach == bfd_mach_dvp_vu)
    return print_vu (memaddr, info);

  (*info->fprintf_func) (info->stream, _("*unknown*"));
  return 4;
}

/* Print one DMA instruction from PC on INFO->STREAM.
   Return the size of the instruction.  */

static int
print_dma (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  bfd_byte buffer[8];
  int i, status, len;
  DVP_INSN insn_buf[2];

  /* The length of a dma tag is 16, however the upper two words are
     vif insns.  */

  len = 8;
  status = (*info->read_memory_func) (memaddr, buffer, len, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

  for (i = 0; i < 2; ++i)
    insn_buf[i] = bfd_getl32 (buffer + i * 4);

  print_insn (DVP_DMA,
	      dma_opcode_lookup_dis (insn_buf[1]), dma_operands,
	      memaddr, info, insn_buf);
  return len;
}

/* Print one VIF instruction from PC on INFO->STREAM.
   Return the size of the instruction.  */

static int
print_vif (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  int len;
  /* Non-zero if vu code follows (i.e. this is mpg).  */
  dvp_cpu cpu;
  DVP_INSN insn_buf[5];

  if (read_word (memaddr, info, insn_buf) != 0)
    return -1;

  len = vif_insn_len (insn_buf[0], &cpu);
  switch (len)
    {
    case 5 :
      if (read_word (memaddr + 16, info, insn_buf + 4) != 0)
	return -1;
      if (read_word (memaddr + 12, info, insn_buf + 3) != 0)
	return -1;
      if (read_word (memaddr + 8, info, insn_buf + 2) != 0)
	return -1;
      /* fall through */
    case 2 :
      if (read_word (memaddr + 4, info, insn_buf + 1) != 0)
	return -1;
      /* fall through */
    case 1 :
    case 0 :
      break;
    }

  print_insn (DVP_VIF,
	      vif_opcode_lookup_dis (insn_buf[0]), vif_operands,
	      memaddr, info, insn_buf);

  /* If symbols are present and this is an mpg or direct insn, assume there
     are symbols to distinguish the mach type so that we can properly
     disassemble the vu code and gif tags.  */
  if (info->symbols
      && (cpu == DVP_VUUP || cpu == DVP_GIF))
    return 4;
  return len * 4;
}

/* Print one GIF instruction from PC on INFO->STREAM.
   Return the size of the instruction.  */

static int
print_gif (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  bfd_byte buffer[16];
  int i, status, len, type, nloop, nregs;
  DVP_INSN insn_buf[4];

  status = (*info->read_memory_func) (memaddr, buffer, 16, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }
  len = 16;

  for (i = 0; i < 4; ++i)
    insn_buf[i] = bfd_getl32 (buffer + i * 4);

  print_insn (DVP_GIF,
	      gif_opcode_lookup_dis (insn_buf[1]), gif_operands,
	      memaddr, info, insn_buf);

  type = (insn_buf[1] >> 26) & 3;
  nloop = insn_buf[0] & 0x7fff;
  nregs = (insn_buf[1] >> 28) & 15;
  switch (type)
    {
    case GIF_PACKED :
      return len + nloop * nregs * 16;
    case GIF_REGLIST :
      return len + ((nloop * nregs + 1) & ~1) * 8;
    case GIF_IMAGE :
      return len + nloop * 16;
    }

  return len;
}

/* Print one VU instruction from PC on INFO->STREAM.
   Return the size of the instruction.  */

static int
print_vu (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  bfd_byte buffer[8];
  void *stream = info->stream;
  fprintf_ftype func = info->fprintf_func;
  int status;
  /* First element is upper, second is lower.  */
  DVP_INSN upper,lower;

  status = (*info->read_memory_func) (memaddr, buffer, 8, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }
  /* The lower instruction has the lower address.  */
  upper = bfd_getl32 (buffer + 4);
  lower = bfd_getl32 (buffer);

  /* FIXME: This will need revisiting.  */
  print_insn (DVP_VUUP,
	      vu_upper_opcode_lookup_dis (upper), vu_operands,
	      memaddr, info, &upper);
#ifdef VERTICAL_BAR_SEPARATOR
  (*func) (stream, " | ");
#else
  /* Not sure how much whitespace to print here.
     At least two spaces, not more than 9, and having columns line up somewhat
     seems reasonable.  */
  (*func) (stream, " \t");
#endif
  /* If the 'i' bit is set then the lower word is not an insn but a 32 bit
     floating point immediate value.  */
  if (upper & 0x80000000)
    {
      /* FIXME: assumes float/int are same size/endian.  */
      union { float f; int i; } x;
      x.i = lower;
      (*func) (stream, "loi %g", x.f);
    }
  else
    print_insn (DVP_VULO,
		vu_lower_opcode_lookup_dis (lower), vu_operands,
		memaddr, info, &lower);

  return 8;
}

/* Print one instruction.
   OPCODE is a pointer to the head of the hash list.  */

static void
print_insn (cpu, opcode, operand_table, memaddr, info, insn)
     dvp_cpu cpu;
     const dvp_opcode *opcode;
     const dvp_operand *operand_table;
     bfd_vma memaddr;
     disassemble_info *info;
     DVP_INSN *insn;
{
  void *stream = info->stream;
  fprintf_ftype func = info->fprintf_func;

  for ( ; opcode != NULL; opcode = DVP_OPCODE_NEXT_DIS (opcode))
    {
      const unsigned char *syn;
      int mods,invalid,num_operands;
      long value;
      const dvp_operand *operand;

      /* Ignore insns that have a mask value of 0.
	 Such insns are not intended to be disassembled by us.  */
      if (opcode->mask == 0)
	continue;
      if (opcode->flags & DVP_OPCODE_IGNORE_DIS)
	continue;
      /* Basic bit mask must be correct.  */
      if ((insn[opcode->opcode_word] & opcode->mask) != opcode->value)
	continue;

      /* Make two passes over the operands.  First see if any of them
	 have extraction functions, and, if they do, make sure the
	 instruction is valid.  */

      dvp_opcode_init_print ();
      invalid = 0;

      for (syn = opcode->syntax; *syn; ++syn)
	{
	  int index;

	  if (*syn < 128)
	    continue;

	  mods = 0;
	  index = DVP_OPERAND_INDEX (*syn);
	  while (DVP_MOD_P (operand_table[index].flags))
	    {
	      mods |= operand_table[index].flags & DVP_MOD_BITS;
	      ++syn;
	      index = DVP_OPERAND_INDEX (*syn);
	    }
	  operand = operand_table + index;
	  if (operand->extract)
	    (*operand->extract) (opcode, operand, mods, insn, &invalid);
	}
      if (invalid)
	continue;

      /* The instruction is valid.  */

      (*func) (stream, "%s", opcode->mnemonic);
      num_operands = 0;
      for (syn = opcode->syntax; *syn; ++syn)
	{
	  int index;

	  if (*syn < 128)
	    {
	      (*func) (stream, "%c", *syn);
	      continue;
	    }

	  /* We have an operand.  Fetch any special modifiers.  */
	  mods = 0;
	  index = DVP_OPERAND_INDEX (*syn);
	  while (DVP_MOD_P (operand_table[index].flags))
	    {
	      mods |= operand_table[index].flags & DVP_MOD_BITS;
	      ++syn;
	      index = DVP_OPERAND_INDEX (*syn);
	    }
	  operand = operand_table + index;

	  /* Extract the value from the instruction.  */
	  if (operand->extract)
	    {
	      value = (*operand->extract) (opcode, operand, mods,
					   insn, (int *) NULL);
	    }
	  else
	    {
	      /* We currently assume a field does not cross a word boundary.  */
	      int shift = ((mods & DVP_MOD_THIS_WORD)
			   ? (operand->shift & 31)
			   : operand->shift);
	      /* FIXME: There are currently two ways to specify which word:
		 the `word' member and shift / 32.  */
	      int word = operand->word ? operand->word : shift / 32;
	      DVP_INSN mask = (operand->bits == 32
			       ? 0xffffffff : ((1 << operand->bits) - 1));
	      shift = shift % 32;
	      value = (insn[word] >> shift) & mask;
	      if ((operand->flags & DVP_OPERAND_SIGNED) != 0
		  && (value & (1 << (operand->bits - 1)))
		  && operand->bits < 8 * sizeof (long))
		value -= 1L << operand->bits;
	    }

	  /* Print the operand as directed by the flags.  */
	  if (operand->print)
	    (*operand->print) (opcode, operand, mods, insn, info, value);
	  else if (operand->flags & DVP_OPERAND_FAKE)
	    ; /* nothing to do (??? at least not yet) */
	  else if (operand->flags & DVP_OPERAND_RELATIVE_BRANCH)
	    (*info->print_address_func) (memaddr + 8 + (value << 3), info);
	  /* ??? Not all cases of this are currently caught.  */
	  else if (operand->flags & DVP_OPERAND_ABSOLUTE_BRANCH)
	    (*info->print_address_func) ((bfd_vma) value & 0xffffffff, info);
	  else if (operand->flags & DVP_OPERAND_MIPS_ADDRESS)
	    (*info->print_address_func) ((bfd_vma) value & 0xffffffff, info);
	  else if (operand->flags & (DVP_OPERAND_VU_ADDRESS | DVP_OPERAND_UNPACK_ADDRESS))
	    (*func) (stream, "0x%lx", value & 0xffffffff);
          else if ((operand->flags & (DVP_OPERAND_SIGNED | DVP_OPERAND_RELOC_11_S4)) != 0
		   || (value >= -1 && value < 16))
	    (*func) (stream, "%ld", value);
	  else
	    (*func) (stream, "0x%lx", value);

	  if (! (operand->flags & DVP_OPERAND_SUFFIX))
	    ++num_operands;
	}

      /* We have found and printed an instruction; return.  */
      return;
    }

  (*func) (stream, _("*unknown*"));
}

/* Utility to read one word.
   The result is 0 for success, -1 for failure.  */

static int
read_word (memaddr, info, insn_buf)
     bfd_vma memaddr;
     disassemble_info *info;
     DVP_INSN *insn_buf;
{
  int status;
  bfd_byte buffer[4];

  status = (*info->read_memory_func) (memaddr, buffer, 4, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }
  *insn_buf = bfd_getl32 (buffer);
  return 0;
}
