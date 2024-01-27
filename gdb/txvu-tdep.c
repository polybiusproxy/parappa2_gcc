/* Target-dependent code for the TXVU architecture, for GDB, the GNU Debugger.
   Copyright 1998, Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include <ctype.h>
#include "symtab.h"
#include "target.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "value.h"
#include "symfile.h"
#include "objfiles.h"
#include "bfd.h"
#include "buildsym.h"
#include "breakpoint.h"
#include <setjmp.h>	/* needed by top.h :-( */
#include "top.h"

int txvu_breakpoint_initial_cpu PARAMS ((struct symtab_and_line *));

CORE_ADDR txvu_read_pc_pid PARAMS((int pid));
CORE_ADDR txvu_read_sp PARAMS((void));
void txvu_write_sp PARAMS((CORE_ADDR val));

CORE_ADDR txvu_call_return_address PARAMS ((void));
int txvu_is_leaf_routine PARAMS ((struct frame_info *));
void txvu_init_frame_pc_first PARAMS ((int, struct frame_info *));

void txvu_symfile_postread PARAMS ((struct objfile *));

int txvu_is_phantom_section PARAMS ((asection *));

void txvu_redefine_default_ops PARAMS ((struct target_ops *));

static int addr_is_dvp PARAMS ((CORE_ADDR addr));

/* Current CPU context */
enum txvu_cpu_context txvu_cpu;

static char *txvu_cpu_string;

static const char *txvu_cpu_names[TXVU_CPU_NUM] = {
  "auto", "master", "vu0", "vu1", "vif0", "vif1"
};

/* Boolean to say whether context is part of prompt or not. */
static int context_in_prompt = 0;

enum { ORDER_XYZW, ORDER_WZYX };

static int printvector_order = ORDER_WZYX;

static void txvu_set_prompt PARAMS((enum txvu_cpu_context to_cpu));

static void context_switch PARAMS ((enum txvu_cpu_context to_cpu));

struct overlay_map {
  short is_mapped;		/* non-zero if actively mapped  */
  short cpu;			/* which unit are we mapped to? */
  unsigned start, end;		/* VU address range		*/
  struct obj_section *osect;
};

#define OTABLE_SIZE	64

static struct overlay_map overlay_table[OTABLE_SIZE]; /* static size for now */

static int otable_size = OTABLE_SIZE;
static int num_overlays = 0;

static void add_section_to_overlay_table PARAMS ((struct obj_section *sect));
static void init_overlay_table PARAMS ((struct objfile *objfile));
static void clear_overlay_table PARAMS ((void));
static void invalidate_overlay_map PARAMS((void));
static struct overlay_map* find_overlay_map PARAMS ((CORE_ADDR addr));
static int overlay_section_index PARAMS ((asection *asect));

static void update_overlay PARAMS ((enum txvu_cpu_context cpu));
static void check_overlap PARAMS ((enum txvu_cpu_context cpu));
static void txvu_overlay_update PARAMS ((struct obj_section *osect));

static void set_cpu_command PARAMS ((char*, int, struct cmd_list_element*));
static void printvector_command PARAMS ((char *, int));


int
txvu_step_skips_delay (pc)
     CORE_ADDR pc;
{
  return ((txvu_cpu == TXVU_CPU_AUTO || txvu_cpu == TXVU_CPU_MASTER)
	  ? mips_step_skips_delay (pc)
	  : 0);
}


static struct minimal_symbol *
lookup_minimal_symbol_by_addr (pc)
     CORE_ADDR pc;
{
  int lo;
  int hi;
  int new;
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  struct minimal_symbol *best_symbol = NULL;

  for (objfile = object_files;
       objfile != NULL;
       objfile = objfile -> next)
    {
      /* If this objfile has a minimal symbol table, go search it using
	 a binary search.  Note that a minimal symbol table always consists
	 of at least two symbols, a "real" symbol and the terminating
	 "null symbol".  If there are no real symbols, then there is no
	 minimal symbol table at all. */

      if ((msymbol = objfile -> msymbols) != NULL)
	{
	  lo = 0;
	  hi = objfile -> minimal_symbol_count - 1;

	  if (pc >= SYMBOL_VALUE_ADDRESS (&msymbol[lo]))
	    {
	      while (SYMBOL_VALUE_ADDRESS (&msymbol[hi]) > pc)
		{
		  /* pc is still strictly less than highest address */
		  /* Note "new" will always be >= lo */
		  new = (lo + hi) / 2;
		  if ((SYMBOL_VALUE_ADDRESS (&msymbol[new]) >= pc) ||
		      (lo == new))
		    {
		      hi = new;
		    }
		  else
		    {
		      lo = new;
		    }
		}

	      /* There may be many symbols at the same address. To get the
		 info field right, we start at the first one and work towards
		 the last one.  */
	      while (hi >= 1
		     && (SYMBOL_VALUE_ADDRESS (&msymbol[hi])
			 == SYMBOL_VALUE_ADDRESS (&msymbol[hi-1])))
		hi--;

	      /* We want to keep the info field if it is set */
	      if (MSYMBOL_INFO (&msymbol[hi]))
		best_symbol = &msymbol[hi];
		  
	      /* If we have multiple symbols at the same address, we want
		 hi to point to the last one.  That way we can find the
		 right symbol if it has an index greater than hi.  */
	      while (hi < objfile -> minimal_symbol_count - 1
		     && (SYMBOL_VALUE_ADDRESS (&msymbol[hi])
			 == SYMBOL_VALUE_ADDRESS (&msymbol[hi+1])))
		{
		  hi++;

		  if (MSYMBOL_INFO (&msymbol[hi]))
		    best_symbol = &msymbol[hi];
		}

	      while (hi >= 0 && msymbol[hi].type == mst_abs)
		--hi;

	      if (hi >= 0
		  && ((best_symbol == NULL) ||
		      (SYMBOL_VALUE_ADDRESS (best_symbol) < 
		       SYMBOL_VALUE_ADDRESS (&msymbol[hi]))))
		{
		  best_symbol = &msymbol[hi];
		}
	    }
	}
    }

  return (best_symbol);
}

/* Address space definition (from sky-vu.h) */

#define VU0_MEM0_WINDOW_START 	0xb1000000
#define VU0_MEM0_SIZE  		0x1000	/* 4K = 4096 */
#define VU1_MEM0_WINDOW_START 	0xb1008000
#define VU1_MEM0_SIZE		0x4000	/* 16K = 16384 */

static int
addr_is_dvp (addr)
     CORE_ADDR addr;
{
  struct minimal_symbol *sym;
  asection *section;

  if (addr>=VU0_MEM0_WINDOW_START && addr<VU0_MEM0_WINDOW_START+VU0_MEM0_SIZE)
    return bfd_mach_dvp_vu;

  if (addr>=VU1_MEM0_WINDOW_START && addr<VU1_MEM0_WINDOW_START+VU1_MEM0_SIZE)
    return bfd_mach_dvp_vu;

  /* All the special "." symbols are in the unmapped range, as are the "_$"
     equivalents to all the user defined symbols.  Therefore, we increase 
     our chances of finding something if we translate to the unmapped area.
     By definition, however, we are supposed to use main memory addresses 
     unless the context tells us otherwise.   */

  if (txvu_cpu==TXVU_CPU_VU0 || txvu_cpu==TXVU_CPU_VU1)
    {
      section = find_pc_overlay (addr);
      addr = overlay_unmapped_address (addr, section);
    }

  sym = lookup_minimal_symbol_by_addr (addr);
  if (sym)
    return (int) MSYMBOL_INFO (sym);

  return 0;
}

/* Although I would prefer not to, I had to widen the gdb<->sim interface
   slightly. The actual communication is through the GDB_COMM area, but
   a few of the target vectors had to be redefined to provide the hook to
   instantiate the communication. The original target ops are cached here.  */
struct target_ops_cache
{
  int  	      (*to_wait) PARAMS ((int, struct target_waitstatus *));
  int  	      (*to_insert_breakpoint) PARAMS ((CORE_ADDR, char *));
  int  	      (*to_remove_breakpoint) PARAMS ((CORE_ADDR, char *));
  int  	      (*to_xfer_memory) PARAMS ((CORE_ADDR memaddr, char *myaddr,
					 int len, int write,
					 struct target_ops * target));
} default_target_ops;

static int txvu_wait PARAMS ((int pid, struct target_waitstatus *status));
static int txvu_insert_breakpoint PARAMS ((CORE_ADDR addr, char *contents));
static int txvu_remove_breakpoint PARAMS ((CORE_ADDR addr, char *contents));
static int txvu_xfer_memory PARAMS ((CORE_ADDR memaddr, char *myaddr,
				     int len, int write,
				     struct target_ops * target));

void
txvu_redefine_default_ops (ops)
     struct target_ops *ops;
{
  default_target_ops.to_wait = ops->to_wait;
  default_target_ops.to_insert_breakpoint = ops->to_insert_breakpoint;
  default_target_ops.to_remove_breakpoint = ops->to_remove_breakpoint;
  default_target_ops.to_xfer_memory = ops->to_xfer_memory;

  ops->to_wait = txvu_wait;
  ops->to_insert_breakpoint = txvu_insert_breakpoint;
  ops->to_remove_breakpoint = txvu_remove_breakpoint;
  ops->to_xfer_memory = txvu_xfer_memory;
}

/* The simulator does not know what GDB thinks the context is. 
   This is another case where we redefine the default target op so
   that we can fudge the addresses on the way in.  */

static int 
txvu_xfer_memory (memaddr, myaddr, len, write, target)
     CORE_ADDR memaddr;
     char *myaddr;
     int len, write;
     struct target_ops * target;
{
  CORE_ADDR addr = memaddr;

  if (memaddr<VU0_MEM0_SIZE && txvu_cpu==TXVU_CPU_VU0)
    addr += VU0_MEM0_WINDOW_START;

  if (memaddr<VU1_MEM0_SIZE && txvu_cpu==TXVU_CPU_VU1)
    addr += VU1_MEM0_WINDOW_START;

  return default_target_ops.to_xfer_memory (addr, myaddr, len, write, target);
}

#define TXVU_VU_INSN_SIZE	8
#define TXVU_VU_BRK_LEN TXVU_VU_INSN_SIZE

static unsigned char vu_break_insn[TXVU_VU_BRK_LEN];
static unsigned char vif_break_insn[] = { 0, 0, 0, TXVU_VIF_BRK_MASK };

static int fifo_bpt_cnt = 0;

