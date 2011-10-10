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

#include "tree.hpp"
#include <sstream>

namespace KDL {
using namespace std;

Tree::Tree(const std::string& _root_name) :
  nrOfSegments(0), nrOfJoints(0),root_name(_root_name) {
    segments.insert(make_pair(root_name, TreeElement::Root(root_name)));
}

Tree::Tree(const Tree& in) {
    segments.clear();
    nrOfSegments = 0;
    nrOfJoints = 0;
    root_name = in.root_name;

    segments.insert(make_pair(root_name, TreeElement::Root(root_name)));
    this->addTree(in, root_name);
}

Tree& Tree::operator=(const Tree& in) {
    segments.clear();
    nrOfSegments = 0;
    nrOfJoints = 0;
    root_name = in.root_name;

    segments.insert(make_pair(in.root_name, TreeElement::Root(root_name)));
    this->addTree(in, root_name);
    return *this;
}

bool Tree::addSegment(const Segment& segment, const std::string& hook_name) {
    SegmentMap::iterator parent = segments.find(hook_name);
    //check if parent exists
    if (parent == segments.end())
        return false;
    pair<SegmentMap::iterator, bool> retval;
    //insert new element
    unsigned int q_nr = segment.getJoint().getType() != Joint::None ? nrOfJoints : 0;
    retval = segments.insert(make_pair(segment.getName(), TreeElement(segment, parent, q_nr)));
    //check if insertion succeeded
    if (!retval.second)
        return false;
    //add iterator to new element in parents children list
    parent->second.children.push_back(retval.first);
    //increase number of segments
    nrOfSegments++;
    //increase number of joints
    if (segment.getJoint().getType() != Joint::None)
        nrOfJoints++;
    return true;
}
    
bool Tree::addChain(const Chain& chain, const std::string& hook_name) {
    string parent_name = hook_name;
    for (unsigned int i = 0; i < chain.getNrOfSegments(); i++) {
        if (this->addSegment(chain.getSegment(i), parent_name))
            parent_name = chain.getSegment(i).getName();
        else
            return false;
    }
    return true;
}

bool Tree::addTree(const Tree& tree, const std::string& hook_name) {
    return this->addTreeRecursive(tree.getRootSegment(), hook_name);
}

bool Tree::addTreeRecursive(SegmentMap::const_iterator root, const std::string& hook_name) {
    //get iterator for root-segment
    SegmentMap::const_iterator child;
    //try to add all of root's children
    for (unsigned int i = 0; i < root->second.children.size(); i++) {
        child = root->second.children[i];
        //Try to add the child
        if (this->addSegment(child->second.segment, hook_name)) {
            //if child is added, add all the child's children
            if (!(this->addTreeRecursive(child, child->first)))
                //if it didn't work, return false
                return false;
        } else
            //If the child could not be added, return false
            return false;
    }
    return true;
}
    
    bool Tree::getChain(const std::string& chain_root, const std::string& chain_tip, Chain& chain)const
    {
        // clear chain
        chain = Chain();
        
        // walk down from chain_root and chain_tip to the root of the tree
        vector<SegmentMap::key_type> parents_chain_root, parents_chain_tip;
        for (SegmentMap::const_iterator s=getSegment(chain_root); s!=segments.end(); s=s->second.parent){
            parents_chain_root.push_back(s->first);
            if (s->first == root_name) break;
        }
        if (parents_chain_root.empty() || parents_chain_root.back() != root_name) return false;
        for (SegmentMap::const_iterator s=getSegment(chain_tip); s!=segments.end(); s=s->second.parent){
            parents_chain_tip.push_back(s->first);
            if (s->first == root_name) break;
        }
        if (parents_chain_tip.empty() || parents_chain_tip.back()  != root_name) return false;
        
        // remove common part of segment lists
        SegmentMap::key_type last_segment = root_name;
        while (!parents_chain_root.empty() && !parents_chain_tip.empty() &&
               parents_chain_root.back() == parents_chain_tip.back()){
            last_segment = parents_chain_root.back();
            parents_chain_root.pop_back();
            parents_chain_tip.pop_back();
        }
        parents_chain_root.push_back(last_segment);
        
        
        // add the segments from the root to the common frame
        for (unsigned int s=0; s<parents_chain_root.size()-1; s++){
            Segment seg = getSegment(parents_chain_root[s])->second.segment;
            Frame f_tip = seg.pose(0.0).Inverse();
            Joint jnt = seg.getJoint();
            if (jnt.getType() == Joint::RotX || jnt.getType() == Joint::RotY || jnt.getType() == Joint::RotZ || jnt.getType() == Joint::RotAxis)
	      jnt = Joint(jnt.getName(), f_tip*jnt.JointOrigin(), f_tip.M*(-jnt.JointAxis()), Joint::RotAxis);
	    else if (jnt.getType() == Joint::TransX || jnt.getType() == Joint::TransY || jnt.getType() == Joint::TransZ || jnt.getType() == Joint::TransAxis)
	      jnt = Joint(jnt.getName(),f_tip*jnt.JointOrigin(), f_tip.M*(-jnt.JointAxis()), Joint::TransAxis);
	    chain.addSegment(Segment(getSegment(parents_chain_root[s+1])->second.segment.getName(),
                                     jnt, f_tip, getSegment(parents_chain_root[s+1])->second.segment.getInertia()));
        }
        
        // add the segments from the common frame to the tip frame
        for (int s=parents_chain_tip.size()-1; s>-1; s--){
            chain.addSegment(getSegment(parents_chain_tip[s])->second.segment);
        }
        return true;
    }
    
}//end of namespace


