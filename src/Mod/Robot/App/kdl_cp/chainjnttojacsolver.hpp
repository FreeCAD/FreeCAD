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

#include "solveri.hpp"
#include "frames.hpp"
#include "jacobian.hpp"
#include "jntarray.hpp"
#include "chain.hpp"

namespace KDL
{
    /**
     * @brief  Class to calculate the jacobian of a general
     * KDL::Chain, it is used by other solvers. It should not be used
     * outside of KDL.
     *
     *
     */

    class ChainJntToJacSolver : public SolverI
    {
    public:
        static const int E_JAC_FAILED = -100; //! Jac solver failed

        explicit ChainJntToJacSolver(const Chain& chain);
        virtual ~ChainJntToJacSolver();
        /**
         * Calculate the jacobian expressed in the base frame of the
         * chain, with reference point at the end effector of the
         * *chain. The algorithm is similar to the one used in
         * KDL::ChainFkSolverVel_recursive
         *
         * @param q_in input joint positions
         * @param jac output jacobian
         *
         * @return always returns 0
         */
        virtual int JntToJac(const JntArray& q_in, Jacobian& jac, int segmentNR=-1);
        
        int setLockedJoints(const std::vector<bool> locked_joints);

        /// @copydoc KDL::SolverI::strError()
        virtual const char* strError(const int error) const;

    private:
        const Chain chain;
        Twist t_tmp;
        Frame T_tmp;
        std::vector<bool> locked_joints_;
        unsigned int nr_of_unlocked_joints_;
    };
}