static int
txvu_insert_breakpoint (addr, contents_cache)
     CORE_ADDR addr;
     char *contents_cache;
{
  int val, len, mach = 0;
  long addr_4;
  int bp_type;
  struct minimal_symbol *msym = lookup_minimal_symbol_by_addr (addr);

  if (msym)
    mach = (int) MSYMBOL_INFO (msym);
  
  /* The standard insert_breakpoint function can place breakpoints in 
     addressable memory. However, the FIFOs are not externally addressable,
     so we need to pass the information so the simulator can do it locally. */

  switch (mach)
    {
    case bfd_mach_dvp_dma: /* Can't set DMA breakpoints */
    case bfd_mach_dvp_gif: /* Can't set GIF breakpoints */
      /* Since the search (in lookup_minimal_symbol_by_addr) is inexact, 
	 should we assume the nearest thing is a VIF instruction?  */
      val = 1;
      break;

    case bfd_mach_dvp_vif:
      len = sizeof (vif_break_insn);
      val = target_read_memory (addr, vif_break_insn, len);

      if (val != 0)
	return val;

      memcpy (contents_cache, vif_break_insn, len);

      /* We get the memory back in little-endian order. 
	 That means MSB is byte 3.  */
      vif_break_insn[3] |= TXVU_VIF_BRK_MASK;

      val = target_write_memory (addr, vif_break_insn, len);
      if (val != 0)
	return val;

      bp_type = TXVU_CPU_VIF1;
      target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ, 
			   (char*) &bp_type, 4);

      addr_4 = addr;
      target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ + 4, 
			   (char*) &addr_4, 4);

      target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ + 8,
			   contents_cache, 4);

      ++fifo_bpt_cnt;
      val = target_write_memory (FIFO_BPT_CNT, (char *) &fifo_bpt_cnt, 4);
      break;

    case bfd_mach_dvp_vu:
      {
	struct overlay_map *ovly;
	enum txvu_cpu_context curr_cpu = txvu_cpu;
	asection *sect = SYMBOL_BFD_SECTION (msym);
	int i = overlay_section_index (sect);

	/* i is negative if msym is not in an overlay section, which happens
	   if we are given the unmapped address. */
	if (i >= 0)
	  {
	    ovly = &(overlay_table[i]);

	    if (ovly->is_mapped)
	      txvu_cpu = ovly->cpu;
	  }

	val = target_read_memory (addr, vu_break_insn, TXVU_VU_BRK_LEN);

	memcpy (contents_cache, vu_break_insn, TXVU_VU_BRK_LEN);

	if (val == 0)
	  {
	    /* We get the memory back in little-endian order. 
	       That means MSB is byte 7.  */
	    vu_break_insn[7] |= TXVU_VU_BRK_MASK;

	    val = target_write_memory (addr, vu_break_insn, TXVU_VU_BRK_LEN);
	  }

	txvu_cpu = curr_cpu;

	if (val != 0)
	  return val;

	if (!(pc_in_mapped_range (addr, sect)))
	  {
	    bp_type = TXVU_CPU_VU1;
	    target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ, 
				 (char*) &bp_type, 4);

	    addr_4 = addr;
	    target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ + 4, 
				 (char*) &addr_4, 4);

	    target_write_memory (FIFO_BPT_TBL + fifo_bpt_cnt*BPT_ELEM_SZ + 8,
				 contents_cache + 4, 4);

	    ++fifo_bpt_cnt;
	    val = target_write_memory (FIFO_BPT_CNT, (char*) &fifo_bpt_cnt, 4);
	  }
	break;
      }
    
    default:
      {
	CORE_ADDR bp_addr = addr;
	int bp_size = 0;
	unsigned char *bp = BREAKPOINT_FROM_PC (&bp_addr, &bp_size);
	val = target_read_memory (bp_addr, contents_cache, bp_size);

	if (val != 0)
	  return val;

#ifdef TARGET_SKY_B
	printf_filtered ("Can't place breakpoint at %llx\n", addr);
#else
	val = target_write_memory (bp_addr, bp, bp_size);
#endif
	break;
      }
    }

  return val;
}

static int
txvu_remove_breakpoint (addr, contents_cache)
     CORE_ADDR addr;
     char *contents_cache;
{
  int val, mach = 0;
  struct minimal_symbol *msym = lookup_minimal_symbol_by_addr (addr);

  if (msym)
    mach = (int) MSYMBOL_INFO (msym);
  
  if (mach == bfd_mach_dvp_vu)
    {
      struct overlay_map *ovly;
      enum txvu_cpu_context curr_cpu = txvu_cpu;
      asection *sect = SYMBOL_BFD_SECTION (msym);
      int i = overlay_section_index (sect);

      /* i is negative if msym is not in an overlay section, which happens
	 if we are given the unmapped address. */
      if (i >= 0)
	{
	  ovly = &(overlay_table[i]);

	  if (ovly->is_mapped)
	    txvu_cpu = ovly->cpu;
	}

      val = target_write_memory (addr, contents_cache, TXVU_VU_BRK_LEN);

      txvu_cpu = curr_cpu;
    }
  else
    val = target_write_memory (addr, contents_cache, 4);

  return val;
}


int
txvu_breakpoint_initial_cpu (sal)
     struct symtab_and_line *sal;
{
  return txvu_cpu;
}

/* CPU context state. */
static struct {
  struct symtab *symtab;	/* symtab in context i */
  int line;			/* last source line */
  int bp_valid;			/* for setting default breakpoints */
  CORE_ADDR bp_address;
  struct symtab *bp_symtab;
  int bp_line;
} cpu_state[TXVU_CPU_NUM];

extern int default_breakpoint_valid;
extern CORE_ADDR default_breakpoint_address;
extern struct symtab *default_breakpoint_symtab;
extern int default_breakpoint_line;

static void
context_switch (to_cpu)
     enum txvu_cpu_context to_cpu;
{
#ifdef TARGET_SKY_B
  switch (to_cpu)
    {
    case TXVU_CPU_MASTER:
    case TXVU_CPU_VU0:
    case TXVU_CPU_VIF0:
      return;	/* Leave the context where it was. */

    default:
      break;
    }
#endif

  if (to_cpu == txvu_cpu)
    return;

  if (txvu_cpu != TXVU_CPU_AUTO)
    {
      /* Save current state. */
      cpu_state[txvu_cpu].symtab = current_source_symtab;
      cpu_state[txvu_cpu].line = current_source_line;

      cpu_state[txvu_cpu].bp_valid = default_breakpoint_valid;

      if (default_breakpoint_valid)
	{
	  cpu_state[txvu_cpu].bp_address = default_breakpoint_address;
	  cpu_state[txvu_cpu].bp_symtab = default_breakpoint_symtab;
	  cpu_state[txvu_cpu].bp_line = default_breakpoint_line;
	}
    }

  txvu_cpu = to_cpu;

  if (txvu_cpu_string != NULL) 
    free (txvu_cpu_string);

  txvu_cpu_string = strsave (txvu_cpu_names[txvu_cpu+1]);

  if (context_in_prompt)
    txvu_set_prompt (txvu_cpu);
  else
    printf_filtered ("CPU context is now %s\n", txvu_cpu_string);

  if (txvu_cpu != TXVU_CPU_AUTO)
    {
      /* Restore previous state. */
      if (cpu_state[txvu_cpu].symtab)
	  current_source_symtab = cpu_state[txvu_cpu].symtab;

      if (cpu_state[txvu_cpu].line)
	current_source_line = cpu_state[txvu_cpu].line;

      set_default_breakpoint (cpu_state[txvu_cpu].bp_valid, 
			      cpu_state[txvu_cpu].bp_address, 
			      cpu_state[txvu_cpu].bp_symtab, 
			      cpu_state[txvu_cpu].bp_line);
    }
}

/* A local version of the global symfile.c::overlay_cache_invalid flag.  */
static int txvu_overlay_cache_invalid = 1;

static int
txvu_wait (pid, status)
     int pid;
     struct target_waitstatus *status;
{
  char last_device;
  int wpid = default_target_ops.to_wait (pid, status);

  txvu_overlay_cache_invalid = 1;

  /* If we halted for some reason, set the context to the CPU that stopped. */
  if (status->kind == TARGET_WAITKIND_STOPPED)
    {
      if (target_read_memory (LAST_DEVICE, &last_device, 1))
	printf ("Can't read last device?\n");

      if (last_device != txvu_cpu)
	{
	  /* Set the context to the device that interrupted */
	  context_switch (last_device);
	}
    }

  fifo_bpt_cnt = 0;
  target_write_memory (FIFO_BPT_CNT, (char *) &fifo_bpt_cnt, 4);

  return wpid;
}

CORE_ADDR
txvu_read_pc_pid (pid)
     int pid;
{
  CORE_ADDR pc;
  char buf[MAX_REGISTER_RAW_SIZE];

  switch (txvu_cpu)
    {
    case TXVU_CPU_VU0:
      read_register_gen (VU0_PC_REGNUM, buf);
      pc = extract_unsigned_integer (buf, 4);
      pc <<= 3;			/* Convert from double words to bytes */
      break;

    case TXVU_CPU_VU1:
      read_register_gen (VU1_PC_REGNUM, buf);
      pc = extract_unsigned_integer (buf, 4);
      pc <<= 3;			 /* Convert from double words to bytes */
      break;

    case TXVU_CPU_VIF0:
      read_register_gen (VIF0_PC_REGNUM, buf);
      pc = extract_unsigned_integer (buf, 4);
      break;

    case TXVU_CPU_VIF1:
      read_register_gen (VIF1_PC_REGNUM, buf);
      pc = extract_unsigned_integer (buf, 4);
      break;

    case TXVU_CPU_MASTER:
    default:
      pc = ADDR_BITS_REMOVE ((CORE_ADDR) read_register_pid (PC_REGNUM, pid));
      break;
    }

  return pc;
}

CORE_ADDR
txvu_read_sp ()
{
  CORE_ADDR sp;
  char buf[MAX_REGISTER_RAW_SIZE];

  switch (txvu_cpu)
    {
    case TXVU_CPU_VU0:
      read_register_gen (VU0_SP_REGNUM, buf);
      sp = extract_unsigned_integer (buf, 2);
      sp <<= 3;			/* Convert from double words to bytes */
      break;

    case TXVU_CPU_VU1:
      read_register_gen (VU1_SP_REGNUM, buf);
      sp = extract_unsigned_integer (buf, 2);
      sp <<= 3;			 /* Convert from double words to bytes */
      break;

    case TXVU_CPU_VIF0:
    case TXVU_CPU_VIF1:
      sp = 0; /* These have no stack. */
      break;

    case TXVU_CPU_MASTER:
    default:
      sp = ADDR_BITS_REMOVE (read_register (SP_REGNUM));
      break;
    }

  return sp;
}

void
txvu_write_sp (val)
     CORE_ADDR val;
{
  switch (txvu_cpu)
    {
    case TXVU_CPU_VU0:
      val >>= 3;	/* Convert from bytes to double words */
      write_register (VU0_SP_REGNUM, val);
      break;

    case TXVU_CPU_VU1:
      val >>= 3;	/* Convert from bytes to double words */
      write_register (VU1_SP_REGNUM, val);
      break;

    case TXVU_CPU_VIF0:
    case TXVU_CPU_VIF1:
      /* These have no stack. */
      break;

    case TXVU_CPU_MASTER:
    default:
      write_register (SP_REGNUM, val);
      break;
    }
}

CORE_ADDR
txvu_call_return_address ()
{
  CORE_ADDR ra;
  char buf[MAX_REGISTER_RAW_SIZE];

  switch (txvu_cpu)
    {
    case TXVU_CPU_MASTER:
    case TXVU_CPU_AUTO:
      return read_register (RA_REGNUM);

    case TXVU_CPU_VU0:
      read_register_gen (VU0_RA_REGNUM, buf);
      ra = extract_unsigned_integer (buf, 2);
      ra <<= 3;			/* Convert from double words to bytes */
      break;

    case TXVU_CPU_VU1:
      read_register_gen (VU1_RA_REGNUM, buf);
      ra = extract_unsigned_integer (buf, 2);
      ra <<= 3;			 /* Convert from double words to bytes */
      break;

    default:	/* no sub-routines for VIF */
      ra = 0;
      break;
    }

  return ra;
}

