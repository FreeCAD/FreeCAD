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

#include "chainiksolvervel_pinv_nso.hpp"
#include "utilities/svd_eigen_HH.hpp"

namespace KDL
{
    ChainIkSolverVel_pinv_nso::ChainIkSolverVel_pinv_nso(const Chain& _chain, JntArray _opt_pos, JntArray _weights, double _eps, int _maxiter, double _alpha):
        chain(_chain),
        jnt2jac(chain),
        nj(chain.getNrOfJoints()),
        jac(nj),
        U(MatrixXd::Zero(6,nj)),
        S(VectorXd::Zero(nj)),
        Sinv(VectorXd::Zero(nj)),
        V(MatrixXd::Zero(nj,nj)),
        tmp(VectorXd::Zero(nj)),
        tmp2(VectorXd::Zero(nj)),
        eps(_eps),
        maxiter(_maxiter),
        alpha(_alpha),
        weights(_weights),
        opt_pos(_opt_pos)
    {
    }

    ChainIkSolverVel_pinv_nso::ChainIkSolverVel_pinv_nso(const Chain& _chain, double _eps, int _maxiter, double _alpha):
        chain(_chain),
        jnt2jac(chain),
        nj(chain.getNrOfJoints()),
        jac(nj),
        U(MatrixXd::Zero(6,nj)),
        S(VectorXd::Zero(nj)),
        Sinv(VectorXd::Zero(nj)),
        V(MatrixXd::Zero(nj,nj)),
        tmp(VectorXd::Zero(nj)),
        tmp2(VectorXd::Zero(nj)),
        eps(_eps),
        maxiter(_maxiter),
        alpha(_alpha)
    {
    }

    ChainIkSolverVel_pinv_nso::~ChainIkSolverVel_pinv_nso()
    {
    }


    int ChainIkSolverVel_pinv_nso::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        //Let the ChainJntToJacSolver calculate the jacobian "jac" for
        //the current joint positions "q_in" 
        jnt2jac.JntToJac(q_in,jac);

        //Do a singular value decomposition of "jac" with maximum
        //iterations "maxiter", put the results in "U", "S" and "V"
        //jac = U*S*Vt
        int svdResult = svd_eigen_HH(jac.data,U,S,V,tmp,maxiter);
        if (0 != svdResult)
        {
            qdot_out.data.setZero() ;
            return svdResult;
        }

        unsigned int i;

        // We have to calculate qdot_out = jac_pinv*v_in
        // Using the svd decomposition this becomes(jac_pinv=V*S_pinv*Ut):
        // qdot_out = V*S_pinv*Ut*v_in

        // S^-1
        for (i = 0; i < nj; ++i) {
            Sinv(i) = fabs(S(i))<eps ? 0.0 : 1.0/S(i);
        }
        for (i = 0; i < 6; ++i) {
            tmp(i) = v_in(i);
        }

        qdot_out.data = V * Sinv.asDiagonal() * U.transpose() * tmp.head(6);

        // Now onto NULL space
        // Given the cost function g, and the current joints q, desired joints qd, and weights w:
        // t = g(q) = 1/2 * Sum( w_i * (q_i - qd_i)^2 )
        //
        // The jacobian Jc is:
        //  t_dot = Jc(q) * q_dot
        //  Jc = dt/dq = w_j * (q_i - qd_i) [1 x nj vector]
        //
        // The pseudo inverse (^-1) is
        // Jc^-1 = w_j * (q_i - qd_i) / A [nj x 1 vector]
        // A = Sum( w_i^2 * (q_i - qd_i)^2 )
        //
        // We can set the change as the step needed to remove the error times a value alpha:
        // t_dot = -2 * alpha * t
        //
        // When we put it together and project into the nullspace, the final result is
        // q'_out += (I_n - J^-1 * J) * Jc^-1 * (-2 * alpha * g(q))

        double g = 0; // g(q)
        double A = 0; // normalizing term
        for (i = 0; i < nj; ++i) {
            double qd = q_in(i) - opt_pos(i);
            g += 0.5 * qd*qd * weights(i);
            A += qd*qd * weights(i)*weights(i);
        }

        if (A > 1e-9) {
          // Calculate inverse Jacobian Jc^-1
          for (i = 0; i < nj; ++i) {
              tmp(i) = weights(i)*(q_in(i) - opt_pos(i)) / A;
          }

          // Calculate J^-1 * J * Jc^-1 = V*S^-1*U' * U*S*V' * tmp
          tmp2 = V * Sinv.asDiagonal() * U.transpose() * U * S.asDiagonal() * V.transpose() * tmp;

          for (i = 0; i < nj; ++i) {
              //std::cerr << i <<": "<< qdot_out(i) <<", "<< -2*alpha*g * (tmp(i) - tmp2(i)) << std::endl;
              qdot_out(i) += -2*alpha*g * (tmp(i) - tmp2(i));
          }
        }
        //return the return value of the svd decomposition
        return svdResult;
    }

    int ChainIkSolverVel_pinv_nso::setWeights(const JntArray & _weights)
    {
      weights = _weights;
      return 0;
    }

    int ChainIkSolverVel_pinv_nso::setOptPos(const JntArray & _opt_pos)
    {
      opt_pos = _opt_pos;
      return 0;
    }

    int ChainIkSolverVel_pinv_nso::setAlpha(const double _alpha)
    {
      alpha = _alpha;
      return 0;
    }


}
