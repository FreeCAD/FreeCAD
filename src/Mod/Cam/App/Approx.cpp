/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *   Human Rezai <human@mytum.de>                                          *
 *   Mohamad Najib Muhammad Noor <najib_bean@yahoo.co.uk>                  *
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

/*****************APPROX.CPP*****************
* Contains implementations from Approx.h
*
*
*********************************************/


/*********MAIN INCLUDES***********/
#include "PreCompiled.h"
#include "Approx.h"
#include <iostream>
#include <algorithm>
#include <Base/Exception.h>


#include <TColgp_Array2OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_BSplineSurface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

/*************BOOST***************/

/********UBLAS********/
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/blas.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

/*********BINDINGS********/
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/traits/ublas_sparse.hpp>
#include <boost/numeric/bindings/umfpack/umfpack.hpp>
#include <boost/numeric/bindings/atlas/cblas.hpp>
#include <boost/numeric/bindings/atlas/clapack.hpp>

/*****ADDITIONAL FOR DEBUGGING*****/
//#include <fstream>
/*****NAMESPACE**/
using namespace boost::numeric::bindings;

Approximate::Approximate(const MeshCore::MeshKernel &m,std::vector<double> &_Cntrl, std::vector<double> &_KnotU, std::vector<double> &_KnotV,
                         int &_OrderU, int &_OrderV, double tol)
{
    MinX = 0, MinY = 0, MaxX = 0;
    MaxY = 0;
	tolerance = tol;
	int NumPnts = m.CountPoints();  //number of points to approximate
	Base::BoundBox3f bbox = m.GetBoundBox(); // get bounding box
	double x_len = bbox.LengthX();
	double y_len = bbox.LengthY();
    
	LocalMesh = m;  //make a copy...


	//Initialize the NURB
    MainNurb.DegreeU = 3;
    MainNurb.DegreeV = 3;
    MainNurb.MaxU = std::max<int>(MainNurb.DegreeU+1, (int)sqrt(double(NumPnts)*y_len/x_len));
    MainNurb.MaxV = std::max<int>(MainNurb.DegreeV+1, (int)sqrt(double(NumPnts)*x_len/y_len));
    
    GenerateUniformKnot(MainNurb.MaxU,MainNurb.DegreeU,MainNurb.KnotU);
    GenerateUniformKnot(MainNurb.MaxV,MainNurb.DegreeV,MainNurb.KnotV);
    MainNurb.MaxKnotU = MainNurb.KnotU.size();
    MainNurb.MaxKnotV = MainNurb.KnotV.size();
    
	//GOTO Main program
    ApproxMain();

	ofstream anOutputFile;
	anOutputFile.open("c:/approx_build_surface.txt");
	
	anOutputFile << "start building surface" << endl;
    
	//Copying the Output
    //mesh = ParameterMesh;
    _Cntrl = MainNurb.CntrlArray;
    _KnotU = MainNurb.KnotU;
    _KnotV = MainNurb.KnotV;
    _OrderU = MainNurb.DegreeU + 1;
    _OrderV = MainNurb.DegreeV + 1;

	gp_Pnt pnt;
    TColgp_Array2OfPnt Poles(1,MainNurb.MaxU+1, 1,MainNurb.MaxV+1);

    for(int j=0; j< (MainNurb.MaxV+1); ++j)
	{	
		for(int i=0; i < (MainNurb.MaxU+1); ++i)
		{
			pnt.SetX(_Cntrl[3*i   + j*3*(MainNurb.MaxU+1)]);
			pnt.SetY(_Cntrl[3*i+1 + j*3*(MainNurb.MaxU+1)]);
			pnt.SetZ(_Cntrl[3*i+2 + j*3*(MainNurb.MaxU+1)]);

			Poles.SetValue(i+1,j+1,pnt);
		}
    }

	anOutputFile << "control points done" << endl;

    int c=1;
    for (unsigned int i=0; i<_KnotU.size()-1; ++i)
    {
        if (_KnotU[i+1] != _KnotU[i])
        {
            ++c;
        }
    }


    TColStd_Array1OfReal    UKnots(1,c);
    TColStd_Array1OfInteger UMults(1,c);

    c=1;
    for (unsigned int i=0; i<_KnotV.size()-1; ++i)
    {
        if (_KnotV[i+1] != _KnotV[i])
        {
            ++c;
        }
    }


    TColStd_Array1OfReal    VKnots(1,c);
    TColStd_Array1OfInteger VMults(1,c);

    int d=0;
    c=1;
    for (unsigned int i=0; i<_KnotU.size(); ++i)
    {
        if (_KnotU[i+1] != _KnotU[i])
        {
            UKnots.SetValue(d+1,_KnotU[i]);
            UMults.SetValue(d+1,c);
            ++d;
            c=1;

        }
        else
        {
            ++c;
        }

        if (i==(_KnotU.size()-2))
        {
            UKnots.SetValue(d+1,_KnotU[i+1]);
            UMults.SetValue(d+1,c);
            break;
        }
    }

    d=0;
    c=1;
    for (unsigned int i=0; i<_KnotV.size(); ++i)
    {
        if (_KnotV[i+1] != _KnotV[i])
        {
            VKnots.SetValue(d+1,_KnotV[i]);
            VMults.SetValue(d+1,c);
            ++d;
            c=1;

        }
        else
        {
            ++c;
        }

        if (i==(_KnotV.size()-2))
        {
            VKnots.SetValue(d+1,_KnotV[i+1]);
            VMults.SetValue(d+1,c);
            break;
        }
    }

    /*cout << "UKnots: " << endl;
    for(int i=0; i<UKnots.Upper(); ++i)
     cout << UKnots.Value(i+1) << ", ";
    cout << endl;


    cout << "UMults: " << endl;
    for(int i=0; i<UMults.Upper(); ++i)
     cout << UMults.Value(i+1) << ", ";
    cout << endl;


    cout << "VKnots: " << endl;
    for(int i=0; i<VKnots.Upper(); ++i)
     cout << VKnots.Value(i+1) << ", ";
    cout << endl;


    cout << "VMults: " << endl;
    for(int i=0; i<VMults.Upper(); ++i)
     cout << VMults.Value(i+1) << ", ";
    cout << endl;*/




    const Handle(Geom_BSplineSurface) surface = new Geom_BSplineSurface(
        Poles,        // const TColgp_Array2OfPnt &    Poles,
        UKnots,       // const TColStd_Array1OfReal &   UKnots,
        VKnots,       // const TColStd_Array1OfReal &   VKnots,
        UMults,       // const TColStd_Array1OfInteger &   UMults,
        VMults,       // const TColStd_Array1OfInteger &   VMults,
        3,            // const Standard_Integer   UDegree,
        3             // const Standard_Integer   VDegree,
        // const Standard_Boolean   UPeriodic = Standard_False,
        // const Standard_Boolean   VPeriodic = Standard_False
    );

    aAdaptorSurface.Load(surface);

	
	
}

Approximate::~Approximate()
{
}
/*! \brief Main Approximation Program.
*/
void Approximate::ApproxMain()
{
    std::cout << "BEGIN COMPUTE NURB" << std::endl;  
    ParameterBoundary();
    ParameterInnerPoints();

    ErrorApprox();
    std::cout << "END COMPUTE NURB" << std::endl;
}