/* Heuristically determine if a pc is inside a leaf subroutine. */
static int 
addr_is_likely_ra (ra)
     CORE_ADDR ra;
{
  /* If they're following conventions, the return address is in VI15 */
  CORE_ADDR jaddr;
  char insn[TXVU_VU_INSN_SIZE]; 

  /* The BAL/JALR instruction is 2 double-words back. */
  jaddr = ra - 2*8;
  if (txvu_cpu == TXVU_CPU_VU0)	/* Move to the alias area */
    {
      if (ra > VU0_MEM0_SIZE)	/* too big */
	return 0;

      jaddr += VU0_MEM0_WINDOW_START;
    }      
  else
    {
      if (ra > VU1_MEM0_SIZE)	/* too big */
	return 0;

      jaddr += VU1_MEM0_WINDOW_START;
    }

  if (target_read_memory (jaddr, insn, TXVU_VU_INSN_SIZE))
    return 0;

  /* Check for BAL and JALR in MSB of lower insn (little-endian) */
  if (insn[3] == 0x42 || insn[3] == 0x45)
    return 1;

  return 0;
}

#if 0
/* This was used to guess if a subroutine is a leaf routine or not by looking
   at the return address in VI15. The routine was called as:

   #define FRAMELESS_FUNCTION_INVOCATION(FI, ISLEAF) { \
        ISLEAF = txvu_is_leaf_routine(FI); \
      }

   Unfortunately, its a bit too easy to find old, but valid, return addresses
   lying around. Consequently, the FFI macro always sets ISLEAF to 0.  */

int
txvu_is_leaf_routine (fi)
     struct frame_info *fi;
{
  CORE_ADDR ra = txvu_call_return_address (fi);

  if (addr_is_likely_ra (ra))
    {
      FRAME_FP (fi) = ra;
      return 1;
    }

  return 0;
}
#endif

extern void mips_init_frame_pc_first PARAMS ((int, struct frame_info *));

void txvu_init_frame_pc_first (fromleaf, prev)
     int fromleaf;
     struct frame_info *prev;
{
  switch (txvu_cpu)
    {
    case TXVU_CPU_AUTO:
    case TXVU_CPU_MASTER:
      mips_init_frame_pc_first (fromleaf, prev);
      break;

    case TXVU_CPU_VU0:
    case TXVU_CPU_VU1:
      /* txvu_is_leaf_routine (see above) should have set the frame address
	 if a valid return address was found.  */
      if (fromleaf)
	{
	  prev->pc = prev->frame - 2*8;
	  prev->frame = 0;
	}
      break;

    default:
      break;
    }
}

static void 
add_section_to_overlay_table (sect)
     struct obj_section *sect;
{
  int i = 0;

  while (i<otable_size && overlay_table[i].osect)
    i++;

  if (i >= otable_size)
    error ("too many overlays?\n");

  overlay_table[i].osect = sect;
  ++num_overlays;
}

static void 
init_overlay_table (objfile)
     struct objfile *objfile;
{
  struct obj_section *sect;

  ALL_OBJFILE_OSECTIONS (objfile, sect)
    {
      asection *asect = sect->the_bfd_section;

      if (section_is_overlay (asect))
	{
	  /* When the sections were added (in objfile::add_to_objfile_sections)
	     we hadn't yet relocated the overlays. 
	     Consequently, the obj_section entries have the original VMA of 
	     the phantom sections.  */
	  sect->addr = asect->vma;
	  sect->endaddr = sect->addr + bfd_section_size (objfile->obfd, asect);

	  add_section_to_overlay_table (sect);
	}
    }
}

static void 
clear_overlay_table()
{
  int i;

  for (i=0; i<otable_size; i++)
    {
      overlay_table[i].is_mapped = 0; 
      overlay_table[i].cpu = 0;
      overlay_table[i].start = 0;
      overlay_table[i].end = 0;
      overlay_table[i].osect = 0;
    }

  num_overlays = 0;
}

static void 
invalidate_overlay_map()
{
  int i;

  for (i=0; i<num_overlays; i++)
    {
      overlay_table[i].is_mapped = 0;
      overlay_table[i].cpu = 0;
      overlay_table[i].start = 0;
      overlay_table[i].end = 0;
    }
}

static struct overlay_map*
find_overlay_map (addr)
     CORE_ADDR addr;
{
  int i = 0;
  asection *asect;
  unsigned size;

  while (i<num_overlays)
    {
      asect = overlay_table[i].osect->the_bfd_section;
      size = bfd_get_section_size_before_reloc (asect);

      if (asect->lma<=addr && addr<(asect->lma + size))
	return &(overlay_table[i]);

      i++;
    }

  return NULL;
}

static int 
overlay_section_index (asect)
     asection *asect;
{
  int i = 0;

  while (i<num_overlays)
    {
      if (asect == overlay_table[i].osect->the_bfd_section)
	return i;

      i++;
    }

  return -1;
}

/* VU source-addr tracking tables - from sky-pke.h */
#define VU0_MEM0_SRCADDR_START 0xb9800000
#define VU0_MEM1_SRCADDR_START 0xb9804000
#define VU1_MEM0_SRCADDR_START 0xb9808000
#define VU1_MEM1_SRCADDR_START 0xb980C000

static void
update_overlay (cpu)
     enum txvu_cpu_context cpu;
{
  CORE_ADDR track_start;
  unsigned len, vu_addr = 0;
  CORE_ADDR track, src;
  char buf[4];
  struct overlay_map *prev, *omap = NULL;

  if (cpu == TXVU_CPU_VU0)
    {
      track_start = (CORE_ADDR) VU0_MEM0_SRCADDR_START;
      len = VU0_MEM0_SIZE >> 1;
    }
  else
    {
      track_start = (CORE_ADDR) VU1_MEM0_SRCADDR_START;
      len = VU1_MEM0_SIZE >> 1;
    }

  for (track=track_start; track<(track_start+len); track+=4, vu_addr+=8 )
    {
      if (target_read_memory (track, buf, 4))
	printf ("Can't read tracking table at %llx\n", track);
  
      if (*((int *) buf) == 0) /* nothing downloaded */
	continue;

      src = extract_unsigned_integer (buf, 4);

      prev = omap;
      omap = find_overlay_map (src);
      if (!omap)
	printf ("Can't find overlay section for %llx\n", src);

      if (omap->cpu && omap->cpu != cpu)
	printf ("overlay is in both VUs\n");

      omap->cpu = cpu;

      if (omap != prev)
	{
	  /* We just changed overlay sections, so vu_addr must be the
	     start address for this section.  */
	  if (omap->start != 0)
	    printf ("update overlay: start already set\n");

	  if (omap->is_mapped)
	    printf ("update overlay: already mapped\n");

	  omap->start = vu_addr;
	}
      omap->end = vu_addr + 8;

      omap->is_mapped = 1; /* mark it as actively mapped */
    } 
}

/* As overlays are mapped in and out the VU memory can start to appear
   fragmented. This is because overlays do not have to start at the same
   VU address and do not have to have the same size. Although this residue
   code from previous mappings is legal (in some sense) the rest of GDB
   assumes that overlays are either entirely mapped of unmapped. 

   The following checks that the first VU resident address of an apparently
   active overlay corresponds to the VMA in the BFD section. The VMA is 
   gleaned from the MPG instruction so the two should correspond.

   Similarly, we can check that the end address corresponds to VMA+size.  */
static void
check_overlap (cpu)
     enum txvu_cpu_context cpu;
{
  int i;
  struct overlay_map *omap;
  unsigned size;
  asection *asect;

  for (i=0; i<num_overlays; i++)
    {
      omap = &(overlay_table[i]);
      if (omap->is_mapped && omap->cpu==cpu)
	{
	  asect = omap->osect->the_bfd_section;

	  if ((bfd_vma) omap->start != asect->vma)
	    omap->is_mapped = 0;

	  size = bfd_get_section_size_before_reloc (asect);
	  if ((bfd_vma) omap->end != asect->vma + size)
	    omap->is_mapped = 0;
	}
    }
}

static void
txvu_overlay_update (osect)
     struct obj_section *osect;
{
  int i;
  struct overlay_map *omap = NULL;

  /* We have to be careful here. symfile.c (overlay_is_mapped) uses the 
     ovly_mapped field of the obj_section osect. It also uses a cache_invalid
     flag to say that it doesn't have to call this routine. However, we want
     it to always call this routine, because the user may switch contexts 
     and we may return a different answer. On the other hand, we don't want
     to keep re-reading the tracking tables from the VIF. The solution is to
     have a local cache_invalid flag that is reset everytime we run, and
     leave the global flag untouched.  */

  if (txvu_overlay_cache_invalid)
    {
      invalidate_overlay_map();

#ifndef TARGET_SKY_B
      update_overlay (TXVU_CPU_VU0); 
      check_overlap  (TXVU_CPU_VU0);
#endif
      update_overlay (TXVU_CPU_VU1);
      check_overlap  (TXVU_CPU_VU1);

      txvu_overlay_cache_invalid = 0;
    }

  for (i=0; i<num_overlays; i++)
    {
      if (overlay_table[i].osect == osect)
	{
	  omap = &overlay_table[i];
	  break;
	}
    }

  overlay_cache_invalid = 1;
  osect->ovly_mapped = -1;

  if (!omap)		/* I actually don't think this is supposed to happen */
    return;

  if (!omap->is_mapped)
    return;

  switch (txvu_cpu)
    {
    case TXVU_CPU_VU0:
    case TXVU_CPU_VU1:
      if (omap->cpu == txvu_cpu)
	osect->ovly_mapped = 1;
      break;

    default:
      osect->ovly_mapped = 1;
      break;
    }
}

static void relocate_minimal_symbols PARAMS ((struct objfile *objfile,
					      asection *sect, CORE_ADDR vma));

static struct blockvector *copy_blockvector PARAMS ((struct objfile *objfile,
						     struct blockvector *src));

static void patch_addr_range PARAMS ((struct partial_symtab *pst,
				      struct symtab *symtab));

static void 
relocate_minimal_symbols (objfile, sect, vma)
     struct objfile *objfile;
     asection *sect;
     CORE_ADDR vma;
{
  /* Currently, the minimal symbols are linked to their absolute VU address
     instead of relative to the start of the overlay section that contains
     them. As a result, they do not have to be relocated.  */
#if 0
  struct minimal_symbol *msymbol;
  CORE_ADDR lma = sect->lma;

  ALL_OBJFILE_MSYMBOLS (objfile, msymbol)
    {
      if (SYMBOL_BFD_SECTION(msymbol) == sect)
	SYMBOL_VALUE_ADDRESS(msymbol) = (SYMBOL_VALUE_ADDRESS(msymbol) - lma) 
	  + vma;
    }
#endif
}

static struct blockvector *
copy_blockvector (objfile, src)
     struct objfile *objfile;
     struct blockvector *src;
{
  int i, nblocks = BLOCKVECTOR_NBLOCKS (src);
  struct block *srcblock, *dstblock;
  int blsize;
  struct blockvector *dst = 
    (struct blockvector *) obstack_alloc (&objfile -> symbol_obstack, 
					  (sizeof (struct blockvector) +
					  (nblocks-1)*sizeof (struct block*)));

  BLOCKVECTOR_NBLOCKS (dst) = nblocks;
  for (i=0; i<nblocks; i++)
    {
      srcblock = BLOCKVECTOR_BLOCK (src, i);
      blsize = 	sizeof (struct block) 
	+ (BLOCK_NSYMS (srcblock) * sizeof (struct symbol *));

      dstblock = (struct block *) obstack_alloc (&objfile -> symbol_obstack,
						 blsize);

      /* Blast everything across - the higher levels with sort through the
	 symbols. */
      memcpy (dstblock, srcblock, blsize);

      BLOCKVECTOR_BLOCK (dst, i) = dstblock;
    }

  return dst;
}

