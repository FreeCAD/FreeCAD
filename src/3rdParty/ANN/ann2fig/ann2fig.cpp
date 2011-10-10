//----------------------------------------------------------------------
// File:			ann2fig.cpp
// Programmer:		David Mount
// Last modified:	05/03/05
// Description:		convert ann dump file to fig file
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
//		Changed dump file suffix from .ann to .dmp.
//	Revision 1.1  05/03/05
//		Fixed usage output string.
//----------------------------------------------------------------------
//	This program inputs an ann dump file of a search structure
//	perhaps along with point coordinates, and outputs a fig (Ver 3.1)
//	file (see fig2dev (1)) displaying the tree.  The fig file may
//	then be displayed using xfig, or converted to any of a number of
//	other formats using fig2dev.
//
//	If the dimension is 2 then the entire tree is display.  If the
//	dimension is larger than 2 then the user has the option of
//	selecting which two dimensions will be displayed, and the slice
//	value for each of the remaining dimensions.  All leaf cells
//	intersecting the slice are shown along with the points in these
//	cells. See the procedure getArgs() below for the command-line
//	arguments.
//----------------------------------------------------------------------

#include <cstdio>						// C standard I/O
#include <fstream>						// file I/O
#include <string>						// string manipulation
#include <ANN/ANNx.h>					// all ANN includes

using namespace std;					// make std:: accessible

//----------------------------------------------------------------------
// Globals and their defaults
//----------------------------------------------------------------------

const int		STRING_LEN		= 500;	// string lengths
const int		MAX_DIM			= 1000; // maximum dimension
const double	DEF_SLICE_VAL	= 0;	// default slice value
const char		FIG_HEAD[]		= {"#FIG 3.1"}; // fig file header
const char		DUMP_SUFFIX[]	= {".dmp"};	// suffix for dump file
const char		FIG_SUFFIX[]	= {".fig"};	// suffix for fig file

char			file_name[STRING_LEN];	// (root) file name (say xxx)
char			infile_name[STRING_LEN];// input file name (xxx.dmp)
char			outfile_name[STRING_LEN];// output file name (xxx.fig)
char			caption[STRING_LEN];	// caption line (= command line)
ofstream		ofile;					// output file stream
ifstream		ifile;					// input file stream
int				dim_x = 0;				// horizontal dimension
int				dim_y = 1;				// vertical dimension
double			slice_val[MAX_DIM];		// array of slice values
double			u_per_in = 1200;		// fig units per inch (version 3.1)
double			in_size = 5;			// size of figure (in inches)
double			in_low_x = 1;			// fig upper left corner (in inches)
double			in_low_y = 1;			// fig upper left corner (in inches)
double			u_size = 6000;			// size of figure (in units)
double			u_low_x = 1200;			// fig upper left corner (in units)
double			u_low_y = 1200;			// fig upper left corner (in units)
int				pt_size = 10;			// point size (in fig units)

int				dim;					// dimension
int				n_pts;					// number of points
ANNpointArray	pts = NULL;				// point array

double			scale;					// scale factor for transformation
double			offset_x;				// offsets for transformation
double			offset_y;

										// transformations
#define TRANS_X(p)		(offset_x + scale*(p[dim_x]))
#define TRANS_Y(p)		(offset_y - scale*(p[dim_y]))

//----------------------------------------------------------------------
//	Error handler
//----------------------------------------------------------------------

void Error(char *msg, ANNerr level)
{
	if (level == ANNabort) {
		cerr << "ann2fig: ERROR------->" << msg << "<-------------ERROR\n";
		exit(1);
	}
	else {
		cerr << "ann2fig: WARNING----->" << msg << "<-------------WARNING\n";
	}
}

//----------------------------------------------------------------------
// set_slice_val - set all slice values to given value
//----------------------------------------------------------------------

void set_slice_val(double val)
{
	for (int i = 0; i < MAX_DIM; i++) {
		slice_val[i] = val;
	}
}