/*! \brief Parameterizing the Boundary Points

 This function will first parameterized the boundary points. Using Routines::FindCorner() that it inherited
 to find the corner points, it will parameterized from side to side, using the ratio between the distance
 between current evaluated point and the it's next boundary to the total distance of the side.

*/
void Approximate::ParameterBoundary()
{
    std::cout << "Computing the parameter for the outer boundary..." << std::endl;

    ParameterMesh = LocalMesh;
    MeshCore::MeshAlgorithm algo(ParameterMesh);
    MeshCore::MeshPointIterator p_it(ParameterMesh);
    NumOfPoints = LocalMesh.CountPoints();
    NumOfOuterPoints = 0;
    double distance = 0;
    unsigned int a;
    std::vector<unsigned long> CornerIndex;
    std::vector<double> Pointdistance;
    std::vector<unsigned long>::iterator vec_it;
    std::vector <Base::Vector3f>::iterator pnt_it;

    //Get BoundariesIndex and BoundariesPoints and extract it to v_neighbour
    algo.GetMeshBorders(BoundariesIndex);
    algo.GetMeshBorders(BoundariesPoints);
    std::list< std::vector <unsigned long> >::iterator bInd = BoundariesIndex.begin();
    std::list< std::vector <Base::Vector3f> >::iterator bPnt = BoundariesPoints.begin();
    std::vector <unsigned long> v_neighbour = *bInd;
    std::vector <Base::Vector3f> v_pnts = *bPnt;

    Base::BoundBox3f BoundBox = ParameterMesh.GetBoundBox();

    //Resize the needed arrays
    NumOfOuterPoints = v_neighbour.size()-1;
    BoundariesX.resize(NumOfOuterPoints);
    BoundariesY.resize(NumOfOuterPoints);
    UnparamBoundariesX.resize(NumOfOuterPoints);
    UnparamBoundariesY.resize(NumOfOuterPoints);
    UnparamBoundariesZ.resize(NumOfOuterPoints);
    std::vector <unsigned long> nei_tmp(NumOfOuterPoints);
    std::vector <Base::Vector3f> pnts_tmp(NumOfOuterPoints);


    //Look for corner points
    std::cout << "Looking for corners..." << std::endl;
    CornerIndex.push_back(FindCorner(BoundBox.MinX,BoundBox.MinY,v_neighbour,v_pnts));  //FindCorner(Parameter1, Parameter2, neighbour_list, mesh)
    CornerIndex.push_back(FindCorner(BoundBox.MaxX,BoundBox.MinY,v_neighbour,v_pnts));
    CornerIndex.push_back(FindCorner(BoundBox.MaxX,BoundBox.MaxY,v_neighbour,v_pnts));
    CornerIndex.push_back(FindCorner(BoundBox.MinX,BoundBox.MaxY,v_neighbour,v_pnts));

    //Redo the list, start from (0,0), we are using the arrays in nei_tmp and pnts_tmp
    vec_it = find(v_neighbour.begin(),v_neighbour.end(),CornerIndex[0]);
    pnt_it = find(v_pnts.begin(), v_pnts.end(), LocalMesh.GetPoint(CornerIndex[0]));
    for (unsigned int i = 0; i < v_neighbour.size()-1; i++)
    {
        nei_tmp[i] = *vec_it;
        pnts_tmp[i] = *pnt_it;
        UnparamBoundariesX[i] = (*pnt_it).x;
        UnparamBoundariesY[i] = (*pnt_it).y;
        UnparamBoundariesZ[i] = (*pnt_it).z;
        ++vec_it;
        ++pnt_it;
        if (vec_it == v_neighbour.end()-1)
        {
            vec_it = v_neighbour.begin();
            pnt_it = v_pnts.begin();
            a = i;
        }


    }
    v_pnts.clear();
    v_pnts = pnts_tmp;

    std::cout << "Parametirizing..." << std::endl;
    //Parameter the boundaries

    //Parameter the _ Boundaries
    //v_handle = CornerIndex[0];
    double totaldistance = 0;
    unsigned int g = 0;
    unsigned int temp1 = g;
    unsigned int temp2 = g;
    do  //Get distance first
    {
        distance = sqrt(((v_pnts[g+1][0] - v_pnts[g][0])*(v_pnts[g+1][0] - v_pnts[g][0]))
                        + ((v_pnts[g+1][1] - v_pnts[g][1])*(v_pnts[g+1][1] - v_pnts[g][1])));
        Pointdistance.push_back(distance);
        totaldistance += distance;
        g++;
    }
    while ((vec_it = find(CornerIndex.begin(), CornerIndex.end(), nei_tmp[g])) == CornerIndex.end());

    //Secure current g-counter and reinitialize g-counter to initial value
    temp1 = g;
    g = temp2;

    //Parameter the first point, fill up the list we are using
    pnts_tmp[g][0] = 0.0f, pnts_tmp[g][1] = 0.0f;
    BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
    g++;
    for (unsigned int i = 0; i < Pointdistance.size() - 1;i++)
    {
        //Parametirizing
        //0 < X < 1, Y = 0.0, Z = don't care lalalala
        pnts_tmp[g][0] = (Pointdistance[i]/totaldistance)+pnts_tmp[g-1][0], pnts_tmp[g][1] = 0.0f;
        BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
        g++;
    }

    //Secure the counters and reinitialize the things we needed
    g = temp1;
    temp2 = g;
    Pointdistance.clear();
    totaldistance = 0;

    //Parameter the -| boundary
    do  //Get distance first
    {
        distance = sqrt(((v_pnts[g+1][0] - v_pnts[g][0])*(v_pnts[g+1][0] - v_pnts[g][0]))
                        + ((v_pnts[g+1][1] - v_pnts[g][1])*(v_pnts[g+1][1] - v_pnts[g][1])));
        Pointdistance.push_back(distance);
        totaldistance += distance;
        g++;
    }
    while ((vec_it = find(CornerIndex.begin(), CornerIndex.end(), nei_tmp[g])) == CornerIndex.end());

    //Secure current g-counter and reinitialize g-counter to initial value
    temp1 = g;
    g = temp2;
    pnts_tmp[g][0] = 1.0f, pnts_tmp[g][1] = 0.0f;
    BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
    g++;
    for (unsigned int i = 0; i < Pointdistance.size() - 1;i++)
    {
        //Parametirizing
        //X = 1, 0 < Y < 1, Z = don't care lalalala
        pnts_tmp[g][0] = 1.0f, pnts_tmp[g][1] = (Pointdistance[i]/totaldistance)+pnts_tmp[g-1][1];
        BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
        g++;
    }

    //Secure the counters and reinitialize´the things we needed
    g = temp1;
    temp2 = g;
    Pointdistance.clear();
    totaldistance = 0;

    //Parameter the - boundary
    do  //Get distance first
    {
        distance = sqrt(((v_pnts[g+1][0] - v_pnts[g][0])*(v_pnts[g+1][0] - v_pnts[g][0]))
                        + ((v_pnts[g+1][1] - v_pnts[g][1])*(v_pnts[g+1][1] - v_pnts[g][1])));
        Pointdistance.push_back(distance);
        totaldistance += distance;
        g++;
    }
    while ((vec_it = find(CornerIndex.begin(), CornerIndex.end(), nei_tmp[g])) == CornerIndex.end());

    //Secure current g-counter and reinitialize g-counter to initial value
    temp1 = g;
    g = temp2;
    pnts_tmp[g][0] = 1.0f, pnts_tmp[g][1] = 1.0f;
    float prev = pnts_tmp[g][0];
    BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
    g++;
    for (unsigned int i = 0; i < Pointdistance.size() - 1;i++)
    {
        //Parametirizing
        //0 < X < 1,Y = 1, Z = don't care lalalala
        pnts_tmp[g][0] = (Pointdistance[i]/totaldistance)+prev, pnts_tmp[g][1] = 1.0f;
        prev = pnts_tmp[g][0];
        pnts_tmp[g][0] = 2.0f - pnts_tmp[g][0];
        BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
        g++;
    }

    //Secure the counters and reinitialize´the things we needed
    g = temp1;
    temp2 = g;
    Pointdistance.clear();
    totaldistance = 0;

    //Parameter the |- boundary
    do  //Get distance first
    {
        distance = sqrt(((v_pnts[g+1][0] - v_pnts[g][0])*(v_pnts[g+1][0] - v_pnts[g][0]))
                        + ((v_pnts[g+1][1] - v_pnts[g][1])*(v_pnts[g+1][1] - v_pnts[g][1])));
        Pointdistance.push_back(distance);
        totaldistance += distance;
        g++;
        if (g+1 == v_pnts.size())
        {
            distance = sqrt(((v_pnts[0][0] - v_pnts[g][0])*(v_pnts[0][0] - v_pnts[g][0]))
                            + ((v_pnts[0][1] - v_pnts[g][1])*(v_pnts[0][1] - v_pnts[g][1])));
            Pointdistance.push_back(distance);
            totaldistance += distance;
            break;
        }
    }
    while ((vec_it = find(CornerIndex.begin(), CornerIndex.end(), nei_tmp[g])) == CornerIndex.end());

    //Secure current g-counter and reinitialize g-counter to initial value
    temp1 = g;
    g = temp2;
    pnts_tmp[g][0] = 0.0f, pnts_tmp[g][1] = 1.0f;
    prev = pnts_tmp[g][1];
    BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
    g++;
    for (unsigned int i = 0; i < Pointdistance.size() - 1;i++)
    {
        //Parametirizing
        //0 < X < 1, Y = 0, Z = don't care lalalala
        pnts_tmp[g][0] = 0.0, pnts_tmp[g][1] = (Pointdistance[i]/totaldistance)+prev;
        prev = pnts_tmp[g][1];
        pnts_tmp[g][1] = 2.0f - pnts_tmp[g][1];
        BoundariesX[g] = pnts_tmp[g][0], BoundariesY[g] = pnts_tmp[g][1];
        g++;
    }

    //Secure the counters and reinitialize´the things we needed
    g = temp1;
    temp2 = g;
    Pointdistance.clear();
    totaldistance = 0;

    ParameterX.resize(NumOfOuterPoints);
    ParameterY.resize(NumOfOuterPoints);
    int count = 0;
    NumOfInnerPoints = NumOfPoints - NumOfOuterPoints;
    mapper.resize(NumOfPoints);
    MeshCore::MeshPointIterator v_it(LocalMesh);
    a = 0;
    int b = 0;
    for (v_it.Begin(); !v_it.EndReached(); ++v_it) //For all points...
    {
        if ((vec_it = find(nei_tmp.begin(), nei_tmp.end(), v_it.Position())) != nei_tmp.end()) //...is it boundary?
        {
            //Yes
            ParameterX[count] = pnts_tmp[int(vec_it-nei_tmp.begin())][0];
            ParameterY[count] = pnts_tmp[int(vec_it-nei_tmp.begin())][1];
            mapper[v_it.Position()] = a+NumOfInnerPoints;
            a++, count++;
        }
        else
        {
            //No
            mapper[v_it.Position()] = b;
            b++;
        }
    }
}
/*! \brief Parameterizing the Inner Points

 This function will parameterize the inner points. Using the algorithm based on paper from
 Michael S. Floater, published in Computer Aided Design 14(1997) page 231 - 250,
 entitled Parametrization and smooth approximation of surface triangulation
*/
void Approximate::ParameterInnerPoints()
{
    std::cout << "Computing the parameter for the inner points..." << std::endl;
    MeshCore::MeshPointIterator v_it(LocalMesh);
    MeshCore::MeshAlgorithm algo(LocalMesh);
    MeshCore::MeshRefPointToPoints vv_it(LocalMesh);
    MeshCore::MeshRefPointToFacets vf_it(LocalMesh);
    std::set<unsigned long> PntNei;
    std::set<unsigned long> FacetNei;
    ublas::compressed_matrix<double> Lambda(NumOfInnerPoints, NumOfPoints);
    int count = 0;

    std::cout << "Prepping the Lambda" << std::endl;
    std::list< std::vector <unsigned long> >::iterator bInd = BoundariesIndex.begin();
    std::vector <unsigned long> neiIndexes = *bInd;
    std::vector <unsigned long>::iterator vec_it;
    for (v_it.Begin(); !v_it.EndReached(); ++v_it)
    {
        if ((vec_it = find(neiIndexes.begin(), neiIndexes.end(), v_it.Position())) == neiIndexes.end())
        {

            std::vector<Base::Vector3f> NeiPnts;
            std::vector<unsigned long> nei;
            std::vector<unsigned int>::iterator nei_it;
            PntNei = vv_it[v_it.Position()];
            FacetNei = vf_it[v_it.Position()];
            ReorderNeighbourList(PntNei,FacetNei,nei,v_it.Position());
            std::vector<double> Angle;
            std::vector<double> Magnitude;
            Base::Vector3f CurrentPoint(*v_it);
            double TotAngle = 0;
            unsigned int NumOfNeighbour = PntNei.size();
            unsigned int i = 0;
            //Angle and magnitude calculations for projection.
            //With respect to CurrentPoint
            while (i < NumOfNeighbour)
            {
                if (i+1 != NumOfNeighbour)
                {
                    Base::Vector3f CurrentNeighbour(LocalMesh.GetPoint(nei[i]));
                    Base::Vector3f NextNeighbour(LocalMesh.GetPoint(nei[i+1]));
                    double ang = CalcAngle(CurrentNeighbour, CurrentPoint, NextNeighbour);
                    double magn = sqrt((CurrentNeighbour - CurrentPoint) * (CurrentNeighbour - CurrentPoint));
                    Angle.push_back(ang);
                    Magnitude.push_back(magn);
                    TotAngle += ang;
                    i++;
                }
                else
                {
                    Base::Vector3f CurrentNeighbour(LocalMesh.GetPoint(nei[i]));
                    Base::Vector3f NextNeighbour(LocalMesh.GetPoint(nei[0]));
                    double ang = CalcAngle(CurrentNeighbour, CurrentPoint, NextNeighbour);
                    double magn = sqrt((CurrentNeighbour - CurrentPoint) * (CurrentNeighbour - CurrentPoint));
                    Angle.push_back(ang);
                    Magnitude.push_back(magn);
                    TotAngle += ang;
                    i++;
                }

            }
            //Projection
            //Current point is (0,0), First neighbour is on the X-Axis (y = 0), all other are projected
            //depending on angle and magnitude
            Base::Vector3f CurrentNeighbour(Magnitude[0],0.0,0.0);
            NeiPnts.push_back(CurrentNeighbour);
            double alpha = 0;
            unsigned int k = 0;
            for (unsigned int i = 1; i < NumOfNeighbour; i++)
            {
                alpha = alpha + ((2 * D_PI * Angle[i-1]) / TotAngle);
                double x_pnt = Magnitude[i] * cos(alpha), y_pnt = Magnitude[i] * sin(alpha);
                Base::Vector3f NewPoint(x_pnt,y_pnt,0.0);
                NeiPnts.push_back(NewPoint);
                ++k;

            }
            k = 0;
            //Preparing the needed matrix for iterating
            std::vector< std::vector<double> > Mu(NumOfNeighbour, std::vector<double>(NumOfNeighbour, 0.0));  //Mu Matrix
            std::vector< std::vector<double> > MMatrix(2, std::vector<double>(2,0.0));  //for the solver
            std::vector<double> bMatrix(2,0.0);
            std::vector<double> lMatrix(2,0.0);
            if (NumOfNeighbour > 3) //if NumOfNeighbour > 3...
            {
                for (k = 0; k < NumOfNeighbour;k++)  //...for all neighbours...
                {
                    for (unsigned int l = k+1; l < NumOfNeighbour+k; l++) //...another iterator iterate through other neighbour
                    {
                        MMatrix[0][0] = NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][0] -
                                        NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][0];
                        MMatrix[1][0] =  NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][1] -
                                         NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][1];
                        MMatrix[0][1] = NeiPnts[k][0];
                        MMatrix[1][1] = NeiPnts[k][1];

                        bMatrix[0] = -NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][0];
                        bMatrix[1] = -NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][1];

                        if (det2(MMatrix) != 0)
                        {
                            CramerSolve(MMatrix, bMatrix, lMatrix);  //Solve it
                            if (lMatrix[0] <= 1.00001f && lMatrix[0] >= 0.0f && lMatrix[1] > 0.0f) //Condition for a solution
                            {
                                unsigned int r = (unsigned int)fmod((double)l-1,(double)NumOfNeighbour);
                                MMatrix[0][0] = NeiPnts[k][0] -
                                                NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][0];
                                MMatrix[1][0] = NeiPnts[k][1] -
                                                NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][1];
                                MMatrix[0][1] = NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][0] -
                                                NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][0];
                                MMatrix[1][1] = NeiPnts[(unsigned int)fmod((double)l-1,(double)NumOfNeighbour)][1] -
                                                NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][1];

                                bMatrix[0] = -NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][0];
                                bMatrix[1] = -NeiPnts[(unsigned int)fmod((double)l,(double)NumOfNeighbour)][1];
                                CramerSolve(MMatrix, bMatrix, lMatrix);  //Solve it

                                Mu[k][k] = fabs(lMatrix[0]);  //Solution found, fill up the Mu
                                Mu[r][k] = fabs(lMatrix[1]);
                                if ((unsigned int)fmod((double)r,(double)NumOfNeighbour)+1 == NumOfNeighbour)
                                    Mu[0][k] = 1 -lMatrix[0] - lMatrix[1];
                                else
                                    Mu[(unsigned int)fmod((double)r,(double)NumOfNeighbour)+1][k] = 1 -lMatrix[0] - lMatrix[1];
                                break;

                            }
                        }
                    }
                }
                //Fill in the lambda
                /*for(int j = 0; j < NumOfNeighbour; j++)  //quick checker, if any bug comes out, one of
                {             //possible bugspawn is here
                 double sum = 0;
                 for(int k = 0; k < NumOfNeighbour; k++)
                  sum += Mu[k][j];

                 if(sum < 0.9999 || sum > 1.0001)
                 {
                  std::cout << "Mu is not correct.\nSum: " << sum <<
                  "\nCount: " << count << "\nNumOfNeighbour: " << NumOfNeighbour << std::endl;
                  getchar();
                 }

                }*/
                for (unsigned int j = 0; j < NumOfNeighbour; j++)
                {
                    double sum = 0;
                    for (unsigned int k = 0; k < NumOfNeighbour; k++)
                        sum += Mu[j][k];

                    Lambda(count,mapper[nei[j]]) = sum/NumOfNeighbour;
                }
                count++;
            }
            else if (NumOfNeighbour == 3) //If NumOfNeighbour == 3...
            {
                Base::Vector3f Point1(NeiPnts[0]);
                Base::Vector3f Point2(NeiPnts[1]);
                Base::Vector3f Point3(NeiPnts[2]);
                Base::Vector3f Zeroes(0,0,0);
                double A = AreaTriangle(Point1,Point2,Point3);

                Lambda(count,mapper[nei[0]]) =
                    AreaTriangle(Zeroes,Point2,Point3) / A;

                Lambda(count,mapper[nei[1]]) =
                    AreaTriangle(Point1,Zeroes,Point3) / A;

                Lambda(count,mapper[nei[2]]) =
                    AreaTriangle(Point1,Point2,Zeroes) / A;

                count++;
            }
            else   //Can an inside point have less than 3 neighbours...?
            {
                throw Base::RuntimeError("Something's wrong here. Less than 3 Neighbour");
            }
        }
    }

    //Now, we split the lambda matrix
    //Using the compressed matrix class from boost
    std::cout << "Splitting the lambdas..." << std::endl;
    ublas::compressed_matrix<double, ublas::column_major, 0, ublas::unbounded_array<int>, ublas::unbounded_array<double> >
    UpperTerm(NumOfInnerPoints, NumOfInnerPoints);
    ublas::compressed_matrix<double, ublas::column_major> OutLambda(NumOfInnerPoints, NumOfOuterPoints);
    //We need to extract this columnwise, and (i,j) is a row-major style of numbering...
    //Also, later we need to I - Upperterm, I := IdentityMatrix, this will also be done in this step
    for (int i = 0; i < NumOfInnerPoints; i++)
    {
        for (int j = 0; j < NumOfInnerPoints; j++)
        {
            if (Lambda(i,j) != 0)
                UpperTerm(i,j) = -Lambda(i,j);
            else if (i == j)
                UpperTerm(i,j) = 1.0 - Lambda(i,j);
        }
    }

    for (int j = NumOfInnerPoints, k = 0; j < NumOfPoints; j++, k++)
    {
        for (int i = 0; i < NumOfInnerPoints; i++)
        {
            if (Lambda(i,j) != 0)
                OutLambda(i,k) = Lambda(i,j);
        }
    }

    //Result Storage
    ublas::vector<double> xResult(NumOfInnerPoints);
    ublas::vector<double> MultResult(NumOfInnerPoints);
    ublas::vector<double> yResult(NumOfInnerPoints);

	ofstream anOutputFile;
	anOutputFile.open("c:/approx_param_log.txt");
	anOutputFile << 1 << endl;
    //Solving the X Term
    std::cout << "Solving the X Term..." << std::endl;
    //atlas::gemm and atlas::gemv can't work with sparse matrices it seems, therefore using axpy_prod
    ublas::axpy_prod(OutLambda, ParameterX, MultResult);
	anOutputFile << 2 << endl;
    bindings::umfpack::umf_solve (UpperTerm, xResult,MultResult );  //umfpack to solve sparse matrices equations
	anOutputFile << 3 << endl;
    //Solving the Y Term
    std::cout << "Solving the Y Term..." << std::endl;
    ublas::axpy_prod(OutLambda, ParameterY, MultResult);
	anOutputFile << 4 << endl;
    bindings::umfpack::umf_solve (UpperTerm, yResult, MultResult);
	anOutputFile << 5 << endl;
    std::cout << "Replacing the results.." << std::endl;

	

    //Another counter, temporary storage for old ParameterX and ParameterY, and resizing to include ALL points now
    unsigned int s = 0;
    unsigned int b = 0;

    ublas::vector<double> tempox = ParameterX;
    ublas::vector<double> tempoy = ParameterY;
    ParameterX.clear(), ParameterX.resize(NumOfPoints);
    ParameterY.clear(), ParameterY.resize(NumOfPoints);
    UnparamX.resize(NumOfPoints), UnparamY.resize(NumOfPoints),UnparamZ.resize(NumOfPoints);
    //Reextract the boundaries list
	anOutputFile << 6 << endl;
    std::vector <unsigned long> boundarieslist = *bInd;

	anOutputFile << 7 << endl;



	/**************************/
	MeshParam = LocalMesh;
	MeshCore::MeshPointArray pntArr= MeshParam.GetPoints();
	MeshCore::MeshFacetArray fctArr= MeshParam.GetFacets();
	/**************************/
    
	
	for (v_it.Begin(); !v_it.EndReached(); ++v_it)
    {
		anOutputFile << 0 << endl;
        //Inner Points goes into the beginning of the list
        if ((vec_it = find(boundarieslist.begin(), boundarieslist.end(), v_it.Position())) == boundarieslist.end())
        {
            ParameterX[s] = xResult[s];
            ParameterY[s] = yResult[s];

			pntArr[v_it.Position()].x = ParameterX[s];
			pntArr[v_it.Position()].y = ParameterY[s];
			pntArr[v_it.Position()].z = 0;

            UnparamX[s] = LocalMesh.GetPoint(v_it.Position())[0];
            UnparamY[s] = LocalMesh.GetPoint(v_it.Position())[1];
            UnparamZ[s] = LocalMesh.GetPoint(v_it.Position())[2];
            s++;
        }
        //Boundaries goes into the end of the list
        else
        {
            ParameterX[NumOfInnerPoints+b] = tempox[b];
            ParameterY[NumOfInnerPoints+b] = tempoy[b];

			pntArr[v_it.Position()].x = ParameterX[NumOfInnerPoints+b];
			pntArr[v_it.Position()].y = ParameterY[NumOfInnerPoints+b];
			pntArr[v_it.Position()].z = 0;

            UnparamX[NumOfInnerPoints+b] = LocalMesh.GetPoint(v_it.Position())[0];
            UnparamY[NumOfInnerPoints+b] = LocalMesh.GetPoint(v_it.Position())[1];
            UnparamZ[NumOfInnerPoints+b] = LocalMesh.GetPoint(v_it.Position())[2];
            b++;
        }
    }

	anOutputFile << 8 << endl;
	MeshParam.Assign(pntArr,fctArr);
    anOutputFile << 9 << endl;

    std::cout << "DONE" << std::endl;
    std::cout << "Information about the meshes:-" << std::endl;
    std::cout << "Number of Points: " << NumOfPoints << std::endl;
    std::cout << "Number of Inner Points: " << NumOfInnerPoints << std::endl;
    std::cout << "Number of Outer Points: " << NumOfOuterPoints << std::endl;
    //clear the mesh, we will continue working with the lists we have made
    ParameterMesh.Clear();
	anOutputFile << 10 << endl;
    LocalMesh.Clear();
	anOutputFile << 11 << endl;
	anOutputFile.close();

}


