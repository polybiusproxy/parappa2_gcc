/* Generic partial redundancy elimination with lazy code motion
   support.
   Copyright (C) 1998 Free Software Foundation, Inc.

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

/* These routines are meant to be used by various optimization
   passes which can be modeled as lazy code motion problems. 
   Including, but not limited to:

	* Traditional partial redundancy elimination.

	* Placement of caller/caller register save/restores.

	* Load/store motion.

	* Copy motion.

	* Conversion of flat register files to a stacked register
	model.

	* Dead load/store elimination.

  These routines accept as input:

	* Basic block information (number of blocks, lists of
	predecessors and successors).  Note the granularity
	does not need to be basic block, they could be statements
	or functions.

	* Bitmaps of local properties (computed, transparent and
	anticipatable expressions).

  The output of these routines is bitmap of redundant computations
  and a bitmap of optimal placement points.  */


#include "config.h"
#include "system.h"

#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "insn-config.h"
#include "recog.h"
#include "basic-block.h"

static void compute_antinout 	PROTO ((int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *, sbitmap *));
static void compute_earlyinout	PROTO ((int, int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *, sbitmap *));
static void compute_delayinout  PROTO ((int, int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *,
					sbitmap *, sbitmap *));
static void compute_latein	PROTO ((int, int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *));
static void compute_isoinout	PROTO ((int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *, sbitmap *));
static void compute_optimal	PROTO ((int, sbitmap *,
					sbitmap *, sbitmap *));
static void compute_redundant	PROTO ((int, int, sbitmap *,
					sbitmap *, sbitmap *, sbitmap *));

/* Similarly, but for the reversed flowgraph.  */
static void compute_avinout 	PROTO ((int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *, sbitmap *));
static void compute_fartherinout	PROTO ((int, int, int_list_ptr *,
						sbitmap *, sbitmap *,
						sbitmap *, sbitmap *));
static void compute_earlierinout  PROTO ((int, int, int_list_ptr *, sbitmap *,
					  sbitmap *, sbitmap *,
					  sbitmap *, sbitmap *));
static void compute_firstout	PROTO ((int, int, int_list_ptr *, sbitmap *,
					sbitmap *, sbitmap *));
static void compute_rev_isoinout PROTO ((int, int_list_ptr *, sbitmap *,
					 sbitmap *, sbitmap *, sbitmap *));

/* Edge based lcm routines.  */
static void compute_antinout_edge  PROTO ((sbitmap *, sbitmap *,
					   sbitmap *, sbitmap *));
static void compute_earliest  PROTO((struct edge_list *, int, sbitmap *,
				     sbitmap *, sbitmap *, sbitmap *,
				     sbitmap *));
static void compute_laterin  PROTO((struct edge_list *, int, sbitmap *,
				     sbitmap *, sbitmap *, sbitmap *));
static void compute_insert_delete  PROTO ((struct edge_list *edge_list,
					   sbitmap *, sbitmap *, sbitmap *,
					   sbitmap *, sbitmap *));

/* Given local properties TRANSP, ANTLOC, return the redundant and optimal
   computation points for expressions.

   To reduce overall memory consumption, we allocate memory immediately
   before its needed and deallocate it as soon as possible.  */
void
pre_lcm (n_blocks, n_exprs, s_preds, s_succs, transp,
	 antloc, redundant, optimal)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_preds;
     int_list_ptr *s_succs;
     sbitmap *transp;
     sbitmap *antloc;
     sbitmap *redundant;
     sbitmap *optimal;
{
  sbitmap *antin, *antout, *earlyin, *earlyout, *delayin, *delayout;
  sbitmap *latein, *isoin, *isoout;

  /* Compute global anticipatability.  ANTOUT is not needed except to
     compute ANTIN, so free its memory as soon as we return from
     compute_antinout.  */
  antin = sbitmap_vector_alloc (n_blocks, n_exprs);
  antout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_antinout (n_blocks, s_succs, antloc,
		    transp, antin, antout);
  free (antout);
  antout = NULL;

  /* Compute earliestness.  EARLYOUT is not needed except to compute
     EARLYIN, so free its memory as soon as we return from
     compute_earlyinout.  */
  earlyin = sbitmap_vector_alloc (n_blocks, n_exprs);
  earlyout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_earlyinout (n_blocks, n_exprs, s_preds, transp, antin,
		      earlyin, earlyout);
  free (earlyout);
  earlyout = NULL;

  /* Compute delayedness.  DELAYOUT is not needed except to compute
     DELAYIN, so free its memory as soon as we return from
     compute_delayinout.  We also no longer need ANTIN and EARLYIN.  */
  delayin = sbitmap_vector_alloc (n_blocks, n_exprs);
  delayout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_delayinout (n_blocks, n_exprs, s_preds, antloc,
		      antin, earlyin, delayin, delayout);
  free (delayout);
  delayout = NULL;
  free (antin);
  antin = NULL;
  free (earlyin);
  earlyin = NULL;

  /* Compute latestness.  We no longer need DELAYIN after we compute
     LATEIN.  */
  latein = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_latein (n_blocks, n_exprs, s_succs, antloc, delayin, latein);
  free (delayin);
  delayin = NULL;

  /* Compute isolatedness.  ISOIN is not needed except to compute
     ISOOUT, so free its memory as soon as we return from
     compute_isoinout.  */
  isoin = sbitmap_vector_alloc (n_blocks, n_exprs);
  isoout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_isoinout (n_blocks, s_succs, antloc, latein, isoin, isoout);
  free (isoin);
  isoin = NULL;

  /* Now compute optimal placement points and the redundant expressions.  */
  compute_optimal (n_blocks, latein, isoout, optimal);
  compute_redundant (n_blocks, n_exprs, antloc, latein, isoout, redundant);
  free (latein);
  latein = NULL;
  free (isoout);
  isoout = NULL;
}