//----------------------------------------------------------------------
// getArgs - get input arguments
//
//		Syntax:
//		ann2fig [-upi scale] [-x low_x] [-y low_y] 
//				[-sz size] [-dx dim_x] [-dy dim_y] [-sl dim value]*
//				[-ps pointsize]
//				file
//		
//		where:
//			-upi scale			fig units per inch (default = 1200)
//			-x low_x			x and y offset of upper left corner (inches)
//			-y low_y			...(default = 1)
//			-sz size			maximum side length of figure (in inches)
//								...(default = 5)
//			-dx dim_x			horizontal dimension (default = 0)
//			-dy dim_y			vertical dimension (default = 1)
//			-sv value			default slice value (default = 0)
//			-sl dim value		each such pair defines the value along the
//								...given dimension at which to slice.  This
//								...may be supplied for all dimensions except
//								...dim_x and dim_y.
//			-ps pointsize		size of points in fig units (def = 10)
//			file				file (input=file.dmp, output=file.fig)
//
//----------------------------------------------------------------------

void getArgs(int argc, char **argv)
{
	int i;
	int sl_dim;									// temp slice dimension
	double sl_val;								// temp slice value

	set_slice_val(DEF_SLICE_VAL);				// set initial slice-values

	if (argc <= 1) {
		cerr << "Syntax:\n\
        ann2fig [-upi scale] [-x low_x] [-y low_y]\n\
                [-sz size] [-dx dim_x] [-dy dim_y] [-sl dim value]*\n\
                file\n\
        \n\
        where:\n\
            -upi scale          fig units per inch (default = 1200)\n\
            -x low_x            x and y offset of upper left corner (inches)\n\
            -y low_y            ...(default = 1)\n\
            -sz size            maximum side length of figure (in inches)\n\
                                ...(default = 5)\n\
            -dx dim_x           horizontal dimension (default = 0)\n\
            -dy dim_y           vertical dimension (default = 1)\n\
            -sv value           default slice value (default = 0)\n\
            -sl dim value       each such pair defines the value along the\n\
                                ...given dimension at which to slice.  This\n\
                                ...may be supplied for each dimension except\n\
                                ...dim_x and dim_y.\n\
            -ps pointsize       size of points in fig units (def = 10)\n\
            file                file (input=file.dmp, output=file.fig)\n";
		exit(0);
	}

	ANNbool fileSeen = ANNfalse;				// file argument seen?

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-upi")) {			// process -upi option
			sscanf(argv[++i], "%lf", &u_per_in);
		}
		else if (!strcmp(argv[i], "-x")) {		// process -x option
			sscanf(argv[++i], "%lf", &in_low_x);
		}
		else if (!strcmp(argv[i], "-y")) {		// process -y option
			sscanf(argv[++i], "%lf", &in_low_y);
		}
		else if (!strcmp(argv[i], "-sz")) {		// process -sz option
			sscanf(argv[++i], "%lf", &in_size);
		}
		else if (!strcmp(argv[i], "-dx")) {		// process -dx option
			sscanf(argv[++i], "%d", &dim_x);
		}
		else if (!strcmp(argv[i], "-dy")) {		// process -dy option
			sscanf(argv[++i], "%d", &dim_y);
		}
		else if (!strcmp(argv[i], "-sv")) {		// process -sv option
			sscanf(argv[++i], "%lf", &sl_val);
			set_slice_val(sl_val);				// set slice values
		}
		else if (!strcmp(argv[i], "-sl")) {		// process -sl option
			sscanf(argv[++i], "%d", &sl_dim);
			if (sl_dim < 0 || sl_dim >= MAX_DIM) {
				Error("Slice dimension out of bounds", ANNabort);
			}
			sscanf(argv[++i], "%lf", &slice_val[sl_dim]);
		}
		if (!strcmp(argv[i], "-ps")) {			// process -ps option
			sscanf(argv[++i], "%i", &pt_size);
		}
		else {									// must be file name
			fileSeen = ANNtrue;
			sscanf(argv[i], "%s", file_name);
			strcpy(infile_name, file_name);		// copy to input file name
	    	strcat(infile_name, DUMP_SUFFIX);
	    	strcpy(outfile_name, file_name);	// copy to output file name
	    	strcat(outfile_name, FIG_SUFFIX);
		}
	}

	if (!fileSeen) {							// no file seen
		Error("File argument is required", ANNabort);
	}

	ifile.open(infile_name, ios::in);			// open for reading
	if (!ifile) {
		Error("Cannot open input file", ANNabort);
	}
	ofile.open(outfile_name, ios::out);			// open for writing
	if (!ofile) {
		Error("Cannot open output file", ANNabort);
	}

	u_low_x = u_per_in * in_low_x;				// convert inches to fig units
	u_low_y = u_per_in * in_low_y;
	u_size  = u_per_in * in_size;

	strcpy(caption, argv[0]);					// copy command line to caption
	for (i = 1; i < argc; i++) {
		strcat(caption, " ");
		strcat(caption, argv[i]);
	}
}