static void
patch_addr_range (pst, symtab)
     struct partial_symtab *pst;
     struct symtab *symtab;
{
  int low_not_set = 1;
  CORE_ADDR addr, low, high = 0;
  struct linetable *l = LINETABLE (symtab);
  int i, nlines = l->nitems;
  struct blockvector *bv;
  struct block *b;

  low_not_set = 1;
  low = high = 0;
  for (i=0; i<nlines; i++)
    {
      addr = l->item[i].pc;
      if (low_not_set)
	{
	  low = addr;
	  low_not_set = 0;
	}
      else if (addr < low)
	low = addr;

      if (addr > high)
	high = addr;
    }

  high += 8; /* Allow for the last instruction */

  /* Fix the psymtab... */
  pst->textlow = low;
  pst->texthigh = high; 

  /* ...and then the symtab itself. While there may be local blocks, they
     are not text addresses, otherwise the text range would have been set. */

  bv = BLOCKVECTOR(symtab);
  for (i=GLOBAL_BLOCK; i<=STATIC_BLOCK; i++)
    {
      b = BLOCKVECTOR_BLOCK (bv, i);
      BLOCK_START(b) = low;
      BLOCK_END(b) = high;
    }
}

/* Ovly_linetab is used to split the linetables and symbol blocks into
   their respective overlay portions.  */

struct psym_sublist
{
  int npsyms;
  struct partial_symbol *psym[1];
};

struct ovly_linetab 
{
  int low_not_set;
  CORE_ADDR low;
  CORE_ADDR high;
  struct psym_sublist *psym_sublist[2];
  struct subfile *subfile;
  struct pending *pending[2];
  struct pending_block *pending_blocks;
};

/* The main linetab is for things that are not in an overlay section. */
static struct ovly_linetab main_linetab;

/* The indices in this vector track those in the overlay_table.  */
static struct ovly_linetab overlay_linetab[OTABLE_SIZE];

/* Here are the local routines that make use of the overlay linetables in
   some way.  */
static void init_linetab PARAMS ((struct ovly_linetab *linetab));
static void init_overlay_linetab PARAMS (());

static void free_linetab_pending_blocks PARAMS((struct ovly_linetab *linetab));
static void free_overlay_pending_blocks PARAMS (());

static void linetab_to_buildsym PARAMS ((struct ovly_linetab *linetab, 
					 struct symtab *s));

static struct ovly_linetab* select_linetab PARAMS((CORE_ADDR p, asection **s));
static struct ovly_linetab* find_next_linetab PARAMS (());

static void compress_blockvector PARAMS ((struct blockvector *bv,
					  struct ovly_linetab *linetab));
static void inherit_statics PARAMS ((struct ovly_linetab *linetab));
static void lose_loners PARAMS ((struct ovly_linetab *ovlt));

static void build_overlay_linetables PARAMS ((struct linetable *l));
static void build_overlay_symbols PARAMS ((struct symtab *symtab));

static int check_psymbol_list PARAMS ((struct psymbol_allocation_list *list,
				       int offset, int count));
static void build_psym_sublists PARAMS((int lindex,
					  struct psymbol_allocation_list *list,
					  int offset, int count));
static void build_overlay_psymbols PARAMS ((struct objfile *objfile,
					    struct partial_symtab *pst));

static int repartition_psym_sublist PARAMS ((struct psymbol_allocation_list*,
					     struct psym_sublist *sublist,
					     int offset));
static void repartition_psymtab PARAMS ((struct objfile *objfile,
					 struct partial_symtab *pst,
					 struct ovly_linetab *linetab));

/* The main driver for the psymtab/symtab restructuring */
static void relocate_symtabs PARAMS ((struct objfile *objfile));

static void
init_linetab (linetab)
     struct ovly_linetab *linetab;
{
  linetab->low_not_set = 1;
  linetab->high = 0;

  if (!linetab->subfile)
    {
      linetab->subfile = (struct subfile *) xmalloc 
	(sizeof (struct subfile));

      memset (linetab->subfile, 0, sizeof (struct subfile));
    }
  else
    {
      if (linetab->subfile->line_vector)
	linetab->subfile->line_vector->nitems = 0;
    }

  linetab->psym_sublist[GLOBAL_BLOCK] = NULL;
  linetab->psym_sublist[STATIC_BLOCK] = NULL;

  linetab->pending[GLOBAL_BLOCK] = NULL;
  linetab->pending[STATIC_BLOCK] = NULL;
}

static void
init_overlay_linetab ()
{
  int i;

  init_linetab (&main_linetab);

  for (i=0; i<num_overlays; i++)
    init_linetab (&overlay_linetab[i]);
}

static void
free_linetab_pending_blocks (linetab)
     struct ovly_linetab *linetab;
{
  struct pending_block *p;

  /* buildsym.c uses the symbol_obstack for these. We don't. */
  while (linetab->pending_blocks)
    {
      p = linetab->pending_blocks;
      linetab->pending_blocks = p->next;

      free (p);
    }

  if (linetab->psym_sublist[GLOBAL_BLOCK])
    {
      free (linetab->psym_sublist[GLOBAL_BLOCK]);
      linetab->psym_sublist[GLOBAL_BLOCK] = NULL;
    }

  if (linetab->psym_sublist[STATIC_BLOCK])
    {
      free (linetab->psym_sublist[STATIC_BLOCK]);
      linetab->psym_sublist[STATIC_BLOCK] = NULL;
    }
}

static void 
free_overlay_pending_blocks ()
{
  int i;

  free_linetab_pending_blocks (&main_linetab);

  for (i=0; i<num_overlays; i++)
    free_linetab_pending_blocks (&overlay_linetab[i]);
}

/* Put a linetable subfile in the buildsym subfiles list to prepare for
   call to end_psymtab.  Set the other globals end_symtab needs.  */

static void
linetab_to_buildsym (linetab, s)
     struct ovly_linetab *linetab;
     struct symtab *s;
{
  linetab->subfile->name = savestring (s->filename, strlen (s->filename));
  linetab->subfile->dirname =
    (s->dirname == NULL) ? NULL : savestring (s->dirname, strlen (s->dirname));

  linetab->subfile->debugformat =
    (s->debugformat == NULL) ? NULL : savestring (s->debugformat, 
						  strlen (s->debugformat));
  linetab->subfile->language = s->language;

  subfiles = linetab->subfile;
  linetab->subfile = NULL;	/* will be free'd in end_symtab */

  last_source_start_addr = linetab->low;

  file_symbols = linetab->pending[STATIC_BLOCK];
  global_symbols = linetab->pending[GLOBAL_BLOCK];

  pending_blocks = linetab->pending_blocks;
}

/* Find the overlay section and linetab for the current linetable entry */

static struct ovly_linetab*
select_linetab (pc, osect)
     CORE_ADDR pc;
     asection **osect;
{
  struct ovly_linetab *linetab;
  int index;

  *osect = find_pc_overlay (pc);
  if (*osect == NULL)
    linetab = &main_linetab;	/* not an overlay */
  else
    {
      index = overlay_section_index (*osect);
      if (index >= 0)
	linetab = &overlay_linetab[index];
      else
	{
	  /* This really shouldn't happen, but there's really no reason to
	     stop if it does.  */

	  fprintf_unfiltered( gdb_stderr, "Overlay section for %s not found\n",
			      (*osect)->name);
	  *osect = NULL;
	  linetab = &main_linetab;
	}    
    }

  return linetab;
}

/* Find the next linetab that still has valid info (low_not_set == 0).  */

static struct ovly_linetab*
find_next_linetab()
{
  int i;

  if (main_linetab.low_not_set == 0)
    return &main_linetab;
  else
    {
      for (i=0; i<num_overlays; i++)
	{
	  if (overlay_linetab[i].low_not_set == 0)
	    return &(overlay_linetab[i]);
	}
    }

  return NULL;
}

/* All the symbols for the source file were originally in the main blockvector.
   The ones that are moving into overlay symtabs must be removed from the
   main blockvector. Since the symbols removed could be anywhere in the
   symbols array, it must be compressed to get rid of the holes.  */
   
static void
compress_blockvector (bv, linetab)
     struct blockvector *bv;
     struct ovly_linetab *linetab;
{
  struct block *block;
  struct pending *p;
  int n, i, j, cursor;
  int npending, b_nsyms;
  struct pending_block *pb;

  for (n=GLOBAL_BLOCK; n<=STATIC_BLOCK; n++)
    {
      block = BLOCKVECTOR_BLOCK (bv, n);
      b_nsyms = BLOCK_NSYMS (block);

      if (b_nsyms == 0) /* nothing to compress */
	continue;

      p = linetab->pending[n];
      if (p)
	{
	  if (p->nsyms > b_nsyms)
	    printf ("more syms to remove than we have?\n");

	  if (p->nsyms == b_nsyms)
	    {
	      /* Remove them all */
	      BLOCK_NSYMS (block) = 0;
	      continue;
	    }

	  /* First NULL out the symbols */
	  for (i=0; i<p->nsyms; i++)
	    {
	      for (j=0; j<b_nsyms; j++)
		if (BLOCK_SYM (block, j) == p->symbol[i])
		  {
		    BLOCK_SYM (block, j) = NULL;
		    break;
		  }

	      if (j == b_nsyms)
		printf ("couldn't remove %s\n", SYMBOL_NAME (p->symbol[i]));
	    }

	  /* Then compress the array */
	  for (cursor=i=0; i<(b_nsyms - p->nsyms); i++)
	    {
	      if (BLOCK_SYM (block, cursor) == NULL)
		{
		  for (j=cursor+1; j<b_nsyms; j++)
		    BLOCK_SYM (block, j-1) = BLOCK_SYM (block, j);

		  /* Don't advance cursor: the new entry may be null too. */
		}
	      else
		cursor ++;
	    }

	  BLOCK_NSYMS (block) = BLOCK_NSYMS (block) - p->nsyms;
	}
    }

  /* Now remove the local symbols */

  pb = linetab->pending_blocks;
  if (!pb) /* None to remove */
    return;

  /* Count the pending blocks to see if we remove everything. */
  for (npending=0; pb; pb = pb->next) npending++;

  if (npending == (BLOCKVECTOR_NBLOCKS (bv) - 2))
    {
      BLOCKVECTOR_NBLOCKS (bv) = 2; /* remove them all */
      return;
    }

  /* As before, we first remove the blocks, then compress the array */
  for (pb=linetab->pending_blocks; pb; pb = pb->next)
    {
      for (n=STATIC_BLOCK+1; n<BLOCKVECTOR_NBLOCKS (bv); n++)
	if (pb->block == BLOCKVECTOR_BLOCK (bv, n))
	  {
	    BLOCKVECTOR_BLOCK (bv, n) = 0;
	    break;
	  }

      if (n == BLOCKVECTOR_NBLOCKS (bv))
	printf ("couldn't remove block %s\n", 
		SYMBOL_NAME (pb->block->function));
    }

  /* Now compress */
  for (cursor=n=STATIC_BLOCK+1; n<(BLOCKVECTOR_NBLOCKS (bv) - npending); n++)
    {
      if (BLOCKVECTOR_BLOCK (bv, cursor) == 0)
	{
	  for (i=cursor+1; i<BLOCKVECTOR_NBLOCKS (bv); i++)
	    BLOCKVECTOR_BLOCK (bv, i-1) = BLOCKVECTOR_BLOCK (bv, i);

	  /* Don't advance cursor: the new entry may be null too. */
	}
      else
	cursor ++;
    }

  BLOCKVECTOR_NBLOCKS (bv) = BLOCKVECTOR_NBLOCKS (bv) - npending;
}

