//----------------------------------------------------------------------
// File:			kd_util.cpp
// Programmer:		Sunil Arya and David Mount
// Description:		Common utilities for kd-trees
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
//----------------------------------------------------------------------

#include "kd_util.h"					// kd-utility declarations

#include <ANN/ANNperf.h>				// performance evaluation

//----------------------------------------------------------------------
// The following routines are utility functions for manipulating
// points sets, used in determining splitting planes for kd-tree
// construction.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	NOTE: Virtually all point indexing is done through an index (i.e.
//	permutation) array pidx.  Consequently, a reference to the d-th
//	coordinate of the i-th point is pa[pidx[i]][d].  The macro PA(i,d)
//	is a shorthand for this.
//----------------------------------------------------------------------
										// standard 2-d indirect indexing
#define PA(i,d)			(pa[pidx[(i)]][(d)])
										// accessing a single point
#define PP(i)			(pa[pidx[(i)]])

//----------------------------------------------------------------------
//	annAspectRatio
//		Compute the aspect ratio (ratio of longest to shortest side)
//		of a rectangle.
//----------------------------------------------------------------------

double annAspectRatio(
	int					dim,			// dimension
	const ANNorthRect	&bnd_box)		// bounding cube
{
	ANNcoord length = bnd_box.hi[0] - bnd_box.lo[0];
	ANNcoord min_length = length;				// min side length
	ANNcoord max_length = length;				// max side length
	for (int d = 0; d < dim; d++) {
		length = bnd_box.hi[d] - bnd_box.lo[d];
		if (length < min_length) min_length = length;
		if (length > max_length) max_length = length;
	}
	return max_length/min_length;
}

//----------------------------------------------------------------------
//	annEnclRect, annEnclCube
//		These utilities compute the smallest rectangle and cube enclosing
//		a set of points, respectively.
//----------------------------------------------------------------------

void annEnclRect(
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension
	ANNorthRect			&bnds)			// bounding cube (returned)
{
	for (int d = 0; d < dim; d++) {		// find smallest enclosing rectangle
		ANNcoord lo_bnd = PA(0,d);		// lower bound on dimension d
		ANNcoord hi_bnd = PA(0,d);		// upper bound on dimension d
		for (int i = 0; i < n; i++) {
			if (PA(i,d) < lo_bnd) lo_bnd = PA(i,d);
			else if (PA(i,d) > hi_bnd) hi_bnd = PA(i,d);
		}
		bnds.lo[d] = lo_bnd;
		bnds.hi[d] = hi_bnd;
	}
}

void annEnclCube(						// compute smallest enclosing cube
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension
	ANNorthRect			&bnds)			// bounding cube (returned)
{
	int d;
										// compute smallest enclosing rect
	annEnclRect(pa, pidx, n, dim, bnds);

	ANNcoord max_len = 0;				// max length of any side
	for (d = 0; d < dim; d++) {			// determine max side length
		ANNcoord len = bnds.hi[d] - bnds.lo[d];
		if (len > max_len) {			// update max_len if longest
			max_len = len;
		}
	}
	for (d = 0; d < dim; d++) {			// grow sides to match max
		ANNcoord len = bnds.hi[d] - bnds.lo[d];
		ANNcoord half_diff = (max_len - len) / 2;
		bnds.lo[d] -= half_diff;
		bnds.hi[d] += half_diff;
	}
}

//----------------------------------------------------------------------
//	annBoxDistance - utility routine which computes distance from point to
//		box (Note: most distances to boxes are computed using incremental
//		distance updates, not this function.)
//----------------------------------------------------------------------

ANNdist annBoxDistance(			// compute distance from point to box
	const ANNpoint		q,				// the point
	const ANNpoint		lo,				// low point of box
	const ANNpoint		hi,				// high point of box
	int					dim)			// dimension of space
{
	register ANNdist dist = 0.0;		// sum of squared distances
	register ANNdist t;

	for (register int d = 0; d < dim; d++) {
		if (q[d] < lo[d]) {				// q is left of box
			t = ANNdist(lo[d]) - ANNdist(q[d]);
			dist = ANN_SUM(dist, ANN_POW(t));
		}
		else if (q[d] > hi[d]) {		// q is right of box
			t = ANNdist(q[d]) - ANNdist(hi[d]);
			dist = ANN_SUM(dist, ANN_POW(t));
		}
	}
	ANN_FLOP(4*dim)						// increment floating op count

	return dist;
}

//----------------------------------------------------------------------
//	annSpread - find spread along given dimension
//	annMinMax - find min and max coordinates along given dimension
//	annMaxSpread - find dimension of max spread
//----------------------------------------------------------------------

