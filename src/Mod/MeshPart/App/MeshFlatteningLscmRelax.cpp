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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#include <Eigen/SparseCholesky>

#include "MeshFlatteningLscmRelax.h"


// TODO:
// area constrained (scale the final unwrapped mesh to the original area)
// FEM approach

// clang-format off
namespace lscmrelax
{

using trip = Eigen::Triplet<double>;
using spMat = Eigen::SparseMatrix<double>;



ColMat<double, 2> map_to_2D(ColMat<double, 3> points)
{
    ColMat<double, 4> mat(points.size(), 4);
    mat << points, ColMat<double, 1>::Ones(points.size());
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(mat, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Vector3 n = svd.matrixV().row(2);

    n.normalize();
    Vector3 y(0, 1, 0);
    if ((n - y).norm() < 0.0001)
        y = Vector3(1, 0, 0);
    Vector3 x = n.cross(y);
    x.normalize();
    y = n.cross(x);
    Eigen::Matrix<double, 3, 2> transform;
    transform.col(0) = x;
    transform.col(1) = y;
    return points * transform;
}


unsigned int get_max_distance(Vector3 point, RowMat<double, 3> vertices, double & max_dist)
{
    max_dist = 0;
    double dist;
    unsigned long max_dist_index = 0;
    long j = 0;
    for (j=0; j < vertices.cols(); j++)
    {
        // debugging
        dist = (point - vertices.col(j)).norm();
        if (dist > max_dist)
        {
            max_dist = dist;
            max_dist_index = j;
        }
    }
    return max_dist_index;
}


LscmRelax::LscmRelax(
        RowMat<double, 3> vertices,
        RowMat<long, 3> triangles,
        std::vector<long> fixed_pins)
{
    this->vertices = vertices;
    this->triangles = triangles;
    this->flat_vertices.resize(2, this->vertices.cols());
    this->fixed_pins = fixed_pins;

    // set the fixed pins of the flat-mesh:
    this->set_fixed_pins();


    unsigned int fixed_count = 0;
    for (long i=0; i < this->vertices.cols(); i++)
    {
        if (fixed_count < this->fixed_pins.size())
        {
            if (i == this->fixed_pins[fixed_count])
                fixed_count ++;
            else
                this->old_order.push_back(i);
        }
        else
            this->old_order.push_back(i);
    }

    for (auto fixed_index: this->fixed_pins)
        this->old_order.push_back(fixed_index);

    // get the reversed map:
    this->new_order.resize(this->old_order.size());
    long j = 0;
    for (auto index: this->old_order)
    {
        this->new_order[index] = j;
        j++;
    }

    // set the C-mat
    this->C << 1,   nue, 0,
                nue, 1,  0,
                0,   0, (1 - nue) / 2;
    this->C *= elasticity / (1 - nue * nue);
}



//////////////////////////////////////////////////////////////////////////
/////////////////                 F.E.M                      /////////////
//////////////////////////////////////////////////////////////////////////
void LscmRelax::relax(double weight)
{
    ColMat<double, 3> d_q_l_g = this->q_l_m - this->q_l_g;
    // for every triangle
    Eigen::Matrix<double, 3, 6> B;
    Eigen::Matrix<double, 2, 2> T;
    Eigen::Matrix<double, 6, 6> K_m;
    Eigen::Matrix<double, 6, 1> u_m, rhs_m;
    Eigen::VectorXd rhs(this->vertices.cols() * 2 + 3);
    if (this->sol.size() == 0)
        this->sol.Zero(this->vertices.cols() * 2 + 3);
    spMat K_g(this->vertices.cols() * 2 + 3, this->vertices.cols() * 2 + 3);
    std::vector<trip> K_g_triplets;
    Vector2 v1, v2, v3, v12, v23, v31;
    long row_pos, col_pos;
    double A;

    rhs.setZero();

    for (long i=0; i<this->triangles.cols(); i++)
    {
        // 1: construct B-mat in m-system
        v1 = this->flat_vertices.col(this->triangles(0, i));
        v2 = this->flat_vertices.col(this->triangles(1, i));
        v3 = this->flat_vertices.col(this->triangles(2, i));
        v12 = v2 - v1;
        v23 = v3 - v2;
        v31 = v1 - v3;
        B << -v23.y(),   0,        -v31.y(),   0,        -v12.y(),   0,
              0,         v23.x(),   0,         v31.x(),   0,         v12.x(),
             -v23.x(),   v23.y(),  -v31.x(),   v31.y(),  -v12.x(),   v12.y();
        T << v12.x(), -v12.y(),
             v12.y(), v12.x();
        T /= v12.norm();
        A = std::abs(this->q_l_m(i, 0) * this->q_l_m(i, 2) / 2);
        B /= A * 2; // (2*area)

        // 2: sigma due dqlg in m-system
        u_m << Vector2(0, 0), T * Vector2(d_q_l_g(i, 0), 0), T * Vector2(d_q_l_g(i, 1), d_q_l_g(i, 2));

        // 3: rhs_m = B.T * C * B * dqlg_m
        //    K_m = B.T * C * B
        rhs_m = B.transpose() * this->C * B * u_m * A;
        K_m = B.transpose() * this->C * B * A;

        // 5: add to rhs_g, K_g
        for (int j=0; j < 3; j++)
        {
            row_pos = this->triangles(j, i);
            rhs[row_pos * 2]     += rhs_m[j * 2];
            rhs[row_pos * 2 + 1] += rhs_m[j * 2 +1];
            for (int k=0; k < 3; k++)
            {
                col_pos = this->triangles(k, i);
                K_g_triplets.emplace_back(trip(row_pos * 2,     col_pos * 2,        K_m(j * 2,      k * 2)));
                K_g_triplets.emplace_back(trip(row_pos * 2 + 1, col_pos * 2,        K_m(j * 2 + 1,  k * 2)));
                K_g_triplets.emplace_back(trip(row_pos * 2 + 1, col_pos * 2 + 1,    K_m(j * 2 + 1,  k * 2 + 1)));
                K_g_triplets.emplace_back(trip(row_pos * 2,     col_pos * 2 + 1,    K_m(j * 2,      k * 2 + 1)));
                // we don't have to fill all because the matrix is symmetric.
            }
        }
    }
    // FIXING SOME PINS:
    // - if there are no pins (or only one pin) selected solve the system without the nullspace solution.
    // - if there are some pins selected, delete all columns, rows that refer to this pins
    //          set the diagonal element of these pins to 1 + the rhs to zero
    //          (? is it possible to fix in the inner of the face? for sure for fem, but lscm could have some problems)
    //          (we also need some extra variables to see if the pins come from user)

    // fixing some points
    // although only internal forces are applied there has to be locked
    // at least 3 degrees of freedom to stop the mesh from pure rotation and pure translation
    // std::vector<long> fixed_dof;
    // fixed_dof.push_back(this->triangles(0, 0) * 2); //x0
    // fixed_dof.push_back(this->triangles(0, 0) * 2 + 1); //y0
    // fixed_dof.push_back(this->triangles(1, 0) * 2 + 1); // y1

    // align flat mesh to fixed edge
    // Vector2 edge = this->flat_vertices.col(this->triangles(1, 0)) -
    //                this->flat_vertices.col(this->triangles(0, 0));
    // edge.normalize();
    // Eigen::Matrix<double, 2, 2> rot;
    // rot << edge.x(), edge.y(), -edge.y(), edge.x();
    // this->flat_vertices = rot * this->flat_vertices;

    // // return true if triplet row / col is in fixed_dof
    // auto is_in_fixed_dof = [fixed_dof](const trip & element) -> bool {
    //     return (
    //         (std::find(fixed_dof.begin(), fixed_dof.end(), element.row()) != fixed_dof.end()) or
    //         (std::find(fixed_dof.begin(), fixed_dof.end(), element.col()) != fixed_dof.end()));
    // };
    // std::cout << "size of triplets: " << K_g_triplets.size() << std::endl;
    // K_g_triplets.erase(
    //     std::remove_if(K_g_triplets.begin(), K_g_triplets.end(), is_in_fixed_dof),
    //     K_g_triplets.end());
    // std::cout << "size of triplets: " << K_g_triplets.size() << std::endl;
    // for (long fixed: fixed_dof)
    // {
    //     K_g_triplets.push_back(trip(fixed, fixed, 1.));
    //     rhs[fixed] = 0;
    // }

    // for (long i=0; i< this->vertices.cols() * 2; i++)
    //     K_g_triplets.push_back(trip(i, i, 0.01));

    // lagrange multiplier
    for (long i=0; i < this->flat_vertices.cols() ; i++)
    {
        // fixing total ux
        K_g_triplets.emplace_back(trip(i * 2, this->flat_vertices.cols() * 2, 1));
        K_g_triplets.emplace_back(trip(this->flat_vertices.cols() * 2, i * 2, 1));
        // fixing total uy
        K_g_triplets.emplace_back(trip(i * 2 + 1, this->flat_vertices.cols() * 2 + 1, 1));
        K_g_triplets.emplace_back(trip(this->flat_vertices.cols() * 2 + 1, i * 2 + 1, 1));
        // fixing ux*y-uy*x
        K_g_triplets.emplace_back(trip(i * 2, this->flat_vertices.cols() * 2 + 2, - this->flat_vertices(1, i)));
        K_g_triplets.emplace_back(trip(this->flat_vertices.cols() * 2 + 2, i * 2, - this->flat_vertices(1, i)));
        K_g_triplets.emplace_back(trip(i * 2 + 1, this->flat_vertices.cols() * 2 + 2, this->flat_vertices(0, i)));
        K_g_triplets.emplace_back(trip(this->flat_vertices.cols() * 2 + 2, i * 2 + 1, this->flat_vertices(0, i)));
    }

    // project out the nullspace solution:

    // Eigen::VectorXd nullspace1(this->flat_vertices.cols() * 2);
    // Eigen::VectorXd nullspace2(this->flat_vertices.cols() * 2);
    // nullspace1.setZero();
    // nullspace2.setOnes();
    // for (long i= 0; i < this->flat_vertices.cols(); i++)
    // {
    //     nullspace1(i) = 1;
    //     nullspace2(i) = 0;
    // }
    // nullspace1.normalize();
    // nullspace2.normalize();
    // rhs -= nullspace1.dot(rhs) * nullspace1;
    // rhs -= nullspace2.dot(rhs) * nullspace2;

    K_g.setFromTriplets(K_g_triplets.begin(), K_g_triplets.end());
    // rhs +=  K_g * Eigen::VectorXd::Ones(K_g.rows());

    // solve linear system (privately store the value for guess in next step)
    Eigen::SimplicialLDLT<spMat, Eigen::Lower> solver;
    solver.compute(K_g);
    this->sol = solver.solve(-rhs);
    this->set_shift(this->sol.head(this->vertices.cols() * 2) * weight);
    this->set_q_l_m();
}


void LscmRelax::area_relax(double weight)
{
//     TODO: doesn't work so far
    if (this->sol.size() == 0)
        this->sol.Zero(this->vertices.cols());
    std::vector<trip> K_g_triplets;
    spMat K_g(this->vertices.cols() * 2, this->vertices.cols() * 2);
    spMat K_g_lsq(this->triangles.cols(), this->vertices.cols() * 2);
    Eigen::VectorXd rhs_lsq(this->triangles.cols());
    Eigen::VectorXd rhs(this->vertices.cols() * 2);
    rhs.setZero();

    Eigen::Matrix<double, 1, 6> B;
    double delta_a;
    Vector2 v1, v2, v3, v12, v23, v31;


    for (long i=0; i<this->triangles.cols(); i++)
    {
//         1: construct B-mat in m-system
        v1 = this->flat_vertices.col(this->triangles(0, i));
        v2 = this->flat_vertices.col(this->triangles(1, i));
        v3 = this->flat_vertices.col(this->triangles(2, i));
        v12 = v2 - v1;
        v23 = v3 - v2;
        v31 = v1 - v3;
        B << -v23.y(), v23.x(), -v31.y(), v31.x(), -v12.y(), v12.x();
	delta_a = fabs(this->q_l_g(i, 0) * this->q_l_g(i, 2)) -
		  fabs(this->q_l_m(i, 0) * this->q_l_m(i, 2));
	rhs_lsq[i] = delta_a * 0.1;

    std::array<int, 6> range_6 {{0, 1, 2, 3, 4, 5}};
	std::array<long, 6> indices;
	for (int index=0; index<3; index++)
	{
	    indices[index * 2] = this->triangles(index, i) * 2;
	    indices[index * 2 + 1] = this->triangles(index, i) * 2 + 1;
	}

	for(auto col: range_6)
	{
        K_g_triplets.emplace_back(trip(i, indices[col], (double) B[col]));
	}
    }
    K_g_lsq.setFromTriplets(K_g_triplets.begin(), K_g_triplets.end());
    K_g_triplets.clear();

    K_g.setFromTriplets(K_g_triplets.begin(), K_g_triplets.end());
    Eigen::ConjugateGradient<spMat> solver;
    solver.compute(K_g);
    this->sol = solver.solve(-rhs);
    this->set_shift(this->sol * weight);

}

void LscmRelax::edge_relax(double weight)
{
//  1. get all edges
    std::set<std::array<long, 2>> edges;
    std::array<long, 2> edge;
    for(long i=0; i<this->triangles.cols(); i++)
    {
        for(int j=0; j<3; j++)
        {
            long k = j+1;
            if (k==3)
                k = 0;
            if (this->triangles(j, i) < this->triangles(k, i))
                edge = std::array<long, 2>{{this->triangles(j, i), this->triangles(k, i)}};
            else
                edge = std::array<long, 2>{{this->triangles(k, i), this->triangles(j, i)}};
            edges.insert(edge);
        }
    }
//  2. create system

    if (this->sol.size() == 0)
        this->sol.Zero(this->vertices.cols());

    std::vector<trip> K_g_triplets;
    spMat K_g(this->vertices.cols() * 2, this->vertices.cols() * 2);
    Eigen::VectorXd rhs(this->vertices.cols() * 2);
    rhs.setZero();
    for(auto edge : edges)
    {
// 	this goes to the right side
	Vector3 v1_g = this->vertices.col(edge[0]);
	Vector3 v2_g = this->vertices.col(edge[1]);
	Vector2 v1_l = this->flat_vertices.col(edge[0]);
	Vector2 v2_l = this->flat_vertices.col(edge[1]);
	std::vector<int> range_4 {0, 1, 2, 3};
	std::vector<long> indices {edge[0] * 2, edge[0] * 2 + 1, edge[1] * 2, edge[1] * 2 + 1};
	double l_g = (v2_g - v1_g).norm();
	double l_l = (v2_l - v1_l).norm();
	Vector2 t = (v2_l - v1_l);  // direction
	t.normalize();

	Eigen::Matrix<double, 1, 4> B;
	Eigen::Matrix<double, 4, 4> K;
	Eigen::Matrix<double, 4, 1> rhs_m;

	B << -t.x(), -t.y(), t.x(), t.y();
	K = 1. / l_g * B.transpose() * B;
	rhs_m = - B.transpose() * (l_g - l_l);


	for(auto row: range_4)
	{
	    for (auto col: range_4)
	    {
        K_g_triplets.emplace_back(trip(indices[row], indices[col], (double) K(row, col)));
	    }
	    rhs(indices[row]) += rhs_m[row];
	}
    }

    K_g.setFromTriplets(K_g_triplets.begin(), K_g_triplets.end());
    Eigen::ConjugateGradient<spMat,Eigen::Lower, NullSpaceProjector> solver;
    solver.preconditioner().setNullSpace(this->get_nullspace());
    solver.compute(K_g);
    this->sol = solver.solve(-rhs);
    this->set_shift(this->sol * weight);
}


//////////////////////////////////////////////////////////////////////////
/////////////////                 L.S.C.M                    /////////////
//////////////////////////////////////////////////////////////////////////
void LscmRelax::lscm()
{
    this->set_q_l_g();
    std::vector<trip> triple_list;
    long i;
    double x21, x31, y31, x32;

    // 1. create the triplet list (t * 2, v * 2)
    for(i=0; i<this->triangles.cols(); i++)
    {
        x21 = this->q_l_g(i, 0);
        x31 = this->q_l_g(i, 1);
        y31 = this->q_l_g(i, 2);
        x32 = x31 - x21;

        triple_list.emplace_back(trip(2 * i, this->new_order[this->triangles(0, i)] * 2, x32));
        triple_list.emplace_back(trip(2 * i, this->new_order[this->triangles(0, i)] * 2 + 1, -y31));
        triple_list.emplace_back(trip(2 * i, this->new_order[this->triangles(1, i)] * 2, -x31));
        triple_list.emplace_back(trip(2 * i, this->new_order[this->triangles(1, i)] * 2 + 1, y31));
        triple_list.emplace_back(trip(2 * i, this->new_order[this->triangles(2, i)] * 2, x21));

        triple_list.emplace_back(trip(2 * i + 1, this->new_order[this->triangles(0, i)] * 2, y31));
        triple_list.emplace_back(trip(2 * i + 1, this->new_order[this->triangles(0, i)] * 2 + 1, x32));
        triple_list.emplace_back(trip(2 * i + 1, this->new_order[this->triangles(1, i)] * 2, -y31));
        triple_list.emplace_back(trip(2 * i + 1, this->new_order[this->triangles(1, i)] * 2 + 1, -x31));
        triple_list.emplace_back(trip(2 * i + 1, this->new_order[this->triangles(2, i)] * 2 + 1, x21));

    }
    // 2. divide the triplets in matrix(unknown part) and rhs(known part) and reset the position
    std::vector<trip> rhs_triplets;
    std::vector<trip> mat_triplets;
    for (auto triplet: triple_list)
    {
        if (triplet.col() > static_cast<int>((this->vertices.cols() - this->fixed_pins.size()) * 2 - 1))
            rhs_triplets.push_back(triplet);
        else
            mat_triplets.push_back(triplet);
    }

    // 3. create a rhs_pos vector
    Eigen::VectorXd rhs_pos(this->vertices.cols() * 2);
    rhs_pos.setZero();
    for (auto index: this->fixed_pins)
    {
        rhs_pos[this->new_order[index] * 2] = this->flat_vertices(0, index);      //TODO: not yet set
        rhs_pos[this->new_order[index] * 2 + 1] = this->flat_vertices(1, index);
    }

    // 4. fill a sparse matrix and calculdate the rhs
    Eigen::VectorXd rhs(this->triangles.cols() * 2); // maybe use a sparse vector
    spMat B(this->triangles.cols() * 2, this->vertices.cols() * 2);
    B.setFromTriplets(rhs_triplets.begin(), rhs_triplets.end());
    rhs = B * rhs_pos;

    // 5. create the lhs matrix
    spMat A(this->triangles.cols() * 2, (this->vertices.cols() - this->fixed_pins.size()) * 2);
    A.setFromTriplets(mat_triplets.begin(), mat_triplets.end());

    // 6. solve the system and set the flatted coordinates
    // Eigen::SparseQR<spMat, Eigen::COLAMDOrdering<int> > solver;
    Eigen::LeastSquaresConjugateGradient<spMat > solver;
    Eigen::VectorXd sol(this->vertices.size() * 2);
    solver.compute(A);
    sol = solver.solve(-rhs);

    // TODO: create function, is needed also in the fem step
    this->set_position(sol);
    this->set_q_l_m();
    this->transform(true);
    // this->rotate_by_min_bound_area();
    this->set_q_l_m();

}

void LscmRelax::set_q_l_g()
{
    // get the coordinates of a triangle in local coordinates from the 3d mesh
    // x1, y1, y2 = 0
    // -> vector<x2, x3, y3>
    this->q_l_g.resize(this->triangles.cols(), 3);
    for (long i = 0; i < this->triangles.cols(); i++)
    {
        Vector3 r1 = this->vertices.col(this->triangles(0, i));
        Vector3 r2 = this->vertices.col(this->triangles(1, i));
        Vector3 r3 = this->vertices.col(this->triangles(2, i));
        Vector3 r21 = r2 - r1;
        Vector3 r31 = r3 - r1;
        double r21_norm = r21.norm();
        r21.normalize();
        // if triangle is fliped this gives wrong results?
        this->q_l_g.row(i) << r21_norm, r31.dot(r21), r31.cross(r21).norm();
    }
}

void LscmRelax::set_q_l_m()
{
    // get the coordinates of a triangle in local coordinates from the 2d map
    // x1, y1, y2 = 0
    // -> vector<x2, x3, y3>
    this->q_l_m.resize(this->triangles.cols(), 3);
    for (long i = 0; i < this->triangles.cols(); i++)
    {
        Vector2 r1 = this->flat_vertices.col(this->triangles(0, i));
        Vector2 r2 = this->flat_vertices.col(this->triangles(1, i));
        Vector2 r3 = this->flat_vertices.col(this->triangles(2, i));
        Vector2 r21 = r2 - r1;
        Vector2 r31 = r3 - r1;
        double r21_norm = r21.norm();
        r21.normalize();
        // if triangle is fliped this gives wrong results!
        this->q_l_m.row(i) << r21_norm, r31.dot(r21), -(r31.x() * r21.y() - r31.y() * r21.x());
    }
}

void LscmRelax::set_fixed_pins()
{
    // if less then one fixed pin is set find two by an automated algorithm and align them to y = 0
    // if more then two pins are chosen find a leastsquare-plane and project the points on it
    // insert the points in the flat-vertices vector
    if (this->fixed_pins.size() == 0)
        this->fixed_pins.push_back(0);
    if (this->fixed_pins.size() == 1)
    {

        double dist;
        this->fixed_pins.push_back(get_max_distance(this->vertices.col(this->fixed_pins[0]), this->vertices, dist));
        this->flat_vertices.col(this->fixed_pins[0]) = Vector2(0, 0);
        this->flat_vertices.col(this->fixed_pins[1]) = Vector2(dist, 0);
    }
    std::sort(this->fixed_pins.begin(), this->fixed_pins.end());
    // not yet working
    // if (this->fixed_pins.size() > 2)
    // {
    //     std::vector<Vector3> fixed_3d_points;
    //     for (unsigned int index: this->fixed_pins)
    //         fixed_3d_points.push_back(this->vertices[index]);
    //     std::vector<Vector2> maped_points = map_to_2D(fixed_3d_points);
    //     unsigned int i = 0;
    //     for (unsigned int index: this->fixed_pins)
    //     {
    //         this->flat_vertices[i] = maped_points[i];
    //         i++;
    //     }
    // }
}


ColMat<double, 3> LscmRelax::get_flat_vertices_3D()
{
    ColMat<double, 2> mat = this->flat_vertices.transpose();
    ColMat<double, 3> mat3d(mat.rows(), 3);
    mat3d << mat, ColMat<double, 1>::Zero(mat.rows());
    return mat3d;
}

void LscmRelax::set_position(Eigen::VectorXd sol)
{
    for (long i=0; i < this->vertices.size(); i++)
    {
        if (sol.size() > i * 2 + 1)
            this->flat_vertices.col(this->old_order[i]) << sol[i * 2], sol[i * 2 + 1];
    }
}

void LscmRelax::set_shift(Eigen::VectorXd sol)
{
    for (long i=0; i < this->vertices.size(); i++)
    {
        if (sol.size() > i * 2 + 1)
            this->flat_vertices.col(i) += Vector2(sol[i * 2], sol[i * 2 + 1]);
    }
}

double LscmRelax::get_area()
{
    double area = 0;
    for(long i = 0; i < this->triangles.cols(); i++)
    {
        area += this->q_l_g(i, 0) * this->q_l_g(i, 2);
    }
    return area / 2;
}

double LscmRelax::get_flat_area()
{
    double area = 0;
    for(long i = 0; i < this->triangles.cols(); i++)
    {
        area += this->q_l_m(i, 0) * this->q_l_m(i, 2);
    }
    return area / 2;
}

void LscmRelax::transform(bool scale)
{
    // assuming q_l_m and flat_vertices are set
    Vector2 weighted_center, center;
    weighted_center.setZero();
    double flat_area = 0;
    double global_area = 0;
    double element_area;
    for(long i = 0; i < this->triangles.cols(); i++)
    {
        global_area += this->q_l_g(i, 0) * this->q_l_g(i, 2) / 2;
        element_area = this->q_l_m(i, 0) * this->q_l_m(i, 2) / 2;
        for (int j=0; j < 3; j++)
            weighted_center += this->flat_vertices.col(this->triangles(j, i)) * element_area / 3;
        flat_area += element_area;
    }
    center = weighted_center / flat_area;
    for (long i = 0; i < this->flat_vertices.cols(); i++)
        this->flat_vertices.col(i) -= center;
    if (scale)
        this->flat_vertices *= std::pow(global_area / flat_area, 0.5);
    this->set_q_l_m();
}

void LscmRelax::rotate_by_min_bound_area()
{
    int n = 100;
    double phi;
    double min_phi = 0;
    double  min_area = 0;
    bool x_dominant = false;
    // rotate vector by 90 degree and find min area
    for (int i = 0; i < n + 1; i++ )
    {
        phi = i * M_PI / n;
        Eigen::VectorXd x_proj = this->flat_vertices.transpose() * Vector2(std::cos(phi), std::sin(phi));
        Eigen::VectorXd y_proj = this->flat_vertices.transpose() * Vector2(-std::sin(phi), std::cos(phi));
        double x_distance = x_proj.maxCoeff() - x_proj.minCoeff();
        double y_distance = y_proj.maxCoeff() - y_proj.minCoeff();
        double area = x_distance * y_distance;
        if (min_area == 0 || area < min_area)
        {
            min_area = area;
            min_phi = phi;
            x_dominant = x_distance > y_distance;
        }
    }
    Eigen::Matrix<double, 2, 2> rot;
    min_phi += x_dominant * M_PI / 2;
    rot << std::cos(min_phi), std::sin(min_phi), -std::sin(min_phi), std::cos(min_phi);
    this->flat_vertices = rot * this->flat_vertices;
}

std::vector<long> LscmRelax::get_fem_fixed_pins()
{
    long min_x_index = 0;
    double min_x = this->vertices(0, 0);
    for (long i=0; i<this->flat_vertices.cols(); i++)
    {
        // get index with min x
        if (this->flat_vertices(0, i) < min_x)
        {
            min_x = this->flat_vertices(0, i);
            min_x_index = i;
        }
    }
    double min_y = this->flat_vertices(1, min_x_index);
    long max_x_index = 0;
    double max_x = 0;
    for (long i=0; i<this->flat_vertices.cols(); i++)
    {
        // get index with min x
        double d_x = (this->flat_vertices(0, i) - min_x);
        double d_y = (this->flat_vertices(1, i) - min_y);
        if (d_x * d_x - d_y * d_y > max_x)
        {
            max_x = d_x * d_x - d_y * d_y;
            max_x_index = i;
        }
    }
    return std::vector<long>{min_x_index * 2, min_x_index * 2 + 1, max_x_index * 2 + 1};
}

Eigen::MatrixXd LscmRelax::get_nullspace()
{
    Eigen::MatrixXd null_space;
    null_space.setZero(this->flat_vertices.size() * 2, 3);

    for (int i=0; i<this->flat_vertices.cols(); i++)
    {
        null_space(i * 2, 0) =  1;  // x-translation
        null_space(i * 2 + 1, 1) =  1;  // y-translation
        null_space(i * 2, 2) =  -  this->flat_vertices(1, i);  // z-rotation
        null_space(i * 2 + 1, 2) = this->flat_vertices(0, i);  // z-rotation

    }
    return null_space;
}


}
// clang-format on
