//----------------------------------------------------------------------
// File:			bd_pr_search.cpp
// Programmer:		David Mount
// Description:		Priority search for bd-trees
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
//History:
//	Revision 0.1  03/04/98
//		Initial release
//----------------------------------------------------------------------

#include "bd_tree.h"					// bd-tree declarations
#include "kd_pr_search.h"				// kd priority search declarations

//----------------------------------------------------------------------
//	Approximate priority searching for bd-trees.
//		See the file kd_pr_search.cc for general information on the
//		approximate nearest neighbor priority search algorithm.  Here
//		we include the extensions for shrinking nodes.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	bd_shrink::ann_search - search a shrinking node
//----------------------------------------------------------------------

void ANNbd_shrink::ann_pri_search(ANNdist box_dist)
{
	ANNdist inner_dist = 0;						// distance to inner box
	for (int i = 0; i < n_bnds; i++) {			// is query point in the box?
		if (bnds[i].out(ANNprQ)) {				// outside this bounding side?
												// add to inner distance
			inner_dist = (ANNdist) ANN_SUM(inner_dist, bnds[i].dist(ANNprQ));
		}
	}
	if (inner_dist <= box_dist) {				// if inner box is closer
		if (child[ANN_OUT] != KD_TRIVIAL)		// enqueue outer if not trivial
			ANNprBoxPQ->insert(box_dist,child[ANN_OUT]);
												// continue with inner child
		child[ANN_IN]->ann_pri_search(inner_dist);
	}
	else {										// if outer box is closer
		if (child[ANN_IN] != KD_TRIVIAL)		// enqueue inner if not trivial
			ANNprBoxPQ->insert(inner_dist,child[ANN_IN]);
												// continue with outer child
		child[ANN_OUT]->ann_pri_search(box_dist);
	}
	ANN_FLOP(3*n_bnds)							// increment floating ops
	ANN_SHR(1)									// one more shrinking node
}
