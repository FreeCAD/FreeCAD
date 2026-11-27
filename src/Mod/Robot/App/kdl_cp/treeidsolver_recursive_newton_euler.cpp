// Copyright  (C)  2009  Ruben Smits <ruben dot smits at intermodalics dot eu>

// Version: 1.0
// Author: Franco Fusco <franco dot fusco at ls2n dot fr>
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

#include "treeidsolver_recursive_newton_euler.hpp"
#include "frames_io.hpp"
#include <stdexcept>

namespace KDL{

    TreeIdSolver_RNE::TreeIdSolver_RNE(const Tree& tree_, Vector grav):
        tree(tree_), nj(tree.getNrOfJoints()), ns(tree.getNrOfSegments())
    {
      ag=-Twist(grav,Vector::Zero());
      initAuxVariables();
    }

    void TreeIdSolver_RNE::updateInternalDataStructures() {
      nj = tree.getNrOfJoints();
      ns = tree.getNrOfSegments();
      initAuxVariables();
    }

    void TreeIdSolver_RNE::initAuxVariables() {
      const SegmentMap& segments = tree.getSegments();
      for(SegmentMap::const_iterator seg = segments.begin(); seg != segments.end(); seg++) {
        X[seg->first] = Frame();
        S[seg->first] = Twist();
        v[seg->first] = Twist();
        a[seg->first] = Twist();
        f[seg->first] = Wrench();
      }
    }

    int TreeIdSolver_RNE::CartToJnt(const JntArray &q, const JntArray &q_dot, const JntArray &q_dotdot, const WrenchMap& f_ext, JntArray &torques)
    {
      //Check that the tree was not modified externally
      if(nj != tree.getNrOfJoints() || ns != tree.getNrOfSegments())
        return (error = E_NOT_UP_TO_DATE);

      //Check sizes of joint vectors
      if(q.rows()!=nj || q_dot.rows()!=nj || q_dotdot.rows()!=nj || torques.rows()!=nj)
        return (error = E_SIZE_MISMATCH);

      try {
        //Do the recursion here
        rne_step(tree.getRootSegment(), q, q_dot, q_dotdot, f_ext, torques);
      }
      catch(const std::out_of_range&) {
        //If in rne_step we get an out_of_range exception it means that some call
        //to map::at failed. This can happen only if updateInternalDataStructures
        //was not called after changing something in the tree. Note that it
        //should be impossible to reach this point as consistency of the tree is
        //checked above.
        return (error = E_NOT_UP_TO_DATE);
      }
      return (error = E_NOERROR);
    }


    void TreeIdSolver_RNE::rne_step(SegmentMap::const_iterator segment, const JntArray &q, const JntArray &q_dot, const JntArray &q_dotdot, const WrenchMap& f_ext, JntArray& torques) {
      const Segment& seg = GetTreeElementSegment(segment->second);
      const std::string& segname = segment->first;
      const std::string& parname = GetTreeElementParent(segment->second)->first;

      //Do forward calculations involving velocity & acceleration of this segment
      double q_, qdot_, qdotdot_;
      unsigned int j = GetTreeElementQNr(segment->second);
      if(seg.getJoint().getType()!=Joint::Fixed) {
        q_ = q(j);
        qdot_ = q_dot(j);
        qdotdot_ = q_dotdot(j);
      }
      else
        q_ = qdot_ = qdotdot_ = 0.0;

      //Calculate segment properties: X,S,vj,cj

      //Remark this is the inverse of the frame for transformations from the parent to the current coord frame
      X.at(segname) = seg.pose(q_);

      //Transform velocity and unit velocity to segment frame
      Twist vj = X.at(segname).M.Inverse( seg.twist(q_,qdot_) );
      S.at(segname) = X.at(segname).M.Inverse( seg.twist(q_,1.0) );

      //calculate velocity and acceleration of the segment (in segment coordinates)
      if(segment == tree.getRootSegment()) {
        v.at(segname) = vj;
        a.at(segname) = X.at(segname).Inverse(ag) + S.at(segname)*qdotdot_+ v.at(segname)*vj;
      }
      else {
        v.at(segname) = X.at(segname).Inverse(v.at(parname)) + vj;
        a.at(segname) = X.at(segname).Inverse(a.at(parname)) + S.at(segname)*qdotdot_ + v.at(segname)*vj;
      }

      //Calculate the force for the joint
      //Collect RigidBodyInertia and external forces
      const RigidBodyInertia& I = seg.getInertia();
      f.at(segname) = I*a.at(segname) + v.at(segname)*(I*v.at(segname));
      if(f_ext.find(segname) != f_ext.end())
        f.at(segname) = f.at(segname) - f_ext.at(segname);

      //propagate calculations over each child segment
      SegmentMap::const_iterator child;
      for (unsigned int i = 0; i < GetTreeElementChildren(segment->second).size(); i++) {
        child = GetTreeElementChildren(segment->second)[i];
        rne_step(child, q, q_dot, q_dotdot, f_ext, torques);
      }

      //do backward calculations involving wrenches and joint efforts

      //If there is a moving joint, evaluate its effort
      if(seg.getJoint().getType()!=Joint::Fixed) {
        torques(j) = dot(S.at(segname), f.at(segname));
        torques(j) += seg.getJoint().getInertia()*q_dotdot(j);  // add torque from joint inertia
      }

      //add reaction forces to parent segment
      if(segment != tree.getRootSegment())
        f.at(parname) = f.at(parname) + X.at(segname)*f.at(segname);
    }
}//namespace
