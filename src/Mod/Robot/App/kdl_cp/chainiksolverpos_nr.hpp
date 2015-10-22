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

#ifndef KDLCHAINIKSOLVERPOS_NR_HPP
#define KDLCHAINIKSOLVERPOS_NR_HPP

#include "chainiksolver.hpp"
#include "chainfksolver.hpp"

namespace KDL {

    /**
     * Implementation of a general inverse position kinematics
     * algorithm based on Newton-Raphson iterations to calculate the
     * position transformation from Cartesian to joint space of a general
     * KDL::Chain.
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverPos_NR : public ChainIkSolverPos
    {
    public:
        /**
         * Constructor of the solver, it needs the chain, a forward
         * position kinematics solver and an inverse velocity
         * kinematics solver for that chain.
         *
         * @param chain the chain to calculate the inverse position for
         * @param fksolver a forward position kinematics solver
         * @param iksolver an inverse velocity kinematics solver
         * @param maxiter the maximum Newton-Raphson iterations,
         * default: 100
         * @param eps the precision for the position, used to end the
         * iterations, default: epsilon (defined in kdl.hpp)
         *
         * @return
         */
        ChainIkSolverPos_NR(const Chain& chain,ChainFkSolverPos& fksolver,ChainIkSolverVel& iksolver,
                            unsigned int maxiter=100,double eps=1e-6);
        ~ChainIkSolverPos_NR();

        virtual int CartToJnt(const JntArray& q_init, const Frame& p_in, JntArray& q_out);

    private:
        const Chain chain;
        ChainIkSolverVel& iksolver;
        ChainFkSolverPos& fksolver;
        JntArray delta_q;
        Frame f;
        Twist delta_twist;

        unsigned int maxiter;
        double eps;
    };

}

#endif
