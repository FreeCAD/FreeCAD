/***************************************************************************
 *   Copyright (c) Shsi Seger (shaise at gmail) 2017                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <boost/regex.hpp>

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Exception.h>

// KDL stuff - at the moment, not used
//#include "Mod/Robot/App/kdl_cp/path_line.hpp"
//#include "Mod/Robot/App/kdl_cp/path_circle.hpp"
//#include "Mod/Robot/App/kdl_cp/rotational_interpolation_sa.hpp"
//#include "Mod/Robot/App/kdl_cp/utilities/error.h"

#include "PathSimulator.h"

using namespace Path;
using namespace Base;

TYPESYSTEM_SOURCE(Path::PathSimulator , Base::Persistence);





 
