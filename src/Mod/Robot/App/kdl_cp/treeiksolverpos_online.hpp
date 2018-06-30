// Copyright  (C)  2011  PAL Robotics S.L.  All rights reserved.
// Copyright  (C)  2007-2008  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008  Mikael Mayer
// Copyright  (C)  2008  Julia Jesse

// Version: 1.0
// Author: Marcus Liebhardt
// This class has been derived from the KDL::TreeIkSolverPos_NR_JL class
// by Julia Jesse, Mikael Mayer and Ruben Smits

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

#ifndef KDLTREEIKSOLVERPOS_ONLINE_HPP
#define KDLTREEIKSOLVERPOS_ONLINE_HPP

#include <vector>
#include <string>
#include "treeiksolver.hpp"
#include "treefksolver.hpp"

namespace KDL {

/**
 * Implementation of a general inverse position kinematics algorithm to calculate the position transformation from
 * Cartesian to joint space of a general KDL::Tree. This class has been derived from the TreeIkSolverPos_NR_JL class,
 * but was modified for online solving for use in realtime systems. Thus, the calculation is only done once,
 * meaning that no iteration is done, because this solver is intended to run at a high frequency.
 * It enforces velocity limits in task as well as in joint space. It also takes joint limits into account.
 *
 * @ingroup KinematicFamily
 */
class TreeIkSolverPos_Online: public TreeIkSolverPos {
public:
    /**
     * Constructor of the solver, it needs the number of joints of the tree, a list of the endpoints
     * you are interested in, the maximum and minimum values you want to enforce and a forward position kinematics
     * solver as well as an inverse velocity kinematics solver for the calculations
     *
     * @param nr_of_jnts number of joints of the tree to calculate the joint positions for
     * @param endpoints the list of endpoints you are interested in
     * @param q_min the minimum joint positions
     * @param q_max the maximum joint positions
     * @param q_dot_max the maximum joint velocities
     * @param x_dot_trans_max the maximum translational velocity of your endpoints
     * @param x_dot_rot_max the maximum rotational velocity of your endpoints
     * @param fksolver a forward position kinematics solver
     * @param iksolver an inverse velocity kinematics solver
     *
     * @return
     */

    TreeIkSolverPos_Online(const double& nr_of_jnts,
                           const std::vector<std::string>& endpoints,
                           const JntArray& q_min,
                           const JntArray& q_max,
                           const JntArray& q_dot_max,
                           const double x_dot_trans_max,
                           const double x_dot_rot_max,
                           TreeFkSolverPos& fksolver,
                           TreeIkSolverVel& iksolver);

    ~TreeIkSolverPos_Online();

    virtual double CartToJnt(const JntArray& q_in, const Frames& p_in, JntArray& q_out);

private:
  /**
   * Scales the class member KDL::JntArray q_dot_, if one (or more) joint velocity exceeds the maximum value.
   * Scaling is done proportional to the biggest overshoot among all joint velocities.
   */
  void enforceJointVelLimits();

  /**
   * Scales translational and rotational velocity vectors of the class member KDL::Twist twist_,
   * if at least one of both exceeds the maximum value/length.
   * Scaling is done proportional to the biggest overshoot among both velocities.
   */
  void enforceCartVelLimits();

  JntArray q_min_;
  JntArray q_max_;
  JntArray q_dot_max_;
  double x_dot_trans_max_;
  double x_dot_rot_max_;
  TreeFkSolverPos& fksolver_;
  TreeIkSolverVel& iksolver_;
  JntArray q_dot_;
  Twist twist_;
  Frames frames_;
  Twists delta_twists_;
};

} // namespace

#endif /* KDLTREEIKSOLVERPOS_ONLINE_HPP */

