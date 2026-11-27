// SPDX-License-Identifier: LGPL-2.1-or-later

// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008  Mikael Mayer
// Copyright  (C)  2008  Julia Jesse

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

#include "chainiksolverpos_nr_jl.hpp"

#include <limits>

namespace KDL
{
    ChainIkSolverPos_NR_JL::ChainIkSolverPos_NR_JL(const Chain& _chain, const JntArray& _q_min, const JntArray& _q_max, ChainFkSolverPos& _fksolver,ChainIkSolverVel& _iksolver,
                                             unsigned int _maxiter, double _eps):
        chain(_chain), nj(chain.getNrOfJoints()),
        q_min(_q_min), q_max(_q_max),
        iksolver(_iksolver), fksolver(_fksolver),
        delta_q(_chain.getNrOfJoints()),
        maxiter(_maxiter),eps(_eps)
    {

    }

    ChainIkSolverPos_NR_JL::ChainIkSolverPos_NR_JL(const Chain& _chain, ChainFkSolverPos& _fksolver,ChainIkSolverVel& _iksolver,
            unsigned int _maxiter, double _eps):
         chain(_chain), nj(chain.getNrOfJoints()),
         q_min(nj), q_max(nj),
         iksolver(_iksolver), fksolver(_fksolver),
         delta_q(nj),
         maxiter(_maxiter),eps(_eps)
    {
        q_min.data.setConstant(std::numeric_limits<double>::min());
        q_max.data.setConstant(std::numeric_limits<double>::max());
    }

    void ChainIkSolverPos_NR_JL::updateInternalDataStructures() {
       nj = chain.getNrOfJoints();
       q_min.data.conservativeResizeLike(Eigen::VectorXd::Constant(nj,std::numeric_limits<double>::min()));
       q_max.data.conservativeResizeLike(Eigen::VectorXd::Constant(nj,std::numeric_limits<double>::max()));
       iksolver.updateInternalDataStructures();
       fksolver.updateInternalDataStructures();
       delta_q.resize(nj);
    }

    int ChainIkSolverPos_NR_JL::CartToJnt(const JntArray& q_init, const Frame& p_in, JntArray& q_out)
    {
        if(nj != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);

        if(nj != q_init.rows() || nj != q_out.rows() || nj != q_min.rows() || nj != q_max.rows())
            return (error = E_SIZE_MISMATCH);

        q_out = q_init;

        unsigned int i;
        for(i=0;i<maxiter;i++){
            if ( fksolver.JntToCart(q_out,f) < 0)
                return (error = E_FKSOLVERPOS_FAILED);
            delta_twist = diff(f,p_in);

            if(Equal(delta_twist,Twist::Zero(),eps))
                break;

            if ( iksolver.CartToJnt(q_out,delta_twist,delta_q) < 0)
                return (error = E_IKSOLVERVEL_FAILED);
            Add(q_out,delta_q,q_out);

            for(unsigned int j=0; j<q_min.rows(); j++) {
                if(q_out(j) < q_min(j))
                q_out(j) = q_min(j);
            }


            for(unsigned int j=0; j<q_max.rows(); j++) {
                if(q_out(j) > q_max(j))
                    q_out(j) = q_max(j);
            }
        }

        if(i!=maxiter)
            return (error = E_NOERROR);
        else
            return (error = E_MAX_ITERATIONS_EXCEEDED);
    }

    int ChainIkSolverPos_NR_JL::setJointLimits(const JntArray& q_min_in, const JntArray& q_max_in) {
        if (q_min_in.rows() != nj || q_max_in.rows() != nj)
            return (error = E_SIZE_MISMATCH);
        q_min = q_min_in;
        q_max = q_max_in;
        return (error = E_NOERROR);
    }

    ChainIkSolverPos_NR_JL::~ChainIkSolverPos_NR_JL()
    {
    }

    const char* ChainIkSolverPos_NR_JL::strError(const int error) const
    {
        if (E_FKSOLVERPOS_FAILED == error) return "Internal forward position solver failed.";
        else if (E_IKSOLVERVEL_FAILED == error) return "Internal inverse velocity solver failed.";
        else return SolverI::strError(error);
    }
}

