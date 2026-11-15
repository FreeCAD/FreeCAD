// Copyright  (C)  2018  Craig Carignan <craigc at ssl dot umd dot edu>

// Version: 1.0
// Author: Craig Carignan <craigc at ssl dot umd dot edu>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot eu>
// URL: http://www.orocos.org/kdl

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// Inverse of a positive definite symmetric matrix times a vector
// based on LDL^T Decomposition
#ifndef LDL_SOLVER_EIGEN_HPP
#define LDL_SOLVER_EIGEN_HPP


#include <Eigen/Core>
#include "../solveri.hpp"

namespace KDL
{
    /**
     * \brief Solves the system of equations Aq = v for q via LDL decomposition,
     *        where A is a square positive definite matrix
     *
     * The algorithm factor A into the product of three matrices LDL^T, where L
     * is a lower triangular matrix and D is a diagonal matrix.  This allows q
     * to be computed without explicitly inverting A.  Note that the LDL decomposition
     * is a variant of the classical Cholesky Decomposition that does not require
     * the computation of square roots.
     * Input parameters:
     * @param A matrix<double>(nxn)
     * @param v vector<double> n
     * @param vtmp vector<double> n [temp variable]
     * Output parameters:
     * @param L matrix<double>(nxn)
     * @param D vector<double> n
     * @param q vector<double> n
     * @return 0 if successful, E_SIZE_MISMATCH if dimensions do not match
     * References:
     * https://en.wikipedia.org/wiki/Cholesky_decomposition
     */
    int ldl_solver_eigen(const Eigen::MatrixXd& A, const Eigen::VectorXd& v, Eigen::MatrixXd& L, Eigen::VectorXd& D, Eigen::VectorXd& vtmp, Eigen::VectorXd& q);
}
#endif