/*! \brief Main Error Approximations routine

 This function will be ended once max_err < tolerance
*/
void Approximate::ErrorApprox()
{
 	ofstream anOutputFile;
	anOutputFile.open("c:/approx_param_log.txt");

    anOutputFile << "Begin constructing NURB for first pass" << std::endl;
    double max_err = 0;
    bool ErrThere = true;  //for breaking the while
    int h = 0;
	int cnt = 0;
    double av = 0, c2 = 0;
	double lam;
    
	anOutputFile << "Constructing" << std::endl;
    ublas::matrix<double> C_Temp(NumOfPoints,3);
    anOutputFile << "C_Temp successfully constructed" << std::endl;
    //Time saving... C_Temp matrix is constant for all time

	anOutputFile << "number of points: " << NumOfPoints << std::endl;
    std::vector <double> err_w(NumOfPoints);
	anOutputFile << "checkpoint 1 " << std::endl;
    for (int i=1; i<NumOfPoints; i++)
        err_w[i] = 1;

	anOutputFile << "checkpoint 2 " << std::endl;
	int g = 1;

    while(ErrThere)
    {
		anOutputFile << "checkpoint 3 " << std::endl;
		anOutputFile << "checkpoint 3 " << std::endl;
		anOutputFile << "size(B_Matrix): " <<  NumOfPoints << " x " << (MainNurb.MaxU+1)*(MainNurb.MaxV+1) << std::endl;
        ublas::matrix<double> B_Matrix(NumOfPoints,(MainNurb.MaxU+1)*(MainNurb.MaxV+1));
		anOutputFile << "********************************" << endl;
        anOutputFile << "B_Matrix successfully constructed" << std::endl;
        anOutputFile << "Preparing B-Matrix..." << std::endl;

        std::vector<double> N_u(MainNurb.MaxU+1, 0.0);
        std::vector<double> N_v(MainNurb.MaxV+1, 0.0);
        std::vector<double> TempU(MainNurb.DegreeU+1, 0.0);
        std::vector<double> TempV(MainNurb.DegreeV+1, 0.0);
        std::vector<double> swapDegreeU(MainNurb.DegreeU+1, 0.0);
        std::vector<double> swapDegreeV(MainNurb.DegreeV+1, 0.0);
        std::vector<double> swapV(MainNurb.MaxV+1, 0.0);
        std::vector<double> swapU(MainNurb.MaxU+1, 0.0);

        for (int i = 0; i < NumOfPoints; i++)
        {
            std::vector<double> N_u(MainNurb.MaxU+1, 0.0);
            std::vector<double> N_v(MainNurb.MaxV+1, 0.0);
            std::vector<double> TempU(MainNurb.DegreeU+1, 0.0);
            std::vector<double> TempV(MainNurb.DegreeV+1, 0.0);

            int j1 = FindSpan(MainNurb.MaxU, MainNurb.DegreeU, ParameterX[i], MainNurb.KnotU);
            int j2 = FindSpan(MainNurb.MaxV, MainNurb.DegreeV, ParameterY[i], MainNurb.KnotV);


            Basisfun(j1,ParameterX[i], MainNurb.DegreeU, MainNurb.KnotU, TempU);
            Basisfun(j2,ParameterY[i], MainNurb.DegreeV, MainNurb.KnotV, TempV);

            for (int k = j1 - MainNurb.DegreeU, s = 0; k < j1+1; k++, s++)
                  N_u[k] = TempU[s];
            for (int k = j2 - MainNurb.DegreeV, s = 0; k < j2+1; k++, s++)
                N_v[k] = TempV[s];

            for (int j = 0; j <= MainNurb.MaxV; j++)
            {
                for (int h = 0; h <= MainNurb.MaxU; h++)
                {
                    //double result = N_u[h] * N_v[j];
                    B_Matrix(i,(j*(MainNurb.MaxU+1))+h) =  sqrt(err_w[i]) * N_u[h] * N_v[j];
                }
            }
        }

        for (unsigned int i = 0; i < UnparamX.size(); ++i)
        {
            C_Temp(i,0) = sqrt(err_w[i])*UnparamX[i];
            C_Temp(i,1) = sqrt(err_w[i])*UnparamY[i];
            C_Temp(i,2) = sqrt(err_w[i])*UnparamZ[i];
        }

        ublas::matrix<double> G_Matrix((MainNurb.MaxU+1)*(MainNurb.MaxV+1),(MainNurb.MaxU+1)*(MainNurb.MaxV+1));
        ublas::matrix<double> C_Tempo((MainNurb.MaxU+1)*(MainNurb.MaxV+1),3);
        atlas::gemm(CblasTrans, CblasNoTrans, 1.0, B_Matrix,C_Temp,0.0,C_Tempo);
        atlas::gemm(CblasTrans,CblasNoTrans,1.0,B_Matrix,B_Matrix,0.0,G_Matrix);  //Multiplication with Boost bindings
        //to ATLAS's bindings to LAPACK
        B_Matrix.resize(1,1,false);
        B_Matrix.clear();

        anOutputFile << "Preparing the A_Matrix" << std::endl;
        //ublas::matrix<double> G_Matrix((MainNurb.MaxU+1)*(MainNurb.MaxV+1),(MainNurb.MaxU+1)*(MainNurb.MaxV+1));
        ublas::compressed_matrix<double, ublas::column_major, 0, ublas::unbounded_array<int>, ublas::unbounded_array<double> >
        A_Matrix((MainNurb.MaxU+1)*(MainNurb.MaxV+1),(MainNurb.MaxU+1)*(MainNurb.MaxV+1));
        anOutputFile << "Multiply..." << std::endl;

        anOutputFile << "Euclidean Norm" << std::endl;
        A_Matrix = G_Matrix;

		if(cnt == 0)
			lam = 10*ublas::norm_frobenius(G_Matrix);
		else
			lam /= 2;
		
        G_Matrix.resize(1,1, false);
        G_Matrix.clear();
        ublas::compressed_matrix<double> E_Matrix((MainNurb.MaxU+1)*(MainNurb.MaxV+1), (MainNurb.MaxU+1)*(MainNurb.MaxV+1));
        anOutputFile << "E_Matrix successfully constructed" << std::endl;
        anOutputFile << "Smoothing..." << std::endl;
        eFair2(E_Matrix);
		
		if(cnt == 0){
			lam = lam / ublas::norm_frobenius(E_Matrix);
		}

        ++cnt;

		anOutputFile << "lam: " << lam << std::endl;
        A_Matrix = A_Matrix + (E_Matrix*lam);
        
		//Destroying the unneeded matrix
        E_Matrix.resize(1,1,false);
        E_Matrix.clear();
        anOutputFile << "Preparing the C_Matrix" << std::endl;
        std::vector< std::vector<double> > Solver;
        std::vector<double> TempoSolv((MainNurb.MaxU+1)*(MainNurb.MaxV+1));
        std::vector<double> TempoB;

        anOutputFile << "Solving" << std::endl;
        for (unsigned int i = 0; i < 3; i++) //Since umfpack can only solve Ax = B, where x and B are vectors
                                             //instead of matrices...
        {
            //We will solve it column wise
            for (int j = 0; j < (MainNurb.MaxU+1)*(MainNurb.MaxV+1); j++)
                TempoB.push_back(C_Tempo(j,i));  //push back a column

            umfpack::umf_solve(A_Matrix,TempoSolv,TempoB);  //solve
            Solver.push_back(TempoSolv);  //push back the solution
            TempoB.clear();
        }
        MainNurb.CntrlArray.clear();
        anOutputFile << "Loading the Control Points" << std::endl;
        for (int i = 0; i < (MainNurb.MaxU+1)*(MainNurb.MaxV+1); i++) //now load the control points
        {
            MainNurb.CntrlArray.push_back(Solver[0][i]); //X
            MainNurb.CntrlArray.push_back(Solver[1][i]); //Y
            MainNurb.CntrlArray.push_back(Solver[2][i]); //Z
        }

		std::vector<double> ContrArr = MainNurb.CntrlArray;

        anOutputFile << "Cntrl Count" << std::endl;
        anOutputFile << "U: " << MainNurb.MaxU + 1 << std::endl;
        anOutputFile << "V: " << MainNurb.MaxV + 1 << std::endl;

        //ComputeError(h, 0.1, 0.1, max_err,av, c2, err_w);

        //anOutputFile << "Maximum error is " << max_err <<std::endl;
        //
        //anOutputFile << "Average points in error: " << c2 << std::endl;


		//Reparameterize & error-measurement
		anOutputFile << "Reparameterize ..." <<  std::endl;
        av = Reparam();  
		anOutputFile << "End Reparameterize" <<  std::endl;
		anOutputFile << "Average error: " << av << std::endl;
        
		if (av <= tolerance)   //Error still bigger than our tolerance?
			ErrThere = false;
    }

	anOutputFile.close();
}

