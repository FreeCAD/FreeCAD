/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *   Human Rezai <human@mytum.de>                                          *
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

/*****************best_fit.CPP*****************
* Contains implementations from best_fit.h
*
*
*********************************************/

#include "PreCompiled.h"
#include "best_fit.h"
#include "routine.h"
#include <strstream>

#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Builder.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>

#include <Base/Builder3D.h>

#include <BRep_Tool.hxx>
#include "BRepUtils.h"
#include <BRepBuilderAPI_Sewing.hxx>

//#include <BRepMeshAdapt.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepMesh.hxx>

#include <BRepGProp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GProp_PrincipalProps.hxx>

#include <Handle_Poly_Triangulation.hxx>
#include <Poly_Triangulation.hxx>

#include <ANN/ANN.h> // ANN declarations

#include <SMESH_Gen.hxx>


best_fit::best_fit()
{
    m_LSPnts.resize(2);
}

best_fit::~best_fit()
{
}

void best_fit::Load(const MeshCore::MeshKernel &mesh, const TopoDS_Shape &cad)
{
    m_Mesh = mesh;
    m_Cad  = cad;

    m_MeshWork = m_Mesh;

}

double best_fit::ANN()
{
    ANNpointArray   dataPts;                // data points
    ANNpoint        queryPt;                // query point
    ANNidxArray     nnIdx;                    // near neighbor indices
    ANNdistArray    dists;                    // near neighbor distances
    ANNkd_tree*     kdTree;                    // search structure

	Base::Builder3D log_error;

    //MeshCore::MeshPointArray meshPnts = m_MeshWork.GetPoints();
	m_pntCloud_2;
    Base::Vector3f projPoint;

    double error = 0.0;

    int a_dim = 3;
    int a_nbPnts =(int) m_pntCloud_2.size();        // Size vom eingescanntem Netz
    int a_nbNear = 1;                               // anzahl der rückgabewerte
    queryPt = annAllocPt(a_dim);                    // allocate query point storage
    dataPts = annAllocPts(a_nbPnts, a_dim);         // allocate data points storage
    nnIdx = new ANNidx[a_nbNear];                   // allocate near neigh indices
    dists = new ANNdist[a_nbNear];                  // allocatenear neighbor dists

    m_LSPnts[0].clear();
    m_LSPnts[1].clear();

    for (int i=0; i<a_nbPnts; ++i)
    {
        dataPts[i][0] = m_pntCloud_2[i].x;
        dataPts[i][1] = m_pntCloud_2[i].y;
        dataPts[i][2] = m_pntCloud_2[i].z;
    }

    kdTree = new ANNkd_tree(        // build search structure
        dataPts,                    // the data points
        a_nbPnts,            // number of points
        a_dim);                     // dimension of space


    for (unsigned int i = 0 ; i < m_pntCloud_1.size() ; i++ )
    {
        queryPt[0] = m_pntCloud_1[i].x;
        queryPt[1] = m_pntCloud_1[i].y;
        queryPt[2] = m_pntCloud_1[i].z;

        kdTree->annkSearch(                        // search
            queryPt,							   // query point
            a_nbNear,						       // number of near neighbors
            nnIdx,                                 // nearest neighbors (returned)
            dists                                  // distance (returned)
        );                                         // error bound

        m_LSPnts[1].push_back(m_pntCloud_1[i]);
        m_LSPnts[0].push_back(m_pntCloud_2[nnIdx[0]]);

		if(m_pntCloud_1[i].z <= m_pntCloud_2[nnIdx[0]].z)
		{
			log_error.addSingleLine(m_pntCloud_1[i],m_pntCloud_2[nnIdx[0]],8,1,0,0);
		}
		else
		{
			log_error.addSingleLine(m_pntCloud_1[i],m_pntCloud_2[nnIdx[0]],8,0,1,0);
		}

		//if(dists[0] > error)
			error += dists[0];

    }

	log_error.saveToFile("c:/errorVec_fit.iv");

    error /= double(m_pntCloud_1.size());
    m_weights_loc = m_weights;


    delete [] nnIdx; // clean things up
    delete [] dists;
    delete kdTree;
    annClose(); // done with ANN

    return error;
}

bool best_fit::Perform()
{
    Base::Matrix4D M;

    ofstream Runtime_BestFit;
	time_t sec1, sec2;

	Runtime_BestFit.open("c:/Runtime_BestFit.txt");
	Runtime_BestFit << "Runtime Best-Fit" << std::endl;


    cout << "tessellate shape" << endl;

	sec1 = time(NULL);
   	  Tesselate_Shape(m_Cad, m_CadMesh, 1); // Tessellates m_Cad Shape and stores Tessellation in m_CadMesh 
	sec2 = time(NULL);

	Runtime_BestFit << "Tessellate Shape: " << sec2 - sec1 << " sec" << std::endl;  

	sec1 = time(NULL);
      Comp_Weights(); // m_pntCloud_1, m_weights, m_normals des/r Cad-Meshs/Punktewolke werden hier gefüllt
    sec2 = time(NULL);

	Runtime_BestFit << "Compute Weights: " << sec2 - sec1 << " sec" << std::endl;  
	

	/*RotMat(M, 180, 1);
    m_MeshWork.Transform(M);
	return true;*/


	//MeshCore::MeshPointArray pntarr = m_MeshWork.GetPoints();
	//MeshCore::MeshFacetArray facetarr = m_MeshWork.GetFacets();

	//for(int i=0; i<pntarr.size(); ++i)
	//{
	//	pntarr[i].x -= 200;
	//	pntarr[i].y -= 200;
	//	pntarr[i].z += 50;
	//}

	//m_MeshWork.Assign(pntarr,facetarr);


    sec1 = time(NULL);
	
	MeshFit_Coarse();  // Transformation Mesh -> CAD
    ShapeFit_Coarse(); // Translation    CAD  -> Origin

	//return true;
	
    M.setToUnity();
    M[0][3] = m_cad2orig.X();
    M[1][3] = m_cad2orig.Y();
    M[2][3] = m_cad2orig.Z();

    m_CadMesh.Transform(M); // besser: tesselierung nach der trafo !!!
    m_MeshWork.Transform(M);
    PointTransform(m_pntCloud_1,M);

	MeshCore::MeshPointArray pnts = m_MeshWork.GetPoints();

	for (unsigned int i=0; i<pnts.size(); ++i)
	{
		m_pntCloud_2.push_back(pnts[i]);

	}

    Runtime_BestFit << "- Error: " << ANN() << endl;

    Coarse_correction();

	sec2 = time(NULL);
	Runtime_BestFit << "Coarse Correction: " << sec2-sec1 << " sec" << endl;

    sec1 = time(NULL);
	LSM();
	sec2 = time(NULL);
    Runtime_BestFit << "Least-Square-Matching: " << sec2-sec1 << " sec" << endl;
	Runtime_BestFit.close();

    Base::Matrix4D T;
    T.setToUnity();
    T[0][3] = -m_cad2orig.X();
    T[1][3] = -m_cad2orig.Y();
    T[2][3] = -m_cad2orig.Z();
    m_MeshWork.Transform(T);
    m_CadMesh.Transform(T);

	m_Mesh = m_MeshWork;
    CompTotalError();

    return true;


}

bool best_fit::Perform_PointCloud()
{
	Base::Matrix4D M;

	ofstream Runtime_BestFit;
	time_t sec1, sec2;

	Runtime_BestFit.open("c:/Runtime_BestFit_PntCld.txt");
	Runtime_BestFit << "Runtime Best-Fit" << std::endl;

	PointCloud_Coarse();  

	M.setToUnity();

	for(unsigned int i=0; i<m_pntCloud_1.size(); i++)
		m_weights.push_back(1.0);
	
	M[0][3] = m_cad2orig.X();
	M[1][3] = m_cad2orig.Y();
	M[2][3] = m_cad2orig.Z();

	PointTransform(m_pntCloud_1,M);
	PointTransform(m_pntCloud_2,M);

	//Runtime_BestFit << "- Error: " << ANN() << endl;
    sec1 = time(NULL);
	Coarse_correction();
	sec2 = time(NULL);
	Runtime_BestFit << "Coarse Correction: " << sec2-sec1 << " sec" << endl;

	//M[0][3] = m_cad2orig.X();
	//M[1][3] = m_cad2orig.Y();
	//M[2][3] = m_cad2orig.Z();

	//PointTransform(m_pntCloud_1,M);
	//PointTransform(m_pntCloud_2,M);

	sec1 = time(NULL);
	LSM();
	sec2 = time(NULL);
	Runtime_BestFit << "Least-Square-Matching: " << sec2-sec1 << " sec" << endl;
	Runtime_BestFit.close();

	Base::Matrix4D T;
	T.setToUnity();
	T[0][3] = -m_cad2orig.X();
	T[1][3] = -m_cad2orig.Y();
	T[2][3] = -m_cad2orig.Z();
	PointTransform(m_pntCloud_1, T);
	PointTransform(m_pntCloud_2, T);
	m_MeshWork.Transform(T);
	m_CadMesh.Transform(T);

	m_Mesh = m_MeshWork;
	CompTotalError();

	return true;
}

