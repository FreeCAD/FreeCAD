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

#include "chainexternalwrenchestimator.hpp"

namespace KDL {

ChainExternalWrenchEstimator::ChainExternalWrenchEstimator(const Chain &chain, const Vector &gravity, const double sample_frequency, const double estimation_gain, const double filter_constant, const double eps, const int maxiter) :
    CHAIN(chain),
    DT_SEC(1.0 / sample_frequency), FILTER_CONST(filter_constant),
    svd_eps(eps),
    svd_maxiter(maxiter),
    nj(CHAIN.getNrOfJoints()), ns(CHAIN.getNrOfSegments()),
    jnt_mass_matrix(nj), previous_jnt_mass_matrix(nj), jnt_mass_matrix_dot(nj),
    initial_jnt_momentum(nj), estimated_momentum_integral(nj), filtered_estimated_ext_torque(nj),
    gravity_torque(nj), coriolis_torque(nj), total_torque(nj), estimated_ext_torque(nj),
    jacobian_end_eff(nj),
    jacobian_end_eff_transpose(Eigen::MatrixXd::Zero(nj, 6)), jacobian_end_eff_transpose_inv(Eigen::MatrixXd::Zero(6, nj)),
    U(Eigen::MatrixXd::Zero(nj, 6)), V(Eigen::MatrixXd::Zero(6, 6)),
    S(Eigen::VectorXd::Zero(6)), S_inv(Eigen::VectorXd::Zero(6)), tmp(Eigen::VectorXd::Zero(6)),
    ESTIMATION_GAIN(Eigen::VectorXd::Constant(nj, estimation_gain)),
    dynparam_solver(CHAIN, gravity),
    jacobian_solver(CHAIN),
    fk_pos_solver(CHAIN)
{
}

void ChainExternalWrenchEstimator::updateInternalDataStructures()
{
    nj = CHAIN.getNrOfJoints();
    ns = CHAIN.getNrOfSegments();
    jnt_mass_matrix.resize(nj);
    previous_jnt_mass_matrix.resize(nj);
    // The chain has changed, so the previously saved mass matrix is no longer valid.
    // Setting it to zero makes JntToExtWrench() re-initialize it on the next call.
    previous_jnt_mass_matrix.data.setZero();
    jnt_mass_matrix_dot.resize(nj);
    initial_jnt_momentum.resize(nj);
    estimated_momentum_integral.resize(nj);
    filtered_estimated_ext_torque.resize(nj);
    gravity_torque.resize(nj);
    coriolis_torque.resize(nj);
    total_torque.resize(nj);
    estimated_ext_torque.resize(nj);
    jacobian_end_eff.resize(nj);
    jacobian_end_eff_transpose.conservativeResizeLike(Eigen::MatrixXd::Zero(nj, 6));
    jacobian_end_eff_transpose_inv.conservativeResizeLike(Eigen::MatrixXd::Zero(6, nj));
    U.conservativeResizeLike(Eigen::MatrixXd::Zero(nj, 6));
    V.conservativeResizeLike(Eigen::MatrixXd::Zero(6, 6));
    S.conservativeResizeLike(Eigen::VectorXd::Zero(6));
    S_inv.conservativeResizeLike(Eigen::VectorXd::Zero(6));
    tmp.conservativeResizeLike(Eigen::VectorXd::Zero(6));
    ESTIMATION_GAIN.conservativeResizeLike(Eigen::VectorXd::Constant(nj, ESTIMATION_GAIN(0)));
    dynparam_solver.updateInternalDataStructures();
    jacobian_solver.updateInternalDataStructures();
    fk_pos_solver.updateInternalDataStructures();
}

// Calculates robot's initial momentum in the joint space. If this method is not called by the user, zero values will be taken for the initial momentum.
int ChainExternalWrenchEstimator::setInitialMomentum(const JntArray &joint_position, const JntArray &joint_velocity)
{
    // Check sizes first
    if (joint_position.rows() != nj || joint_velocity.rows() != nj)
        return (error = E_SIZE_MISMATCH);

    // Calculate robot's inertia and momentum in the joint space
    if (E_NOERROR != dynparam_solver.JntToMass(joint_position, jnt_mass_matrix))
        return (error = E_DYNPARAMSOLVERMASS_FAILED);

    initial_jnt_momentum.data = jnt_mass_matrix.data * joint_velocity.data;

    // Reset data because of the new momentum offset
    SetToZero(estimated_momentum_integral);
    SetToZero(filtered_estimated_ext_torque);

    // Initialize the mass matrix used by the finite-difference computation of its derivative,
    // to avoid a spike in the first estimation step
    previous_jnt_mass_matrix = jnt_mass_matrix;

    return (error = E_NOERROR);
}

// Sets singular-value eps parameter for the SVD calculation
void ChainExternalWrenchEstimator::setSVDEps(const double eps_in)
{
    svd_eps = eps_in;
}

// Sets maximum iteration parameter for the SVD calculation
void ChainExternalWrenchEstimator::setSVDMaxIter(const int maxiter_in)
{
    svd_maxiter = maxiter_in;
}

// This method calculates the external wrench that is applied on the robot's end-effector.
int ChainExternalWrenchEstimator::JntToExtWrench(const JntArray &joint_position, const JntArray &joint_velocity, const JntArray &joint_torque, Wrench &external_wrench)
{
    /**
     * ==========================================================================
     * First-order momentum observer, an implementation based on:
     * S. Haddadin, A. De Luca and A. Albu-Schäffer,
     * "Robot Collisions: A Survey on Detection, Isolation, and Identification,"
     * in IEEE Transactions on Robotics, vol. 33(6), pp. 1292-1312, 2017.
     * ==========================================================================
     */

    // Check sizes first
    if (nj != CHAIN.getNrOfJoints() || ns != CHAIN.getNrOfSegments())
        return (error = E_NOT_UP_TO_DATE);
    if (joint_position.rows() != nj || joint_velocity.rows() != nj || joint_torque.rows() != nj)
        return (error = E_SIZE_MISMATCH);

    /**
     * ================================================================================================================
     * Part I: Estimation of the torques felt in joints as a result of the external wrench being applied on the robot
     * ================================================================================================================
     */

    // Calculate decomposed robot's dynamics
    if (E_NOERROR != dynparam_solver.JntToMass(joint_position, jnt_mass_matrix))
        return (error = E_DYNPARAMSOLVERMASS_FAILED);

    // A zero matrix is not a valid inertia matrix, so it indicates that there is no
    // previously saved mass matrix yet (the user is not required to call setInitialMomentum()).
    // Use the current mass matrix instead, to avoid a spike in the mass matrix derivative below.
    if (previous_jnt_mass_matrix.data.isZero())
        previous_jnt_mass_matrix.data = jnt_mass_matrix.data;

    if (E_NOERROR != dynparam_solver.JntToCoriolis(joint_position, joint_velocity, coriolis_torque))
        return (error = E_DYNPARAMSOLVERCORIOLIS_FAILED);

    if (E_NOERROR != dynparam_solver.JntToGravity(joint_position, gravity_torque))
        return (error = E_DYNPARAMSOLVERGRAVITY_FAILED);

    // Calculate the change of robot's inertia in the joint space
    jnt_mass_matrix_dot.data = (jnt_mass_matrix.data - previous_jnt_mass_matrix.data) / DT_SEC;

    // Save data for the next iteration
    previous_jnt_mass_matrix.data = jnt_mass_matrix.data;

    // Calculate total torque exerted on the joint
    total_torque.data = joint_torque.data - gravity_torque.data - coriolis_torque.data + jnt_mass_matrix_dot.data * joint_velocity.data;

    // Accumulate main integral
    estimated_momentum_integral.data += (total_torque.data + filtered_estimated_ext_torque.data) * DT_SEC;

    // Estimate external joint torque
    estimated_ext_torque.data = ESTIMATION_GAIN.asDiagonal() * (jnt_mass_matrix.data * joint_velocity.data - estimated_momentum_integral.data - initial_jnt_momentum.data);

    // First order low-pass filter: filter out the noise from the estimated signal
    // This filter can be turned off by setting FILTER_CONST value to 0
    filtered_estimated_ext_torque.data = FILTER_CONST * filtered_estimated_ext_torque.data + (1.0 - FILTER_CONST) * estimated_ext_torque.data;

    /**
     * ==================================================================================================================
     * Part II: Propagate above-estimated joint torques to Cartesian wrench using a pseudo-inverse of Jacobian-Transpose
     * ==================================================================================================================
     */
    
    // Compute robot's end-effector frame, expressed in the base frame
    Frame end_eff_frame;
    if (E_NOERROR != fk_pos_solver.JntToCart(joint_position, end_eff_frame))
        return (error = E_FKSOLVERPOS_FAILED);

    // Compute robot's jacobian for the end-effector frame, expressed in the base frame
    if (E_NOERROR != jacobian_solver.JntToJac(joint_position, jacobian_end_eff))
        return (error = E_JACSOLVER_FAILED);

    // Transform the jacobian from the base frame to the end-effector frame. 
    // This part can be commented out if the user wants estimated wrench to be expressed w.r.t. base frame 
    jacobian_end_eff.changeBase(end_eff_frame.M.Inverse()); // Jacobian is now expressed w.r.t. end-effector frame

    // SVD of "Jac^T" with maximum iterations "maxiter": Jac^T = U * S^-1 * V^T
    jacobian_end_eff_transpose = jacobian_end_eff.data.transpose();
    if (E_NOERROR != svd_eigen_HH(jacobian_end_eff_transpose, U, S, V, tmp, svd_maxiter))
        return (error = E_SVD_FAILED);

    // Invert singular values: S^-1
    for (int i = 0; i < S.size(); ++i)
        S_inv(i) = (std::fabs(S(i)) < svd_eps) ? 0.0 : 1.0 / S(i);

    // Compose the inverse: (Jac^T)^-1 = V * S^-1 * U^T
    jacobian_end_eff_transpose_inv = V * S_inv.asDiagonal() * U.adjoint();

    // Compute end-effector's Cartesian wrench from the estimated joint torques: (Jac^T)^-1 * ext_tau
    Vector6d estimated_wrench = jacobian_end_eff_transpose_inv * filtered_estimated_ext_torque.data;
    for (int i = 0; i < 6; i++) 
        external_wrench(i) = estimated_wrench(i);

    return (error = E_NOERROR);
}

// Getter for the torques felt in the robot's joints due to the external wrench being applied on the robot
void ChainExternalWrenchEstimator::getEstimatedJntTorque(JntArray &external_joint_torque)
{
    assert(external_joint_torque.rows() == filtered_estimated_ext_torque.rows());
    external_joint_torque = filtered_estimated_ext_torque;
}

const char* ChainExternalWrenchEstimator::strError(const int error) const
{
    if (E_FKSOLVERPOS_FAILED == error) return "Internally-used Forward Position Kinematics (Recursive) solver failed";
    else if (E_JACSOLVER_FAILED == error) return "Internally-used Jacobian solver failed";
    else if (E_DYNPARAMSOLVERMASS_FAILED == error) return "Internally-used Dynamics Parameters (Mass) solver failed";
    else if (E_DYNPARAMSOLVERCORIOLIS_FAILED == error) return "Internally-used Dynamics Parameters (Coriolis) solver failed";
    else if (E_DYNPARAMSOLVERGRAVITY_FAILED == error) return "Internally-used Dynamics Parameters (Gravity) solver failed";
    else return SolverI::strError(error);
}

} // namespace
