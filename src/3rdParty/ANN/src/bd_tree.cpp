//----------------------------------------------------------------------
// File:			bd_tree.cpp
// Programmer:		David Mount
// Description:		Basic methods for bd-trees.
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
//	Revision l.0  04/01/05
//		Fixed centroid shrink threshold condition to depend on the
//			dimension.
//		Moved dump routine to kd_dump.cpp.
//----------------------------------------------------------------------

#include "bd_tree.h"					// bd-tree declarations
#include "kd_util.h"					// kd-tree utilities
#include "kd_split.h"					// kd-tree splitting rules

#include <ANN/ANNperf.h>				// performance evaluation

//----------------------------------------------------------------------
//	Printing a bd-tree 
//		These routines print a bd-tree.   See the analogous procedure
//		in kd_tree.cpp for more information.
//----------------------------------------------------------------------

void ANNbd_shrink::print(				// print shrinking node
		int level,						// depth of node in tree
		ostream &out)					// output stream
{
	child[ANN_OUT]->print(level+1, out);		// print out-child

	out << "    ";
	for (int i = 0; i < level; i++)				// print indentation
		out << "..";
	out << "Shrink";
	for (int j = 0; j < n_bnds; j++) {			// print sides, 2 per line
		if (j % 2 == 0) {
			out << "\n";						// newline and indentation
			for (int i = 0; i < level+2; i++) out << "  ";
		}
		out << "  ([" << bnds[j].cd << "]"
			 << (bnds[j].sd > 0 ? ">=" : "< ")
			 << bnds[j].cv << ")";
	}
	out << "\n";

	child[ANN_IN]->print(level+1, out);			// print in-child
}

//----------------------------------------------------------------------
//	kd_tree statistics utility (for performance evaluation)
//		This routine computes various statistics information for
//		shrinking nodes.  See file kd_tree.cpp for more information.
//----------------------------------------------------------------------

void ANNbd_shrink::getStats(					// get subtree statistics
	int					dim,					// dimension of space
	ANNkdStats			&st,					// stats (modified)
	ANNorthRect			&bnd_box)				// bounding box
{
	ANNkdStats ch_stats;						// stats for children
	ANNorthRect inner_box(dim);					// inner box of shrink

	annBnds2Box(bnd_box,						// enclosing box
				dim,							// dimension
				n_bnds,							// number of bounds
				bnds,							// bounds array
				inner_box);						// inner box (modified)
												// get stats for inner child
	ch_stats.reset();							// reset
	child[ANN_IN]->getStats(dim, ch_stats, inner_box);
	st.merge(ch_stats);							// merge them
												// get stats for outer child
	ch_stats.reset();							// reset
	child[ANN_OUT]->getStats(dim, ch_stats, bnd_box);
	st.merge(ch_stats);							// merge them

	st.depth++;									// increment depth
	st.n_shr++;									// increment number of shrinks
}

//----------------------------------------------------------------------
// bd-tree constructor
//		This is the main constructor for bd-trees given a set of points.
//		It first builds a skeleton kd-tree as a basis, then computes the
//		bounding box of the data points, and then invokes rbd_tree() to
//		actually build the tree, passing it the appropriate splitting
//		and shrinking information.
//----------------------------------------------------------------------

ANNkd_ptr rbd_tree(						// recursive construction of bd-tree
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	int					bsp,			// bucket space
	ANNorthRect			&bnd_box,		// bounding box for current node
	ANNkd_splitter		splitter,		// splitting routine
	ANNshrinkRule		shrink);		// shrinking rule