/* If the original symtab has no global symbols left (because they've all been
   moved to included files) then we should move the statics (mostly typedefs
   and stuff) to the included file as well. LINETAB is the overlay linetable
   that will be used to build the included file's symtab.  */
   
static void
inherit_statics (linetab)
     struct ovly_linetab *linetab;
{
  int i;

  if (!linetab->pending[STATIC_BLOCK])
    {
      linetab->pending[STATIC_BLOCK] = main_linetab.pending[STATIC_BLOCK];
      main_linetab.pending[STATIC_BLOCK] = NULL;
    }
  else
    {
      for (i=0; i<main_linetab.pending[STATIC_BLOCK]->nsyms; i++)
	add_symbol_to_list (main_linetab.pending[STATIC_BLOCK]->symbol[i],
			      &(linetab->pending[STATIC_BLOCK]));
    }

  /* Do the same for the psymbols  */
  if (!linetab->psym_sublist[STATIC_BLOCK])
    {
      linetab->psym_sublist[STATIC_BLOCK] 
	= main_linetab.psym_sublist[STATIC_BLOCK];
      main_linetab.psym_sublist[STATIC_BLOCK] = NULL;
    }
  else
    {
      int n = linetab->psym_sublist[STATIC_BLOCK]->npsyms;

      for (i=0; i<main_linetab.psym_sublist[STATIC_BLOCK]->npsyms; i++)
	linetab->psym_sublist[STATIC_BLOCK]->psym[n++] =
	  linetab->psym_sublist[STATIC_BLOCK]->psym[i];

      linetab->psym_sublist[STATIC_BLOCK]->npsyms = n;
    }

}

/* It often happens that there are 1 or 2 "outlier" lines from things like
   automatically inserted MPG instructions, etc. Don't bother to build whole
   symtabs just for these loners. */

static void 
lose_loners (ovlt)
     struct ovly_linetab *ovlt;
{
  struct linetable *l;
  int i, contiguous = 0;

  if (ovlt->low_not_set)
    return;	/* nothing for this overlay */

  l = ovlt->subfile->line_vector;
  if (!l || l->nitems > 10)
    return;

  /* A potential loner - it has 10 or fewer lines. To avoid discarding 
     legitimate, but small, segments we see if any of the lines are
     contiguous. Loners are usually scattered here and there and are not
     contiguous.  */

  for (i=0; i<l->nitems-1; i++)
    {
      if (abs (l->item[i+1].line - l->item[i].line) < 2)
	{
	  contiguous = 1;
	  break;
	}
    }

  if (!contiguous)
    ovlt->low_not_set = 1;	/* Lose it */
}

/* Split a given linetable into several - one for each overlay referenced, 
   and a default main one for non-overlay addresses. The overlay linetable
   parallels the main overlay table, so that there is (at most) one linetable
   per overlay section. The rationale is that overlays are contiguous ranges
   of addresses, and the only discontinuities that exist are at overlay 
   boundaries.  */

static void
build_overlay_linetables (l)
     struct linetable *l;
{
  int i, nlines = l->nitems;
  CORE_ADDR addr;
  asection *osect;
  struct ovly_linetab *current_linetab;

  current_linetab = select_linetab (l->item[0].pc, &osect);

  for (i=0; i<nlines; i++)
    {
      addr = overlay_mapped_address (l->item[i].pc, osect);

      if (addr == l->item[i].pc)
	{
	  current_linetab = select_linetab (l->item[i].pc, &osect);

	  addr = overlay_mapped_address (l->item[i].pc, osect);
	}

      record_line (current_linetab->subfile, l->item[i].line, addr);

      if (current_linetab->low_not_set)
	{
	  current_linetab->low = addr;
	  current_linetab->low_not_set = 0;
	}
      else if (addr < current_linetab->low)
	{
	  if (!current_linetab->low_not_set)
	    printf ("Hey! I thought line numbers were sorted!\n" );

	  current_linetab->low = addr;
	}

      if ((addr + 8) > current_linetab->high)
	current_linetab->high = addr + 8; /* Allow for last instruction */
    }

  lose_loners (&main_linetab);
  for (i=0; i<num_overlays; i++)
    lose_loners (&overlay_linetab[i]);
}

static void
build_overlay_symbols (symtab)
     struct symtab *symtab;
{
  int i, n;
  asection *osect = NULL;
  struct blockvector *blockvector = BLOCKVECTOR (symtab);
  struct block *outer, *inner;
  struct pending_block *p;
  struct symbol *s;
  CORE_ADDR addr;
  unsigned size;
  struct ovly_linetab *linetab;

  /* First build the global/static pending lists */
  for (i=GLOBAL_BLOCK; i<=STATIC_BLOCK; i++)
    {
      outer = BLOCKVECTOR_BLOCK (blockvector, i);

      for (n=0; n<BLOCK_NSYMS(outer); n++)
	{
	  s = BLOCK_SYM (outer, n);
	  
	  switch (SYMBOL_CLASS (s))
	    {
	    case LOC_LABEL:
	    case LOC_STATIC:
	      addr = SYMBOL_VALUE_ADDRESS (s);
	      linetab = select_linetab (addr, &osect);

	      if (osect)
		{
		  addr = overlay_mapped_address (addr, osect);
		  SYMBOL_VALUE_ADDRESS (s) = addr;
		}

	      add_symbol_to_list (s, &(linetab->pending[i]));
	      break;

	    case LOC_BLOCK:
	      inner = (SYMBOL_BLOCK_VALUE (s));
	      addr = BLOCK_START (inner);
	      linetab = select_linetab (addr, &osect);

	      add_symbol_to_list (s, &(linetab->pending[i]));
	      if (linetab->low_not_set)
		printf ("block: low_not_set for linetab\n");
	      break;

	    default:
	      add_symbol_to_list (s, &(main_linetab.pending[i]));
	      if (linetab->low_not_set)
		printf ("default: low_not_set for linetab\n");
	      break;
	    }
	}
    }

  /* Now put the local blocks that follow on the pending block list.
     Process the blocks in reverse order since we insert them at the
     head (which reverses them again :-).  */

  for (i=BLOCKVECTOR_NBLOCKS(blockvector)-1; i>STATIC_BLOCK; i--)
    {
      inner = BLOCKVECTOR_BLOCK (blockvector, i);

      p = (struct pending_block *) xmalloc (sizeof (struct pending_block));
      p->block = inner;

      linetab = select_linetab (BLOCK_START (inner), &osect);
      p->next = linetab->pending_blocks;
      linetab->pending_blocks = p;

      if (osect)
	{
	  /* Turns out it is pretty common for functions to cross overlay 
	     boundaries (DMA packets have a relatively small max size). 
	     The functions themselves are still contiguous in VU space. 
	     Using the original size to set the overlay extent is the safest 
	     way through this one.  */

	  size = BLOCK_END (inner) - BLOCK_START (inner);
	  addr = overlay_mapped_address (BLOCK_START (inner), osect);
	  BLOCK_START (inner) = addr;
	  BLOCK_END (inner) = addr + size;
	}

      if (linetab->low_not_set)
	{
	  linetab->low = BLOCK_START (inner);
	  linetab->high = BLOCK_END (inner);
	  linetab->low_not_set = 0;
	}
    }
}

/* Check the partial_symbols for a particular allocation list of a file.
   Return TRUE if any are in overlays.  */

static int
check_psymbol_list (list, offset, count)
     struct psymbol_allocation_list *list;
     int offset, count;
{
  int i;
  asection *sect;
  struct partial_symbol *ps, **p;

  for (i=0; i<count; i++)
    {
      p = list->list + offset + i;
      ps = *p;
	      
      if (SYMBOL_NAMESPACE(ps)==VAR_NAMESPACE && SYMBOL_CLASS(ps)==LOC_BLOCK)
	{
	  sect = find_pc_overlay (SYMBOL_VALUE_ADDRESS (ps));
	  if (sect)
	    return 1;
	}
    }

  return 0;
}

/* Check the partial_symbols for a particular allocation list of a file.
   Relocate any that are in overlays, and separate them into the overlay
   table.  */

static void
build_psym_sublists (lindex, list, offset, count)
     int lindex;	/* The list index - global or static */
     struct psymbol_allocation_list *list;
     int offset, count;
{
  int i, n;
  asection *osect = NULL;
  CORE_ADDR addr;
  struct partial_symbol *ps;
  struct ovly_linetab *linetab;

  for (i=0; i<count; i++)
    {
      ps = list->list[offset + i];
	      
      if (SYMBOL_NAMESPACE(ps)==VAR_NAMESPACE && SYMBOL_CLASS(ps)==LOC_BLOCK)
	{
	  addr = SYMBOL_VALUE_ADDRESS (ps);
	  linetab = select_linetab (addr, &osect);

	  if (osect)	/* Relocate the psym */
	    {
	      SYMBOL_VALUE_ADDRESS (ps) = overlay_mapped_address (addr, osect);
	      SYMBOL_BFD_SECTION (ps) = osect;
	    }
	}
      else
	linetab = &main_linetab;

      /* Add the symbol to the appropriate psym_sublist */
      if (!linetab->psym_sublist[lindex])
	{
	  linetab->psym_sublist[lindex] = (struct psym_sublist *) xmalloc 
	    (sizeof (struct psym_sublist) + 
	     (count-1)*sizeof (struct partial_symbol *));
	  linetab->psym_sublist[lindex]->npsyms = 0;
	}

      n = linetab->psym_sublist[lindex]->npsyms++;
      linetab->psym_sublist[lindex]->psym[n] = ps;
    }
}

static int
repartition_psym_sublist (list, sublist, offset)
     struct psymbol_allocation_list *list;
     struct psym_sublist *sublist;
     int offset;
{
  int i;

  for (i=0; i<sublist->npsyms; i++)
    list->list[offset++] = sublist->psym[i];

  return offset;
}

static int goffset, soffset;

static void 
repartition_psymtab (objfile, pst, linetab)
     struct objfile *objfile;
     struct partial_symtab *pst;
     struct ovly_linetab *linetab;
{
  if (linetab->psym_sublist[GLOBAL_BLOCK])
    {
      pst->globals_offset = goffset;
      goffset = repartition_psym_sublist (&objfile->global_psymbols,
					  linetab->psym_sublist[GLOBAL_BLOCK],
					  goffset);

      pst->n_global_syms = linetab->psym_sublist[GLOBAL_BLOCK]->npsyms;
    }
  else
    pst->n_global_syms = 0;

  if (linetab->psym_sublist[STATIC_BLOCK])
    {
      pst->statics_offset = soffset;
      soffset = repartition_psym_sublist (&objfile->static_psymbols,
					  linetab->psym_sublist[STATIC_BLOCK],
					  soffset);

      pst->n_static_syms = linetab->psym_sublist[STATIC_BLOCK]->npsyms;
    }
  else
    pst->n_static_syms = 0;
}

static void 
build_overlay_psymbols (objfile, pst)
     struct objfile *objfile;
     struct partial_symtab *pst;
{
  build_psym_sublists (GLOBAL_BLOCK, &objfile->global_psymbols, 
			 pst->globals_offset, pst->n_global_syms);

  build_psym_sublists (STATIC_BLOCK, &objfile->static_psymbols, 
			 pst->statics_offset, pst->n_static_syms);
}

static void
print_overlay_linetab (linetab)
     struct ovly_linetab *linetab;
{
  int i, n;
  struct pending_block *p;

  if (linetab->low_not_set)
    {
      printf ("not used?\n");
    }

  printf ("text range %llx->%llx\n", linetab->low, linetab->high);

