// Copyright  (C)  2009  Ruben Smits <ruben dot smits at intermodalics dot eu>

// Version: 1.0
// Author: Franco Fusco <franco dot fusco at ls2n dot fr>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot eu>
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

#ifndef KDL_TREE_IDSOLVER_RECURSIVE_NEWTON_EULER_HPP
#define KDL_TREE_IDSOLVER_RECURSIVE_NEWTON_EULER_HPP

#include "treeidsolver.hpp"

namespace KDL{
    /**
     * \brief Recursive newton euler inverse dynamics solver for kinematic trees.
     *
     * It calculates the torques for the joints, given the motion of
     * the joints (q,qdot,qdotdot), external forces on the segments
     * (expressed in the segments reference frame) and the dynamical
     * parameters of the segments.
     *
     * This is an extension of the inverse dynamic solver for kinematic chains,
     * \see ChainIdSolver_RNE. The main difference is the use of STL maps
     * instead of vectors to represent external wrenches (as well as internal
     * variables exploited during the recursion).
     */
    class TreeIdSolver_RNE : public TreeIdSolver {
    public:
        /**
         * Constructor for the solver, it will allocate all the necessary memory
         * \param tree The kinematic tree to calculate the inverse dynamics for, an internal reference will be stored.
         * \param grav The gravity vector to use during the calculation.
         */
        TreeIdSolver_RNE(const Tree& tree, Vector grav);

        /**
         * Function to calculate from Cartesian forces to joint torques.
         * Input parameters;
         * \param q The current joint positions
         * \param q_dot The current joint velocities
         * \param q_dotdot The current joint accelerations
         * \param f_ext The external forces (no gravity) on the segments
         * Output parameters:
         * \param torques the resulting torques for the joints
         */
        int CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &q_dotdot, const WrenchMap& f_ext, JntArray &torques);

        /// @copydoc KDL::SolverI::updateInternalDataStructures
        virtual void updateInternalDataStructures();

    private:
        ///Helper function to initialize private members X, S, v, a, f
        void initAuxVariables();

        ///One recursion step
        void rne_step(SegmentMap::const_iterator segment, const JntArray &q, const JntArray &q_dot, const JntArray &q_dotdot, const WrenchMap& f_ext, JntArray& torques);

        const Tree& tree;
        unsigned int nj;
        unsigned int ns;
        std::map<std::string,Frame> X;
        std::map<std::string,Twist> S;
        std::map<std::string,Twist> v;
        std::map<std::string,Twist> a;
        std::map<std::string,Wrench> f;
        Twist ag;
    };
}

#endif
