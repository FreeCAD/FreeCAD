//----------------------------------------------------------------------
// File:			perf.cpp
// Programmer:		Sunil Arya and David Mount
// Description:		Methods for performance stats
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
//		Changed names to avoid namespace conflicts.
//		Added flush after printing performance stats to fix bug
//			in Microsoft Windows version.
//----------------------------------------------------------------------

#include <ANN/ANN.h>					// basic ANN includes
#include <ANN/ANNperf.h>				// performance includes

using namespace std;					// make std:: available

//----------------------------------------------------------------------
//	Performance statistics
//		The following data and routines are used for computing
//		performance statistics for nearest neighbor searching.
//		Because these routines can slow the code down, they can be
//		activated and deactiviated by defining the PERF variable,
//		by compiling with the option: -DPERF
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//	Global counters for performance measurement
//----------------------------------------------------------------------

int				ann_Ndata_pts  = 0;		// number of data points
int				ann_Nvisit_lfs = 0;		// number of leaf nodes visited
int				ann_Nvisit_spl = 0;		// number of splitting nodes visited
int				ann_Nvisit_shr = 0;		// number of shrinking nodes visited
int				ann_Nvisit_pts = 0;		// visited points for one query
int				ann_Ncoord_hts = 0;		// coordinate hits for one query
int				ann_Nfloat_ops = 0;		// floating ops for one query
ANNsampStat		ann_visit_lfs;			// stats on leaf nodes visits
ANNsampStat		ann_visit_spl;			// stats on splitting nodes visits
ANNsampStat		ann_visit_shr;			// stats on shrinking nodes visits
ANNsampStat		ann_visit_nds;			// stats on total nodes visits
ANNsampStat		ann_visit_pts;			// stats on points visited
ANNsampStat		ann_coord_hts;			// stats on coordinate hits
ANNsampStat		ann_float_ops;			// stats on floating ops
//
ANNsampStat		ann_average_err;		// average error
ANNsampStat		ann_rank_err;			// rank error

//----------------------------------------------------------------------
//	Routines for statistics.
//----------------------------------------------------------------------

DLL_API void annResetStats(int data_size) // reset stats for a set of queries
{
	ann_Ndata_pts  = data_size;
	ann_visit_lfs.reset();
	ann_visit_spl.reset();
	ann_visit_shr.reset();
	ann_visit_nds.reset();
	ann_visit_pts.reset();
	ann_coord_hts.reset();
	ann_float_ops.reset();
	ann_average_err.reset();
	ann_rank_err.reset();
}

DLL_API void annResetCounts()				// reset counts for one query
{
	ann_Nvisit_lfs = 0;
	ann_Nvisit_spl = 0;
	ann_Nvisit_shr = 0;
	ann_Nvisit_pts = 0;
	ann_Ncoord_hts = 0;
	ann_Nfloat_ops = 0;
}

DLL_API void annUpdateStats()				// update stats with current counts
{
	ann_visit_lfs += ann_Nvisit_lfs;
	ann_visit_nds += ann_Nvisit_spl + ann_Nvisit_lfs;
	ann_visit_spl += ann_Nvisit_spl;
	ann_visit_shr += ann_Nvisit_shr;
	ann_visit_pts += ann_Nvisit_pts;
	ann_coord_hts += ann_Ncoord_hts;
	ann_float_ops += ann_Nfloat_ops;
}

										// print a single statistic
void print_one_stat(char *title, ANNsampStat s, double div)
{
	cout << title << "= [ ";
	cout.width(9); cout << s.mean()/div			<< " : ";
	cout.width(9); cout << s.stdDev()/div		<< " ]<";
	cout.width(9); cout << s.min()/div			<< " , ";
	cout.width(9); cout << s.max()/div			<< " >\n";
}

DLL_API void annPrintStats(				// print statistics for a run
	ANNbool validate)					// true if average errors desired
{
	cout.precision(4);					// set floating precision
	cout << "  (Performance stats: "
		 << " [      mean :    stddev ]<      min ,       max >\n";
	print_one_stat("    leaf_nodes       ", ann_visit_lfs, 1);
	print_one_stat("    splitting_nodes  ", ann_visit_spl, 1);
	print_one_stat("    shrinking_nodes  ", ann_visit_shr, 1);
	print_one_stat("    total_nodes      ", ann_visit_nds, 1);
	print_one_stat("    points_visited   ", ann_visit_pts, 1);
	print_one_stat("    coord_hits/pt    ", ann_coord_hts, ann_Ndata_pts);
	print_one_stat("    floating_ops_(K) ", ann_float_ops, 1000);
	if (validate) {
		print_one_stat("    average_error    ", ann_average_err, 1);
		print_one_stat("    rank_error       ", ann_rank_err, 1);
	}
	cout.precision(0);					// restore the default
	cout << "  )\n";
	cout.flush();
}