  if (linetab->psym_sublist[GLOBAL_BLOCK])
    {
      n = linetab->psym_sublist[GLOBAL_BLOCK]->npsyms;
      printf ("%d Global Psyms: ", n);

      for (i=0; i<n; i++)
	printf ("%s ", SYMBOL_NAME (linetab->psym_sublist[0]->psym[i]));
      printf ("\n");
    }

  if (linetab->psym_sublist[STATIC_BLOCK])
    {
      n = linetab->psym_sublist[STATIC_BLOCK]->npsyms;
      printf ("%d Static Psyms: ", n);

      for (i=0; i<n; i++)
	printf ("%s ", SYMBOL_NAME (linetab->psym_sublist[1]->psym[i]));
      printf ("\n");
    }

  if (linetab->pending[GLOBAL_BLOCK])
    {
      n = linetab->pending[GLOBAL_BLOCK]->nsyms;
      printf ("%d Globals: ", n);

      for (i=0; i<n; i++)
	printf ("%s ", SYMBOL_NAME (linetab->pending[0]->symbol[i]));
      printf ("\n");
    }

  if (linetab->pending[STATIC_BLOCK])
    {
      n = linetab->pending[STATIC_BLOCK]->nsyms;
      printf ("%d Statics: ", n);

      for (i=0; i<n; i++)
	printf ("%s ", SYMBOL_NAME (linetab->pending[1]->symbol[i]));
      printf ("\n");
    }

  if (linetab->pending_blocks)
    {
      printf ("pending blocks: ");
      for (p=linetab->pending_blocks; p; p = p->next)
	{
	  if (!p->block->function)
	    printf ("?? ");
	  else
	    printf ("%s ", SYMBOL_NAME (p->block->function));
	}
      printf ("\n");
    }
}

/* Part of setting up the overlays is relocating the line numbers and symbols
   in the debug info to their VMA address.  */

static void
relocate_symtabs (objfile)
     struct objfile *objfile;
{
  struct partial_symtab *pst;
  struct symtab *s;
  asection *osect;
  int i, len;

  /* First read all the symbols for the source files we suspect have only
     line numbers. The heuristic is that the textlow/texthigh range is set
     to a null range. This means there was some debug info in the file
     (or we would not have a partial symtab entry) but there were no stabs
     for symbols (or we would have a non-null text range).

     We also have to read the line numbers for sources that have stabs and
     the text range is in an overlay section.  */

  ALL_OBJFILE_PSYMTABS (objfile, pst)
    {
      /* Included files can cause the files that include them to be read in */
      if (pst->readin)	
	continue;

      if (pst->textlow == pst->texthigh)
	pst->read_symtab (pst);
      else
	{
	  /* If we have function symbols, then texthigh and textlow should
	     at least be set to the bounds of the function. However, the
	     symbols must be checked to see if any have addresses that lie in 
	     an overlay section. If so, read in the whole symtab.  */

	  if (check_psymbol_list (&objfile->global_psymbols, 
				 pst->globals_offset, pst->n_global_syms)
	      || check_psymbol_list (&objfile->static_psymbols, 
				    pst->statics_offset, pst->n_static_syms))
	    {
		pst->read_symtab (pst);
	    }
	}
    }

  /* Now find all the symtabs that have line tables and relocate them if
     necessary.  */
  ALL_OBJFILE_SYMTABS (objfile, s)
    {
      struct linetable *l = LINETABLE (s);
      struct blockvector *bv;
      struct block *b;
      struct ovly_linetab *linetab;

      if (!l)
	continue;

      /* Get the partial symtab corresponding to s.  Its possible that there 
	 is more than one pst with this filename.  */

      ALL_OBJFILE_PSYMTABS (objfile, pst)
	{
	  if (!pst->readin)
	    continue;

	  if (pst->symtab == s)
	    break;
	  else if (!pst->symtab)
	    {
	      if (STREQ (s->filename, pst->filename))
		break;
	    }
	}

      if (!pst) 
	printf ("no pst for symtab %s?\n", s->filename);

      /* Figure out whether this file is just assembler source with lines or
	 if it also has overlay sections to relocate.  */

      osect = NULL;
      for (i=0; i<l->nitems; i++)
	{
	  osect = find_pc_overlay (l->item[i].pc);
	  if (osect)
	    break;
	}

      if (!osect) /* no line numbers or symbols in an overlay section */
	{
	  /* However, we did ask for this symtab to be built, so its 
	     probably a file that contains only line numbers. 
	     Fix textlow and texthigh in the symtab and psymtab.  */

	  b = BLOCKVECTOR_BLOCK (BLOCKVECTOR(s), GLOBAL_BLOCK);
	  if (pst->textlow==pst->texthigh && BLOCK_START(b)==BLOCK_END(b))
	    patch_addr_range (pst, s);

	  continue;
	}

      /* This symtab has overlay symbols and/or lines. GDB assumes each symtab
	 covers a single contiguous address range, which overlays break.
	 Here we split the symtabs (and psymtabs) to separate contiguous 
	 ranges.  */

      init_overlay_linetab ();

      build_overlay_linetables (l);
      build_overlay_symbols (s);

      build_overlay_psymbols (objfile, pst);

      if (pst->readin && !pst->symtab)
	{
	  /* This symtab is for an included file, so gdb has bundled its
	     line numbers (and block vectors :-( in with the including 
	     file. This would be ok except that part of the file is in an 
	     overlay, so it needs its own symtab. */

	  /* Make a copy of the block vector, since we don't want to
	     share it with the file that includes us.  */

	  BLOCKVECTOR(s) = copy_blockvector (objfile, BLOCKVECTOR (s));

	  pst->symtab = s;		/* no longer "included" */

	  /* The file that included this one has all the symbols. They will
	     be moved to the included symtab below. Here we remove the overlay
	     symbols from the parent. */

	  linetab = find_next_linetab();

	  for (i=0; i<pst->number_of_dependencies; i++)
	    {
	      struct partial_symtab *dep = pst->dependencies[i];
	      struct symtab *parent = dep->symtab;

	      if (!parent)
		printf ("File %s (which includes %s), was not processed!\n",
			dep->filename, s->filename);

	      compress_blockvector (BLOCKVECTOR (parent), linetab);
	    }

	  if (!main_linetab.pending[GLOBAL_BLOCK] 
	      && main_linetab.pending[STATIC_BLOCK])
	    inherit_statics (linetab);

	  s->primary = 1;
	}

      /* First fix-up the original symtab. */

      /* Switch in the new linetable. The new linetable can be no
	 larger than the original, so we just use the original.  */

      linetab = find_next_linetab();
      if (linetab == NULL)
	printf ("Lost my linetabs?\n");

      len = linetab->subfile->line_vector->nitems;
      memcpy (s->linetable, linetab->subfile->line_vector, 
	      sizeof (struct linetable) + len*sizeof (struct linetable_entry));

      /* Install the psyms for this psymtab. */

      goffset = pst->globals_offset;
      soffset = pst->statics_offset;
      repartition_psymtab (objfile, pst, linetab);

      /* Fix the address ranges in both the partial symtab and the
	 full symtab.  */

      pst->textlow = linetab->low;
      pst->texthigh = linetab->high;

      bv = BLOCKVECTOR(s);

      for (i=GLOBAL_BLOCK; i<=STATIC_BLOCK; i++)
	{
	  b = BLOCKVECTOR_BLOCK (bv, i);

	  BLOCK_START(b) = linetab->low;
	  BLOCK_END(b) = linetab->high;
	}

      linetab->low_not_set = 1;	/* mark entry as unused */

      /* Now create new symtabs for the other overlay ranges. */

      linetab = find_next_linetab();
      while (linetab)
	{
	  compress_blockvector (BLOCKVECTOR (s), linetab);

	  if (!main_linetab.pending[GLOBAL_BLOCK] 
	      && main_linetab.pending[STATIC_BLOCK])
	    inherit_statics (linetab);

	  pst = allocate_psymtab (s->filename, objfile);
	  
	  pst->section_offsets = objfile->section_offsets;

	  repartition_psymtab (objfile, pst, linetab);

	  linetab_to_buildsym (linetab, s);

	  pst->symtab = end_symtab (linetab->high, objfile, 
				    s->block_line_section);
	  pst->readin = 1;
	  pst->textlow = linetab->low;
	  pst->texthigh = linetab->high;

	  pst->symtab->primary = 1;

	  linetab->low_not_set = 1;
	  linetab = find_next_linetab();
	}

      free_overlay_pending_blocks();
    }

  really_free_pendings (0);
}

typedef unsigned ovly_tbl_el_typ;
#define OVLY_TBL_EL_SZ	(sizeof(ovly_tbl_el_typ))

void
txvu_symfile_postread (objfile)
     struct objfile *objfile;
{
  asection *tsect, *osect;
  char *sname, *ovly_tab, *ovly_strtab;
  bfd_vma strtab_lma;
  int i, len, size;

  if (objfile->obfd == NULL)
    error ("No bfd for objfile %s\n", objfile->name);

  clear_overlay_table();

  tsect = bfd_get_section_by_name (objfile->obfd, ".DVP.ovlytab");
  if (tsect == NULL)
    return;	/* no overlays */

  /* Get the overlay table mappings from the executable */
  size = bfd_get_section_size_before_reloc (tsect);
  ovly_tab = xmalloc (size);
  bfd_get_section_contents (objfile->obfd, tsect, ovly_tab, 0, size);

  tsect = bfd_get_section_by_name (objfile->obfd, ".DVP.ovlystrtab");
  if (tsect == NULL)
    error ("No string table for overlay table\n");

  /* Get the overlay table string tab from the executable. */
  len = bfd_get_section_size_before_reloc (tsect);
  strtab_lma = bfd_section_lma (objfile->obfd, tsect);
  ovly_strtab = xmalloc (len);
  bfd_get_section_contents (objfile->obfd, tsect, ovly_strtab, 0, len);

  /* Each entry in the overlay table has 3 values (each is size bfd_vma):
     - offset into overlay strtab for section name
     - lma of phantom section
     - vma of phantom section
     */

  for (i=0; i<size; i+=3*OVLY_TBL_EL_SZ)
    {
      ovly_tbl_el_typ str_off;
      bfd_vma lma, vma;

      str_off = extract_unsigned_integer (ovly_tab + i, OVLY_TBL_EL_SZ);
      sname = ovly_strtab + (str_off - strtab_lma);

      osect = bfd_get_section_by_name (objfile->obfd, sname);
      if (osect == NULL)
	error ("Can't find overlay section %s\n", sname);

      lma = extract_unsigned_integer (ovly_tab + i + OVLY_TBL_EL_SZ, 
				      OVLY_TBL_EL_SZ);
      vma = extract_unsigned_integer (ovly_tab + i + 2*OVLY_TBL_EL_SZ,
				      OVLY_TBL_EL_SZ);

      relocate_minimal_symbols (objfile, osect, vma);

      osect->lma = lma;
      osect->vma = vma;
    }
 
  msymbols_sort (objfile);

  free (ovly_tab);
  free (ovly_strtab);

  init_overlay_table (objfile);

  relocate_symtabs (objfile);
}

int
txvu_is_phantom_section (asect)
     asection *asect;
{
  if (!strncmp (asect->name, ".DVP.overlay", strlen (".DVP.overlay")))
    return 1;

  return 0;
}

