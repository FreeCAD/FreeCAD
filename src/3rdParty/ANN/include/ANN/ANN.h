//----------------------------------------------------------------------
// File:			ANN.h
// Programmer:		Sunil Arya and David Mount
// Last modified:	05/03/05 (Release 1.1)
// Description:		Basic include file for approximate nearest
//					neighbor searching.
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
//		Added copyright and revision information
//		Added ANNcoordPrec for coordinate precision.
//		Added methods theDim, nPoints, maxPoints, thePoints to ANNpointSet.
//		Cleaned up C++ structure for modern compilers
//	Revision 1.1  05/03/05
//		Added fixed-radius k-NN searching
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// ANN - approximate nearest neighbor searching
//	ANN is a library for approximate nearest neighbor searching,
//	based on the use of standard and priority search in kd-trees
//	and balanced box-decomposition (bbd) trees. Here are some
//	references to the main algorithmic techniques used here:
//
//		kd-trees:
//			Friedman, Bentley, and Finkel, ``An algorithm for finding
//				best matches in logarithmic expected time,'' ACM
//				Transactions on Mathematical Software, 3(3):209-226, 1977.
//
//		Priority search in kd-trees:
//			Arya and Mount, ``Algorithms for fast vector quantization,''
//				Proc. of DCC '93: Data Compression Conference, eds. J. A.
//				Storer and M. Cohn, IEEE Press, 1993, 381-390.
//
//		Approximate nearest neighbor search and bbd-trees:
//			Arya, Mount, Netanyahu, Silverman, and Wu, ``An optimal
//				algorithm for approximate nearest neighbor searching,''
//				5th Ann. ACM-SIAM Symposium on Discrete Algorithms,
//				1994, 573-582.
//----------------------------------------------------------------------

#ifndef ANN_H
#define ANN_H

#ifdef WIN32
  //----------------------------------------------------------------------
  // For Microsoft Visual C++, externally accessible symbols must be
  // explicitly indicated with DLL_API, which is somewhat like "extern."
  //
  // The following ifdef block is the standard way of creating macros
  // which make exporting from a DLL simpler. All files within this DLL
  // are compiled with the DLL_EXPORTS preprocessor symbol defined on the
  // command line. In contrast, projects that use (or import) the DLL
  // objects do not define the DLL_EXPORTS symbol. This way any other
  // project whose source files include this file see DLL_API functions as
  // being imported from a DLL, wheras this DLL sees symbols defined with
  // this macro as being exported.
  //----------------------------------------------------------------------
  #ifdef DLL_EXPORTS
	 #define DLL_API __declspec(dllexport)
  #else
	#define DLL_API __declspec(dllimport)
  #endif
  //----------------------------------------------------------------------
  // DLL_API is ignored for all other systems
  //----------------------------------------------------------------------
#else
  #define DLL_API
#endif

//----------------------------------------------------------------------
//  basic includes
//----------------------------------------------------------------------

#include <cmath>			// math includes
#include <iostream>			// I/O streams

//----------------------------------------------------------------------
// Limits
// There are a number of places where we use the maximum double value as
// default initializers (and others may be used, depending on the
// data/distance representation). These can usually be found in limits.h
// (as LONG_MAX, INT_MAX) or in float.h (as DBL_MAX, FLT_MAX).
//
// Not all systems have these files.  If you are using such a system,
// you should set the preprocessor symbol ANN_NO_LIMITS_H when
// compiling, and modify the statements below to generate the
// appropriate value. For practical purposes, this does not need to be
// the maximum double value. It is sufficient that it be at least as
// large than the maximum squared distance between between any two
// points.
//----------------------------------------------------------------------
#ifdef ANN_NO_LIMITS_H					// limits.h unavailable
  #include <cvalues>					// replacement for limits.h
  const double ANN_DBL_MAX = MAXDOUBLE;	// insert maximum double
#else
  #include <climits>
  #include <cfloat>
  const double ANN_DBL_MAX = DBL_MAX;
#endif

