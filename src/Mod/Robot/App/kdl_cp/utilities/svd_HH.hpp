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

#ifndef KDL_SVD_HH_HPP
#define KDL_SVD_HH_HPP

#include "../jacobian.hpp"
#include "../jntarray.hpp"
#include <vector>

namespace KDL
{

    class SVD_HH
    {
    public:
        SVD_HH(const Jacobian& jac);
        ~SVD_HH();

        int calculate(const Jacobian& jac,std::vector<JntArray>& U,
                      JntArray& w,std::vector<JntArray>& v,int maxiter);
    private:
        JntArray tmp;
    };
}
#endif


