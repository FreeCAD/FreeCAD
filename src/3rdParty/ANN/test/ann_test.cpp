//----------------------------------------------------------------------
//	File:			ann_test.cpp
//	Programmer:		Sunil Arya and David Mount
//	Description:	test program for ANN (approximate nearest neighbors)
//  Last modified:	08/04/06 (Version 1.1.1)
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
//	Revision 0.2  06/26/98
//		Added CLOCKS_PER_SEC definition if needed
//	Revision 1.0  04/01/05
//		Added comments (from "#" to eol)
//		Added clus_orth_flats and clus_ellipsoids distributions
//		Fixed order of fair and midpt in split_table
//		Added dump/load operations
//		Cleaned up C++ for modern compilers
//	Revision 1.1  05/03/05
//		Added fixed radius kNN search
//	Revision 1.1.1 08/04/06
//		Added planted distribution
//----------------------------------------------------------------------

#include <ctime>						// clock
#include <cmath>						// math routines
#include <string>						// C string ops
#include <fstream>						// file I/O

#include <ANN/ANN.h>					// ANN declarations
#include <ANN/ANNx.h>					// more ANN declarations
#include <ANN/ANNperf.h>				// performance evaluation

#include "rand.h"						// random point generation

#ifndef CLOCKS_PER_SEC					// define clocks-per-second if needed
  #define CLOCKS_PER_SEC 1000000
#endif

using namespace std;					// make std:: available

//----------------------------------------------------------------------
// ann_test
//
// This program is a driver for testing and evaluating the ANN library
// for computing approximate nearest neighbors.  It allows the user to
// generate data and query sets of various sizes, dimensions, and
// distributions, to build kd- and bbd-trees of various types, and then
// run queries and outputting various performance statistics.
//
// Overview:
// ---------
// The test program is run as follows:
// 
//		ann_test < test_input > test_output
//
// where the test_input file contains a list of directives as described
// below.  Directives consist of a directive name, followed by list of
// arguments (depending on the directive).  Arguments and directives are
// separated by white space (blank, tab, and newline).  String arguments
// are not quoted, and consist of a string of nonwhite chacters.  A
// character "#" denotes a comment.  The following characters up to
// the end of line are ignored.  Comments may only be inserted between
// directives (not within the argument list of a directive).
//
// Basic operations:
// -----------------
// The test program can perform the following operations.  How these
// operations are performed depends on the options which are described
// later.
//
//		Data Generation:
//		----------------
//		read_data_pts <file>	Create a set of data points whose
//								coordinates are input from file <file>.
//		gen_data_pts			Create a set of data points whose
//								coordinates are generated from the
//								current point distribution.
//
//		Building the tree:
//		------------------
//		build_ann				Generate an approximate nearest neighbor
//								structure for the current data set, using
//								the selected splitting rules.  Any existing
//								tree will be destroyed.
//
//		Query Generation/Searching:
//		---------------------------
//		read_query_pts <file>	Create a set of query points whose
//								coordinates are input from file <file>.
//		gen_query_pts			Create a set of query points whose
//								coordinates are generated from the
//								current point distribution.
//		run_queries <string>	Apply nearest neighbor searching to the
//								query points using the approximate nearest
//								neighbor structure and the given search
//								strategy.  Possible strategies are:
//									standard = standard kd-tree search
//									priority = priority search
//
//		Miscellaneous:
//		--------------
//		output_label			Output a label to the output file.
//		dump <file>				Dump the current structure to given file.
//								(The dump format is explained further in
//								the source file kd_tree.cc.)
//	  	load <file>				Load a tree from a data file which was
//								created by the dump operation.	Any
//								existing tree will be destroyed.
//
// Options:
// --------
// How these operations are performed depends on a set of options.
// If an option is not specified, a default value is used. An option
// retains its value until it is set again.  String inputs are not
// enclosed in quotes, and must contain no embedded white space (sorry,
// this is C++'s convention).
//
// Options affecting search tree structure:
// ----------------------------------------
//		split_rule <type>		Type of splitting rule to use in building
//								the search tree.  Choices are:
//									kd			= optimized kd-tree
//									midpt		= midpoint split
//									fair		= fair split
//									sl_midpt	= sliding midpt split
//									sl_fair		= sliding fair split
//									suggest		= authors' choice for best
//								The default is "suggest".  See the file
//								kd_split.cc for more detailed information.
//
//		shrink_rule <type>		Type of shrinking rule to use in building
//								a bd-tree data structure.  If "none" is
//								given, then no shrinking is performed and
//								the result is a kd-tree.  Choices are:
//									none		= perform no shrinking
//									simple		= simple shrinking
//									centroid	= centroid shrinking
//									suggest		= authors' choice for best
//								The default is "none".  See the file
//								bd_tree.cc for more information.
//		bucket_size <int>		Bucket size, that is, the maximum number of
//								points stored in each leaf node.
//
// Options affecting data and query point generation:
// --------------------------------------------------
//		dim <int>				Dimension of space.
//		seed <int>				Seed for random number generation.
//		data_size <int>			Number of data points.  When reading data
//								points from a file, this indicates the
//								maximum number of points for storage
//								allocation. Default = 100.
//		query_size <int>		Same as data_size for query points.
//		std_dev <float>			Standard deviation (used in gauss,
//								planted, and clustered distributions).
//								This is the "small" distribution for
//								clus_ellipsoids.  Default = 1.
//		std_dev_lo <float>		Low and high standard deviations (used in
//		std_dev_hi <float>		clus_ellipsoids).  Default = 1.
//		corr_coef <float>		Correlation coefficient (used in co-gauss
//								and co_lapace distributions). Default = 0.05.
//		colors <int>			Number of color classes (clusters) (used
//								in the clustered distributions).  Default = 5.
//		new_clust				Once generated, cluster centers are not
//								normally regenerated.  This is so that both
//								query points and data points can be generated
//								using the same set of clusters.  This option
//								forces new cluster centers to be generated
//								with the next generation of either data or
//								query points.
//		max_clus_dim <int>		Maximum dimension of clusters (used in
//								clus_orth_flats and clus_ellipsoids).
//								Default = 1.
//		distribution <string>	Type of input distribution
//									uniform		= uniform over cube [-1,1]^d.
//									gauss		= Gaussian with mean 0
//									laplace		= Laplacian, mean 0 and var 1
//									co_gauss	= correlated Gaussian
//									co_laplace	= correlated Laplacian
//									clus_gauss	= clustered Gaussian
//									clus_orth_flats = clusters of orth flats
//									clus_ellipsoids = clusters of ellipsoids
//									planted		= planted distribution
//								See the file rand.cpp for further information.
//
// Options affecting nearest neighbor search:
// ------------------------------------------
//		epsilon <float>			Error bound for approx. near neigh. search.
//		near_neigh <int>		Number of nearest neighbors to compute.
//		max_pts_visit <int>		Maximum number of points to visit before
//								terminating.  (Used in applications where
//								real-time performance is important.)
//								(Default = 0, which means no limit.)
//		radius_bound <float>	Sets an upper bound on the nearest
//								neighbor search radius.  If the bound is
//								positive, then fixed-radius nearest
//								neighbor searching is performed, and the
//								count of the number of points in the
//								range is returned.  If the bound is
//								zero, then standard search is used.
//								This can only be used with standard, not
//								priority, search.  (Default = 0, which
//								means standard search.)
//
// Options affection general program behavior:
// -------------------------------------------
//		stats <string>			Level of statistics output
//									silent		 = no output,
//									exec_time	+= execution time only
//									prep_stats	+= preprocessing statistics
//									query_stats += query performance stats
//									query_res	+= results of queries
//									show_pts	+= show the data points
//									show_struct += print search structure
//		validate <string>		Validate experiment and compute average
//								error.  Since validation causes exact
//								nearest neighbors to be computed by the
//								brute force method, this can take a long
//								time.  Valid arguments are:
//									on			= turn validation on
//									off			= turn validation off
//		true_near_neigh <int>	Number of true nearest neighbors to compute.
//								When validating, we compute the difference
//								in rank between each reported nearest neighbor
//								and the true nearest neighbor of the same
//								rank.  Thus it is necessary to compute a
//								few more true nearest neighbors.  By default
//								we compute 10 more than near_neigh.  With
//								this option the exact number can be set.
//								(Used only when validating.)
//
// Example:
// --------
//	output_label test_run_0		# output label for this run
//	  validate off				# do not perform validation
//	  dim 16					# points in dimension 16
//	  stats query_stats			# output performance statistics for queries
//	  seed 121212				# random number seed
//	  data_size 1000
//	  distribution uniform
//	gen_data_pts				# 1000 uniform data points in dim 16
//	  query_size 100
//	  std_dev 0.05
//	  distribution clus_gauss
//	gen_query_pts				# 100 points in 10 clusters with std_dev 0.05
//	  bucket_size 2
//	  split_rule kd
//	  shrink_rule none
//	build_ann					# kd-tree, bucket size 2
//	  epsilon 0.1
//	  near_neigh 5
//	  max_pts_visit 100			# stop search if more than 100 points seen
//	run_queries standard		# run queries; 5 nearest neighbors, 10% error
//	  data_size 500
//	read_data_pts data.in		# read up to 500 points from file data.in
//	  split_rule sl_midpt
//	  shrink_rule simple
//	build_ann					# bd-tree; simple shrink, sliding midpoint split
//	  epsilon 0
//	run_queries priority		# run same queries; 0 allowable error
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//	Constants
//------------------------------------------------------------------------

