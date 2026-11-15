// Copyright  (C)  2009  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

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

#ifndef KDL_CHAIN_IDSOLVER_HPP
#define KDL_CHAIN_IDSOLVER_HPP

#include "chain.hpp"
#include "frames.hpp"
#include "jntarray.hpp"
#include "solveri.hpp"

namespace KDL
{

    typedef std::vector<Wrench> Wrenches;

	/**
	 * \brief This <strong>abstract</strong> class encapsulates the inverse
	 * dynamics solver for a KDL::Chain.
	 *
	 */
	class ChainIdSolver : public KDL::SolverI
	{
		public:
			/** 
			 * Calculate inverse dynamics, from joint positions, velocity, acceleration, external forces
			 * to joint torques/forces.
			 * 
			 * @param q input joint positions
			 * @param q_dot input joint velocities
			 * @param q_dotdot input joint accelerations
			 *
			 * @param torque output joint torques
			 * 
			 * @return if < 0 something went wrong
			 */
        virtual int CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &q_dotdot, const Wrenches& f_ext,JntArray &torques)=0;

        // Need functions to return the manipulator mass, coriolis and gravity matrices - Lagrangian Formulation.
	};

}

#endif
