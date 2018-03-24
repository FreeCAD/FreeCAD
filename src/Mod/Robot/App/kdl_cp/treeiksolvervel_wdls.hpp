/*
 * TreeIkSolverVel_wdls.hpp
 *
 *  Created on: Nov 28, 2008
 *      Author: rubensmits
 */

#ifndef TREEIKSOLVERVEL_WDLS_HPP_
#define TREEIKSOLVERVEL_WDLS_HPP_

#include "treeiksolver.hpp"
#include "treejnttojacsolver.hpp"
#include <Eigen/Core>

namespace KDL {

    using namespace Eigen;

    class TreeIkSolverVel_wdls: public TreeIkSolverVel {
    public:
        static const int E_SVD_FAILED = -100; //! Child SVD failed

        TreeIkSolverVel_wdls(const Tree& tree, const std::vector<std::string>& endpoints);
        virtual ~TreeIkSolverVel_wdls();
        
        virtual double CartToJnt(const JntArray& q_in, const Twists& v_in, JntArray& qdot_out);

        /*
         * Set the joint space weighting matrix
         *
         * @param weight_js joint space weighting symmetric matrix,
         * default : identity.  M_q : This matrix being used as a
         * weight for the norm of the joint space speed it HAS TO BE
         * symmetric and positive definite. We can actually deal with
         * matrices containing a symmetric and positive definite block
         * and 0s otherwise. Taking a diagonal matrix as an example, a
         * 0 on the diagonal means that the corresponding joints will
         * not contribute to the motion of the system. On the other
         * hand, the bigger the value, the most the corresponding
         * joint will contribute to the overall motion. The obtained
         * solution q_dot will actually minimize the weighted norm
         * sqrt(q_dot'*(M_q^-2)*q_dot). In the special case we deal
         * with, it does not make sense to invert M_q but what is
         * important is the physical meaning of all this : a joint
         * that has a zero weight in M_q will not contribute to the
         * motion of the system and this is equivalent to saying that
         * it gets an infinite weight in the norm computation.  For
         * more detailed explanation : vincent.padois@upmc.fr
         */
        void setWeightJS(const MatrixXd& Mq);
        const MatrixXd& getWeightJS() const {return Wq;}
        
        /*
         * Set the task space weighting matrix
         *
         * @param weight_ts task space weighting symmetric matrix,
         * default: identity M_x : This matrix being used as a weight
         * for the norm of the error (in terms of task space speed) it
         * HAS TO BE symmetric and positive definite. We can actually
         * deal with matrices containing a symmetric and positive
         * definite block and 0s otherwise. Taking a diagonal matrix
         * as an example, a 0 on the diagonal means that the
         * corresponding task coordinate will not be taken into
         * account (ie the corresponding error can be really big). If
         * the rank of the jacobian is equal to the number of task
         * space coordinates which do not have a 0 weight in M_x, the
         * weighting will actually not impact the results (ie there is
         * an exact solution to the velocity inverse kinematics
         * problem). In cases without an exact solution, the bigger
         * the value, the most the corresponding task coordinate will
         * be taken into account (ie the more the corresponding error
         * will be reduced). The obtained solution will minimize the
         * weighted norm sqrt(|x_dot-Jq_dot|'*(M_x^2)*|x_dot-Jq_dot|).
         * For more detailed explanation : vincent.padois@upmc.fr
         */
        void setWeightTS(const MatrixXd& Mx);
        const MatrixXd& getWeightTS() const {return Wy;}

        void setLambda(const double& lambda);
        double getLambda () const {return lambda;}

    private:
        Tree tree;
        TreeJntToJacSolver jnttojacsolver;
        Jacobians jacobians;
        
        MatrixXd J, Wy, Wq, J_Wq, Wy_J_Wq, U, V, Wy_U, Wq_V;
        VectorXd t, Wy_t, qdot, tmp, S;
        double lambda;
    };
    
}

#endif /* TREEIKSOLVERVEL_WDLS_HPP_ */
