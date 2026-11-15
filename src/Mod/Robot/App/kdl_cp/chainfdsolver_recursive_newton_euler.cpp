// Copyright  (C)  2018  Craig Carignan <craigc at ssl dot umd dot edu>

// Version: 1.0
// Author: Craig Carignan <craigc at ssl dot umd dot edu>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot be>
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

#include "chainfdsolver_recursive_newton_euler.hpp"
#include "utilities/ldl_solver_eigen.hpp"
#include "frames_io.hpp"
#include "kinfam_io.hpp"

namespace KDL{

    ChainFdSolver_RNE::ChainFdSolver_RNE(const Chain& _chain, Vector _grav):
        chain(_chain),
        DynSolver(chain, _grav),
        IdSolver(chain, _grav),
        nj(chain.getNrOfJoints()),
        ns(chain.getNrOfSegments()),
        H(nj),
        Tzeroacc(nj),
        H_eig(nj,nj),
        Tzeroacc_eig(nj),
        L_eig(nj,nj),
        D_eig(nj),
        r_eig(nj),
        acc_eig(nj)
    {
    }

    void ChainFdSolver_RNE::updateInternalDataStructures() {
        nj = chain.getNrOfJoints();
        ns = chain.getNrOfSegments();
    }

    int ChainFdSolver_RNE::CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &torques, const Wrenches& f_ext, JntArray &q_dotdot)
    {
        if(nj != chain.getNrOfJoints() || ns != chain.getNrOfSegments())
            return (error = E_NOT_UP_TO_DATE);

        //Check sizes of function parameters
        if(q.rows()!=nj || q_dot.rows()!=nj || q_dotdot.rows()!=nj || torques.rows()!=nj || f_ext.size()!=ns)
            return (error = E_SIZE_MISMATCH);

        // Inverse Dynamics:
        //   T = H * qdd + Tcor + Tgrav - J^T * Fext
        // Forward Dynamics:
        //   1. Call ChainDynParam->JntToMass to get H
        //   2. Call ChainIdSolver_RNE->CartToJnt with qdd=0 to get Tcor+Tgrav-J^T*Fext
        //   3. Calculate qdd = H^-1 * T where T are applied joint torques minus non-inertial internal torques

        // Calculate Joint Space Inertia Matrix
        error = DynSolver.JntToMass(q, H);
        if (error < 0)
            return (error);

        // Calculate non-inertial internal torques by inputting zero joint acceleration to ID
        for(unsigned int i=0;i<nj;i++){
            q_dotdot(i) = 0.;
        }
        error = IdSolver.CartToJnt(q, q_dot, q_dotdot, f_ext, Tzeroacc);
        if (error < 0)
            return (error);

        // Calculate acceleration using inverse symmetric matrix times vector
        for(unsigned int i=0;i<nj;i++){
            Tzeroacc_eig(i) =  (torques(i)-Tzeroacc(i));
            for(unsigned int j=0;j<nj;j++){
                H_eig(i,j) =  H(i,j);
            }
        }
        ldl_solver_eigen(H_eig, Tzeroacc_eig, L_eig, D_eig, r_eig, acc_eig);
        for(unsigned int i=0;i<nj;i++){
            q_dotdot(i) = acc_eig(i);
        }

        return (error = E_NOERROR);
    }

    void ChainFdSolver_RNE::RK4Integrator(unsigned int& nj, const double& t, double& dt, KDL::JntArray& q, KDL::JntArray& q_dot,
                                          KDL::JntArray& torques, KDL::Wrenches& f_ext, KDL::ChainFdSolver_RNE& fdsolver,
                                          KDL::JntArray& q_dotdot, KDL::JntArray& dq, KDL::JntArray& dq_dot,
                                          KDL::JntArray& q_temp, KDL::JntArray& q_dot_temp)
    {
        fdsolver.CartToJnt(q, q_dot, torques, f_ext, q_dotdot);
        for (unsigned int i=0; i<nj; ++i)
        {
            q_temp(i) = q(i) + q_dot(i)*dt/2.0;
            q_dot_temp(i) = q_dot(i) + q_dotdot(i)*dt/2.0;
            dq(i) = q_dot(i);
            dq_dot(i) = q_dotdot(i);
        }
        fdsolver.CartToJnt(q_temp, q_dot_temp, torques, f_ext, q_dotdot);
        for (unsigned int i=0; i<nj; ++i)
        {
            q_temp(i) = q(i) + q_dot_temp(i)*dt/2.0;
            q_dot_temp(i) = q_dot(i) + q_dotdot(i)*dt/2.0;
            dq(i) += 2.0*q_dot_temp(i);
            dq_dot(i) += 2.0*q_dotdot(i);
        }
        fdsolver.CartToJnt(q_temp, q_dot_temp, torques, f_ext, q_dotdot);
        for (unsigned int i=0; i<nj; ++i)
        {
            q_temp(i) = q(i) + q_dot_temp(i)*dt;
            q_dot_temp(i) = q_dot(i) + q_dotdot(i)*dt;
            dq(i) += 2.0*q_dot_temp(i);
            dq_dot(i) += 2.0*q_dotdot(i);
        }
        fdsolver.CartToJnt(q_temp, q_dot_temp, torques, f_ext, q_dotdot);
        for (unsigned int i=0; i<nj; ++i)
        {
            dq(i) = (dq(i)+q_dot_temp(i))*dt/6.0;
            dq_dot(i) = (dq_dot(i)+q_dotdot(i))*dt/6.0;
        }
        for (unsigned int i=0; i<nj; ++i)
        {
            q(i) += dq(i);
            q_dot(i) += dq_dot(i);
        }
        return;
    }

}//namespace