ANNbd_tree::ANNbd_tree(					// construct from point array
	ANNpointArray		pa,				// point array (with at least n pts)
	int					n,				// number of points
	int					dd,				// dimension
	int					bs,				// bucket size
	ANNsplitRule		split,			// splitting rule
	ANNshrinkRule		shrink)			// shrinking rule
	: ANNkd_tree(n, dd, bs)				// build skeleton base tree
{
	pts = pa;							// where the points are
	if (n == 0) return;					// no points--no sweat

	ANNorthRect bnd_box(dd);			// bounding box for points
										// construct bounding rectangle
	annEnclRect(pa, pidx, n, dd, bnd_box);
										// copy to tree structure
	bnd_box_lo = annCopyPt(dd, bnd_box.lo);
	bnd_box_hi = annCopyPt(dd, bnd_box.hi);

	switch (split) {					// build by rule
	case ANN_KD_STD:					// standard kd-splitting rule
		root = rbd_tree(pa, pidx, n, dd, bs, bnd_box, kd_split, shrink);
		break;
	case ANN_KD_MIDPT:					// midpoint split
		root = rbd_tree(pa, pidx, n, dd, bs, bnd_box, midpt_split, shrink);
		break;
	case ANN_KD_SUGGEST:				// best (in our opinion)
	case ANN_KD_SL_MIDPT:				// sliding midpoint split
		root = rbd_tree(pa, pidx, n, dd, bs, bnd_box, sl_midpt_split, shrink);
		break;
	case ANN_KD_FAIR:					// fair split
		root = rbd_tree(pa, pidx, n, dd, bs, bnd_box, fair_split, shrink);
		break;
	case ANN_KD_SL_FAIR:				// sliding fair split
		root = rbd_tree(pa, pidx, n, dd, bs,
						bnd_box, sl_fair_split, shrink);
		break;
	default:
		annError("Illegal splitting method", ANNabort);
	}
}

//----------------------------------------------------------------------
//	Shrinking rules
//----------------------------------------------------------------------

enum ANNdecomp {SPLIT, SHRINK};			// decomposition methods

//----------------------------------------------------------------------
//	trySimpleShrink - Attempt a simple shrink
//
//		We compute the tight bounding box of the points, and compute
//		the 2*dim ``gaps'' between the sides of the tight box and the
//		bounding box.  If any of the gaps is large enough relative to
//		the longest side of the tight bounding box, then we shrink
//		all sides whose gaps are large enough.  (The reason for
//		comparing against the tight bounding box, is that after
//		shrinking the longest box size will decrease, and if we use
//		the standard bounding box, we may decide to shrink twice in
//		a row.  Since the tight box is fixed, we cannot shrink twice
//		consecutively.)
//----------------------------------------------------------------------
const float BD_GAP_THRESH = 0.5;		// gap threshold (must be < 1)
const int   BD_CT_THRESH  = 2;			// min number of shrink sides

ANNdecomp trySimpleShrink(				// try a simple shrink
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	const ANNorthRect	&bnd_box,		// current bounding box
	ANNorthRect			&inner_box)		// inner box if shrinking (returned)
{
	int i;
												// compute tight bounding box
	annEnclRect(pa, pidx, n, dim, inner_box);

	ANNcoord max_length = 0;					// find longest box side
	for (i = 0; i < dim; i++) {
		ANNcoord length = inner_box.hi[i] - inner_box.lo[i];
		if (length > max_length) {
			max_length = length;
		}
	}

	int shrink_ct = 0;							// number of sides we shrunk
	for (i = 0; i < dim; i++) {					// select which sides to shrink
												// gap between boxes
		ANNcoord gap_hi = bnd_box.hi[i] - inner_box.hi[i];
												// big enough gap to shrink?
		if (gap_hi < max_length*BD_GAP_THRESH)
			inner_box.hi[i] = bnd_box.hi[i];	// no - expand
		else shrink_ct++;						// yes - shrink this side

												// repeat for high side
		ANNcoord gap_lo = inner_box.lo[i] - bnd_box.lo[i];
		if (gap_lo < max_length*BD_GAP_THRESH)
			inner_box.lo[i] = bnd_box.lo[i];	// no - expand
		else shrink_ct++;						// yes - shrink this side
	}

	if (shrink_ct >= BD_CT_THRESH)				// did we shrink enough sides?
		 return SHRINK;
	else return SPLIT;
}

