//----------------------------------------------------------------------
//	File:			rand.cpp
//	Programmer:		Sunil Arya and David Mount
//	Description:	Routines for random point generation
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
//	Revision 0.2  03/26/98
//		Changed random/srandom declarations for SGI's.
//	Revision 1.0  04/01/05
//		annClusGauss centers distributed over [-1,1] rather than [0,1]
//		Added annClusOrthFlats distribution
//		Changed procedure names to avoid namespace conflicts
//		Added annClusFlats distribution
//		Added rand/srand option and fixed annRan0() initialization.
//	Revision 1.1.1  08/04/06
//		Added planted distribution
//----------------------------------------------------------------------

#include "rand.h"						// random generator declarations

using namespace std;					// make std:: accessible

//----------------------------------------------------------------------
//	Globals
//----------------------------------------------------------------------
int		annIdum = 0;					// used for random number generation

//------------------------------------------------------------------------
//	annRan0 - (safer) uniform random number generator
//
//	The code given here is taken from "Numerical Recipes in C" by
//	William Press, Brian Flannery, Saul Teukolsky, and William
//	Vetterling. The task of the code is to do an additional randomizing
//	shuffle on the system-supplied random number generator to make it
//	safer to use. 
//
//	Returns a uniform deviate between 0.0 and 1.0 using the
//	system-supplied routine random() or rand(). Set the global
//	annIdum to any negative value to initialise or reinitialise
//	the sequence.
//------------------------------------------------------------------------

double annRan0()
{
	const int TAB_SIZE = 97;			// table size: any large number
	int j;

	static double y, v[TAB_SIZE];
	static int iff = 0;
	const double RAN_DIVISOR = double(ANN_RAND_MAX + 1UL);
	if (RAN_DIVISOR < 0) {
		cout << "RAN_DIVISOR " << RAN_DIVISOR << endl;
		exit(0);
	}

	//--------------------------------------------------------------------
	// As a precaution against misuse, we will always initialize on the
	// first call, even if "annIdum" is not set negative. Determine
	// "maxran", the next integer after the largest representable value
	// of type int. We assume this is a factor of 2 smaller than the
	// corresponding value of type unsigned int. 
	//--------------------------------------------------------------------

	if (annIdum < 0 || iff == 0) {		// initialize
		iff = 1;
		ANN_SRAND(annIdum);				// (re)seed the generator
		annIdum = 1;

		for (j = 0; j < TAB_SIZE; j++)	// exercise the system routine
			ANN_RAND();					// (values intentionally ignored)

		for (j = 0; j < TAB_SIZE; j++)	// then save TAB_SIZE-1 values
			v[j] = ANN_RAND();
		y = ANN_RAND();					// generate starting value
	 }

	//--------------------------------------------------------------------
	// This is where we start if not initializing. Use the previously
	// saved random number y to get an index j between 1 and TAB_SIZE-1.
	// Then use the corresponding v[j] for both the next j and as the
	// output number.
	//--------------------------------------------------------------------

	j = int(TAB_SIZE * (y / RAN_DIVISOR));
	y = v[j];
	v[j] = ANN_RAND();					// refill the table entry
	return y / RAN_DIVISOR;
}

//------------------------------------------------------------------------
//	annRanInt - generate a random integer from {0,1,...,n-1}
//
//		If n == 0, then -1 is returned.
//------------------------------------------------------------------------

static int annRanInt(
	int					n)
{
	int r = (int) (annRan0()*n);
	if (r == n) r--;					// (in case annRan0() == 1 or n == 0)
	return r;
}

//------------------------------------------------------------------------
//	annRanUnif - generate a random uniform in [lo,hi]
//------------------------------------------------------------------------

static double annRanUnif(
	double				lo,
	double				hi)
{
	return annRan0()*(hi-lo) + lo;
}

//------------------------------------------------------------------------
//	annRanGauss - Gaussian random number generator
//		Returns a normally distributed deviate with zero mean and unit
//		variance, using annRan0() as the source of uniform deviates.
//------------------------------------------------------------------------