/* Given local properties TRANSP, AVLOC, return the redundant and optimal
   computation points for expressions on the reverse flowgraph.

   To reduce overall memory consumption, we allocate memory immediately
   before its needed and deallocate it as soon as possible.  */

void
pre_rev_lcm (n_blocks, n_exprs, s_preds, s_succs, transp,
	     avloc, redundant, optimal)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_preds;
     int_list_ptr *s_succs;
     sbitmap *transp;
     sbitmap *avloc;
     sbitmap *redundant;
     sbitmap *optimal;
{
  sbitmap *avin, *avout, *fartherin, *fartherout, *earlierin, *earlierout;
  sbitmap *firstout, *rev_isoin, *rev_isoout;

  /* Compute global availability.  AVIN is not needed except to
     compute AVOUT, so free its memory as soon as we return from
     compute_avinout.  */
  avin = sbitmap_vector_alloc (n_blocks, n_exprs);
  avout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_avinout (n_blocks, s_preds, avloc, transp, avin, avout);
  free (avin);
  avin = NULL;

  /* Compute fartherness.  FARTHERIN is not needed except to compute
     FARTHEROUT, so free its memory as soon as we return from
     compute_earlyinout.  */
  fartherin = sbitmap_vector_alloc (n_blocks, n_exprs);
  fartherout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_fartherinout (n_blocks, n_exprs, s_succs, transp,
			avout, fartherin, fartherout);
  free (fartherin);
  fartherin = NULL;

  /* Compute earlierness.  EARLIERIN is not needed except to compute
     EARLIEROUT, so free its memory as soon as we return from
     compute_delayinout.  We also no longer need AVOUT and FARTHEROUT.  */
  earlierin = sbitmap_vector_alloc (n_blocks, n_exprs);
  earlierout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_earlierinout (n_blocks, n_exprs, s_succs, avloc,
		        avout, fartherout, earlierin, earlierout);
  free (earlierin);
  earlierin = NULL;
  free (avout);
  avout = NULL;
  free (fartherout);
  fartherout = NULL;

  /* Compute firstness.  We no longer need EARLIEROUT after we compute
     FIRSTOUT.  */
  firstout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_firstout (n_blocks, n_exprs, s_preds, avloc, earlierout, firstout);
  free (earlierout);
  earlierout = NULL;

  /* Compute rev_isolatedness.  ISOIN is not needed except to compute
     ISOOUT, so free its memory as soon as we return from
     compute_isoinout.  */
  rev_isoin = sbitmap_vector_alloc (n_blocks, n_exprs);
  rev_isoout = sbitmap_vector_alloc (n_blocks, n_exprs);
  compute_rev_isoinout (n_blocks, s_preds, avloc, firstout,
			rev_isoin, rev_isoout);
  free (rev_isoout);
  rev_isoout = NULL;

  /* Now compute optimal placement points and the redundant expressions.  */
  compute_optimal (n_blocks, firstout, rev_isoin, optimal);
  compute_redundant (n_blocks, n_exprs, avloc, firstout, rev_isoin, redundant);
  free (firstout);
  firstout = NULL;
  free (rev_isoin);
  rev_isoin = NULL;
}

/* Compute expression anticipatability at entrance and exit of each block.  */

static void
compute_antinout (n_blocks, s_succs, antloc, transp, antin, antout)
     int n_blocks;
     int_list_ptr *s_succs;
     sbitmap *antloc;
     sbitmap *transp;
     sbitmap *antin;
     sbitmap *antout;
{
  int bb, changed, passes;
  sbitmap old_changed, new_changed;

  sbitmap_zero (antout[n_blocks - 1]);
  sbitmap_vector_ones (antin, n_blocks);

  old_changed = sbitmap_alloc (n_blocks);
  new_changed = sbitmap_alloc (n_blocks);
  sbitmap_ones (old_changed);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      sbitmap_zero (new_changed);
      /* We scan the blocks in the reverse order to speed up
	 the convergence.  */
      for (bb = n_blocks - 1; bb >= 0; bb--)
	{
	  int_list_ptr ps;

	  /* If none of the successors of this block have changed,
	     then this block is not going to change.  */
	  for (ps = s_succs[bb] ; ps; ps = ps->next)
	    {
	      if (INT_LIST_VAL (ps) == EXIT_BLOCK
		  || INT_LIST_VAL (ps) == ENTRY_BLOCK)
		break;

	      if (TEST_BIT (old_changed, INT_LIST_VAL (ps))
		  || TEST_BIT (new_changed, INT_LIST_VAL (ps)))
		break;
	    }

	  if (!ps)
	    continue;

	  if (bb != n_blocks - 1)
	    sbitmap_intersect_of_successors (antout[bb], antin,
					     bb, s_succs);
 	  if (sbitmap_a_or_b_and_c (antin[bb], antloc[bb],
				    transp[bb], antout[bb]))
	    {
	      changed = 1;
	      SET_BIT (new_changed, bb);
	    }
	}
      sbitmap_copy (old_changed, new_changed);
      passes++;
    }
  free (old_changed);
  free (new_changed);
}

/* Compute expression earliestness at entrance and exit of each block.

   From Advanced Compiler Design and Implementation pp411.

   An expression is earliest at the entrance to basic block BB if no
   block from entry to block BB both evaluates the expression and
   produces the same value as evaluating it at the entry to block BB
   does.  Similarly for earlistness at basic block BB exit.  */

