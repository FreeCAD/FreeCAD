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
#include <cmath>
#include <iostream>
#endif

#include "MeshFlatteningNurbs.h"


// clang-format off
namespace nurbs{

double divide(double a, double b)
{
    if (fabs(b) < 10e-14)
        return 0;
    else
        return a / b;
}

Eigen::VectorXd NurbsBase1D::getKnotSequence(double u_min, double u_max, int u_deg, int num_u_poles)
{
    // boarder poles are on the surface
    std::vector<double> u_knots;
    for (int i=0; i < u_deg; i++)
        u_knots.push_back(u_min);
    for (int i=0; i < num_u_poles; i++)
        u_knots.push_back(u_min + (u_max - u_min) * i / (num_u_poles - 1));
    for (int i=0; i < u_deg; i++)
        u_knots.push_back(u_max);
    return Eigen::Map<Eigen::VectorXd>(u_knots.data(), u_knots.size());
}

Eigen::VectorXd NurbsBase1D::getWeightList(Eigen::VectorXd knots, int u_deg)
{
    Eigen::VectorXd weights;
    weights.resize(knots.rows() - u_deg - 1);
    weights.setOnes();
    return weights;
}


// DE BOOR ALGORITHM FROM OPENGLIDER
std::function<double(double)> get_basis(int degree, int i, Eigen::VectorXd knots)
    // Return a basis_function for the given degree """
{
    if (degree == 0)
    {
        return [degree, i, knots](double t)
        {
            // The basis function for degree = 0 as per eq. 7
            (void)degree;
            double t_this = knots[i];
            double t_next = knots[i+1];
            if (t == knots[0])
                return (double)(t_next >= t && t >= t_this);
            return (double)(t_next >= t && t > t_this);
        };
    }
    else
    {
        return [degree, i, knots](double t)
        {
            // The basis function for degree > 0 as per eq. 8
            if (t < knots[0])
                return get_basis(degree, i, knots)(knots[0]);
            else if (t > knots[knots.rows() - 1])
                return get_basis(degree, i, knots)(knots[knots.rows() - 1]);
            double out = 0.;
            double t_this = knots[i];
            double t_next = knots[i + 1];
            double t_precog  = knots[i + degree];
            double t_horizon = knots[i + degree + 1];
            if (t_this == t_horizon)
                return 0.;

            double bottom = (t_precog - t_this);
            out = divide(t - t_this, bottom) * get_basis(degree - 1, i, knots)(t);

            bottom = (t_horizon - t_next);
            out += divide(t_horizon - t, bottom) * get_basis(degree - 1, i + 1, knots)(t);
            return out;
        };
    }
}


std::function<double(double)> get_basis_derivative(int order, int degree, int i, Eigen::VectorXd knots)
    // Return the derivation of the basis function
    // order of basis function
    // degree of basis function
    //
    // knots sequence
{
    if (order == 1)
    {
        return [degree, i, knots, order](double t)
        {
            (void)order;
            double out = 0;
            if (!(knots[i + degree] - knots[i] == 0))
            {
                out +=  get_basis(degree - 1, i, knots)(t) *
                        degree / (knots[i + degree] - knots[i]);
            }
            if (!(knots[i + degree + 1] - knots[i + 1] == 0))
            {
                out -=  get_basis(degree - 1, i + 1, knots)(t) *
                        degree / (knots[i + degree + 1] - knots[i + 1]);
            }
            return out;
        };
    }
    else
    {
        return [degree, i, knots, order](double t)
        {
            double out = 0;
            if (!(knots[i + degree] - knots[i] == 0))
            {
                out +=  get_basis_derivative(order - 1, degree - 1, i, knots)(t) *
                        degree / (knots[i + degree] - knots[i]);
            }
            if (!(knots[i + degree + 1] - knots[i + 1] == 0))
            {
                out -=  get_basis_derivative(order - 1, degree - 1, i + 1, knots)(t) *
                        degree / (knots[i + degree + 1] - knots[i + 1]);
            }
            return out;
        };
    }
}


NurbsBase2D::NurbsBase2D(Eigen::VectorXd u_knots, Eigen::VectorXd v_knots,
                     Eigen::VectorXd weights,
                     int degree_u, int degree_v)
{
    // assert(weights.size() == u_knots.size() * v_knots.size());
    this->u_knots = u_knots;
    this->v_knots = v_knots;
    this->weights = weights;
    this->degree_u = degree_u;
    this->degree_v = degree_v;
    for (int u_i = 0; u_i < u_knots.size() - degree_u - 1; u_i ++)
        this->u_functions.push_back(get_basis(degree_u, u_i, u_knots));
    for (int v_i = 0; v_i < v_knots.size() - degree_v - 1; v_i ++)
        this->v_functions.push_back(get_basis(degree_v, v_i, v_knots));
}

Eigen::VectorXd NurbsBase2D::getInfluenceVector(Eigen::Vector2d u)
{
    Eigen::VectorXd n_u, n_v;
    double sum_weights = 0;
    Eigen::VectorXd infl(this->u_functions.size() * this->v_functions.size());
    int i = 0;

    n_u.resize(this->u_functions.size());
    n_v.resize(this->v_functions.size());

    for (unsigned int i = 0; i < this->u_functions.size(); i ++)
        n_u[i] = this->u_functions[i](u.x());
    for (unsigned int i = 0; i < this->v_functions.size(); i ++)
        n_v[i] = this->v_functions[i](u.y());

    for (unsigned int u_i = 0; u_i < this->u_functions.size(); u_i++)
    {
        for (unsigned int v_i = 0; v_i < this->v_functions.size(); v_i++)
        {
            sum_weights += weights[i] * n_u[u_i] * n_v[v_i];
            infl[i] = weights[i] * n_u[u_i] * n_v[v_i];
            i ++;
        }
    }
    return infl / sum_weights;
}

void add_triplets(Eigen::VectorXd values, double row, std::vector<trip> &triplets)
{
    for (unsigned int i=0; i < values.size(); i++)
    {
        if (values(i) != 0.) {
            triplets.emplace_back(trip(row, i, values(i)));
        }
    }
}

spMat NurbsBase2D::getInfluenceMatrix(Eigen::Matrix<double, Eigen::Dynamic, 2> U)
{
    std::vector<trip> triplets;
    for (unsigned int row_index = 0; row_index < U.rows(); row_index++)
        add_triplets(this->getInfluenceVector(U.row(row_index)), row_index, triplets);
    spMat mat(U.rows(), this->u_functions.size() * this->v_functions.size());
    mat.setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}

void NurbsBase2D::computeFirstDerivatives()
{
    for (unsigned int u_i = 0; u_i < u_functions.size(); u_i ++)
        this->Du_functions.push_back(get_basis_derivative(1, this->degree_u, u_i, this->u_knots));
    for (unsigned int v_i = 0; v_i < v_functions.size(); v_i ++)
        this->Dv_functions.push_back(get_basis_derivative(1, this->degree_v, v_i, this->v_knots));
}

void NurbsBase2D::computeSecondDerivatives()
{
    for (unsigned int u_i = 0; u_i < u_functions.size(); u_i ++)
        this->DDu_functions.push_back(get_basis_derivative(2, this->degree_u, u_i, this->u_knots));
    for (unsigned int v_i = 0; v_i < v_functions.size(); v_i ++)
        this->DDv_functions.push_back(get_basis_derivative(2, this->degree_v, v_i, this->v_knots));
}

Eigen::VectorXd NurbsBase2D::getDuVector(Eigen::Vector2d u)
{
    Eigen::VectorXd A1, A2;
    double C1, C2;
    A1.resize(this->u_functions.size() * this->v_functions.size());
    A2.resize(this->u_functions.size() * this->v_functions.size());
    double A3 = 0;
    double A5 = 0;
    int i = 0;
    Eigen::VectorXd n_u, n_v, Dn_u;
    n_u.resize(this->u_functions.size());
    Dn_u.resize(this->u_functions.size());
    n_v.resize(this->v_functions.size());
    for (unsigned int u_i=0; u_i < this->u_functions.size(); u_i++)
    {
        // std::cout << "u_i: " << u_i << " , n_u: " << n_u.size()
        //           << " , Dn_u: " << Dn_u.size() << std::endl;
        n_u[u_i] = this->u_functions[u_i](u.x());
        Dn_u[u_i] = this->Du_functions[u_i](u.x());
    }
    for (unsigned int v_i=0; v_i < this->v_functions.size(); v_i++)
    {
        n_v[v_i] = this->v_functions[v_i](u.y());
        // std::cout << v_i << std::endl;
    }

    for (unsigned int u_i=0; u_i < this->u_functions.size(); u_i++)
    {
        for (unsigned int v_i=0; v_i < this->v_functions.size(); v_i++)
        {
            C1 = weights[i] * Dn_u[u_i] * n_v[v_i];
            C2 = weights[i] * n_u[u_i] * n_v[v_i];
            A1[i] = C1;
            A2[i] = C2;
            A3 += C2;
            A5 += C1;
            i ++;
        }
    }
    return (A1 * A3 - A2 * A5) / A3 / A3;
}

Eigen::VectorXd NurbsBase2D::getDvVector(Eigen::Vector2d u)
{
    Eigen::VectorXd A1, A2;
    double C1, C2;
    A1.resize(this->u_functions.size() * this->v_functions.size());
    A2.resize(this->u_functions.size() * this->v_functions.size());
    double A3 = 0;
    double A5 = 0;
    int i = 0;
    Eigen::VectorXd n_u, n_v, Dn_v;
    n_u.resize(this->u_functions.size());
    Dn_v.resize(this->v_functions.size());
    n_v.resize(this->v_functions.size());
    for (unsigned int u_i=0; u_i < this->u_functions.size(); u_i++)
    {
        n_u[u_i] = this->u_functions[u_i](u.x());
    }
    for (unsigned int v_i=0; v_i < this->v_functions.size(); v_i++)
    {
        n_v[v_i] = this->v_functions[v_i](u.y());
        Dn_v[v_i] = this->Dv_functions[v_i](u.y());
    }

    for (unsigned int u_i=0; u_i < this->u_functions.size(); u_i++)
    {
        for (unsigned int v_i=0; v_i < this->v_functions.size(); v_i++)
        {
            C1 = weights[i] * Dn_v[v_i] * n_u[u_i];
            C2 = weights[i] * n_v[v_i] * n_u[u_i];
            A1[i] = C1;
            A2[i] = C2;
            A3 += C2;
            A5 += C1;
            i ++;
        }
    }
    return (A1 * A3 - A2 * A5) / A3 / A3;
}


spMat NurbsBase2D::getDuMatrix(Eigen::Matrix<double, Eigen::Dynamic, 2> U)
{
    std::vector<trip> triplets;
    for (unsigned int row_index = 0; row_index < U.rows(); row_index++)
        add_triplets(this->getDuVector(U.row(row_index)), row_index, triplets);
    spMat mat(U.rows(), this->u_functions.size() * this->v_functions.size());
    mat.setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}

spMat NurbsBase2D::getDvMatrix(Eigen::Matrix<double, Eigen::Dynamic, 2> U)
{
    std::vector<trip> triplets;
    for (unsigned int row_index = 0; row_index < U.rows(); row_index++)
        add_triplets(this->getDvVector(U.row(row_index)), row_index, triplets);
    spMat mat(U.rows(), this->u_functions.size() * this->v_functions.size());
    mat.setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}


std::tuple<NurbsBase2D, Eigen::MatrixXd> NurbsBase2D::interpolateUBS(
    Eigen::Matrix<double, Eigen::Dynamic, 3> poles,
    int degree_u,
    int degree_v,
    int num_u_poles,
    int num_v_poles,
    int num_u_points,
    int num_v_points)
{
    double u_min = this->u_knots(0);
    double u_max = this->u_knots(this->u_knots.size() - 1);
    double v_min = this->v_knots(0);
    double v_max = this->v_knots(this->v_knots.size() - 1);
    Eigen::VectorXd weights, u_knots, v_knots;
    u_knots = NurbsBase1D::getKnotSequence(u_min, u_max, degree_u, num_u_poles);
    v_knots = NurbsBase1D::getKnotSequence(v_min, v_max, degree_v, num_v_poles);

    weights.resize((u_knots.rows() - degree_u - 1) * (v_knots.rows() - degree_v - 1));
    weights.setOnes();
    NurbsBase2D new_base(u_knots, v_knots, weights, degree_u, degree_v);
    Eigen::Matrix<double, Eigen::Dynamic, 2> uv_points = this->getUVMesh(num_u_points, num_v_points);
    Eigen::Matrix<double, Eigen::Dynamic, 3> xyz_points = this->getInfluenceMatrix(uv_points) * poles;
    spMat A = new_base.getInfluenceMatrix(uv_points);
    Eigen::LeastSquaresConjugateGradient<spMat > solver;
    solver.compute(A);
    Eigen::Matrix<double, Eigen::Dynamic, 3> new_poles = solver.solve(xyz_points);
    return std::tuple<NurbsBase2D, Eigen::MatrixXd >(new_base, new_poles);
}

Eigen::Matrix<double, Eigen::Dynamic, 2> NurbsBase2D::getUVMesh(int num_u_points, int num_v_points)
{
    double u_min = this->u_knots(0);
    double u_max = this->u_knots(this->u_knots.size() - 1);
    double v_min = this->v_knots(0);
    double v_max = this->v_knots(this->v_knots.size() - 1);
    Eigen::Matrix<double, Eigen::Dynamic, 2> uv_points;
    uv_points.resize(num_u_points * num_v_points, 2);
    int i = 0;
    for (int u = 0; u < num_u_points; u++)
    {
        for (int v = 0; v < num_v_points; v++)
        {
            uv_points(i, 0) = u_min + (u_max - u_min) * u / (num_u_points - 1);
            uv_points(i, 1) = v_min + (v_max - v_min) * v / (num_v_points - 1);
            i++;
        }

    }
    return uv_points;
}


NurbsBase1D::NurbsBase1D(Eigen::VectorXd u_knots, Eigen::VectorXd weights, int degree_u)
{
    this->u_knots = u_knots;
    this->weights = weights;
    this->degree_u = degree_u;
    for (int u_i = 0; u_i < u_knots.size() - degree_u - 1; u_i ++)
        this->u_functions.push_back(get_basis(degree_u, u_i, u_knots));
}

Eigen::VectorXd NurbsBase1D::getInfluenceVector(double u)
{
    Eigen::VectorXd n_u;
    double sum_weights = 0;
    Eigen::VectorXd infl(this->u_functions.size());

    n_u.resize(this->u_functions.size());

    for (unsigned int i = 0; i < this->u_functions.size(); i ++)
        n_u[i] = this->u_functions[i](u);

    for (unsigned int u_i = 0; u_i < this->u_functions.size(); u_i++)
    {
        sum_weights += weights[u_i] * n_u[u_i];
        infl[u_i] = weights[u_i] * n_u[u_i];
    }
    return infl / sum_weights;
}

spMat NurbsBase1D::getInfluenceMatrix(Eigen::VectorXd u)
{
    std::vector<trip> triplets;
    for (unsigned int row_index = 0; row_index < u.size(); row_index++)
        add_triplets(this->getInfluenceVector(u[row_index]), row_index, triplets);
    spMat mat(u.size(), this->u_functions.size());
    mat.setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}

void NurbsBase1D::computeFirstDerivatives()
{
    for (unsigned int u_i = 0; u_i < u_functions.size(); u_i ++)
        this->Du_functions.push_back(get_basis_derivative(1, this->degree_u, u_i, this->u_knots));
}

void NurbsBase1D::computeSecondDerivatives()
{
    for (unsigned int u_i = 0; u_i < u_functions.size(); u_i ++)
        this->DDu_functions.push_back(get_basis_derivative(2, this->degree_u, u_i, this->u_knots));
}


Eigen::VectorXd NurbsBase1D::getDuVector(double u)
{
    Eigen::VectorXd A1, A2;
    double C1, C2;
    double C3 = 0;
    double C4 = 0;
    int i = 0;
    A1.resize(this->u_functions.size());
    A2.resize(this->u_functions.size());
    Eigen::VectorXd n_u, Dn_u;
    n_u.resize(this->u_functions.size());
    Dn_u.resize(this->u_functions.size());

    for (unsigned u_i=0; u_i < this->u_functions.size(); u_i++)
    {
        n_u[u_i] = this->u_functions[u_i](u);
        Dn_u[u_i] = this->Du_functions[u_i](u);
    }

    for (unsigned int u_i=0; u_i < this->Du_functions.size(); u_i++)
    {
        C1 = weights[i] * Dn_u[u_i];
        C2 = weights[i] * n_u[u_i];
        C3 += C1;
        C4 += C2;

        A1[i] = C1;
        A2[i] = C2;
        i ++;
    }
    return (A1 * C4 - A2 * C3) / C4 / C4 ;
}


spMat NurbsBase1D::getDuMatrix(Eigen::VectorXd U)
{
    std::vector<trip> triplets;
    for (unsigned int row_index = 0; row_index < U.size(); row_index++)
        add_triplets(this->getDuVector(U[row_index]), row_index, triplets);
    spMat mat(U.size(), this->u_functions.size());
    mat.setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}

std::tuple<NurbsBase1D, Eigen::Matrix<double, Eigen::Dynamic, 3>> NurbsBase1D::interpolateUBS(
    Eigen::Matrix<double,
    Eigen::Dynamic, 3> poles,
    int degree,
    int num_poles,
    int num_points)
{
    double u_min = this->u_knots(0);
    double u_max = this->u_knots(this->u_knots.size() - 1);
    Eigen::VectorXd u_knots, weights;
    u_knots = NurbsBase1D::getKnotSequence(u_min, u_max, degree, num_poles);
    weights = NurbsBase1D::getWeightList(u_knots, degree);
    NurbsBase1D new_base(u_knots, weights, degree);
    Eigen::Matrix<double, Eigen::Dynamic, 1> u_points = this->getUMesh(num_points);
    Eigen::Matrix<double, Eigen::Dynamic, 3> xyz_points;
    xyz_points = this->getInfluenceMatrix(u_points) * poles;
    spMat A = new_base.getInfluenceMatrix(u_points);
    Eigen::LeastSquaresConjugateGradient<spMat > solver;
    solver.compute(A);
    Eigen::Matrix<double, Eigen::Dynamic, 3> new_poles = solver.solve(xyz_points);
    return std::tuple<NurbsBase1D, Eigen::Matrix<double, Eigen::Dynamic, 3> >(new_base, new_poles);
}

Eigen::VectorXd NurbsBase1D::getUMesh(int num_u_points)
{
    double u_min = this->u_knots(0);
    double u_max = this->u_knots(this->u_knots.size() - 1);
    Eigen::Matrix<double, Eigen::Dynamic, 1> u_points;
    u_points.setLinSpaced(num_u_points, u_min, u_max);
    return u_points;
}


}
// clang-format on
