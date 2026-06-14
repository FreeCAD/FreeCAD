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

#pragma once

// This file comes along with oce but not with occ.
//#include "config.h"

#include "segment.hpp"
#include "chain.hpp"

#include <string>
#include <map>

#ifdef KDL_USE_NEW_TREE_INTERFACE
#include <memory>
#endif //#ifdef KDL_USE_NEW_TREE_INTERFACE

namespace KDL
{
    class TreeElement;

#ifdef KDL_USE_NEW_TREE_INTERFACE
    //We use smart pointers for managing tree nodes for now because
    //c++11 and unique_ptr support is not ubiquitous
    typedef std::shared_ptr<TreeElement> TreeElementPtr;
    typedef std::shared_ptr<const TreeElement> TreeElementConstPtr;
    typedef std::map<std::string, TreeElementPtr> SegmentMap;
    typedef TreeElementPtr TreeElementType;

#define GetTreeElementChildren(tree_element) (tree_element)->children
#define GetTreeElementParent(tree_element) (tree_element)->parent
#define GetTreeElementQNr(tree_element) (tree_element)->q_nr
#define GetTreeElementSegment(tree_element) (tree_element)->segment

#else //#ifdef KDL_USE_NEW_TREE_INTERFACE
    //Forward declaration
    typedef std::map<std::string,TreeElement> SegmentMap;
    typedef TreeElement TreeElementType;

#define GetTreeElementChildren(tree_element) (tree_element).children
#define GetTreeElementParent(tree_element) (tree_element).parent
#define GetTreeElementQNr(tree_element) (tree_element).q_nr
#define GetTreeElementSegment(tree_element) (tree_element).segment

#endif //#ifdef KDL_USE_NEW_TREE_INTERFACE

    class TreeElement
    {
    public:
        TreeElement(const Segment& segment_in,const SegmentMap::const_iterator& parent_in,unsigned int q_nr_in):
            segment(segment_in),
            q_nr(q_nr_in),
            parent(parent_in)
        {}

        static TreeElementType Root(const std::string& root_name)
        {
#ifdef KDL_USE_NEW_TREE_INTERFACE
            return TreeElementType(new TreeElement(root_name));
#else //#define KDL_USE_NEW_TREE_INTERFACE
            return TreeElementType(root_name);
#endif
        }

        Segment segment;
        unsigned int q_nr;
        SegmentMap::const_iterator  parent;
        std::vector<SegmentMap::const_iterator > children;

    private:
        TreeElement(const std::string& name):segment(name), q_nr(0) {}
    };

    /**
     * \brief  This class encapsulates a <strong>tree</strong>
     * kinematic interconnection structure. It is built out of segments.
     *
     * @ingroup KinematicFamily
     */
    class Tree
    {
    private:
        SegmentMap segments;
        unsigned int nrOfJoints;
        unsigned int nrOfSegments;

        std::string root_name;

        bool addTreeRecursive(SegmentMap::const_iterator root, const std::string& hook_name);

    public:
        /**
         * The constructor of a tree, a new tree is always empty
         */
        explicit Tree(const std::string& root_name="root");
        Tree(const Tree& in);
        Tree& operator= (const Tree& arg);

        /**
         * Adds a new segment to the end of the segment with
         * hook_name as segment_name
         *
         * @param segment new segment to add
         * @param hook_name name of the segment to connect this
         * segment with.
         *
         * @return false if hook_name could not be found.
         */
         bool addSegment(const Segment& segment, const std::string& hook_name);

        /**
         * Adds a complete chain to the end of the segment with
         * hook_name as segment_name. 
         *
         * @param hook_name name of the segment to connect the chain with.
         *
         * @return false if hook_name could not be found.
         */
        bool addChain(const Chain& chain, const std::string& hook_name);

        /**
         * Adds a complete tree to the end of the segment with
         * hookname as segment_name. 
         *
         * @param tree Tree to add
         * @param hook_name name of the segment to connect the tree with
         *
         * @return false if hook_name could not be found
         */
        bool addTree(const Tree& tree, const std::string& hook_name);

        /**
         * Request the total number of joints in the tree.\n
         * <strong> Important:</strong> It is not the same as the
         * total number of segments since a segment does not need to have
         * a joint.
         *
         * @return total nr of joints
         */
        unsigned int getNrOfJoints()const
        {
            return nrOfJoints;
        };

        /**
         * Request the total number of segments in the tree.
         * @return total number of segments
         */
        unsigned int getNrOfSegments()const {return nrOfSegments;};

        /**
         * Request the segment of the tree with name segment_name.
         *
         * @param segment_name the name of the requested segment
         *
         * @return constant iterator pointing to the requested segment
         */
        SegmentMap::const_iterator getSegment(const std::string& segment_name)const
        {
            return segments.find(segment_name);
        };
        /**
         * Request the root segment of the tree
         *
         * @return constant iterator pointing to the root segment
         */
        SegmentMap::const_iterator getRootSegment()const
        {
          return segments.find(root_name);
        };

        /**
         * Request the chain of the tree between chain_root and chain_tip.  The chain_root
         * and chain_tip can be in different branches of the tree, the chain_root can be
         * an ancestor of chain_tip, and chain_tip can be an ancestor of chain_root.
         *
         * @param chain_root the name of the root segment of the chain
         * @param chain_tip the name of the tip segment of the chain
         * @param chain the resulting chain
         *
         * @return success or failure
         */
      bool getChain(const std::string& chain_root, const std::string& chain_tip, Chain& chain)const;


        const SegmentMap& getSegments()const
        {
            return segments;
        }

        virtual ~Tree(){};

    };
}