static void
compute_earlyinout (n_blocks, n_exprs, s_preds, transp, antin,
		    earlyin, earlyout)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_preds;
     sbitmap *transp;
     sbitmap *antin;
     sbitmap *earlyin;
     sbitmap *earlyout;
{
  int bb, changed, passes;
  sbitmap temp_bitmap;
  sbitmap old_changed, new_changed;

  temp_bitmap = sbitmap_alloc (n_exprs);

  sbitmap_vector_zero (earlyout, n_blocks);
  sbitmap_ones (earlyin[0]);

  old_changed = sbitmap_alloc (n_blocks);
  new_changed = sbitmap_alloc (n_blocks);
  sbitmap_ones (old_changed);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      sbitmap_zero (new_changed);
      for (bb = 0; bb < n_blocks; bb++)
	{
	  int_list_ptr ps;

	  /* If none of the predecessors of this block have changed,
	     then this block is not going to change.  */
	  for (ps = s_preds[bb] ; ps; ps = ps->next)
	    {
	      if (INT_LIST_VAL (ps) == EXIT_BLOCK
		  || INT_LIST_VAL (ps) == ENTRY_BLOCK)
		break;

	      if (TEST_BIT (old_changed, INT_LIST_VAL (ps))
		  || TEST_BIT (new_changed, INT_LIST_VAL (ps)))
		break;
	    }

	  if (!ps)
	    continue;

	  if (bb != 0)
	    sbitmap_union_of_predecessors (earlyin[bb], earlyout,
					   bb, s_preds);
	  sbitmap_not (temp_bitmap, transp[bb]);
	  if (sbitmap_union_of_diff (earlyout[bb], temp_bitmap,
				     earlyin[bb], antin[bb]))
	    {
	      changed = 1;
	      SET_BIT (new_changed, bb);
	    }
	}
      sbitmap_copy (old_changed, new_changed);
      passes++;
    }
  free (old_changed);
  free (new_changed);
  free (temp_bitmap);
}

/* Compute expression delayedness at entrance and exit of each block.

   From Advanced Compiler Design and Implementation pp411.

   An expression is delayed at the entrance to BB if it is anticipatable
   and earliest at that point and if all subsequent computations of
   the expression are in block BB.   */

static void
compute_delayinout (n_blocks, n_exprs, s_preds, antloc,
		    antin, earlyin, delayin, delayout)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_preds;
     sbitmap *antloc;
     sbitmap *antin;
     sbitmap *earlyin;
     sbitmap *delayin;
     sbitmap *delayout;
{
  int bb, changed, passes;
  sbitmap *anti_and_early;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  /* This is constant throughout the flow equations below, so compute
     it once to save time.  */
  anti_and_early = sbitmap_vector_alloc (n_blocks, n_exprs);
  for (bb = 0; bb < n_blocks; bb++)
    sbitmap_a_and_b (anti_and_early[bb], antin[bb], earlyin[bb]);
  
  sbitmap_vector_zero (delayout, n_blocks);
  sbitmap_copy (delayin[0], anti_and_early[0]);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = 0; bb < n_blocks; bb++)
	{
	  if (bb != 0)
	    {
	      sbitmap_intersect_of_predecessors (temp_bitmap, delayout,
						 bb, s_preds);
	      changed |= sbitmap_a_or_b (delayin[bb],
					 anti_and_early[bb],
					 temp_bitmap);
	    }
	  sbitmap_not (temp_bitmap, antloc[bb]);
	  changed |= sbitmap_a_and_b (delayout[bb],
				      temp_bitmap,
				      delayin[bb]);
	}
      passes++;
    }

  /* We're done with this, so go ahead and free it's memory now instead
     of waiting until the end of pre.  */
  free (anti_and_early);
  free (temp_bitmap);
}

/* Compute latestness.

   From Advanced Compiler Design and Implementation pp412.

   An expression is latest at the entrance to block BB if that is an optimal
   point for computing the expression and if on every path from block BB's
   entrance to the exit block, any optimal computation point for the 
   expression occurs after one of the points at which the expression was
   computed in the original flowgraph.  */

static void
compute_latein (n_blocks, n_exprs, s_succs, antloc, delayin, latein)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_succs;
     sbitmap *antloc;
     sbitmap *delayin;
     sbitmap *latein;
{
  int bb;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  for (bb = 0; bb < n_blocks; bb++)
    {
      /* The last block is succeeded only by the exit block; therefore,
	 temp_bitmap will not be set by the following call!  */
      if (bb == n_blocks - 1)
	{
          sbitmap_intersect_of_successors (temp_bitmap, delayin,
				           bb, s_succs);
	  sbitmap_not (temp_bitmap, temp_bitmap);
	}
      else
	sbitmap_ones (temp_bitmap);
      sbitmap_a_and_b_or_c (latein[bb], delayin[bb],
			    antloc[bb], temp_bitmap);
    }
  free (temp_bitmap);
}

/* Compute isolated.

   From Advanced Compiler Design and Implementation pp413.

   A computationally optimal placement for the evaluation of an expression
   is defined to be isolated if and only if on every path from a successor
   of the block in which it is computed to the exit block, every original
   computation of the expression is preceded by the optimal placement point.  */

static void
compute_isoinout (n_blocks, s_succs, antloc, latein, isoin, isoout)
     int n_blocks;
     int_list_ptr *s_succs;
     sbitmap *antloc;
     sbitmap *latein;
     sbitmap *isoin;
     sbitmap *isoout;
{
  int bb, changed, passes;

  sbitmap_vector_zero (isoin, n_blocks);
  sbitmap_zero (isoout[n_blocks - 1]);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = n_blocks - 1; bb >= 0; bb--)
	{
	  if (bb != n_blocks - 1)
	    sbitmap_intersect_of_successors (isoout[bb], isoin,
					     bb, s_succs);
	  changed |= sbitmap_union_of_diff (isoin[bb], latein[bb],
					    isoout[bb], antloc[bb]);
	}
      passes++;
    }
}

/* Compute the set of expressions which have optimal computational points
   in each basic block.  This is the set of expressions that are latest, but
   that are not isolated in the block.  */

