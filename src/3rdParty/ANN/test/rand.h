//----------------------------------------------------------------------
//	File:			rand.h
//	Programmer:		Sunil Arya and David Mount
//	Description:	Basic include file for random point generators
//	Last modified:	08/04/06 (Version 1.1.1)
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
//		Added annClusOrthFlats distribution
//		Changed procedure names to avoid namespace conflicts
//		Added annClusFlats distribution
//	Revision 1.1.1  08/04/06
//		Added planted distribution
//----------------------------------------------------------------------

#ifndef rand_H
#define rand_H

//----------------------------------------------------------------------
//	Basic includes
//----------------------------------------------------------------------
#include <cstdlib>						// standard includes (rand/random)
#include <cmath>						// math routines
#include <ANN/ANN.h>					// basic ANN includes

//----------------------------------------------------------------------
//	Although random/srandom is a more reliable random number generator,
//	many systems do not have it.  If it is not available, set the
//	preprocessor symbol ANN_NO_RANDOM, and this will substitute the use
//	of rand/srand for them.
//----------------------------------------------------------------------
#ifdef ANN_NO_RANDOM					// for systems not having random()
  #define ANN_RAND		rand
  #define ANN_SRAND		srand
  #define ANN_RAND_MAX	RAND_MAX
#else									// otherwise use rand()
  #define ANN_RAND		random
  #define ANN_SRAND		srandom
  #define ANN_RAND_MAX	2147483647UL		// 2**{31} - 1
  // #define ANN_RAND_MAX	1073741824UL		// 2**{30}
#endif

//----------------------------------------------------------------------
//	Globals
//----------------------------------------------------------------------
extern	int		annIdum;				// random number seed

//----------------------------------------------------------------------
//	External entry points
//----------------------------------------------------------------------

void annUniformPts(				// uniform distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim);		// dimension

void annGaussPts(				// Gaussian distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	double			std_dev);	// standard deviation

void annCoGaussPts(				// correlated-Gaussian distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	double			correlation);	// correlation

void annLaplacePts(				// Laplacian distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim);		// dimension

void annCoLaplacePts(			// correlated-Laplacian distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	double			correlation);	// correlation

void annClusGaussPts(			// clustered-Gaussian distribution
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	int				n_clus,		// number of colors (clusters)
	ANNbool			new_clust,	// generate new cluster centers
	double			std_dev);	// standard deviation within clusters

void annClusOrthFlats(          // clustered along orthogonal flats
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	int				n_clus,		// number of colors
	ANNbool			new_clust,	// generate new clusters.
	double			std_dev,	// standard deviation within clusters
	int				max_dim);	// maximum dimension of the flats

void annClusEllipsoids(			// clustered around ellipsoids
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	int				n_clus,		// number of colors
	ANNbool			new_clust,	// generate new clusters.
	double			std_dev_small,	// small standard deviation
	double			std_dev_lo,	// low standard deviation for ellipses
	double			std_dev_hi,	// high standard deviation for ellipses
	int				max_dim);	// maximum dimension of the flats

void annPlanted(				// planted nearest neighbors
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	ANNpointArray	src,		// source point array
	int				n_src,		// source size
	double			std_dev);	// standard deviation about source

#endif
