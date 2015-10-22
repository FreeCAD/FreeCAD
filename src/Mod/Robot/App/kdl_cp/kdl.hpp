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

/**
 * \defgroup KDL Kinematics and Dynamics Library
 * \ingroup ROBOT
 *
 * This is the API reference of the
 * <a href="http://www.orocos.org/kdl">Kinematics and Dynamics Library</a> (KDL), a sub-project of <a href="http://www.orocos.org">Orocos</a>, but that can be used independently of Orocos. KDL offers different kinds of functionality:
 *
 * Geometric primitives and their transformations:
 * - KDL::Vector
 * - KDL::Rotation
 * - KDL::Frame
 * - KDL::Twist
 * - KDL::Wrench
 *
 * Other modules:
 * - \ref KinFam : functionality to build kinematic chains and access their kinematic and dynamic properties, such as e.g. Forward and Inverse kinematics and dynamics.
 * - \ref Motion : functionality to specify motion trajectories of frames and kinematic chains, such as e.g. Trapezoidal Velocity profiles.
 *
 */

/* This code doesn't seems to be integrated with freecad
- \ref KDLTK : the interface code to integrate KDL into the Orocos <a href="http://www.orocos.org/rtt/">Real-Time Toolkit</a> (RTT). */

