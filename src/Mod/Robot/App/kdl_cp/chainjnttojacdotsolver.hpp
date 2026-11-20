/*
    Computes the Jacobian time derivative
    Copyright (C) 2015  Antoine Hoarau <hoarau [at] isir.upmc.fr>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef KDL_CHAINJNTTOJACDOTSOLVER_HPP
#define KDL_CHAINJNTTOJACDOTSOLVER_HPP

#include "solveri.hpp"
#include "frames.hpp"
#include "jntarrayvel.hpp"
#include "jacobian.hpp"
#include "chain.hpp"
#include "framevel.hpp"
#include "chainjnttojacsolver.hpp"
#include "chainfksolverpos_recursive.hpp"

namespace KDL
{

/**
 * @brief Computes the Jacobian time derivative (Jdot) by calculating the
 * partial derivatives regarding to a joint angle, in the Hybrid, Body-fixed
 * or Inertial representation.
 *
 * This work is based on :
 * Symbolic differentiation of the velocity mapping for a serial kinematic chain
 * H. Bruyninckx, J. De Schutter
 * doi:10.1016/0094-114X(95)00069-B
 *
 * url : http://www.sciencedirect.com/science/article/pii/0094114X9500069B
 */
class ChainJntToJacDotSolver : public SolverI
{
public:
    static const int E_JAC_DOT_FAILED = -100;
    static const int E_JACSOLVER_FAILED = -101;
    static const int E_FKSOLVERPOS_FAILED = -102;

    // Hybrid representation ref Frame: base, ref Point: end-effector
    static const int HYBRID = 0;
    // Body-fixed representation ref Frame: end-effector, ref Point: end-effector
    static const int BODYFIXED = 1;
    // Inertial representation ref Frame: base, ref Point: base
    static const int INERTIAL = 2;

    explicit ChainJntToJacDotSolver(const Chain& chain);
    virtual ~ChainJntToJacDotSolver();
    /**
     * @brief Computes \f$ {}_{bs}\dot{J}^{ee}.\dot{q} \f$
     *
     * @param q_in Current joint positions and velocities
     * @param jac_dot_q_dot The twist representing Jdot*qdot
     * @param seg_nr The final segment to compute
     * @return int 0 if no errors happened
     */
    virtual int JntToJacDot(const KDL::JntArrayVel& q_in, KDL::Twist& jac_dot_q_dot, int seg_nr = -1);
    /**
     * @brief Computes \f$ {}_{bs}\dot{J}^{ee} \f$
     *
     * @param q_in Current joint positions and velocities
     * @param jdot The jacobian time derivative in the configured representation
     *        (HYBRID, BODYFIXED or INERTIAL)
     * @param seg_nr The final segment to compute
     * @return int 0 if no errors happened
     */
    virtual int JntToJacDot(const KDL::JntArrayVel& q_in, KDL::Jacobian& jdot, int seg_nr = -1);
    int setLockedJoints(const std::vector<bool>& locked_joints);

    /**
     * @brief JntToJacDot() will compute in the Hybrid representation (ref Frame: base, ref Point: end-effector)
     *
     *
     * @return void
     */
    void setHybridRepresentation(){setRepresentation(HYBRID);}
    /**
     * @brief JntToJacDot() will compute in the Body-fixed representation (ref Frame: end-effector, ref Point: end-effector)
     *
     * @return void
     */
    void setBodyFixedRepresentation(){setRepresentation(BODYFIXED);}
    /**
     * @brief JntToJacDot() will compute in the Inertial representation (ref Frame: base, ref Point: base)
     *
     * @return void
     */
    void setInertialRepresentation(){setRepresentation(INERTIAL);}
    /**
     * @brief Sets the internal variable for the representation (with a check on the value)
     *
     * @param representation The representation for Jdot : HYBRID,BODYFIXED or INERTIAL
     * @return void
     */
    void setRepresentation(const int& representation);

    /// @copydoc KDL::SolverI::updateInternalDataStructures
    virtual void updateInternalDataStructures();

    /// @copydoc KDL::SolverI::strError()
    virtual const char* strError(const int error) const;


protected:
     /**
     * @brief Computes \f$ \frac{\partial {}_{bs}J^{i,ee}}{\partial q^{j}}.\dot{q}^{j} \f$
     *
     * @param bs_J_ee The Jacobian expressed in the base frame with the end effector as the reference point (default in KDL Jacobian Solver)
     * @param joint_idx The index of the current joint (j in the formula)
     * @param column_idx The index of the current column (i in the formula)
     * @return Twist The twist representing dJi/dqj .qdotj
     */
    const Twist& getPartialDerivativeHybrid(const Jacobian& bs_J_ee,
                                     const unsigned int& joint_idx,
                                     const unsigned int& column_idx);
     /**
     * @brief Computes \f$ \frac{\partial {}_{ee}J^{i,ee}}{\partial q^{j}}.\dot{q}^{j} \f$
     *
     * @param bs_J_ee The Jacobian expressed in the end effector frame with the end effector as the reference point
     * @param joint_idx The indice of the current joint (j in the formula)
     * @param column_idx The indice of the current column (i in the formula)
     * @return Twist The twist representing dJi/dqj .qdotj
     */
    const Twist& getPartialDerivativeBodyFixed(const Jacobian& ee_J_ee,
                                        const unsigned int& joint_idx,
                                        const unsigned int& column_idx);
     /**
     * @brief Computes \f$ \frac{\partial {}_{bs}J^{i,bs}}{\partial q^{j}}.\dot{q}^{j} \f$
     *
     * @param ee_J_ee The Jacobian expressed in the base frame with the base as the reference point
     * @param joint_idx The indice of the current joint (j in the formula)
     * @param column_idx The indice of the current column (i in the formula)
     * @return Twist The twist representing dJi/dqj .qdotj
     */
    const Twist& getPartialDerivativeInertial(const Jacobian& bs_J_bs,
                                       const unsigned int& joint_idx,
                                       const unsigned int& column_idx);
     /**
     * @brief Computes \f$ \frac{\partial J^{i,ee}}{\partial q^{j}}.\dot{q}^{j} \f$
     *
     * @param bs_J_bs The Jacobian expressed in the base frame with the end effector as the reference point
     * @param joint_idx The indice of the current joint (j in the formula)
     * @param column_idx The indice of the current column (i in the formula)
     * @param representation The representation (Hybrid,Body-fixed,Inertial) in which you want to get dJ/dqj .qdotj
     * @return Twist The twist representing dJi/dqj .qdotj
     */
    const Twist& getPartialDerivative(const Jacobian& J,
                               const unsigned int& joint_idx,
                               const unsigned int& column_idx,
                               const int& representation);
private:

    const Chain& chain;
    std::vector<bool> locked_joints_;
    unsigned int nr_of_unlocked_joints_;
    ChainJntToJacSolver jac_solver_;
    Jacobian jac_;
    Jacobian jac_dot_;
    int representation_;
    ChainFkSolverPos_recursive fk_solver_;
    Frame F_bs_ee_;
    Twist jac_dot_k_;
    Twist jac_j_, jac_i_;
    Twist t_djdq_;
};

}
#endif
