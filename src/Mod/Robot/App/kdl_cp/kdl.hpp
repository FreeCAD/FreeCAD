// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2011  Erwin Aertbelien  <Erwin dot Aertbelien at mech dot kuleuven dot be>

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

/**
 * \defgroup KDL Kinematics and Dynamics Library
 * \ingroup ROBOT
 *
 * This is the API reference of the
 * <a href="http://www.orocos.org/kdl">Kinematics and Dynamics
 * Library</a> (KDL), a sub-project of <a
 * href="http://www.orocos.org">Orocos</a>, but that can be used
 * independently of Orocos. KDL offers different kinds of
 * functionality, grouped in the following  Modules:
 * - \subpage geomprim
 * - \ref KinematicFamily : functionality to build kinematic chains and access their kinematic and dynamic properties, such as e.g. Forward and Inverse kinematics and dynamics.
 * - \ref Motion : functionality to specify motion trajectories of frames and kinematic chains, such as e.g. Trapezoidal Velocity profiles.
 *
 *
**/
/**
 * \page geomprim Geometric Primitives
 * \section Introduction 
 * Geometric primitives are represented by the following classes.
 * - KDL::Vector
 * - KDL::Rotation
 * - KDL::Frame
 * - KDL::Twist
 * - KDL::Wrench
 *
 * \par Twist and Wrench transformations
 * 3 different types of transformations do exist for the twists
 * and wrenches.
 *
 * \verbatim
 *      1) Frame * Twist or Frame * Wrench :
 *              this transforms both the velocity/force reference point
 *             and the basis to which the twist/wrench are expressed.
 *      2) Rotation * Twist or Rotation * Wrench :
 *              this transforms the basis to which the twist/wrench are
 *              expressed, but leaves the reference point intact.
 *      3) Twist.RefPoint(v_base_AB) or Wrench.RefPoint(v_base_AB)
 *              this transforms only the reference point. v is expressed
 *              in the same base as the twist/wrench and points from the
 *              old reference point to the new reference point.
 * \endverbatim
 *
 *\warning
 *       Efficienty can be improved by writing p2 = A*(B*(C*p1))) instead of
 *          p2=A*B*C*p1
 * 
 * \par PROPOSED NAMING CONVENTION FOR FRAME-like OBJECTS
 *
 * \verbatim
 *      A naming convention of objects of the type defined in this file :
 *          (1) Frame : F...
 *              Rotation : R ...
 *          (2) Twist    : T ...
 *              Wrench   : W ...
 *              Vector   : V ...
 *      This prefix is followed by :
 *      for category (1) :
 *          F_A_B : w.r.t. frame A, frame B expressed
 *          ( each column of F_A_B corresponds to an axis of B,
 *            expressed w.r.t. frame A )
 *          in mathematical convention :
 *                   A
 *         F_A_B ==    F
 *                   B
 *
 *      for category (2) :
 *          V_B   : a vector expressed w.r.t. frame B
 *
 *      This can also be prepended by a name :
 *          e.g. : temporaryV_B
 *
 *      With this convention one can write :
 *
 *      F_A_B = F_B_A.Inverse();
 *      F_A_C = F_A_B * F_B_C;
 *      V_B   = F_B_C * V_C;    // both translation and rotation
 *      V_B   = R_B_C * V_C;    // only rotation
 * \endverbatim
 *
 * \par CONVENTIONS FOR WHEN USED WITH ROBOTS :
 *
 * \verbatim
 *       world : represents the frame ([1 0 0,0 1 0,0 0 1],[0 0 0]')
 *       mp    : represents mounting plate of a robot
 *               (i.e. everything before MP is constructed by robot manufacturer
 *                    everything after MP is tool )
 *       tf    : represents task frame of a robot
 *               (i.e. frame in which motion and force control is expressed)
 *       sf    : represents sensor frame of a robot
 *               (i.e. frame at which the forces measured by the force sensor
 *               are expressed )
 *
 *          Frame F_world_mp=...;
 *          Frame F_mp_sf(..)
 *          Frame F_mp_tf(,.)
 *
 *          Wrench are measured in sensor frame SF, so one could write :
 *                Wrench_tf = F_mp_tf.Inverse()* ( F_mp_sf * Wrench_sf );
 * \endverbatim
 *
 * \par CONVENTIONS REGARDING UNITS :
 *      Typically we use the standard S.I. units:  N, m, sec. 
 *
 */
 
/* This code doesn't seems to be integrated with freecad - \ref KDLTK : the interface code to integrate KDL into the Orocos <a href="http://www.orocos.org/rtt/">Real-Time Toolkit</a> (RTT). */