static double annRanGauss()
{
	static int iset=0;
	static double gset;

	if (iset == 0) {					// we don't have a deviate handy
		double v1, v2;
		double r = 2.0;
		while (r >= 1.0) {
			//------------------------------------------------------------
			// Pick two uniform numbers in the square extending from -1 to
			// +1 in each direction, see if they are in the circle of radius
			// 1.  If not, try again 
			//------------------------------------------------------------
			v1 = annRanUnif(-1, 1);
			v2 = annRanUnif(-1, 1);
			r = v1 * v1 + v2 * v2;
		}
		double fac = sqrt(-2.0 * log(r) / r);
		//-----------------------------------------------------------------
		// Now make the Box-Muller transformation to get two normal
		// deviates.  Return one and save the other for next time.
		//-----------------------------------------------------------------
		gset = v1 * fac;
		iset = 1;						// set flag
		return v2 * fac;
	}
	else {								// we have an extra deviate handy
		iset = 0;						// so unset the flag
		return gset;					// and return it
	}
}

//------------------------------------------------------------------------
//	annRanLaplace - Laplacian random number generator
//		Returns a Laplacian distributed deviate with zero mean and
//		unit variance, using annRan0() as the source of uniform deviates. 
//
//				prob(x) = b/2 * exp(-b * |x|).
//
//		b is chosen to be sqrt(2.0) so that the variance of the Laplacian
//		distribution [2/(b^2)] becomes 1. 
//------------------------------------------------------------------------

static double annRanLaplace() 
{
	const double b = 1.4142136;

	double laprand = -log(annRan0()) / b;
	double sign = annRan0();
	if (sign < 0.5) laprand = -laprand;
	return(laprand);
}

//----------------------------------------------------------------------
//	annUniformPts - Generate uniformly distributed points
//		A uniform distribution over [-1,1].
//----------------------------------------------------------------------

