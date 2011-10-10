//----------------------------------------------------------------------
// File:			kd_split.h
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
//----------------------------------------------------------------------

#ifndef ANN_KD_SPLIT_H
#define ANN_KD_SPLIT_H

#include "kd_tree.h"					// kd-tree definitions

//----------------------------------------------------------------------
//	External entry points
//		These are all splitting procedures for kd-trees.
//----------------------------------------------------------------------

void kd_split(							// standard (optimized) kd-splitter
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

void midpt_split(						// midpoint kd-splitter
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

void sl_midpt_split(					// sliding midpoint kd-splitter
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

void fair_split(						// fair-split kd-splitter
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

void sl_fair_split(						// sliding fair-split kd-splitter
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

#endif
