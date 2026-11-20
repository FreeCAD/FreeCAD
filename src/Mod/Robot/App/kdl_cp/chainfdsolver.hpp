// Copyright  (C)  2018  Ruben Smits <ruben dot smits at intermodalics dot be>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at intermodalics dot be>
// Author: Craig Carignan <craigc at ssl dot umd dot edu>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot be>
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

#ifndef KDL_CHAIN_FDSOLVER_HPP
#define KDL_CHAIN_FDSOLVER_HPP

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
	class ChainFdSolver : public KDL::SolverI
	{
		public:
			/**
			 * Calculate forward dynamics from joint positions, joint velocities, joint torques/forces,
			 * and externally applied forces/torques to joint accelerations.
			 *
			 * @param q input joint positions
			 * @param q_dot input joint velocities
             * @param torque input joint torques
             * @param f_ext external forces
			 *
             * @param q_dotdot output joint accelerations
			 *
			 * @return if < 0 something went wrong
			 */
        virtual int CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &torques, const Wrenches& f_ext,JntArray &q_dotdot)=0;
	};

}

#endif
