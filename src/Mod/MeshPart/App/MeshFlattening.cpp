/***************************************************************************
 *   Copyright (c) 2017 Lorenz Lechner                                     *
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


#include "MeshFlattening.h"
#include "MeshFlatteningLscmRelax.h"
#include <Poly_Triangulation.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <Geom_BSplineSurface.hxx>

FaceUnwrapper::FaceUnwrapper(const TopoDS_Face& face)
{
    int i = 0;
//  transform to nurbs:
    TopLoc_Location location;
    
//  triangulate:
    const Handle(Poly_Triangulation) &triangulation = BRep_Tool::Triangulation(face, location);

    if (triangulation.IsNull())
        throw std::runtime_error("null triangulation in face construction");

//  compute uv coordinates
    if (triangulation->HasUVNodes())
    {
        const TColgp_Array1OfPnt2d &_uv_nodes = triangulation->UVNodes();
        this->uv_nodes.resize(triangulation->NbNodes(), 2);
        i = 0;
        for (gp_Pnt2d _uv_node: _uv_nodes)
        {
            this->uv_nodes.row(i) << _uv_node.X(), _uv_node.Y();
            i++;
        }
    }
// 
    const TColgp_Array1OfPnt &_nodes = triangulation->Nodes();
    this->xyz_nodes.resize(triangulation->NbNodes(), 3);
    i = 0;
    for (gp_Pnt _node: _nodes)
    {
        this->xyz_nodes.row(i) << _node.X(), _node.Y(), _node.Z();
        i++;
    }
    
    const Poly_Array1OfTriangle &_tris = triangulation->Triangles();
    this->tris.resize(triangulation->NbTriangles(), 3);
    i = 0;
    for (Poly_Triangle _tri: _tris)
    {
        int n1, n2, n3;
        _tri.Get(n1, n2, n3);
        this->tris.row(i) << n1-1, n2-1, n3-1;
        i++;
    }
}

void FaceUnwrapper::findFlatNodes()
{
    std::vector<long> fixed_pins;
    LscmRelax mesh_flattener(this->xyz_nodes.transpose(), this->tris.transpose(), fixed_pins);
    mesh_flattener.lscm();
    mesh_flattener.relax(0.9);
    this->ze_nodes = mesh_flattener.flat_vertices.transpose();
}

ColMat<double, 3> FaceUnwrapper::interpolateNurbsFace(const TopoDS_Face& face)
{
    // extract xyz poles, knots, weights, degree
    const Handle(Geom_Surface) &_surface = BRep_Tool::Surface(face);
    const Handle(Geom_BSplineSurface) &_bspline = Handle(Geom_BSplineSurface)::DownCast(_surface);
    const TColStd_Array1OfReal &_uknots = _bspline->UKnotSequence();
    const TColStd_Array1OfReal &_vknots = _bspline->VKnotSequence();    
    
    Eigen::VectorXd weights;
    weights.resize(_bspline->NbUPoles() * _bspline->NbVPoles());
    int i = 0;
    for (int u=1; u <= _bspline->NbUPoles(); u++)
    {
        for (int v=1; v <= _bspline->NbVPoles(); v++)
        {
            weights[i] = _bspline->Weight(u, v);
            i++;
        }
    }
    
    Eigen::VectorXd u_knots;
    Eigen::VectorXd v_knots;
    u_knots.resize(_uknots.Size());
    v_knots.resize(_vknots.Size());
    for (int u=1; u <= _uknots.Size(); u++)
    {
        u_knots[u - 1] = _uknots.Value(u);
    }
    for (int v=1; v <= _vknots.Size(); v++)
    {
        v_knots[v - 1] = _vknots.Value(v);
    }
    

    nu = nurbs::NurbsBase2D(u_knots, v_knots, weights, _bspline->UDegree(), _bspline->VDegree());
    A = nu.getInfluenceMatrix(this->uv_nodes);
    
    Eigen::LeastSquaresConjugateGradient<spMat > solver;
    solver.compute(A);
    ColMat<double, 2> ze_poles;
    ColMat<double, 3> flat_poles;
    ze_poles.resize(weights.rows(), 2);
    flat_poles.resize(weights.rows(), 3);
    flat_poles.setZero();
    ze_poles = solver.solve(ze_nodes);
    flat_poles.col(0) << ze_poles.col(0);
    flat_poles.col(1) << ze_poles.col(1);
    return flat_poles;    
}