static void 
txvu_do_fp_row (regnum,ncols)
     int regnum;
     int ncols;
{
  int i, j, step;
  int inv;
  double flt;
  char regbuf[MAX_REGISTER_RAW_SIZE];

  if (printvector_order == ORDER_XYZW)
    {
      j = 0;
      step = 1;
    }
  else /* print in wzyx order */
    {
      j = ncols - 1;
      step = -1;
    }

  for (i = 0; i < ncols; i++, j+=step) 
    {
      if (read_relative_register_raw_bytes (regnum + j, regbuf))
	error ("can't read register %s", REGISTER_NAME (regnum + j));

      flt = unpack_double (builtin_type_float, regbuf, &inv);
      if (inv)
	printf_filtered ("<invalid float>" );
      else
	printf_filtered ("%15.7g ", flt );
    }

  printf_filtered ("\n");
}

static void
txvu_print_register (regnum)
     int regnum;
{
  char *rname = REGISTER_NAME (regnum);
  int i, j, print_count, step;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  char vec_name[20];

  if (rname == 0 || *rname == '\0')
    return;

  /* If asked for vu[01]_vf??x print the entire four element vector.  */
  if ((STREQN (rname, "vu0_vf", 6) || STREQN (rname, "vu1_vf", 6))
      && *(rname + 8) == 'x')
    {
      memcpy (vec_name, rname, 8);
      if (printvector_order == ORDER_XYZW)
	{
	  memcpy (vec_name+8, "xyzw", 4); 
	  j = 0;
	  step = 1;
	}
      else
	{
	  memcpy (vec_name+8, "wzyx", 4); 
	  j = 3;
	  step = -1;
	}

      vec_name[12] = '\0';

      printf_filtered ("%s: ", vec_name);
      print_count = 4;
    }
  else
    {
      printf_filtered ("%s: ", REGISTER_NAME (regnum));
      print_count = 1;
      j = 0;
      step = 1;
    }

  for (i=0; i<print_count; i++, j+=step)
    {
      /* Get the data in raw format.  */
  
      if (read_relative_register_raw_bytes (regnum + j, raw_buffer))
	error ("can't read register %s", REGISTER_NAME (regnum + j));

      /* If virtual format is floating, print it that way.  */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum + j)) == TYPE_CODE_FLT)
	val_print (REGISTER_VIRTUAL_TYPE (regnum + j), raw_buffer, 0, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
      /* Else print as integer in hex.  */
      else
	print_scalar_formatted (raw_buffer, REGISTER_VIRTUAL_TYPE (regnum + j),
				'x', 0, gdb_stdout);

      printf_filtered (" ");
    }

  /* Note: new line not printed */
}

static void 
txvu_do_vu_registers (unit)
     int unit;
{
  int offset = NUM_CORE_REGS + unit*NUM_VU_REGS;
  int ncols = 4;
  int i, j, step;
  int inv, regnum;
  double flt;
  unsigned char regbuf[MAX_REGISTER_RAW_SIZE];

  /* First the 16 (visible) integer registers */
  for (i = offset; i < (offset+NUM_VU_INTEGER_REGS); i+=ncols)
    {
      /* print ncols integer registers per row */
      printf_filtered ("%s - %s:  ", REGISTER_NAME (i), REGISTER_NAME (i + ncols-1));

      for (j = 0; j < ncols; j++) 
	{
	  regnum = i + j;

	  if (read_relative_register_raw_bytes (regnum, regbuf))
	    error ("can't read register %d (%s)", regnum, REGISTER_NAME (regnum));

	  printf_filtered ("%02x%02x  ", regbuf[1], regbuf[0]);
	}

      printf_filtered ("\n");
    }

  /* Now the control/special registers */
  for (i = offset+NUM_VU_INTEGER_REGS; i < (offset+FIRST_VEC_REG-4); i+=ncols)
    {
      printf_filtered ("\n          ");
      for (j = i; j < i+ncols; j++ )
	{
	  if( *REGISTER_NAME (j) == '\0' )	/* Can happen for P register */
	    continue;

	  printf_filtered ("%15s ", REGISTER_NAME (j));
	}

      printf_filtered ("\n          ");
      for (j = i; j < i+ncols; j++ )
	{
	  if( *REGISTER_NAME (j) == '\0' )	/* Can happen for P register */
	    continue;
      
	  if (read_relative_register_raw_bytes (j, regbuf))
	    error ("can't read register %d (%s)", j, REGISTER_NAME (j));

	  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (j)) == TYPE_CODE_FLT)
	    {
	      flt = unpack_double (builtin_type_float, regbuf, &inv);
	      if (inv)
		printf_filtered ("<invalid float>" );
	      else
		printf_filtered ("%15.7g ", flt );
	    }
	  else
	    {
	      regnum = unpack_long (REGISTER_VIRTUAL_TYPE (j), regbuf);
	      printf_filtered ("       %08x ", regnum);
	    }
	}
    }

  /* Next is the ACC vector register */
  if (printvector_order == ORDER_XYZW)
    {
      j = 0;
      step = 1;
    }
  else /* print in wzyx order */
    {
      j = 3;
      step = -1;
    }

  regnum = offset + FIRST_VEC_REG - 4;
  printf_filtered ("\n          ");
  for (i = 0; i < 4; i++, j+=step )
    printf_filtered ("%15s ", REGISTER_NAME (regnum + j));

  printf_filtered ("\n          ");
  txvu_do_fp_row (regnum, ncols);

  /* Finally do the vector registers. Format the table with vector
     register name on left, and "x y z w" or "w z y x" across the top,
     as specified by printvector_order.  */
  
  printf_filtered ("\n                        ");

  if (printvector_order == ORDER_XYZW)
    {
      for (j = 0; j < 3; j++ )
	printf_filtered ("%c               ", 'x' + j);
      printf_filtered ("w\n");
    }
  else /* print in wzyx order */
    {
      printf_filtered ("w               ");
      for (j = 2; j >= 0; j-- )
	printf_filtered ("%c               ", 'x' + j);
      printf_filtered ("\n");
    }

  for (i = offset+FIRST_VEC_REG; i < (offset+NUM_VU_REGS); i+=ncols )
    {
      char vecbuf[10];

      strncpy (vecbuf, REGISTER_NAME (i), 8 );
      vecbuf[8] = '\0';
      printf_filtered ("%s: ", vecbuf);
      txvu_do_fp_row (i,  ncols);
    }
}

static void 
txvu_do_vif_registers (unit)
     int unit;
{
  int offset = NUM_CORE_REGS + 2*NUM_VU_REGS + unit*NUM_VIF_REGS;
  int ncols = 4;
  int i, j, regnum;
  char regbuf[MAX_REGISTER_RAW_SIZE];

  /* First the PC source address and index */
  txvu_print_register( offset + NUM_VIF_REGS-2 );
  printf_filtered ("    ");
  txvu_print_register( offset + NUM_VIF_REGS-1 );
  printf_filtered ("\n");

  for (i = offset; i < (offset+NUM_VIF_REGS-2); i+=ncols)
    {
      /* print ncols integer registers per row */
      for (j = 0; j < ncols; j++)
	{
	  if (*REGISTER_NAME (i+j) != '\0')
	    printf_filtered ("%15s ", REGISTER_NAME (i + j));
	}

      printf_filtered("\n       ");
      for (j = 0; j < ncols; j++) 
	{
	  int byte;

	  regnum = i + j;

	  if (*REGISTER_NAME (regnum) == '\0')
	    continue;

	  if (read_relative_register_raw_bytes (regnum, regbuf))
	    error ("can't read register %d (%s)", regnum, REGISTER_NAME (regnum));

	  for (byte = 3; byte >= 0; byte--)
	    printf_filtered ("%02x", (unsigned char) regbuf[byte]);
	  printf_filtered ("        ");

	}

      printf_filtered ("\n");
    }
}

/* TXVU_DO_REGISTERS_INFO(): called by "info register" command */

extern void mips_do_registers_info PARAMS ((int, int));

void
txvu_do_registers_info (regnum, fpregs)
     int regnum;
     int fpregs;
{
  if (regnum == -1)
    {
      switch( txvu_cpu )
	{
	case TXVU_CPU_MASTER:
	  mips_do_registers_info (regnum, fpregs);
	  break;

	case TXVU_CPU_VU0:
	  txvu_do_vu_registers (0);
	  break;

	case TXVU_CPU_VU1:
	  txvu_do_vu_registers (1);
	  break;

	case TXVU_CPU_VIF0:
	  txvu_do_vif_registers (0);
	  break;

	case TXVU_CPU_VIF1:
	  txvu_do_vif_registers (1);
	  break;

	default:
#ifndef TARGET_SKY_B
	  mips_do_registers_info (regnum, fpregs);
	  txvu_do_vu_registers (0);
#endif
	  txvu_do_vu_registers (1);
#ifndef TARGET_SKY_B
	  txvu_do_vif_registers (0);
#endif
	  txvu_do_vif_registers (1);
	  break;
	}
    }
  else 
    {
      if (regnum < NUM_CORE_REGS)
	mips_do_registers_info (regnum, fpregs);
      else
	{
	  txvu_print_register (regnum);
	  printf_filtered ("\n");
	}
    }
}

/* REGISTER_NAME_ALIAS_HOOK(): called by target_map_name_to_register().

   Allows context-sensitive abbreviations when either of the VUs is 
   selected as the current cpu, eg. vf01y instead of vu0_vf01y. */

int
txvu_register_name_alias (name, len)
     char *name;
     int len;
{
  char full_name[20];
  int regnum, start, count;

  /* We want to allow the vector form (eg. vu1_vf04) in any context.  */
  if (len==8 && (STREQN (name, "vu0_vf", 6) || STREQN (name, "vu1_vf", 6)))
    {
      start = NUM_CORE_REGS + FIRST_VEC_REG;
      count = 2*NUM_VU_REGS;

      strcpy (full_name, name);
      full_name[8] = 'x';
      full_name[9] = '\0';
      ++len;
    }
  else switch (txvu_cpu) /* prepend context and try again. */
    {
    case TXVU_CPU_VU0:
      if (!STREQN (name, "vu0_", 4))
	{
	  strcpy (full_name, "vu0_");
	  len += 4;
	}
      else
	full_name[0] = 0;

      start = NUM_CORE_REGS;
      count = NUM_VU_REGS;

      strcat (full_name, name);

      /* Check for vector form, eg. vf07.  */
      if (len==8 && STREQN (full_name, "vu0_vf", 6))
	{
	  full_name[8] = 'x';
	  full_name[9] = '\0';
	  ++len;
	}
      break;

    case TXVU_CPU_VU1:
      if (!STREQN (name, "vu1_", 4))
	{
	  strcpy (full_name, "vu1_");
	  len += 4;
	}
      else
	full_name[0] = 0;

      start = NUM_CORE_REGS + NUM_VU_REGS;
      count = NUM_VU_REGS;

      strcat (full_name, name);

      /* Check for vector form, eg. vf07.  */
      if (len==8 && STREQN (full_name, "vu1_vf", 6))
	{
	  full_name[8] = 'x';
	  full_name[9] = '\0';
	  ++len;
	}
      break;

    case TXVU_CPU_VIF0:
      start = NUM_CORE_REGS + 2*NUM_VU_REGS;
      count = NUM_VIF_REGS;
      len += 5;

      strcpy (full_name, "vif0_");
      strcat (full_name, name);
      break;

    case TXVU_CPU_VIF1:
      start = NUM_CORE_REGS + 2*NUM_VU_REGS + NUM_VIF_REGS;
      count = NUM_VIF_REGS;
      len += 5;

      strcpy (full_name, "vif1_");
      strcat (full_name, name);
      break;
      
    default:
      return -1;
    }

  /* Start search at first unit register. */
  for (regnum = start; regnum < (start + count); regnum++)
    if (STREQN (full_name, REGISTER_NAME (regnum), len)
	&& strlen (REGISTER_NAME (regnum)) == len)
      return regnum;

  /* Stack pointers for the VUs are the last potential alias. */
  if (STREQN (name, "sp", 2))
    {
      if (txvu_cpu == TXVU_CPU_VU0)
	return VU0_SP_REGNUM;
      else if (txvu_cpu == TXVU_CPU_VU1)
	return VU1_SP_REGNUM;
    }

  return -1;
}

