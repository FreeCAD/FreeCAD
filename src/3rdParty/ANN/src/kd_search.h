//----------------------------------------------------------------------
// File:			kd_search.h
// Programmer:		Sunil Arya and David Mount
// Description:		Standard kd-tree search
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

#ifndef ANN_kd_search_H
#define ANN_kd_search_H

#include "kd_tree.h"					// kd-tree declarations
#include "kd_util.h"					// kd-tree utilities
#include "pr_queue_k.h"					// k-element priority queue

#include <ANN/ANNperf.h>				// performance evaluation

//----------------------------------------------------------------------
//	More global variables
//		These are active for the life of each call to annkSearch(). They
//		are set to save the number of variables that need to be passed
//		among the various search procedures.
//----------------------------------------------------------------------

extern int				ANNkdDim;		// dimension of space (static copy)
extern ANNpoint			ANNkdQ;			// query point (static copy)
extern double			ANNkdMaxErr;	// max tolerable squared error
extern ANNpointArray	ANNkdPts;		// the points (static copy)
extern ANNmin_k			*ANNkdPointMK;	// set of k closest points
extern int				ANNptsVisited;	// number of points visited

#endif