void annUniformPts(				// uniform distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim)			// dimension
{
	for (int i = 0; i < n; i++) {
		for (int d = 0; d < dim; d++) {
			pa[i][d] = (ANNcoord) (annRanUnif(-1,1));
		}
	}
}

//----------------------------------------------------------------------
//	annGaussPts - Generate Gaussian distributed points
//		A Gaussian distribution with zero mean and the given standard
//		deviation.
//----------------------------------------------------------------------

void annGaussPts(						// Gaussian distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	double				std_dev)		// standard deviation
{
	for (int i = 0; i < n; i++) {
		for (int d = 0; d < dim; d++) {
			pa[i][d] = (ANNcoord) (annRanGauss() * std_dev);
		}
	}
}

//----------------------------------------------------------------------
//	annLaplacePts - Generate Laplacian distributed points
//		Generates a Laplacian distribution (zero mean and unit variance).
//----------------------------------------------------------------------

void annLaplacePts(				// Laplacian distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim)			// dimension
{
	for (int i = 0; i < n; i++) {
		for (int d = 0; d < dim; d++) {
			pa[i][d] = (ANNcoord) annRanLaplace();
		}
	}
}

//----------------------------------------------------------------------
//	annCoGaussPts - Generate correlated Gaussian distributed points
//		Generates a Gauss-Markov distribution of zero mean and unit
//		variance.
//----------------------------------------------------------------------

void annCoGaussPts(				// correlated-Gaussian distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	double				correlation)	// correlation
{
	double std_dev_w = sqrt(1.0 - correlation * correlation);
	for (int i = 0; i < n; i++) {
		double previous = annRanGauss();
		pa[i][0] = (ANNcoord) previous;
		for (int d = 1; d < dim; d++) {
			previous = correlation*previous + std_dev_w*annRanGauss();
			pa[i][d] = (ANNcoord) previous;
		} 
	}
}

//----------------------------------------------------------------------
//	annCoLaplacePts - Generate correlated Laplacian distributed points
//		Generates a Laplacian-Markov distribution of zero mean and unit
//		variance.
//----------------------------------------------------------------------

void annCoLaplacePts(			// correlated-Laplacian distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	double				correlation)	// correlation
{
	double wn;
	double corr_sq = correlation * correlation;

	for (int i = 0; i < n; i++) {
		double previous = annRanLaplace();
		pa[i][0] = (ANNcoord) previous;
		for (int d = 1; d < dim; d++) {
			double temp = annRan0();
			if (temp < corr_sq)
				wn = 0.0;
			else
				wn = annRanLaplace();
			previous = correlation * previous + wn;
			pa[i][d] = (ANNcoord) previous;
		} 
	}
}

//----------------------------------------------------------------------
//	annClusGaussPts - Generate clusters of Gaussian distributed points
//		Cluster centers are uniformly distributed over [-1,1], and the
//		standard deviation within each cluster is fixed.
//
//		Note: Once cluster centers have been set, they are not changed,
//		unless new_clust = true.  This is so that subsequent calls generate
//		points from the same distribution.  It follows, of course, that any
//		attempt to change the dimension or number of clusters without
//		generating new clusters is asking for trouble.
//
//		Note: Cluster centers are not generated by a call to uniformPts().
//		Although this could be done, it has been omitted for
//		compatibility with annClusGaussPts() in the colored version,
//		rand_c.cc.
//----------------------------------------------------------------------

void annClusGaussPts(			// clustered-Gaussian distribution
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	int					n_clus,			// number of colors
	ANNbool				new_clust,		// generate new clusters.
	double				std_dev)		// standard deviation within clusters
{
	static ANNpointArray clusters = NULL;// cluster storage

	if (clusters == NULL || new_clust) {// need new cluster centers
		if (clusters != NULL)			// clusters already exist
			annDeallocPts(clusters);	// get rid of them
		clusters = annAllocPts(n_clus, dim);
										// generate cluster center coords
		for (int i = 0; i < n_clus; i++) {
			for (int d = 0; d < dim; d++) {
				clusters[i][d] = (ANNcoord) annRanUnif(-1,1);
			}
		}
	}

	for (int i = 0; i < n; i++) {
		int c = annRanInt(n_clus);				// generate cluster index
		for (int d = 0; d < dim; d++) {
		  pa[i][d] = (ANNcoord) (std_dev*annRanGauss() + clusters[c][d]);
		}
	}
}

//----------------------------------------------------------------------
//	annClusOrthFlats - points clustered along orthogonal flats
//
//		This distribution consists of a collection points clustered
//		among a collection of axis-aligned low dimensional flats in
//		the hypercube [-1,1]^d.  A set of n_clus orthogonal flats are
//		generated, each whose dimension is a random number between 1
//		and max_dim.  The points are evenly distributed among the clusters.
//		For each cluster, we generate points uniformly distributed along
//		the flat within the hypercube.
//
//		This is done as follows.  Each cluster is defined by a d-element
//		control vector whose components are either:
//		
//				CO_FLAG indicating that this component is to be generated
//						uniformly in [-1,1],
//				x		a value other than CO_FLAG in the range [-1,1],
//						which indicates that this coordinate is to be
//						generated as x plus a Gaussian random deviation
//						with the given standard deviation.
//						
//		The number of zero components is the dimension of the flat, which
//		is a random integer in the range from 1 to max_dim.  The points
//		are disributed between clusters in nearly equal sized groups.
//
//		Note: Once cluster centers have been set, they are not changed,
//		unless new_clust = true.  This is so that subsequent calls generate
//		points from the same distribution.  It follows, of course, that any
//		attempt to change the dimension or number of clusters without
//		generating new clusters is asking for trouble.
//
//		To make this a bad scenario at query time, query points should be
//		selected from a different distribution, e.g. uniform or Gaussian.
//
//		We use a little programming trick to generate groups of roughly
//		equal size.  If n is the total number of points, and n_clus is
//		the number of clusters, then the c-th cluster (0 <= c < n_clus)
//		is given floor((n+c)/n_clus) points.  It can be shown that this
//		will exactly consume all n points.
//
//		This procedure makes use of the utility procedure, genOrthFlat
//		which generates points in one orthogonal flat, according to
//		the given control vector.
//
//----------------------------------------------------------------------
const double CO_FLAG = 999;						// special flag value

static void genOrthFlat(		// generate points on an orthog flat
	ANNpointArray		pa,				// point array
	int					n,				// number of points
	int					dim,			// dimension
	double				*control,		// control vector
	double				std_dev)		// standard deviation
{
	for (int i = 0; i < n; i++) {				// generate each point
		for (int d = 0; d < dim; d++) {			// generate each coord
			if (control[d] == CO_FLAG)			// dimension on flat
				pa[i][d] = (ANNcoord) annRanUnif(-1,1);
			else								// dimension off flat
				pa[i][d] = (ANNcoord) (std_dev*annRanGauss() + control[d]);
		}
	}
}

void annClusOrthFlats(			// clustered along orthogonal flats
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	int					n_clus,			// number of colors
	ANNbool				new_clust,		// generate new clusters.
	double				std_dev,		// standard deviation within clusters
	int					max_dim)		// maximum dimension of the flats
{
	static ANNpointArray control = NULL;		// control vectors

	if (control == NULL || new_clust) {			// need new cluster centers
		if (control != NULL) {					// clusters already exist
			annDeallocPts(control);				// get rid of them
		}
		control = annAllocPts(n_clus, dim);

		for (int c = 0; c < n_clus; c++) {		// generate clusters
			int n_dim = 1 + annRanInt(max_dim); // number of dimensions in flat
			for (int d = 0; d < dim; d++) {		// generate side locations
												// prob. of picking next dim
				double Prob = ((double) n_dim)/((double) (dim-d));
				if (annRan0() < Prob) {			// add this one to flat
					control[c][d] = CO_FLAG;	// flag this entry
					n_dim--;					// one fewer dim to fill
				}
				else {							// don't take this one
					control[c][d] = annRanUnif(-1,1);// random value in [-1,1]
				}
			}
		}
	}
	int offset = 0;								// offset in pa array
	for (int c = 0; c < n_clus; c++) {			// generate clusters
		int pick = (n+c)/n_clus;				// number of points to pick
												// generate the points
		genOrthFlat(pa+offset, pick, dim, control[c], std_dev);
		offset += pick;							// increment offset
	}
}

//----------------------------------------------------------------------
//	annClusEllipsoids - points clustered around axis-aligned ellipsoids
//
//		This distribution consists of a collection points clustered
//		among a collection of low dimensional ellipsoids whose axes
//		are alligned with the coordinate axes in the hypercube [-1,1]^d.
//		The objective is to model distributions in which the points are
//		distributed in lower dimensional subspaces, and within this
//		lower dimensional space the points are distributed with a
//		Gaussian distribution (with no correlation between the
//		dimensions).
//
//		The distribution is given the number of clusters or "colors"
//		(n_clus), maximum number of dimensions (max_dim) of the lower
//		dimensional subspace, a "small" standard deviation
//		(std_dev_small), and a "large" standard deviation range
//		(std_dev_lo, std_dev_hi).
//
//		The algorithm generates n_clus cluster centers uniformly from
//		the hypercube [-1,1]^d.  For each cluster, it selects the
//		dimension of the subspace as a random number r between 1 and
//		max_dim.  These are the dimensions of the ellipsoid.  Then it
//		generates a d-element std dev vector whose entries are the
//		standard deviation for the coordinates of each cluster in the
//		distribution.  Among the d-element control vector, r randomly
//		chosen values are chosen uniformly from the range [std_dev_lo,
//		std_dev_hi].  The remaining values are set to std_dev_small.
//
//		Note that annClusGaussPts is a special case of this in which
//		max_dim = 0, and std_dev = std_dev_small.
//
//		If the flag new_clust is set, then new cluster centers are
//		generated.
//
//		This procedure makes use of the utility procedure genGauss
//		which generates points distributed according to a Gaussian
//		distribution.
//
//----------------------------------------------------------------------

static void genGauss(			// generate points on a general Gaussian
	ANNpointArray		pa,				// point array
	int					n,				// number of points
	int					dim,			// dimension
	double				*center,		// center vector
	double				*std_dev)		// standard deviation vector
{
	for (int i = 0; i < n; i++) {
		for (int d = 0; d < dim; d++) {
			pa[i][d] = (ANNcoord) (std_dev[d]*annRanGauss() + center[d]);
		}
	}
}

void annClusEllipsoids(			// clustered around ellipsoids
	ANNpointArray		pa,				// point array (modified)
	int					n,				// number of points
	int					dim,			// dimension
	int					n_clus,			// number of colors
	ANNbool				new_clust,		// generate new clusters.
	double				std_dev_small,	// small standard deviation
	double				std_dev_lo,		// low standard deviation for ellipses
	double				std_dev_hi,		// high standard deviation for ellipses
	int					max_dim)		// maximum dimension of the flats
{
	static ANNpointArray centers = NULL;		// cluster centers
	static ANNpointArray std_dev = NULL;		// standard deviations

	if (centers == NULL || new_clust) {			// need new cluster centers
		if (centers != NULL)					// clusters already exist
			annDeallocPts(centers);				// get rid of them
		if (std_dev != NULL)					// std deviations already exist
			annDeallocPts(std_dev);				// get rid of them

		centers = annAllocPts(n_clus, dim);		// alloc new clusters and devs
		std_dev	 = annAllocPts(n_clus, dim);

		for (int i = 0; i < n_clus; i++) {		// gen cluster center coords
			for (int d = 0; d < dim; d++) {
				centers[i][d] = (ANNcoord) annRanUnif(-1,1);
			}
		}
		for (int c = 0; c < n_clus; c++) {		// generate cluster std dev
			int n_dim = 1 + annRanInt(max_dim); // number of dimensions in flat
			for (int d = 0; d < dim; d++) {		// generate std dev's
												// prob. of picking next dim
				double Prob = ((double) n_dim)/((double) (dim-d));
				if (annRan0() < Prob) {			// add this one to ellipse
												// generate random std dev
					std_dev[c][d] = annRanUnif(std_dev_lo, std_dev_hi);
					n_dim--;					// one fewer dim to fill
				}
				else {							// don't take this one
					std_dev[c][d] = std_dev_small;// use small std dev
				}
			}
		}
	}

	int offset = 0;								// next slot to fill
	for (int c = 0; c < n_clus; c++) {			// generate clusters
		int pick = (n+c)/n_clus;				// number of points to pick
												// generate the points
		genGauss(pa+offset, pick, dim, centers[c], std_dev[c]);
		offset += pick;							// increment offset in array
	}
}

//----------------------------------------------------------------------
//	annPlanted - Generates points from a "planted" distribution
//		In high dimensional spaces, interpoint distances tend to be
//		highly clustered around the mean value.  Approximate nearest
//		neighbor searching makes little sense in this context, unless it
//		is the case that each query point is significantly closer to its
//		nearest neighbor than to other points.  Thus, the query points
//		should be planted close to the data points. Given a source data
//		set, this procedure generates a set of query points having this
//		property.
//
//		We are given a source data array and a standard deviation.  We
//		generate points as follows.  We select a random point from the
//		source data set, and we generate a Gaussian point centered about
//		this random point and perturbed by a normal distributed random
//		variable with mean zero and the given standard deviation along
//		each coordinate.
//
//		Note that this essentially the same a clustered Gaussian
//		distribution, but where the cluster centers are given by the
//		source data set.
//----------------------------------------------------------------------

void annPlanted(				// planted nearest neighbors
	ANNpointArray	pa,			// point array (modified)
	int				n,			// number of points
	int				dim,		// dimension
	ANNpointArray	src,		// source point array
	int				n_src,		// source size
	double			std_dev)	// standard deviation about source
{
	for (int i = 0; i < n; i++) {
		int c = annRanInt(n_src);			// generate source index
		for (int d = 0; d < dim; d++) {
		  pa[i][d] = (ANNcoord) (std_dev*annRanGauss() + src[c][d]);
		}
	}
}