ANNcoord annSpread(				// compute point spread along dimension
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d)				// dimension to check
{
	ANNcoord min = PA(0,d);				// compute max and min coords
	ANNcoord max = PA(0,d);
	for (int i = 1; i < n; i++) {
		ANNcoord c = PA(i,d);
		if (c < min) min = c;
		else if (c > max) max = c;
	}
	return (max - min);					// total spread is difference
}

void annMinMax(					// compute min and max coordinates along dim
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension to check
	ANNcoord			&min,			// minimum value (returned)
	ANNcoord			&max)			// maximum value (returned)
{
	min = PA(0,d);						// compute max and min coords
	max = PA(0,d);
	for (int i = 1; i < n; i++) {
		ANNcoord c = PA(i,d);
		if (c < min) min = c;
		else if (c > max) max = c;
	}
}

int annMaxSpread(						// compute dimension of max spread
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim)			// dimension of space
{
	int max_dim = 0;					// dimension of max spread
	ANNcoord max_spr = 0;				// amount of max spread

	if (n == 0) return max_dim;			// no points, who cares?

	for (int d = 0; d < dim; d++) {		// compute spread along each dim
		ANNcoord spr = annSpread(pa, pidx, n, d);
		if (spr > max_spr) {			// bigger than current max
			max_spr = spr;
			max_dim = d;
		}
	}
	return max_dim;
}

//----------------------------------------------------------------------
//	annMedianSplit - split point array about its median
//		Splits a subarray of points pa[0..n] about an element of given
//		rank (median: n_lo = n/2) with respect to dimension d.  It places
//		the element of rank n_lo-1 correctly (because our splitting rule
//		takes the mean of these two).  On exit, the array is permuted so
//		that:
//
//		pa[0..n_lo-2][d] <= pa[n_lo-1][d] <= pa[n_lo][d] <= pa[n_lo+1..n-1][d].
//
//		The mean of pa[n_lo-1][d] and pa[n_lo][d] is returned as the
//		splitting value.
//
//		All indexing is done indirectly through the index array pidx.
//
//		This function uses the well known selection algorithm due to
//		C.A.R. Hoare.
//----------------------------------------------------------------------

										// swap two points in pa array
#define PASWAP(a,b) { int tmp = pidx[a]; pidx[a] = pidx[b]; pidx[b] = tmp; }

void annMedianSplit(
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			&cv,			// cutting value
	int					n_lo)			// split into n_lo and n-n_lo
{
	int l = 0;							// left end of current subarray
	int r = n-1;						// right end of current subarray
	while (l < r) {
		register int i = (r+l)/2;		// select middle as pivot
		register int k;

		if (PA(i,d) > PA(r,d))			// make sure last > pivot
			PASWAP(i,r)
		PASWAP(l,i);					// move pivot to first position

		ANNcoord c = PA(l,d);			// pivot value
		i = l;
		k = r;
		for(;;) {						// pivot about c
			while (PA(++i,d) < c) ;
			while (PA(--k,d) > c) ;
			if (i < k) PASWAP(i,k) else break;
		}
		PASWAP(l,k);					// pivot winds up in location k

		if (k > n_lo)	   r = k-1;		// recurse on proper subarray
		else if (k < n_lo) l = k+1;
		else break;						// got the median exactly
	}
	if (n_lo > 0) {						// search for next smaller item
		ANNcoord c = PA(0,d);			// candidate for max
		int k = 0;						// candidate's index
		for (int i = 1; i < n_lo; i++) {
			if (PA(i,d) > c) {
				c = PA(i,d);
				k = i;
			}
		}
		PASWAP(n_lo-1, k);				// max among pa[0..n_lo-1] to pa[n_lo-1]
	}
										// cut value is midpoint value
	cv = (PA(n_lo-1,d) + PA(n_lo,d))/2.0;
}

//----------------------------------------------------------------------
//	annPlaneSplit - split point array about a cutting plane
//		Split the points in an array about a given plane along a
//		given cutting dimension.  On exit, br1 and br2 are set so
//		that:
//		
//				pa[ 0 ..br1-1] <  cv
//				pa[br1..br2-1] == cv
//				pa[br2.. n -1] >  cv
//
//		All indexing is done indirectly through the index array pidx.
//
//----------------------------------------------------------------------

