/* Generic sibling call optimization support
   Copyright (C) 1999 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"

#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "insn-config.h"
#include "recog.h"
#include "basic-block.h"

/* Temporary until we have stack slot life analysis to determine if any
   stack slots are live at potential sibling or tail recursion call sites.  */
extern HOST_WIDE_INT frame_offset;
extern int current_function_calls_alloca;

/* We need to know what the current function's return value looks like so
   we can recognize sets and uses of its value.  */
extern rtx current_function_return_rtx;

/* ???  We really should find a good way to include function.h here.  */
extern int current_function_varargs;
extern int current_function_stdarg;

static rtx skip_copy_from_return_value	PROTO ((rtx));
static rtx skip_copy_to_return_value	PROTO ((rtx));
static rtx skip_use_of_return_value	PROTO ((rtx));
static rtx skip_stack_adjustment	PROTO ((rtx));
static rtx skip_jump_insn		PROTO ((rtx));
static int uses_addressof		PROTO ((rtx));
static int sequence_uses_addressof	PROTO ((rtx));
static int purge_reg_equiv_notes	PROTO ((void));

/* If the first real insn after ORIG_INSN copies from this function's
   return value, then return the insn which performs the copy.  Otherwise
   return ORIG_INSN.  */

static rtx
skip_copy_from_return_value (orig_insn)
     rtx orig_insn;
{
  rtx insn, set = NULL_RTX;

  insn = next_nonnote_insn (orig_insn);

  if (insn)
    set = single_set (insn);

  /* The source must be the same as the current function's return value to
     ensure that any return value is put in the same place by the current
     function and the function we're calling.   The destination register
     must be a pseudo.  */
  if (insn
      && set
      && SET_SRC (set) == current_function_return_rtx
      && REG_P (SET_DEST (set))
      && REGNO (SET_DEST (set)) >= FIRST_PSEUDO_REGISTER)
    return insn;

  /* It did not look like a copy of the return value, so return the
     same insn we were passed.  */
  return orig_insn;
  
}

/* If the first real insn after ORIG_INSN copies to this function's
   return value, then return the insn which performs the copy.  Otherwise
   return ORIG_INSN.  */

static rtx
skip_copy_to_return_value (orig_insn)
     rtx orig_insn;
{
  rtx insn, set = NULL_RTX;

  insn = next_nonnote_insn (orig_insn);

  if (insn)
    set = single_set (insn);

  /* The source must be the same as the current function's return value to
     ensure that any return value is put in the same place by the current
     function and the function we're calling.   The destination register
     must be a pseudo.  */
  if (insn
      && set
      && SET_DEST (set) == current_function_return_rtx
      && REG_P (SET_SRC (set))
      && REGNO (SET_SRC (set)) >= FIRST_PSEUDO_REGISTER)
    return insn;

  /* It did not look like a copy of the return value, so return the
     same insn we were passed.  */
  return orig_insn;
  
}

/* If the first real insn after ORIG_INSN is a USE of this function's return
   value, return the USE insn.  Otherwise return ORIG_INSN.  */

static rtx
skip_use_of_return_value (orig_insn)
     rtx orig_insn;
{
  rtx insn;

  insn = next_nonnote_insn (orig_insn);

  if (insn
      && GET_CODE (insn) == INSN
      && GET_CODE (PATTERN (insn)) == USE
      && (XEXP (PATTERN (insn), 0) == current_function_return_rtx
	  || XEXP (PATTERN (insn), 0) == const0_rtx))
    return insn;

  return orig_insn;
}

/* If the first real insn after ORIG_INSN adjusts the stack pointer
   by a constant, return the insn with the stack pointer adjustment.
   Otherwise return ORIG_INSN.  */

static rtx
skip_stack_adjustment (orig_insn)
     rtx orig_insn;
{
  rtx insn, set = NULL_RTX;

  insn = next_nonnote_insn (orig_insn);

  if (insn)
    set = single_set (insn);

  /* The source must be the same as the current function's return value to
     ensure that any return value is put in the same place by the current
     function and the function we're calling.   The destination register
     must be a pseudo.  */
  if (insn
      && set
      && GET_CODE (SET_SRC (set)) == PLUS
      && XEXP (SET_SRC (set), 0) == stack_pointer_rtx
      && GET_CODE (XEXP (SET_SRC (set), 1)) == CONST_INT
      && SET_DEST (set) == stack_pointer_rtx)
    return insn;

  /* It did not look like a copy of the return value, so return the
     same insn we were passed.  */
  return orig_insn;
  
}

