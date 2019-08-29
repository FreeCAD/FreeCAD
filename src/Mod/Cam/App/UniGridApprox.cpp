/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *   Human Rezai <Human@web.de>                                            *
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

#include "PreCompiled.h"
#include "UniGridApprox.h"

#include "best_fit.h"

#include <Mod/Mesh/App/Core/Grid.h>
#include <Base/Builder3D.h>

#include <TColgp_Array2OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Handle_Geom_BSplineSurface.hxx>

//
///*********BINDINGS********/
#include <boost/numeric/bindings/traits/ublas_matrix.hpp>
#include <boost/numeric/bindings/atlas/cblas.hpp>
#include <boost/numeric/bindings/atlas/clapack.hpp>
//
using namespace boost::numeric::bindings;

typedef ublas::matrix<double, ublas::column_major> cm_t;
typedef ublas::symmetric_adaptor<cm_t, ublas::upper> adapt;

UniGridApprox::UniGridApprox(const MeshCore::MeshKernel &mesh, double Tol)
        :m_Mesh(mesh),m_Tol(Tol),m_udeg(3),m_vdeg(3)
{
}

UniGridApprox::~UniGridApprox()
{
}

bool UniGridApprox::Perform(double TOL)
{

    double maxErr;
    cout << "MeshOffset" << endl;
    MeshOffset();

    cout << "SurfMeshParam" << endl;
    SurfMeshParam();

    while (true)
    {
        cout << "CompKnots" << endl;
        CompKnots(uCP, vCP);

        cout << "MatComp" << endl;
        MatComp(uCP, vCP);

        BuildSurf();

        cout << "Compute Error";
        maxErr = CompMeshError();

        if (maxErr == -1)
            throw Base::RuntimeError("CompError() couldn't project one point...");

        cout << " -> " << maxErr << endl;

        if (maxErr <= TOL) break;
        else
        {
            if (uCP >= vCP)
            {
                uCP += 10;
                vCP += vCP*10/uCP;
            }
            else
            {
                vCP += 10;
                uCP += uCP*10/vCP;
            }
        }

        if ( (uCP > (n_x + m_udeg + 1)) || (vCP > (n_y + m_vdeg + 1)) ) break;

        m_Grid.clear();
        m_Grid = m_GridCopy;
    }

    return true;
}

