//----------------------------------------------------------------------
// File:			kd_dump.cc
// Programmer:		David Mount
// Description:		Dump and Load for kd- and bd-trees
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
//		Moved dump out of kd_tree.cc into this file.
//		Added kd-tree load constructor.
//----------------------------------------------------------------------
// This file contains routines for dumping kd-trees and bd-trees and
// reloading them. (It is an abuse of policy to include both kd- and
// bd-tree routines in the same file, sorry.  There should be no problem
// in deleting the bd- versions of the routines if they are not
// desired.)
//----------------------------------------------------------------------

#include "kd_tree.h"					// kd-tree declarations
#include "bd_tree.h"					// bd-tree declarations
#include <cstring>
#include <cstdlib>

using namespace std;					// make std:: available

//----------------------------------------------------------------------
//		Constants
//----------------------------------------------------------------------

const int		STRING_LEN		= 500;	// maximum string length
const double	EPSILON			= 1E-5; // small number for float comparison

enum ANNtreeType {KD_TREE, BD_TREE};	// tree types (used in loading)

//----------------------------------------------------------------------
//		Procedure declarations
//----------------------------------------------------------------------

static ANNkd_ptr annReadDump(			// read dump file
	istream				&in,					// input stream
	ANNtreeType			tree_type,				// type of tree expected
	ANNpointArray		&the_pts,				// new points (if applic)
	ANNidxArray			&the_pidx,				// point indices (returned)
	int					&the_dim,				// dimension (returned)
	int					&the_n_pts,				// number of points (returned)
	int					&the_bkt_size,			// bucket size (returned)
	ANNpoint			&the_bnd_box_lo,		// low bounding point
	ANNpoint			&the_bnd_box_hi);		// high bounding point

static ANNkd_ptr annReadTree(			// read tree-part of dump file
	istream				&in,					// input stream
	ANNtreeType			tree_type,				// type of tree expected
	ANNidxArray			the_pidx,				// point indices (modified)
	int					&next_idx);				// next index (modified)

//----------------------------------------------------------------------
//	ANN kd- and bd-tree Dump Format
//		The dump file begins with a header containing the version of
//		ANN, an optional section containing the points, followed by
//		a description of the tree.	The tree is printed in preorder.
//
//		Format:
//		#ANN <version number> <comments> [END_OF_LINE]
//		points <dim> <n_pts>			(point coordinates: this is optional)
//		0 <xxx> <xxx> ... <xxx>			(point indices and coordinates)
//		1 <xxx> <xxx> ... <xxx>
//		  ...
//		tree <dim> <n_pts> <bkt_size>
//		<xxx> <xxx> ... <xxx>			(lower end of bounding box)
//		<xxx> <xxx> ... <xxx>			(upper end of bounding box)
//				If the tree is null, then a single line "null" is
//				output.	 Otherwise the nodes of the tree are printed
//				one per line in preorder.  Leaves and splitting nodes 
//				have the following formats:
//		Leaf node:
//				leaf <n_pts> <bkt[0]> <bkt[1]> ... <bkt[n-1]>
//		Splitting nodes:
//				split <cut_dim> <cut_val> <lo_bound> <hi_bound>
//
//		For bd-trees:
//
//		Shrinking nodes:
//				shrink <n_bnds>
//						<cut_dim> <cut_val> <side>
//						<cut_dim> <cut_val> <side>
//						... (repeated n_bnds times)
//----------------------------------------------------------------------

void ANNkd_tree::Dump(					// dump entire tree
		ANNbool with_pts,				// print points as well?
		ostream &out)					// output stream
{
	out << "#ANN " << ANNversion << "\n";
	out.precision(ANNcoordPrec);		// use full precision in dumping
	if (with_pts) {						// print point coordinates
		out << "points " << dim << " " << n_pts << "\n";
		for (int i = 0; i < n_pts; i++) {
			out << i << " ";
			annPrintPt(pts[i], dim, out);
			out << "\n";
		}
	}
	out << "tree "						// print tree elements
		<< dim << " "
		<< n_pts << " "
		<< bkt_size << "\n";

	annPrintPt(bnd_box_lo, dim, out);	// print lower bound
	out << "\n";
	annPrintPt(bnd_box_hi, dim, out);	// print upper bound
	out << "\n";

	if (root == NULL)					// empty tree?
		out << "null\n";
	else {
		root->dump(out);				// invoke printing at root
	}
	out.precision(0);					// restore default precision
}