/* Search SEQ for any block notes and emit-emit them before WHERE.

   This is necessary to prevent the tree structures from having
   block scopes without corresponding notes in the rtl chain.

   ?!? One day we'll have a better way to do this.  Like pointers
   from the RTL to the appropriate BLOCK note in the tree.  */

void
reemit_block_notes_from_seq (where, seq)
     rtx where, seq;
{
  for ( ; seq; seq = NEXT_INSN (seq))
    {
      if (GET_CODE (seq) == NOTE
	  && (NOTE_LINE_NUMBER (seq) == NOTE_INSN_BLOCK_BEG
	      || NOTE_LINE_NUMBER (seq) == NOTE_INSN_BLOCK_END))
	emit_note_before (NOTE_LINE_NUMBER (seq), where);
    }
}

/* If the first real insn after ORIG_INSN is a jump, return the JUMP_INSN.
   Otherwise return ORIG_INSN.  */

static rtx
skip_jump_insn (orig_insn)
     rtx orig_insn;
{
  rtx insn;

  insn = next_nonnote_insn (orig_insn);

  if (insn
      && GET_CODE (insn) == JUMP_INSN
      && simplejump_p (insn))
    return insn;

  return orig_insn;
}

/* Scan the rtx X for an ADDRESSOF expressions.  Return nonzero if an ADDRESSOF
   expresion is found, else return zero.  */

static int
uses_addressof (x)
     rtx x;
{
  RTX_CODE code;
  int i, j;
  char *fmt;

  if (x == NULL_RTX)
    return 0;

  code = GET_CODE (x);

  if (code == ADDRESSOF)
    return 1;

  /* Scan all subexpressions. */
  fmt = GET_RTX_FORMAT (code);
  for (i = 0; i < GET_RTX_LENGTH (code); i++, fmt++)
    {
      if (*fmt == 'e')
	{
	  if (uses_addressof (XEXP (x, i)))
	    return 1;
	}
      else if (*fmt == 'E')
	{
	  for (j = 0; j < XVECLEN (x, i); j++)
	    if (uses_addressof (XVECEXP (x, i, j)))
	      return 1;
	}
    }
  return 0;
}

/* Scan the sequence of insns in SEQ to see if any have an ADDRESSOF
   rtl expression.  If an ADDRESSOF expression is found, return nonzero,
   else return zero.

   This function handles CALL_PLACEHOLDERs which contain multiple sequences
   of insns.  */

static int
sequence_uses_addressof (seq)
     rtx seq;
{
  rtx insn;

  for (insn = seq; insn; insn = NEXT_INSN (insn))
    if (GET_RTX_CLASS (GET_CODE (insn)) == 'i')
      {
	/* If this is a CALL_PLACEHOLDER, then recursively call ourselves
	   with each nonempty sequence attached to the CALL_PLACEHOLDER.  */
	if (GET_CODE (insn) == CALL_INSN
	    && GET_CODE (PATTERN (insn)) == CALL_PLACEHOLDER)
	  {
	    if (XEXP (PATTERN (insn), 0) != NULL_RTX
		&& sequence_uses_addressof (XEXP (PATTERN (insn), 0)))
	      return 1;
	    if (XEXP (PATTERN (insn), 1) != NULL_RTX
		&& sequence_uses_addressof (XEXP (PATTERN (insn), 1)))
	      return 1;
	    if (XEXP (PATTERN (insn), 2) != NULL_RTX
		&& sequence_uses_addressof (XEXP (PATTERN (insn), 2)))
	      return 1;
	  }
	else if (uses_addressof (PATTERN (insn))
		 || (REG_NOTES (insn) && uses_addressof (REG_NOTES (insn))))
	  return 1;
      }
  return 0;
}

/* Remove all REG_EQUIV notes found in the insn chain.  */
static int
purge_reg_equiv_notes ()
{
  rtx insn;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      while (1)
	{
	  rtx note = find_reg_note (insn, REG_EQUIV, 0);
	  if (note)
	    {
	      /* Remove the note and keep looking at the notes for
		 this insn.  */
	      remove_note (insn, note);
	      continue;
	    }
	  break;
	}
    }
}

/* Given a (possibly empty) set of potential sibling or tail recursion call
   sites, determine if optimization is possible.

   Potential sibling or tail recursion calls are marked with CALL_PLACEHOLDER
   insns.  The CALL_PLACEHOLDER insn holds chains of insns to implement a
   normal call, sibling call or tail recursive call.

   Replace the CALL_PLACEHOLDER with an appropriate insn chain.  */

