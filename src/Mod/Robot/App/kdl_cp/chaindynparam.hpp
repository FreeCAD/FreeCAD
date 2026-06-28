// SPDX-License-Identifier: LGPL-2.1-or-later

// Copyright  (C)  2009  Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>

// Version: 1.0
// Author: Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>
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

#pragma once

#include "chainidsolver_recursive_newton_euler.hpp"
#include "articulatedbodyinertia.hpp"
#include "jntspaceinertiamatrix.hpp"

namespace KDL {

    /**
     * Implementation of a method to calculate the matrices H (inertia),C(coriolis) and G(gravitation) 
     * for the calculation torques out of the pose and derivatives.
     * (inverse dynamics)
     *
     * The algorithm implementation for H is based on the book "Rigid Body
     * Dynamics Algorithms" of Roy Featherstone, 2008
     * (ISBN:978-0-387-74314-1) See page 107 for the pseudo-code.
     * This algorithm is extended for the use of fixed joints
     *
     * It calculates the joint-space inertia matrix, given the motion of
     * the joints (q,qdot,qdotdot), external forces on the segments
     * (expressed in the segments reference frame) and the dynamical
     * parameters of the segments.
     */
    class ChainDynParam
    {
    public:
        ChainDynParam(const Chain& chain, Vector _grav);
        virtual ~ChainDynParam();

        virtual int JntToCoriolis(const JntArray &q, const JntArray &q_dot, JntArray &coriolis);
	virtual int JntToMass(const JntArray &q, JntSpaceInertiaMatrix& H);
	virtual int JntToGravity(const JntArray &q,JntArray &gravity);

    private:
        const Chain chain;
	//int nr;
	unsigned int nj;
        unsigned int ns;
	Vector grav;
	Vector vectornull;
	JntArray jntarraynull;
	ChainIdSolver_RNE chainidsolver_coriolis;
	ChainIdSolver_RNE chainidsolver_gravity;
	std::vector<Wrench> wrenchnull;
        std::vector<Frame> X;
        std::vector<Twist> S;
        //std::vector<RigidBodyInertia> I;
        std::vector<ArticulatedBodyInertia> Ic;
        Wrench F;
        Twist ag;
	
    };

}