static void
compute_optimal (n_blocks, latein, isoout, optimal)
     int n_blocks;
     sbitmap *latein;
     sbitmap *isoout;
     sbitmap *optimal;
{
  int bb;

  for (bb = 0; bb < n_blocks; bb++)
    sbitmap_difference (optimal[bb], latein[bb], isoout[bb]);
}

/* Compute the set of expressions that are redundant in a block.  They are
   the expressions that are used in the block and that are neither isolated
   or latest.  */

static void
compute_redundant (n_blocks, n_exprs, antloc, latein, isoout, redundant)
     int n_blocks;
     int n_exprs;
     sbitmap *antloc;
     sbitmap *latein;
     sbitmap *isoout;
     sbitmap *redundant;
{
  int bb;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  for (bb = 0; bb < n_blocks; bb++)
    {
      sbitmap_a_or_b (temp_bitmap, latein[bb], isoout[bb]);
      sbitmap_difference (redundant[bb], antloc[bb], temp_bitmap);
    }
  free (temp_bitmap);
}

/* Compute expression availability at entrance and exit of each block.  */

static void
compute_avinout (n_blocks, s_preds, avloc, transp, avin, avout)
     int n_blocks;
     int_list_ptr *s_preds;
     sbitmap *avloc;
     sbitmap *transp;
     sbitmap *avin;
     sbitmap *avout;
{
  int bb, changed, passes;

  sbitmap_zero (avin[0]);
  sbitmap_vector_ones (avout, n_blocks);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = 0; bb < n_blocks; bb++)
	{
	  if (bb != 0)
	    sbitmap_intersect_of_predecessors (avin[bb], avout,
					       bb, s_preds);
	  changed |= sbitmap_a_or_b_and_c (avout[bb], avloc[bb],
					   transp[bb], avin[bb]);
	}
      passes++;
    }
}

/* Compute expression latestness.

   This is effectively the same as earliestness computed on the reverse
   flow graph.  */

static void
compute_fartherinout (n_blocks, n_exprs, s_succs,
		      transp, avout, fartherin, fartherout)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_succs;
     sbitmap *transp;
     sbitmap *avout;
     sbitmap *fartherin;
     sbitmap *fartherout;
{
  int bb, changed, passes;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  sbitmap_vector_zero (fartherin, n_blocks);
  sbitmap_ones (fartherout[n_blocks - 1]);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = n_blocks - 1; bb >= 0; bb--)
	{
	  if (bb != n_blocks - 1)
	    sbitmap_union_of_successors (fartherout[bb], fartherin,
					 bb, s_succs);
	  sbitmap_not (temp_bitmap, transp[bb]);
	  changed |= sbitmap_union_of_diff (fartherin[bb], temp_bitmap,
					    fartherout[bb], avout[bb]);
	}
      passes++;
    }

  free (temp_bitmap);
}

/* Compute expression earlierness at entrance and exit of each block.

   This is effectively the same as delayedness computed on the reverse
   flow graph.  */

static void
compute_earlierinout (n_blocks, n_exprs, s_succs, avloc,
		      avout, fartherout, earlierin, earlierout)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_succs;
     sbitmap *avloc;
     sbitmap *avout;
     sbitmap *fartherout;
     sbitmap *earlierin;
     sbitmap *earlierout;
{
  int bb, changed, passes;
  sbitmap *av_and_farther;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  /* This is constant throughout the flow equations below, so compute
     it once to save time.  */
  av_and_farther = sbitmap_vector_alloc (n_blocks, n_exprs);
  for (bb = 0; bb < n_blocks; bb++)
    sbitmap_a_and_b (av_and_farther[bb], avout[bb], fartherout[bb]);
  
  sbitmap_vector_zero (earlierin, n_blocks);
  sbitmap_copy (earlierout[n_blocks - 1], av_and_farther[n_blocks - 1]);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = n_blocks - 1; bb >= 0; bb--)
	{
	  if (bb != n_blocks - 1)
	    {
	      sbitmap_intersect_of_successors (temp_bitmap, earlierin,
					       bb, s_succs);
	      changed |= sbitmap_a_or_b (earlierout[bb],
					 av_and_farther[bb],
					 temp_bitmap);
	    }
	  sbitmap_not (temp_bitmap, avloc[bb]);
	  changed |= sbitmap_a_and_b (earlierin[bb],
				      temp_bitmap,
				      earlierout[bb]);
	}
      passes++;
    }

  /* We're done with this, so go ahead and free it's memory now instead
     of waiting until the end of pre.  */
  free (av_and_farther);
  free (temp_bitmap);
}

/* Compute firstness. 

   This is effectively the same as latestness computed on the reverse
   flow graph.  */

static void
compute_firstout (n_blocks, n_exprs, s_preds, avloc, earlierout, firstout)
     int n_blocks;
     int n_exprs;
     int_list_ptr *s_preds;
     sbitmap *avloc;
     sbitmap *earlierout;
     sbitmap *firstout;
{
  int bb;
  sbitmap temp_bitmap;

  temp_bitmap = sbitmap_alloc (n_exprs);

  for (bb = 0; bb < n_blocks; bb++)
    {
      /* The first block is preceded only by the entry block; therefore,
	 temp_bitmap will not be set by the following call!  */
      if (bb != 0)
	{
	  sbitmap_intersect_of_predecessors (temp_bitmap, earlierout,
					     bb, s_preds);
	  sbitmap_not (temp_bitmap, temp_bitmap);
	}
      else
	{
	  sbitmap_ones (temp_bitmap);
	}
      sbitmap_a_and_b_or_c (firstout[bb], earlierout[bb],
			    avloc[bb], temp_bitmap);
    }
  free (temp_bitmap);
}

/* Compute reverse isolated.

   This is effectively the same as isolatedness computed on the reverse
   flow graph.  */