void
optimize_sibling_and_tail_recursive_calls ()
{
  rtx insn, insns;
  int_list_ptr *s_preds, *s_succs, p;
  int *num_preds, *num_succs, bb;
  int alternate_exit = EXIT_BLOCK, potential_sibcalls;
  int x = 0;
  int current_function_uses_addressof;
  int successful_sibling_call = 0;

  insns = get_insns ();

  /* We do not perform these calls when flag_exceptions is true, so this
     is probably a NOP at the current time.  However, we may want to support
     sibling and tail recursion optimizations in the future, so let's plan
     ahead and find all the EH labels.  */
  find_exception_handler_labels ();

  /* Run a jump optimization pass to clean up the CFG.  We primarily want
     this to thread jumps so that it is obvious which blocks jump to the
     epilouge.  */
  jump_optimize_minimal (insns);

  /* We need cfg information to determine which blocks are succeeded
     only by the epilogue.  */
  find_basic_blocks (insns, max_reg_num (), 0);

  /* If there are no basic blocks, then there is nothing to do.  */
  if (n_basic_blocks == 0)
    return;

  /* Allocate memory to hold the pred/succ lists and counts.  */
  s_preds = (int_list_ptr *) alloca (n_basic_blocks * sizeof (int_list_ptr));
  s_succs = (int_list_ptr *) alloca (n_basic_blocks * sizeof (int_list_ptr));
  num_preds = (int *) alloca (n_basic_blocks * sizeof (int));
  num_succs = (int *) alloca (n_basic_blocks * sizeof (int));

  /* Build the cfg.  */
  compute_preds_succs (s_preds, s_succs, num_preds, num_succs, 0);

  /* Find the exit block.

     It is possible that we have blocks which can reach the exit block
     directly.  However, most of the time a block will jump (or fall into)
     N_BASIC_BLOCKS - 1, which in turn falls into the exit block.  */
  if (num_succs[n_basic_blocks - 1] == 1
      && INT_LIST_VAL (s_succs[n_basic_blocks - 1]) == EXIT_BLOCK)
    {
      rtx insn;

      /* Walk forwards through the last normal block and see if it
	 does nothing except fall into the exit block.  */
      for (insn = BLOCK_HEAD (n_basic_blocks - 1);
	   insn;
	   insn = NEXT_INSN (insn))
	{
	  /* This should only happen once, at the start of this block.  */
	  if (GET_CODE (insn) == CODE_LABEL)
	    continue;

	  if (GET_CODE (insn) == NOTE)
	    continue;

	  if (GET_CODE (insn) == INSN
	      && GET_CODE (PATTERN (insn)) == USE)
	    continue;

	  break;
	}

      /* If INSN is nonzero, then the search stopped before we hit the end
	 of the function, presumably on an insn which we must execute.  So
	 this block is not an valid exit block for sibling and tail recursive
	 call optimizations.  */
      if (insn)
	alternate_exit = EXIT_BLOCK;
      else 
	alternate_exit = n_basic_blocks - 1;
    }

  /* If the function uses ADDRESSOF, we can't (easily) determine
     at this point if the value will end up on the stack.  */
  current_function_uses_addressof = sequence_uses_addressof (insns);

  /* Walk the insn chain and find any CALL_PLACEHOLDER insns.  We need to
     select one of the insn sequences attached to each CALL_PLACEHOLDER.

     The different sequences represent different ways to implement the call,
     ie, tail recursion, sibling call or normal call.  */
  for (insn = insns; insn; insn = NEXT_INSN (insn))
    {
      if (GET_CODE (insn) == CALL_INSN
	  && GET_CODE (PATTERN (insn)) == CALL_PLACEHOLDER)
	{
	  int sibcall = (XEXP (PATTERN (insn), 1) != NULL_RTX);
	  int tailrecursion = (XEXP (PATTERN (insn), 2) != NULL_RTX);
	  int succ_block, call_block;
	  rtx prev, set, temp;
	  int_list_ptr ps;

	  /* We must be careful with stack slots which are live at
	     potential optimization sites.

	     ?!? This test is overly conservative and will be replaced.  */
	  if (frame_offset)
	    sibcall = 0, tailrecursion = 0;

	  /* alloca (until we have stack slot life analysis) inhibits
	     sibling call optimizations, but not tail recursion.

	     Similarly if we have ADDRESSOF expressions.

	     Similarly if we use varargs or stdarg since they implicitly
	     may take the address of an argument.  */
 	  if (current_function_calls_alloca || current_function_uses_addressof
	      || current_function_varargs || current_function_stdarg)
	    sibcall = 0;

	  call_block = BLOCK_NUM (insn);
	  /* If the block has more than one successor, then we can not
	     perform sibcall or tail recursion optimizations.  */
	  if (num_succs[call_block] != 1)
	    sibcall = 0, tailrecursion = 0;

	  /* If the single successor is not the exit block, then we can not
	     perform sibcall or tail recursion optimizations.  */
	  ps = s_succs[call_block];
	  if (ps)
	    succ_block = INT_LIST_VAL (ps);
	  else
	    succ_block = EXIT_BLOCK;

	  if (succ_block != EXIT_BLOCK
	      && succ_block != alternate_exit)
	    sibcall = 0, tailrecursion = 0;

	  /* If the call was the end of the block, then we're OK.  */
	  if (insn == BLOCK_END (call_block))
	    goto success;

#if 0
	  /* Skip over copying the return value from a hard reg into a
	     pseudo. 

	     ??? Need to bullet proof this more.  This is only valid if we
	     return with no value (pseudo was dead), or we immediately copy
	     the pseudo back to the return register, then return.  */
	  temp = skip_copy_from_return_value (insn);
	  if (temp == BLOCK_END (call_block))
	    goto success;

	  /* ??? Need to bullet proof this more.  This is only valid if the
	     source register is the return value from this call.  */
	  temp = skip_copy_to_return_value (insn);
	  if (temp == BLOCK_END (call_block))
	    goto success;
#endif
	    
	  /* Skip over a USE of the return value (as a hard reg).  */
	  temp = skip_use_of_return_value (insn);
	  if (temp == BLOCK_END (call_block))
	    goto success;

	  /* Skip any stack adjustment.  */
	  temp = skip_stack_adjustment (temp);
	  if (temp == BLOCK_END (call_block))
	    goto success;

	  /* Skip over the JUMP_INSN at the end of the block.  */
	  temp = skip_jump_insn (temp);
	  if (GET_CODE (temp) == NOTE)
	    temp = next_nonnote_insn (temp);
	  if (temp == BLOCK_END (call_block))
	    goto success;

	  /* There are operations at the end of the block which we must
	     execute after returning from the function call.  So this call
	     can not be optimized.  */
	  sibcall = 0, tailrecursion = 0;

success:
	
	  /* Select a set of insns to implement the call and emit them.
	     Tail recursion is the most efficient, so select it over
	     a tail/sibling call.  */

	  if (tailrecursion)
	    {
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 1));
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 0));
	      emit_insns_before (XEXP (PATTERN (insn), 2), insn);
	    }
	  else if (sibcall)
	    {
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 2));
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 0));
	      emit_insns_before (XEXP (PATTERN (insn), 1), insn);
	      successful_sibling_call = 1;
	    }
	  else
	    {
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 2));
	      reemit_block_notes_from_seq (insn, XEXP (PATTERN (insn), 1));
	      emit_insns_before (XEXP (PATTERN (insn), 0), insn);
	    }

	  /* Turn off LABEL_PRESERVE_P for the tail recursion label if it
	     exists.  We only had to set it long enough to keep the jump
	     pass above from deleting it as unused.  */
	  if (XEXP (PATTERN (insn), 3))
	    LABEL_PRESERVE_P (XEXP (PATTERN (insn), 3)) = 0;

	  /* "Delete" the placeholder insn.

	      Since we do not create nested CALL_PLACEHOLDERs, the scan
	      continues with the insn that was after the CALL_PLACEHOLDER,
	      not the insns we just created.  */
	  PUT_CODE (insn, NOTE);
	  NOTE_SOURCE_FILE (insn) = 0;
	  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
	}
    }

  /* A sibling call sequence invalidates any REG_EQUIV notes made for
     this function's incoming arguments. 

     At the start of RTL generation we know the only REG_EQUIV notes
     in the rtl chain are those for incoming arguments, so we can safely
     flush any REG_EQUIV note. 

     This is (slight) overkill.  We could keep track of the highest argument
     we clobber and be more selective in removing notes, but it does not
     seem to be worth the effort.  */
  if (successful_sibling_call)
    purge_reg_equiv_notes ();

  /* This information will be invalid after inline expansion -- get rid
     of it now so we don't read state data across obstack changes.  */
  free_basic_block_vars (0);
}
