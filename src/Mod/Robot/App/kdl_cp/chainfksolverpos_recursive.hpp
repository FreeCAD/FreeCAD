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

#ifndef KDLCHAINFKSOLVERPOS_RECURSIVE_HPP
#define KDLCHAINFKSOLVERPOS_RECURSIVE_HPP

#include "chainfksolver.hpp"

namespace KDL {

    /**
     * Implementation of a recursive forward position kinematics
     * algorithm to calculate the position transformation from joint
     * space to Cartesian space of a general kinematic chain (KDL::Chain).
     *
     * @ingroup KinematicFamily
     */
    class ChainFkSolverPos_recursive : public ChainFkSolverPos
    {
    public:
        ChainFkSolverPos_recursive(const Chain& chain);
        ~ChainFkSolverPos_recursive();

        virtual int JntToCart(const JntArray& q_in, Frame& p_out, int segmentNr=-1);
        virtual int JntToCart(const JntArray& q_in, std::vector<Frame>& p_out, int segmentNr=-1);

        virtual void updateInternalDataStructures() {};

    private:
        const Chain& chain;
    };

}

#endif