/*! \brief Smoothing
*/
void Approximate::eFair2(ublas::compressed_matrix<double> &E_Matrix)
{
	ofstream anOutputFile;
	anOutputFile.open("c:/approx_param_log_efair.txt");

    int precision = 100;
    std::vector<double> U(precision+1, 0);
    std::vector<double> V(precision+1, 0);
    for (int i = 1; i <= precision; i++) //Load U and V vectors uniformly, according to precision
    {
        U[i] = (1.0/(double)precision) + U[i-1];
        V[i] = (1.0/(double)precision) + V[i-1];
    }
    U[precision] = 1.0;
    V[precision] = 1.0;
    precision = precision + 1;

    int mu = MainNurb.MaxKnotU;
    int mv = MainNurb.MaxKnotV;
    int k = mu-MainNurb.MaxU-2;
    int l = mv-MainNurb.MaxV-2;
    //Preparing the needed matrices
    std::vector< std::vector<double> > N_u0(precision, std::vector<double>(mu-1-k,0.0));
    std::vector< std::vector<double> > N_u1(precision, std::vector<double>(mu-1-k,0.0));
    std::vector< std::vector<double> > N_u2(precision, std::vector<double>(mu-1-k,0.0));
    std::vector< std::vector<double> > N_v0(precision, std::vector<double>(mu-1-l,0.0));
    std::vector< std::vector<double> > N_v1(precision, std::vector<double>(mu-1-l,0.0));
    std::vector< std::vector<double> > N_v2(precision, std::vector<double>(mu-1-l,0.0));

    std::vector<double> A_1(precision,0.0);
    std::vector<double> B_1(precision,0.0);
    std::vector<double> C_1(precision,0.0);
    std::vector<double> A_2(precision,0.0);
    std::vector<double> B_2(precision,0.0);
    std::vector<double> C_2(precision,0.0);

    //Filling up the first six matrices
    for (int i = 0; i < precision; i++)
    {
        std::vector< std::vector<double> > dersU(2+1, std::vector<double>(k+1));
        std::vector< std::vector<double> > dersV(2+1, std::vector<double>(k+1));
        int s = FindSpan(MainNurb.MaxU, k, U[i], MainNurb.KnotU);
        DersBasisFuns(s, U[i], k, 2, MainNurb.KnotU, dersU);
        for (int a = s-k, b = 0; a < s+1; a++, b++)
        {
            N_u0[i][a] = dersU[0][b];
            N_u1[i][a] = dersU[1][b];
            N_u2[i][a] = dersU[2][b];
        }

        s = FindSpan(MainNurb.MaxV, l, V[i], MainNurb.KnotV);
        DersBasisFuns(s, V[i], l, 2, MainNurb.KnotV, dersV);
        for (int a = s-l, b = 0; a < s+1; a++, b++)
        {
            N_v0[i][a] = dersV[0][b];
            N_v1[i][a] = dersV[1][b];
            N_v2[i][a] = dersV[2][b];
        }

    }

    double A,B,C; //Needed Variables for the Trapezoid-Integration
    //Now lets fill up the E
    for (int a = 0; a < MainNurb.MaxV+1; a++)
    {

        //anOutputFile << "\r" << ceil(100.0*((double) a/(double) MainNurb.MaxV)) << "%" << " ";
        for (int b = 0; b < MainNurb.MaxU+1; b++)
        {
            for (int c = 0; c < MainNurb.MaxV+1; c++)
            {

                for (int d = 0; d < MainNurb.MaxU+1; d++)
                {
                    for (int w = 0; w < precision; w++)  //Fill up the last 6 Matrices from the first matrix
                    {
                        A_1[w] = (N_u2[w][b]*N_u2[w][d]);
                        A_2[w] = (N_v0[w][a]*N_v0[w][c]);

                        B_1[w] = (N_u1[w][b]*N_u1[w][d]);
                        B_2[w] = (N_v1[w][a]*N_v1[w][c]);

                        C_1[w] = (N_u0[w][b]*N_u0[w][d]);
                        C_2[w] = (N_v2[w][a]*N_v2[w][c]);
                    }



                    //SehnenTrapezRegel
                    A = TrapezoidIntergration(U, A_1);
                    A *= TrapezoidIntergration(U, A_2);

                    B = TrapezoidIntergration(U, B_1);
                    B *= TrapezoidIntergration(U, B_2);

                    C = TrapezoidIntergration(U, C_1);
                    C *= TrapezoidIntergration(U, C_2);

                    //result = A + 2*B + C;
                    E_Matrix((a*(MainNurb.MaxU+1))+b,(c*(MainNurb.MaxV+1))+d) = A + 2*B + C;
                }
            }
        }
    }
    anOutputFile << std::endl;
	anOutputFile.close();
}

