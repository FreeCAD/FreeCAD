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

#include "chain.hpp"
#include "frames.hpp"
#include "framevel.hpp"
#include "frameacc.hpp"
#include "jntarray.hpp"
#include "jntarrayvel.hpp"
#include "jntarrayacc.hpp"
#include "solveri.hpp"

namespace KDL {

    /**
	  * \brief This <strong>abstract</strong> class encapsulates the inverse
	  * position solver for a KDL::Chain.
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverPos : public KDL::SolverI {
    public:
        /**
         * Calculate inverse position kinematics, from cartesian
         *coordinates to joint coordinates.
         *
         * @param q_init initial guess of the joint coordinates
         * @param p_in input cartesian coordinates
         * @param q_out output joint coordinates
         *
         * @return if < 0 something went wrong
         */
        virtual int CartToJnt(const JntArray& q_init, const Frame& p_in, JntArray& q_out)=0;

        virtual ~ChainIkSolverPos(){};
    };

    /**
	  * \brief This <strong>abstract</strong> class encapsulates the inverse
	  * velocity solver for a KDL::Chain.
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverVel : public KDL::SolverI {
    public:
        /**
         * Calculate inverse velocity kinematics, from joint positions
         *and cartesian velocity to joint velocities.
         *
         * @param q_in input joint positions
         * @param v_in input cartesian velocity
         * @param qdot_out output joint velocities
         *
         * @return if < 0 something went wrong
         */
        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)=0;
        /**
         * Calculate inverse position and velocity kinematics, from
         *cartesian position and velocity to joint positions and velocities.
         *
         * @param q_init initial joint positions
         * @param v_in input cartesian position and velocity
         * @param q_out output joint position and velocity
         *
         * @return if < 0 something went wrong
         */
        virtual int CartToJnt(const JntArray& q_init, const FrameVel& v_in, JntArrayVel& q_out)=0;

        virtual ~ChainIkSolverVel(){};

    };

    /**
	  * \brief This <strong>abstract</strong> class encapsulates the inverse
	  * acceleration solver for a KDL::Chain.
     *
     * @ingroup KinematicFamily
     */

    class ChainIkSolverAcc : public KDL::SolverI {
    public:
        /**
         * Calculate inverse acceleration kinematics from joint
         * positions, joint velocities and cartesian acceleration to joint accelerations.
         *
         * @param q_in input joint positions
         * @param qdot_in input joint velocities
         * @param a_in input cartesian acceleration
         * @param qdotdot_out output joint accelerations
         *
         * @return if < 0 something went wrong
         */
        virtual int CartToJnt(const JntArray& q_in, const JntArray& qdot_in, const Twist a_in,
                         JntArray& qdotdot_out)=0;
        /**
         * Calculate inverse position, velocity and acceration
         *kinematics from cartesian coordinates to joint coordinates
         *
         * @param q_init initial guess for joint positions
         * @param a_in input cartesian position, velocity and acceleration
         * @param q_out output joint position, velocity and acceleration
         *
         * @return if < 0 something went wrong
         */
        virtual int CartTojnt(const JntArray& q_init, const FrameAcc& a_in,
                         JntArrayAcc& q_out)=0;

        /**
         * Calculate inverse velocity and acceleration kinematics from
         * joint positions and cartesian velocity and acceleration to
         * joint velocities and accelerations.
         *
         * @param q_in input joint positions
         * @param v_in input cartesian velocity
         * @param a_in input cartesian acceleration
         * @param qdot_out output joint velocities
         * @param qdotdot_out output joint accelerations
         *
         * @return if < 0 something went wrong
         */
        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, const Twist& a_in,
                         JntArray& qdot_out, JntArray& qdotdot_out)=0;
        /**
         * Calculate inverse position and acceleration kinematics from
         *joint velocities and cartesian position and acceleration to
         *joint positions and accelerations
         *
         * @param q_init initial guess for joint positions
         * @param p_in input cartesian position
         * @param qdot_in input joint velocities
         * @param a_in input cartesian acceleration
         * @param q_out output joint positions
         * @param qdotdot_out output joint accelerations
         *
         * @return if < 0 something went wrong
         */
        virtual int CartTojnt(const JntArray& q_init, const Frame& p_in, const JntArray& qdot_in, const Twist& a_in,
                         JntArray& q_out, JntArray& qdotdot_out)=0;

        virtual ~ChainIkSolverAcc(){};
    };


}//end of namespace KDL