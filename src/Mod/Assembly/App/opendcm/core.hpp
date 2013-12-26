/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_CORE_H
#define DCM_CORE_H

#ifdef _WIN32
	//warning about to long decoraded names, won't affect the code correctness
	#pragma warning( disable : 4503 )
	//warning about changed pod initalising behaviour (boost blank in variant)
	#pragma warning( disable : 4345 )
	//warning about multiple assignemnt operators in Equation
	#pragma warning( disable : 4522 )

	//disable boost concept checks, as some of them have alignment problems which bring msvc to an error
	//(for example DFSvisitor check in boost::graph::depht_first_search)
	//this has no runtime effect as these are only compile time checks
	#include <boost/concept/assert.hpp>
	#undef BOOST_CONCEPT_ASSERT
	#define BOOST_CONCEPT_ASSERT(Model)
	#include <boost/concept_check.hpp>

#endif

#include "core/defines.hpp"
#include "core/geometry.hpp"
#include "core/kernel.hpp"
#include "core/system.hpp"


#ifdef DCM_EXTERNAL_CORE

#define DCM_EXTERNAL_CORE_INCLUDE_01 "opendcm/core/imp/system_imp.hpp"
#define DCM_EXTERNAL_CORE_01( Sys )\
    template class dcm::System<Sys::Kernel, Sys::Module1, Sys::Module2, Sys::Module3>; \
    template struct dcm::Equation<dcm::Distance, mpl::vector2<double, dcm::SolutionSpace>, 1>; \
    template struct dcm::Equation<dcm::Orientation, dcm::Direction, 2, dcm::rotation>; \
    template struct dcm::Equation<dcm::Angle, mpl::vector2<double, dcm::SolutionSpace>, 3, dcm::rotation>; 

#endif //external

#endif //DCM_CORE_H