/*! \brief This function will compute the current error
*/
void Approximate::ComputeError(int &h, double eps_1, double eps_2, double &max_error,
                               double &av, double &c2, std::vector <double> &err_w)
{
    std::cout << "Computing Error..." << std::endl;
    av = 0;
    c2 = 0;
    max_error = 0;
    for (int i = 0; i < NumOfPoints; i++)  //For all points
    {

        std::vector<NURBS> DerivNurb;
        std::vector<NURBS> DerivUNurb;
        std::vector<NURBS> DerivVNurb;
        std::vector<std::vector<double> > Jac;
        std::vector<double> CurPoint;
        CurPoint.push_back(ParameterX[i]);
        CurPoint.push_back(ParameterY[i]);
        std::vector<double> V(2,0.0);
        V[0] = CurPoint[0];
        V[1] = CurPoint[1];
        unsigned int j = 0;
        PointNrbDerivate(MainNurb, DerivNurb);
        PointNrbDerivate(DerivNurb[0], DerivUNurb);
        PointNrbDerivate(DerivNurb[1], DerivVNurb);
        int c = 0;
        std::vector<double> error;
        std::vector<double> EvalPoint;

        NrbDEval(MainNurb, DerivNurb, CurPoint, EvalPoint, Jac);
        EvalPoint[0] = EvalPoint[0] - UnparamX[i];
        EvalPoint[1] = EvalPoint[1] - UnparamY[i];
        EvalPoint[2] = EvalPoint[2] - UnparamZ[i];
        ublas::matrix<double> EvalMat(1, EvalPoint.size());
        for (j = 0; j < 3; j++)
            EvalMat(0,j) = EvalPoint[j];
        for (j = 0; j < 2; j++)
        {
            ublas::matrix<double> Holder(1, 1);
            ublas::matrix<double> JacPoint(Jac[j].size(), 1);
            for (unsigned int k = 0; k < Jac[j].size(); k++)
                JacPoint(k,0) = Jac[j][k];

            atlas::gemm(CblasTrans, CblasTrans, 1.0, JacPoint,EvalMat,0.0,Holder);
            double lam = ublas::norm_frobenius(JacPoint) * ublas::norm_frobenius(EvalMat);
            if (lam == 0)
                throw "Division by Zero in ComputeError function";
            lam = fabs(Holder(0,0) / lam);
            error.push_back(lam);
        }
        //recheck the... thingy here...
        while (((norm_frobenius(EvalMat) > eps_1) && ((error[0] > eps_2) || (error[1] > eps_2))) && c < 1000) //If somehow the eps is not reached...
        {
            c += 1;
            std::vector<double> p_uu;
            std::vector<double> p_uv;
            std::vector<double> p_vu;
            std::vector<double> p_vv;
            ublas::matrix<double> P_UU(3, 1);
            ublas::matrix<double> P_UV(3, 1);
            ublas::matrix<double> P_VU(3, 1);
            ublas::matrix<double> P_VV(3, 1);
            ublas::matrix<double> JacPoint1(Jac[0].size(), 1);
            ublas::matrix<double> JacPoint2(Jac[0].size(), 1);
            PointNrbEval(p_uu,CurPoint,DerivUNurb[0]);
            PointNrbEval(p_uv,CurPoint,DerivUNurb[1]);
            PointNrbEval(p_vu,CurPoint,DerivVNurb[0]);
            PointNrbEval(p_vv,CurPoint,DerivVNurb[1]);

            //Prepping the needed matrix

            for (unsigned int a = 0; a < Jac[0].size();a++)
                JacPoint1(a,0) = Jac[0][a];
            for (unsigned int a = 0; a < Jac[1].size();a++)
                JacPoint2(a,0) = Jac[1][a];
            for (unsigned int a = 0; a < 3; a++)
            {
                P_UU(a,0) = p_uu[a];
                P_UV(a,0) = p_uv[a];
                P_VU(a,0) = p_vu[a];
                P_VV(a,0) = p_vv[a];
            }

            std::vector< std::vector<double> > J(2, std::vector<double>(2,0.0));
            std::vector<double> K(2,0.0);
            //Newton iterate
            //J[0][0]
            ublas::matrix<double> MultResult(1,1);
            atlas::gemm(CblasTrans, CblasNoTrans, 1.0, JacPoint1,JacPoint1,0.0,MultResult);
            J[0][0] += MultResult(0,0);
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,P_UU,0.0,MultResult);
            J[0][0] += MultResult(0,0);

            //J[0][1]
            atlas::gemm(CblasTrans, CblasNoTrans, 1.0, JacPoint1,JacPoint2,0.0,MultResult);
            J[0][1] += MultResult(0,0);
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,P_UV,0.0,MultResult);
            J[0][1] += MultResult(0,0);

            //J[1][0]
            atlas::gemm(CblasTrans, CblasNoTrans, 1.0, JacPoint1,JacPoint2,0.0,MultResult);
            J[1][0] += MultResult(0,0);
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,P_VU,0.0,MultResult);
            J[1][0] += MultResult(0,0);

            //J[1][1]
            atlas::gemm(CblasTrans, CblasNoTrans, 1.0, JacPoint2,JacPoint2,0.0,MultResult);
            J[1][1] += MultResult(0,0);
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,P_VV,0.0,MultResult);
            J[1][1] += MultResult(0,0);

            //K[0]
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,JacPoint1,0.0,MultResult);
            K[0] = (J[0][0]*V[0]) + (J[0][1]*V[1]) - MultResult(0,0);

            //K[1]
            atlas::gemm(CblasNoTrans, CblasNoTrans, 1.0, EvalMat,JacPoint2,0.0,MultResult);
            K[1] = (J[1][0]*V[0]) + (J[1][1]*V[1]) - MultResult(0,0);

            CramerSolve(J,K,V);

            if (V[0] < 0)
                V[0] = 0;
            else if (V[0] > 1)
                V[0] = 1;

            if (V[1] < 0)
                V[1] = 0;
            else if (V[1] > 1)
                V[1] = 1;

            if (c == 500)
            {
                V[0] = 0.5;
                V[1] = 0.5;
            }
            //Reevaluate
            error.clear();
            CurPoint[0] = V[0];
            CurPoint[1] = V[1];
            EvalPoint.clear();
            Jac.clear();
            NrbDEval(MainNurb, DerivNurb, CurPoint, EvalPoint, Jac);
            EvalPoint[0] = EvalPoint[0] - UnparamX[i];
            EvalPoint[1] = EvalPoint[1] - UnparamY[i];
            EvalPoint[2] = EvalPoint[2] - UnparamZ[i];
            for (j = 0; j < 3; j++)
                EvalMat(0,j) = EvalPoint[j];
            for (j = 0; j < 2; j++)
            {
                ublas::matrix<double> Holder(1, 1);
                ublas::matrix<double> JacPoint(Jac[j].size(), 1);
                for (unsigned int k = 0; k < Jac[j].size(); k++)
                    JacPoint(k,0) = Jac[j][k];

                atlas::gemm(CblasTrans, CblasTrans, 1.0, JacPoint,EvalMat,0.0,Holder);
                double lam = ublas::norm_frobenius(JacPoint) * ublas::norm_frobenius(EvalMat);
                if (lam == 0)
                    throw "Division by Zero in ComputeError function";
                else
                    lam = fabs(Holder(0,0) / lam);
                error.push_back(lam);
            }
        }
        ParameterX[i] = V[0];
        ParameterY[i] = V[1];
        av += norm_frobenius(EvalMat);  //Average Error

        err_w[i] = norm_frobenius(EvalMat);

        if (norm_frobenius(EvalMat) > max_error && c < 1000)
        {
            max_error = norm_frobenius(EvalMat);
            h = i;
            //if(max_error > (3*tolerance))
            // break;

        }
        if (norm_frobenius(EvalMat) > tolerance)
            c2++;    //% of point's error still above tolerance
    }
    c2 /= NumOfPoints;
    av /= NumOfPoints;

    std::cout << " DONE" << std::endl;
}

