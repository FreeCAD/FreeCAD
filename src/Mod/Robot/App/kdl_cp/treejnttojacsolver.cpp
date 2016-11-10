/*
 * TreeJntToJacSolver.cpp
 *
 *  Created on: Nov 27, 2008
 *      Author: rubensmits
 */

#include "treejnttojacsolver.hpp"
#include <iostream>
#include "kinfam_io.hpp"

namespace KDL {

TreeJntToJacSolver::TreeJntToJacSolver(const Tree& tree_in) :
    tree(tree_in) {
}

TreeJntToJacSolver::~TreeJntToJacSolver() {
}

int TreeJntToJacSolver::JntToJac(const JntArray& q_in, Jacobian& jac, const std::string& segmentname) {
    //First we check all the sizes:
    if (q_in.rows() != tree.getNrOfJoints() || jac.columns() != tree.getNrOfJoints())
        return -1;
    
    //Lets search the tree-element
    SegmentMap::const_iterator it = tree.getSegments().find(segmentname);

    //If segmentname is not inside the tree, back out:
    if (it == tree.getSegments().end())
        return -2;
    
    //Let's make the jacobian zero:
    SetToZero(jac);
    
    SegmentMap::const_iterator root = tree.getRootSegment();

    Frame T_total = Frame::Identity();
    //Lets recursively iterate until we are in the root segment
    while (it != root) {
        //get the corresponding q_nr for this TreeElement:
        unsigned int q_nr = GetTreeElementQNr(it->second);
        
        //get the pose of the segment:
        Frame T_local = GetTreeElementSegment(it->second).pose(q_in(q_nr));
        //calculate new T_end:
        T_total = T_local * T_total;
        
        //get the twist of the segment:
        if (GetTreeElementSegment(it->second).getJoint().getType() != Joint::None) {
            Twist t_local = GetTreeElementSegment(it->second).twist(q_in(q_nr), 1.0);
            //transform the endpoint of the local twist to the global endpoint:
            t_local = t_local.RefPoint(T_total.p - T_local.p);
            //transform the base of the twist to the endpoint
            t_local = T_total.M.Inverse(t_local);
            //store the twist in the jacobian:
            jac.setColumn(q_nr,t_local);
        }//endif
        //goto the parent
        it = GetTreeElementParent(it->second);
    }//endwhile
    //Change the base of the complete jacobian from the endpoint to the base
    changeBase(jac, T_total.M, jac);
    
    return 0;
    
}//end JntToJac
}//end namespace

