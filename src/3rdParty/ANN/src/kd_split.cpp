//----------------------------------------------------------------------
// File:			kd_split.cpp
// Programmer:		Sunil Arya and David Mount
// Description:		Methods for splitting kd-trees
// Last modified:	01/04/05 (Version 1.0)
//----------------------------------------------------------------------
// Copyright (c) 1997-2005 University of Maryland and Sunil Arya and
// David Mount.  All Rights Reserved.
// 
// This software and related documentation is part of the Approximate
// Nearest Neighbor Library (ANN).  This software is provided under
// the provisions of the Lesser GNU Public License (LGPL).  See the
// file ../ReadMe.txt for further information.
// 
// The University of Maryland (U.M.) and the authors make no
// representations about the suitability or fitness of this software for
// any purpose.  It is provided "as is" without express or implied
// warranty.
//----------------------------------------------------------------------
// History:
//	Revision 0.1  03/04/98
//		Initial release
//	Revision 1.0  04/01/05
//----------------------------------------------------------------------

#include "kd_tree.h"					// kd-tree definitions
#include "kd_util.h"					// kd-tree utilities
#include "kd_split.h"					// splitting functions

//----------------------------------------------------------------------
//	Constants
//----------------------------------------------------------------------

const double ERR = 0.001;				// a small value
const double FS_ASPECT_RATIO = 3.0;		// maximum allowed aspect ratio
										// in fair split. Must be >= 2.

//----------------------------------------------------------------------
//	kd_split - Bentley's standard splitting routine for kd-trees
//		Find the dimension of the greatest spread, and split
//		just before the median point along this dimension.
//----------------------------------------------------------------------

void kd_split(
	ANNpointArray		pa,				// point array (permuted on return)
	ANNidxArray			pidx,			// point indices
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo)			// num of points on low side (returned)
{
										// find dimension of maximum spread
	cut_dim = annMaxSpread(pa, pidx, n, dim);
	n_lo = n/2;							// median rank
										// split about median
	annMedianSplit(pa, pidx, n, cut_dim, cut_val, n_lo);
}

//----------------------------------------------------------------------
//	midpt_split - midpoint splitting rule for box-decomposition trees
//
//		This is the simplest splitting rule that guarantees boxes
//		of bounded aspect ratio.  It simply cuts the box with the
//		longest side through its midpoint.  If there are ties, it
//		selects the dimension with the maximum point spread.
//
//		WARNING: This routine (while simple) doesn't seem to work
//		well in practice in high dimensions, because it tends to
//		generate a large number of trivial and/or unbalanced splits.
//		Either kd_split(), sl_midpt_split(), or fair_split() are
//		recommended, instead.
//----------------------------------------------------------------------

void midpt_split(
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo)			// num of points on low side (returned)
{
	int d;

	ANNcoord max_length = bnds.hi[0] - bnds.lo[0];
	for (d = 1; d < dim; d++) {			// find length of longest box side
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (length > max_length) {
			max_length = length;
		}
	}
	ANNcoord max_spread = -1;			// find long side with most spread
	for (d = 0; d < dim; d++) {
										// is it among longest?
		if (double(bnds.hi[d] - bnds.lo[d]) >= (1-ERR)*max_length) {
										// compute its spread
			ANNcoord spr = annSpread(pa, pidx, n, d);
			if (spr > max_spread) {		// is it max so far?
				max_spread = spr;
				cut_dim = d;
			}
		}
	}
										// split along cut_dim at midpoint
	cut_val = (bnds.lo[cut_dim] + bnds.hi[cut_dim]) / 2;
										// permute points accordingly
	int br1, br2;
	annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
	//------------------------------------------------------------------
	//	On return:		pa[0..br1-1] < cut_val
	//					pa[br1..br2-1] == cut_val
	//					pa[br2..n-1] > cut_val
	//
	//	We can set n_lo to any value in the range [br1..br2].
	//	We choose split so that points are most evenly divided.
	//------------------------------------------------------------------
	if (br1 > n/2) n_lo = br1;
	else if (br2 < n/2) n_lo = br2;
	else n_lo = n/2;
}

//----------------------------------------------------------------------
//	sl_midpt_split - sliding midpoint splitting rule
//
//		This is a modification of midpt_split, which has the nonsensical
//		name "sliding midpoint".  The idea is that we try to use the
//		midpoint rule, by bisecting the longest side.  If there are
//		ties, the dimension with the maximum spread is selected.  If,
//		however, the midpoint split produces a trivial split (no points
//		on one side of the splitting plane) then we slide the splitting
//		(maintaining its orientation) until it produces a nontrivial
//		split. For example, if the splitting plane is along the x-axis,
//		and all the data points have x-coordinate less than the x-bisector,
//		then the split is taken along the maximum x-coordinate of the
//		data points.
//
//		Intuitively, this rule cannot generate trivial splits, and
//		hence avoids midpt_split's tendency to produce trees with
//		a very large number of nodes.
//
//----------------------------------------------------------------------