static void
compute_rev_isoinout (n_blocks, s_preds, avloc, firstout,
		      rev_isoin, rev_isoout)
     int n_blocks;
     int_list_ptr *s_preds;
     sbitmap *avloc;
     sbitmap *firstout;
     sbitmap *rev_isoin;
     sbitmap *rev_isoout;
{
  int bb, changed, passes;

  sbitmap_vector_zero (rev_isoout, n_blocks);
  sbitmap_zero (rev_isoin[0]);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = 0; bb < n_blocks; bb++)
	{
	  if (bb != 0)
	    sbitmap_intersect_of_predecessors (rev_isoin[bb], rev_isoout,
					       bb, s_preds);
	  changed |= sbitmap_union_of_diff (rev_isoout[bb], firstout[bb],
					    rev_isoin[bb], avloc[bb]);
	}
      passes++;
    }
}

/* Edge based lcm routines.  */

/* Compute expression anticipatability at entrance and exit of each block. 
   This is done based on the flow graph, and not on the pred-succ lists.  
   Other than that, its pretty much identical to compute_antinout.  */

static void
compute_antinout_edge (antloc, transp, antin, antout)
     sbitmap *antloc;
     sbitmap *transp;
     sbitmap *antin;
     sbitmap *antout;
{
  int i, changed, passes;
  sbitmap old_changed, new_changed;
  edge e;

  sbitmap_vector_zero (antout, n_basic_blocks);
  sbitmap_vector_ones (antin, n_basic_blocks);

  old_changed = sbitmap_alloc (n_basic_blocks);
  new_changed = sbitmap_alloc (n_basic_blocks);
  sbitmap_ones (old_changed);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      sbitmap_zero (new_changed);

      /* We scan the blocks in the reverse order to speed up
	 the convergence.  */
      for (i = n_basic_blocks - 1; i >= 0; i--)
	{
	  basic_block bb = BASIC_BLOCK (i);
	  /* If none of the successors of this block have changed,
	     then this block is not going to change.  */
	  for (e = bb->succ ; e; e = e->succ_next)
	    {
	      if (e->dest == EXIT_BLOCK_PTR)
		break;

	      if (TEST_BIT (old_changed, e->dest->index)
		  || TEST_BIT (new_changed, e->dest->index))
		break;
	    }

	  if (!e)
	    continue;

          /* If an Exit blocks is the ONLY successor, its has a zero ANTIN, 
	     which is the opposite of the default definition for an 
	     intersection of succs definition.  */
	  if (e->dest == EXIT_BLOCK_PTR && e->succ_next == NULL 
	      && e->src->succ == e)
	    sbitmap_zero (antout[bb->index]);
	  else
	    {
	      sbitmap_intersection_of_succs (antout[bb->index],
					     antin, 
					     bb->index);
	    }

 	  if (sbitmap_a_or_b_and_c (antin[bb->index], antloc[bb->index],
				    transp[bb->index], antout[bb->index]))
	    {
	      changed = 1;
	      SET_BIT (new_changed, bb->index);
	    }
	}
      sbitmap_copy (old_changed, new_changed);
      passes++;
    }

  free (old_changed);
  free (new_changed);
}

/* Compute the earliest vector for edge based lcm.  */
static void
compute_earliest (edge_list, n_exprs, antin, antout, avout, kill, earliest)
     struct edge_list *edge_list;
     int n_exprs;
     sbitmap *antin, *antout, *avout, *kill, *earliest;
{
  sbitmap difference, temp_bitmap;
  int x, num_edges; 
  basic_block pred, succ;

  num_edges = NUM_EDGES (edge_list);

  difference = sbitmap_alloc (n_exprs);
  temp_bitmap = sbitmap_alloc (n_exprs);

  for (x = 0; x < num_edges; x++)
    {
      pred = INDEX_EDGE_PRED_BB (edge_list, x);
      succ = INDEX_EDGE_SUCC_BB (edge_list, x);
      if (pred == ENTRY_BLOCK_PTR)
	sbitmap_copy (earliest[x], antin[succ->index]);
      else
        {
	  if (succ == EXIT_BLOCK_PTR)
	    {
	      sbitmap_zero (earliest[x]);
	    }
	  else
	    {
	      sbitmap_difference (difference, antin[succ->index], 
	      			  avout[pred->index]);
	      sbitmap_not (temp_bitmap, antout[pred->index]);
	      sbitmap_a_and_b_or_c (earliest[x], difference, kill[pred->index], 
				    temp_bitmap);
	    }
	}
    }
  free (temp_bitmap);
  free (difference);
}