#define ANNversion 		"1.1.1"			// ANN version and information
#define ANNversionCmt	""
#define ANNcopyright	"David M. Mount and Sunil Arya"
#define ANNlatestRev	"Aug 4, 2006"

//----------------------------------------------------------------------
//	ANNbool
//	This is a simple boolean type. Although ANSI C++ is supposed
//	to support the type bool, some compilers do not have it.
//----------------------------------------------------------------------

enum ANNbool {ANNfalse = 0, ANNtrue = 1}; // ANN boolean type (non ANSI C++)

//----------------------------------------------------------------------
//	ANNcoord, ANNdist
//		ANNcoord and ANNdist are the types used for representing
//		point coordinates and distances.  They can be modified by the
//		user, with some care.  It is assumed that they are both numeric
//		types, and that ANNdist is generally of an equal or higher type
//		from ANNcoord.	A variable of type ANNdist should be large
//		enough to store the sum of squared components of a variable
//		of type ANNcoord for the number of dimensions needed in the
//		application.  For example, the following combinations are
//		legal:
//
//		ANNcoord		ANNdist
//		---------		-------------------------------
//		short			short, int, long, float, double
//		int				int, long, float, double
//		long			long, float, double
//		float			float, double
//		double			double
//
//		It is the user's responsibility to make sure that overflow does
//		not occur in distance calculation.
//----------------------------------------------------------------------

typedef double	ANNcoord;				// coordinate data type
typedef double	ANNdist;				// distance data type

//----------------------------------------------------------------------
//	ANNidx
//		ANNidx is a point index.  When the data structure is built, the
//		points are given as an array.  Nearest neighbor results are
//		returned as an integer index into this array.  To make it
//		clearer when this is happening, we define the integer type
//		ANNidx.	 Indexing starts from 0.
//		
//		For fixed-radius near neighbor searching, it is possible that
//		there are not k nearest neighbors within the search radius.  To
//		indicate this, the algorithm returns ANN_NULL_IDX as its result.
//		It should be distinguishable from any valid array index.
//----------------------------------------------------------------------

typedef int		ANNidx;					// point index
const ANNidx	ANN_NULL_IDX = -1;		// a NULL point index

//----------------------------------------------------------------------
//	Infinite distance:
//		The code assumes that there is an "infinite distance" which it
//		uses to initialize distances before performing nearest neighbor
//		searches.  It should be as larger or larger than any legitimate
//		nearest neighbor distance.
//
//		On most systems, these should be found in the standard include
//		file <limits.h> or possibly <float.h>.  If you do not have these
//		file, some suggested values are listed below, assuming 64-bit
//		long, 32-bit int and 16-bit short.
//
//		ANNdist ANN_DIST_INF	Values (see <limits.h> or <float.h>)
//		------- ------------	------------------------------------
//		double	DBL_MAX			1.79769313486231570e+308
//		float	FLT_MAX			3.40282346638528860e+38
//		long	LONG_MAX		0x7fffffffffffffff
//		int		INT_MAX			0x7fffffff
//		short	SHRT_MAX		0x7fff
//----------------------------------------------------------------------

const ANNdist	ANN_DIST_INF = ANN_DBL_MAX;

//----------------------------------------------------------------------
//	Significant digits for tree dumps:
//		When floating point coordinates are used, the routine that dumps
//		a tree needs to know roughly how many significant digits there
//		are in a ANNcoord, so it can output points to full precision.
//		This is defined to be ANNcoordPrec.  On most systems these
//		values can be found in the standard include files <limits.h> or
//		<float.h>.  For integer types, the value is essentially ignored.
//
//		ANNcoord ANNcoordPrec	Values (see <limits.h> or <float.h>)
//		-------- ------------	------------------------------------
//		double	 DBL_DIG		15
//		float	 FLT_DIG		6
//		long	 doesn't matter 19
//		int		 doesn't matter 10
//		short	 doesn't matter 5
//----------------------------------------------------------------------

