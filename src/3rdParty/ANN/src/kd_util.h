//----------------------------------------------------------------------
// File:			kd_util.h
// Programmer:		Sunil Arya and David Mount
// Description:		Common utilities for kd- trees
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

#ifndef ANN_kd_util_H
#define ANN_kd_util_H

#include "kd_tree.h"					// kd-tree declarations

//----------------------------------------------------------------------
//	externally accessible functions
//----------------------------------------------------------------------

double annAspectRatio(			// compute aspect ratio of box
	int					dim,			// dimension
	const ANNorthRect	&bnd_box);		// bounding cube

void annEnclRect(				// compute smallest enclosing rectangle
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension
	ANNorthRect &bnds);					// bounding cube (returned)

void annEnclCube(				// compute smallest enclosing cube
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension
	ANNorthRect &bnds);					// bounding cube (returned)

ANNdist annBoxDistance(			// compute distance from point to box
	const ANNpoint		q,				// the point
	const ANNpoint		lo,				// low point of box
	const ANNpoint		hi,				// high point of box
	int					dim);			// dimension of space

ANNcoord annSpread(				// compute point spread along dimension
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d);				// dimension to check

void annMinMax(					// compute min and max coordinates along dim
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension to check
	ANNcoord&			min,			// minimum value (returned)
	ANNcoord&			max);			// maximum value (returned)

int annMaxSpread(				// compute dimension of max spread
	ANNpointArray		pa,				// point array
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim);			// dimension of space

void annMedianSplit(			// split points along median value
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			&cv,			// cutting value
	int					n_lo);			// split into n_lo and n-n_lo

void annPlaneSplit(				// split points by a plane
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			cv,				// cutting value
	int					&br1,			// first break (values < cv)
	int					&br2);			// second break (values == cv)

void annBoxSplit(				// split points by a box
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					dim,			// dimension of space
	ANNorthRect			&box,			// the box
	int					&n_in);			// number of points inside (returned)

int annSplitBalance(			// determine balance factor of a split
	ANNpointArray		pa,				// points to split
	ANNidxArray			pidx,			// point indices
	int					n,				// number of points
	int					d,				// dimension along which to split
	ANNcoord			cv);			// cutting value

void annBox2Bnds(				// convert inner box to bounds
	const ANNorthRect	&inner_box,		// inner box
	const ANNorthRect	&bnd_box,		// enclosing box
	int					dim,			// dimension of space
	int					&n_bnds,		// number of bounds (returned)
	ANNorthHSArray		&bnds);			// bounds array (returned)

void annBnds2Box(				// convert bounds to inner box
	const ANNorthRect	&bnd_box,		// enclosing box
	int					dim,			// dimension of space
	int					n_bnds,			// number of bounds
	ANNorthHSArray		bnds,			// bounds array
	ANNorthRect			&inner_box);	// inner box (returned)

#endif
