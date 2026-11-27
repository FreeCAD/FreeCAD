// Copyright  (C)  2007  Ruben Smits <ruben dot smits at intermodalics dot eu>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at intermodalics dot eu>
// Maintainer: Ruben Smits <ruben dot smits at intermodalics dot eu>
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
        chain(_chain),locked_joints_(chain.getNrOfJoints(),false)
    {
    }

    void ChainJntToJacSolver::updateInternalDataStructures() {
        locked_joints_.resize(chain.getNrOfJoints(),false);
    }
    ChainJntToJacSolver::~ChainJntToJacSolver()
    {
    }

    int ChainJntToJacSolver::setLockedJoints(const std::vector<bool> locked_joints)
    {
        if(locked_joints_.size() != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);
        if(locked_joints.size()!=locked_joints_.size())
            return (error = E_SIZE_MISMATCH);
        locked_joints_=locked_joints;
        return (error = E_NOERROR);
    }

    int ChainJntToJacSolver::JntToJac(const JntArray& q_in, Jacobian& jac, int seg_nr)
    {
        if(locked_joints_.size() != chain.getNrOfJoints())
            return (error = E_NOT_UP_TO_DATE);
        unsigned int segmentNr;
        if(seg_nr<0)
            segmentNr=chain.getNrOfSegments();
        else
            segmentNr = seg_nr;

        //Initialize Jacobian to zero since only segmentNr columns are computed
        SetToZero(jac) ;

        if( q_in.rows()!=chain.getNrOfJoints() || jac.columns() != chain.getNrOfJoints())
            return (error = E_SIZE_MISMATCH);
        else if(segmentNr>chain.getNrOfSegments())
            return (error = E_OUT_OF_RANGE);

        T_tmp = Frame::Identity();
        SetToZero(t_tmp);
        int j=0;
        int k=0;
        Frame total;
        for (unsigned int i=0;i<segmentNr;i++) {
            //Calculate new Frame_base_ee
            if(chain.getSegment(i).getJoint().getType()!=Joint::Fixed) {
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
            if(chain.getSegment(i).getJoint().getType()!=Joint::Fixed) {
                //Only put the twist inside if it is not locked
                if(!locked_joints_[j])
                    jac.setColumn(k++,t_tmp);
                j++;
            }

            T_tmp = total;
        }
        return (error = E_NOERROR);
    }
}