#ifdef DBL_DIG							// number of sig. bits in ANNcoord
	const int	 ANNcoordPrec	= DBL_DIG;
#else
	const int	 ANNcoordPrec	= 15;	// default precision
#endif

//----------------------------------------------------------------------
// Self match?
//	In some applications, the nearest neighbor of a point is not
//	allowed to be the point itself. This occurs, for example, when
//	computing all nearest neighbors in a set.  By setting the
//	parameter ANN_ALLOW_SELF_MATCH to ANNfalse, the nearest neighbor
//	is the closest point whose distance from the query point is
//	strictly positive.
//----------------------------------------------------------------------

const ANNbool	ANN_ALLOW_SELF_MATCH	= ANNtrue;

//----------------------------------------------------------------------
//	Norms and metrics:
//		ANN supports any Minkowski norm for defining distance.  In
//		particular, for any p >= 1, the L_p Minkowski norm defines the
//		length of a d-vector (v0, v1, ..., v(d-1)) to be
//
//				(|v0|^p + |v1|^p + ... + |v(d-1)|^p)^(1/p),
//
//		(where ^ denotes exponentiation, and |.| denotes absolute
//		value).  The distance between two points is defined to be the
//		norm of the vector joining them.  Some common distance metrics
//		include
//
//				Euclidean metric		p = 2
//				Manhattan metric		p = 1
//				Max metric				p = infinity
//
//		In the case of the max metric, the norm is computed by taking
//		the maxima of the absolute values of the components.  ANN is
//		highly "coordinate-based" and does not support general distances
//		functions (e.g. those obeying just the triangle inequality).  It
//		also does not support distance functions based on
//		inner-products.
//
//		For the purpose of computing nearest neighbors, it is not
//		necessary to compute the final power (1/p).  Thus the only
//		component that is used by the program is |v(i)|^p.
//
//		ANN parameterizes the distance computation through the following
//		macros.  (Macros are used rather than procedures for
//		efficiency.) Recall that the distance between two points is
//		given by the length of the vector joining them, and the length
//		or norm of a vector v is given by formula:
//
//				|v| = ROOT(POW(v0) # POW(v1) # ... # POW(v(d-1)))
//
//		where ROOT, POW are unary functions and # is an associative and
//		commutative binary operator mapping the following types:
//
//			**	POW:	ANNcoord				--> ANNdist
//			**	#:		ANNdist x ANNdist		--> ANNdist
//			**	ROOT:	ANNdist (>0)			--> double
//
//		For early termination in distance calculation (partial distance
//		calculation) we assume that POW and # together are monotonically
//		increasing on sequences of arguments, meaning that for all
//		v0..vk and y:
//
//		POW(v0) #...# POW(vk) <= (POW(v0) #...# POW(vk)) # POW(y).
//
//	Incremental Distance Calculation:
//		The program uses an optimized method of computing distances for
//		kd-trees and bd-trees, called incremental distance calculation.
//		It is used when distances are to be updated when only a single
//		coordinate of a point has been changed.  In order to use this,
//		we assume that there is an incremental update function DIFF(x,y)
//		for #, such that if:
//
//					s = x0 # ... # xi # ... # xk 
//
//		then if s' is equal to s but with xi replaced by y, that is, 
//		
//					s' = x0 # ... # y # ... # xk
//
//		then the length of s' can be computed by:
//
//					|s'| = |s| # DIFF(xi,y).
//
//		Thus, if # is + then DIFF(xi,y) is (yi-x).  For the L_infinity
//		norm we make use of the fact that in the program this function
//		is only invoked when y > xi, and hence DIFF(xi,y)=y.
//
//		Finally, for approximate nearest neighbor queries we assume
//		that POW and ROOT are related such that
//
//					v*ROOT(x) = ROOT(POW(v)*x)
//
//		Here are the values for the various Minkowski norms:
//
//		L_p:	p even:							p odd:
//				-------------------------		------------------------
//				POW(v)			= v^p			POW(v)			= |v|^p
//				ROOT(x)			= x^(1/p)		ROOT(x)			= x^(1/p)
//				#				= +				#				= +
//				DIFF(x,y)		= y - x			DIFF(x,y)		= y - x 
//
//		L_inf:
//				POW(v)			= |v|
//				ROOT(x)			= x
//				#				= max
//				DIFF(x,y)		= y
//
//		By default the Euclidean norm is assumed.  To change the norm,
//		uncomment the appropriate set of macros below.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	Use the following for the Euclidean norm
//----------------------------------------------------------------------
#define ANN_POW(v)			((v)*(v))
#define ANN_ROOT(x)			sqrt(x)
#define ANN_SUM(x,y)		((x) + (y))
#define ANN_DIFF(x,y)		((y) - (x))

