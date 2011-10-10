//----------------------------------------------------------------------
//	File:			ANNperf.h
//	Programmer:		Sunil Arya and David Mount
//	Last modified:	03/04/98 (Release 0.1)
//	Description:	Include file for ANN performance stats
//
//	Some of the code for statistics gathering has been adapted
//	from the SmplStat.h package in the g++ library.
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
//      History:
//      Revision 0.1  03/04/98
//          Initial release
//      Revision 1.0  04/01/05
//          Added ANN_ prefix to avoid name conflicts.
//----------------------------------------------------------------------

#ifndef ANNperf_H
#define ANNperf_H

//----------------------------------------------------------------------
//	basic includes
//----------------------------------------------------------------------

#include <ANN/ANN.h>					// basic ANN includes

//----------------------------------------------------------------------
// kd-tree stats object
//	This object is used for collecting information about a kd-tree
//	or bd-tree.
//----------------------------------------------------------------------

class ANNkdStats {			// stats on kd-tree
public:
	int		dim;			// dimension of space
	int		n_pts;			// no. of points
	int		bkt_size;		// bucket size
	int		n_lf;			// no. of leaves (including trivial)
	int		n_tl;			// no. of trivial leaves (no points)
	int		n_spl;			// no. of splitting nodes
	int		n_shr;			// no. of shrinking nodes (for bd-trees)
	int		depth;			// depth of tree
	float	sum_ar;			// sum of leaf aspect ratios
	float	avg_ar;			// average leaf aspect ratio
 //
							// reset stats
	void reset(int d=0, int n=0, int bs=0)
	{
		dim = d; n_pts = n; bkt_size = bs;
		n_lf = n_tl = n_spl = n_shr = depth = 0;
		sum_ar = avg_ar = 0.0;
	}

	ANNkdStats()			// basic constructor
	{ reset(); }

	void merge(const ANNkdStats &st);	// merge stats from child 
};

//----------------------------------------------------------------------
//  ANNsampStat
//	A sample stat collects numeric (double) samples and returns some
//	simple statistics.  Its main functions are:
//
//		reset()		Reset to no samples.
//		+= x		Include sample x.
//		samples()	Return number of samples.
//		mean()		Return mean of samples.
//		stdDev()	Return standard deviation
//		min()		Return minimum of samples.
//		max()		Return maximum of samples.
//----------------------------------------------------------------------
class DLL_API ANNsampStat {
	int				n;				// number of samples
	double			sum;			// sum
	double			sum2;			// sum of squares
	double			minVal, maxVal;	// min and max
public :
	void reset()				// reset everything
	{  
		n = 0;
		sum = sum2 = 0;
		minVal = ANN_DBL_MAX;
		maxVal = -ANN_DBL_MAX; 
	}

	ANNsampStat() { reset(); }		// constructor

	void operator+=(double x)		// add sample
	{
		n++;  sum += x;  sum2 += x*x;
		if (x < minVal) minVal = x;
		if (x > maxVal) maxVal = x;
	}

	int samples() { return n; }		// number of samples

	double mean() { return sum/n; } // mean

									// standard deviation
	double stdDev() { return sqrt((sum2 - (sum*sum)/n)/(n-1));}

	double min() { return minVal; } // minimum
	double max() { return maxVal; } // maximum
};

//----------------------------------------------------------------------
//		Operation count updates
//----------------------------------------------------------------------

#ifdef ANN_PERF
  #define ANN_FLOP(n)	{ann_Nfloat_ops += (n);}
  #define ANN_LEAF(n)	{ann_Nvisit_lfs += (n);}
  #define ANN_SPL(n)	{ann_Nvisit_spl += (n);}
  #define ANN_SHR(n)	{ann_Nvisit_shr += (n);}
  #define ANN_PTS(n)	{ann_Nvisit_pts += (n);}
  #define ANN_COORD(n)	{ann_Ncoord_hts += (n);}
#else
  #define ANN_FLOP(n)
  #define ANN_LEAF(n)
  #define ANN_SPL(n)
  #define ANN_SHR(n)
  #define ANN_PTS(n)
  #define ANN_COORD(n)
#endif

//----------------------------------------------------------------------
//	Performance statistics
//	The following data and routines are used for computing performance
//	statistics for nearest neighbor searching.  Because these routines
//	can slow the code down, they can be activated and deactiviated by
//	defining the ANN_PERF variable, by compiling with the option:
//	-DANN_PERF
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	Global counters for performance measurement
//
//	visit_lfs	The number of leaf nodes visited in the
//				tree.
//
//	visit_spl	The number of splitting nodes visited in the
//				tree.
//
//	visit_shr	The number of shrinking nodes visited in the
//				tree.
//
//	visit_pts	The number of points visited in all the
//				leaf nodes visited. Equivalently, this
//				is the number of points for which distance
//				calculations are performed.
//
//	coord_hts	The number of times a coordinate of a 
//				data point is accessed. This is generally
//				less than visit_pts*d if partial distance
//				calculation is used.  This count is low
//				in the sense that if a coordinate is hit
//				many times in the same routine we may
//				count it only once.
//
//	float_ops	The number of floating point operations.
//				This includes all operations in the heap
//				as well as distance calculations to boxes.
//
//	average_err	The average error of each query (the
//				error of the reported point to the true
//				nearest neighbor).  For k nearest neighbors
//				the error is computed k times.
//
//	rank_err	The rank error of each query (the difference
//				in the rank of the reported point and its
//				true rank).
//
//	data_pts	The number of data points.  This is not
//				a counter, but used in stats computation.
//----------------------------------------------------------------------

extern int			ann_Ndata_pts;	// number of data points
extern int			ann_Nvisit_lfs;	// number of leaf nodes visited
extern int			ann_Nvisit_spl;	// number of splitting nodes visited
extern int			ann_Nvisit_shr;	// number of shrinking nodes visited
extern int			ann_Nvisit_pts;	// visited points for one query
extern int			ann_Ncoord_hts;	// coordinate hits for one query
extern int			ann_Nfloat_ops;	// floating ops for one query
extern ANNsampStat	ann_visit_lfs;	// stats on leaf nodes visits
extern ANNsampStat	ann_visit_spl;	// stats on splitting nodes visits
extern ANNsampStat	ann_visit_shr;	// stats on shrinking nodes visits
extern ANNsampStat	ann_visit_nds;	// stats on total nodes visits
extern ANNsampStat	ann_visit_pts;	// stats on points visited
extern ANNsampStat	ann_coord_hts;	// stats on coordinate hits
extern ANNsampStat	ann_float_ops;	// stats on floating ops
//----------------------------------------------------------------------
//  The following need to be part of the public interface, because
//  they are accessed outside the DLL in ann_test.cpp.
//----------------------------------------------------------------------
DLL_API extern ANNsampStat ann_average_err;	// average error
DLL_API extern ANNsampStat ann_rank_err;	// rank error

//----------------------------------------------------------------------
//	Declaration of externally accessible routines for statistics
//----------------------------------------------------------------------

DLL_API void annResetStats(int data_size);	// reset stats for a set of queries

DLL_API void annResetCounts();				// reset counts for one queries

DLL_API void annUpdateStats();				// update stats with current counts

DLL_API void annPrintStats(ANNbool validate); // print statistics for a run

#endif