void sl_midpt_split(
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo)			// num of points on low side (returned)
{
	int d;

	ANNcoord max_length = bnds.hi[0] - bnds.lo[0];
	for (d = 1; d < dim; d++) {			// find length of longest box side
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (length > max_length) {
			max_length = length;
		}
	}
	ANNcoord max_spread = -1;			// find long side with most spread
	for (d = 0; d < dim; d++) {
										// is it among longest?
		if ((bnds.hi[d] - bnds.lo[d]) >= (1-ERR)*max_length) {
										// compute its spread
			ANNcoord spr = annSpread(pa, pidx, n, d);
			if (spr > max_spread) {		// is it max so far?
				max_spread = spr;
				cut_dim = d;
			}
		}
	}
										// ideal split at midpoint
	ANNcoord ideal_cut_val = (bnds.lo[cut_dim] + bnds.hi[cut_dim])/2;

	ANNcoord min, max;
	annMinMax(pa, pidx, n, cut_dim, min, max);	// find min/max coordinates

	if (ideal_cut_val < min)			// slide to min or max as needed
		cut_val = min;
	else if (ideal_cut_val > max)
		cut_val = max;
	else
		cut_val = ideal_cut_val;

										// permute points accordingly
	int br1, br2;
	annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
	//------------------------------------------------------------------
	//	On return:		pa[0..br1-1] < cut_val
	//					pa[br1..br2-1] == cut_val
	//					pa[br2..n-1] > cut_val
	//
	//	We can set n_lo to any value in the range [br1..br2] to satisfy
	//	the exit conditions of the procedure.
	//
	//	if ideal_cut_val < min (implying br2 >= 1),
	//			then we select n_lo = 1 (so there is one point on left) and
	//	if ideal_cut_val > max (implying br1 <= n-1),
	//			then we select n_lo = n-1 (so there is one point on right).
	//	Otherwise, we select n_lo as close to n/2 as possible within
	//			[br1..br2].
	//------------------------------------------------------------------
	if (ideal_cut_val < min) n_lo = 1;
	else if (ideal_cut_val > max) n_lo = n-1;
	else if (br1 > n/2) n_lo = br1;
	else if (br2 < n/2) n_lo = br2;
	else n_lo = n/2;
}

//----------------------------------------------------------------------
//	fair_split - fair-split splitting rule
//
//		This is a compromise between the kd-tree splitting rule (which
//		always splits data points at their median) and the midpoint
//		splitting rule (which always splits a box through its center.
//		The goal of this procedure is to achieve both nicely balanced
//		splits, and boxes of bounded aspect ratio.
//
//		A constant FS_ASPECT_RATIO is defined. Given a box, those sides
//		which can be split so that the ratio of the longest to shortest
//		side does not exceed ASPECT_RATIO are identified.  Among these
//		sides, we select the one in which the points have the largest
//		spread. We then split the points in a manner which most evenly
//		distributes the points on either side of the splitting plane,
//		subject to maintaining the bound on the ratio of long to short
//		sides. To determine that the aspect ratio will be preserved,
//		we determine the longest side (other than this side), and
//		determine how narrowly we can cut this side, without causing the
//		aspect ratio bound to be exceeded (small_piece).
//
//		This procedure is more robust than either kd_split or midpt_split,
//		but is more complicated as well.  When point distribution is
//		extremely skewed, this degenerates to midpt_split (actually
//		1/3 point split), and when the points are most evenly distributed,
//		this degenerates to kd-split.
//----------------------------------------------------------------------

void fair_split(
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo)			// num of points on low side (returned)
{
	int d;
	ANNcoord max_length = bnds.hi[0] - bnds.lo[0];
	cut_dim = 0;
	for (d = 1; d < dim; d++) {			// find length of longest box side
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (length > max_length) {
			max_length = length;
			cut_dim = d;
		}
	}

	ANNcoord max_spread = 0;			// find legal cut with max spread
	cut_dim = 0;
	for (d = 0; d < dim; d++) {
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
										// is this side midpoint splitable
										// without violating aspect ratio?
		if (((double) max_length)*2.0/((double) length) <= FS_ASPECT_RATIO) {
										// compute spread along this dim
			ANNcoord spr = annSpread(pa, pidx, n, d);
			if (spr > max_spread) {		// best spread so far
				max_spread = spr;
				cut_dim = d;			// this is dimension to cut
			}
		}
	}

	max_length = 0;						// find longest side other than cut_dim
	for (d = 0; d < dim; d++) {
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (d != cut_dim && length > max_length)
			max_length = length;
	}
										// consider most extreme splits
	ANNcoord small_piece = max_length / FS_ASPECT_RATIO;
	ANNcoord lo_cut = bnds.lo[cut_dim] + small_piece;// lowest legal cut
	ANNcoord hi_cut = bnds.hi[cut_dim] - small_piece;// highest legal cut

	int br1, br2;
										// is median below lo_cut ?
	if (annSplitBalance(pa, pidx, n, cut_dim, lo_cut) >= 0) {
		cut_val = lo_cut;				// cut at lo_cut
		annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
		n_lo = br1;
	}
										// is median above hi_cut?
	else if (annSplitBalance(pa, pidx, n, cut_dim, hi_cut) <= 0) {
		cut_val = hi_cut;				// cut at hi_cut
		annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
		n_lo = br2;
	}
	else {								// median cut preserves asp ratio
		n_lo = n/2;						// split about median
		annMedianSplit(pa, pidx, n, cut_dim, cut_val, n_lo);
	}
}

