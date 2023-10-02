/***************************************************************************
 *   Copyright (c) 2011 Konstantinos Poulios <logari81@gmail.com>          *
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

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

#include <Eigen/QR>
#include <iostream>

using namespace Eigen;

// minimizes ( 0.5 * x^T * H * x + g^T * x ) under the condition ( A*x + c = 0 )
// it returns the solution in x, the row-space of A in Y, and the null space of A in Z
int qp_eq(MatrixXd& H, VectorXd& g, MatrixXd& A, VectorXd& c, VectorXd& x, MatrixXd& Y, MatrixXd& Z)
{
    FullPivHouseholderQR<MatrixXd> qrAT(A.transpose());
    MatrixXd Q = qrAT.matrixQ();

    size_t params_num = qrAT.rows();
    size_t constr_num = qrAT.cols();
    size_t rank = qrAT.rank();

    if (rank != constr_num || constr_num > params_num) {
        return -1;
    }

    // A^T = Q*R*P^T = Q1*R1*P^T
    // Q = [Q1,Q2], R=[R1;0]
    // Y = Q1 * inv(R^T) * P^T
    // Z = Q2
    Y = qrAT.matrixQR()
            .topRows(constr_num)
            .triangularView<Upper>()
            .transpose()
            .solve<OnTheRight>(Q.leftCols(rank))
        * qrAT.colsPermutation().transpose();
    if (params_num == rank) {
        x = -Y * c;
    }
    else {
        Z = Q.rightCols(params_num - rank);

        MatrixXd ZTHZ = Z.transpose() * H * Z;
        VectorXd rhs = Z.transpose() * (H * Y * c - g);

        VectorXd y = ZTHZ.colPivHouseholderQr().solve(rhs);

        x = -Y * c + Z * y;
    }

    return 0;
}
