/*
 * TreeIkSolverVel_wdls.cpp
 *
 *  Created on: Nov 28, 2008
 *      Author: rubensmits
 */

#include "treeiksolvervel_wdls.hpp"
#include "utilities/svd_eigen_HH.hpp"

namespace KDL {
    using namespace std;
    
    TreeIkSolverVel_wdls::TreeIkSolverVel_wdls(const Tree& tree_in, const std::vector<std::string>& endpoints) :
        tree(tree_in), jnttojacsolver(tree),
        J(MatrixXd::Zero(6 * endpoints.size(), tree.getNrOfJoints())),
        Wy(MatrixXd::Identity(J.rows(),J.rows())),
        Wq(MatrixXd::Identity(J.cols(),J.cols())),
        J_Wq(J.rows(),J.cols()),Wy_J_Wq(J.rows(),J.cols()),
        U(MatrixXd::Identity(J.rows(),J.cols())),
        V(MatrixXd::Identity(J.cols(),J.cols())),
        Wy_U(J.rows(),J.rows()),Wq_V(J.cols(),J.cols()),
        t(VectorXd::Zero(J.rows())), Wy_t(VectorXd::Zero(J.rows())),
        qdot(VectorXd::Zero(J.cols())),
        tmp(VectorXd::Zero(J.cols())),S(VectorXd::Zero(J.cols()))
    {
        
        for (size_t i = 0; i < endpoints.size(); ++i) {
            jacobians.insert(Jacobians::value_type(endpoints[i], Jacobian(tree.getNrOfJoints())));
        }
        
    }
    
    TreeIkSolverVel_wdls::~TreeIkSolverVel_wdls() {
    }
    
    void TreeIkSolverVel_wdls::setWeightJS(const MatrixXd& Mq) {
        Wq = Mq;
    }
    
    void TreeIkSolverVel_wdls::setWeightTS(const MatrixXd& Mx) {
        Wy = Mx;
    }
    
    void TreeIkSolverVel_wdls::setLambda(const double& lambda_in) {
        lambda = lambda_in;
    }
    
    double TreeIkSolverVel_wdls::CartToJnt(const JntArray& q_in, const Twists& v_in, JntArray& qdot_out) {
        
        //First check if we are configured for this Twists:
        for (Twists::const_iterator v_it = v_in.begin(); v_it != v_in.end(); ++v_it) {
            if (jacobians.find(v_it->first) == jacobians.end())
                return -2;
        }
        //Check if q_in has the right size
        if (q_in.rows() != tree.getNrOfJoints())
            return -1;
        
        //Lets get all the jacobians we need:
        unsigned int k = 0;
        for (Jacobians::iterator jac_it = jacobians.begin(); jac_it
                 != jacobians.end(); ++jac_it) {
            int ret = jnttojacsolver.JntToJac(q_in, jac_it->second, jac_it->first);
            if (ret < 0)
                return ret;
            else {
                //lets put the jacobian in the big matrix and put the twist in the big t:
                J.block(6*k,0, 6,tree.getNrOfJoints()) = jac_it->second.data;
                const Twist& twist=v_in.find(jac_it->first)->second;
                t.segment(6*k,3)   = Eigen::Map<Eigen::Vector3d>(twist.vel.data);
                t.segment(6*k+3,3) = Eigen::Map<Eigen::Vector3d>(twist.rot.data);
            }
            ++k;
        }
        
        //Lets use the wdls algorithm to find the qdot:
        // Create the Weighted jacobian
        J_Wq = (J * Wq).lazy();
        Wy_J_Wq = (Wy * J_Wq).lazy();
        
        // Compute the SVD of the weighted jacobian
        int ret = svd_eigen_HH(Wy_J_Wq, U, S, V, tmp);
        
        //Pre-multiply U and V by the task space and joint space weighting matrix respectively
        Wy_t = (Wy * t).lazy();
        Wq_V = (Wq * V).lazy();
        
        // tmp = (Si*Wy*U'*y),
        for (unsigned int i = 0; i < J.cols(); i++) {
            double sum = 0.0;
            for (unsigned int j = 0; j < J.rows(); j++) {
                if (i < Wy_t.size())
                    sum += U(j, i) * Wy_t(j);
                else
                    sum += 0.0;
            }
            tmp( i) = sum * ((S(i) / (S(i) * S(i) + lambda * lambda)));
        }
        
        // x = Lx^-1*V*tmp + x
        qdot_out.data = (Wq_V * tmp).lazy();
        
        return Wy_t.norm();
    }
}
