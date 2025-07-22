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
 * @defgroup KinematicFamily Kinematic Families
 * \ingroup KDL
 * @brief All classes to support kinematic families.
 *
 * The Kinematic Families classes range from the basic building blocks
 * (KDL::Joint and KDL::Segment) and their interconnected kinematic
 * structures (KDL::Chain, KDL::Tree and KDL::Graph), to the solver
 * algorithms for the kinematics and dynamics of particular kinematic
 * families.
 *
 * A <em>kinematic family</em> is a set of kinematic structures that have
 * similar properties, such as the same interconnection topology, the same
 * numerical or analytical solver algorithms, etc. Different members of the
 * same kinematic family differ only by the concrete values of their
 * kinematic and dynamic properties (link lengths, mass, etc.).
 *
 * Each kinematic structure is built from one or more Segments
 * (KDL::Segment). A KDL::Chain is a <strong>serial</strong> connection of
 * these segments; a KDL:Tree is a <strong>tree-structured</strong>
 * interconnection; and a KDL:Graph is a kinematic structure with a
 * <strong>general graph</strong> topology. (The current implementation
 * supports only KDL::Chain.)
 *
 * A KDL::Segment contains a KDL::Joint and an offset frame ("link length",
 * defined by a KDL::Frame), that represents the geometric pose
 * between the KDL::Joint on the previous segment and its own KDL::Joint.
 *
 * A list of all the classes is available on the modules page: \ref KinFam
 *
 *
 */
