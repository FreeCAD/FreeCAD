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
        jnt2jac(chain),
        jac(chain.getNrOfJoints()),
        transpose(chain.getNrOfJoints()>6),toggle(true),
        m(max(6,chain.getNrOfJoints())),
        n(min(6,chain.getNrOfJoints())),
        jac_eigen(m,n),
        U(MatrixXd::Identity(m,m)),
        V(MatrixXd::Identity(n,n)),
        B(m,n),
        S(n),
        tempi(m),
        tempj(m),
        UY(VectorXd::Zero(6)),
        SUY(VectorXd::Zero(chain.getNrOfJoints())),
        qdot_eigen(chain.getNrOfJoints()),
        v_in_eigen(6)
    {
    }

    ChainIkSolverVel_pinv_givens::~ChainIkSolverVel_pinv_givens()
    {
    }


    int ChainIkSolverVel_pinv_givens::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        toggle=!toggle;

        jnt2jac.JntToJac(q_in,jac);

        for(unsigned int i=0;i<6;i++)
            v_in_eigen(i)=v_in(i);

        for(unsigned int i=0;i<m;i++){
            for(unsigned int j=0;j<n;j++)
                if(transpose)
                    jac_eigen(i,j)=jac(j,i);
                else
                    jac_eigen(i,j)=jac(i,j);
        }
        int ret = svd_eigen_Macie(jac_eigen,U,S,V,B,tempi,1e-15,toggle);
        //std::cout<<"# sweeps: "<<ret<<std::endl;

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

        return ret;

    }

}