/* Compute later and laterin vectors for edge based lcm.  */
static void
compute_laterin (edge_list, n_exprs,
		 earliest, antloc, later, laterin)
     struct edge_list *edge_list;
     int n_exprs;
     sbitmap *earliest, *antloc, *later, *laterin;
{
  sbitmap difference, temp_bitmap;
  int x, num_edges; 
  basic_block pred, succ;
  int done = 0;

  num_edges = NUM_EDGES (edge_list);

  /* Laterin has an extra block allocated for the exit block.  */
  sbitmap_vector_ones (laterin, n_basic_blocks + 1);
  sbitmap_vector_zero (later, num_edges);

  /* Initialize laterin to the intersection of EARLIEST for all edges
     from predecessors to this block. */

  for (x = 0; x < num_edges; x++)
    {
      succ = INDEX_EDGE_SUCC_BB (edge_list, x);
      pred = INDEX_EDGE_PRED_BB (edge_list, x);
      if (succ != EXIT_BLOCK_PTR)
	sbitmap_a_and_b (laterin[succ->index], laterin[succ->index], 
			 earliest[x]);
      /* We already know the correct value of later for edges from
         the entry node, so set it now.  */
      if (pred == ENTRY_BLOCK_PTR)
	sbitmap_copy (later[x], earliest[x]);
    }

  difference = sbitmap_alloc (n_exprs);

  while (!done)
    {
      done = 1;
      for (x = 0; x < num_edges; x++)
	{
          pred = INDEX_EDGE_PRED_BB (edge_list, x);
	  if (pred != ENTRY_BLOCK_PTR)
	    {
	      sbitmap_difference (difference, laterin[pred->index], 
	      			  antloc[pred->index]);
	      if (sbitmap_a_or_b (later[x], difference, earliest[x]))
		done = 0;
	    }
	}
      if (done)
        break;

      sbitmap_vector_ones (laterin, n_basic_blocks);

      for (x = 0; x < num_edges; x++)
	{
	  succ = INDEX_EDGE_SUCC_BB (edge_list, x);
	  if (succ != EXIT_BLOCK_PTR)
	    sbitmap_a_and_b (laterin[succ->index], laterin[succ->index], 
	    		     later[x]);
	  else
	    /* We allocated an extra block for the exit node.  */
	    sbitmap_a_and_b (laterin[n_basic_blocks], laterin[n_basic_blocks], 
	    		     later[x]);
	}
    }

  free (difference);
}

/* Compute the insertion and deletion points for edge based LCM.  */
static void
compute_insert_delete (edge_list, antloc, later, laterin,
		       insert, delete)
     struct edge_list *edge_list;
     sbitmap *antloc, *later, *laterin, *insert, *delete;
{
  int x;

  for (x = 0; x < n_basic_blocks; x++)
    sbitmap_difference (delete[x], antloc[x], laterin[x]);
     
  for (x = 0; x < NUM_EDGES (edge_list); x++)
    {
      basic_block b = INDEX_EDGE_SUCC_BB (edge_list, x);
      if (b == EXIT_BLOCK_PTR)
	sbitmap_difference (insert[x], later[x], laterin[n_basic_blocks]);
      else
	sbitmap_difference (insert[x], later[x], laterin[b->index]);
    }
}

/* Given local properties TRANSP, ANTLOC, AVOUT, KILL return the 
   insert and delete vectors for edge based LCM.  Returns an
   edgelist which is used to map the insert vector to what edge
   an expression should be inserted on.  */

struct edge_list *
pre_edge_lcm (file, n_exprs, transp, avloc, antloc, kill, insert, delete)
     FILE *file;
     int n_exprs;
     sbitmap *transp;
     sbitmap *avloc;
     sbitmap *antloc;
     sbitmap *kill;
     sbitmap **insert;
     sbitmap **delete;
{
  sbitmap *antin, *antout, *earliest;
  sbitmap *avin, *avout;
  sbitmap *later, *laterin;
  struct edge_list *edge_list;
  int num_edges;

  edge_list = create_edge_list ();
  num_edges = NUM_EDGES (edge_list);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      fprintf (file, "Edge List:\n");
      verify_edge_list (file, edge_list);
      print_edge_list (file, edge_list);
      dump_sbitmap_vector (file, "transp", "", transp, n_basic_blocks);
      dump_sbitmap_vector (file, "antloc", "", antloc, n_basic_blocks);
      dump_sbitmap_vector (file, "avloc", "", avloc, n_basic_blocks);
      dump_sbitmap_vector (file, "kill", "", kill, n_basic_blocks);
    }
#endif

  /* Compute global availability.  */
  avin = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  avout = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  compute_available (avloc, kill, avout, avin);

  free (avin);

  /* Compute global anticipatability.  */
  antin = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  antout = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  compute_antinout_edge (antloc, transp, antin, antout);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "antin", "", antin, n_basic_blocks);
      dump_sbitmap_vector (file, "antout", "", antout, n_basic_blocks);
    }
#endif

  /* Compute earliestness.  */
  earliest = sbitmap_vector_alloc (num_edges, n_exprs);
  compute_earliest (edge_list, n_exprs, antin, antout, avout, kill, earliest);

#ifdef LCM_DEBUG_INFO
  if (file)
    dump_sbitmap_vector (file, "earliest", "", earliest, num_edges);
#endif

  free (antout);
  free (antin);
  free (avout);

  later = sbitmap_vector_alloc (num_edges, n_exprs);
  /* Allocate an extra element for the exit block in the laterin vector.  */
  laterin = sbitmap_vector_alloc (n_basic_blocks + 1, n_exprs);
  compute_laterin (edge_list, n_exprs, earliest, antloc, later, laterin);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "laterin", "", laterin, n_basic_blocks + 1);
      dump_sbitmap_vector (file, "later", "", later, num_edges);
    }
#endif

  free (earliest);

  *insert = sbitmap_vector_alloc (num_edges, n_exprs);
  *delete = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  compute_insert_delete (edge_list, antloc, later, laterin, *insert, *delete);

  free (laterin);
  free (later);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "pre_insert_map", "", *insert, num_edges);
      dump_sbitmap_vector (file, "pre_delete_map", "", *delete, n_basic_blocks);
    }
#endif

  return edge_list;
}

int
compute_available (avloc, kill, avout, avin)
     sbitmap *avloc, *kill, *avout, *avin;  
{
  int bb, changed, passes;
  int last = n_basic_blocks - 1;

  sbitmap_zero (avin[0]);
  sbitmap_copy (avout[0] /*dst*/, avloc[0] /*src*/);

  for (bb = 1; bb < n_basic_blocks; bb++)
    sbitmap_not (avout[bb], kill[bb]);
    
  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = 1; bb < n_basic_blocks; bb++)
        {
          sbitmap_intersection_of_preds (avin[bb], avout, bb);
          changed |= sbitmap_union_of_diff (avout[bb], avloc[bb],
                                            avin[bb], kill[bb]);
        }
      passes++;
    }
  return passes;
}
/* Reverse version of the edge based routines.  */