//----------------------------------------------------------------------
//	Use the following for the L_1 (Manhattan) norm
//----------------------------------------------------------------------
// #define ANN_POW(v)		fabs(v)
// #define ANN_ROOT(x)		(x)
// #define ANN_SUM(x,y)		((x) + (y))
// #define ANN_DIFF(x,y)	((y) - (x))

//----------------------------------------------------------------------
//	Use the following for a general L_p norm
//----------------------------------------------------------------------
// #define ANN_POW(v)		pow(fabs(v),p)
// #define ANN_ROOT(x)		pow(fabs(x),1/p)
// #define ANN_SUM(x,y)		((x) + (y))
// #define ANN_DIFF(x,y)	((y) - (x))

//----------------------------------------------------------------------
//	Use the following for the L_infinity (Max) norm
//----------------------------------------------------------------------
// #define ANN_POW(v)		fabs(v)
// #define ANN_ROOT(x)		(x)
// #define ANN_SUM(x,y)		((x) > (y) ? (x) : (y))
// #define ANN_DIFF(x,y)	(y)

//----------------------------------------------------------------------
//	Array types
//		The following array types are of basic interest.  A point is
//		just a dimensionless array of coordinates, a point array is a
//		dimensionless array of points.  A distance array is a
//		dimensionless array of distances and an index array is a
//		dimensionless array of point indices.  The latter two are used
//		when returning the results of k-nearest neighbor queries.
//----------------------------------------------------------------------

typedef ANNcoord* ANNpoint;			// a point
typedef ANNpoint* ANNpointArray;	// an array of points 
typedef ANNdist*  ANNdistArray;		// an array of distances 
typedef ANNidx*   ANNidxArray;		// an array of point indices

//----------------------------------------------------------------------
//	Basic point and array utilities:
//		The following procedures are useful supplements to ANN's nearest
//		neighbor capabilities.
//
//		annDist():
//			Computes the (squared) distance between a pair of points.
//			Note that this routine is not used internally by ANN for
//			computing distance calculations.  For reasons of efficiency
//			this is done using incremental distance calculation.  Thus,
//			this routine cannot be modified as a method of changing the
//			metric.
//
//		Because points (somewhat like strings in C) are stored as
//		pointers.  Consequently, creating and destroying copies of
//		points may require storage allocation.  These procedures do
//		this.
//
//		annAllocPt() and annDeallocPt():
//				Allocate a deallocate storage for a single point, and
//				return a pointer to it.  The argument to AllocPt() is
//				used to initialize all components.
//
//		annAllocPts() and annDeallocPts():
//				Allocate and deallocate an array of points as well a
//				place to store their coordinates, and initializes the
//				points to point to their respective coordinates.  It
//				allocates point storage in a contiguous block large
//				enough to store all the points.  It performs no
//				initialization.
//
//		annCopyPt():
//				Creates a copy of a given point, allocating space for
//				the new point.  It returns a pointer to the newly
//				allocated copy.
//----------------------------------------------------------------------
   
DLL_API ANNdist annDist(
	int				dim,		// dimension of space
	ANNpoint		p,			// points
	ANNpoint		q);

DLL_API ANNpoint annAllocPt(
	int				dim,		// dimension
	ANNcoord		c = 0);		// coordinate value (all equal)