void annPlaneSplit(				// split points by a plane
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			cv,				// cutting value
	int					&br1,			// first break (values < cv)
	int					&br2)			// second break (values == cv)
{
	int l = 0;
	int r = n-1;
	for(;;) {							// partition pa[0..n-1] about cv
		while (l < n && PA(l,d) < cv) l++;
		while (r >= 0 && PA(r,d) >= cv) r--;
		if (l > r) break;
		PASWAP(l,r);
		l++; r--;
	}
	br1 = l;					// now: pa[0..br1-1] < cv <= pa[br1..n-1]
	r = n-1;
	for(;;) {							// partition pa[br1..n-1] about cv
		while (l < n && PA(l,d) <= cv) l++;
		while (r >= br1 && PA(r,d) > cv) r--;
		if (l > r) break;
		PASWAP(l,r);
		l++; r--;
	}
	br2 = l;					// now: pa[br1..br2-1] == cv < pa[br2..n-1]
}


//----------------------------------------------------------------------
//	annBoxSplit - split point array about a orthogonal rectangle
//		Split the points in an array about a given orthogonal
//		rectangle.  On exit, n_in is set to the number of points
//		that are inside (or on the boundary of) the rectangle.
//
//		All indexing is done indirectly through the index array pidx.
//
//----------------------------------------------------------------------

void annBoxSplit(				// split points by a box
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension of space
	ANNorthRect			&box,			// the box
	int					&n_in)			// number of points inside (returned)
{
	int l = 0;
	int r = n-1;
	for(;;) {							// partition pa[0..n-1] about box
		while (l < n && box.inside(dim, PP(l))) l++;
		while (r >= 0 && !box.inside(dim, PP(r))) r--;
		if (l > r) break;
		PASWAP(l,r);
		l++; r--;
	}
	n_in = l;					// now: pa[0..n_in-1] inside and rest outside
}

//----------------------------------------------------------------------
//	annSplitBalance - compute balance factor for a given plane split
//		Balance factor is defined as the number of points lying
//		below the splitting value minus n/2 (median).  Thus, a
//		median split has balance 0, left of this is negative and
//		right of this is positive.  (The points are unchanged.)
//----------------------------------------------------------------------

int annSplitBalance(			// determine balance factor of a split
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			cv)				// cutting value
{
	int n_lo = 0;
	for(int i = 0; i < n; i++) {		// count number less than cv
		if (PA(i,d) < cv) n_lo++;
	}
	return n_lo - n/2;
}

//----------------------------------------------------------------------
//	annBox2Bnds - convert bounding box to list of bounds
//		Given two boxes, an inner box enclosed within a bounding
//		box, this routine determines all the sides for which the
//		inner box is strictly contained with the bounding box,
//		and adds an appropriate entry to a list of bounds.  Then
//		we allocate storage for the final list of bounds, and return
//		the resulting list and its size.
//----------------------------------------------------------------------

void annBox2Bnds(						// convert inner box to bounds
	const ANNorthRect	&inner_box,		// inner box
	const ANNorthRect	&bnd_box,		// enclosing box
	int					dim,			// dimension of space
	int					&n_bnds,		// number of bounds (returned)
	ANNorthHSArray		&bnds)			// bounds array (returned)
{
	int i;
	n_bnds = 0;									// count number of bounds
	for (i = 0; i < dim; i++) {
		if (inner_box.lo[i] > bnd_box.lo[i])	// low bound is inside
				n_bnds++;
		if (inner_box.hi[i] < bnd_box.hi[i])	// high bound is inside
				n_bnds++;
	}

	bnds = new ANNorthHalfSpace[n_bnds];		// allocate appropriate size

	int j = 0;
	for (i = 0; i < dim; i++) {					// fill the array
		if (inner_box.lo[i] > bnd_box.lo[i]) {
				bnds[j].cd = i;
				bnds[j].cv = inner_box.lo[i];
				bnds[j].sd = +1;
				j++;
		}
		if (inner_box.hi[i] < bnd_box.hi[i]) {
				bnds[j].cd = i;
				bnds[j].cv = inner_box.hi[i];
				bnds[j].sd = -1;
				j++;
		}
	}
}

//----------------------------------------------------------------------
//	annBnds2Box - convert list of bounds to bounding box
//		Given an enclosing box and a list of bounds, this routine
//		computes the corresponding inner box.  It is assumed that
//		the box points have been allocated already.
//----------------------------------------------------------------------

void annBnds2Box(
	const ANNorthRect	&bnd_box,		// enclosing box
	int					dim,			// dimension of space
	int					n_bnds,			// number of bounds
	ANNorthHSArray		bnds,			// bounds array
	ANNorthRect			&inner_box)		// inner box (returned)
{
	annAssignRect(dim, inner_box, bnd_box);		// copy bounding box to inner

	for (int i = 0; i < n_bnds; i++) {
		bnds[i].project(inner_box.lo);			// project each endpoint
		bnds[i].project(inner_box.hi);
	}
}
