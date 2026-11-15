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

#include "tree.hpp"
#include <sstream>

namespace KDL {

Tree::Tree(const std::string& _root_name) :
        nrOfJoints(0), nrOfSegments(0), root_name(_root_name)
{
    segments.insert(make_pair(root_name, TreeElement::Root(root_name)));
}

Tree::Tree(const Tree& in) {
    segments.clear();
    nrOfSegments = 0;
    nrOfJoints = 0;
    root_name = in.root_name;

    segments.insert(make_pair(root_name, TreeElement::Root(root_name)));
    addTree(in, root_name);
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
    std::pair<SegmentMap::iterator, bool> retval;
    //insert new element
    unsigned int q_nr = segment.getJoint().getType() != Joint::Fixed ? nrOfJoints : 0;

#ifdef KDL_USE_NEW_TREE_INTERFACE
    retval = segments.insert(make_pair(segment.getName(), TreeElementType( new TreeElement(segment, parent, q_nr))));
#else //#ifdef KDL_USE_NEW_TREE_INTERFACE
    retval = segments.insert(make_pair(segment.getName(), TreeElementType(segment, parent, q_nr)));
#endif //#ifdef KDL_USE_NEW_TREE_INTERFACE

    //check if insertion succeeded
    if (!retval.second)
        return false;
    //add iterator to new element in parents children list
    GetTreeElementChildren(parent->second).push_back(retval.first);
    //increase number of segments
    nrOfSegments++;
    //increase number of joints
    if (segment.getJoint().getType() != Joint::Fixed)
        nrOfJoints++;
    return true;
}

bool Tree::addChain(const Chain& chain, const std::string& hook_name) {
    std::string parent_name = hook_name;
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
    for (unsigned int i = 0; i < GetTreeElementChildren(root->second).size(); i++) {
        child = GetTreeElementChildren(root->second)[i];
        //Try to add the child
        if (this->addSegment(GetTreeElementSegment(child->second), hook_name)) {
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
    std::vector<SegmentMap::key_type> parents_chain_root, parents_chain_tip;
    for (SegmentMap::const_iterator s=getSegment(chain_root); s!=segments.end(); s = GetTreeElementParent(s->second)){
        parents_chain_root.push_back(s->first);
        if (s->first == root_name) break;
    }
    if (parents_chain_root.empty() || parents_chain_root.back() != root_name) return false;
    for (SegmentMap::const_iterator s=getSegment(chain_tip); s!=segments.end(); s = GetTreeElementParent(s->second)){
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
        Segment seg = GetTreeElementSegment(getSegment(parents_chain_root[s])->second);
        Frame f_tip = seg.pose(0.0).Inverse();
        Joint jnt = seg.getJoint();
        if (jnt.getType() == Joint::RotX || jnt.getType() == Joint::RotY || jnt.getType() == Joint::RotZ || jnt.getType() == Joint::RotAxis)
            jnt = Joint(jnt.getName(), f_tip*jnt.JointOrigin(), f_tip.M*(-jnt.JointAxis()), Joint::RotAxis);
        else if (jnt.getType() == Joint::TransX || jnt.getType() == Joint::TransY || jnt.getType() == Joint::TransZ || jnt.getType() == Joint::TransAxis)
            jnt = Joint(jnt.getName(),f_tip*jnt.JointOrigin(), f_tip.M*(-jnt.JointAxis()), Joint::TransAxis);
        chain.addSegment(Segment(GetTreeElementSegment(getSegment(parents_chain_root[s+1])->second).getName(),
                                 jnt, f_tip, GetTreeElementSegment(getSegment(parents_chain_root[s+1])->second).getInertia()));
    }

    // add the segments from the common frame to the tip frame
    for (auto rit=parents_chain_tip.rbegin(); rit != parents_chain_tip.rend(); ++rit){
        chain.addSegment(GetTreeElementSegment(getSegment(*rit)->second));
    }
    return true;
}

bool Tree::getSubTree(const std::string& segment_name, Tree& tree) const
{
  //check if segment_name exists
  SegmentMap::const_iterator root = segments.find(segment_name);
  if (root == segments.end())
    return false;
  //init the tree, segment_name is the new root.
  tree = Tree(root->first);
  return tree.addTreeRecursive(root, segment_name);
}

}//end of namespace
