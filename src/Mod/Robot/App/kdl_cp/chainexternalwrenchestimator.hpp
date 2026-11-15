// Copyright  (C)  2021 Djordje Vukcevic <djordje dot vukcevic at h-brs dot de>

// Version: 1.0
// Author: Djordje Vukcevic <djordje dot vukcevic at h-brs dot de>
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

#ifndef KDL_CHAIN_EXTERNAL_WRENCH_ESTIMATOR_HPP
#define KDL_CHAIN_EXTERNAL_WRENCH_ESTIMATOR_HPP

#include <Eigen/Core>
#include "utilities/svd_eigen_HH.hpp"
#include "chaindynparam.hpp"
#include "chainjnttojacsolver.hpp"
#include "chainfksolverpos_recursive.hpp"
#include <iostream>

namespace KDL {

    /**
     * \brief First-order momentum observer for the estimation of external wrenches applied on the robot's end-effector. 
     *
     * Implementation based on:
     * S. Haddadin, A. De Luca and A. Albu-Schäffer,
     * "Robot Collisions: A Survey on Detection, Isolation, and Identification,"
     * in IEEE Transactions on Robotics, vol. 33(6), pp. 1292-1312, 2017.
     * 
     * Note: This component assumes that the external wrench is applied on the end-effector (last) link of the robot's chain.
     */
    class ChainExternalWrenchEstimator : public SolverI
    {
        typedef Eigen::Matrix<double, 6, 1 > Vector6d;

    public:

        static const int E_FKSOLVERPOS_FAILED = -100; //! Internally-used Forward Position Kinematics (Recursive) solver failed
        static const int E_JACSOLVER_FAILED = -101; //! Internally-used Jacobian solver failed
        static const int E_DYNPARAMSOLVERMASS_FAILED = -102; //! Internally-used Dynamics Parameters (Mass) solver failed
        static const int E_DYNPARAMSOLVERCORIOLIS_FAILED = -103; //! Internally-used Dynamics Parameters (Coriolis) solver failed
        static const int E_DYNPARAMSOLVERGRAVITY_FAILED = -104; //! Internally-used Dynamics Parameters (Gravity) solver failed

        /**
         * Constructor for the estimator, it will allocate all the necessary memory
         * \param chain The kinematic chain of the robot, an internal copy will be made.
         * \param gravity The gravity-acceleration vector to use during the calculation.
         * \param sample_frequency Frequency at which users updates it estimation loop (in Hz).
         * \param estimation_gain Parameter used to control the estimator's convergence
         * \param filter_constant Parameter defining how much the estimated signal should be filtered by the low-pass filter.
         *                        This input value should be between 0 and 1. Higher the number means more noise needs to be filtered-out.
         *                        The filter can be turned off by setting this value to 0.
         * \param eps If a SVD-singular value is below this value, its inverse is set to zero. Default: 0.00001
         * \param maxiter Maximum iterations for the SVD computations. Default: 150.
         */
        ChainExternalWrenchEstimator(const Chain &chain, const Vector &gravity, const double sample_frequency, const double estimation_gain, const double filter_constant, const double eps = 0.00001, const int maxiter = 150);
        ~ChainExternalWrenchEstimator(){};

        /**
         * Calculates robot's initial momentum in the joint space. 
         * Basically, sets the offset for future estimation (momentum calculation).
         * If this method is not called by the user, zero values will be taken for the initial momentum.
         */
        int setInitialMomentum(const JntArray &joint_position, const JntArray &joint_velocity);

        // Sets singular-value eps parameter for the SVD calculation
        void setSVDEps(const double eps_in);

        // Sets maximum iteration parameter for the SVD calculation
        void setSVDMaxIter(const int maxiter_in);

        /**
         * This method calculates the external wrench that is applied on the robot's end-effector.
         * Input parameters:
         * \param joint_position The current (measured) joint positions.
         * \param joint_velocity The current (measured) joint velocities.
         * \param joint_torque The joint space torques. 
         *                     Depending on the user's choice, this array can represent commanded or measured joint torques.
         *                     A particular choice depends on the available sensors in robot's joint. 
         *                     For more details see the above-referenced article.
         *
         * Output parameters:
         * \param external_wrench The estimated external wrench applied on the robot's end-effector.
         *                        The wrench will be expressed w.r.t. end-effector's frame.
         *
         * @return error/success code
         */
        int JntToExtWrench(const JntArray &joint_position, const JntArray &joint_velocity, const JntArray &joint_torque, Wrench &external_wrench);

        // Returns the torques felt in the robot's joints as a result of the external wrench being applied on the robot.
        void getEstimatedJntTorque(JntArray &external_joint_torque);

        /// @copydoc KDL::SolverI::updateInternalDataStructures()
        virtual void updateInternalDataStructures();

        /// @copydoc KDL::SolverI::strError()
        virtual const char* strError(const int error) const;

    private:
        const Chain &CHAIN;
        const double DT_SEC, FILTER_CONST;
        double svd_eps;
        int svd_maxiter;
        unsigned int nj, ns;
        JntSpaceInertiaMatrix jnt_mass_matrix, previous_jnt_mass_matrix, jnt_mass_matrix_dot;
        JntArray initial_jnt_momentum, estimated_momentum_integral, filtered_estimated_ext_torque, 
                 gravity_torque, coriolis_torque, total_torque, estimated_ext_torque;
        Jacobian jacobian_end_eff;
        Eigen::MatrixXd jacobian_end_eff_transpose, jacobian_end_eff_transpose_inv, U, V;
        Eigen::VectorXd S, S_inv, tmp, ESTIMATION_GAIN;
        ChainDynParam dynparam_solver;
        ChainJntToJacSolver jacobian_solver;
        ChainFkSolverPos_recursive fk_pos_solver;
    };
}

#endif