//----------------------------------------------------------------------
// Graphics utilities for fig output
//
//		writeHeader				write header for fig file
//		writePoint				write a point
//		writeBox				write a box
//		writeLine				write a line
//----------------------------------------------------------------------

void writeHeader()
{
	ofile << FIG_HEAD << "\n"					// fig file header
		 << "Portrait\n"
		 << "Center\n"
		 << "Inches\n"
		 << (int) u_per_in << " 2\n";
}

void writePoint(ANNpoint p)						// write a single point
{
												// filled black point object
	ofile << "1 3 0 1 -1 7 0 0 0 0.000 1 0.0000 ";
	int cent_x = (int) TRANS_X(p);				// transform center coords
	int cent_y = (int) TRANS_Y(p);
	ofile << cent_x << " " << cent_y << " "		// write center, radius, bounds
		 << pt_size << " " << pt_size << " "
		 << cent_x << " " << cent_y << " "
		 << cent_x + pt_size << " " << cent_y + pt_size << "\n";
}

void writeBox(const ANNorthRect &r)				// write box
{
												// unfilled box object
	ofile << "2 2 0 1 -1 7 0 0 -1 0.000 0 0 -1 0 0 5\n";

	int p0_x = (int) TRANS_X(r.lo);				// transform endpoints
	int p0_y = (int) TRANS_Y(r.lo);
	int p1_x = (int) TRANS_X(r.hi);
	int p1_y = (int) TRANS_Y(r.hi);
	ofile << "\t"
		 << p0_x << " " << p0_y << " "			// write vertices
		 << p1_x << " " << p0_y << " "
		 << p1_x << " " << p1_y << " "
		 << p0_x << " " << p1_y << " "
		 << p0_x << " " << p0_y << "\n";
}

void writeLine(ANNpoint p0, ANNpoint p1)		// write line
{
												// unfilled line object
	ofile << "2 1 0 1 -1 7 0 0 -1 0.000 0 0 -1 0 0 2\n";

	int p0_x = (int) TRANS_X(p0);				// transform endpoints
	int p0_y = (int) TRANS_Y(p0);
	int p1_x = (int) TRANS_X(p1);
	int p1_y = (int) TRANS_Y(p1);
	ofile << "\t"
		 << p0_x << " " << p0_y << " "			// write vertices
		 << p1_x << " " << p1_y << "\n";
}

void writeCaption(								// write caption text
	const ANNorthRect	&bnd_box,				// bounding box
	char				*caption)				// caption
{
	if (!strcmp(caption, "\0")) return;			// null string?
	int px = (int) TRANS_X(bnd_box.lo);			// put .5 in. lower left
	int py = (int) (TRANS_Y(bnd_box.lo) + 0.50 * u_per_in); 
	ofile << "4 0 -1 0 0 0 20 0.0000 4 255 2000 ";
	ofile << px << " " << py << " " << caption << "\\001\n";
}

//----------------------------------------------------------------------
// overlap - test whether a box overlap slicing region
//
//		The slicing region is a 2-dimensional plane in space
//		which contains points (x1, x2, ..., xn) satisfying the
//		n-2 linear equalities:
//
//						xi == slice_val[i]		for i != dim_x, dim_y
//
//		This procedure returns true of the box defined by
//		corner points box.lo and box.hi overlap this plane.
//----------------------------------------------------------------------

ANNbool overlap(const ANNorthRect &box)
{
	for (int i = 0; i < dim; i++) {
		if (i != dim_x && i != dim_y &&
		   (box.lo[i] > slice_val[i] || box.hi[i] < slice_val[i]))
			return ANNfalse;
	}
	return ANNtrue;
}

//----------------------------------------------------------------------
// readTree, recReadTree - inputs tree and outputs figure
//
//		readTree procedure initializes things and then calls recReadTree
//		which does all the work.
//
//		recReadTree reads in a node of the tree, makes any recursive
//		calls as needed to input the children of this node (if internal)
//		and maintains the bounding box.  Note that the bounding box
//		is modified within this procedure, but it is the responsibility
//		of the procedure that it be restored to its original value
//		on return.
//
//		Recall that these are the formats.  The tree is given in
//		preorder.
//
//		Leaf node:
//				leaf <n_pts> <bkt[0]> <bkt[1]> ... <bkt[n-1]>
//		Splitting nodes:
//				split <cut_dim> <cut_val> <lo_bound> <hi_bound>
//		Shrinking nodes:
//				shrink <n_bnds>
//						<cut_dim> <cut_val> <side>
//						<cut_dim> <cut_val> <side>
//						... (repeated n_bnds times)
//
//		On reading a leaf we determine whether we should output the
//		cell's points (if dimension = 2 or this cell overlaps the
//		slicing region).  For splitting nodes we check whether the
//		current cell overlaps the slicing plane and whether the
//		cutting dimension coincides with either the x or y drawing
//		dimensions.  If so, we output the corresponding splitting
//		segment.
//----------------------------------------------------------------------