/*
bool best_fit::Intersect(const Base::Vector3f &normal,const MeshCore::MeshKernel &mesh, Base::Vector3f &P, Base::Vector3f &I)
{
    MeshCore::MeshFacetIterator f_it(mesh);

    for (f_it.Begin(); f_it.More(); f_it.Next())
    {
        if (intersect_RayTriangle(normal, *f_it, P, I) == 1)
            return true;
    }

    return false;
}

int best_fit::intersect_RayTriangle(const Base::Vector3f &normal,const MeshCore::MeshGeomFacet &T, Base::Vector3f &P, Base::Vector3f &I)
{
    Base::Vector3f    u, v, n, null(0.0,0.0,0.0), J;          // triangle vectors
    Base::Vector3f    dir, w0, w;                             // ray vectors
    float             r, a, b;                                // params to calc ray-plane intersect

    J = P+normal;

    // get triangle edge vectors and plane normal
    u = T._aclPoints[1] - T._aclPoints[0];
    v = T._aclPoints[2] - T._aclPoints[0];
    n = u % v;                // cross product
    if (n == null)            // triangle is degenerate
        return -1;            // do not deal with this case

    dir = normal;    // ray direction vector
    w0  = P - T._aclPoints[0];
    a   = n * w0;
    b   = n * dir;
    if (fabs(b) < SMALL_NUM)      // ray is parallel to triangle plane
    {
        if (a == 0)                // ray lies in triangle plane
            return 2;
        else return 0;             // ray disjoint from plane
    }

    // get intersect point of ray with triangle plane
    r = -a / b;

    //if (r < 0.0)                   // ray goes away from triangle
    //    return 0;                  // => no intersect
    // for a segment, also test if (r > 1.0) => no intersect

    dir.Scale(r,r,r);
    I = P + dir;              // intersect point of ray and plane

    // is I inside T?
    float    uu, uv, vv, wu, wv, D;
    uu = u*u;
    uv = u*v;
    vv = v*v;
    w = I - T._aclPoints[0];
    wu = w*u;
    wv = w*v;
    D = uv * uv - uu * vv;

    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0)        // I is outside T
        return 0;
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0)  // I is outside T
        return 0;

    return 1;                      // I is in T
}
*/

bool best_fit::Coarse_correction()
{
    double error, error_tmp, rot = 0.0;
    Base::Matrix4D M,T;
	ofstream CoarseCorr;

	CoarseCorr.open("c:/CoarseCorr.txt");

    std::vector<Base::Vector3f> m_pntCloud_Work = m_pntCloud_2;

    T.setToUnity();
    best_fit befi; 
    
	//error = CompError_GetPnts(m_pnts, m_normals)[0];  // startfehler    int n=360/rstep_corr;
    
	error = ANN();

    for (int i=1; i<4; ++i)
    {
        RotMat(M, 180, i);
        PointTransform(m_pntCloud_2, M);
		//m_MeshWork.Transform(M);

        error_tmp = ANN();
		
		//error_tmp = CompError_GetPnts(m_pnts, m_normals)[0];
        //error_tmp = befi.CompTotalError(m_MeshWork);
		
		CoarseCorr << i << ", " << error_tmp << endl;

        if (error_tmp < error)
        {
            T = M;
            error = error_tmp;
        }

        m_pntCloud_2 = m_pntCloud_Work;
    }

	CoarseCorr << "BEST CHOICE: " << error << endl;
	CoarseCorr.close();
    PointTransform(m_pntCloud_2, T);
	m_MeshWork.Transform(T);


    return true;
}


bool best_fit::LSM()
{
    double TOL  = 0.05;          // Abbruchkriterium des Newton-Verfahren
    int maxIter = 100;            // maximale Anzahl von Iterationen für den Fall,
							     // dass das Abbruchkriterium nicht erfüllt wird

   int mult = 2;                 // zur Halbierung der Schrittweite bei Misserfolg des Newton Verfahrens

    double val, tmp = 1e+10, delta, delta_tmp = 0.0;
    Base::Matrix4D Tx,Ty,Tz,Rx,Ry,Rz,M;   // Transformaitonsmatrizen

	ofstream anOutputFile;
	anOutputFile.open("c:/outputBestFit.txt");
    anOutputFile.precision(7);

      int c=0; // Laufvariable
    


    std::vector<double> del_x(3,0.0);
    std::vector<double>     x(3,0.0); // Startparameter entspricht Nullvektor

    Base::Vector3f centr_l,centr_r, cent;  // Schwerpunkte der Punktesätze

    // Newton Verfahren: 1. Löse  H*del_x = -J
    //                   2. Setze x = x + del_x

    std::vector<double> Jac(3);           // 1.Ableitung der Fehlerfunktion (Jacobi-Matrix)
    std::vector< std::vector<double> > H; // 2.Ableitung der Fehlerfunktion (Hesse-Matrix)

    time_t seconds1, seconds2, sec1, sec2;
    seconds1 = time(NULL);

    while (true)
    {

        seconds1 = time(NULL);
        //m_Mesh = m_MeshWork;
        
		// Fehlerberechnung vom CAD -> Mesh
        //tmp = CompError_GetPnts(m_pnts, m_normals);      // hier: - Berechnung der LS-Punktesätze
        //      CompTotalError()                           //       - Berechnung der zugehörigen Gewichtungen

		delta = delta_tmp;
        delta_tmp = ANN();          // gibt durchschnittlichen absoluten Fehler aus
		delta = delta - delta_tmp ; // hier wird die Fehlerverbesserung zum vorigen Iterationsschritt gespeichert

		if (c==maxIter || delta < ERR_TOL && c>1) break; // Abbruchkriterium (falls maximale Iterationsschrite erreicht
										                 //                   oder falls Fehleränderung unsignifikant gering)

        seconds2 = time(NULL);
		anOutputFile << c << ", " << delta_tmp << ", " << delta << "    -    Time: " << seconds2 - seconds1 << " sec" << endl;
		seconds1 = time(NULL);

		sec1 = time(NULL);
        for (unsigned int i=0; i<x.size(); ++i) x[i] = 0.0; // setzt startwerte für newton auf null


        // Berechne gewichtete Schwerpunkte und verschiebe die Punktesätze entsprechend:
        centr_l.Scale(0.0,0.0,0.0);
        centr_r.Scale(0.0,0.0,0.0);

        Base::Vector3f p,q;
		double Sum = 0.0;
        for (unsigned int i=0; i<m_LSPnts[0].size(); ++i)
        {
            p = m_LSPnts[0][i];
            q = m_LSPnts[1][i];

            p.Scale((float) m_weights_loc[i],(float) m_weights_loc[i],(float) m_weights_loc[i]);
            q.Scale((float) m_weights_loc[i],(float) m_weights_loc[i],(float) m_weights_loc[i]);

			Sum += m_weights_loc[i];
			centr_l += p;
            centr_r += q;
        }

		centr_l.Scale( -1.0f/float(Sum), -1.0f/float(Sum), -1.0f/float(Sum));
		centr_r.Scale( -1.0f/(float)Sum, -1.0f/(float)Sum, -1.0f/(float)Sum);

       /* float s = (float) m_LSPnts[0].size();
        s = float(-1.0/s);

        centr_l.Scale(s,s,s);
        centr_r.Scale(s,s,s);*/



        // Verschiebung der Schwerpunkte zum Ursprung
        TransMat(Tx,centr_l.x,1);
        TransMat(Ty,centr_l.y,2);
        TransMat(Tz,centr_l.z,3);

        M = Tx*Ty*Tz;
        PointTransform(m_LSPnts[0],M);
		PointTransform(m_pntCloud_1,M);
		PointTransform(m_pntCloud_2,M);
        m_MeshWork.Transform(M);

        TransMat(Tx,centr_r.x,1); // Berechnung der Translationsmatrix in x-Richtung
        TransMat(Ty,centr_r.y,2); // Berechnung der Translationsmatrix in y-Richtung
        TransMat(Tz,centr_r.z,3); // Berechnung der Translationsmatrix in z-Richtung

        M = Tx*Ty*Tz;                  // Zusammenfügen zu einer Gesamttranslationsmatrix
        PointTransform(m_LSPnts[1],M); // Anwendung der Translation auf m_LSPnts
        PointTransform(m_pntCloud_1,M);
		//PointNormalTransform(m_pnts, m_normals, M); // Anwendung der Translation auf m_pnts
        m_CadMesh.Transform(M);                       // Anwendung der Translation auf das CadMesh

        sec2 = time(NULL);
		anOutputFile << c+1 << " - Initialisierung und Transformation um gewichtete Schwerpunkte: " << sec2 - sec1 << " sec" << endl;

		sec1 = time(NULL);
        // Newton-Verfahren zur Berechnung der Rotationsmatrix:
        while (true)
        {

            Jac = Comp_Jacobi(x);  // berechne 1.Ableitung
            H   = Comp_Hess(x);    // berechne 2.Ableitung

            val = 0.0;
            for (unsigned int i=0; i<Jac.size(); ++i)
            {
                val += Jac[i]*Jac[i];
            }
            val = sqrt(val);

            if (val < TOL) break; // Abbruchkriterium des Newton-Verfahren

            if (val>tmp && mult < 1e+4)
            {
                for (unsigned int i=0; i<del_x.size(); ++i)
                {
                    x[i] -= del_x[i]/double(mult);  // Halbiere Schrittweite falls keine Verbesserung
                }
                mult *= 2;
                continue;
            }
            else
            {
                mult = 2;
            }

            tmp = val;

            del_x = Routines::NewtonStep(Jac,H);      // löst Gl.system:          H*del_x = -J
            for (unsigned int i=0; i<x.size(); ++i)   // nächster Iterationswert: x = x + del_x
            {
                x[i] += del_x[i];
            }
        }

		sec2 = time(NULL);
		anOutputFile << c+1 << " - Newton: " << seconds2 - seconds1 << " sec" << endl;
        sec1 = time(NULL);
        // Rotiere und verschiebe zurück zum Ursprung der !!! CAD-Geometrie !!!
        RotMat  (Rx,(x[0]*180.0/PI),1);
        RotMat  (Ry,(x[1]*180.0/PI),2);
        RotMat  (Rz,(x[2]*180.0/PI),3);

		Base::Matrix4D R = Rx*Ry*Rz;
		centr_l.Scale(-1.0, -1.0, -1.0);

		cent.x =(float)  (R[0][0]*centr_l.x + R[0][1]*centr_l.y + R[0][2]*centr_l.z);
		cent.y =(float)  (R[1][0]*centr_l.x + R[1][1]*centr_l.y + R[1][2]*centr_l.z);
		cent.z =(float)  (R[2][0]*centr_l.x + R[2][1]*centr_l.y + R[2][2]*centr_l.z);
			 
		TransMat(Tx, -centr_r.x - cent.x + centr_l.x, 1);
        TransMat(Ty, -centr_r.y - cent.y + centr_l.y, 2);
		TransMat(Tz, -centr_r.z - cent.z + centr_l.z, 3);

        M = Tx*Ty*Tz*Rx*Ry*Rz; // Rotiere zuerst !!! (Rotationen stets um den Nullpunkt...)
        
		PointTransform(m_pntCloud_2,M);
		m_MeshWork.Transform(M);

		TransMat(Tx, -centr_r.x, 1);
		TransMat(Ty, -centr_r.y, 2);
		TransMat(Tz, -centr_r.z, 3);

        M = Tx*Ty*Tz;
		PointTransform(m_pntCloud_1,M);
        m_CadMesh.Transform(M);
        //PointNormalTransform(m_pnts, m_normals, M);

   		sec2 = time(NULL);
		
		anOutputFile << c+1 << " - Trafo: " << seconds2 - seconds1 << " sec" << endl;
        ++c;  //Erhöhe Laufvariable 
    }

	anOutputFile.close();
return true;

    /*TransMat(Tx,-centr_l.x,1);
       TransMat(Ty,-centr_l.y,2);
    TransMat(Tz,-centr_l.z,3);

    M = Tx*Ty*Tz;
    PointTransform(m_LSPnts[0],M);

    TransMat(Tx,-centr_r.x,1);
       TransMat(Ty,-centr_r.y,2);
    TransMat(Tz,-centr_r.z,3);

    M = Tx*Ty*Tz;
    PointTransform(m_LSPnts[1],M);

    Base::Builder3D log;

    for(unsigned int i=0; i<m_LSPnts[0].size(); ++i)
     log.addSingleArrow(m_LSPnts[0][i],m_LSPnts[1][i]);

    log.saveToFile("c:/newton_pnts.iv");*/

  
}