void ANNkd_split::dump(					// dump a splitting node
		ostream &out)					// output stream
{
	out << "split " << cut_dim << " " << cut_val << " ";
	out << cd_bnds[ANN_LO] << " " << cd_bnds[ANN_HI] << "\n";

	child[ANN_LO]->dump(out);			// print low child
	child[ANN_HI]->dump(out);			// print high child
}

void ANNkd_leaf::dump(					// dump a leaf node
		ostream &out)					// output stream
{
	if (this == KD_TRIVIAL) {			// canonical trivial leaf node
		out << "leaf 0\n";				// leaf no points
	}
	else{
		out << "leaf " << n_pts;
		for (int j = 0; j < n_pts; j++) {
			out << " " << bkt[j];
		}
		out << "\n";
	}
}

void ANNbd_shrink::dump(				// dump a shrinking node
		ostream &out)					// output stream
{
	out << "shrink " << n_bnds << "\n";
	for (int j = 0; j < n_bnds; j++) {
		out << bnds[j].cd << " " << bnds[j].cv << " " << bnds[j].sd << "\n";
	}
	child[ANN_IN]->dump(out);			// print in-child
	child[ANN_OUT]->dump(out);			// print out-child
}

//----------------------------------------------------------------------
// Load kd-tree from dump file
//		This rebuilds a kd-tree which was dumped to a file.	 The dump
//		file contains all the basic tree information according to a
//		preorder traversal.	 We assume that the dump file also contains
//		point data.	 (This is to guarantee the consistency of the tree.)
//		If not, then an error is generated.
//
//		Indirectly, this procedure allocates space for points, point
//		indices, all nodes in the tree, and the bounding box for the
//		tree.  When the tree is destroyed, all but the points are
//		deallocated.
//
//		This routine calls annReadDump to do all the work.
//----------------------------------------------------------------------

ANNkd_tree::ANNkd_tree(					// build from dump file
	istream				&in)					// input stream for dump file
{
	int the_dim;								// local dimension
	int the_n_pts;								// local number of points
	int the_bkt_size;							// local number of points
	ANNpoint the_bnd_box_lo;					// low bounding point
	ANNpoint the_bnd_box_hi;					// high bounding point
	ANNpointArray the_pts;						// point storage
	ANNidxArray the_pidx;						// point index storage
	ANNkd_ptr the_root;							// root of the tree

	the_root = annReadDump(						// read the dump file
		in,										// input stream
		KD_TREE,								// expecting a kd-tree
		the_pts,								// point array (returned)
		the_pidx,								// point indices (returned)
		the_dim, the_n_pts, the_bkt_size,		// basic tree info (returned)
		the_bnd_box_lo, the_bnd_box_hi);		// bounding box info (returned)

												// create a skeletal tree
	SkeletonTree(the_n_pts, the_dim, the_bkt_size, the_pts, the_pidx);

	bnd_box_lo = the_bnd_box_lo;
	bnd_box_hi = the_bnd_box_hi;

	root = the_root;							// set the root
}

ANNbd_tree::ANNbd_tree(					// build bd-tree from dump file
	istream				&in) : ANNkd_tree()		// input stream for dump file
{
	int the_dim;								// local dimension
	int the_n_pts;								// local number of points
	int the_bkt_size;							// local number of points
	ANNpoint the_bnd_box_lo;					// low bounding point
	ANNpoint the_bnd_box_hi;					// high bounding point
	ANNpointArray the_pts;						// point storage
	ANNidxArray the_pidx;						// point index storage
	ANNkd_ptr the_root;							// root of the tree

	the_root = annReadDump(						// read the dump file
		in,										// input stream
		BD_TREE,								// expecting a bd-tree
		the_pts,								// point array (returned)
		the_pidx,								// point indices (returned)
		the_dim, the_n_pts, the_bkt_size,		// basic tree info (returned)
		the_bnd_box_lo, the_bnd_box_hi);		// bounding box info (returned)

												// create a skeletal tree
	SkeletonTree(the_n_pts, the_dim, the_bkt_size, the_pts, the_pidx);
	bnd_box_lo = the_bnd_box_lo;
	bnd_box_hi = the_bnd_box_hi;

	root = the_root;							// set the root
}