//----------------------------------------------------------------------
//	tryCentroidShrink - Attempt a centroid shrink
//
//	We repeatedly apply the splitting rule, always to the larger subset
//	of points, until the number of points decreases by the constant
//	fraction BD_FRACTION.  If this takes more than dim*BD_MAX_SPLIT_FAC
//	splits for this to happen, then we shrink to the final inner box
//	Otherwise we split.
//----------------------------------------------------------------------

const float	BD_MAX_SPLIT_FAC = 0.5;		// maximum number of splits allowed
const float	BD_FRACTION = 0.5;			// ...to reduce points by this fraction
										// ...This must be < 1.

ANNdecomp tryCentroidShrink(			// try a centroid shrink
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	const ANNorthRect	&bnd_box,		// current bounding box
	ANNkd_splitter		splitter,		// splitting procedure
	ANNorthRect			&inner_box)		// inner box if shrinking (returned)
{
	int n_sub = n;						// number of points in subset
	int n_goal = (int) (n*BD_FRACTION); // number of point in goal
	int n_splits = 0;					// number of splits needed
										// initialize inner box to bounding box
	annAssignRect(dim, inner_box, bnd_box);

	while (n_sub > n_goal) {			// keep splitting until goal reached
		int cd;							// cut dim from splitter (ignored)
		ANNcoord cv;					// cut value from splitter (ignored)
		int n_lo;						// number of points on low side
										// invoke splitting procedure
		(*splitter)(pa, pidx, inner_box, n_sub, dim, cd, cv, n_lo);
		n_splits++;						// increment split count

		if (n_lo >= n_sub/2) {			// most points on low side
			inner_box.hi[cd] = cv;		// collapse high side
			n_sub = n_lo;				// recurse on lower points
		}
		else {							// most points on high side
			inner_box.lo[cd] = cv;		// collapse low side
			pidx += n_lo;				// recurse on higher points
			n_sub -= n_lo;
		}
	}
    if (n_splits > dim*BD_MAX_SPLIT_FAC)// took too many splits
		return SHRINK;					// shrink to final subset
	else
		return SPLIT;
}

//----------------------------------------------------------------------
//	selectDecomp - select which decomposition to use
//----------------------------------------------------------------------

ANNdecomp selectDecomp(			// select decomposition method
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	const ANNorthRect	&bnd_box,		// current bounding box
	ANNkd_splitter		splitter,		// splitting procedure
	ANNshrinkRule		shrink,			// shrinking rule
	ANNorthRect			&inner_box)		// inner box if shrinking (returned)
{
	ANNdecomp decomp = SPLIT;			// decomposition

	switch (shrink) {					// check shrinking rule
	case ANN_BD_NONE:					// no shrinking allowed
		decomp = SPLIT;
		break;
	case ANN_BD_SUGGEST:				// author's suggestion
	case ANN_BD_SIMPLE:					// simple shrink
		decomp = trySimpleShrink(
				pa, pidx,				// points and indices
				n, dim,					// number of points and dimension
				bnd_box,				// current bounding box
				inner_box);				// inner box if shrinking (returned)
		break;
	case ANN_BD_CENTROID:				// centroid shrink
		decomp = tryCentroidShrink(
				pa, pidx,				// points and indices
				n, dim,					// number of points and dimension
				bnd_box,				// current bounding box
				splitter,				// splitting procedure
				inner_box);				// inner box if shrinking (returned)
		break;
	default:
		annError("Illegal shrinking rule", ANNabort);
	}
	return decomp;
}

