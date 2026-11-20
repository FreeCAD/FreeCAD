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

#ifndef KDL_CHAINIDSOLVER_VERESHCHAGIN_HPP
#define KDL_CHAINIDSOLVER_VERESHCHAGIN_HPP

#include "chainhdsolver_vereshchagin.hpp"

namespace KDL
{

class ChainIdSolver_Vereshchagin : public ChainHdSolver_Vereshchagin
{
public:
    ChainIdSolver_Vereshchagin(const Chain& chain, const Twist &root_acc, const unsigned int nc);
};

}

#endif // KDL_CHAINIDSOLVER_VERESHCHAGIN_HPP