/*! \brief Reparameterization after error computation
*/
double Approximate::Reparam()
{
	MeshCore::MeshPointArray pntArr = MeshParam.GetPoints();
	MeshCore::MeshFacetArray fctArr = MeshParam.GetFacets();

	double error = 0.0;

    std::cout << "Reparameterization...";
    for (int i = 0; i < NumOfPoints; i++)
    {
        std::vector<NURBS> DerivNurb;
        std::vector<double> p;
        std::vector<double> EvalPoint;
        std::vector< std::vector<double> > T;
        std::vector< std::vector<double> > A(2,std::vector<double>(2,0.0));
        std::vector<double> bt(2,0.0);

        p.push_back(ParameterX[i]);
        p.push_back(ParameterY[i]);
        PointNrbDerivate(MainNurb, DerivNurb);
        NrbDEval(MainNurb, DerivNurb, p, EvalPoint, T);
        
		EvalPoint[0] = UnparamX[i] - EvalPoint[0];
        EvalPoint[1] = UnparamY[i] - EvalPoint[1];
        EvalPoint[2] = UnparamZ[i] - EvalPoint[2];

		error += sqrt(EvalPoint[0]*EvalPoint[0] + EvalPoint[1]*EvalPoint[1] + EvalPoint[2]*EvalPoint[2]);

        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                std::vector<double> JacHolder1(3, 0.0);
                std::vector<double> JacHolder2(3, 0.0);
                std::vector<double> Result(1, 0.0);
                if (j == 0 && k == 0)
                {
                    for (int l = 0; l < 3; l++)
                    {
                        JacHolder1[l] = T[0][l];
                        JacHolder2[l] = T[0][l];
                    }
                    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,1,1,3,1.0,
                                &JacHolder1[0],3,&JacHolder2[0],1,0.0,&Result[0], 1);

                    A[j][k] = Result[0];
                }
                else if (j == 1 && k == 1)
                {
                    for (int l = 0; l < 3; l++)
                    {
                        JacHolder1[l] = T[1][l];
                        JacHolder2[l] = T[1][l];
                    }
                    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,1,1,3,1.0,
                                &JacHolder1[0],3,&JacHolder2[0],1,0.0,&Result[0], 1);

                    A[j][k] = Result[0];
                }
                else
                {
                    for (int l = 0; l < 3; l++)
                    {
                        JacHolder1[l] = T[0][l];
                        JacHolder2[l] = T[1][l];
                    }
                    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,1,1,3,1.0,
                                &JacHolder1[0],3,&JacHolder2[0],1,0.0,&Result[0], 1);

                    A[j][k] = Result[0];
                }
            }
            std::vector<double> Resultant(1, 0.0);
            std::vector<double> BJacHolder(3, 0.0);
            for (int l = 0; l < 3; l++)
                BJacHolder[l] = T[j][l];
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,1,1,3,1.0,
                        &EvalPoint[0],3,&BJacHolder[0],1,0.0,&Resultant[0], 1);

            bt[j] = Resultant[0];

        }
        
		std::vector<double> delt(2,0.0);
        CramerSolve(A,bt,delt);

        //Reparam
        if (ParameterX[i] + delt[0] <= 0)
            ParameterX[i] = 0;
        else if (ParameterX[i] + delt[0] >= 1)
            ParameterX[i] = 1;
        else ParameterX[i] += delt[0];

        if (ParameterY[i] + delt[1] <= 0)
            ParameterY[i] = 0;
        else if (ParameterY[i] + delt[1] >= 1)
            ParameterY[i] = 1;
        else ParameterY[i] += delt[1];

	}

	error /= NumOfPoints;

	MeshCore::MeshPointIterator v_it(MeshParam);
	std::list< std::vector <unsigned long> >::iterator bInd = BoundariesIndex.begin();
	std::vector <unsigned long> boundarieslist = *bInd;
    std::vector <unsigned long>::iterator vec_it;
	int s=0;
	int b=0;
	for (v_it.Begin(); !v_it.EndReached(); ++v_it)
    {
        //Inner Points goes into the beginning of the list
        if ((vec_it = find(boundarieslist.begin(), boundarieslist.end(), v_it.Position())) == boundarieslist.end())
        {

			pntArr[v_it.Position()].x = ParameterX[s];
			pntArr[v_it.Position()].y = ParameterY[s];
            s++;
        }
        //Boundaries goes into the end of the list
        else
        {

			pntArr[v_it.Position()].x = ParameterX[NumOfInnerPoints+b];
			pntArr[v_it.Position()].y = ParameterY[NumOfInnerPoints+b];

            b++;
        }
    }


	MeshParam.Assign(pntArr,fctArr);
    std::cout << "DONE" << std::endl;
	return error;
}