std::vector<double> best_fit::Comp_Jacobi(const std::vector<double> &x)
{
    std::vector<double> F(3,0.0);
    double s1 = sin(x[0]), c1 = cos(x[0]);
    double s2 = sin(x[1]), c2 = cos(x[1]);
    double s3 = sin(x[2]), c3 = cos(x[2]);

    Base::Vector3f p,q;

    for (unsigned int i=0; i<m_LSPnts[0].size(); ++i)
    {
        p = m_LSPnts[0][i];
        q = m_LSPnts[1][i];

        F[0] += ( q.y * ( (-c1*s2*c3-s1*s3) * p.x +  (c1*s2*s3-s1*c3) * p.y + (-c1*c2) * p.z ) +
                  q.z * ( (-s1*s2*c3+c1*s3) * p.x +  (s1*s2*s3+c1*c3) * p.y + (-s1*c2) * p.z ) )*m_weights_loc[i];

        F[1] += ( q.x * (          (-s2*c3) * p.x +           (s2*s3) * p.y +    (-c2) * p.z ) +
                  q.y * (       (-s1*c2*c3) * p.x +        (s1*c2*s3) * p.y +  (s1*s2) * p.z ) +
                  q.z * (        (c1*c2*c3) * p.x +       (-c1*c2*s3) * p.y + (-c1*s2) * p.z ) )*m_weights_loc[i];

        F[2] += ( q.x * (          (-c2*s3) * p.x +          (-c2*c3) * p.y) +
                  q.y * (  (s1*s2*s3+c1*c3) * p.x +  (s1*s2*c3-c1*s3) * p.y) +
                  q.z * ( (-c1*s2*s3+s1*c3) * p.x + (-c1*s2*c3-s1*s3) * p.y) )*m_weights_loc[i];
    }

    return F;
}

std::vector<std::vector<double> > best_fit::Comp_Hess(const std::vector<double> &x)
{
    std::vector<std::vector<double> > DF(3);
    for (unsigned int i=0; i<DF.size(); ++i)
    {
        DF[i].resize(3,0.0);
    }

    double s1 = sin(x[0]), c1 = cos(x[0]);
    double s2 = sin(x[1]), c2 = cos(x[1]);
    double s3 = sin(x[2]), c3 = cos(x[2]);

    double sum1 = 0.0, sum2 = 0.0, sum3 = 0.0, sum4 = 0.0, sum5 = 0.0, sum6 = 0.0;

    Base::Vector3f p,q;

    for (unsigned int i=0; i<m_LSPnts[0].size(); ++i)
    {
        p = m_LSPnts[0][i];
        q = m_LSPnts[1][i];

        sum1      = q.y * (  (s1*s2*c3-c1*s3) * p.x + (-s1*s2*s3-c1*c3) * p.y +  (s1*c2) * p.z ) +
                    q.z * ( (-c1*s2*c3-s1*s3) * p.x +  (c1*s2*s3-s1*c3) * p.y + (-c1*c2) * p.z );

        sum2      = q.x * (          (-c2*c3) * p.x +           (c2*s3) * p.y +     (s2) * p.z ) +
                    q.y * (        (s1*s2*c3) * p.x +       (-s1*s2*s3) * p.y +  (s1*c2) * p.z ) +
                    q.z * (       (-c1*s2*c3) * p.x +        (c1*s2*s3) * p.y + (-c1*c2) * p.z );

        sum3      = q.x * (          (-c2*c3) * p.x +           (c2*s3) * p.y) +
                    q.y * (  (s1*s2*c3-c1*s3) * p.x + (-s1*s2*s3-c1*c3) * p.y) +
                    q.z * ( (-c1*s2*c3-s1*s3) * p.x +  (c1*s2*s3-s1*c3) * p.y);

        sum4      = q.y * (       (-c1*c2*c3) * p.x +        (c1*c2*s3) * p.y +  (c1*s2) * p.z ) +
                    q.z * (       (-s1*c2*c3) * p.x +        (s1*c2*s3) * p.y +  (s1*s2) * p.z );

        sum5      = q.y * (  (c1*s2*s3-s1*c3) * p.x +  (c1*s2*c3+s1*s3) * p.y) +
                    q.z * (  (s1*s2*s3+c1*c3) * p.x +  (s1*s2*c3-c1*s3) * p.y);

        sum6      = q.x * (           (s2*s3) * p.x +           (s2*c3) * p.y) +
                    q.y * (        (s1*c2*s3) * p.x +        (s1*c2*c3) * p.y) +
                    q.z * (       (-c1*c2*s3) * p.x +       (-c1*c2*c3) * p.y);

        DF[0][0] += sum1*m_weights_loc[i];
        DF[1][1] += sum2*m_weights_loc[i];
        DF[2][2] += sum3*m_weights_loc[i];
        DF[0][1] += sum4*m_weights_loc[i];
        DF[1][0] += sum4*m_weights_loc[i];
        DF[0][2] += sum5*m_weights_loc[i];
        DF[2][0] += sum5*m_weights_loc[i];
        DF[1][2] += sum6*m_weights_loc[i];
        DF[2][1] += sum6*m_weights_loc[i];
    }

    return DF;
}