/* Print an instruction given a memory address.
   There are four things to think about here:
   1. If the context is vu[01] and memaddr is in local memory bounds
      - print as VU instruction.
   2. If the address is out of local bounds but the section is .vutext
      - print as VU instruction.
   3. If the address is the aliased address of VU local memory
      - print as VU instruction.
   4. Otherwise print as MIPS instruction. (Should we check that it is
      in the .text section?)		*/

extern int gdb_print_insn_mips PARAMS ((bfd_vma, disassemble_info *));

static int
txvu_print_insn (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  int mach = 0;
  asymbol *asym;

  if ((memaddr<VU0_MEM0_SIZE && txvu_cpu==TXVU_CPU_VU0)
      || (memaddr<VU1_MEM0_SIZE && txvu_cpu==TXVU_CPU_VU1))
    {
      memaddr &= ~7;	/* Force double-word alignment */

      info->mach = bfd_mach_dvp_vu;
      return print_insn_dvp (memaddr, info);
    }
  
  mach = addr_is_dvp (memaddr);
  if (bfd_mach_dvp_p (mach))
    {
      int cnt;

      info->mach = mach;

      /* Round down the instruction address to the appropriate boundary.  */
      if (mach == bfd_mach_dvp_vif)
	{
	  memaddr &= ~3;

	  /* The disassembler will not try to expand the operands of an MPG
	     unless info->symbols is set. On the other hand, since we're 
	     not trying to set it to a real symbol, its safest to leave
	     info->num_symbols = 0.  */

	  info->symbols = &asym;
	  info->num_symbols = 0;
	}
      else
	memaddr &= ~7;
	
      cnt = print_insn_dvp (memaddr, info);

      info->symbols = NULL;
      return cnt;
    }

#ifdef TARGET_SKY_B
  {
    int insn, val;

    val = info->read_memory_func (memaddr, (bfd_byte *) &insn, 4, info);
    if (val)
      {
	info->memory_error_func (val, memaddr, info);
	return 0;
      }

    info->fprintf_func (info->stream, "%08x", insn);
    return 4;
  }
#endif

  return gdb_print_insn_mips (memaddr, info);
}

struct type *
txvu_register_type (n)
     int n;
{
  if (n < NUM_CORE_REGS) 
    return REGISTER_VIRTUAL_TYPE(n);

  n -= NUM_CORE_REGS;	  /* VU0 types */
  if (n < NUM_VU_INTEGER_REGS)
    return builtin_type_short;

  if (n < (NUM_VU_INTEGER_REGS + 9))	/* PC to R register */
    return builtin_type_uint32;

  if (n < NUM_VU_REGS)
    return builtin_type_float;

  n -= NUM_VU_REGS;	/* VU1 types */
  if (n < NUM_VU_INTEGER_REGS)
    return builtin_type_short;

  if (n < (NUM_VU_INTEGER_REGS + 9))	/* PC and R register */
    return builtin_type_uint32;

  if (n < NUM_VU_REGS)
    return builtin_type_float;

  return builtin_type_uint32;
}

/* ARGSUSED */
static void
printvector_command (exp, from_tty)
     char *exp;
     int from_tty;
{
  char *end;
  int regnum;
  char name[8];

  if (!exp || exp=='\0')
    return;

  do
    {
      if (exp[0] == '$')
	exp++;

      end = exp;
      while (*end != '\0' && *end != ' ' && *end != '\t')
	++end;

      /* permit the vector registers to be specified by number, 
	 eg. "pv 4" is the same as "printvector vf04".  */
      if (*exp >= '0' && *exp <= '9')
	{
	  regnum = atoi (exp);
	  if (regnum >= 0 && regnum < 32)
	    {
	      sprintf (name, "vf%02d", regnum);

	      regnum = target_map_name_to_register (name, 4);
	    }
	  else regnum = -1;
	}
      else
	regnum = target_map_name_to_register (exp, end - exp);

      if (regnum < 0)
	error ("%.*s: invalid vector register", end - exp, exp);

      txvu_print_register (regnum);
      printf_filtered ("\n");

      exp = end;
      while (*exp == ' ' || *exp == '\t')
	++exp;
    } while (*exp != '\0');
}

static char *vector_order_string;

#ifdef WITH_SIM
extern void simulator_command PARAMS ((char *args, int from_tty));
#endif

/* Command to set the printvector order.  vector_order_string will have been 
   set to the user's argument.  */

/*ARGSUSED*/
static void
set_printvector_order_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
  int illegal_spec = 0;
  char sim_cmd[20];

  if (vector_order_string == NULL || *vector_order_string == '\0' ) 
    printvector_order = ORDER_XYZW;
  else 
    {
      if (*vector_order_string == 'W' || *vector_order_string == 'w')
	printvector_order = ORDER_WZYX;
      else if (*vector_order_string == 'X' || *vector_order_string == 'x')
	printvector_order = ORDER_XYZW;
      else
	{
	  illegal_spec = 1;
	  printf_filtered ("Illegal order specification. Run `help set printvector-order' for legal selections.");
	}
    }

  if (vector_order_string != NULL)
    free (vector_order_string);

  /* Canonicalize the string for use by "show".  */
  if (printvector_order == ORDER_XYZW)
    vector_order_string = strsave ("xyzw");
  else
    vector_order_string = strsave ("wzyx");

#ifdef WITH_SIM
  /* Pass the command to the simulator (for printing the pipe).  */
  strcpy (sim_cmd, "pipe order ");
  strcat (sim_cmd, vector_order_string);
  simulator_command (sim_cmd, from_tty);
#endif
}

static void
show_printvector_order_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
}

/* Command to set CPU context.  txvu_cpu_string will have been set to the
   user's argument.  Set txvu_cpu based on txvu_cpu_string, and then
   canonicalize txvu_cpu_string.  */

/*ARGSUSED*/
static void
set_cpu_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
  int i = 0;
  enum txvu_cpu_context new_cpu = txvu_cpu;

  if (txvu_cpu_string == NULL || *txvu_cpu_string == '\0' ) 
    new_cpu = TXVU_CPU_AUTO;
  else 
    {
      for (i = 0; i < TXVU_CPU_NUM; i++ )
	{
	  if (strcasecmp (txvu_cpu_string, txvu_cpu_names[i]) == 0)
	    {
	      new_cpu = i-1; /* Because TXVU_CPU_AUTO == -1 */
	      break;
	    }
	}
    }

#ifdef TARGET_SKY_B
  switch (new_cpu)
    {
    case TXVU_CPU_MASTER:
    case TXVU_CPU_VU0:
    case TXVU_CPU_VIF0:
      i = TXVU_CPU_NUM; /* Sorry - can't use these in Subsystem B */
      break;

    default:
      break;
    }
#endif

  if (i == TXVU_CPU_NUM) 
    {
      /* We have to do this because if the user typed "set cpu foo"
       * txvu_cpu_string currently contains "foo". So we reset it.  */

      if (txvu_cpu_string != NULL) 
	free (txvu_cpu_string);

      txvu_cpu_string = strsave (txvu_cpu_names[txvu_cpu+1]);

      error ("Unknown CPU. Run `help set cpu' for legal selections.");
    }

  context_switch (new_cpu);
}

static void
show_cpu_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
}

static void
txvu_set_prompt (cpu)
     enum txvu_cpu_context cpu;
{
  char *prompt = get_prompt ();
  int promptlen = strlen (prompt);
  int i, closing = -1;
  char *suffix;
  struct cmd_list_element *c;
  char *p, *newprompt = alloca (promptlen + 20);

  /* First see if the old prompt had a context suffix */
  for (i=0; i<TXVU_CPU_NUM; i++)
    {
      suffix = strstr (prompt, txvu_cpu_names[i]);
      if (suffix)
	{
	  /* yup, had a suffix - toss it, but remember what followed */
	  closing = (suffix + strlen (txvu_cpu_names[i])) - prompt;

	  if (suffix != prompt && *(suffix-1) == '-')
	    --suffix; 

	  break;
	}
    }

  /* See if there is some closing part, ie. the ") " of "(gdb) ". */
  if (closing == -1)
    {
      /* There was no context suffix if closing wasn't set above. */
      suffix = 0; 
      for (closing=promptlen; closing>0; closing--)
	{
	  if (isalnum (prompt[closing]))
	    {
	      closing++;
	      break;
	    }
	}
    }

  strcpy (newprompt, "set prompt ");

  /* Build up the new prompt as 3 parts: the original prefix; "-cpu"; and
     the closing part. Skip the '-' if there was no prefix (eg. "> ").  */

  if (suffix)
    strncat (newprompt, prompt, (suffix-prompt));
  else
    strncat (newprompt, prompt, closing);

  if (cpu != TXVU_CPU_AUTO)
    {
      if (closing != 0 && suffix != prompt)
	strcat (newprompt, "-");

      strcat (newprompt, txvu_cpu_names[cpu+1]);
    }

  strcat (newprompt, prompt+closing);

  p = newprompt;
  c = lookup_cmd (&p, cmdlist, "", 0, 1);
  if (!c)
    printf ("set prompt command not found?\n");

  do_setshow_command (p, 0, c);
}

/*ARGSUSED*/
static void
set_context_in_prompt_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
  if (context_in_prompt)
    txvu_set_prompt (txvu_cpu);
  else
    txvu_set_prompt (TXVU_CPU_AUTO);
}

static void
show_context_in_prompt_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
}

/* Target vector for refreshing overlay mapped state */
extern void (*target_overlay_update) PARAMS ((struct obj_section *)); 

void
_initialize_txvu_tdep ()
{
  struct cmd_list_element *c;

  tm_print_insn = txvu_print_insn;

  c = add_set_cmd ("cpu", class_support, var_string_noescape,
		   (char *) &txvu_cpu_string,
		   TXVU_SET_CPU_HELP_STRING,
		   &setlist);
  c->function.sfunc = set_cpu_command;
  c = add_show_from_set (c, &showlist);
  c->function.sfunc = show_cpu_command;

  txvu_cpu = TXVU_CPU_AUTO;
  txvu_cpu_string = strsave ("auto");

  c = add_set_cmd ("context-in-prompt", class_support, var_boolean,
		   (char *) &context_in_prompt,
		   "Set current CPU context as part of the prompt.",
		   &setlist);
  c->function.sfunc = set_context_in_prompt_command;
  c = add_show_from_set (c, &showlist);
  c->function.sfunc = show_context_in_prompt_command;

  add_com ("printvector", class_vars, printvector_command,
	   "Print value of expression EXP as a 4-element vector.");
  add_com_alias ("pv", "printvector", class_vars, 1);

  c = add_set_cmd ("printvector-order", class_support, var_string_noescape,
		   (char *) &vector_order_string,
		   "Set the order in which vectors are printed.\n\
Legal orders are 'wzyx' (the default), or 'xyzw'.",
		   &setlist);
  c->function.sfunc = set_printvector_order_command;
  c = add_show_from_set (c, &showlist);
  c->function.sfunc = show_printvector_order_command;

  vector_order_string = strsave ("wzyx");

  target_overlay_update = txvu_overlay_update;
  overlay_debugging = -1; /* enable auto overlay */
}
