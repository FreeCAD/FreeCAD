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

#ifndef KDL_CHAIN_IKSOLVERVEL_PINV_HPP
#define KDL_CHAIN_IKSOLVERVEL_PINV_HPP

#include "chainiksolver.hpp"
#include "chainjnttojacsolver.hpp"
#include "utilities/svd_HH.hpp"

namespace KDL
{
    /**
     * Implementation of a inverse velocity kinematics algorithm based
     * on the generalize pseudo inverse to calculate the velocity
     * transformation from Cartesian to joint space of a general
     * KDL::Chain. It uses a svd-calculation based on householders
     * rotations.
     *
     * @ingroup KinematicFamily
     */
    class ChainIkSolverVel_pinv : public ChainIkSolverVel
    {
    public:
        static const int E_SVD_FAILED = -100; //! Child SVD failed
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
        explicit ChainIkSolverVel_pinv(const Chain& chain,double eps=0.00001,int maxiter=150);
        ~ChainIkSolverVel_pinv();

        /**
         * Find an output joint velocity \a qdot_out, given a starting joint pose
         * \a q_init and a desired cartesian velocity \a v_in
         *
         * @return
         *  E_NOERROR=solution converged to <eps in maxiter
         *  E_SVD_FAILED=SVD computation failed
         *  E_CONVERGE_PINV_SINGULAR=solution converged but (pseudo)inverse is singular
         *
         * @note if E_CONVERGE_PINV_SINGULAR returned then converged and can
         * continue motion, but have degraded solution
         *
         * @note If E_SVD_FAILED returned, then getSvdResult() returns the error code
         * from the SVD algorithm.
         */
        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out);
        /**
         * not (yet) implemented.
         *
         */
        virtual int CartToJnt(const JntArray& /*q_init*/, const FrameVel& /*v_in*/, JntArrayVel& /*q_out*/){return -1;};

        /**
         * Retrieve the number of singular values of the jacobian that are < eps;
         * if the number of near zero singular values is > jac.col()-jac.row(),
         * then the jacobian pseudoinverse is singular
         */
        unsigned int getNrZeroSigmas()const {return nrZeroSigmas;};

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
        SVD_HH svd;
        std::vector<JntArray> U;
        JntArray S;
        std::vector<JntArray> V;
        JntArray tmp;
        double eps;
        int maxiter;
        unsigned int nrZeroSigmas;
        int svdResult;

    };
}
#endif