bool best_fit::Comp_Weights()
{
	double weight_low = 1, weight_high = 2;
    TopExp_Explorer aExpFace;
    MeshCore::MeshKernel FaceMesh;
    MeshCore::MeshFacetArray facetArr;

    MeshCore::MeshKernel mesh1,mesh2;
    MeshCore::MeshBuilder builder1(mesh1);
    MeshCore::MeshBuilder builder2(mesh2);
    builder1.Initialize(1000);
    builder2.Initialize(1000);

    Base::Vector3f Points[3];
    TopLoc_Location aLocation;

    bool bf;

    // explores all faces  ------------  Hauptschleife
    for (aExpFace.Init(m_Cad,TopAbs_FACE);aExpFace.More();aExpFace.Next())
    {
        TopoDS_Face aFace = TopoDS::Face(aExpFace.Current());

        bf = false;
        for (unsigned int i=0; i<m_LowFaces.size(); ++i)
        {
            if (m_LowFaces[i].HashCode(IntegerLast()) == aFace.HashCode(IntegerLast())) bf = true;
        }

        // takes the triangulation of the face aFace:
        Handle_Poly_Triangulation aTr = BRep_Tool::Triangulation(aFace,aLocation);
        if (!aTr.IsNull()) // if this triangulation is not NULL
        {
            // takes the array of nodes for this triangulation:
            const TColgp_Array1OfPnt& aNodes = aTr->Nodes();

            // takes the array of triangles for this triangulation:
            const Poly_Array1OfTriangle& triangles = aTr->Triangles();

            // create array of node points in absolute coordinate system
            TColgp_Array1OfPnt aPoints(1, aNodes.Length());
            for ( Standard_Integer i = 1; i <= aNodes.Length(); i++)
                aPoints(i) = aNodes(i).Transformed(aLocation);

            // Takes the node points of each triangle of this triangulation.
            // takes a number of triangles:
            Standard_Integer nnn = aTr->NbTriangles();
            Standard_Integer nt,n1,n2,n3;
            for ( nt = 1 ; nt < nnn+1 ; nt++)
            {
                // takes the node indices of each triangle in n1,n2,n3:
                triangles(nt).Get(n1,n2,n3);
                // takes the node points:
                gp_Pnt aPnt1 = aPoints(n1);
                Points[0].Set(float(aPnt1.X()),float(aPnt1.Y()),float(aPnt1.Z()));
                gp_Pnt aPnt2 = aPoints(n2);
                Points[1].Set((float) aPnt2.X(),(float) aPnt2.Y(),(float) aPnt2.Z());
                gp_Pnt aPnt3 = aPoints(n3);
                Points[2].Set((float) aPnt3.X(),(float) aPnt3.Y(),(float) aPnt3.Z());

                // give the occ faces to the internal mesh structure of freecad
                MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);

                if (bf == false) builder1.AddFacet(Face);
                else   builder2.AddFacet(Face);

            }
        }
    }

    builder1.Finish();
    builder2.Finish();

    m_pntCloud_1.clear();
    m_weights.clear();

    MeshCore::MeshPointArray pnts = mesh1.GetPoints();

    for (unsigned int i=0; i<pnts.size(); ++i)
    {
        m_pntCloud_1.push_back(pnts[i]);
        m_weights.push_back(weight_low);
    }

    pnts = mesh2.GetPoints();

    for (unsigned int i=0; i<pnts.size(); ++i)
    {
        m_pntCloud_1.push_back(pnts[i]);
        m_weights.push_back(weight_high);
    }

    m_normals = Comp_Normals(mesh1);
    std::vector<Base::Vector3f> tmp = Comp_Normals(mesh2);

    for (unsigned int i=0; i<tmp.size(); ++i)
    {
        m_normals.push_back(tmp[i]);
    }

    return true;
}

bool best_fit::RotMat(Base::Matrix4D &M, double degree, int axis)
{
    M.setToUnity();
    degree = 2*PI*degree/360;  // trafo bogenmaß

    switch (axis)
    {
    case 1:
        M[1][1]=cos(degree);
        M[1][2]=-sin(degree);
        M[2][1]=sin(degree);
        M[2][2]=cos(degree);
        break;
    case 2:
        M[0][0]=cos(degree);
        M[0][2]=-sin(degree);
        M[2][0]=sin(degree);
        M[2][2]=cos(degree);
        break;
    case 3:
        M[0][0]=cos(degree);
        M[0][1]=-sin(degree);
        M[1][0]=sin(degree);
        M[1][1]=cos(degree);
        break;
    default:
        throw Base::RuntimeError("second input value differs from 1,2,3 (x,y,z)");
    }

    return true;
}

bool best_fit::TransMat(Base::Matrix4D &M, double trans, int axis)
{
    M.setToUnity();

    switch (axis)
    {
    case 1:
        M[0][3] = trans;
        break;
    case 2:
        M[1][3] = trans;
        break;
    case 3:
        M[2][3] = trans;
        break;
    default:
        throw Base::RuntimeError("second input value differs from 1,2,3 (x,y,z)");
    }

    return true;
}

bool best_fit::PointNormalTransform(std::vector<Base::Vector3f> &pnts,
                                    std::vector<Base::Vector3f> &normals,
                                    Base::Matrix4D              &M)
{
    int m = pnts.size();
    Base::Vector3f pnt,normal;

    for (int i=0; i<m; ++i)
    {
        pnt.x  = float(M[0][0]*pnts[i].x + M[0][1]*pnts[i].y + M[0][2]*pnts[i].z + M[0][3]);
        pnt.y  = float(M[1][0]*pnts[i].x + M[1][1]*pnts[i].y + M[1][2]*pnts[i].z + M[1][3]);
        pnt.z  = float(M[2][0]*pnts[i].x + M[2][1]*pnts[i].y + M[2][2]*pnts[i].z + M[2][3]);

        pnts[i] = pnt;

        normal.x = float(M[0][0]*normals[i].x + M[0][1]*normals[i].y + M[0][2]*normals[i].z);
        normal.y = float(M[1][0]*normals[i].x + M[1][1]*normals[i].y + M[1][2]*normals[i].z);
        normal.z = float(M[2][0]*normals[i].x + M[2][1]*normals[i].y + M[2][2]*normals[i].z);

        normals[i] = normal;
    }

    return true;
}

bool best_fit::output_best_fit_mesh()
{
	
	SMDS_NodeIteratorPtr aNodeIter = m_meshtobefit->GetMeshDS()->nodesIterator();

	for(;aNodeIter->more();) 
	{
		const SMDS_MeshNode* aNode = aNodeIter->next();
		m_meshtobefit->GetMeshDS()->MoveNode(aNode,m_pntCloud_2[(aNode->GetID()-1)].x,m_pntCloud_2[(aNode->GetID()-1)].y,m_pntCloud_2[(aNode->GetID()-1)].z);
	}
	m_meshtobefit->ExportUNV("c:/best_fit_mesh.unv");

	return true;
}

bool best_fit::Initialize_Mesh_Geometrie_1()
{
	m_aMeshGen1 = new SMESH_Gen();
	m_referencemesh = m_aMeshGen1->CreateMesh(1,false);
	m_referencemesh->UNVToMesh("c:/cad_mesh_cenaero.unv");

	m_pntCloud_1.clear();

	//add the nodes
	SMDS_NodeIteratorPtr aNodeIter = m_referencemesh->GetMeshDS()->nodesIterator();
	for(;aNodeIter->more();) {
		const SMDS_MeshNode* aNode = aNodeIter->next();
		Base::Vector3f a3DVector;
		a3DVector.Set((float) aNode->X(),(float)  aNode->Y(),(float)  aNode->Z()),
		m_pntCloud_1.push_back(a3DVector);
    }



	return true;
}



