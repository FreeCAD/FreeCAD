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

#ifndef KDL_CHAIN_IKSOLVERVEL_WDLS_HPP
#define KDL_CHAIN_IKSOLVERVEL_WDLS_HPP

#include "chainiksolver.hpp"
#include "chainjnttojacsolver.hpp"
#include <Eigen/Core>

namespace KDL
{
    /**
     * Implementation of a inverse velocity kinematics algorithm based
     * on the weighted pseudo inverse with damped least-square to calculate the velocity
     * transformation from Cartesian to joint space of a general
     * KDL::Chain. It uses a svd-calculation based on householders
     * rotations.
     *
     * J# = M_q*Vb*pinv_dls(Db)*Ub'*M_x
     *
     * where B = Mx*J*Mq
     *
     * and B = Ub*Db*Vb' is the SVD decomposition of B
     *
     * Mq and Mx represent, respectively, the joint-space and task-space weighting
     * matrices.
     * Please refer to the documentation of setWeightJS(const Eigen::MatrixXd& Mq)
     * and setWeightTS(const Eigen::MatrixXd& Mx) for details on the effects of
     * these matrices.
     *
     * For more details on Weighted Pseudo Inverse, see :
     * 1) [Ben Israel 03] A. Ben Israel & T.N.E. Greville.
     * Generalized Inverses : Theory and Applications,
     * second edition. Springer, 2003. ISBN 0-387-00293-6.
     *
     * 2) [Doty 93] K. L. Doty, C. Melchiorri & C. Boniveto.
     * A theory of generalized inverses applied to Robotics.
     * The International Journal of Robotics Research,
     * vol. 12, no. 1, pages 1-19, february 1993.
     *
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverVel_wdls : public ChainIkSolverVel
    {
    public:
        static const int E_SVD_FAILED = -100; //! SVD solver failed
        /// solution converged but (pseudo)inverse is singular
        static const int E_CONVERGE_PINV_SINGULAR = +100;

        /**
         * Constructor of the solver
         *
         * @param chain the chain to calculate the inverse velocity
         * kinematics for
         * @param eps if a singular value is below this value, its
         * inverse is set to zero, default: 0.00001
         * @param maxiter maximum iterations for the svd calculation,
         * default: 150
         *
         */

        explicit ChainIkSolverVel_wdls(const Chain& chain,double eps=0.00001,int maxiter=150);
        //=ublas::identity_matrix<double>
        ~ChainIkSolverVel_wdls();

        /**
         * Find an output joint velocity \a qdot_out, given a starting joint pose
         * \a q_init and a desired cartesian velocity \a v_in
         *
         * @return
         *  E_NOERROR=svd solution converged in maxiter
         *  E_SVD_FAILED=svd solution failed
         *  E_CONVERGE_PINV_SINGULAR=svd solution converged but (pseudo)inverse singular
         *
         * @note if E_CONVERGE_PINV_SINGULAR returned then converged and can
         * continue motion, but have degraded solution
         *
         * @note If E_SVD_FAILED returned, then getSvdResult() returns the error
         * code from the SVD algorithm.
		 */
        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out);
        /**
         * not (yet) implemented.
         *
         */
        virtual int CartToJnt(const JntArray& q_init, const FrameVel& v_in, JntArrayVel& q_out){return -1;};

        /**
         * Set the joint space weighting matrix
         *
         * @param weight_js joint space weighting symetric matrix,
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
        void setWeightJS(const Eigen::MatrixXd& Mq);

        /**
         * Set the task space weighting matrix
         *
         * @param weight_ts task space weighting symetric matrix,
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
        void setWeightTS(const Eigen::MatrixXd& Mx);

        /**
         * Set lambda
         */
        void setLambda(const double lambda);
        /**
         * Set eps
         */
        void setEps(const double eps_in);
        /**
         * Set maxIter
         */
        void setMaxIter(const int maxiter_in);

        /**
         * Request the number of singular values of the jacobian that are < eps;
         * if the number of near zero singular values is > jac.col()-jac.row(),
         * then the jacobian pseudoinverse is singular
         */
        unsigned int getNrZeroSigmas()const {return nrZeroSigmas;};

        /**
         * Request the minimum of the first six singular values
         */
        double getSigmaMin()const {return sigmaMin;};

        /**
         * Request the value of lambda for the minimum
         */
        double getLambda()const {return lambda;};

        /**
         * Request the scaled value of lambda for the minimum
         * singular value 1-6
         */
        double getLambdaScaled()const {return lambda_scaled;};

        /**
         * Retrieve the latest return code from the SVD algorithm
         * @return 0 if CartToJnt() not yet called, otherwise latest SVD result code.
         */
        int getSVDResult()const {return svdResult;};

        /// @copydoc KDL::SolverI::strError()
        virtual const char* strError(const int error) const;

    private:
        const Chain chain;
        ChainJntToJacSolver jnt2jac;
        Jacobian jac;
        Eigen::MatrixXd U;
        Eigen::VectorXd S;
        Eigen::MatrixXd V;
        double eps;
        int maxiter;
        Eigen::VectorXd tmp;
        Eigen::MatrixXd tmp_jac;
        Eigen::MatrixXd tmp_jac_weight1;
        Eigen::MatrixXd tmp_jac_weight2;
        Eigen::MatrixXd tmp_ts;
        Eigen::MatrixXd tmp_js;
        Eigen::MatrixXd weight_ts;
        Eigen::MatrixXd weight_js;
        double lambda;
		double lambda_scaled;
		unsigned int nrZeroSigmas ;
		int svdResult;
		double sigmaMin;
    };
}
#endif