bool UniGridApprox::MeshOffset()
{

    MeshCore::MeshPointIterator p_it(m_Mesh);

    //vorläufige Lösung bis CAD-Normalen verwendet werden können
    std::vector<Base::Vector3f> normals = best_fit::Comp_Normals(m_Mesh);

    double x_max=-(1e+10),y_max=-(1e+10),z_max=-(1e+10),x_min=1e+10,y_min=1e+10,st_x,st_y;
    int n = normals.size();

    // führe verschiebung durch

    //for(int i=0; i<n; ++i)
    //{
    // normals[i].Normalize();
    // normals[i].Scale(m_offset,m_offset,m_offset);
    // m_Mesh.MovePoint(i,normals[i]);
    //}

    // erzeuge nun ein uniformes Rechtecksgitter auf dem CAD-Netz
    m_Mesh.RecalcBoundBox();

    for (p_it.Begin(); p_it.More(); p_it.Next())
    {
        if (p_it->z>z_max) z_max = p_it->z;
        if (p_it->x>x_max) x_max = p_it->x;
        if (p_it->x<x_min) x_min = p_it->x;
        if (p_it->y>y_max) y_max = p_it->y;
        if (p_it->y<y_min) y_min = p_it->y;
    }

    // gittergrößen bestimmung über die bounding-box
    n_x = int((x_max - x_min)/(y_max - y_min)*sqrt((x_max - x_min)*(y_max - y_min)));
    n_y = int((y_max - y_min)/(x_max - x_min)*sqrt((x_max - x_min)*(y_max - y_min)));

    st_x = (x_max - x_min)/n_x;
    st_y = (y_max - y_min)/n_y;

    uCP = n_x/10;
    vCP = n_y/10;

    m_Grid.resize(n_x+1);
    for (int i=0; i<n_x+1; ++i)
        m_Grid[i].resize(n_y+1);

    unsigned long  facetIndex;
    MeshCore::MeshFacetGrid aFacetGrid(m_Mesh);
    MeshCore::MeshAlgorithm malg(m_Mesh);
    MeshCore::MeshAlgorithm malg2(m_Mesh);
    Base::Vector3f  projPoint, pnt, aNormal(0,0,1.0);
    Base::Builder3D log3d;

    pnt.z = float(z_max);

    //gp_Pnt p;
    //TColgp_Array2OfPnt Input(1,n_x+1,1,n_y+1);

    for (int i=0; i<n_x+1; ++i)
    {
        cout << double(i)/double(n_x) << endl;
        pnt.x = float(x_min + i*st_x);
        for (int j=0; j<n_y+1 - 10; ++j)
        {
            pnt.y = float(y_min + j*st_y);
            aNormal.z = 1.0;
            if (!malg.NearestFacetOnRay(pnt, aNormal, aFacetGrid, projPoint, facetIndex))
            {
                aNormal.Scale(1,1,-1);// gridoptimiert
                if (!malg.NearestFacetOnRay(pnt, aNormal, aFacetGrid, projPoint, facetIndex))
                {
                    aNormal.Scale(1,1,-1);
                    if (!malg2.NearestFacetOnRay(pnt, aNormal, projPoint, facetIndex))
                    {
                        aNormal.Scale(1,1,-1);
                        if (!malg2.NearestFacetOnRay(pnt, aNormal, projPoint, facetIndex))
                        {
                            if (i != 0 && i !=n_x && j != 0 && j!= n_y)
                            {
                                pnt.x += float(st_x / 10.0);
                                aNormal.Scale(1,1,-1);
                                if (malg.NearestFacetOnRay(pnt, aNormal, aFacetGrid, projPoint, facetIndex))
                                {
                                    log3d.addSinglePoint(projPoint, 3, 1,0,0);
                                    m_Grid[i][j] = projPoint;

                                }
                                else
                                {
                                    log3d.addSinglePoint(pnt, 3, 0,0,0);
                                    pnt.z = 1e+10;
                                    m_Grid[i][j] = pnt;
                                    pnt.z = (float) z_max;
                                }
                            }
                            else
                            {
                                log3d.addSinglePoint(pnt, 3, 0,0,0);
                                m_Grid[i][j] = pnt;
                            }
                        }
                        else
                        {
                            log3d.addSinglePoint(projPoint, 3, 1,1,1);
                            m_Grid[i][j] = projPoint;
                        }
                    }
                    else
                    {
                        log3d.addSinglePoint(projPoint, 3, 1,1,1);
                        m_Grid[i][j] = projPoint;
                    }
                }
                else
                {
                    log3d.addSinglePoint(projPoint, 3, 1,1,1);
                    m_Grid[i][j] = projPoint;
                }
            }
            else
            {
                log3d.addSinglePoint(projPoint, 3, 1,1,1);
                m_Grid[i][j] = projPoint;
            }
        }
    }

    int c=0;
    for (int i=0; i<n_x+1; ++i)
    {
        for (int j=0; j<n_y+1; ++j)
        {
            c=0;
            if (m_Grid[i][j].z == 1e+10)
            {

                m_Grid[i][j].x = 0;
                m_Grid[i][j].y = 0;
                m_Grid[i][j].z = 0;

                if (m_Grid[i-1][j].z != 1e+10)
                {
                    m_Grid[i][j] += (m_Grid[i-1][j]);
                    c++;
                }

                if (m_Grid[i][j-1].z != 1e+10)
                {
                    m_Grid[i][j] += (m_Grid[i][j-1]);
                    c++;
                }

                if (m_Grid[i][j+1].z != 1e+10)
                {
                    m_Grid[i][j] += (m_Grid[i][j+1]);
                    c++;
                }

                if (m_Grid[i+1][j].z != 1e+10)
                {
                    m_Grid[i][j] += (m_Grid[i+1][j]);
                    c++;
                }

                m_Grid[i][j].Scale( float(1.0 / double(c)), float(1.0 / double(c)), float(1.0 / double(c)));;
                log3d.addSinglePoint(m_Grid[i][j], 3, 0,0,0);
            }
        }
    }

    m_GridCopy = m_Grid;

    log3d.saveToFile("c:/projection.iv");

    return true;
}

