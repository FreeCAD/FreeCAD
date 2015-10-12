// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

#ifndef KDL_CHAIN_IKSOLVERVEL_PINV_GIVENS_HPP
#define KDL_CHAIN_IKSOLVERVEL_PINV_GIVENS_HPP

#include "chainiksolver.hpp"
#include "chainjnttojacsolver.hpp"

#include <Eigen/Core>

using namespace Eigen;

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
         * @param eps if a singular value is below this value, its
         * inverse is set to zero, default: 0.00001
         * @param maxiter maximum iterations for the svd calculation,
         * default: 150
         *
         */
        explicit ChainIkSolverVel_pinv_givens(const Chain& chain);
        ~ChainIkSolverVel_pinv_givens();

        virtual int CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out);
        /**
         * not (yet) implemented.
         *
         */
        virtual int CartToJnt(const JntArray& q_init, const FrameVel& v_in, JntArrayVel& q_out){return -1;};
    private:
        const Chain chain;
        ChainJntToJacSolver jnt2jac;
        Jacobian jac;
        bool transpose,toggle;
        unsigned int m,n;
        MatrixXd jac_eigen,U,V,B;
        VectorXd S,tempi,tempj,UY,SUY,qdot_eigen,v_in_eigen;
    };
}
#endif
