// Copyright  (C)  2018  Craig Carignan <craigc at ssl dot umd dot edu>

// Version: 1.0
// Author: Craig Carignan
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

#include "ldl_solver_eigen.hpp"

namespace KDL{

    int ldl_solver_eigen(const Eigen::MatrixXd& A, const Eigen::VectorXd& v, Eigen::MatrixXd& L, Eigen::VectorXd& D, Eigen::VectorXd& vtmp, Eigen::VectorXd& q)
    {
        const int n = static_cast<int>(A.rows());
        int error=SolverI::E_NOERROR;

        //Check sizes
        if(A.cols()!=n || v.rows()!=n || L.rows()!=n || L.cols()!=n || D.rows()!=n || vtmp.rows()!=n || q.rows()!=n)
            return (error = SolverI::E_SIZE_MISMATCH);

        for(int i=0;i<n;++i)  {
            D(i)=A(i,i);
            if(i>0) {
                for(int j=0;j<=i-1;++j)
                    D(i) -= D(j)*L(i,j)*L(i,j);
            }
            for(int j=1;j<n;++j)  {
                if(j>i)  {
                    L(j,i)=A(i,j)/D(i);
                    if(i>0)  {
                        for(int k=0;k<=i-1;++k)
                            L(j,i) -= L(j,k)*L(i,k)*D(k)/D(i);
                    }
                }
            }
        }
        for(int i=0;i<n;++i)  {
            vtmp(i)=v(i);
            if(i>0) {
                for(int j=0;j<=i-1;++j)
                    vtmp(i) -= L(i,j)*vtmp(j);
            }
        }
        for(int i=n-1;i>=0;--i) {
            q(i)=vtmp(i)/D(i);
            if(i<n-1)  {
                for(int j=i+1;j<n;++j)
                    q(i) -= L(j,i)*q(j);
            }
        }
        // optional: changes diagonal elements of L to 1 as per LDL decomposition
        //           A = L * Ddiag * L^T where Ddiag(i,i) = D(i)
        for(int i=0;i<n;++i)  {
            L(i,i) = 1.;
        }
        // optional: sets upper triangular, off-diagonal elements of L to 0
        //           because algorithm does not do this automatically
        if ( n > 1 )
        {
            for(int i=0;i<n;++i)  {
                for(int j=i+1;j<n;++j)  {
                    L(i,j) = 0.0;
                }
            }
        }
        return(error);
    }
}
