// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

#ifndef KDL_CHAIN_IKSOLVERVEL_PINV_GIVENS_HPP
#define KDL_CHAIN_IKSOLVERVEL_PINV_GIVENS_HPP

#include "chainiksolver.hpp"
#include "chainjnttojacsolver.hpp"

#include <Eigen/Core>

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
    class ChainIkSolverVel_pinv_givens : public ChainIkSolverVel
    {
    public:

        /**
         * Constructor of the solver
         *
         * @param chain the chain to calculate the inverse velocity
         * kinematics for
         *
         */
        explicit ChainIkSolverVel_pinv_givens(const Chain& chain);
        ~ChainIkSolverVel_pinv_givens();

        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out);
        /**
         * not (yet) implemented.
         *
         */
        virtual int CartToJnt(const JntArray& /*q_init*/, const FrameVel& /*v_in*/, JntArrayVel& /*q_out*/){return (error = E_NOT_IMPLEMENTED);};

        /// @copydoc KDL::SolverI::updateInternalDataStructures
        virtual void updateInternalDataStructures();

    private:
        const Chain& chain;
        unsigned int nj;
        ChainJntToJacSolver jnt2jac;
        Jacobian jac;
        bool transpose,toggle;
        unsigned int m,n;
        Eigen::MatrixXd jac_eigen,U,V,B;
        Eigen::VectorXd S,tempi,UY,SUY,qdot_eigen,v_in_eigen;
    };
}
#endif
