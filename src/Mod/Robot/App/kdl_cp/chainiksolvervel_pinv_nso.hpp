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

#pragma once

#include "chainiksolver.hpp"
#include "chainjnttojacsolver.hpp"
#include <Eigen/Core>

namespace KDL
{
    
    // FIXME: seems this class is unused/unmaintained/unfinished for several years
    //        it supposed to be either fixer or removed

    /**
     * Implementation of a inverse velocity kinematics algorithm based
     * on the generalize pseudo inverse to calculate the velocity
     * transformation from Cartesian to joint space of a general
     * KDL::Chain. It uses a svd-calculation based on householders
     * rotations.
     *
     * In case of a redundant robot this solver optimizes the following criterium:
     * g=0.5*sum(weight*(Desired_joint_positions - actual_joint_positions))^2 as described in 
     *  A. Liegeois. Automatic supervisory control of the configuration and 
     * behavior of multibody mechanisms. IEEE Transactions on Systems, Man, and 
     * Cybernetics, 7(12):868–871, 1977
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverVel_pinv_nso : public ChainIkSolverVel
    {
    public:
        /**
         * Constructor of the solver
         *
         * @param chain the chain to calculate the inverse velocity
         * kinematics for
         * @param opt_pos the desired positions of the chain used by to resolve the redundancy
         * @param weights the weights applied in the joint space
         * @param eps if a singular value is below this value, its
         * inverse is set to zero, default: 0.00001
         * @param maxiter maximum iterations for the svd calculation,
         * default: 150
         * @param alpha the null-space velocity gain
         *
         */
        
        // FIXME: alpha is int but is initialized with a float value.
        ChainIkSolverVel_pinv_nso(const Chain& chain, JntArray opt_pos, JntArray weights, double eps=0.00001,int maxiter=150, double alpha = 0.25);
        explicit ChainIkSolverVel_pinv_nso(const Chain& chain, double eps=0.00001,int maxiter=150, double alpha = 0.25);
        ~ChainIkSolverVel_pinv_nso();

        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out);
        /**
         * not (yet) implemented.
         *
         */
        virtual int CartToJnt(const JntArray& /*q_init*/, const FrameVel& /*v_in*/, JntArrayVel& /*q_out*/){return -1;};

        /**
         *Set joint weights for optimization criterion
         *
         *@param weights the joint weights
         *
         */
        virtual int setWeights(const JntArray &weights);

        /**
         *Set optimal joint positions
         *
         *@param opt_pos optimal joint positions
         *
         */
        virtual int setOptPos(const JntArray &opt_pos);

        /**
         *Set null space velocity gain
         *
         *@param alpha NUllspace velocity cgain
         *
         */
        virtual int setAlpha(const double alpha);

    private:
        const Chain chain;
        ChainJntToJacSolver jnt2jac;
        unsigned int nj;
        Jacobian jac;
        Eigen::MatrixXd U;
        Eigen::VectorXd S;
        Eigen::VectorXd Sinv;
        Eigen::MatrixXd V;
        Eigen::VectorXd tmp;
        Eigen::VectorXd tmp2;
        double eps;
        int maxiter;

        double alpha;
        JntArray weights;
        JntArray opt_pos;
    };
}