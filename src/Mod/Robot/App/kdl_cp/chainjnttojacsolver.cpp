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

#include "chainjnttojacsolver.hpp"

namespace KDL
{
    ChainJntToJacSolver::ChainJntToJacSolver(const Chain& _chain):
        chain(_chain),locked_joints_(chain.getNrOfJoints(),false),
        nr_of_unlocked_joints_(chain.getNrOfJoints())
    {
    }

    ChainJntToJacSolver::~ChainJntToJacSolver()
    {
    }

    int ChainJntToJacSolver::setLockedJoints(const std::vector<bool> locked_joints)
    {
        if(locked_joints.size()!=locked_joints_.size())
            return -1;
        locked_joints_=locked_joints;
        nr_of_unlocked_joints_=0;
        for(unsigned int i=0;i<locked_joints_.size();i++){
            if(!locked_joints_[i])
                nr_of_unlocked_joints_++;
        }
    }

    int ChainJntToJacSolver::JntToJac(const JntArray& q_in,Jacobian& jac)
    {
        if(q_in.rows()!=chain.getNrOfJoints()||nr_of_unlocked_joints_!=jac.columns())
            return -1;
        T_tmp = Frame::Identity();
        SetToZero(t_tmp);
        int j=0;
        int k=0;
        Frame total;
        for (unsigned int i=0;i<chain.getNrOfSegments();i++) {
            //Calculate new Frame_base_ee
            if(chain.getSegment(i).getJoint().getType()!=Joint::None){
            	//pose of the new end-point expressed in the base
                total = T_tmp*chain.getSegment(i).pose(q_in(j));
                //changing base of new segment's twist to base frame if it is not locked
                //t_tmp = T_tmp.M*chain.getSegment(i).twist(1.0);
                if(!locked_joints_[j])
                    t_tmp = T_tmp.M*chain.getSegment(i).twist(q_in(j),1.0);
            }else{
                total = T_tmp*chain.getSegment(i).pose(0.0);

            }

            //Changing Refpoint of all columns to new ee
            changeRefPoint(jac,total.p-T_tmp.p,jac);

            //Only increase jointnr if the segment has a joint
            if(chain.getSegment(i).getJoint().getType()!=Joint::None){
                //Only put the twist inside if it is not locked
                if(!locked_joints_[j])
                    jac.setColumn(k++,t_tmp);
                j++;
            }

            T_tmp = total;
        }
        return 0;
    }
}