/*! \brief Extend the Nurb

 Once error is computed and the generated nurb is still not satisfactory (i.e max_err > tolerance), this function will extend
 the given Nurb by extending the Knot vectors by 2 and, because the degree is held constant, the control points
*/
void Approximate::ExtendNurb(double c2, int h)
{
    std::cout << "Extending Knot Vector" << std::endl;
    std::cout << "Two knot extension" << std::endl;
    MainNurb.MaxU += 2;
    MainNurb.MaxV += 2;
    MainNurb.MaxKnotU += 2;
    MainNurb.MaxKnotV += 2;
    //U-V Knot Extension
    ExtendKnot(ParameterX[h], MainNurb.DegreeU, MainNurb.MaxU, MainNurb.KnotU);
    ExtendKnot(ParameterY[h], MainNurb.DegreeV, MainNurb.MaxV, MainNurb.KnotV);

}

/*! \brief Reorder the neighbour list

 This function will reorder the list in one-direction. Clockwise or counter clockwise is dependent on the
 facet list and will not be checked by this function. (i.e the third vertex i.e vertex in first facet that
 is not the CurIndex or the first neighbour in pnt[Ok, I am also lost with this... just debug and step to see what I mean...])
*/
void Approximate::ReorderNeighbourList(std::set<unsigned long> &pnt,
                                       std::set<unsigned long> &face, std::vector<unsigned long> &nei, unsigned long CurInd)
{
    MeshCore::MeshPointArray::_TConstIterator v_beg = LocalMesh.GetPoints().begin();
    MeshCore::MeshFacetArray::_TConstIterator f_beg = LocalMesh.GetFacets().begin();
    std::set<unsigned long>::iterator pnt_it;
    std::set<unsigned long>::iterator face_it;
    std::vector<unsigned long>::iterator vec_it;
    std::vector<unsigned long>::iterator ulong_it;
    unsigned long PrevIndex;
    pnt_it = pnt.begin();
    face_it = face.begin();
    nei.push_back(v_beg[*pnt_it]._ulProp);  //push back first neighbour
    vec_it = nei.begin();
    PrevIndex = nei[0];  //Initialize PrevIndex
    for (unsigned int i = 1; i < pnt.size(); i++) //Start
    {
        while (true)
        {
            std::vector<unsigned long> facetpnt;
            facetpnt.push_back(f_beg[*face_it]._aulPoints[0]); //push back into a vector for easier iteration
            facetpnt.push_back(f_beg[*face_it]._aulPoints[1]);
            facetpnt.push_back(f_beg[*face_it]._aulPoints[2]);
            if ((ulong_it = find(facetpnt.begin(), facetpnt.end(), PrevIndex)) == facetpnt.end())  //if PrevIndex not found
            {
                //in current facet
                ++face_it; //next face please
                continue;
            }
            else
            {
                for (unsigned int k = 0 ; k < 3; k++)
                {
                    //If current facetpnt[k] is not yet in nei_list AND it is not the CurIndex
                    if (((vec_it = find(nei.begin(), nei.end(), facetpnt[k])) == nei.end()) && facetpnt[k] != CurInd)
                    {
                        face.erase(face_it);   //erase this face
                        nei.push_back(facetpnt[k]);  //push back the index
                        PrevIndex = facetpnt[k];   //this index is now PrevIndex
                        face_it = face.begin(); //reassign the iterator
                        break;   //end this for-loop
                    }
                }
                break;  //end this while loop
            }
        }
    }
}