bool UniGridApprox::SurfMeshParam()
{
    // hier wird das in MeshOffset erzeugte gitter parametrisiert
    // parametrisierung: (x,y) -> (u,v)  ,  ( R x R ) -> ( [0,1] x [0,1] )

    int n = m_Grid.size()-1;      // anzahl der zu approximierenden punkte in x-richtung
    int m = m_Grid[0].size()-1;   // anzahl der zu approximierenden punkte in y-richtung

    std::vector<double> dist_x, dist_y;
    double sum,d;
    Base::Vector3f vlen;

    m_uParam.clear();
    m_vParam.clear();
    m_uParam.resize(n+1);
    m_vParam.resize(m+1);
    m_uParam[n] = 1.0;
    m_vParam[m] = 1.0;

    // berechne knotenvektor in u-richtung (entspricht x-richtung)
    for (int j=0; j<m+1; ++j)
    {
        sum = 0.0;
        dist_x.clear();
        for (int i=0; i<n; ++i)
        {
            vlen = (m_Grid[i+1][j] - m_Grid[i][j]);
            dist_x.push_back(vlen.Length());
            sum += dist_x[i];
        }
        d = 0.0;
        for (int i=0; i<n-1; ++i)
        {
            d += dist_x[i];
            m_uParam[i+1] = m_uParam[i+1] + d/sum;
        }
    }

    for (int i=0; i<n; ++i)
        m_uParam[i] /= m+1;

    // berechne knotenvektor in v-richtung (entspricht y-richtung)
    for (int i=0; i<n+1; ++i)
    {
        sum = 0.0;
        dist_y.clear();
        for (int j=0; j<m; ++j)
        {
            vlen = (m_Grid[i][j+1] - m_Grid[i][j]);
            dist_y.push_back(vlen.Length());
            sum += dist_y[j];
        }
        d = 0.0;
        for (int j=0; j<m-1; ++j)
        {
            d += dist_y[j];
            m_vParam[j+1] = m_vParam[j+1] + d/sum;
        }
    }

    for (int j=0; j<m; ++j)
        m_vParam[j] /= n+1;

    /*cout << "uParam:" << endl;
    for(int i=0; i<m_uParam.size(); ++i){
     cout << " " << m_uParam[i] << ", " << endl;
    }

    cout << "vParam:" << endl;
    for(int i=0; i<m_vParam.size(); ++i){
     cout << " " << m_vParam[i] << ", " << endl;
    }*/

    return true;
}

bool UniGridApprox::CompKnots(int u_CP, int v_CP)
{

    // berechnung der knotenvektoren
    // siehe NURBS-BOOK Seite 412

    int r = n_x;
    int s = n_y;

    int n = u_CP-1;
    int m = v_CP-1;

    m_um = u_CP;
    m_vm = v_CP;

    int p = m_udeg;
    int q = m_vdeg;

    // U-Knot Vector Computation
    double d = ((double)r + 1.0)/((double)n - (double)p + 1.0);

    m_uknots.clear();
    m_uknots.resize(n + p + 2);

    for (int i=(p + 1) ; i<(n + p + 2); ++i)
        m_uknots[i] = 1.0;

    int ind;
    double alp;

    for (int i=1; i<(n - p + 1); ++i)
    {

        ind = int(i*d);          // abgerundete ganze zahl
        alp = i*d - ind;    // rest
        m_uknots[p+i] = ((1 - alp) * m_uParam[ind-1]) + (alp * m_uParam[ind]);
    }

    /*for(int i=0; i<m_uknots.size(); ++i){
     cout << " " << m_uknots[i] << ", " << endl;
    }*/

    // V-Knot Vector Computation
    d = ((double)s + 1.0)/((double)m - (double)q + 1.0);

    m_vknots.clear();
    m_vknots.resize(m + q + 2);

    for (int i = (q + 1) ;  i< (m + q + 2); ++i)
        m_vknots[i] = 1.0;

    for (int i=1; i<(m - q + 1); ++i)
    {

        ind = int(i*d);          // abgerundete ganze zahl
        alp = i*d - ind;    // rest
        m_vknots[q+i] = ((1 - alp) * m_vParam[ind-1]) + (alp * m_vParam[ind]);
    }

    /*for(int i=0; i<m_vknots.size(); ++i){
     cout << " " << m_vknots[i] << ", " << endl;
    }*/

    return true;
}

