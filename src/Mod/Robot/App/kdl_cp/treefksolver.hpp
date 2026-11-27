// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008  Julia Jesse

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

#ifndef KDL_TREE_FKSOLVER_HPP
#define KDL_TREE_FKSOLVER_HPP

#include <string>

#include "tree.hpp"
//#include "framevel.hpp"
//#include "frameacc.hpp"
#include "jntarray.hpp"
//#include "jntarrayvel.hpp"
//#include "jntarrayacc.hpp"

namespace KDL {

    /**
	  * \brief This <strong>abstract</strong> class encapsulates a
	  * solver for the forward position kinematics for a KDL::Tree.
     *
     * @ingroup KinematicFamily
     */

    //Forward definition
    class TreeFkSolverPos {
    public:
        /**
         * Calculate forward position kinematics for a KDL::Tree,
         * from joint coordinates to cartesian pose.
         *
         * @param q_in input joint coordinates
         * @param p_out reference to output cartesian pose
         *
         * @return if < 0 something went wrong
         */
        virtual int JntToCart(const JntArray& q_in, Frame& p_out, std::string segmentName)=0;
        virtual ~TreeFkSolverPos(){};
    };

    /**
     * \brief This <strong>abstract</strong> class encapsulates a solver
     * for the forward velocity kinematics for a KDL::Tree.
     *
     * @ingroup KinematicFamily
     */
//    class TreeFkSolverVel {
//    public:
        /**
         * Calculate forward position and velocity kinematics, from
         * joint coordinates to cartesian coordinates.
         *
         * @param q_in input joint coordinates (position and velocity)
         * @param out output cartesian coordinates (position and velocity)
         *
         * @return if < 0 something went wrong
         */
//        virtual int JntToCart(const JntArrayVel& q_in, FrameVel& out,int segmentNr=-1)=0;

//        virtual ~TreeFkSolverVel(){};
//    };
    
    /**
     * \brief This <strong>abstract</strong> class encapsulates a solver
     * for the forward acceleration kinematics for a KDL::Tree.
     *
     * @ingroup KinematicFamily
     */

//    class TreeFkSolverAcc {
//   public:
        /**
         * Calculate forward position, velocity and acceleration
         * kinematics, from joint coordinates to cartesian coordinates
         *
         * @param q_in input joint coordinates (position, velocity and
         * acceleration
         @param out output cartesian coordinates (position, velocity
         * and acceleration
         *
         * @return if < 0 something went wrong
         */
//    virtual int JntToCart(const JntArrayAcc& q_in, FrameAcc& out,int segmentNr=-1)=0;

//        virtual ~TreeFkSolverAcc()=0;
//    };


}//end of namespace KDL

#endif