void recReadTree(ANNorthRect &box)
{
	char tag[STRING_LEN];						// tag (leaf, split, shrink)
	int n_pts;									// number of points in leaf
	int idx;									// point index
	int cd;										// cut dimension
	ANNcoord cv;								// cut value
	ANNcoord lb;								// low bound
	ANNcoord hb;								// high bound
	int n_bnds;									// number of bounding sides
	int sd;										// which side

	ifile >> tag;								// input node tag
	if (strcmp(tag, "leaf") == 0) {				// leaf node

		ifile >> n_pts;							// input number of points
												// check for overlap
		if (dim == 2 || overlap(box)) { 
			for (int i = 0; i < n_pts; i++) {	// yes, write the points
				ifile >> idx;
				writePoint(pts[idx]);
			}
		}
		else {									// input but ignore points
			for (int i = 0; i < n_pts; i++) {
				ifile >> idx;
			}
		}
	}
	else if (strcmp(tag, "split") == 0) {		// splitting node

		ifile >> cd >> cv >> lb >> hb;
		if (lb != box.lo[cd] || hb != box.hi[cd]) {
			Error("Bounding box coordinates are fishy", ANNwarn);
		}

		ANNcoord lv = box.lo[cd];				// save bounds for cutting dim
		ANNcoord hv = box.hi[cd];

		//--------------------------------------------------------------
		//	The following code is rather fragile so modify at your
		//	own risk.  We first decrease the high-end of the bounding
		//	box down to the cutting plane and then read the left subtree.
		//	Then we increase the low-end of the bounding box up to the
		//	cutting plane (thus collapsing the bounding box to a d-1
		//	dimensional hyperrectangle).  Then we draw the projection of
		//	its diagonal if it crosses the slicing plane.  This will have
		//	the effect of drawing its intersection on the slicing plane.
		//	Then we restore the high-end of the bounding box and read
		//	the right subtree.  Finally we restore the low-end of the
		//	bounding box, before returning.
		//--------------------------------------------------------------
		box.hi[cd] = cv;						// decrease high bounds
		recReadTree(box);						// read left subtree
												// check for overlap
		box.lo[cd] = cv;						// increase low bounds
		if (dim == 2 || overlap(box)) {			// check for overlap
			if (cd == dim_x || cd == dim_y) {	// cut through slice plane
				writeLine(box.lo, box.hi);		// draw cutting line
			}
		}
		box.hi[cd] = hv;						// restore high bounds

		recReadTree(box);						// read right subtree
		box.lo[cd] = lv;						// restore low bounds
	}
	else if (strcmp(tag, "shrink") == 0) {		// splitting node

		ANNorthRect inner(dim, box);			// copy bounding box
		ifile >> n_bnds;						// number of bounding sides
		for (int i = 0; i < n_bnds; i++) {
			ifile >> cd >> cv >> sd;			// input bounding halfspace
			ANNorthHalfSpace hs(cd, cv, sd);	// create orthogonal halfspace
			hs.project(inner.lo);				// intersect by projecting
			hs.project(inner.hi);
		}
		if (dim == 2 || overlap(inner)) {
			writeBox(inner);					// draw inner rectangle
		}
		recReadTree(inner);						// read inner subtree
		recReadTree(box);						// read outer subtree
	}
	else {
		Error("Illegal node type in dump file", ANNabort);
	}
}

void readTree(ANNorthRect &bnd_box)
{
	writeHeader();								// output header
	writeBox(bnd_box);							// draw bounding box
	writeCaption(bnd_box, caption);				// write caption
	recReadTree(bnd_box);						// do it
}

