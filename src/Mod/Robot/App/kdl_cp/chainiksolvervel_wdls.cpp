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
        jac(chain.getNrOfJoints()),
        U(MatrixXd::Zero(6,chain.getNrOfJoints())),
        S(VectorXd::Zero(chain.getNrOfJoints())),
        V(MatrixXd::Zero(chain.getNrOfJoints(),chain.getNrOfJoints())),
        eps(_eps),
        maxiter(_maxiter),
        tmp(VectorXd::Zero(chain.getNrOfJoints())),
        tmp_jac(MatrixXd::Zero(6,chain.getNrOfJoints())),
        tmp_jac_weight1(MatrixXd::Zero(6,chain.getNrOfJoints())),
        tmp_jac_weight2(MatrixXd::Zero(6,chain.getNrOfJoints())),
        tmp_ts(MatrixXd::Zero(6,6)),
        tmp_js(MatrixXd::Zero(chain.getNrOfJoints(),chain.getNrOfJoints())),
        weight_ts(MatrixXd::Identity(6,6)),
        weight_js(MatrixXd::Identity(chain.getNrOfJoints(),chain.getNrOfJoints())),
        lambda(0.0),
        lambda_scaled(0.0),
        nrZeroSigmas(0),
        svdResult(0),
        sigmaMin(0)
    {
    }
    
    ChainIkSolverVel_wdls::~ChainIkSolverVel_wdls()
    {
    }
    
    void ChainIkSolverVel_wdls::setWeightJS(const Eigen::MatrixXd& Mq){
        weight_js = Mq;
    }
    
    void ChainIkSolverVel_wdls::setWeightTS(const Eigen::MatrixXd& Mx){
        weight_ts = Mx;
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

    int ChainIkSolverVel_wdls::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        jnt2jac.JntToJac(q_in,jac);
        
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
        if ( nrZeroSigmas > (jac.columns()-jac.rows()) )	{
            return (error = E_CONVERGE_PINV_SINGULAR);  // converged but pinv singular
        } else {
            return (error = E_NOERROR);                 // have converged
        }
    }

    const char* ChainIkSolverVel_wdls::strError(const int error) const
    {
        if (E_SVD_FAILED == error)
            return "SVD failed";
        else return SolverI::strError(error);
    }

}