DLL_API ANNpointArray annAllocPts(
	int				n,			// number of points
	int				dim);		// dimension

DLL_API void annDeallocPt(
	ANNpoint		&p);		// deallocate 1 point
   
DLL_API void annDeallocPts(
	ANNpointArray	&pa);		// point array

DLL_API ANNpoint annCopyPt(
	int				dim,		// dimension
	ANNpoint		source);	// point to copy

//----------------------------------------------------------------------
//Overall structure: ANN supports a number of different data structures
//for approximate and exact nearest neighbor searching.  These are:
//
//		ANNbruteForce	A simple brute-force search structure.
//		ANNkd_tree		A kd-tree tree search structure.  ANNbd_tree
//		A bd-tree tree search structure (a kd-tree with shrink
//		capabilities).
//
//		At a minimum, each of these data structures support k-nearest
//		neighbor queries.  The nearest neighbor query, annkSearch,
//		returns an integer identifier and the distance to the nearest
//		neighbor(s) and annRangeSearch returns the nearest points that
//		lie within a given query ball.
//
//		Each structure is built by invoking the appropriate constructor
//		and passing it (at a minimum) the array of points, the total
//		number of points and the dimension of the space.  Each structure
//		is also assumed to support a destructor and member functions
//		that return basic information about the point set.
//
//		Note that the array of points is not copied by the data
//		structure (for reasons of space efficiency), and it is assumed
//		to be constant throughout the lifetime of the search structure.
//
//		The search algorithm, annkSearch, is given the query point (q),
//		and the desired number of nearest neighbors to report (k), and
//		the error bound (eps) (whose default value is 0, implying exact
//		nearest neighbors).  It returns two arrays which are assumed to
//		contain at least k elements: one (nn_idx) contains the indices
//		(within the point array) of the nearest neighbors and the other
//		(dd) contains the squared distances to these nearest neighbors.
//
//		The search algorithm, annkFRSearch, is a fixed-radius kNN
//		search.  In addition to a query point, it is given a (squared)
//		radius bound.  (This is done for consistency, because the search
//		returns distances as squared quantities.) It does two things.
//		First, it computes the k nearest neighbors within the radius
//		bound, and second, it returns the total number of points lying
//		within the radius bound. It is permitted to set k = 0, in which
//		case it effectively answers a range counting query.  If the
//		error bound epsilon is positive, then the search is approximate
//		in the sense that it is free to ignore any point that lies
//		outside a ball of radius r/(1+epsilon), where r is the given
//		(unsquared) radius bound.
//
//		The generic object from which all the search structures are
//		dervied is given below.  It is a virtual object, and is useless
//		by itself.
//----------------------------------------------------------------------

class DLL_API ANNpointSet {
public:
	virtual ~ANNpointSet() {}			// virtual distructor

	virtual void annkSearch(			// approx k near neighbor search
		ANNpoint		q,				// query point
		int				k,				// number of near neighbors to return
		ANNidxArray		nn_idx,			// nearest neighbor array (modified)
		ANNdistArray	dd,				// dist to near neighbors (modified)
		double			eps=0.0			// error bound
		) = 0;							// pure virtual (defined elsewhere)

	virtual int annkFRSearch(			// approx fixed-radius kNN search
		ANNpoint		q,				// query point
		ANNdist			sqRad,			// squared radius
		int				k = 0,			// number of near neighbors to return
		ANNidxArray		nn_idx = NULL,	// nearest neighbor array (modified)
		ANNdistArray	dd = NULL,		// dist to near neighbors (modified)
		double			eps=0.0			// error bound
		) = 0;							// pure virtual (defined elsewhere)

	virtual int theDim() = 0;			// return dimension of space
	virtual int nPoints() = 0;			// return number of points
										// return pointer to points
	virtual ANNpointArray thePoints() = 0;
};

