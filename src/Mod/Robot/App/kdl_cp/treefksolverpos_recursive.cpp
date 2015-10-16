// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008 Julia Jesse

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

#include "treefksolverpos_recursive.hpp"
#include <iostream>

namespace KDL {

    TreeFkSolverPos_recursive::TreeFkSolverPos_recursive(const Tree& _tree):
        tree(_tree)
    {
    }

    int TreeFkSolverPos_recursive::JntToCart(const JntArray& q_in, Frame& p_out, std::string segmentName)
    {      
		SegmentMap::const_iterator it = tree.getSegment(segmentName); 
       
        
        if(q_in.rows() != tree.getNrOfJoints())
    	    	return -1;
        else if(it == tree.getSegments().end()) //if the segment name is not found
         	return -2;
        else{
			p_out = recursiveFk(q_in, it);
        	return 0;        	
        }
    }

	Frame TreeFkSolverPos_recursive::recursiveFk(const JntArray& q_in, const SegmentMap::const_iterator& it)
	{
		//gets the frame for the current element (segment)
        const TreeElementType& currentElement = it->second;
        Frame currentFrame = GetTreeElementSegment(currentElement).pose(q_in(GetTreeElementQNr(currentElement)));

		SegmentMap::const_iterator rootIterator = tree.getRootSegment();
		if(it == rootIterator){
			return currentFrame;	
		}
		else{
            SegmentMap::const_iterator parentIt = GetTreeElementParent(currentElement);
			return recursiveFk(q_in, parentIt) * currentFrame;
		}
	}

    TreeFkSolverPos_recursive::~TreeFkSolverPos_recursive()
    {
    }


}
