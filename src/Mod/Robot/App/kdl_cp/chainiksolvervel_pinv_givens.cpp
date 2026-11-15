// SPDX-License-Identifier: LGPL-2.1-or-later

// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Maintainer: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
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

#include "chainiksolvervel_pinv_givens.hpp"
#include "utilities/svd_eigen_Macie.hpp"

namespace KDL
{
    ChainIkSolverVel_pinv_givens::ChainIkSolverVel_pinv_givens(const Chain& _chain):
        chain(_chain),
        nj(chain.getNrOfJoints()),
        jnt2jac(chain),
        jac(nj),
        transpose(nj>6),toggle(true),
        m(static_cast<unsigned int>(max(6,nj))),
        n(static_cast<unsigned int>(min(6,nj))),
        jac_eigen(m,n),
        U(Eigen::MatrixXd::Identity(m,m)),
        V(Eigen::MatrixXd::Identity(n,n)),
        B(m,n),
        S(n),
        tempi(m),
        UY(Eigen::VectorXd::Zero(6)),
        SUY(Eigen::VectorXd::Zero(nj)),
        qdot_eigen(nj),
        v_in_eigen(6)
    {
    }

    void ChainIkSolverVel_pinv_givens::updateInternalDataStructures() {
        nj = chain.getNrOfJoints();
        jnt2jac.updateInternalDataStructures();
        jac.resize(nj);
        transpose = (nj > 6);
        m = static_cast<unsigned int>(max(6,nj));
        n = static_cast<unsigned int>(min(6,nj));
        jac_eigen.conservativeResize(m,n);
        U.conservativeResizeLike(Eigen::MatrixXd::Identity(m,m));
        V.conservativeResizeLike(Eigen::MatrixXd::Identity(n,n));
        B.conservativeResize(m,n);
        S.conservativeResize(n);
        tempi.conservativeResize(m);
        SUY.conservativeResizeLike(Eigen::VectorXd::Zero(nj));
        qdot_eigen.conservativeResize(nj);
    }

    ChainIkSolverVel_pinv_givens::~ChainIkSolverVel_pinv_givens()
    {
    }


    int ChainIkSolverVel_pinv_givens::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        if (nj != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);

        if (nj != q_in.rows() || nj != qdot_out.rows())
            return (error = E_SIZE_MISMATCH);

        toggle=!toggle;

        error = jnt2jac.JntToJac(q_in,jac);
        if (E_NOERROR > error )
            return error;

        for(unsigned int i=0;i<6;i++)
            v_in_eigen(i)=v_in(i);

        for(unsigned int i=0;i<m;i++){
            for(unsigned int j=0;j<n;j++)
                if(transpose)
                    jac_eigen(i,j)=jac(j,i);
                else
                    jac_eigen(i,j)=jac(i,j);
        }
        svd_eigen_Macie(jac_eigen,U,S,V,B,tempi,1e-15,toggle);

        if(transpose)
            UY.noalias() = V.transpose() * v_in_eigen;
        else
            UY.noalias() = U.transpose() * v_in_eigen;

        for (unsigned int i = 0; i < n; i++){
            double wi = UY(i);
            double alpha = S(i);
            
            if (alpha != 0)
                alpha = 1.0 / alpha;
            else
                alpha = 0.0;
            SUY(i)= alpha * wi;
        }

        if(transpose)
            qdot_eigen.noalias() = U * SUY;
        else
            qdot_eigen.noalias() = V * SUY;

        for (unsigned int j=0;j<chain.getNrOfJoints();j++)
            qdot_out(j)=qdot_eigen(j);

        return (error = E_NOERROR);
    }
}
