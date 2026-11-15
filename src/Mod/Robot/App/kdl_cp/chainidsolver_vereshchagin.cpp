// Copyright  (C)  2009  Ruben Smits <ruben dot smits at intermodalics dot eu>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at intermodalics dot eu>
// Author: Herman Bruyninckx
// Author: Azamat Shakhimardanov
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

#include "chainidsolver_vereshchagin.hpp"

#include <iostream>

namespace KDL
{

ChainIdSolver_Vereshchagin::ChainIdSolver_Vereshchagin(const Chain& chain_, const Twist &root_acc, const unsigned int nc_) : ChainHdSolver_Vereshchagin(chain_, root_acc, nc_)
{
    std::cout << "ChainIdSolver_Vereshchagin is renamed to ChainHdSolver_Vereshchagin in version 1.5. The old name will be dropped in version 1.6" << std::endl;
}

}