//----------------------------------------------------------------------
//	Brute-force nearest neighbor search:
//		The brute-force search structure is very simple but inefficient.
//		It has been provided primarily for the sake of comparison with
//		and validation of the more complex search structures.
//
//		Query processing is the same as described above, but the value
//		of epsilon is ignored, since all distance calculations are
//		performed exactly.
//
//		WARNING: This data structure is very slow, and should not be
//		used unless the number of points is very small.
//
//		Internal information:
//		---------------------
//		This data structure bascially consists of the array of points
//		(each a pointer to an array of coordinates).  The search is
//		performed by a simple linear scan of all the points.
//----------------------------------------------------------------------

class DLL_API ANNbruteForce: public ANNpointSet {
	int				dim;				// dimension
	int				n_pts;				// number of points
	ANNpointArray	pts;				// point array
public:
	ANNbruteForce(						// constructor from point array
		ANNpointArray	pa,				// point array
		int				n,				// number of points
		int				dd);			// dimension

	~ANNbruteForce();					// destructor

	void annkSearch(					// approx k near neighbor search
		ANNpoint		q,				// query point
		int				k,				// number of near neighbors to return
		ANNidxArray		nn_idx,			// nearest neighbor array (modified)
		ANNdistArray	dd,				// dist to near neighbors (modified)
		double			eps=0.0);		// error bound

	int annkFRSearch(					// approx fixed-radius kNN search
		ANNpoint		q,				// query point
		ANNdist			sqRad,			// squared radius
		int				k = 0,			// number of near neighbors to return
		ANNidxArray		nn_idx = NULL,	// nearest neighbor array (modified)
		ANNdistArray	dd = NULL,		// dist to near neighbors (modified)
		double			eps=0.0);		// error bound

	int theDim()						// return dimension of space
		{ return dim; }

	int nPoints()						// return number of points
		{ return n_pts; }

	ANNpointArray thePoints()			// return pointer to points
		{  return pts;  }
};

//----------------------------------------------------------------------
// kd- and bd-tree splitting and shrinking rules
//		kd-trees supports a collection of different splitting rules.
//		In addition to the standard kd-tree splitting rule proposed
//		by Friedman, Bentley, and Finkel, we have introduced a
//		number of other splitting rules, which seem to perform
//		as well or better (for the distributions we have tested).
//
//		The splitting methods given below allow the user to tailor
//		the data structure to the particular data set.  They are
//		are described in greater details in the kd_split.cc source
//		file.  The method ANN_KD_SUGGEST is the method chosen (rather
//		subjectively) by the implementors as the one giving the
//		fastest performance, and is the default splitting method.
//
//		As with splitting rules, there are a number of different
//		shrinking rules.  The shrinking rule ANN_BD_NONE does no
//		shrinking (and hence produces a kd-tree tree).  The rule
//		ANN_BD_SUGGEST uses the implementors favorite rule.
//----------------------------------------------------------------------

enum ANNsplitRule {
		ANN_KD_STD				= 0,	// the optimized kd-splitting rule
		ANN_KD_MIDPT			= 1,	// midpoint split
		ANN_KD_FAIR				= 2,	// fair split
		ANN_KD_SL_MIDPT			= 3,	// sliding midpoint splitting method
		ANN_KD_SL_FAIR			= 4,	// sliding fair split method
		ANN_KD_SUGGEST			= 5};	// the authors' suggestion for best
const int ANN_N_SPLIT_RULES		= 6;	// number of split rules

enum ANNshrinkRule {
		ANN_BD_NONE				= 0,	// no shrinking at all (just kd-tree)
		ANN_BD_SIMPLE			= 1,	// simple splitting
		ANN_BD_CENTROID			= 2,	// centroid splitting
		ANN_BD_SUGGEST			= 3};	// the authors' suggested choice
const int ANN_N_SHRINK_RULES	= 4;	// number of shrink rules

