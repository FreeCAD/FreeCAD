// Copyright  (C)  2018  Craig Carignan <craigc at ssl dot umd dot edu>

// Version: 1.0
// Author: Craig Carignan  <craigc at ssl dot umd dot edu>
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

#ifndef KDL_CHAIN_FDSOLVER_RECURSIVE_NEWTON_EULER_HPP
#define KDL_CHAIN_FDSOLVER_RECURSIVE_NEWTON_EULER_HPP

#include "chainfdsolver.hpp"
#include "chainidsolver_recursive_newton_euler.hpp"
#include "chaindynparam.hpp"

namespace KDL{
    /**
     * \brief Recursive newton euler forward dynamics solver
     *
     * The algorithm implementation is based on the book "Rigid Body
     * Dynamics Algorithms" of Roy Featherstone, 2008
     * (ISBN:978-0-387-74314-1) See Chapter 6 for basic algorithm.
     *
     * It calculates the accelerations for the joints (qdotdot), given the
     * position and velocity of the joints (q,qdot,qdotdot), external forces
     * on the segments (expressed in the segments reference frame),
     * and the dynamical parameters of the segments.
     */
    class ChainFdSolver_RNE : public ChainFdSolver{
    public:
        /**
         * Constructor for the solver, it will allocate all the necessary memory
         * \param chain The kinematic chain to calculate the forward dynamics for, an internal copy will be made.
         * \param grav The gravity vector to use during the calculation.
         */
        ChainFdSolver_RNE(const Chain& chain, Vector grav);
        ~ChainFdSolver_RNE(){};

        /**
         * Function to calculate from Cartesian forces to joint torques.
         * Input parameters;
         * \param q The current joint positions
         * \param q_dot The current joint velocities
         * \param torques The current joint torques (applied by controller)
         * \param f_ext The external forces (no gravity) on the segments
         * Output parameters:
         * \param q_dotdot The resulting joint accelerations
         */
        int CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &torques, const Wrenches& f_ext, JntArray &q_dotdot);

        /// @copydoc KDL::SolverI::updateInternalDataStructures
        virtual void updateInternalDataStructures();

        /**
         * Function to integrate the joint accelerations resulting from the forward dynamics solver.
         * Input parameters;
         * \param nj The number of joints
         * \param t The current time
         * \param dt The integration period
         * \param q The current joint positions
         * \param q_dot The current joint velocities
         * \param torques The current joint torques (applied by controller)
         * \param f_ext The external forces (no gravity) on the segments
         * \param fdsolver The forward dynamics solver
         * Output parameters:
         * \param t The updated time
         * \param q The updated joint positions
         * \param q_dot The updated joint velocities
         * \param q_dotdot The current joint accelerations
         * \param dq The joint position increment
         * \param dq_dot The joint velocity increment
         * Temporary parameters:
         * \param qtemp Intermediate joint positions
         * \param qdtemp Intermediate joint velocities
         */
        void RK4Integrator(unsigned int& nj, const double& t, double& dt, KDL::JntArray& q, KDL::JntArray& q_dot,
                           KDL::JntArray& torques, KDL::Wrenches& f_ext, KDL::ChainFdSolver_RNE& fdsolver,
                           KDL::JntArray& q_dotdot, KDL::JntArray& dq, KDL::JntArray& dq_dot,
                           KDL::JntArray& q_temp, KDL::JntArray& q_dot_temp);

    private:
        const Chain& chain;
        ChainDynParam DynSolver;
        ChainIdSolver_RNE IdSolver;
        unsigned int nj;
        unsigned int ns;
        JntSpaceInertiaMatrix H;
        JntArray Tzeroacc;
        Eigen::MatrixXd H_eig;
        Eigen::VectorXd Tzeroacc_eig;
        Eigen::MatrixXd L_eig;
        Eigen::VectorXd D_eig;
        Eigen::VectorXd r_eig;
        Eigen::VectorXd acc_eig;
    };
}

#endif