bool UniGridApprox::MatComp(int u_CP, int v_CP)
{
    // hier wird schließlich approximiert

    int r = n_x;
    int s = n_y;

    int n = u_CP-1;
    int m = v_CP-1;

    int p = m_udeg;
    int q = m_vdeg;

    ublas::matrix<double> Nu_full(r - 1, n + 1);
    ublas::matrix<double> Nv_full(s - 1, m + 1);
    ublas::matrix<double> Nu_left(r - 1, n - 1);
    ublas::matrix<double> Nv_left(s - 1, m - 1);
    ublas::matrix<double> Nu     (n - 1, n - 1);
    ublas::matrix<double> Nv     (m - 1, m - 1);

    ublas::matrix<double> bx (1, n - 1);
    ublas::matrix<double> by (1, n - 1);
    ublas::matrix<double> bz (1, n - 1);

    // mit null vorinitialisieren
    for (int i=0; i<r-1; ++i)
        for (int j=0; j<n+1; ++j)
            Nu_full(i,j) = 0.0;

    for (int i=0; i<s-1; ++i)
        for (int j=0; j<m+1; ++j)
            Nv_full(i,j) = 0.0;

    std::vector<double> output(p+1);

    int ind;
    for (int i=1; i<r; ++i)
    {
        output.clear();
        output.resize(p+1);
        ind = Routines::FindSpan(n, p, m_uParam[i],m_uknots);
        Routines::Basisfun(ind,m_uParam[i],p,m_uknots,output);

        for (unsigned int j=0; j<output.size(); ++j)
        {
            Nu_full(i-1,ind-p+j) = output[j];
        }
    }

    for (int i=0; i<r-1; ++i)
    {
        for (int j=0; j<n-1; ++j)
        {
            Nu_left(i,j) = Nu_full(i,j+1);
        }
    }

    //WriteMatrix(Nu_full);

    for (int i=1; i<s; ++i)
    {
        output.clear();
        output.resize(q+1);
        ind = Routines::FindSpan(m, q, m_vParam[i],m_vknots);
        Routines::Basisfun(ind,m_vParam[i],q,m_vknots,output);

        for (unsigned int j=0; j<output.size(); ++j)
        {
            Nv_full(i-1,ind-q+j) = output[j];
        }
    }

    for (int i=0; i<s-1; ++i)
    {
        for (int j=0; j<m-1; ++j)
        {
            Nv_left(i,j) = Nv_full(i,j+1);
        }
    }

    //cout << "NV" << endl;
    //WriteMatrix(Nv_left);

    atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Nu_left,Nu_left, 0.0,Nu);  // Nu_left'*Nu_left = Nu
    atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Nv_left,Nv_left, 0.0,Nv);  // Nv_left'*Nv_left = Nv  !!! Achtung !!!

    std::vector<int> upiv(n - 1);   // pivotelement
    atlas::lu_factor(Nu,upiv);      // führt LU-Zerlegung durch
    std::vector<int> vpiv(m - 1);
    atlas::lu_factor(Nv,vpiv);

    ublas::matrix<double> uCP_x(n + 1, s + 1);
    ublas::matrix<double> uCP_y(n + 1, s + 1);
    ublas::matrix<double> uCP_z(n + 1, s + 1);

    CPx.resize(n + 1, m + 1);
    CPy.resize(n + 1, m + 1);
    CPz.resize(n + 1, m + 1);

    // mit null vorinitialisieren
    for (int i=0; i<n+1; ++i)
        for (int j=0; j<s+1; ++j)
        {
            uCP_x(i,j) = 0.0;
            uCP_y(i,j) = 0.0;
            uCP_z(i,j) = 0.0;
        }

    std::vector< ublas::matrix<double> > Ru_x(s+1);
    std::vector< ublas::matrix<double> > Ru_y(s+1);
    std::vector< ublas::matrix<double> > Ru_z(s+1);

    for (int j=0; j<s+1; ++j)
    {

        Ru_x[j].resize(r-1,1);
        Ru_y[j].resize(r-1,1);
        Ru_z[j].resize(r-1,1);

        uCP_x(0,j) = m_Grid[0][j].x;
        uCP_y(0,j) = m_Grid[0][j].y;
        uCP_z(0,j) = m_Grid[0][j].z;

        uCP_x(n,j) = m_Grid[r][j].x;
        uCP_y(n,j) = m_Grid[r][j].y;
        uCP_z(n,j) = m_Grid[r][j].z;

        for (int k=0; k<r-1; ++k)
        {
            Ru_x[j](k,0) = m_Grid[k+1][j].x - Nu_full(k,0)*m_Grid[0][j].x - Nu_full(k,n)*m_Grid[r][j].x;
            Ru_y[j](k,0) = m_Grid[k+1][j].y - Nu_full(k,0)*m_Grid[0][j].y - Nu_full(k,n)*m_Grid[r][j].y;
            Ru_z[j](k,0) = m_Grid[k+1][j].z - Nu_full(k,0)*m_Grid[0][j].z - Nu_full(k,n)*m_Grid[r][j].z;
        }

        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Ru_x[j],Nu_left, 0.0, bx);
        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Ru_y[j],Nu_left, 0.0, by);
        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Ru_z[j],Nu_left, 0.0, bz);

        atlas::getrs(CblasTrans,Nu,upiv,bx);
        atlas::getrs(CblasTrans,Nu,upiv,by);
        atlas::getrs(CblasTrans,Nu,upiv,bz);

        for (int i=1; i<n; ++i)
        {
            uCP_x(i,j) = bx(0,i-1);
            uCP_y(i,j) = by(0,i-1);
            uCP_z(i,j) = bz(0,i-1);
        }
    }

    Base::Builder3D log3d;
    Base::Vector3f pnt,pnt1;

    for (int j=0; j<s; ++j)
    {
        for (int i=0; i<n; ++i)
        {

            pnt.x = (float) uCP_x(i,j);
            pnt.y = (float) uCP_y(i,j);
            pnt.z = (float) uCP_z(i,j);

            pnt1.x = (float) uCP_x(i+1,j);
            pnt1.y = (float) uCP_y(i+1,j);
            pnt1.z = (float) uCP_z(i+1,j);

            log3d.addSingleLine(pnt,pnt1, 1, 1,0,0);
        }
    }


    log3d.saveToFile("c:/ControlPoints_u.iv");

    //CPx = uCP_x;
    //CPy = uCP_y;
    //CPz = uCP_z;

    //return true;

    m_Grid.clear();
    m_Grid.resize(n+1);
    for (int i=0; i<n+1; ++i)
        m_Grid[i].resize(s+1);

    for (int i=0; i<n+1; ++i)
    {
        for (int j=0; j<s+1; ++j)
        {

            m_Grid[i][j].x = (float) uCP_x(i,j);
            m_Grid[i][j].y = (float) uCP_y(i,j);
            m_Grid[i][j].z = (float) uCP_z(i,j);
        }
    }

    //SurfMeshParam();

    // mit null vorinitialisieren
    for (int i=0; i<n + 1; ++i)
    {
        for (int j=0; j<m + 1; ++j)
        {
            CPx(i,j) = 0.0;
            CPy(i,j) = 0.0;
            CPz(i,j) = 0.0;
        }
    }

    std::vector< ublas::matrix<double> > Rv_x(n+1);
    std::vector< ublas::matrix<double> > Rv_y(n+1);
    std::vector< ublas::matrix<double> > Rv_z(n+1);

    Base::Builder3D log,logo;

    for (int j=0; j<n+1; ++j)
    {

        Rv_x[j].resize(s-1,1);
        Rv_y[j].resize(s-1,1);
        Rv_z[j].resize(s-1,1);

        CPx(j,0) = uCP_x(j,0);
        CPy(j,0) = uCP_y(j,0);
        CPz(j,0) = uCP_z(j,0);

        CPx(j,m) = uCP_x(j,s);
        CPy(j,m) = uCP_y(j,s);
        CPz(j,m) = uCP_z(j,s);

        for (int k=0; k<s-1; ++k)
        {

            Rv_x[j](k,0) = uCP_x(j,k+1) - Nv_full(k,0)*uCP_x(j,0) - Nv_full(k,m)*uCP_x(j,s);
            Rv_y[j](k,0) = uCP_y(j,k+1) - Nv_full(k,0)*uCP_y(j,0) - Nv_full(k,m)*uCP_y(j,s);
            Rv_z[j](k,0) = uCP_z(j,k+1) - Nv_full(k,0)*uCP_z(j,0) - Nv_full(k,m)*uCP_z(j,s);

            pnt.x = (float) uCP_x(j,k+1);
            pnt.y = (float) uCP_y(j,k+1);
            pnt.z = (float) uCP_z(j,k+1);

            pnt1.x = (float) uCP_x(j,k+2);
            pnt1.y = (float) uCP_y(j,k+2);
            pnt1.z = (float) uCP_z(j,k+2);

            log.addSingleLine(pnt,pnt1, 1, 0,0,0);

        }

        bx.clear();
        by.clear();
        bz.clear();
        bx.resize(1, m - 1);
        by.resize(1, m - 1);
        bz.resize(1, m - 1);

        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Rv_x[j],Nv_left, 0.0, bx);
        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Rv_y[j],Nv_left, 0.0, by);
        atlas::gemm(CblasTrans,CblasNoTrans, 1.0, Rv_z[j],Nv_left, 0.0, bz);

        atlas::getrs(CblasTrans,Nv,vpiv,bx);
        atlas::getrs(CblasTrans,Nv,vpiv,by);
        atlas::getrs(CblasTrans,Nv,vpiv,bz);

        for (int i=1; i<m; ++i)
        {
            CPx(j,i) = bx(0,i-1);
            CPy(j,i) = by(0,i-1);
            CPz(j,i) = bz(0,i-1);

            pnt.x = (float) CPx(j,i);
            pnt.y = (float) CPy(j,i);
            pnt.z = (float) CPz(j,i);

            logo.addSinglePoint(pnt,2,0,0,0);
        }
    }

    logo.saveToFile("c:/contrPnts.iv");

    ublas::matrix<double> Tmp = CPz;

    //glättung des kontrollpunktnetzes
    for (int i=1; i<n; ++i)
    {
        for (int j=1; j<m; ++j)
        {
            /*CPz(i,j) = ((Tmp(i-1,j-1) + Tmp(i-1,j) + Tmp(i-1,j+1)) +
                       (Tmp(i,j-1)   + Tmp(i,j)   + Tmp(i,j+1))   +
                     (Tmp(i+1,j-1) + Tmp(i+1,j) + Tmp(i+1,j+1)))/9;*/

            CPz(i,j) = (Tmp(i-1,j) + Tmp(i,j-1) + Tmp(i,j) + Tmp(i,j+1) +Tmp(i+1,j))/5;
        }
    }

    //for(int i=1; i<n; ++i){
    // for(int j=1; j<m; ++j){
    //  /*CPz(i,j) = ((Tmp(i-1,j-1) + Tmp(i-1,j) + Tmp(i-1,j+1)) +