bool best_fit::Initialize_Mesh_Geometrie_2()
{

	m_aMeshGen2 = new SMESH_Gen();
	m_meshtobefit = m_aMeshGen2->CreateMesh(1,false);
	m_meshtobefit->UNVToMesh("c:/mesh_cenaero.unv");

	m_pntCloud_2.clear();

	//add the nodes
	SMDS_NodeIteratorPtr aNodeIter = m_meshtobefit->GetMeshDS()->nodesIterator();
	for(;aNodeIter->more();) {
		const SMDS_MeshNode* aNode = aNodeIter->next();
		Base::Vector3f a3DVector;
		a3DVector.Set((float) aNode->X(),(float)  aNode->Y(), (float) aNode->Z()),
		m_pntCloud_2.push_back(a3DVector);
	}

	////add the 2D edge-Elements
	//SMDS_EdgeIteratorPtr 	anEdgeIter = Reference_Mesh->GetMeshDS()->edgesIterator();
	//for(;anEdgeIter->more();) {
	//	const SMDS_MeshEdge* anElem = anEdgeIter->next();
	//	myElements.push_back( anElem->GetID() );
	//}
	////add the 2D-Planar Elements like triangles 
	//SMDS_FaceIteratorPtr 	aFaceIter = Reference_Mesh->GetMeshDS()->facesIterator();
	//for(;aFaceIter->more();) {
	//	const SMDS_MeshFace* anElem = aFaceIter->next();
	//	int element_node_count = anElem->NbNodes();
	//	myElements.push_back( anElem->GetID() );
	//}
	////Add the Volume-Elements
	//SMDS_VolumeIteratorPtr aVolumeIter = Reference_Mesh->GetMeshDS()->volumesIterator();
	//for(;aVolumeIter->more();) {
	//	const SMDS_MeshVolume* anElem = aVolumeIter->next();
	//	myElements.push_back( anElem->GetID() );
	//}

	//int testsize = myElements.size();

	//SMDS_VolumeTool aTooling;


	////Now take the Element-Vector and work with the elements
	////check validity of element
	//for (unsigned int i=0;i<myElements.size();i++)
	//{
	//	const SMDS_MeshElement* CurrentElement = Reference_Mesh->GetMeshDS()->FindElement(myElements[i]);
	//	if (CurrentElement->GetType() == SMDSAbs_Volume) 
	//	{
	//		//We encountered a Surface-Element like a triangle and we have to check if its a triangle or not
	//		aTooling.Set(CurrentElement);
	//		//Now we have to check what kind of volume element we have
	//		if(aTooling.GetVolumeType()== SMDS_VolumeTool::HEXA)
	//		{
	//			//Found a HEXA Element
	//		}
	//	}
	//}

	return true;
}

bool best_fit::PointTransform(std::vector<Base::Vector3f> &pnts, const Base::Matrix4D &M)
{
    int m = pnts.size();
    Base::Vector3f pnt;

    for (int i=0; i<m; ++i)
    {
        pnt.x  =  float(M[0][0]*pnts[i].x + M[0][1]*pnts[i].y + M[0][2]*pnts[i].z + M[0][3]);
        pnt.y  =  float(M[1][0]*pnts[i].x + M[1][1]*pnts[i].y + M[1][2]*pnts[i].z + M[1][3]);
        pnt.z  =  float(M[2][0]*pnts[i].x + M[2][1]*pnts[i].y + M[2][2]*pnts[i].z + M[2][3]);

        pnts[i] = pnt;
    }

    return true;
}

bool best_fit::PointCloud_Coarse()
{
	GProp_GProps prop;
	GProp_PrincipalProps pprop;

	MeshCore::PlaneFit FitFunc_1, FitFunc_2;

	Base::Vector3f pnt(0.0,0.0,0.0);
	Base::Vector3f DirA_1, DirB_1, DirC_1, Grav_1,
  		           DirA_2, DirB_2, DirC_2, Grav_2;
	Base::Vector3f x,y,z;
	Base::Builder3D log3d_mesh, log3d_cad;
	gp_Pnt orig;

	gp_Vec v1,v2,v3,v,vec; // Hauptachsenrichtungen
	gp_Trsf trafo;

	FitFunc_1.Clear();
	FitFunc_2.Clear();

	FitFunc_1.AddPoints(m_pntCloud_1);
	FitFunc_2.AddPoints(m_pntCloud_2);
	
	FitFunc_1.Fit();
	FitFunc_2.Fit();

	DirA_1 = FitFunc_1.GetDirU();
	DirB_1 = FitFunc_1.GetDirV();
	DirC_1 = FitFunc_1.GetNormal();
	Grav_1 = FitFunc_1.GetGravity();

	m_cad2orig.SetX(-Grav_1.x);
	m_cad2orig.SetY(-Grav_1.y);
	m_cad2orig.SetZ(-Grav_1.z);

	DirA_2 = FitFunc_2.GetDirU();
	DirB_2 = FitFunc_2.GetDirV();
	DirC_2 = FitFunc_2.GetNormal();
	Grav_2 = FitFunc_2.GetGravity();

	Base::Matrix4D T5, T1;

	// Füllt Matrix T5 
	T5[0][0] = DirA_1.x;
	T5[1][0] = DirA_1.y;
	T5[2][0] = DirA_1.z;

	T5[0][1] = DirB_1.x;
	T5[1][1] = DirB_1.y;
	T5[2][1] = DirB_1.z;

	T5[0][2] = DirC_1.x;
	T5[1][2] = DirC_1.y;
	T5[2][2] = DirC_1.z;

	T5[0][3] = Grav_1.x;
	T5[1][3] = Grav_1.y;
	T5[2][3] = Grav_1.z;

	/*T5[0][0] = DirA_1.x;
	T5[0][1] = DirA_1.y;
	T5[0][2] = DirA_1.z;

	T5[1][0] = DirB_1.x;
	T5[1][1] = DirB_1.y;
	T5[1][2] = DirB_1.z;

	T5[2][0] = DirC_1.x;
	T5[2][1] = DirC_1.y;
	T5[2][2] = DirC_1.z;

	T5[0][3] = Grav_1.x;
	T5[1][3] = Grav_1.y;
	T5[2][3] = Grav_1.z;*/


	// Füllt Matrix T1
	T1[0][0] = DirA_2.x;
	T1[1][0] = DirA_2.y;
	T1[2][0] = DirA_2.z;

	T1[0][1] = DirB_2.x;
	T1[1][1] = DirB_2.y;
	T1[2][1] = DirB_2.z;

	T1[0][2] = DirC_2.x;
	T1[1][2] = DirC_2.y;
	T1[2][2] = DirC_2.z;

	T1[0][3] = Grav_2.x;
	T1[1][3] = Grav_2.y;
	T1[2][3] = Grav_2.z;

	v1.SetX(T5[0][0]);v1.SetY(T5[0][1]);v1.SetZ(T5[0][2]);
	v2.SetX(T5[1][0]);v2.SetY(T5[1][1]);v2.SetZ(T5[1][2]);
	v3.SetX(T5[2][0]);v3.SetY(T5[2][1]);v3.SetZ(T5[2][2]);

	v1.Normalize();
	v2.Normalize();
	v3.Normalize();

	v = v1;
	v.Cross(v2);

	// right-hand-system check
	if ( v.Dot(v3) < 0.0 )
		v3 *= -1;

	T1.inverse();

	orig.SetX(T5[0][3]);orig.SetY(T5[1][3]);orig.SetZ(T5[2][3]);

	// plot CAD -> local coordinate system

	x.x =  50.0f*(float)v1.X();	x.y =  50.0f*(float)v1.Y();	x.z =  50.0f*(float)v1.Z();
	y.x =  50.0f*(float)v2.X();	y.y =  50.0f*(float)v2.Y();	y.z =  50.0f*(float)v2.Z();
	z.x =  50.0f*(float)v3.X();	z.y =  50.0f*(float)v3.Y();	z.z =  50.0f*(float)v3.Z();

	pnt.x = (float) orig.X();
	pnt.y = (float) orig.Y();
	pnt.z = (float) orig.Z();

	log3d_cad.addSingleArrow(pnt,x,3,1,0,0);
	log3d_cad.addSingleArrow(pnt,y,3,0,1,0);
	log3d_cad.addSingleArrow(pnt,z,3,0,0,1);

	//log3d_cad.addSinglePoint(pnt,6,1,1,1);

	log3d_cad.saveToFile("c:/CAD_CoordSys.iv");

	PointTransform(m_pntCloud_2,T5*T1);

	//m_MeshWork.Transform(T1);
	// plot Mesh -> local coordinate system

	v1.SetX(T1[0][0]);v1.SetY(T1[0][1]);v1.SetZ(T1[0][2]);
	v2.SetX(T1[1][0]);v2.SetY(T1[1][1]);v2.SetZ(T1[1][2]);
	v3.SetX(T1[2][0]);v3.SetY(T1[2][1]);v3.SetZ(T1[2][2]);

	T1.inverse();
	orig.SetX(T1[0][3]);orig.SetY(T1[1][3]);orig.SetZ(T1[2][3]);

	x.x =  50.0f*(float)v1.X();	x.y =  50.0f*(float)v1.Y();	x.z =  50.0f*(float)v1.Z();
	y.x =  50.0f*(float)v2.X();	y.y =  50.0f*(float)v2.Y();	y.z =  50.0f*(float)v2.Z();
	z.x =  50.0f*(float)v3.X();	z.y =  50.0f*(float)v3.Y();	z.z =  50.0f*(float)v3.Z();

	pnt.x = (float) orig.X();
	pnt.y = (float) orig.Y();
	pnt.z = (float) orig.Z();

	log3d_mesh.addSingleArrow(pnt,x,3,1,0,0);log3d_mesh.addSingleArrow(pnt,y,3,0,1,0);log3d_mesh.addSingleArrow(pnt,z,3,0,0,1);
	log3d_mesh.addSinglePoint(0,0,0,20,1,1,1); // plotte Ursprung
	//log3d_mesh.addSinglePoint(pnt,6,0,0,0);
	log3d_mesh.saveToFile("c:/Mesh_CoordSys.iv");

	/*for(int i=0; i< m_pntCloud_2.size(); i++)
	{
		m_pntCloud_2[i].x = m_pntCloud_2[i].x + Grav_1.x - Grav_2.x;
		m_pntCloud_2[i].y = m_pntCloud_2[i].y + Grav_1.y - Grav_2.y;
		m_pntCloud_2[i].z = m_pntCloud_2[i].z + Grav_1.z - Grav_2.z;
	}*/

	return true;
}
bool best_fit::MeshFit_Coarse()
{

    GProp_GProps prop;
    GProp_PrincipalProps pprop;

    Base::Vector3f pnt(0.0,0.0,0.0);
    Base::Vector3f x,y,z;
    Base::Builder3D log3d_mesh, log3d_cad;
    gp_Pnt orig;

    gp_Vec v1,v2,v3,v,vec; // Hauptachsenrichtungen
    gp_Trsf trafo;

   /* BRepGProp::SurfaceProperties(m_Cad, prop);
    pprop = prop.PrincipalProperties();

    v1 = pprop.FirstAxisOfInertia();
    v2 = pprop.SecondAxisOfInertia();
    v3 = pprop.ThirdAxisOfInertia();*/

	MeshCore::MeshEigensystem pca(m_CadMesh);
    pca.Evaluate();
    Base::Matrix4D T5 =  pca.Transform();

	v1.SetX(T5[0][0]);v1.SetY(T5[0][1]);v1.SetZ(T5[0][2]);
    v2.SetX(T5[1][0]);v2.SetY(T5[1][1]);v2.SetZ(T5[1][2]);
    v3.SetX(T5[2][0]);v3.SetY(T5[2][1]);v3.SetZ(T5[2][2]);

    v1.Normalize();
    v2.Normalize();
    v3.Normalize();


    v = v1;
    v.Cross(v2);

    // right-hand-system check
    if ( v.Dot(v3) < 0.0 )
        v3 *= -1;

	T5.inverse();

    orig.SetX(T5[0][3]);orig.SetY(T5[1][3]);orig.SetZ(T5[2][3]);
    //orig  = prop.CentreOfMass();

    // plot CAD -> local coordinate system

	x.x =  50.0f*(float)v1.X();	x.y =  50.0f*(float)v1.Y();	x.z =  50.0f*(float)v1.Z();
    y.x =  50.0f*(float)v2.X();	y.y =  50.0f*(float)v2.Y();	y.z =  50.0f*(float)v2.Z();
    z.x =  50.0f*(float)v3.X();	z.y =  50.0f*(float)v3.Y();	z.z =  50.0f*(float)v3.Z();

    pnt.x = (float) orig.X();
	pnt.y = (float) orig.Y();
	pnt.z = (float) orig.Z();

    log3d_cad.addSingleArrow(pnt,x,3,1,0,0);
	log3d_cad.addSingleArrow(pnt,y,3,0,1,0);
	log3d_cad.addSingleArrow(pnt,z,3,0,0,1);
	
	//log3d_cad.addSinglePoint(pnt,6,1,1,1);
	
	log3d_cad.saveToFile("c:/CAD_CoordSys.iv");

    MeshCore::MeshEigensystem pca2(m_MeshWork);
    pca2.Evaluate();
    Base::Matrix4D T1 =  pca2.Transform();
    m_MeshWork.Transform(T5*T1);
	//m_MeshWork.Transform(T1);
    // plot Mesh -> local coordinate system

	
    v1.SetX(T1[0][0]);v1.SetY(T1[0][1]);v1.SetZ(T1[0][2]);
    v2.SetX(T1[1][0]);v2.SetY(T1[1][1]);v2.SetZ(T1[1][2]);
    v3.SetX(T1[2][0]);v3.SetY(T1[2][1]);v3.SetZ(T1[2][2]);


    
    T1.inverse();
    orig.SetX(T1[0][3]);orig.SetY(T1[1][3]);orig.SetZ(T1[2][3]);

	x.x = 50.0f*(float)v1.X();	x.y = 50.0f*(float)v1.Y();	x.z = 50.0f*(float)v1.Z();
    y.x = 50.0f*(float)v2.X();	y.y = 50.0f*(float)v2.Y();	y.z = 50.0f*(float)v2.Z();
    z.x = 50.0f*(float)v3.X();	z.y = 50.0f*(float)v3.Y();	z.z = 50.0f*(float)v3.Z();

    pnt.x = (float) orig.X();
	pnt.y = (float) orig.Y();
	pnt.z = (float) orig.Z();

	log3d_mesh.addSingleArrow(pnt,x,3,1,0,0);log3d_mesh.addSingleArrow(pnt,y,3,0,1,0);log3d_mesh.addSingleArrow(pnt,z,3,0,0,1);
	log3d_mesh.addSinglePoint(0,0,0,20,1,1,1); // plotte Ursprung
    //log3d_mesh.addSinglePoint(pnt,6,0,0,0);
	log3d_mesh.saveToFile("c:/Mesh_CoordSys.iv");

    return true;
}

