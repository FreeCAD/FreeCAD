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
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_MODULEPARSER_H
#define DCM_MODULEPARSER_H

#define DCM_USE_MODULESTATE

#ifdef _WIN32
	//warning about to long decoraded names, won't affect the code correctness
	#pragma warning( disable : 4503 )
#endif

//use phoenix v3 to allow normal boost phoenix use outside of spirit and not the spirit phoenix v2 version
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include "moduleState/module.hpp"
#include "moduleState/traits.hpp"

#endif //DCM_MODULEPARSER_H