static void
compute_st_antin (st_antloc, st_kill, st_antout, st_antin)
     sbitmap *st_antloc, *st_kill, *st_antout, *st_antin;  
{
  int bb, changed, passes;
  int last = n_basic_blocks - 1;

  sbitmap_zero (st_antout[0]);

  sbitmap_vector_ones (st_antin, n_basic_blocks);
  sbitmap_copy (st_antin[0] /*dst*/, st_antloc[0] /*src*/);

  for (bb = 1; bb < n_basic_blocks; bb++)
    sbitmap_difference (st_antin[bb], st_antin[bb], st_kill[bb]);
    
  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      for (bb = 1; bb < n_basic_blocks; bb++)
        {
          sbitmap_intersection_of_preds (st_antout[bb], st_antin, bb);
          changed |= sbitmap_union_of_diff (st_antin[bb], st_antloc[bb],
                                            st_antout[bb], st_kill[bb]);
        }
      passes++;
    }
  return passes;
}
/* Reverse version of the edge based routines.  */

static void
compute_st_avinout_edge (st_avloc, st_kill, st_avout, st_avin)
     sbitmap *st_avloc;
     sbitmap *st_kill;
     sbitmap *st_avout;
     sbitmap *st_avin;
{
  int i, changed, passes;
  sbitmap old_changed, new_changed;
  edge e;

  sbitmap_vector_zero (st_avin, n_basic_blocks);
  sbitmap_vector_ones (st_avout, n_basic_blocks);

  old_changed = sbitmap_alloc (n_basic_blocks);
  new_changed = sbitmap_alloc (n_basic_blocks);
  sbitmap_ones (old_changed);

  passes = 0;
  changed = 1;
  while (changed)
    {
      changed = 0;
      sbitmap_zero (new_changed);

      /* We scan the blocks in the reverse order to speed up
	 the convergence.  */
      for (i = n_basic_blocks - 1; i >= 0; i--)
	{
	  basic_block bb = BASIC_BLOCK (i);
	  /* If none of the predecessors of this block have changed,
	     then this block is not going to change.  */
	  for (e = bb->succ ; e; e = e->succ_next)
	    {
	      if (e->dest == EXIT_BLOCK_PTR)
		break;

	      if (TEST_BIT (old_changed, e->src->index)
		  || TEST_BIT (new_changed, e->src->index))
		break;
	    }

	  if (!e)
	    continue;

	  if (i != n_basic_blocks - 1)
	    {
	      sbitmap_intersection_of_succs (st_avin[bb->index],
					     st_avout, 
					     bb->index);
	    }

 	  if (sbitmap_union_of_diff (st_avout[bb->index], st_avloc[bb->index],
				    st_avin[bb->index], st_kill[bb->index]))
	    {
	      changed = 1;
	      SET_BIT (new_changed, bb->index);
	    }
	}
      sbitmap_copy (old_changed, new_changed);
      passes++;
    }

  free (old_changed);
  free (new_changed);
}

/* Compute the farthest vector for edge based lcm.  */
static void
compute_farthest (edge_list, n_exprs, st_avout, st_avin, st_antin, 
		  kill, farthest)
     struct edge_list *edge_list;
     int n_exprs;
     sbitmap *st_avout, *st_avin, *st_antin, *kill, *farthest;
{
  sbitmap difference, temp_bitmap;
  int x, num_edges; 
  basic_block pred, succ;

  num_edges = NUM_EDGES (edge_list);

  difference = sbitmap_alloc (n_exprs);
  temp_bitmap = sbitmap_alloc (n_exprs);

  for (x = 0; x < num_edges; x++)
    {
      pred = INDEX_EDGE_PRED_BB (edge_list, x);
      succ = INDEX_EDGE_SUCC_BB (edge_list, x);
      if (succ == EXIT_BLOCK_PTR)
	sbitmap_copy (farthest[x], st_avout[pred->index]);
      else
	{
	  if (pred == ENTRY_BLOCK_PTR)
	    {
	      sbitmap_zero (farthest[x]);
	    }
	  else
	    {
	      sbitmap_difference (difference, st_avout[pred->index], 
				  st_antin[succ->index]);
	      sbitmap_not (temp_bitmap, st_avin[succ->index]);
	      sbitmap_a_and_b_or_c (farthest[x], difference, 
				    kill[succ->index], temp_bitmap);
	    }
	}
    }
  free (temp_bitmap);
  free (difference);
}

/* Compute nearer and nearerout vectors for edge based lcm.  */
static void
compute_nearerout (edge_list, n_exprs,
		   farthest, st_avloc, nearer, nearerout)
     struct edge_list *edge_list;
     int n_exprs;
     sbitmap *farthest, *st_avloc, *nearer, *nearerout;
{
  sbitmap difference, temp_bitmap;
  int x, num_edges; 
  basic_block pred, succ;
  int done = 0;

  num_edges = NUM_EDGES (edge_list);

  /* nearout has an extra block allocated for the entry block.  */
  sbitmap_vector_ones (nearerout, n_basic_blocks + 1);
  sbitmap_vector_zero (nearer, num_edges);

  /* Initialize nearerout to the intersection of FARTHEST for all edges
     from predecessors to this block. */

  for (x = 0; x < num_edges; x++)
    {
      succ = INDEX_EDGE_SUCC_BB (edge_list, x);
      pred = INDEX_EDGE_PRED_BB (edge_list, x);
      if (pred != ENTRY_BLOCK_PTR)
        {
	  sbitmap_a_and_b (nearerout[pred->index], nearerout[pred->index], 
			   farthest[x]);
	}
      /* We already know the correct value of nearer for edges to 
         the exit node.  */
      if (succ == EXIT_BLOCK_PTR)
	sbitmap_copy (nearer[x], farthest[x]);
    }

  difference = sbitmap_alloc (n_exprs);

  while (!done)
    {
      done = 1;
      for (x = 0; x < num_edges; x++)
	{
          succ = INDEX_EDGE_SUCC_BB (edge_list, x);
	  if (succ != EXIT_BLOCK_PTR)
	    {
	      sbitmap_difference (difference, nearerout[succ->index], 
				  st_avloc[succ->index]);
	      if (sbitmap_a_or_b (nearer[x], difference, farthest[x]))
		done = 0;
	    }
	}

      if (done)
        break;

      sbitmap_vector_zero (nearerout, n_basic_blocks);

      for (x = 0; x < num_edges; x++)
	{
	  pred = INDEX_EDGE_PRED_BB (edge_list, x);
	  if (pred != ENTRY_BLOCK_PTR)
	      sbitmap_a_and_b (nearerout[pred->index], 
			       nearerout[pred->index], nearer[x]);
	    else
	      sbitmap_a_and_b (nearerout[n_basic_blocks], 
			       nearerout[n_basic_blocks], nearer[x]);
	}
    }

  free (difference);
}

