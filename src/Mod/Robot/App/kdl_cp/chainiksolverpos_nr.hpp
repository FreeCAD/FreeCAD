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
        static const int E_IKSOLVER_FAILED = -100; //! Child IK solver failed

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

        /**
         * Find an output joint pose \a q_out, given a starting joint pose
         * \a q_init and a desired cartesian pose \a p_in
         *
         * @return:
         *  E_NOERROR=solution converged to <eps in maxiter
         *  E_DEGRADED=solution converged to <eps in maxiter, but solution is
         *  degraded in quality (e.g. pseudo-inverse in iksolver is singular)
         *  E_IKSOLVER_FAILED=velocity solver failed
         *  E_NO_CONVERGE=solution did not converge (e.g. large displacement, low iterations)
         */
        virtual int CartToJnt(const JntArray& q_init, const Frame& p_in, JntArray& q_out);

        /// @copydoc KDL::SolverI::strError()
        virtual const char* strError(const int error) const;

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