const int		STRING_LEN		= 500;			// max string length
const double	ERR				= 0.00001;		// epsilon (for float compares)

//------------------------------------------------------------------------
//	Enumerated values and conversions
//------------------------------------------------------------------------

typedef enum {DATA, QUERY} PtType;		// point types

//------------------------------------------------------------------------
//	Statistics output levels
//------------------------------------------------------------------------

typedef enum {					// stat levels
		SILENT,							// no output
		EXEC_TIME,						// just execution time
		PREP_STATS,						// preprocessing info
		QUERY_STATS,					// query performance
		QUERY_RES,						// query results
		SHOW_PTS,						// show data points
		SHOW_STRUCT,					// show tree structure
		N_STAT_LEVELS}					// number of levels
		StatLev;

const char stat_table[N_STAT_LEVELS][STRING_LEN] = {
		"silent",						// SILENT
		"exec_time",					// EXEC_TIME
		"prep_stats",					// PREP_STATS
		"query_stats",					// QUERY_STATS
		"query_res",					// QUERY_RES
		"show_pts",						// SHOW_PTS
		"show_struct"};					// SHOW_STRUCT

//------------------------------------------------------------------------
//	Distributions
//------------------------------------------------------------------------

typedef enum {					// distributions
		UNIFORM,						// uniform over cube [-1,1]^d.
		GAUSS,							// Gaussian with mean 0
		LAPLACE,						// Laplacian, mean 0 and var 1
		CO_GAUSS,						// correlated Gaussian
		CO_LAPLACE,						// correlated Laplacian
		CLUS_GAUSS,						// clustered Gaussian
		CLUS_ORTH_FLATS,				// clustered on orthog flats
		CLUS_ELLIPSOIDS,				// clustered on ellipsoids
		PLANTED,						// planted distribution
		N_DISTRIBS}
		Distrib;

const char distr_table[N_DISTRIBS][STRING_LEN] = {
		"uniform",						// UNIFORM
		"gauss",						// GAUSS
		"laplace",						// LAPLACE
		"co_gauss",						// CO_GAUSS
		"co_laplace",					// CO_LAPLACE
		"clus_gauss",					// CLUS_GAUSS
		"clus_orth_flats",				// CLUS_ORTH_FLATS
		"clus_ellipsoids",				// CLUS_ELLIPSOIS
		"planted"};						// PLANTED

//------------------------------------------------------------------------
//	Splitting rules for kd-trees (see ANN.h for types)
//------------------------------------------------------------------------

const int N_SPLIT_RULES = 6;
const char split_table[N_SPLIT_RULES][STRING_LEN] = {
		"standard",						// standard optimized kd-tree
		"midpt",						// midpoint split
		"fair",							// fair split
		"sl_midpt",						// sliding midpt split
		"sl_fair",						// sliding fair split
		"suggest"};						// authors' choice for best

//------------------------------------------------------------------------
//	Shrinking rules for bd-trees (see ANN.h for types)
//------------------------------------------------------------------------

const int N_SHRINK_RULES = 4;
const char shrink_table[N_SHRINK_RULES][STRING_LEN] = {
		"none",							// perform no shrinking (kd-tree)
		"simple",						// simple shrinking
		"centroid",						// centroid shrinking
		"suggest"};						// authors' choice for best

//----------------------------------------------------------------------
//	Short utility functions
//		Error - general error routine
//		printPoint - print a point to standard output
//		lookUp - look up a name in table and return index
//----------------------------------------------------------------------

void Error(								// error routine
	char				*msg,			// error message
	ANNerr				level)			// abort afterwards
{
	if (level == ANNabort) {
		cerr << "ann_test: ERROR------->" << msg << "<-------------ERROR\n";
		exit(1);
	}
	else {
		cerr << "ann_test: WARNING----->" << msg << "<-------------WARNING\n";
	}
}

void printPoint(						// print point
	ANNpoint			p,				// the point
	int					dim)			// the dimension
{
	cout << "[";
	for (int i = 0; i < dim; i++) {
		cout << p[i];
		if (i < dim-1) cout << ",";
	}
	cout << "]";
}

int lookUp(								// look up name in table
	const char	*arg,					// name to look up
	const char	(*table)[STRING_LEN],	// name table
	int			size)					// table size
{
	int i;
	for (i = 0; i < size; i++) {
		if (!strcmp(arg, table[i])) return i;
	}
	return i;
}

//------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------

void generatePts(						// generate data/query points
	ANNpointArray		&pa,			// point array (returned)
	int					n,				// number of points
	PtType				type,			// point type
	ANNbool				new_clust,		// new cluster centers desired?
	ANNpointArray		src = NULL,		// source array (for PLANTED)
	int					n_src = 0);		// source size (for PLANTED)

void readPts(							// read data/query points from file
	ANNpointArray		&pa,			// point array (returned)
	int					&n,				// number of points
	char				*file_nm,		// file name
	PtType				type);			// point type (DATA, QUERY)

void doValidation();					// perform validation
void getTrueNN();						// compute true nearest neighbors

void treeStats(							// print statistics on kd- or bd-tree
	ostream				&out,			// output stream
	ANNbool				verbose);		// print stats

//------------------------------------------------------------------------
//	Default execution parameters
//------------------------------------------------------------------------
const int		extra_nn		= 10;			// how many extra true nn's?

