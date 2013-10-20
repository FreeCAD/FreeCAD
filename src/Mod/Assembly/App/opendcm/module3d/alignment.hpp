/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more detemplate tails.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_ALIGNMENT_HPP
#define DCM_ALIGNMENT_HPP

#include <opendcm/core/equations.hpp>
#include "distance.hpp"

namespace dcm {

struct Alignment : public constraint_sequence< fusion::vector2< Distance, Orientation > > {
  
  using constraint_sequence::operator=;
};

static Alignment alignment;

}//dcm


#endif //DCM_ALIGNMENT_HPP