//----------------------------------------------------------------------
//	rbd_tree - recursive procedure to build a bd-tree
//
//		This is analogous to rkd_tree, but for bd-trees.  See the
//		procedure rkd_tree() in kd_split.cpp for more information.
//
//		If the number of points falls below the bucket size, then a
//		leaf node is created for the points.  Otherwise we invoke the
//		procedure selectDecomp() which determines whether we are to
//		split or shrink.  If splitting is chosen, then we essentially
//		do exactly as rkd_tree() would, and invoke the specified
//		splitting procedure to the points.  Otherwise, the selection
//		procedure returns a bounding box, from which we extract the
//		appropriate shrinking bounds, and create a shrinking node.
//		Finally the points are subdivided, and the procedure is
//		invoked recursively on the two subsets to form the children.
//----------------------------------------------------------------------

ANNkd_ptr rbd_tree(				// recursive construction of bd-tree
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	int					bsp,			// bucket space
	ANNorthRect			&bnd_box,		// bounding box for current node
	ANNkd_splitter		splitter,		// splitting routine
	ANNshrinkRule		shrink)			// shrinking rule
{
	ANNdecomp decomp;					// decomposition method

	ANNorthRect inner_box(dim);			// inner box (if shrinking)

	if (n <= bsp) {						// n small, make a leaf node
		if (n == 0)						// empty leaf node
			return KD_TRIVIAL;			// return (canonical) empty leaf
		else							// construct the node and return
			return new ANNkd_leaf(n, pidx); 
	}
	
	decomp = selectDecomp(				// select decomposition method
				pa, pidx,				// points and indices
				n, dim,					// number of points and dimension
				bnd_box,				// current bounding box
				splitter, shrink,		// splitting/shrinking methods
				inner_box);				// inner box if shrinking (returned)
	
	if (decomp == SPLIT) {				// split selected
		int cd;							// cutting dimension
		ANNcoord cv;					// cutting value
		int n_lo;						// number on low side of cut
										// invoke splitting procedure
		(*splitter)(pa, pidx, bnd_box, n, dim, cd, cv, n_lo);

		ANNcoord lv = bnd_box.lo[cd];	// save bounds for cutting dimension
		ANNcoord hv = bnd_box.hi[cd];

		bnd_box.hi[cd] = cv;			// modify bounds for left subtree
		ANNkd_ptr lo = rbd_tree(		// build left subtree
				pa, pidx, n_lo,			// ...from pidx[0..n_lo-1]
				dim, bsp, bnd_box, splitter, shrink);
		bnd_box.hi[cd] = hv;			// restore bounds

		bnd_box.lo[cd] = cv;			// modify bounds for right subtree
		ANNkd_ptr hi = rbd_tree(		// build right subtree
				pa, pidx + n_lo, n-n_lo,// ...from pidx[n_lo..n-1]
				dim, bsp, bnd_box, splitter, shrink);
		bnd_box.lo[cd] = lv;			// restore bounds
										// create the splitting node
		return new ANNkd_split(cd, cv, lv, hv, lo, hi);
	}
	else {								// shrink selected
		int n_in;						// number of points in box
		int n_bnds;						// number of bounding sides

		annBoxSplit(					// split points around inner box
				pa,						// points to split
				pidx,					// point indices
				n,						// number of points
				dim,					// dimension
				inner_box,				// inner box
				n_in);					// number of points inside (returned)

		ANNkd_ptr in = rbd_tree(		// build inner subtree pidx[0..n_in-1]
				pa, pidx, n_in, dim, bsp, inner_box, splitter, shrink);
		ANNkd_ptr out = rbd_tree(		// build outer subtree pidx[n_in..n]
				pa, pidx+n_in, n - n_in, dim, bsp, bnd_box, splitter, shrink);

		ANNorthHSArray bnds = NULL;		// bounds (alloc in Box2Bnds and
										// ...freed in bd_shrink destroyer)

		annBox2Bnds(					// convert inner box to bounds
				inner_box,				// inner box
				bnd_box,				// enclosing box
				dim,					// dimension
				n_bnds,					// number of bounds (returned)
				bnds);					// bounds array (modified)

										// return shrinking node
		return new ANNbd_shrink(n_bnds, bnds, in, out);
	}
} 