//----------------------------------------------------------------------
//	kd-tree:
//		The main search data structure supported by ANN is a kd-tree.
//		The main constructor is given a set of points and a choice of
//		splitting method to use in building the tree.
//
//		Construction:
//		-------------
//		The constructor is given the point array, number of points,
//		dimension, bucket size (default = 1), and the splitting rule
//		(default = ANN_KD_SUGGEST).  The point array is not copied, and
//		is assumed to be kept constant throughout the lifetime of the
//		search structure.  There is also a "load" constructor that
//		builds a tree from a file description that was created by the
//		Dump operation.
//
//		Search:
//		-------
//		There are two search methods:
//
//			Standard search (annkSearch()):
//				Searches nodes in tree-traversal order, always visiting
//				the closer child first.
//			Priority search (annkPriSearch()):
//				Searches nodes in order of increasing distance of the
//				associated cell from the query point.  For many
//				distributions the standard search seems to work just
//				fine, but priority search is safer for worst-case
//				performance.
//
//		Printing:
//		---------
//		There are two methods provided for printing the tree.  Print()
//		is used to produce a "human-readable" display of the tree, with
//		indenation, which is handy for debugging.  Dump() produces a
//		format that is suitable reading by another program.  There is a
//		"load" constructor, which constructs a tree which is assumed to
//		have been saved by the Dump() procedure.
//		
//		Performance and Structure Statistics:
//		-------------------------------------
//		The procedure getStats() collects statistics information on the
//		tree (its size, height, etc.)  See ANNperf.h for information on
//		the stats structure it returns.
//
//		Internal information:
//		---------------------
//		The data structure consists of three major chunks of storage.
//		The first (implicit) storage are the points themselves (pts),
//		which have been provided by the users as an argument to the
//		constructor, or are allocated dynamically if the tree is built
//		using the load constructor).  These should not be changed during
//		the lifetime of the search structure.  It is the user's
//		responsibility to delete these after the tree is destroyed.
//
//		The second is the tree itself (which is dynamically allocated in
//		the constructor) and is given as a pointer to its root node
//		(root).  These nodes are automatically deallocated when the tree
//		is deleted.  See the file src/kd_tree.h for further information
//		on the structure of the tree nodes.
//
//		Each leaf of the tree does not contain a pointer directly to a
//		point, but rather contains a pointer to a "bucket", which is an
//		array consisting of point indices.  The third major chunk of
//		storage is an array (pidx), which is a large array in which all
//		these bucket subarrays reside.  (The reason for storing them
//		separately is the buckets are typically small, but of varying
//		sizes.  This was done to avoid fragmentation.)  This array is
//		also deallocated when the tree is deleted.
//
//		In addition to this, the tree consists of a number of other
//		pieces of information which are used in searching and for
//		subsequent tree operations.  These consist of the following:
//
//		dim						Dimension of space
//		n_pts					Number of points currently in the tree
//		n_max					Maximum number of points that are allowed
//								in the tree
//		bkt_size				Maximum bucket size (no. of points per leaf)
//		bnd_box_lo				Bounding box low point
//		bnd_box_hi				Bounding box high point
//		splitRule				Splitting method used
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Some types and objects used by kd-tree functions
// See src/kd_tree.h and src/kd_tree.cpp for definitions
//----------------------------------------------------------------------
class ANNkdStats;				// stats on kd-tree
class ANNkd_node;				// generic node in a kd-tree
typedef ANNkd_node*	ANNkd_ptr;	// pointer to a kd-tree node

class DLL_API ANNkd_tree: public ANNpointSet {
protected:
	int				dim;				// dimension of space
	int				n_pts;				// number of points in tree
	int				bkt_size;			// bucket size
	ANNpointArray	pts;				// the points
	ANNidxArray		pidx;				// point indices (to pts array)
	ANNkd_ptr		root;				// root of kd-tree
	ANNpoint		bnd_box_lo;			// bounding box low point
	ANNpoint		bnd_box_hi;			// bounding box high point

	void SkeletonTree(					// construct skeleton tree
		int				n,				// number of points
		int				dd,				// dimension
		int				bs,				// bucket size
		ANNpointArray pa = NULL,		// point array (optional)
		ANNidxArray pi = NULL);			// point indices (optional)

public:
	ANNkd_tree(							// build skeleton tree
		int				n = 0,			// number of points
		int				dd = 0,			// dimension
		int				bs = 1);		// bucket size

