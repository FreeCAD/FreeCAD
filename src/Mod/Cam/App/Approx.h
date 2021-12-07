/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Human Rezai <human@mytum.de>                       *
 *   Copyright (c) 2007 Mohamad Najib Muhammad Noor <najib_bean@yahoo.co.uk>
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
/**************APPROX.H*********************
*Class Approximate, inheriting from Routines
*Dependencies:- BOOST, ATLAS, UMFPACK, BLAS
*    LAPACK
********************************************/

#ifndef APPROX_H
#define APPROX_H

#ifndef NDEBUG //This is for faster matrix operations. In fact, some checks are turned off in the uBlas functions
#define NDEBUG
#endif
/*******MAIN INCLUDE*********/
#include "routine.h"

#include <GeomAdaptor_Surface.hxx>

/*******BOOST********/
#include <boost/numeric/ublas/matrix.hpp>

/*****NAMESPACE******/
using namespace boost::numeric;

/*! \class Approximate
 \brief The main class for the approximate routine

 Inheriting the Routines class defined in Routines.h, it takes a mesh structure and tolerance level as it's input parameter.

 As output, it gives out the following NURBS info:-
 Control Points, Knot U, Knot V, Order U, Order V

 where Control Points, Knot U, Knot V are of type std::vectors, Order U and Order V of type int

 In this program, it will be directly converted into a topo surface from the given information
 */
class CamExport Approximate : protected Routines
{
public:
    Approximate(const MeshCore::MeshKernel &m, std::vector<double> &_Cntrl, std::vector<double> &_KnotU, std::vector<double> &_KnotV,
                int &_OrderU, int &_OrderV, double tol);
	~Approximate();
	MeshCore::MeshKernel MeshParam;

	GeomAdaptor_Surface aAdaptorSurface;
protected:
    void ParameterBoundary();
    void ParameterInnerPoints();
    void ErrorApprox();
    void ApproxMain();
    double Reparam();
    void eFair2(ublas::compressed_matrix<double> &E_Matrix);
    void ComputeError(int &h, double eps_1, double eps_2, double &max_error,
                      double &av, double &c2, std::vector <double> &err_w);
    void ExtendNurb(double c2, int h);
    void ReorderNeighbourList(std::set<unsigned long> &pnt,
                              std::set<unsigned long> &face, std::vector<unsigned long> &nei,unsigned long CurInd);
    //void RemakeList(std::vector<MyMesh::VertexHandle> &v_neighbour);

private:
	/** @brief Local Mesh */
    MeshCore::MeshKernel LocalMesh;  //Local Mesh
	/** @brief Parameterized Mesh */
    MeshCore::MeshKernel ParameterMesh;   //Parameterized Mesh - ONLY USED FOR VISUALIZING TO CHECK FOR PARAMETERIZATION ERRORS
    /** @brief total number of mesh-points */
	int NumOfPoints;    //Info about the Mesh
	/** @brief number of inner mesh-points */
    int NumOfInnerPoints;
	/** @brief number of boundary mesh-points */
    int NumOfOuterPoints;
	/** @brief error-tolerance */
    double tolerance;  //error level
	/** @brief Parametervalues in x-direction*/
    ublas::vector<double> ParameterX;   //Parameterized Coordinate Lists
	/** @brief Parametervalues in y-direction*/
    ublas::vector<double> ParameterY;
	/** @brief Parametervalues of the boundary-points in x-direction*/
    ublas::vector<double> BoundariesX;  //Parametrized Boundaries' Coordinate List
	/** @brief Parametervalues of the boundary-points in y-direction*/
    ublas::vector<double> BoundariesY;
	/** @brief Original Boundaries' Coordinate List in x-direction*/
    std::vector<double> UnparamBoundariesX;   //Original Boundaries' Coordinate List
		/** @brief Original Boundaries' Coordinate List in y-direction*/
    std::vector<double> UnparamBoundariesY;
		/** @brief Original Boundaries' Coordinate List in z-direction*/
    std::vector<double> UnparamBoundariesZ;
	/** @brief Original Coordinate List in x-direction*/
    std::vector<double> UnparamX; //Original Coordinate Lists
	/** @brief Original Coordinate List in y-direction*/
    std::vector<double> UnparamY;
	/** @brief Original Coordinate List in z-direction*/
    std::vector<double> UnparamZ;

    std::vector<int> mapper;
	/** @brief List of indices of the boundary points*/
    std::list< std::vector <unsigned long> > BoundariesIndex;
	/** @brief List of point-coordinates of the boundary points*/
    std::list< std::vector <Base::Vector3f> > BoundariesPoints;

    //NURBS
    NURBS MainNurb;

    //Bounding box
    double MinX;
    double MaxX;
    double MinY;
    double MaxY;

};

#endif  /*APPROX_H DEFINED*/

