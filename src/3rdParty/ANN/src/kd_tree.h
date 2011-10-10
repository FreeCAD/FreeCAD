//----------------------------------------------------------------------
// File:			kd_tree.h
// Programmer:		Sunil Arya and David Mount
// Description:		Declarations for standard kd-tree routines
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
//	Revision 0.1  03/04/98
//		Initial release
//	Revision 1.1  05/03/05
//		Added fixed radius kNN search
//----------------------------------------------------------------------

#ifndef ANN_kd_tree_H
#define ANN_kd_tree_H

#include <ANN/ANNx.h>					// all ANN includes

using namespace std;					// make std:: available

//----------------------------------------------------------------------
//	Generic kd-tree node
//
//		Nodes in kd-trees are of two types, splitting nodes which contain
//		splitting information (a splitting hyperplane orthogonal to one
//		of the coordinate axes) and leaf nodes which contain point
//		information (an array of points stored in a bucket).  This is
//		handled by making a generic class kd_node, which is essentially an
//		empty shell, and then deriving the leaf and splitting nodes from
//		this.
//----------------------------------------------------------------------

class ANNkd_node{						// generic kd-tree node (empty shell)
public:
	virtual ~ANNkd_node() {}					// virtual distroyer

	virtual void ann_search(ANNdist) = 0;		// tree search
	virtual void ann_pri_search(ANNdist) = 0;	// priority search
	virtual void ann_FR_search(ANNdist) = 0;	// fixed-radius search

	virtual void getStats(						// get tree statistics
				int dim,						// dimension of space
				ANNkdStats &st,					// statistics
				ANNorthRect &bnd_box) = 0;		// bounding box
												// print node
	virtual void print(int level, ostream &out) = 0;
	virtual void dump(ostream &out) = 0;		// dump node

	friend class ANNkd_tree;					// allow kd-tree to access us
};

//----------------------------------------------------------------------
//	kd-splitting function:
//		kd_splitter is a pointer to a splitting routine for preprocessing.
//		Different splitting procedures result in different strategies
//		for building the tree.
//----------------------------------------------------------------------

typedef void (*ANNkd_splitter)(			// splitting routine for kd-trees
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices (permuted on return)
	const ANNorthRect	&bnds,			// bounding rectangle for cell
	int					n,				// number of points
	int					dim,			// dimension of space
	int					&cut_dim,		// cutting dimension (returned)
	ANNcoord			&cut_val,		// cutting value (returned)
	int					&n_lo);			// num of points on low side (returned)

//----------------------------------------------------------------------
//	Leaf kd-tree node
//		Leaf nodes of the kd-tree store the set of points associated
//		with this bucket, stored as an array of point indices.  These
//		are indices in the array points, which resides with the
//		root of the kd-tree.  We also store the number of points
//		that reside in this bucket.
//----------------------------------------------------------------------

class ANNkd_leaf: public ANNkd_node		// leaf node for kd-tree
{
	int					n_pts;			// no. points in bucket
	ANNidxArray			bkt;			// bucket of points
public:
	ANNkd_leaf(							// constructor
		int				n,				// number of points
		ANNidxArray		b)				// bucket
		{
			n_pts		= n;			// number of points in bucket
			bkt			= b;			// the bucket
		}

	~ANNkd_leaf() { }					// destructor (none)

	virtual void getStats(						// get tree statistics
				int dim,						// dimension of space
				ANNkdStats &st,					// statistics
				ANNorthRect &bnd_box);			// bounding box
	virtual void print(int level, ostream &out);// print node
	virtual void dump(ostream &out);			// dump node

	virtual void ann_search(ANNdist);			// standard search
	virtual void ann_pri_search(ANNdist);		// priority search
	virtual void ann_FR_search(ANNdist);		// fixed-radius search
};

//----------------------------------------------------------------------
//		KD_TRIVIAL is a special pointer to an empty leaf node. Since
//		some splitting rules generate many (more than 50%) trivial
//		leaves, we use this one shared node to save space.
//
//		The pointer is initialized to NULL, but whenever a kd-tree is
//		created, we allocate this node, if it has not already been
//		allocated. This node is *never* deallocated, so it produces
//		a small memory leak.
//----------------------------------------------------------------------

extern ANNkd_leaf *KD_TRIVIAL;					// trivial (empty) leaf node

//----------------------------------------------------------------------
//	kd-tree splitting node.
//		Splitting nodes contain a cutting dimension and a cutting value.
//		These indicate the axis-parellel plane which subdivide the
//		box for this node. The extent of the bounding box along the
//		cutting dimension is maintained (this is used to speed up point
//		to box distance calculations) [we do not store the entire bounding
//		box since this may be wasteful of space in high dimensions].
//		We also store pointers to the 2 children.
//----------------------------------------------------------------------

class ANNkd_split : public ANNkd_node	// splitting node of a kd-tree
{
	int					cut_dim;		// dim orthogonal to cutting plane
	ANNcoord			cut_val;		// location of cutting plane
	ANNcoord			cd_bnds[2];		// lower and upper bounds of
										// rectangle along cut_dim
	ANNkd_ptr			child[2];		// left and right children
public:
	ANNkd_split(						// constructor
		int cd,							// cutting dimension
		ANNcoord cv,					// cutting value
		ANNcoord lv, ANNcoord hv,				// low and high values
		ANNkd_ptr lc=NULL, ANNkd_ptr hc=NULL)	// children
		{
			cut_dim		= cd;					// cutting dimension
			cut_val		= cv;					// cutting value
			cd_bnds[ANN_LO] = lv;				// lower bound for rectangle
			cd_bnds[ANN_HI] = hv;				// upper bound for rectangle
			child[ANN_LO]	= lc;				// left child
			child[ANN_HI]	= hc;				// right child
		}

	~ANNkd_split()						// destructor
		{
			if (child[ANN_LO]!= NULL && child[ANN_LO]!= KD_TRIVIAL)
				delete child[ANN_LO];
			if (child[ANN_HI]!= NULL && child[ANN_HI]!= KD_TRIVIAL)
				delete child[ANN_HI];
		}

	virtual void getStats(						// get tree statistics
				int dim,						// dimension of space
				ANNkdStats &st,					// statistics
				ANNorthRect &bnd_box);			// bounding box
	virtual void print(int level, ostream &out);// print node
	virtual void dump(ostream &out);			// dump node

	virtual void ann_search(ANNdist);			// standard search
	virtual void ann_pri_search(ANNdist);		// priority search
	virtual void ann_FR_search(ANNdist);		// fixed-radius search
};

//----------------------------------------------------------------------
//		External entry points
//----------------------------------------------------------------------

ANNkd_ptr rkd_tree(				// recursive construction of kd-tree
	ANNpointArray		pa,				// point array (unaltered)
	ANNidxArray			pidx,			// point indices to store in subtree
	int					n,				// number of points
	int					dim,			// dimension of space
	int					bsp,			// bucket space
	ANNorthRect			&bnd_box,		// bounding box for current node
	ANNkd_splitter		splitter);		// splitting routine

#endif
