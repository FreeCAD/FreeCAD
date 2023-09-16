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

#ifndef MESHFLATTENING
#define MESHFLATTENING

// idea:
// - unwrap any meshed shells and output a 2d face (meshing is done externally)
// - unwrap faces which are nurbs and return nurbs (no cuts, meshing internally)
// 	- TODO: map any curves from origin to flattened faces

#include <TopoDS_Face.hxx>
#include <vector>

#include <Eigen/Geometry>

#include "MeshFlatteningNurbs.h"


// clang-format off
using Vector3 = Eigen::Vector3d;
using Vector2 = Eigen::Vector2d;

template <typename type, unsigned int size>
using ColMat = Eigen::Matrix<type, Eigen::Dynamic, size>;

template <typename type, unsigned int size>
using RowMat = Eigen::Matrix<type, size, Eigen::Dynamic>;


using trip = Eigen::Triplet<double>;
using spMat = Eigen::SparseMatrix<double>;


std::vector<ColMat<double, 3>> getBoundaries(ColMat<double, 3> vertices, ColMat<long, 3> tris);

class FaceUnwrapper{
	nurbs::NurbsBase2D nu;
public:
    FaceUnwrapper() = default;
	FaceUnwrapper(const TopoDS_Face & face);
        FaceUnwrapper(ColMat<double, 3> xyz_nodes, ColMat<long, 3> tris);
	void findFlatNodes(int steps, double val);
	ColMat<double, 3> interpolateFlatFace(const TopoDS_Face& face);
        std::vector<ColMat<double, 3>> getFlatBoundaryNodes();

	bool use_nurbs = true;
	// the mesh
	ColMat<long, 3> tris;  // input
	ColMat<long, 1> fixed_nodes; // input
	ColMat<double, 3> xyz_nodes; // compute from uv_mesh (xyz = A * poles)
	ColMat<double, 2> uv_nodes;  // input
	ColMat<double, 2> ze_nodes;  // compute

	// nurbs
	ColMat<double, 2> ze_poles;   // compute
	spMat A; // mapping between nurbs(poles) and mesh(vertices) computed with nurbs-basis-functions and uv_mesh

};
// clang-format on

#endif  // MESHFLATTENING