bool best_fit::ShapeFit_Coarse()
{
    GProp_GProps prop;
    BRepGProp    SurfProp;
    gp_Trsf      trafo;
    gp_Pnt       orig;

    SurfProp.SurfaceProperties(m_Cad, prop);
    orig  = prop.CentreOfMass();

    // CAD-Mesh -> zurück zum Ursprung
    m_cad2orig.SetX(-orig.X());
    m_cad2orig.SetY(-orig.Y());
    m_cad2orig.SetZ(-orig.Z());

    trafo.SetTranslation(m_cad2orig);
    BRepBuilderAPI_Transform trsf(trafo);

    trsf.Perform(m_Cad);
    m_Cad = trsf.Shape();

    return true;
}

bool best_fit::Tesselate_Face(const TopoDS_Face &aface, MeshCore::MeshKernel &mesh, float deflection)
{
    Base::Builder3D aBuild;
    MeshCore::MeshBuilder builder(mesh);
    builder.Initialize(1000);
    Base::Vector3f Points[3];
    if (!BRepTools::Triangulation(aface,0.1))
    {
        // removes all the triangulations of the faces ,
        // and all the polygons on the triangulations of the edges:
        BRepTools::Clean(aface);

        // adds a triangulation of the shape aShape with the deflection aDeflection:
        //BRepMesh_IncrementalMesh Mesh(pcShape->getShape(),aDeflection);

/*The next two lines have been from the occ6.2 adapt mesh library. They do not work within OCC6.3
      TriangleAdapt_Parameters MeshingParams;
       BRepMeshAdapt::Mesh(aface,deflection,MeshingParams);
																*/
	   BRepMesh_IncrementalMesh Mesh(aface,deflection);
    }
    TopLoc_Location aLocation;
    // takes the triangulation of the face aFace:
    Handle_Poly_Triangulation aTr = BRep_Tool::Triangulation(aface,aLocation);
    if (!aTr.IsNull()) // if this triangulation is not NULL
    {
        // takes the array of nodes for this triangulation:
        const TColgp_Array1OfPnt& aNodes = aTr->Nodes();
        // takes the array of triangles for this triangulation:
        const Poly_Array1OfTriangle& triangles = aTr->Triangles();
        // create array of node points in absolute coordinate system
        TColgp_Array1OfPnt aPoints(1, aNodes.Length());
        for ( Standard_Integer i = 1; i < aNodes.Length()+1; i++)
            aPoints(i) = aNodes(i).Transformed(aLocation);
        // Takes the node points of each triangle of this triangulation.
        // takes a number of triangles:
        Standard_Integer nnn = aTr->NbTriangles();
        Standard_Integer nt,n1,n2,n3;
        for ( nt = 1 ; nt < nnn+1 ; nt++)
        {
            // takes the node indices of each triangle in n1,n2,n3:
            triangles(nt).Get(n1,n2,n3);
            // takes the node points:
            gp_Pnt aPnt1 = aPoints(n1);
            Points[0].Set(float(aPnt1.X()),float(aPnt1.Y()),float(aPnt1.Z()));
            gp_Pnt aPnt2 = aPoints(n2);
            Points[1].Set((float) aPnt2.X(),(float) aPnt2.Y(),(float) aPnt2.Z());
            gp_Pnt aPnt3 = aPoints(n3);
            Points[2].Set((float) aPnt3.X(),(float) aPnt3.Y(),(float) aPnt3.Z());
            // give the occ faces to the internal mesh structure of freecad
            MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);
            builder.AddFacet(Face);
        }

    }
    // if the triangulation of only one face is not possible to get
    else
    {
        throw Base::RuntimeError("Empty face triangulation\n");
    }

    // finish FreeCAD Mesh Builder and exit with new mesh
    builder.Finish();
    return true;
}