//             (Tmp(i,j-1)   + Tmp(i,j)   + Tmp(i,j+1))   +
    //           (Tmp(i+1,j-1) + Tmp(i+1,j) + Tmp(i+1,j+1)))/9;*/

    //  CPz(i,j) = (Tmp(i-1,j) + Tmp(i,j-1) + Tmp(i,j) + Tmp(i,j+1) +Tmp(i+1,j))/5;
    // }
    //}

    return true;
}

bool UniGridApprox::BuildSurf()
{

    gp_Pnt pnt;
    TColgp_Array2OfPnt Poles(1,m_um,1,m_vm);



    for (int i=0; i<m_um; ++i)
    {
        for (int j=0; j<m_vm; ++j)
        {
            pnt.SetX(CPx(i,j));
            pnt.SetY(CPy(i,j));
            pnt.SetZ(CPz(i,j));

            Poles.SetValue(i+1,j+1,pnt);
        }
    }

    int c=1;
    for (unsigned int i=0; i<m_uknots.size()-1; ++i)
    {
        if (m_uknots[i+1] != m_uknots[i])
        {
            ++c;
        }
    }


    TColStd_Array1OfReal    UKnots(1,c);
    TColStd_Array1OfInteger UMults(1,c);

    c=1;
    for (unsigned int i=0; i<m_vknots.size()-1; ++i)
    {
        if (m_vknots[i+1] != m_vknots[i])
        {
            ++c;
        }
    }


    TColStd_Array1OfReal    VKnots(1,c);
    TColStd_Array1OfInteger VMults(1,c);

    int d=0;
    c=1;
    for (unsigned int i=0; i<m_uknots.size(); ++i)
    {
        if (m_uknots[i+1] != m_uknots[i])
        {
            UKnots.SetValue(d+1,m_uknots[i]);
            UMults.SetValue(d+1,c);
            ++d;
            c=1;

        }
        else
        {
            ++c;
        }

        if (i==(m_uknots.size()-2))
        {
            UKnots.SetValue(d+1,m_uknots[i+1]);
            UMults.SetValue(d+1,c);
            break;
        }
    }

    d=0;
    c=1;
    for (unsigned int i=0; i<m_vknots.size(); ++i)
    {
        if (m_vknots[i+1] != m_vknots[i])
        {
            VKnots.SetValue(d+1,m_vknots[i]);
            VMults.SetValue(d+1,c);
            ++d;
            c=1;

        }
        else
        {
            ++c;
        }

        if (i==(m_vknots.size()-2))
        {
            VKnots.SetValue(d+1,m_vknots[i+1]);
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
        // const Standard_Boolean   VPeriodic = Standard_False*/
    );

    aAdaptorSurface.Load(surface);

    return true;
}

double UniGridApprox::CompGridError()
{
    GeomAPI_ProjectPointOnSurf proj;
    MeshCore::MeshPointIterator pIt(m_Mesh);
    gp_Pnt pnt;
    double tmp = 0.0;

    mG_err.clear();
    mG_err.resize(m_Grid.size());
    for (unsigned int i=0; i<m_Grid.size(); ++i)
        mG_err[i].resize(m_Grid[i].size());

    for (unsigned int i=0; i<m_Grid.size(); ++i)
    {
        for (unsigned int j=0; j<m_Grid[i].size(); ++j)
        {

            pnt.SetCoord(m_Grid[i][j].x, m_Grid[i][j].y, m_Grid[i][j].z);
            proj.Init(pnt,aAdaptorSurface.BSpline(),0.1);
            if (proj.IsDone() == false)
                return -1.0;
            mG_err[i][j] = proj.LowerDistance();

            if (mG_err[i][j] > tmp)
                tmp = mG_err[i][j];

        }
    }

    return tmp;
}

bool UniGridApprox::WriteMatrix(ublas::matrix<double> M)
{

    int row = M.size1();
    int col = M.size2();

    for (int i=0; i<row; ++i)
    {
        for (int j=0; j<col; ++j)
        {
            cout << M(i,j) << ", ";
        }
        cout << endl;
    }

    return true;
}

double UniGridApprox::CompMeshError()
{
    Base::Builder3D log3d;
    MeshCore::MeshKernel mesh;
    BRepBuilderAPI_MakeFace Face(aAdaptorSurface.BSpline());
    GeomAPI_ProjectPointOnSurf proj;
    best_fit::Tesselate_Face(Face.Face(), mesh, float(0.1));
    cout << mesh.CountPoints() << endl;
    std::vector<Base::Vector3f> normals =  best_fit::Comp_Normals(mesh);

    double tmp = 0.0, sqrdis;
    double errSum = 0.0;
    int c=0;

    m_err.clear();
    m_err.resize(mesh.CountPoints(), 0.0);

    MeshCore::MeshFacetGrid aFacetGrid(m_Mesh);
    MeshCore::MeshAlgorithm malg(m_Mesh);
    MeshCore::MeshAlgorithm malg2(m_Mesh);
    MeshCore::MeshPointIterator p_it(mesh);

    Base::Vector3f projPoint, distVec, pnt;
    unsigned long  facetIndex;


    for (p_it.Begin(); p_it.More(); p_it.Next())
    {
        if (!malg.NearestFacetOnRay(*p_it, normals[p_it.Position()], aFacetGrid, projPoint, facetIndex))   // gridoptimiert
        {
            if (malg2.NearestFacetOnRay(*p_it, normals[p_it.Position()], projPoint, facetIndex))
            {
                pnt.x = p_it->x;
                pnt.y = p_it->y;
                pnt.z = p_it->z;
                log3d.addSingleLine(pnt,projPoint);
                distVec  = projPoint - pnt;
                sqrdis   = distVec*distVec;
            }
            else
            {
                cout << "oops, ";
                continue;
            }
        }
        else
        {
            pnt.x = p_it->x;
            pnt.y = p_it->y;
            pnt.z = p_it->z;
            log3d.addSingleLine(pnt,projPoint);
            distVec  = projPoint - pnt;
            sqrdis   = distVec*distVec;
        }

        errSum += sqrt(sqrdis);
        ++c;

        if (sqrt(sqrdis) > tmp)
        {
            m_err[p_it.Position()] = sqrt(sqrdis);
            tmp = m_err[p_it.Position()];
        }
    }

    log3d.saveToFile("c:/Error.iv");

    return errSum/c;
}
