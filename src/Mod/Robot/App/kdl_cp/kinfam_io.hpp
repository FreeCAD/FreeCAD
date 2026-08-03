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

#include <iostream>
#include <fstream>

#include "joint.hpp"
#include "segment.hpp"
#include "chain.hpp"
#include "jntarray.hpp"
#include "jacobian.hpp"
#include "tree.hpp"
#include "jntspaceinertiamatrix.hpp"

namespace KDL {
std::ostream& operator <<(std::ostream& os, const Joint& joint);
std::istream& operator >>(std::istream& is, Joint& joint);
std::ostream& operator <<(std::ostream& os, const Segment& segment);
std::istream& operator >>(std::istream& is, Segment& segment);
std::ostream& operator <<(std::ostream& os, const Chain& chain);
std::istream& operator >>(std::istream& is, Chain& chain);

std::ostream& operator <<(std::ostream& os, const Tree& tree);
std::istream& operator >>(std::istream& is, Tree& tree);

std::ostream& operator <<(std::ostream& os, SegmentMap::const_iterator it);

std::ostream& operator <<(std::ostream& os, const JntArray& array);
std::istream& operator >>(std::istream& is, JntArray& array);
std::ostream& operator <<(std::ostream& os, const Jacobian& jac);
std::istream& operator >>(std::istream& is, Jacobian& jac);
std::ostream& operator <<(std::ostream& os, const JntSpaceInertiaMatrix& jntspaceinertiamatrix);
std::istream& operator >>(std::istream& is, JntSpaceInertiaMatrix& jntspaceinertiamatrix);

    /*
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
	os << "[";
	for (unsigned int i = 0; i < vec.size(); i++)
		os << vec[i] << " ";
	os << "]";
	return os;
}
;

template<typename T>
std::istream& operator >>(std::istream& is, std::vector<T>& vec) {
	return is;
}
;
    */
}