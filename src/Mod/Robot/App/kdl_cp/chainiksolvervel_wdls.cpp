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
        lambda(0.0)
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

    void ChainIkSolverVel_wdls::setLambda(const double& lambda_in)
    {
        lambda=lambda_in;
    }
    
    int ChainIkSolverVel_wdls::CartToJnt(const JntArray& q_in, const Twist& v_in, JntArray& qdot_out)
    {
        jnt2jac.JntToJac(q_in,jac);
        
        double sum;
        unsigned int i,j;
        
        /*
        for (i=0;i<jac.rows();i++) {
            for (j=0;j<jac.columns();j++)
                tmp_jac(i,j) = jac(i,j);
        }
        */
        
        // Create the Weighted jacobian
        tmp_jac_weight1 = (jac.data*weight_js).lazy();
        tmp_jac_weight2 = (weight_ts*tmp_jac_weight1).lazy();
   
        // Compute the SVD of the weighted jacobian
        int ret = svd_eigen_HH(tmp_jac_weight2,U,S,V,tmp,maxiter);
                
        //Pre-multiply U and V by the task space and joint space weighting matrix respectively
        tmp_ts = (weight_ts*U.corner(Eigen::TopLeft,6,6)).lazy();
        tmp_js = (weight_js*V).lazy(); 
        
        // tmp = (Si*U'*Ly*y), 
        for (i=0;i<jac.columns();i++) {
            sum = 0.0;
            for (j=0;j<jac.rows();j++) {
                if(i<6)
                    sum+= tmp_ts(j,i)*v_in(j);
                else
                    sum+=0.0;
            }
            if(S(i)==0||S(i)<eps)
                tmp(i) = sum*((S(i)/(S(i)*S(i)+lambda*lambda)));   
            else
                tmp(i) = sum/S(i);
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
        qdot_out.data=(tmp_js*tmp).lazy();
        return ret;
    }
    
}