bool best_fit::Tesselate_Shape(const TopoDS_Shape &shape, MeshCore::MeshKernel &mesh, float deflection)
{
    Base::Builder3D aBuild;

    MeshCore::MeshDefinitions::_fMinPointDistanceD1 = (float) 0.0001;
    MeshCore::MeshBuilder builder(mesh);
    builder.Initialize(1000);
    Base::Vector3f Points[3];

    //bool check = BRepUtils::CheckTopologie(shape);
    //if (!check)
    //{
    //    cout <<"an error"<< endl;
    //}

    //BRepBuilderAPI_Sewing aSewer;
    //aSewer.Load(shape);
    //aSewer.Perform();
    //aSewer.IsModified(shape);
    //const TopoDS_Shape& asewedShape = aSewer.SewedShape();
    //if (asewedShape.IsNull())
    //{
    //    cout << "Nothing Sewed" << endl;
    //}
    //int test = aSewer.NbFreeEdges();
    //int test1 = aSewer.NbMultipleEdges();
    //int test2 = aSewer.NbDegeneratedShapes();

    // adds a triangulation of the shape aShape with the deflection deflection:
/*
    TriangleAdapt_Parameters MeshParams;
    MeshParams._minAngle = 30.0;
    //MeshParams._minNbPntsPerEdgeLine = 3;
    //MeshParams._minNbPntsPerEdgeOther = 3;
    //MeshParams._minEdgeSplit = 3;
	MeshParams._maxTriangleSideSize = 10; //10
	MeshParams._maxArea = 10; //50
	*/
    BRepMesh_IncrementalMesh Mesh(shape,deflection);
    //BRepMesh::Mesh(shape,deflection);
    TopExp_Explorer aExpFace;


    for (aExpFace.Init(shape,TopAbs_FACE);aExpFace.More();aExpFace.Next())
    {

        TopoDS_Face aFace = TopoDS::Face(aExpFace.Current());

        TopLoc_Location aLocation;

        // takes the triangulation of the face aFace:
        Handle_Poly_Triangulation aTr = BRep_Tool::Triangulation(aFace,aLocation);
        if (!aTr.IsNull()) // if this triangulation is not NULL
        {
            // takes the array of nodes for this triangulation:
            const TColgp_Array1OfPnt& aNodes = aTr->Nodes();
            // takes the array of triangles for this triangulation:
            const Poly_Array1OfTriangle& triangles = aTr->Triangles();
            // create array of node points in absolute coordinate system
            TColgp_Array1OfPnt aPoints(1, aNodes.Length());
            for ( Standard_Integer i = 1; i <= aNodes.Length(); i++)
                aPoints(i) = aNodes(i).Transformed(aLocation);
            // Takes the node points of each triangle of this triangulation.
            // takes a number of triangles:
            Standard_Integer nnn = aTr->NbTriangles();
            Standard_Integer nt,n1,n2,n3;
            for ( nt = 1 ; nt < nnn+1 ; nt++)
            {
                // takes the node indices of each triangle in n1,n2,n3:
                triangles(nt).Get(n1,n2,n3);
                // takes the node points:
                gp_Pnt aPnt1 = aPoints(n1);
                Points[0].Set(float(aPnt1.X()),float(aPnt1.Y()),float(aPnt1.Z()));
                gp_Pnt aPnt2 = aPoints(n2);
                Points[1].Set((float) aPnt2.X(),(float) aPnt2.Y(),(float) aPnt2.Z());
                gp_Pnt aPnt3 = aPoints(n3);
                Points[2].Set((float) aPnt3.X(),(float) aPnt3.Y(),(float) aPnt3.Z());
                // give the occ faces to the internal mesh structure of freecad
                MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);
                builder.AddFacet(Face);
            }
        }
        // if the triangulation of only one face is not possible to get
        else
        {
            throw Base::RuntimeError("Empty face triangulation\n");
        }
    }
    // finish FreeCAD Mesh Builder and exit with new mesh
    builder.Finish();

    /*MeshCore::MeshAlgorithm algo(mesh);
    std::list< std::vector <unsigned long> > BoundariesIndex;
    algo.GetMeshBorders(BoundariesIndex);*/

    return true;
}

std::vector<Base::Vector3f> best_fit::Comp_Normals(MeshCore::MeshKernel &M)
{
    Base::Builder3D log3d;
    Base::Vector3f normal,local_normal,origPoint;
    MeshCore::MeshRefPointToFacets rf2pt(M);
    MeshCore::MeshGeomFacet        t_face;
    MeshCore::MeshPoint mPnt;
    std::vector<Base::Vector3f>    normals;

    int NumOfPoints = M.CountPoints();
    float local_Area;
    float fArea;

    for (int i=0; i<NumOfPoints; ++i)
    {
        // Satz von Dreiecken zu jedem Punkt
        mPnt = M.GetPoint(i);
        origPoint.x = mPnt.x;
        origPoint.y = mPnt.y;
        origPoint.z = mPnt.z;

        const std::set<unsigned long>& faceSet = rf2pt[i];
        fArea = 0.0;
        normal.Set(0.0,0.0,0.0);

        // Iteriere über die Dreiecke zu jedem Punkt
        for (std::set<unsigned long>::const_iterator it = faceSet.begin(); it != faceSet.end(); ++it)
        {
            // Zweimal derefernzieren, um an das MeshFacet zu kommen und dem Kernel uebergeben, dass er ein MeshGeomFacet liefert
            t_face = M.GetFacet(*it);
            // Flaecheninhalt aufsummieren
            local_Area = t_face.Area();
            local_normal = t_face.GetNormal();
            if (local_normal.z < 0)
            {
                local_normal = local_normal * (-1);
            }

            fArea  = fArea  + local_Area;
            normal = normal + local_Area*local_normal;

        }

        normal.Normalize();
        normals.push_back(normal);

        log3d.addSingleArrow(origPoint,origPoint+normal,1,0,0,0);
    }

    log3d.saveToFile("c:/normals.iv");

    return normals;
}

/*
double best_fit::CompError(std::vector<Base::Vector3f> &pnts, std::vector<Base::Vector3f> &normals)
{
    double err_avg = 0.0;
    double err_max = 0.0;
    double sqrdis = 0.0;

    MeshCore::MeshFacetGrid aFacetGrid(m_CadMesh);
    MeshCore::MeshAlgorithm malg(m_CadMesh);
    MeshCore::MeshAlgorithm malg2(m_CadMesh);

    Base::Vector3f origPoint, projPoint, distVec;
    unsigned long  facetIndex;

    int NumOfPoints = pnts.size();
    int c = 0;

    for (int i=0; i<NumOfPoints; ++i)
    {
        if (!malg.NearestFacetOnRay(pnts[i], normals[i], aFacetGrid, projPoint, facetIndex))  // gridoptimiert
        {
            if (malg2.NearestFacetOnRay(pnts[i], normals[i], projPoint, facetIndex))
            {
                distVec  = projPoint-pnts[i];
                sqrdis   = distVec*distVec;
                err_avg += sqrdis;
            }
            else
                ++c;
        }
        else
        {
            distVec  = projPoint-pnts[i];
            sqrdis   = distVec*distVec;
            err_avg += sqrdis;
        }
    }

    if (c==NumOfPoints)
        return 1e+10;

    return err_avg/(NumOfPoints-c);
}

std::vector<double> best_fit::CompError_GetPnts(std::vector<Base::Vector3f> pnts,
        std::vector<Base::Vector3f> &normals)

{
    double err_avg = 0.0;
    double dist;

    std::vector<double> errVec(2);    // errVec[0]: avg. error, errVec[1]: max. error

    MeshCore::MeshFacetGrid aFacetGrid(m_Mesh);
    MeshCore::MeshAlgorithm malg(m_Mesh);
    unsigned long  facetIndex;

    Base::Vector3f origPoint, projPoint, distVec;
    //Base::Builder3D log;

    unsigned int NumOfPoints = pnts.size();
    int c = 0;

    m_LSPnts[0].clear();
    m_LSPnts[1].clear();

    m_weights_loc.clear();

    for (unsigned int i=0; i<NumOfPoints; ++i)
    {
        if (!malg.NearestFacetOnRay(pnts[i], normals[i], aFacetGrid, projPoint, facetIndex))
               // !Intersect(normals[i], *m_Mesh, pnts[i], projPoint)  // gridoptimiert
        {
            ++c;
        }
        else
        {
            m_LSPnts[0].push_back(projPoint);
            m_LSPnts[1].push_back(pnts[i]);
            m_weights_loc.push_back(m_weights[i]);

            //log.addSingleArrow(pnts[i], projPoint);
        }
    }

    double max_err = 0.0;

    for (unsigned int i=0; i<NumOfPoints-c; ++i)
    {
        distVec  = m_LSPnts[0][i]-m_LSPnts[1][i];
        dist     = distVec*distVec;
        err_avg += dist;

        if (dist > max_err)
            max_err = dist;
    }

    //log.saveToFile("c:/intersection.iv");

    errVec[0] = err_avg /= (NumOfPoints-c);  // durchschnittlicher Fehlerquadrat
    errVec[1] = max_err;         // maximaler Fehlerquadrat

    //cout << c << " projections failed" << endl;

    return errVec;
}

double best_fit::CompError(std::vector<Base::Vector3f> &pnts, std::vector<Base::Vector3f> &normals, bool plot)
{
    if (plot==false)
        return CompError(pnts, normals);
    else
    {
        Base::Builder3D log3d;
        double err_avg = 0.0;
        double err_max = 0.0;
        double sqrdis  = 0.0;


        MeshCore::MeshFacetGrid aFacetGrid(m_CadMesh);
        MeshCore::MeshAlgorithm malg(m_CadMesh);
        MeshCore::MeshAlgorithm malg2(m_CadMesh);

        Base::Vector3f projPoint, distVec;
        unsigned long  facetIndex;

        int NumOfPoints = pnts.size();
        int c=0;

        for (int i=0; i<NumOfPoints; ++i)
        {
            if (!malg.NearestFacetOnRay(pnts[i], normals[i], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
            {
                if (malg2.NearestFacetOnRay(pnts[i], normals[i], projPoint, facetIndex))
                {

                    log3d.addSingleArrow(pnts[i],projPoint, 3, 0,0,0);
                    distVec  = projPoint-pnts[i];
                    sqrdis   = distVec*distVec;
                    err_avg += sqrdis;
                }
                else
                    c++;

            }
            else
            {
                log3d.addSingleArrow(pnts[i],projPoint, 3, 0,0,0);
                distVec  = projPoint-pnts[i];
                sqrdis   = distVec*distVec;
                err_avg += sqrdis;
            }
        }

        log3d.saveToFile("c:/projection.iv");

        if (c>(NumOfPoints/2))
            return 1e+10;

        return err_avg/(NumOfPoints-c);
    }
}
*/