//----------------------------------------------------------------------
//	sl_fair_split - sliding fair split splitting rule
//
//		Sliding fair split is a splitting rule that combines the
//		strengths of both fair split with sliding midpoint split.
//		Fair split tends to produce balanced splits when the points
//		are roughly uniformly distributed, but it can produce many
//		trivial splits when points are highly clustered.  Sliding
//		midpoint never produces trivial splits, and shrinks boxes
//		nicely if points are highly clustered, but it may produce
//		rather unbalanced splits when points are unclustered but not
//		quite uniform.
//
//		Sliding fair split is based on the theory that there are two
//		types of splits that are "good": balanced splits that produce
//		fat boxes, and unbalanced splits provided the cell with fewer
//		points is fat.
//
//		This splitting rule operates by first computing the longest
//		side of the current bounding box.  Then it asks which sides
//		could be split (at the midpoint) and still satisfy the aspect
//		ratio bound with respect to this side.	Among these, it selects
//		the side with the largest spread (as fair split would).	 It
//		then considers the most extreme cuts that would be allowed by
//		the aspect ratio bound.	 This is done by dividing the longest
//		side of the box by the aspect ratio bound.	If the median cut
//		lies between these extreme cuts, then we use the median cut.
//		If not, then consider the extreme cut that is closer to the
//		median.	 If all the points lie to one side of this cut, then
//		we slide the cut until it hits the first point.	 This may
//		violate the aspect ratio bound, but will never generate empty
//		cells.	However the sibling of every such skinny cell is fat,
//		and hence packing arguments still apply.
//
//----------------------------------------------------------------------

void sl_fair_split(
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo)			// num of points on low side (returned)
{
	int d;
	ANNcoord min, max;					// min/max coordinates
	int br1, br2;						// split break points

	ANNcoord max_length = bnds.hi[0] - bnds.lo[0];
	cut_dim = 0;
	for (d = 1; d < dim; d++) {			// find length of longest box side
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (length	> max_length) {
			max_length = length;
			cut_dim = d;
		}
	}

	ANNcoord max_spread = 0;			// find legal cut with max spread
	cut_dim = 0;
	for (d = 0; d < dim; d++) {
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
										// is this side midpoint splitable
										// without violating aspect ratio?
		if (((double) max_length)*2.0/((double) length) <= FS_ASPECT_RATIO) {
										// compute spread along this dim
			ANNcoord spr = annSpread(pa, pidx, n, d);
			if (spr > max_spread) {		// best spread so far
				max_spread = spr;
				cut_dim = d;			// this is dimension to cut
			}
		}
	}

	max_length = 0;						// find longest side other than cut_dim
	for (d = 0; d < dim; d++) {
		ANNcoord length = bnds.hi[d] - bnds.lo[d];
		if (d != cut_dim && length > max_length)
			max_length = length;
	}
										// consider most extreme splits
	ANNcoord small_piece = max_length / FS_ASPECT_RATIO;
	ANNcoord lo_cut = bnds.lo[cut_dim] + small_piece;// lowest legal cut
	ANNcoord hi_cut = bnds.hi[cut_dim] - small_piece;// highest legal cut
										// find min and max along cut_dim
	annMinMax(pa, pidx, n, cut_dim, min, max);
										// is median below lo_cut?
	if (annSplitBalance(pa, pidx, n, cut_dim, lo_cut) >= 0) {
		if (max > lo_cut) {				// are any points above lo_cut?
			cut_val = lo_cut;			// cut at lo_cut
			annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
			n_lo = br1;					// balance if there are ties
		}
		else {							// all points below lo_cut
			cut_val = max;				// cut at max value
			annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
			n_lo = n-1;
		}
	}
										// is median above hi_cut?
	else if (annSplitBalance(pa, pidx, n, cut_dim, hi_cut) <= 0) {
		if (min < hi_cut) {				// are any points below hi_cut?
			cut_val = hi_cut;			// cut at hi_cut
			annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
			n_lo = br2;					// balance if there are ties
		}
		else {							// all points above hi_cut
			cut_val = min;				// cut at min value
			annPlaneSplit(pa, pidx, n, cut_dim, cut_val, br1, br2);
			n_lo = 1;
		}
	}
	else {								// median cut is good enough
		n_lo = n/2;						// split about median
		annMedianSplit(pa, pidx, n, cut_dim, cut_val, n_lo);
	}
}