const int		def_dim			= 2;			// def dimension
const int		def_data_size	= 100;			// def data size
const int		def_query_size	= 100;			// def number of queries
const int		def_n_color		= 5;			// def number of colors
const ANNbool	def_new_clust	= ANNfalse;		// def new clusters flag
const int		def_max_dim		= 1;			// def max flat dimension
const Distrib	def_distr		= UNIFORM;		// def distribution
const double	def_std_dev		= 1.00;			// def standard deviation
const double	def_corr_coef	= 0.05;			// def correlation coef
const int		def_bucket_size = 1;			// def bucket size
const double	def_epsilon		= 0.0;			// def error bound
const int		def_near_neigh	= 1;			// def number of near neighbors
const int		def_max_visit	= 0;			// def number of points visited
const int		def_rad_bound	= 0;			// def radius bound
												// def number of true nn's
const int		def_true_nn		= def_near_neigh + extra_nn;
const int		def_seed		= 0;			// def seed for random numbers
const ANNbool	def_validate	= ANNfalse;		// def validation flag
												// def statistics output level
const StatLev	def_stats		= QUERY_STATS;
const ANNsplitRule								// def splitting rule
				def_split		= ANN_KD_SUGGEST;
const ANNshrinkRule								// def shrinking rule
				def_shrink		= ANN_BD_NONE;

//------------------------------------------------------------------------
//	Global variables - Execution options
//------------------------------------------------------------------------

int				dim;					// dimension
int				data_size;				// data size
int				query_size;				// number of queries
int				n_color;				// number of colors
ANNbool			new_clust;				// generate new clusters?
int				max_dim;				// maximum flat dimension
Distrib			distr;					// distribution
double			corr_coef;				// correlation coef
double			std_dev;				// standard deviation
double			std_dev_lo;				// low standard deviation
double			std_dev_hi;				// high standard deviation
int				bucket_size;			// bucket size
double			epsilon;				// error bound
int				near_neigh;				// number of near neighbors
int				max_pts_visit;			// max number of points to visit
double			radius_bound;			// maximum radius search bound
int				true_nn;				// number of true nn's
ANNbool			validate;				// validation flag
StatLev			stats;					// statistics output level
ANNsplitRule	split;					// splitting rule
ANNshrinkRule	shrink;					// shrinking rule

//------------------------------------------------------------------------
//	More globals - pointers to dynamically allocated arrays and structures
//
//		It is assumed that all these values are set to NULL when nothing
//		is allocated.
//
//		data_pts, query_pts				The data and query points
//		the_tree						Points to the kd- or bd-tree for
//										nearest neighbor searching.
//		apx_nn_idx, apx_dists			Record approximate near neighbor
//										indices and distances
//		apx_pts_in_range				Counts of the number of points in
//										the in approx range, for fixed-
//										radius NN searching.
//		true_nn_idx, true_dists			Record true near neighbor
//										indices and distances
//		min_pts_in_range, max_...		Min and max counts of the number
//										of points in the in approximate
//										range.
//		valid_dirty						To avoid repeated validation,
//										we only validate query results
//										once.  This validation becomes
//										invalid, if a new tree, new data
//										points or new query points have
//										been generated.
//		tree_data_size					The number of points in the
//										current tree.  (This will be the
//										same a data_size unless points have
//										been added since the tree was
//										built.)
//
//		The approximate and true nearest neighbor results are stored
//		in: apx_nn_idx, apx_dists, and true_nn_idx, true_dists.
//		They are really flattened 2-dimensional arrays. Each of these
//		arrays consists of query_size blocks, each of which contains
//		near_neigh (or true_nn) entries, one for each of the nearest
//		neighbors for a given query point.
//------------------------------------------------------------------------

ANNpointArray	data_pts;				// data points
ANNpointArray	query_pts;				// query points
ANNbd_tree*		the_tree;				// kd- or bd-tree search structure
ANNidxArray		apx_nn_idx;				// storage for near neighbor indices
ANNdistArray	apx_dists;				// storage for near neighbor distances
int*			apx_pts_in_range;		// storage for no. of points in range
ANNidxArray		true_nn_idx;			// true near neighbor indices
ANNdistArray	true_dists;				// true near neighbor distances
int*			min_pts_in_range;		// min points in approx range
int*			max_pts_in_range;		// max points in approx range

ANNbool			valid_dirty;			// validation is no longer valid

//------------------------------------------------------------------------
//	Initialize global parameters
//------------------------------------------------------------------------

void initGlobals()
{
	dim					= def_dim;				// init execution parameters
	data_size			= def_data_size;
	query_size			= def_query_size;
	distr				= def_distr;
	corr_coef			= def_corr_coef;
	std_dev				= def_std_dev;
	std_dev_lo			= def_std_dev;
	std_dev_hi			= def_std_dev;
	new_clust			= def_new_clust;
	max_dim				= def_max_dim;
	n_color				= def_n_color;
	bucket_size			= def_bucket_size;
	epsilon				= def_epsilon;
	near_neigh			= def_near_neigh;
	max_pts_visit		= def_max_visit;
	radius_bound		= def_rad_bound;
	true_nn				= def_true_nn;
	validate			= def_validate;
	stats				= def_stats;
	split				= def_split;
	shrink				= def_shrink;
	annIdum				= -def_seed;			// init. global seed for ran0()

	data_pts			= NULL;					// initialize storage pointers
	query_pts			= NULL;
	the_tree			= NULL;
	apx_nn_idx			= NULL;
	apx_dists			= NULL;
	apx_pts_in_range	= NULL;
	true_nn_idx 		= NULL;
	true_dists			= NULL;
	min_pts_in_range	= NULL;
	max_pts_in_range	= NULL;

	valid_dirty			= ANNtrue;				// (validation must be done)
}

//------------------------------------------------------------------------
// getDirective - skip comments and read next directive
//	Returns ANNtrue if directive read, and ANNfalse if eof seen.
//------------------------------------------------------------------------

ANNbool skipComment(				// skip any comments
    istream		&in)				// input stream
{
    char ch = 0;
						// skip whitespace
    do { in.get(ch); } while (isspace(ch) && !in.eof());
    while (ch == '#' && !in.eof()) {		// comment?
						// skip to end of line
	do { in.get(ch); } while(ch != '\n' && !in.eof());
						// skip whitespace
	do { in.get(ch); } while(isspace(ch) && !in.eof());
    }
    if (in.eof()) return ANNfalse;			// end of file
    in.putback(ch);				// put character back
    return ANNtrue;
}

ANNbool getDirective(
    istream		&in,			// input stream
    char		*directive)		// directive storage
{
    if (!skipComment(in))			// skip comments
    	return ANNfalse;			// found eof along the way?
    in >> directive;				// read directive
    return ANNtrue;
}


//------------------------------------------------------------------------
// main program - driver
//		The main program reads input options, invokes the necessary
//		routines to process them.
//------------------------------------------------------------------------