//----------------------------------------------------------------------
// readANN - read the ANN dump file
//
//		This procedure reads in the dump file.  See the format below.
//		It first reads the header line with version number.  If the
//		points section is present it reads them (otherwise just leaves
//		points = NULL), and then it reads the tree section.  It inputs
//		the bounding box and determines the parameters for transforming
//		the image to figure units.  It then invokes the procedure
//		readTree to do all the real work.
//
//		Dump File Format: <xxx> = coordinate value (ANNcoord)
//
//		#ANN <version number> <comments> [END_OF_LINE]
//		points <dim> <n_pts>			(point coordinates: this is optional)
//		0 <xxx> <xxx> ... <xxx>			(point indices and coordinates)
//		1 <xxx> <xxx> ... <xxx>
//		  ...
//		tree <dim> <n_pts> <bkt_size>
//		<xxx> <xxx> ... <xxx>			(lower end of bounding box)
//		<xxx> <xxx> ... <xxx>			(upper end of bounding box)
//				If the tree is null, then a single line "null" is
//				output.  Otherwise the nodes of the tree are printed
//				one per line in preorder.  Leaves and splitting nodes 
//				have the following formats:
//		Leaf node:
//				leaf <n_pts> <bkt[0]> <bkt[1]> ... <bkt[n-1]>
//		Splitting nodes:
//				split <cut_dim> <cut_val> <lo_bound> <hi_bound>
//		Shrinking nodes:
//				shrink <n_bnds>
//						<cut_dim> <cut_val> <side>
//						<cut_dim> <cut_val> <side>
//						... (repeated n_bnds times)
//
//		Note: Infinite lo_ and hi_bounds are printed as the special
//				values "-INF" and "+INF", respectively.  We do not
//				check for this, because the current version of ANN
//				starts with a finite bounding box if the tree is
//				nonempty.
//----------------------------------------------------------------------

void readANN()
{
	int j;
	char str[STRING_LEN];						// storage for string
    char version[STRING_LEN];					// storage for version
	int  bkt_size;								// bucket size

	ifile >> str;								// input header
	if (strcmp(str, "#ANN") != 0) {				// incorrect header
		Error("Incorrect header for dump file", ANNabort);
	}
    ifile.getline(version, STRING_LEN);			// get version (ignore)
	ifile >> str;								// get major heading
	if (strcmp(str, "points") == 0) {			// points section
		ifile >> dim;							// read dimension
		ifile >> n_pts;							// number of points
		pts = annAllocPts(n_pts, dim);			// allocate points
		for (int i = 0; i < n_pts; i++) {		// input point coordinates
			int idx;							// point index
			ifile >> idx;						// input point index
			if (idx < 0 || idx >= n_pts) {
				Error("Point index is out of range", ANNabort);
			}
			for (j = 0; j < dim; j++) {
				ifile >> pts[idx][j];			// read point coordinates
			}
		}
		ifile >> str;							// get next major heading
	}
	if (strcmp(str, "tree") == 0) {				// tree section
		ifile >> dim;							// read dimension
		if (dim_x > dim || dim_y > dim) {
			Error("Dimensions out of bounds", ANNabort);
		}
		ifile >> n_pts;							// number of points
		ifile >> bkt_size;						// bucket size (ignored)
												// read bounding box
		ANNorthRect bnd_box(dim);				// create bounding box
		for (j = 0; j < dim; j++) {
			ifile >> bnd_box.lo[j];				// read box low coordinates
		}
		for (j = 0; j < dim; j++) {
			ifile >> bnd_box.hi[j];				// read box high coordinates
		}
												// compute scaling factors
		double box_len_x = bnd_box.hi[dim_x] - bnd_box.lo[dim_x];
		double box_len_y = bnd_box.hi[dim_y] - bnd_box.lo[dim_y];
												// longer side determines scale
		if (box_len_x > box_len_y) scale = u_size/box_len_x;
		else					   scale = u_size/box_len_y;
												// compute offsets
		offset_x = u_low_x - scale*bnd_box.lo[dim_x];
		offset_y = u_low_y + scale*bnd_box.hi[dim_y];
		readTree(bnd_box);						// read the tree and process
	}
	else if (strcmp(str, "null") == 0) return;	// empty tree
	else {
		cerr << "Input string: " << str << "\n";
		Error("Illegal ann format.  Expecting section heading", ANNabort);
	}
}

//----------------------------------------------------------------------
// Main program
//
// Gets the command-line arguments and invokes the main scanning
// procedure.
//----------------------------------------------------------------------

int main(int argc, char **argv)
{
	getArgs(argc, argv);						// get input arguments
	readANN();									// read the dump file
	return 0;
}
