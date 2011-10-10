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
        chain(_chain),fksolver(_fksolver),iksolver(_iksolver),delta_q(_chain.getNrOfJoints()),
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
                iksolver.CartToJnt(q_out,delta_twist,delta_q);
                Add(q_out,delta_q,q_out);
                if(Equal(delta_twist,Twist::Zero(),eps))
                    break;
            }
            if(i!=maxiter)
                return 0;
            else
                return -3;
    }

    ChainIkSolverPos_NR::~ChainIkSolverPos_NR()
    {
    }

}

