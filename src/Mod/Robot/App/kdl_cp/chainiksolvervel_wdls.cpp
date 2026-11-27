// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include "chainiksolvervel_wdls.hpp"
#include "utilities/svd_eigen_HH.hpp"

namespace KDL
{
    
    ChainIkSolverVel_wdls::ChainIkSolverVel_wdls(const Chain& _chain,double _eps,int _maxiter):
        chain(_chain),
        jnt2jac(chain),
        nj(chain.getNrOfJoints()),
        jac(nj),
        U(Eigen::MatrixXd::Zero(6,nj)),
        S(Eigen::VectorXd::Zero(nj)),
        V(Eigen::MatrixXd::Zero(nj,nj)),
        eps(_eps),
        maxiter(_maxiter),
        tmp(Eigen::VectorXd::Zero(nj)),
        tmp_jac(Eigen::MatrixXd::Zero(6,nj)),
        tmp_jac_weight1(Eigen::MatrixXd::Zero(6,nj)),
        tmp_jac_weight2(Eigen::MatrixXd::Zero(6,nj)),
        tmp_ts(Eigen::MatrixXd::Zero(6,6)),
        tmp_js(Eigen::MatrixXd::Zero(nj,nj)),
        weight_ts(Eigen::MatrixXd::Identity(6,6)),
        weight_js(Eigen::MatrixXd::Identity(nj,nj)),
        lambda(0.0),
        lambda_scaled(0.0),
        nrZeroSigmas(0),
        svdResult(0),
        sigmaMin(0)
    {
    }
    
    void ChainIkSolverVel_wdls::updateInternalDataStructures() {
        jnt2jac.updateInternalDataStructures();
        nj = chain.getNrOfJoints();
        jac.resize(nj);
        Eigen::MatrixXd z6nj = Eigen::MatrixXd::Zero(6,nj);
        Eigen::VectorXd znj = Eigen::VectorXd::Zero(nj);
        Eigen::MatrixXd znjnj = Eigen::MatrixXd::Zero(nj,nj);
        U.conservativeResizeLike(z6nj);
        S.conservativeResizeLike(znj);
        V.conservativeResizeLike(znjnj);
        tmp.conservativeResizeLike(znj);
        tmp_jac.conservativeResizeLike(z6nj);
        tmp_jac_weight1.conservativeResizeLike(z6nj);
        tmp_jac_weight2.conservativeResizeLike(z6nj);
        tmp_js.conservativeResizeLike(znjnj);
        weight_js.conservativeResizeLike(Eigen::MatrixXd::Identity(nj,nj));
    }

    ChainIkSolverVel_wdls::~ChainIkSolverVel_wdls()
    {
    }
    
    int ChainIkSolverVel_wdls::setWeightJS(const Eigen::MatrixXd& Mq){
        if(nj != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);

        if (Mq.size() != weight_js.size())
            return (error = E_SIZE_MISMATCH);
        weight_js = Mq;
        return (error = E_NOERROR);
    }
    
    int ChainIkSolverVel_wdls::setWeightTS(const Eigen::MatrixXd& Mx){
        if (Mx.size() != weight_ts.size())
            return (error = E_SIZE_MISMATCH);
        weight_ts = Mx;
        return (error = E_NOERROR);
    }

    void ChainIkSolverVel_wdls::setLambda(const double lambda_in)
    {
        lambda=lambda_in;
    }

    void ChainIkSolverVel_wdls::setEps(const double eps_in)
    {
        eps=eps_in;
    }

    void ChainIkSolverVel_wdls::setMaxIter(const int maxiter_in)
    {
        maxiter=maxiter_in;
    }

    int ChainIkSolverVel_wdls::getSigma(Eigen::VectorXd& Sout)
    {
        if (Sout.size() != S.size())
            return (error = E_SIZE_MISMATCH);
        Sout=S;
        return (error = E_NOERROR);
    }

    int ChainIkSolverVel_wdls::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        if(nj != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);

        if(nj != q_in.rows() || nj != qdot_out.rows())
            return (error = E_SIZE_MISMATCH);
        error = jnt2jac.JntToJac(q_in,jac);
        if ( error < E_NOERROR) return error;

        double sum;
        unsigned int i,j;

        // Initialize (internal) return values
        nrZeroSigmas = 0 ;
        sigmaMin = 0.;
        lambda_scaled = 0.;

        /*
        for (i=0;i<jac.rows();i++) {
            for (j=0;j<jac.columns();j++)
                tmp_jac(i,j) = jac(i,j);
        }
        */

        // Create the Weighted jacobian
        tmp_jac_weight1 = jac.data.lazyProduct(weight_js);
        tmp_jac_weight2 = weight_ts.lazyProduct(tmp_jac_weight1);

        // Compute the SVD of the weighted jacobian
        svdResult = svd_eigen_HH(tmp_jac_weight2,U,S,V,tmp,maxiter);
        if (0 != svdResult)
        {
            qdot_out.data.setZero() ;
            return (error = E_SVD_FAILED);
        }

        //Pre-multiply U and V by the task space and joint space weighting matrix respectively
        tmp_ts = weight_ts.lazyProduct(U.topLeftCorner(6,6));
        tmp_js = weight_js.lazyProduct(V);

        // Minimum of six largest singular values of J is S(5) if number of joints >=6 and 0 for <6
        if ( jac.columns() >= 6 ) {
            sigmaMin = S(5);
        }
        else {
            sigmaMin = 0.;
        }

        // tmp = (Si*U'*Ly*y),
        for (i=0;i<jac.columns();i++) {
            sum = 0.0;
            for (j=0;j<jac.rows();j++) {
                if(i<6)
                    sum+= tmp_ts(j,i)*v_in(j);
                else
                    sum+=0.0;
            }
            // If sigmaMin > eps, then wdls is not active and lambda_scaled = 0 (default value)
            // If sigmaMin < eps, then wdls is active and lambda_scaled is scaled from 0 to lambda
            // Note:  singular values are always positive so sigmaMin >=0
            if ( sigmaMin < eps )
            {
                lambda_scaled = sqrt(1.0-(sigmaMin/eps)*(sigmaMin/eps))*lambda ;
            }
            if(fabs(S(i))<eps) {
                if (i<6) {
                    // Scale lambda to size of singular value sigmaMin
                    tmp(i) = sum*((S(i)/(S(i)*S(i)+lambda_scaled*lambda_scaled)));
                }
                else {
                    tmp(i)=0.0;  // S(i)=0 for i>=6 due to cols>rows
                }
                //  Count number of singular values near zero
                ++nrZeroSigmas ;
            }
            else {
                tmp(i) = sum/S(i);
            }
        }

        /*
        // x = Lx^-1*V*tmp + x
        for (i=0;i<jac.columns();i++) {
            sum = 0.0;
            for (j=0;j<jac.columns();j++) {
                sum+=tmp_js(i,j)*tmp(j);
            }
            qdot_out(i)=sum;
        }
        */
        qdot_out.data=tmp_js.lazyProduct(tmp);

        // If number of near zero singular values is greater than the full rank
        // of jac, then wdls is active
        if ( nrZeroSigmas > (jac.columns()-jac.rows()) ) {
            return (error = E_CONVERGE_PINV_SINGULAR);  // converged but pinv singular
        } else {
            return (error = E_NOERROR);                 // have converged
        }
    }

    const char* ChainIkSolverVel_wdls::strError(const int error) const
    {
        if (E_CONVERGE_PINV_SINGULAR == error) return "Converged put pseudo inverse of jacobian is singular.";
        else return SolverI::strError(error);
    }

}
