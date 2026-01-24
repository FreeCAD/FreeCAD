// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * TreeJntToJacSolver.hpp
 *
 *  Created on: Nov 27, 2008
 *      Author: rubensmits
 */

#pragma once

#include "tree.hpp"
#include "jacobian.hpp"
#include "jntarray.hpp"

namespace KDL {

class TreeJntToJacSolver {
public:
    explicit TreeJntToJacSolver(const Tree& tree);

    virtual ~TreeJntToJacSolver();

    /*
     * Calculate the jacobian for a part of the tree: from a certain segment, given by segmentname to the root.
     * The resulting jacobian is expressed in the baseframe of the tree ("root"), the reference point is in the end-segment
     */

    int JntToJac(const JntArray& q_in, Jacobian& jac,
            const std::string& segmentname);

private:
    KDL::Tree tree;

};

}//End of namespace