/* Compute the insertion and deletion points for edge based LCM.  */
static void
compute_rev_insert_delete (edge_list, st_avloc, nearer, nearerout,
			   insert, delete)
     struct edge_list *edge_list;
     sbitmap *st_avloc, *nearer, *nearerout, *insert, *delete;
{
  int x;

  for (x = 0; x < n_basic_blocks; x++)
    sbitmap_difference (delete[x], st_avloc[x], nearerout[x]);
     
  for (x = 0; x < NUM_EDGES (edge_list); x++)
    {
      basic_block b = INDEX_EDGE_PRED_BB (edge_list, x);
      if (b == ENTRY_BLOCK_PTR)
	sbitmap_difference (insert[x], nearer[x], nearerout[n_basic_blocks]);
      else
	sbitmap_difference (insert[x], nearer[x], nearerout[b->index]);
    }
}

/* Given local properties TRANSP, ST_AVLOC, ST_ANTLOC, KILL return the 
   insert and delete vectors for edge based reverse LCM.  Returns an
   edgelist which is used to map the insert vector to what edge
   an expression should be inserted on.  */

struct edge_list *
pre_edge_rev_lcm (file, n_exprs, transp, st_avloc, st_antloc, kill, 
		  insert, delete)
     FILE *file;
     int n_exprs;
     sbitmap *transp;
     sbitmap *st_avloc;
     sbitmap *st_antloc;
     sbitmap *kill;
     sbitmap **insert;
     sbitmap **delete;
{
  sbitmap *st_antin, *st_antout;
  sbitmap *st_avout, *st_avin, *farthest;
  sbitmap *nearer, *nearerout;
  struct edge_list *edge_list;
  int x,num_edges;

  edge_list = create_edge_list ();
  num_edges = NUM_EDGES (edge_list);

  st_antin = (sbitmap *) sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  st_antout = (sbitmap *) sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  sbitmap_vector_zero (st_antin, n_basic_blocks);
  sbitmap_vector_zero (st_antout, n_basic_blocks);
  compute_antinout_edge (st_antloc, transp, st_antin, st_antout);

  /* Compute global anticipatability.  */
  st_avout = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  st_avin = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  compute_available (st_avloc, kill, st_avout, st_avin);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      fprintf (file, "Edge List:\n");
      verify_edge_list (file, edge_list);
      print_edge_list (file, edge_list);
      dump_sbitmap_vector (file, "transp", "", transp, n_basic_blocks);
      dump_sbitmap_vector (file, "st_avloc", "", st_avloc, n_basic_blocks);
      dump_sbitmap_vector (file, "st_antloc", "", st_antloc, n_basic_blocks);
      dump_sbitmap_vector (file, "st_antin", "", st_antin, n_basic_blocks);
      dump_sbitmap_vector (file, "st_antout", "", st_antout, n_basic_blocks);
      dump_sbitmap_vector (file, "st_kill", "", kill, n_basic_blocks);
    }
#endif

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "st_avout", "", st_avout, n_basic_blocks);
      dump_sbitmap_vector (file, "st_avin", "", st_avin, n_basic_blocks);
    }
#endif

  /* Compute farthestness.  */
  farthest = sbitmap_vector_alloc (num_edges, n_exprs);
  compute_farthest (edge_list, n_exprs, st_avout, st_avin, st_antin, 
		    kill, farthest);

#ifdef LCM_DEBUG_INFO
  if (file)
    dump_sbitmap_vector (file, "farthest", "", farthest, num_edges);
#endif

  free (st_avin);
  free (st_avout);

  nearer = sbitmap_vector_alloc (num_edges, n_exprs);
  /* Allocate an extra element for the entry block.  */
  nearerout = sbitmap_vector_alloc (n_basic_blocks + 1, n_exprs);
  compute_nearerout (edge_list, n_exprs, farthest, st_avloc, nearer, nearerout);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "nearerout", "", nearerout, 
			   n_basic_blocks + 1);
      dump_sbitmap_vector (file, "nearer", "", nearer, num_edges);
    }
#endif

  free (farthest);

  *insert = sbitmap_vector_alloc (num_edges, n_exprs);
  *delete = sbitmap_vector_alloc (n_basic_blocks, n_exprs);
  compute_rev_insert_delete (edge_list, st_avloc, nearer, nearerout, *insert, *delete);

  free (nearerout);
  free (nearer);

#ifdef LCM_DEBUG_INFO
  if (file)
    {
      dump_sbitmap_vector (file, "pre_insert_map", "", *insert, num_edges);
      dump_sbitmap_vector (file, "pre_delete_map", "", *delete, n_basic_blocks);
    }
#endif

  return edge_list;
}