	ANNkd_tree(							// build from point array
		ANNpointArray	pa,				// point array
		int				n,				// number of points
		int				dd,				// dimension
		int				bs = 1,			// bucket size
		ANNsplitRule	split = ANN_KD_SUGGEST);	// splitting method

	ANNkd_tree(							// build from dump file
		std::istream&	in);			// input stream for dump file

	~ANNkd_tree();						// tree destructor

	void annkSearch(					// approx k near neighbor search
		ANNpoint		q,				// query point
		int				k,				// number of near neighbors to return
		ANNidxArray		nn_idx,			// nearest neighbor array (modified)
		ANNdistArray	dd,				// dist to near neighbors (modified)
		double			eps=0.0);		// error bound

	void annkPriSearch( 				// priority k near neighbor search
		ANNpoint		q,				// query point
		int				k,				// number of near neighbors to return
		ANNidxArray		nn_idx,			// nearest neighbor array (modified)
		ANNdistArray	dd,				// dist to near neighbors (modified)
		double			eps=0.0);		// error bound

	int annkFRSearch(					// approx fixed-radius kNN search
		ANNpoint		q,				// the query point
		ANNdist			sqRad,			// squared radius of query ball
		int				k,				// number of neighbors to return
		ANNidxArray		nn_idx = NULL,	// nearest neighbor array (modified)
		ANNdistArray	dd = NULL,		// dist to near neighbors (modified)
		double			eps=0.0);		// error bound

	int theDim()						// return dimension of space
		{ return dim; }

	int nPoints()						// return number of points
		{ return n_pts; }

	ANNpointArray thePoints()			// return pointer to points
		{  return pts;  }

	virtual void Print(					// print the tree (for debugging)
		ANNbool			with_pts,		// print points as well?
		std::ostream&	out);			// output stream

	virtual void Dump(					// dump entire tree
		ANNbool			with_pts,		// print points as well?
		std::ostream&	out);			// output stream
								
	virtual void getStats(				// compute tree statistics
		ANNkdStats&		st);			// the statistics (modified)
};								

//----------------------------------------------------------------------
//	Box decomposition tree (bd-tree)
//		The bd-tree is inherited from a kd-tree.  The main difference
//		in the bd-tree and the kd-tree is a new type of internal node
//		called a shrinking node (in the kd-tree there is only one type
//		of internal node, a splitting node).  The shrinking node
//		makes it possible to generate balanced trees in which the
//		cells have bounded aspect ratio, by allowing the decomposition
//		to zoom in on regions of dense point concentration.  Although
//		this is a nice idea in theory, few point distributions are so
//		densely clustered that this is really needed.
//----------------------------------------------------------------------

class DLL_API ANNbd_tree: public ANNkd_tree {
public:
	ANNbd_tree(							// build skeleton tree
		int				n,				// number of points
		int				dd,				// dimension
		int				bs = 1)			// bucket size
		: ANNkd_tree(n, dd, bs) {}		// build base kd-tree

	ANNbd_tree(							// build from point array
		ANNpointArray	pa,				// point array
		int				n,				// number of points
		int				dd,				// dimension
		int				bs = 1,			// bucket size
		ANNsplitRule	split  = ANN_KD_SUGGEST,	// splitting rule
		ANNshrinkRule	shrink = ANN_BD_SUGGEST);	// shrinking rule

	ANNbd_tree(							// build from dump file
		std::istream&	in);			// input stream for dump file
};

//----------------------------------------------------------------------
//	Other functions
//	annMaxPtsVisit		Sets a limit on the maximum number of points
//						to visit in the search.
//  annClose			Can be called when all use of ANN is finished.
//						It clears up a minor memory leak.
//----------------------------------------------------------------------

DLL_API void annMaxPtsVisit(	// max. pts to visit in search
	int				maxPts);	// the limit

DLL_API void annClose();		// called to end use of ANN

#endif
