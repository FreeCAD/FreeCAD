// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * treeiksolver.hpp
 *
 *  Created on: Nov 28, 2008
 *      Author: rubensmits
 */

#pragma once

#include "tree.hpp"
#include "jntarray.hpp"
#include "frames.hpp"
#include <map>

namespace KDL {

typedef std::map<std::string, Twist> Twists;
typedef std::map<std::string, Jacobian> Jacobians;
typedef std::map<std::string, Frame> Frames;

/**
 * \brief This <strong>abstract</strong> class encapsulates the inverse
 * position solver for a KDL::Chain.
 *
 * @ingroup KinematicFamily
 */
class TreeIkSolverPos {
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
	  *         otherwise (>=0) remaining (weighted) distance to target
     */
    virtual double CartToJnt(const JntArray& q_init, const Frames& p_in,JntArray& q_out)=0;
    
    virtual ~TreeIkSolverPos() {
    }
    ;
};

/**
 * \brief This <strong>abstract</strong> class encapsulates the inverse
 * velocity solver for a KDL::Tree.
 *
 * @ingroup KinematicFamily
 */
class TreeIkSolverVel {
public:
    /**
     * Calculate inverse velocity kinematics, from joint positions
     *and cartesian velocities to joint velocities.
     *
     * @param q_in input joint positions
     * @param v_in input cartesian velocity
     * @param qdot_out output joint velocities
     *
     * @return if < 0 something went wrong
	  *         distance to goal otherwise (weighted norm of v_in)
     */
    virtual double CartToJnt(const JntArray& q_in, const Twists& v_in, JntArray& qdot_out)=0;

    virtual ~TreeIkSolverVel() {
    }
    ;

};

}