//----------------------------------------------------------------------
//	annReadDump - read a dump file
//
//		This procedure reads a dump file, constructs a kd-tree
//		and returns all the essential information needed to actually
//		construct the tree.	 Because this procedure is used for
//		constructing both kd-trees and bd-trees, the second argument
//		is used to indicate which type of tree we are expecting.
//----------------------------------------------------------------------

static ANNkd_ptr annReadDump(
	istream				&in,					// input stream
	ANNtreeType			tree_type,				// type of tree expected
	ANNpointArray		&the_pts,				// new points (returned)
	ANNidxArray			&the_pidx,				// point indices (returned)
	int					&the_dim,				// dimension (returned)
	int					&the_n_pts,				// number of points (returned)
	int					&the_bkt_size,			// bucket size (returned)
	ANNpoint			&the_bnd_box_lo,		// low bounding point (ret'd)
	ANNpoint			&the_bnd_box_hi)		// high bounding point (ret'd)
{
	int j;
	char str[STRING_LEN];						// storage for string
	char version[STRING_LEN];					// ANN version number
	ANNkd_ptr the_root = NULL;

	//------------------------------------------------------------------
	//	Input file header
	//------------------------------------------------------------------
	in >> str;									// input header
	if (strcmp(str, "#ANN") != 0) {				// incorrect header
		annError("Incorrect header for dump file", ANNabort);
	}
	in.getline(version, STRING_LEN);			// get version (ignore)

	//------------------------------------------------------------------
	//	Input the points
	//			An array the_pts is allocated and points are read from
	//			the dump file.
	//------------------------------------------------------------------
	in >> str;									// get major heading
	if (strcmp(str, "points") == 0) {			// points section
		in >> the_dim;							// input dimension
		in >> the_n_pts;						// number of points
												// allocate point storage
		the_pts = annAllocPts(the_n_pts, the_dim);
		for (int i = 0; i < the_n_pts; i++) {	// input point coordinates
			ANNidx idx;							// point index
			in >> idx;							// input point index
			if (idx < 0 || idx >= the_n_pts) {
				annError("Point index is out of range", ANNabort);
			}
			for (j = 0; j < the_dim; j++) {
				in >> the_pts[idx][j];			// read point coordinates
			}
		}
		in >> str;								// get next major heading
	}
	else {										// no points were input
		annError("Points must be supplied in the dump file", ANNabort);
	}

	//------------------------------------------------------------------
	//	Input the tree
	//			After the basic header information, we invoke annReadTree
	//			to do all the heavy work.  We create our own array of
	//			point indices (so we can pass them to annReadTree())
	//			but we do not deallocate them.	They will be deallocated
	//			when the tree is destroyed.
	//------------------------------------------------------------------
	if (strcmp(str, "tree") == 0) {				// tree section
		in >> the_dim;							// read dimension
		in >> the_n_pts;						// number of points
		in >> the_bkt_size;						// bucket size
		the_bnd_box_lo = annAllocPt(the_dim);	// allocate bounding box pts
		the_bnd_box_hi = annAllocPt(the_dim);

		for (j = 0; j < the_dim; j++) {			// read bounding box low
			in >> the_bnd_box_lo[j];
		}
		for (j = 0; j < the_dim; j++) {			// read bounding box low
			in >> the_bnd_box_hi[j];
		}
		the_pidx = new ANNidx[the_n_pts];		// allocate point index array
		int next_idx = 0;						// number of indices filled
												// read the tree and indices
		the_root = annReadTree(in, tree_type, the_pidx, next_idx);
		if (next_idx != the_n_pts) {			// didn't see all the points?
			annError("Didn't see as many points as expected", ANNwarn);
		}
	}
	else {
		annError("Illegal dump format.	Expecting section heading", ANNabort);
	}
	return the_root;
}