double best_fit::CompTotalError()
{
    Base::Builder3D log3d;
    double err_avg = 0.0;
    double err_max = 0.0;
    double sqrdis  = 0.0;

    std::vector<int> FailProj;

    MeshCore::MeshFacetGrid aFacetGrid(m_Mesh,10);
    MeshCore::MeshAlgorithm malg(m_Mesh);
    MeshCore::MeshAlgorithm malg2(m_Mesh);
    MeshCore::MeshPointIterator p_it(m_CadMesh);

    Base::Vector3f projPoint, distVec, projPoint2;
    unsigned long  facetIndex;
    std::stringstream text;
    m_error.resize(m_CadMesh.CountPoints());

    unsigned int c=0;
    int i=0;

	
    m_LSPnts[0].clear();
    m_LSPnts[1].clear();
    for (p_it.Begin(); p_it.More(); p_it.Next())
    {
        if (malg.NearestFacetOnRay(*p_it, m_normals[i], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
        {
            log3d.addSingleArrow(*p_it, projPoint, 3, 0,0,0);
            distVec  = projPoint - *p_it;
            sqrdis   = distVec*distVec;

			m_LSPnts[1].push_back(*p_it);
            m_LSPnts[0].push_back(projPoint);

            if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
                m_error[i] = sqrt(sqrdis);
            else
                m_error[i] = -sqrt(sqrdis);

            err_avg += sqrdis;
        }
        else
        {

            if (!malg2.NearestFacetOnRay(*p_it, m_normals[i], projPoint, facetIndex))   // nicht gridoptimiert
            {
                c++;
                //m_normals[i].Scale(-10,-10,-10);
                text << p_it->x << ", " << p_it->y << ", " << p_it->z << "; " << m_normals[i].x << ", " << m_normals[i].y << ", " << m_normals[i].z;
                //log3d.addSingleArrow(*p_it, *p_it + m_normals[i], 4, 1,0,0);
                //log3d.addText(*p_it,(text.str()).c_str());
            }
            else
			{
				log3d.addSingleArrow(*p_it, projPoint, 3, 0,0,0);
				distVec  = projPoint - *p_it;
				sqrdis   = distVec*distVec;

				m_LSPnts[1].push_back(*p_it);
				m_LSPnts[0].push_back(projPoint);

				if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
					m_error[i] = sqrt(sqrdis);
				else
					m_error[i] = -sqrt(sqrdis);

				err_avg += sqrdis;
			}
		}
        

        ++i;
    }

    //for (p_it.Begin(); p_it.More(); p_it.Next())
//   {
    //    if (!malg.NearestFacetOnRay(*p_it, m_normals[i], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
//       {
//           if (malg2.NearestFacetOnRay(*p_it, m_normals[i], projPoint, facetIndex))
//           {

//               log3d.addSingleArrow(*p_it, projPoint, 3, 0,0,0);
//               distVec  = projPoint - *p_it;
//               sqrdis   = distVec*distVec;

//               if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
//                   m_error[i] = sqrt(sqrdis);
//               else
//                   m_error[i] = -sqrt(sqrdis);

//               err_avg += sqrdis;
//           }
//           else
//           {
//               c++;
//               FailProj.push_back(i);
//           }
//       }
//       else
//       {
//           distVec  = projPoint-*p_it;
//           sqrdis   = distVec*distVec;

//           m_normals[i].Scale(-1,-1,-1);

//           if (malg.NearestFacetOnRay(*p_it, m_normals[i], aFacetGrid, projPoint2, facetIndex))
//           {
//               distVec  = projPoint2-*p_it;
//               if (sqrdis > distVec*distVec)
//               {
//                   sqrdis   = distVec*distVec;
//                   log3d.addSingleArrow(*p_it, projPoint2, 3, 0,0,0);
//               }
//               else
//               {
//                   log3d.addSingleArrow(*p_it, projPoint, 3, 0,0,0);
//               }

//           }
//           m_normals[p_it.Position()].Scale(-1,-1,-1);

//           if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
//               m_error[i] = sqrt(sqrdis);
//           else
//               m_error[i] = -sqrt(sqrdis);

//           err_avg += sqrdis;
//       }
//       ++i;
    //}


    MeshCore::MeshRefPointToPoints vv_it(m_CadMesh);
    MeshCore::MeshPointArray::_TConstIterator v_beg = m_CadMesh.GetPoints().begin();

    std::set<unsigned long>::const_iterator v_it;
    for (unsigned int i=0; i<FailProj.size(); ++i)
    {
        const std::set<unsigned long>& PntNei = vv_it[FailProj[i]];
        m_error[FailProj[i]] = 0.0;

        for (v_it = PntNei.begin(); v_it !=PntNei.end(); ++v_it)
        {
            m_error[FailProj[i]] += m_error[v_beg[*v_it]._ulProp];
        }

        m_error[FailProj[i]] /= double(PntNei.size());
    }


    log3d.saveToFile("c:/projection.iv");

    if (c>(m_CadMesh.CountPoints()/2))
        return 1e+10;

    return err_avg/(m_CadMesh.CountPoints()-c);

}

double best_fit::CompTotalError(MeshCore::MeshKernel &mesh)
{
    double err_avg = 0.0;
    double err_max = 0.0;
    double sqrdis  = 0.0;

    std::vector<int> FailProj;

    MeshCore::MeshFacetGrid aFacetGrid(mesh,10);
    MeshCore::MeshAlgorithm malg(mesh);
    MeshCore::MeshAlgorithm malg2(mesh);
    MeshCore::MeshPointIterator p_it(m_CadMesh);

    Base::Vector3f projPoint, distVec, projPoint2;
    unsigned long  facetIndex;
    std::stringstream text;
    m_error.resize(m_CadMesh.CountPoints());

    unsigned int c=0;
    int i=0;

    for (p_it.Begin(); p_it.More(); p_it.Next())
    {
        if (malg.NearestFacetOnRay(*p_it, m_normals[i], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
        {
            distVec  = projPoint - *p_it;
            sqrdis   = distVec*distVec;

            if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
                m_error[i] += sqrt(sqrdis);
            else
                m_error[i] += -sqrt(sqrdis);

            err_avg += sqrdis;
        }
        else
        {

            if (!malg2.NearestFacetOnRay(*p_it, m_normals[i], projPoint, facetIndex))   // nicht gridoptimiert
            {
                c++;
				FailProj.push_back(i);
            }
            else
			{
				distVec  = projPoint - *p_it;
				sqrdis   = distVec*distVec;

				if (((projPoint.z - p_it->z) / m_normals[i].z ) > 0)
					m_error[i] += sqrt(sqrdis);
				else
					m_error[i] += -sqrt(sqrdis);

				err_avg += sqrdis;
			}
		}
        

        ++i;
    }

    MeshCore::MeshRefPointToPoints vv_it(m_CadMesh);
    MeshCore::MeshPointArray::_TConstIterator v_beg = m_CadMesh.GetPoints().begin();

	double error;
	std::set<unsigned long>::const_iterator v_it;
    for (unsigned int i=0; i<FailProj.size(); ++i)
    {
        const std::set<unsigned long>& PntNei = vv_it[FailProj[i]];
		error = 0.0;


        for (v_it = PntNei.begin(); v_it !=PntNei.end(); ++v_it)
        {
			error += m_error[v_beg[*v_it]._ulProp];
        }

		error /= double(PntNei.size());
        m_error[FailProj[i]] += error;
    }


    if (c>(m_CadMesh.CountPoints()/2))
        return 1e+10;

    return err_avg/(m_CadMesh.CountPoints()-c);
}
