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

#include "chainiksolverpos_nr.hpp"

namespace KDL
{
    ChainIkSolverPos_NR::ChainIkSolverPos_NR(const Chain& _chain,ChainFkSolverPos& _fksolver,ChainIkSolverVel& _iksolver,
                                             unsigned int _maxiter, double _eps):
        chain(_chain),iksolver(_iksolver),fksolver(_fksolver),delta_q(_chain.getNrOfJoints()),
        maxiter(_maxiter),eps(_eps)
    {
    }

    int ChainIkSolverPos_NR::CartToJnt(const JntArray& q_init, const Frame& p_in, JntArray& q_out)
    {
            q_out = q_init;

            unsigned int i;
            for(i=0;i<maxiter;i++){
                fksolver.JntToCart(q_out,f);
                delta_twist = diff(f,p_in);
                const int rc = iksolver.CartToJnt(q_out,delta_twist,delta_q);
                if (E_NOERROR > rc)
                    return (error = E_IKSOLVER_FAILED);
                // we chose to continue if the child solver returned a positive
                // "error", which may simply indicate a degraded solution
                Add(q_out,delta_q,q_out);
                if(Equal(delta_twist,Twist::Zero(),eps))
                    // converged, but possibly with a degraded solution
                    return (rc > E_NOERROR ? E_DEGRADED : E_NOERROR);
            }
            return (error = E_NO_CONVERGE);        // failed to converge
    }

    ChainIkSolverPos_NR::~ChainIkSolverPos_NR()
    {
    }

    const char* ChainIkSolverPos_NR::strError(const int error) const
    {
        if (E_IKSOLVER_FAILED == error) return "Child IK solver failed";
        else return SolverI::strError(error);
    }
}