//----------------------------------------------------------------------
// annReadTree - input tree and return pointer
//
//		annReadTree reads in a node of the tree, makes any recursive
//		calls as needed to input the children of this node (if internal).
//		It returns a pointer to the node that was created.	An array
//		of point indices is given along with a pointer to the next
//		available location in the array.  As leaves are read, their
//		point indices are stored here, and the point buckets point
//		to the first entry in the array.
//
//		Recall that these are the formats.	The tree is given in
//		preorder.
//
//		Leaf node:
//				leaf <n_pts> <bkt[0]> <bkt[1]> ... <bkt[n-1]>
//		Splitting nodes:
//				split <cut_dim> <cut_val> <lo_bound> <hi_bound>
//
//		For bd-trees:
//
//		Shrinking nodes:
//				shrink <n_bnds>
//						<cut_dim> <cut_val> <side>
//						<cut_dim> <cut_val> <side>
//						... (repeated n_bnds times)
//----------------------------------------------------------------------

static ANNkd_ptr annReadTree(
	istream				&in,					// input stream
	ANNtreeType			tree_type,				// type of tree expected
	ANNidxArray			the_pidx,				// point indices (modified)
	int					&next_idx)				// next index (modified)
{
	char tag[STRING_LEN];						// tag (leaf, split, shrink)
	int n_pts;									// number of points in leaf
	int cd;										// cut dimension
	ANNcoord cv;								// cut value
	ANNcoord lb;								// low bound
	ANNcoord hb;								// high bound
	int n_bnds;									// number of bounding sides
	int sd;										// which side

	in >> tag;									// input node tag

	if (strcmp(tag, "null") == 0) {				// null tree
		return NULL;
	}
	//------------------------------------------------------------------
	//	Read a leaf
	//------------------------------------------------------------------
	if (strcmp(tag, "leaf") == 0) {				// leaf node

		in >> n_pts;							// input number of points
		int old_idx = next_idx;					// save next_idx
		if (n_pts == 0) {						// trivial leaf
			return KD_TRIVIAL;
		}
		else {
			for (int i = 0; i < n_pts; i++) {	// input point indices
				in >> the_pidx[next_idx++];		// store in array of indices
			}
		}
		return new ANNkd_leaf(n_pts, &the_pidx[old_idx]);
	}
	//------------------------------------------------------------------
	//	Read a splitting node
	//------------------------------------------------------------------
	else if (strcmp(tag, "split") == 0) {		// splitting node

		in >> cd >> cv >> lb >> hb;

												// read low and high subtrees
		ANNkd_ptr lc = annReadTree(in, tree_type, the_pidx, next_idx);
		ANNkd_ptr hc = annReadTree(in, tree_type, the_pidx, next_idx);
												// create new node and return
		return new ANNkd_split(cd, cv, lb, hb, lc, hc);
	}
	//------------------------------------------------------------------
	//	Read a shrinking node (bd-tree only)
	//------------------------------------------------------------------
	else if (strcmp(tag, "shrink") == 0) {		// shrinking node
		if (tree_type != BD_TREE) {
			annError("Shrinking node not allowed in kd-tree", ANNabort);
		}

		in >> n_bnds;							// number of bounding sides
												// allocate bounds array
		ANNorthHSArray bds = new ANNorthHalfSpace[n_bnds];
		for (int i = 0; i < n_bnds; i++) {
			in >> cd >> cv >> sd;				// input bounding halfspace
												// copy to array
			bds[i] = ANNorthHalfSpace(cd, cv, sd);
		}
												// read inner and outer subtrees
		ANNkd_ptr ic = annReadTree(in, tree_type, the_pidx, next_idx);
		ANNkd_ptr oc = annReadTree(in, tree_type, the_pidx, next_idx);
												// create new node and return
		return new ANNbd_shrink(n_bnds, bds, ic, oc);
	}
	else {
		annError("Illegal node type in dump file", ANNabort);
		exit(0);								// to keep the compiler happy
	}
}