int main(int argc, char** argv)
{
	long		clock0;						// clock time
	char		directive[STRING_LEN];		// input directive
	char		arg[STRING_LEN];			// all-purpose argument

	cout << "------------------------------------------------------------\n"
		 << "ann_test: Version " << ANNversion << " " << ANNversionCmt << "\n"
		 << "    Copyright: " << ANNcopyright << ".\n"
		 << "    Latest Revision: " << ANNlatestRev << ".\n"
		 << "------------------------------------------------------------\n\n";

	initGlobals();								// initialize global values

	//--------------------------------------------------------------------
	//	Main input loop
	//--------------------------------------------------------------------
												// read input directive
	while (getDirective(cin, directive)) {
		//----------------------------------------------------------------
		//	Read options
		//----------------------------------------------------------------
		if (!strcmp(directive,"dim")) {
			cin >> dim;
		}
		else if (!strcmp(directive,"colors")) {
			cin >> n_color;
		}
		else if (!strcmp(directive,"new_clust")) {
			new_clust = ANNtrue;
		}
		else if (!strcmp(directive,"max_clus_dim")) {
			cin >> max_dim;
		}
		else if (!strcmp(directive,"std_dev")) {
			cin >> std_dev;
		}
		else if (!strcmp(directive,"std_dev_lo")) {
			cin >> std_dev_lo;
		}
		else if (!strcmp(directive,"std_dev_hi")) {
			cin >> std_dev_hi;
		}
		else if (!strcmp(directive,"corr_coef")) {
			cin >> corr_coef;
		}
		else if (!strcmp(directive, "data_size")) {
			cin >> data_size;
		}
		else if (!strcmp(directive,"query_size")) {
			cin >> query_size;
		}
		else if (!strcmp(directive,"bucket_size")) {
			cin >> bucket_size;
		}
		else if (!strcmp(directive,"epsilon")) {
			cin >> epsilon;
		}
		else if (!strcmp(directive,"max_pts_visit")) {
			cin >> max_pts_visit;
			valid_dirty = ANNtrue;				// validation must be redone
		}
		else if (!strcmp(directive,"radius_bound")) {
			cin >> radius_bound;
			valid_dirty = ANNtrue;				// validation must be redone
		}
		else if (!strcmp(directive,"near_neigh")) {
			cin >> near_neigh;
			true_nn = near_neigh + extra_nn;	// also reset true near neighs
			valid_dirty = ANNtrue;				// validation must be redone
		}
		else if (!strcmp(directive,"true_near_neigh")) {
			cin >> true_nn;
			valid_dirty = ANNtrue;				// validation must be redone
		}
		//----------------------------------------------------------------
		//	seed option
		//		The seed is reset by setting the global annIdum to the
		//		negation of the seed value.  See rand.cpp.
		//----------------------------------------------------------------
		else if (!strcmp(directive,"seed")) {
			cin >> annIdum;
			annIdum = -annIdum;
		}
		//----------------------------------------------------------------
		//	validate option
		//----------------------------------------------------------------
		else if (!strcmp(directive,"validate")) {
			cin >> arg;							// input argument
			if (!strcmp(arg, "on")) {
				validate = ANNtrue;
				cout << "validate = on   "
					 << "(Warning: this may slow execution time.)\n";
			}
			else if (!strcmp(arg, "off")) {
				validate = ANNfalse;
			}
			else {
				cerr << "Argument: " << arg << "\n";
				Error("validate argument must be \"on\" or \"off\"", ANNabort);
			}
		}
		//----------------------------------------------------------------
		//	distribution option
		//----------------------------------------------------------------
		else if (!strcmp(directive,"distribution")) {
			cin >> arg;							// input name and translate
			distr = (Distrib) lookUp(arg, distr_table, N_DISTRIBS);
			if (distr >= N_DISTRIBS) {			// not something we recognize
				cerr << "Distribution: " << arg << "\n";
				Error("Unknown distribution", ANNabort);
			}
		}
		//----------------------------------------------------------------
		//	stats option
		//----------------------------------------------------------------
		else if (!strcmp(directive,"stats")) {
			cin >> arg;							// input name and translate
			stats = (StatLev) lookUp(arg, stat_table, N_STAT_LEVELS);
			if (stats >= N_STAT_LEVELS) {		// not something we recognize
				cerr << "Stats level: " << arg << "\n";
				Error("Unknown statistics level", ANNabort);
			}
			if (stats > SILENT)
				cout << "stats = " << arg << "\n";
		}
		//----------------------------------------------------------------
		//	split_rule option
		//----------------------------------------------------------------
		else if (!strcmp(directive,"split_rule")) {
			cin >> arg;							// input split_rule name
			split = (ANNsplitRule) lookUp(arg, split_table, N_SPLIT_RULES);
			if (split >= N_SPLIT_RULES) {		// not something we recognize
				cerr << "Splitting rule: " << arg << "\n";
				Error("Unknown splitting rule", ANNabort);
			}
		}
		//----------------------------------------------------------------
		//	shrink_rule option
		//----------------------------------------------------------------
		else if (!strcmp(directive,"shrink_rule")) {
			cin >> arg;							// input split_rule name
			shrink = (ANNshrinkRule) lookUp(arg, shrink_table, N_SHRINK_RULES);
			if (shrink >= N_SHRINK_RULES) {		// not something we recognize
				cerr << "Shrinking rule: " << arg << "\n";
				Error("Unknown shrinking rule", ANNabort);
			}
		}
		//----------------------------------------------------------------
		//	label operation
		//----------------------------------------------------------------
		else if (!strcmp(directive,"output_label")) {
			cin >> arg;
			if (stats > SILENT)
				cout << "<" << arg << ">\n";
		}
		//----------------------------------------------------------------
		//	gen_data_pts operation
		//----------------------------------------------------------------
		else if (!strcmp(directive,"gen_data_pts")) {
			if (distr == PLANTED) {				// planted distribution
				Error("Cannot use planted distribution for data points", ANNabort);
			}
			generatePts(						// generate data points
				data_pts,						// data points
				data_size,						// data size
				DATA,							// data points
				new_clust);						// new clusters flag
			valid_dirty = ANNtrue;				// validation must be redone
			new_clust = ANNfalse;				// reset flag
		}
		//----------------------------------------------------------------
		//	gen_query_pts operation
		//		If the distribution is PLANTED, then the query points
		//		are planted near the data points (which must already be
		//		generated).
		//----------------------------------------------------------------
		else if (!strcmp(directive,"gen_query_pts")) {
			if (distr == PLANTED) {				// planted distribution
				if (data_pts == NULL) {
					Error("Must generate data points before query points for planted distribution", ANNabort);
				}
				generatePts(					// generate query points
					query_pts,					// point array
					query_size,					// number of query points
					QUERY,						// query points
					new_clust,					// new clusters flag
					data_pts,					// plant around data pts
					data_size);
			}
			else {								// all other distributions
				generatePts(					// generate query points
					query_pts,					// point array
					query_size,					// number of query points
					QUERY,						// query points
					new_clust);					// new clusters flag
			}
			valid_dirty = ANNtrue;				// validation must be redone
			new_clust = ANNfalse;				// reset flag
		}
		//----------------------------------------------------------------
		//	read_data_pts operation
		//----------------------------------------------------------------
		else if (!strcmp(directive,"read_data_pts")) {
			cin >> arg;							// input file name
			readPts(
				data_pts,						// point array
				data_size,						// number of points
				arg,							// file name
				DATA);							// data points
			valid_dirty = ANNtrue;				// validation must be redone
		}
		//----------------------------------------------------------------
		//	read_query_pts operation
		//----------------------------------------------------------------
		else if (!strcmp(directive,"read_query_pts")) {
			cin >> arg;							// input file name
			readPts(
				query_pts,						// point array
				query_size,						// number of points
				arg,							// file name
				QUERY);							// query points
			valid_dirty = ANNtrue;				// validation must be redone
		}
		//----------------------------------------------------------------
		//	build_ann operation
		//		We always invoke the constructor for bd-trees.  Note
		//		that when the shrinking rule is NONE (which is true by
		//		default), then this constructs a kd-tree.
		//----------------------------------------------------------------
		else if (!strcmp(directive,"build_ann")) {
			//------------------------------------------------------------
			//	Build the tree
			//------------------------------------------------------------
			if (the_tree != NULL) {				// tree exists already
				delete the_tree;				// get rid of it
			}
			clock0 = clock();					// start time

			the_tree = new ANNbd_tree(			// build it
					data_pts,					// the data points
					data_size,					// number of points
					dim,						// dimension of space
					bucket_size,				// maximum bucket size
					split,						// splitting rule
					shrink);					// shrinking rule

			//------------------------------------------------------------
			//	Print summary
			//------------------------------------------------------------
			long prep_time = clock() - clock0;	// end of prep time

			if (stats > SILENT) {
				cout << "[Build ann-structure:\n";
				cout << "  split_rule    = " << split_table[split] << "\n";
				cout << "  shrink_rule   = " << shrink_table[shrink] << "\n";
				cout << "  data_size     = " << data_size << "\n";
				cout << "  dim           = " << dim << "\n";
				cout << "  bucket_size   = " << bucket_size << "\n";

				if (stats >= EXEC_TIME) {		// output processing time
					cout << "  process_time  = "
						 << double(prep_time)/CLOCKS_PER_SEC << " sec\n";
				}

				if (stats >= PREP_STATS)		// output or check tree stats
					treeStats(cout, ANNtrue);	// print tree stats
				else
					treeStats(cout, ANNfalse);	// check stats

				if (stats >= SHOW_STRUCT) {		// print the whole tree
					cout << "  (Structure Contents:\n";
					the_tree->Print(ANNfalse, cout);
					cout << "  )\n";
				}
				cout << "]\n";
			}
		}
		//----------------------------------------------------------------
		//	dump operation
		//----------------------------------------------------------------
		else if (!strcmp(directive,"dump")) {
			cin >> arg;							// input file name
			if (the_tree == NULL) {				// no tree
				Error("Cannot dump.  No tree has been built yet", ANNwarn);
			}
			else {								// there is a tree
												// try to open file
				ofstream out_dump_file(arg);
				if (!out_dump_file) {
					cerr << "File name: " << arg << "\n";
					Error("Cannot open dump file", ANNabort);
				}
												// dump the tree and points
				the_tree->Dump(ANNtrue, out_dump_file);
				if (stats > SILENT) {
					cout << "(Tree has been dumped to file " << arg << ")\n";
				}
			}
		}
		//----------------------------------------------------------------
		//	load operation
		//		Since this not only loads a tree, but loads a new set
		//		of data points.
		//----------------------------------------------------------------
		else if (!strcmp(directive,"load")) {
			cin >> arg;							// input file name
			if (the_tree != NULL) {				// tree exists already
				delete the_tree;				// get rid of it
			}
			if (data_pts != NULL) {				// data points exist already
				delete data_pts;				// get rid of them
			}

			ifstream in_dump_file(arg);			// try to open file
			if (!in_dump_file) {
				cerr << "File name: " << arg << "\n";
				Error("Cannot open file for loading", ANNabort);
			}
												// build tree by loading
			the_tree = new ANNbd_tree(in_dump_file);

			dim = the_tree->theDim();			// new dimension
			data_size = the_tree->nPoints();	// number of points
			data_pts = the_tree->thePoints();	// new points

			valid_dirty = ANNtrue;				// validation must be redone

			if (stats > SILENT) {
					cout << "(Tree has been loaded from file " << arg << ")\n";
			}
			if (stats >= SHOW_STRUCT) {			// print the tree
				cout << "  (Structure Contents:\n";
				the_tree->Print(ANNfalse, cout);
				cout << "  )\n";
			}
		}
		//----------------------------------------------------------------
		//	run_queries operation
		//		This section does all the query processing.  It consists
		//		of the following subsections:
		//
		//		**	input the argument (standard or priority) and output
		//			the header describing the essential information.
		//		**	allocate space for the results to be stored.
		//		**	run the queries by invoking the appropriate search
		//			procedure on the query points.  Print nearest neighbor
		//			if requested.
		//		**	print final summaries
		//
		//	The approach for processing multiple nearest neighbors is
		//	pretty crude.  We allocate an array whose size is the
		//	product of the total number of queries times the number of
		//	nearest neighbors (k), and then use each k consecutive
		//	entries to store the results of each query.
		//----------------------------------------------------------------
		else if (!strcmp(directive,"run_queries")) {

			//------------------------------------------------------------
			//	Input arguments and print summary
			//------------------------------------------------------------
			enum {STANDARD, PRIORITY} method;

			cin >> arg;							// input argument
			if (!strcmp(arg, "standard")) {
				method = STANDARD;
			}
			else if (!strcmp(arg, "priority")) {
				method = PRIORITY;
			}
			else {
				cerr << "Search type: " << arg << "\n";
				Error("Search type must be \"standard\" or \"priority\"",
						ANNabort);
			}
			if (data_pts == NULL || query_pts == NULL) {
				Error("Either data set and query set not constructed", ANNabort);
			}
			if (the_tree == NULL) {
				Error("No search tree built.", ANNabort);
			}

			//------------------------------------------------------------
			//	Set up everything
			//------------------------------------------------------------

			#ifdef ANN_PERF						// performance only
				annResetStats(data_size);			// reset statistics
			#endif

			clock0 = clock();					// start time
												// deallocate existing storage
			if (apx_nn_idx	 	 != NULL) delete [] apx_nn_idx;
			if (apx_dists		 != NULL) delete [] apx_dists;
			if (apx_pts_in_range != NULL) delete [] apx_pts_in_range;
												// allocate apx answer storage
			apx_nn_idx = new ANNidx[near_neigh*query_size];
			apx_dists  = new ANNdist[near_neigh*query_size];
			apx_pts_in_range = new int[query_size];

			annMaxPtsVisit(max_pts_visit);		// set max points to visit

			//------------------------------------------------------------
			//	Run the queries
			//------------------------------------------------------------
												// pointers for current query
			ANNidxArray	  curr_nn_idx = apx_nn_idx;
			ANNdistArray  curr_dists  = apx_dists;

			for (int i = 0; i < query_size; i++) {
				#ifdef ANN_PERF
					annResetCounts();			// reset counters
				#endif
				apx_pts_in_range[i] = 0;

				if (radius_bound == 0) {		// no radius bound
					if (method == STANDARD) {
						the_tree->annkSearch(
							query_pts[i],		// query point
							near_neigh,			// number of near neighbors
							curr_nn_idx,		// nearest neighbors (returned)
							curr_dists,			// distance (returned)
							epsilon);			// error bound
					}
					else if (method == PRIORITY) {
						the_tree->annkPriSearch(
							query_pts[i],		// query point
							near_neigh,			// number of near neighbors
							curr_nn_idx,		// nearest neighbors (returned)
							curr_dists,			// distance (returned)
							epsilon);			// error bound
					}
					else {
						Error("Internal error - invalid method", ANNabort);
					}
				}
				else {							// use radius bound
					if (method != STANDARD) {
						Error("A nonzero radius bound assumes standard search",
							ANNwarn);
					}
					apx_pts_in_range[i] = the_tree->annkFRSearch(
						query_pts[i],			// query point
						ANN_POW(radius_bound),	// squared radius search bound
						near_neigh,				// number of near neighbors
						curr_nn_idx,			// nearest neighbors (returned)
						curr_dists,				// distance (returned)
						epsilon);				// error bound
				}
				curr_nn_idx += near_neigh;		// increment current pointers
				curr_dists	+= near_neigh;

				#ifdef ANN_PERF
					annUpdateStats();			// update stats
				#endif
			}

			long query_time = clock() - clock0; // end of query time

			if (validate) {						// validation requested
				if (valid_dirty) getTrueNN();	// get true near neighbors
				doValidation();					// validate
			}

			//------------------------------------------------------------
			//	Print summaries
			//------------------------------------------------------------
		
			if (stats > SILENT) {
				cout << "[Run Queries:\n";
				cout << "  query_size    = " << query_size << "\n";
				cout << "  dim           = " << dim << "\n";
				cout << "  search_method = " << arg << "\n";
				cout << "  epsilon       = " << epsilon << "\n";
				cout << "  near_neigh    = " << near_neigh << "\n";
				if (max_pts_visit != 0)
					cout << "  max_pts_visit = " << max_pts_visit << "\n";
				if (radius_bound != 0)
					cout << "  radius_bound  = " << radius_bound << "\n";
				if (validate)
					cout << "  true_nn       = " << true_nn << "\n";

				if (stats >= EXEC_TIME) {		// print exec time summary
					cout << "  query_time    = " <<
						double(query_time)/(query_size*CLOCKS_PER_SEC)
						 << " sec/query";
					#ifdef ANN_PERF
						cout << " (biased by perf measurements)";
					#endif
					cout << "\n";
				}

				if (stats >= QUERY_STATS) {		// output performance stats
					#ifdef ANN_PERF
						cout.flush();
						annPrintStats(validate);
					#else
						cout << "  (Performance statistics unavailable.)\n";
					#endif
				}

				if (stats >= QUERY_RES) {		// output results
					cout << "  (Query Results:\n";
					cout << "    Pt\tANN\tDist\n";
					curr_nn_idx = apx_nn_idx;	// subarray pointers
					curr_dists  = apx_dists;
												// output nearest neighbors
					for (int i = 0; i < query_size; i++) {
						cout << " " << setw(4) << i;
						for (int j = 0; j < near_neigh; j++) {
												// exit if no more neighbors
							if (curr_nn_idx[j] == ANN_NULL_IDX) {
								cout << "\t[no other pts in radius bound]\n";
								break;
							}
							else {				// output point info
								cout << "\t" << curr_nn_idx[j]
								 	<< "\t" << ANN_ROOT(curr_dists[j])
								 	<< "\n";
							}
						}
												// output range count
						if (radius_bound != 0) {
							cout << "    pts_in_radius_bound = "
								<< apx_pts_in_range[i] << "\n";
						}
												// increment subarray pointers
						curr_nn_idx += near_neigh;
						curr_dists  += near_neigh;
					}
					cout << "  )\n";
				}
				cout << "]\n";
			}
		}
		//----------------------------------------------------------------
		//	Unknown directive
		//----------------------------------------------------------------
		else {
			cerr << "Directive: " << directive << "\n";
			Error("Unknown directive", ANNabort);
		}
	}
	//--------------------------------------------------------------------
	//	End of input loop (deallocate stuff that was allocated)
	//--------------------------------------------------------------------
	if (the_tree  		!= NULL) delete the_tree;
	if (data_pts  		!= NULL) annDeallocPts(data_pts);
	if (query_pts 		!= NULL) annDeallocPts(query_pts);
	if (apx_nn_idx		!= NULL) delete [] apx_nn_idx;
	if (apx_dists		!= NULL) delete [] apx_dists;
	if (apx_pts_in_range != NULL) delete [] apx_pts_in_range;

	annClose();			// close ANN

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------
// generatePts - call appropriate routine to generate points of a
//		given distribution.
//------------------------------------------------------------------------

void generatePts(
	ANNpointArray		&pa,			// point array (returned)
	int					n,				// number of points to generate
	PtType				type,			// point type
	ANNbool				new_clust,		// new cluster centers desired?
	ANNpointArray		src,			// source array (if distr=PLANTED)
	int					n_src)			// source size (if distr=PLANTED)
{
	if (pa != NULL) annDeallocPts(pa);			// get rid of any old points
	pa = annAllocPts(n, dim);					// allocate point storage

	switch (distr) {
		case UNIFORM:							// uniform over cube [-1,1]^d.
			annUniformPts(pa, n, dim);
			break;
		case GAUSS:								// Gaussian with mean 0
			annGaussPts(pa, n, dim, std_dev);
			break;
		case LAPLACE:							// Laplacian, mean 0 and var 1
			annLaplacePts(pa, n, dim);
			break;
		case CO_GAUSS:							// correlated Gaussian
			annCoGaussPts(pa, n, dim, corr_coef);
			break;
		case CO_LAPLACE:						// correlated Laplacian
			annCoLaplacePts(pa, n, dim, corr_coef);
			break;
		case CLUS_GAUSS:						// clustered Gaussian
			annClusGaussPts(pa, n, dim, n_color, new_clust, std_dev);
			break;
		case CLUS_ORTH_FLATS:					// clustered on orthog flats
			annClusOrthFlats(pa, n, dim, n_color, new_clust, std_dev, max_dim);
			break;
		case CLUS_ELLIPSOIDS:					// clustered ellipsoids
			annClusEllipsoids(pa, n, dim, n_color, new_clust, std_dev,
						std_dev_lo, std_dev_hi, max_dim);
			break;
		case PLANTED:							// planted distribution
			annPlanted(pa, n, dim, src, n_src, std_dev);
			break;
		default:
			Error("INTERNAL ERROR: Unknown distribution", ANNabort);
			break;
	}

	if (stats > SILENT) {
		if(type == DATA) cout << "[Generating Data Points:\n";
		else			 cout << "[Generating Query Points:\n";
		cout << "  number        = " << n << "\n";
		cout << "  dim           = " << dim << "\n";
		cout << "  distribution  = " << distr_table[distr] << "\n";
		if (annIdum < 0)
			cout << "  seed          = " << annIdum << "\n";
		if (distr == GAUSS || distr == CLUS_GAUSS
		 || distr == CLUS_ORTH_FLATS)
			cout << "  std_dev       = " << std_dev << "\n";
		if (distr == CLUS_ELLIPSOIDS) {
			cout << "  std_dev       = " << std_dev << " (small) \n";
			cout << "  std_dev_lo    = " << std_dev_lo << "\n";
			cout << "  std_dev_hi    = " << std_dev_hi << "\n";
		}
		if (distr == CO_GAUSS || distr == CO_LAPLACE)
			cout << "  corr_coef     = " << corr_coef << "\n";
		if (distr == CLUS_GAUSS || distr == CLUS_ORTH_FLATS
		 || distr == CLUS_ELLIPSOIDS) {
			cout << "  colors        = " << n_color << "\n";
			if (new_clust)
				cout << "  (cluster centers regenerated)\n";
		}
		if (distr == CLUS_ORTH_FLATS || distr == CLUS_ELLIPSOIDS) {
			cout << "  max_dim       = " << max_dim << "\n";
		}
	}
												// want to see points?
	if ((type == DATA  && stats >= SHOW_PTS) ||
		(type == QUERY && stats >= QUERY_RES)) {
		if(type == DATA) cout << "(Data Points:\n";
		else			 cout << "(Query Points:\n";
		for (int i = 0; i < n; i++) {
			cout << " " << setw(4) << i << "\t";
			printPoint(pa[i], dim);
			cout << "\n";
		}
		cout << "  )\n";
	}
	cout << "]\n";
}

//------------------------------------------------------------------------
// readPts - read a collection of data or query points.
//------------------------------------------------------------------------

void readPts(
	ANNpointArray		&pa,			// point array (returned)
	int					&n,				// number of points
	char				*file_nm,		// file name
	PtType				type)			// point type (DATA, QUERY)
{
	int i;
	//--------------------------------------------------------------------
	//	Open input file and read points
	//--------------------------------------------------------------------
	ifstream in_file(file_nm);					// try to open data file
	if (!in_file) {
		cerr << "File name: " << file_nm << "\n";
		Error("Cannot open input data/query file", ANNabort);
	}
												// allocate storage for points
	if (pa != NULL) annDeallocPts(pa);			// get rid of old points
	pa = annAllocPts(n, dim);

	for (i = 0; i < n; i++) {					// read the data
		if (!(in_file >> pa[i][0])) break;
		for (int d = 1; d < dim; d++) {
			in_file >> pa[i][d];
		}
	}

	char ignore_me;								// character for EOF test
	in_file >> ignore_me;						// try to get one more character
	if (!in_file.eof()) {						// exhausted space before eof
		if (type == DATA) 
			Error("`data_size' too small. Input file truncated.", ANNwarn);
		else
			Error("`query_size' too small. Input file truncated.", ANNwarn);
	}
	n = i;										// number of points read

	//--------------------------------------------------------------------
	//	Print summary
	//--------------------------------------------------------------------
	if (stats > SILENT) {
		if (type == DATA) {
			cout << "[Read Data Points:\n";
			cout << "  data_size  = " << n << "\n";
		}
		else {
			cout << "[Read Query Points:\n";
			cout << "  query_size = " << n << "\n";
		}
		cout << "  file_name  = " << file_nm << "\n";
		cout << "  dim        = " << dim << "\n";
												// print if results requested
		if ((type == DATA && stats >= SHOW_PTS) ||
			(type == QUERY && stats >= QUERY_RES)) {
			cout << "  (Points:\n";
			for (i = 0; i < n; i++) {
				cout << "    " << i << "\t";
				printPoint(pa[i], dim);
				cout << "\n";
			}
			cout << "  )\n";
		}
		cout << "]\n";
	}
}

//------------------------------------------------------------------------
//	getTrueNN
//		Computes the true nearest neighbors.  For purposes of validation,
//		this intentionally done in a rather dumb (but safe way), by
//		invoking the brute-force search.
//
//		The number of true nearest neighbors is somewhat larger than
//		the number of nearest neighbors.  This is so that the validation
//		can determine the expected difference in element ranks.
//
//		This procedure is invoked just prior to running queries.  Since
//		the operation takes a long time, it is performed only if needed.
//		In particular, once generated, it will be regenerated only if
//		new query or data points are generated, or if the requested number
//		of true near neighbors or approximate near neighbors has changed.
//
//		To validate fixed-radius searching, we compute two counts, one
//		with the original query radius (trueSqRadius) and the other with
//		a radius shrunken by the error factor (minSqradius).  We then
//		check that the count of points inside the approximate range is
//		between these two bounds.  Because fixed-radius search is
//		allowed to ignore points within the shrunken radius, we only
//		compute exact neighbors within this smaller distance (for we
//		cannot guarantee that we will even visit the other points).
//------------------------------------------------------------------------

void getTrueNN()						// compute true nearest neighbors
{
	if (stats > SILENT) {
		cout << "(Computing true nearest neighbors for validation.  This may take time.)\n";
	}
												// deallocate existing storage
	if (true_nn_idx			!= NULL) delete [] true_nn_idx;
	if (true_dists			!= NULL) delete [] true_dists;
	if (min_pts_in_range	!= NULL) delete [] min_pts_in_range;
	if (max_pts_in_range	!= NULL) delete [] max_pts_in_range;

	if (true_nn > data_size) {					// can't get more nn than points
		true_nn = data_size;
	}

												// allocate true answer storage
	true_nn_idx = new ANNidx[true_nn*query_size];
	true_dists  = new ANNdist[true_nn*query_size];
	min_pts_in_range = new int[query_size];
	max_pts_in_range = new int[query_size];

	ANNidxArray  curr_nn_idx = true_nn_idx;		// current locations in arrays
	ANNdistArray curr_dists = true_dists;

												// allocate search structure
	ANNbruteForce *the_brute = new ANNbruteForce(data_pts, data_size, dim);
												// compute nearest neighbors
	for (int i = 0; i < query_size; i++) {
		if (radius_bound == 0) {				// standard kNN search
			the_brute->annkSearch(				// compute true near neighbors
						query_pts[i],			// query point
						true_nn,				// number of nearest neighbors
						curr_nn_idx,			// where to put indices
						curr_dists);			// where to put distances
		}
		else {									// fixed radius kNN search
												// search radii limits
			ANNdist trueSqRadius = ANN_POW(radius_bound);
			ANNdist minSqRadius = ANN_POW(radius_bound / (1+epsilon));
			min_pts_in_range[i] = the_brute->annkFRSearch(
						query_pts[i],			// query point
						minSqRadius,			// shrunken search radius
						true_nn,				// number of near neighbors
						curr_nn_idx,			// nearest neighbors (returned)
						curr_dists);			// distance (returned)
			max_pts_in_range[i] = the_brute->annkFRSearch(
						query_pts[i],			// query point
						trueSqRadius,			// true search radius
						0, NULL, NULL);			// (ignore kNN info)
		}
		curr_nn_idx += true_nn;					// increment nn index pointer
		curr_dists  += true_nn;					// increment nn dist pointer
	}
	delete the_brute;							// delete brute-force struct
	valid_dirty = ANNfalse;						// validation good for now
}

//------------------------------------------------------------------------
//	doValidation
//		Compares the approximate answers to the k-nearest neighbors
//		against the true nearest neighbors (computed earlier). It is
//		assumed that the true nearest neighbors and indices have been
//		computed earlier.
//
//		First, we check that all the results are within their allowed
//		limits, and generate an internal error, if not.  For the sake of
//		performance evaluation, we also compute the following two
//		quantities for nearest neighbors:
//
//		Average Error
//		-------------
//		The relative error between the distance to a reported nearest
//		neighbor and the true nearest neighbor (of the same rank),
//
//		Rank Error
//		----------
//		The difference in rank between the reported nearest neighbor and
//		its position (if any) among the true nearest neighbors.  If we
//		cannot find this point among the true nearest neighbors, then
//		it assumed that the rank of the true nearest neighbor is true_nn+1.
//
//		Because of the possibility of duplicate distances, this is computed
//		as follows.  For the j-th reported nearest neighbor, we count the
//		number of true nearest neighbors that are at least this close.  Let
//		this be rnk.  Then the rank error is max(0, j-rnk).  (In the code
//		below, j is an array index and so the first item is 0, not 1.  Thus
//		we take max(0, j+1-rnk) instead.)
//
//		For the results of fixed-radious range count, we verify that the
//		reported number of points in the range lies between the actual
//		number of points in the shrunken and the true search radius.
//------------------------------------------------------------------------

void doValidation()						// perform validation
{
	int*		  curr_apx_idx = apx_nn_idx;	// approx index pointer
	ANNdistArray  curr_apx_dst = apx_dists;		// approx distance pointer
	int*		  curr_tru_idx = true_nn_idx;	// true index pointer
	ANNdistArray  curr_tru_dst = true_dists;	// true distance pointer
	int i, j;

	if (true_nn < near_neigh) {
		Error("Cannot validate with fewer true near neighbors than actual", ANNabort);
	}

	for (i = 0; i < query_size; i++) {			// validate each query
		//----------------------------------------------------------------
		//	Compute result errors
		//		In fixed radius search it is possible that not all k
		//		nearest neighbors were computed.  Because the true
		//		results are computed over the shrunken radius, we should
		//		have at least as many true nearest neighbors as
		//		approximate nearest neighbors. (If not, an infinite
		//		error will be generated, and so an internal error will
		//		will be generated.
		//
		//		Because nearest neighbors are sorted in increasing order
		//		of distance, as soon as we see a null index, we can
		//		terminate the distance checking.  The error in the
		//		result should not exceed epsilon.  However, if
		//		max_pts_visit is nonzero (meaning that the search is
		//		terminated early) this might happen.
		//----------------------------------------------------------------
		for (j = 0; j < near_neigh; j++) {
			if (curr_tru_idx[j] == ANN_NULL_IDX)// no more true neighbors?
				break;
												// true i-th smallest distance
			double true_dist = ANN_ROOT(curr_tru_dst[j]);
												// reported i-th smallest
			double rept_dist = ANN_ROOT(curr_apx_dst[j]);
												// better than optimum?
			if (rept_dist < true_dist*(1-ERR)) {
				Error("INTERNAL ERROR: True nearest neighbor incorrect",
						ANNabort);
			}

			double resultErr;					// result error
			if (true_dist == 0.0) {				// let's not divide by zero
				if (rept_dist != 0.0) resultErr = ANN_DBL_MAX;
				else				  resultErr = 0.0;
			}
			else {
				resultErr = (rept_dist - true_dist) / ((double) true_dist);
			}

			if (resultErr > epsilon && max_pts_visit == 0) {
				Error("INTERNAL ERROR: Actual error exceeds epsilon",
						ANNabort);
			}
			#ifdef ANN_PERF
				ann_average_err += resultErr;	// update statistics error
			#endif
		}
		//--------------------------------------------------------------------
		//  Compute rank errors (only needed for perf measurements)
		//--------------------------------------------------------------------
		#ifdef ANN_PERF
			for (j = 0; j < near_neigh; j++) {
				if (curr_tru_idx[i] == ANN_NULL_IDX) // no more true neighbors?
					break;

				double rnkErr = 0.0;			// rank error
												// reported j-th distance
				ANNdist rept_dist = curr_apx_dst[j];
				int rnk = 0;					// compute rank of this item
				while (rnk < true_nn && curr_tru_dst[rnk] <= rept_dist)
					rnk++;
				if (j+1-rnk > 0) rnkErr = (double) (j+1-rnk);
				ann_rank_err += rnkErr;			// update average rank error
			}
		#endif
		//----------------------------------------------------------------
		//	Check range counts from fixed-radius query
		//----------------------------------------------------------------
		if (radius_bound != 0) {				// fixed-radius search
			if  (apx_pts_in_range[i] < min_pts_in_range[i] ||
				 apx_pts_in_range[i] > max_pts_in_range[i])
				Error("INTERNAL ERROR: Invalid fixed-radius range count",
						ANNabort);
		}

		curr_apx_idx += near_neigh;
		curr_apx_dst += near_neigh;
		curr_tru_idx += true_nn;				// increment current pointers
		curr_tru_dst += true_nn;
	}
}

//----------------------------------------------------------------------
//	treeStats
//		Computes a number of statistics related to kd_trees and
//		bd_trees.  These statistics are printed if in verbose mode,
//		and otherwise they are only printed if they are deemed to
//		be outside of reasonable operating bounds.
//----------------------------------------------------------------------

#define log2(x) (log(x)/log(2.0))				// log base 2

void treeStats(
	ostream		&out,							// output stream
	ANNbool		verbose)						// print stats
{
	const int	MIN_PTS		= 20;				// min no. pts for checking
	const float MAX_FRAC_TL = 0.50;				// max frac of triv leaves
	const float MAX_AVG_AR	= 20;				// max average aspect ratio

	ANNkdStats st;								// statistics structure

	the_tree->getStats(st);						// get statistics
												// total number of nodes
	int n_nodes = st.n_lf + st.n_spl + st.n_shr;
												// should be O(n/bs)
	int opt_n_nodes = (int) (2*(float(st.n_pts)/st.bkt_size));
	int too_many_nodes = 10*opt_n_nodes;
	if (st.n_pts >= MIN_PTS && n_nodes > too_many_nodes) {
		out << "-----------------------------------------------------------\n";
		out << "Warning: The tree has more than 10x as many nodes as points.\n";
		out << "You may want to consider a different split or shrink method.\n";
		out << "-----------------------------------------------------------\n";
		verbose = ANNtrue;
	}
												// fraction of trivial leaves
	float frac_tl = (st.n_lf == 0 ? 0 : ((float) st.n_tl)/ st.n_lf);
	if (st.n_pts >= MIN_PTS && frac_tl > MAX_FRAC_TL) {
		out << "-----------------------------------------------------------\n";
		out << "Warning: A significant fraction of leaves contain no points.\n";
		out << "You may want to consider a different split or shrink method.\n";
		out << "-----------------------------------------------------------\n";
		verbose = ANNtrue;
	}
												// depth should be O(dim*log n)
	int too_many_levels = (int) (2.0 * st.dim * log2((double) st.n_pts));
	int opt_levels = (int) log2(double(st.n_pts)/st.bkt_size);
	if (st.n_pts >= MIN_PTS && st.depth > too_many_levels) {
		out << "-----------------------------------------------------------\n";
		out << "Warning: The tree is more than 2x as deep as (dim*log n).\n";
		out << "You may want to consider a different split or shrink method.\n";
		out << "-----------------------------------------------------------\n";
		verbose = ANNtrue;
	}
												// average leaf aspect ratio
	if (st.n_pts >= MIN_PTS && st.avg_ar > MAX_AVG_AR) {
		out << "-----------------------------------------------------------\n";
		out << "Warning: Average aspect ratio of cells is quite large.\n";
		out << "This may slow queries depending on the point distribution.\n";
		out << "-----------------------------------------------------------\n";
		verbose = ANNtrue;
	}

	//------------------------------------------------------------------
	//  Print summaries if requested
	//------------------------------------------------------------------
	if (verbose) {								// output statistics
		out << "  (Structure Statistics:\n";
		out << "    n_nodes          = " << n_nodes
			<< " (opt = " << opt_n_nodes
			<< ", best if < " << too_many_nodes << ")\n"
			<< "        n_leaves     = " << st.n_lf
			<< " (" << st.n_tl << " contain no points)\n"
			<< "        n_splits     = " << st.n_spl << "\n"
			<< "        n_shrinks    = " << st.n_shr << "\n";
		out << "    empty_leaves     = " << frac_tl*100
			<< " percent (best if < " << MAX_FRAC_TL*100 << " percent)\n";
		out << "    depth            = " << st.depth
			<< " (opt = " << opt_levels
			<< ", best if < " << too_many_levels << ")\n";
		out << "    avg_aspect_ratio = " << st.avg_ar
			<< " (best if < " << MAX_AVG_AR << ")\n";
		out << "  )\n";
	}
}
