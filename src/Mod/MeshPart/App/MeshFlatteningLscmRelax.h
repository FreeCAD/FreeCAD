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


// LeastSquareConformalMapping + fem relaxing
// ------------------------------------------
// 
#ifndef UNWRAP_H
#define UNWRAP_H

// 1: local coordinates 2d representation  q_l_0
// 2: least square conformal map -> flat_vertices_0
// 3: local coordinates of mapped mesh q_l_1
// 4: diff in local coordinates -> forces R.B^T.B.(x1-x0)
// 5: stiffnes mat K
// 6: K.u=forces ->u
// 7: x1, y1 += w * u

#include <vector>
#include <memory>
#include <tuple>

#include <Eigen/Geometry>
#include <Eigen/IterativeLinearSolvers>

#include "MeshFlatteningLscmRelax.h"

typedef Eigen::SparseMatrix<double> spMat;


typedef Eigen::Vector3d Vector3;
typedef Eigen::Vector2d Vector2;

template <typename type, unsigned int size>
using ColMat = Eigen::Matrix<type, Eigen::Dynamic, size>;

template <typename type, unsigned int size>
using RowMat = Eigen::Matrix<type, size, Eigen::Dynamic>;

class LscmRelax{
private:
    ColMat<double, 3> q_l_g;  // the position of the 3d triangles at there locale coord sys
    ColMat<double, 3> q_l_m;  // the mapped position in local coord sys

    void set_q_l_g();
    void set_q_l_m();
    void set_fixed_pins();
    void set_position(Eigen::VectorXd);
    void set_shift(Eigen::VectorXd);

    std::vector<long> new_order;
    std::vector<long> old_order;

    Eigen::Matrix<double, 3, 3> C;
    Eigen::VectorXd sol;

    std::vector<long> get_fem_fixed_pins();

public:
    LscmRelax(
        RowMat<double, 3> vertices, 
        RowMat<long, 3> triangles,
        std::vector<long> fixed_pins);

    std::vector<long> fixed_pins;
    RowMat<double, 3> vertices;
    RowMat<long, 3> triangles;
    RowMat<double, 2> flat_vertices;
    ColMat<double, 1> rhs;
    Eigen::MatrixXd MATRIX;

    double nue=0.0;
    double elasticity=1.;

    void lscm();
    void relax(double);

    ColMat<double, 3> get_flat_vertices_3D();

    void rotate_by_min_bound_area();
    void transform(bool scale=false);
    
    double get_area();
    double get_flat_area();

};


#endif
