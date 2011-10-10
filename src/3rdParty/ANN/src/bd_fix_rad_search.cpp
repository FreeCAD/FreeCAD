//----------------------------------------------------------------------
// File:			bd_fix_rad_search.cpp
// Programmer:		David Mount
// Description:		Standard bd-tree search
// Last modified:	05/03/05 (Version 1.1)
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
//	Revision 1.1  05/03/05
//		Initial release
//----------------------------------------------------------------------

#include "bd_tree.h"					// bd-tree declarations
#include "kd_fix_rad_search.h"			// kd-tree FR search declarations

//----------------------------------------------------------------------
//	Approximate searching for bd-trees.
//		See the file kd_FR_search.cpp for general information on the
//		approximate nearest neighbor search algorithm.  Here we
//		include the extensions for shrinking nodes.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	bd_shrink::ann_FR_search - search a shrinking node
//----------------------------------------------------------------------

void ANNbd_shrink::ann_FR_search(ANNdist box_dist)
{
												// check dist calc term cond.
	if (ANNmaxPtsVisited != 0 && ANNptsVisited > ANNmaxPtsVisited) return;

	ANNdist inner_dist = 0;						// distance to inner box
	for (int i = 0; i < n_bnds; i++) {			// is query point in the box?
		if (bnds[i].out(ANNkdFRQ)) {			// outside this bounding side?
												// add to inner distance
			inner_dist = (ANNdist) ANN_SUM(inner_dist, bnds[i].dist(ANNkdFRQ));
		}
	}
	if (inner_dist <= box_dist) {				// if inner box is closer
		child[ANN_IN]->ann_FR_search(inner_dist);// search inner child first
		child[ANN_OUT]->ann_FR_search(box_dist);// ...then outer child
	}
	else {										// if outer box is closer
		child[ANN_OUT]->ann_FR_search(box_dist);// search outer child first
		child[ANN_IN]->ann_FR_search(inner_dist);// ...then outer child
	}
	ANN_FLOP(3*n_bnds)							// increment floating ops
	ANN_SHR(1)									// one more shrinking node
}
