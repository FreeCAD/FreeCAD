// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#pragma once

#include <FCConfig.h>

// STL
#include <memory>
#include <sstream>

// kdl_cp
#include "kdl_cp/chain.hpp"
#include "kdl_cp/chainfksolverpos_recursive.hpp"
#include "kdl_cp/chainiksolverpos_nr.hpp"
#include "kdl_cp/chainiksolverpos_nr_jl.hpp"
#include "kdl_cp/chainiksolvervel_pinv.hpp"
#include "kdl_cp/frames_io.hpp"
#include "kdl_cp/path_line.hpp"
#include "kdl_cp/path_roundedcomposite.hpp"
#include "kdl_cp/rotational_interpolation_sa.hpp"
#include "kdl_cp/trajectory_composite.hpp"
#include "kdl_cp/trajectory_segment.hpp"
#include "kdl_cp/utilities/error.h"
#include "kdl_cp/velocityprofile_trap.hpp"

// OCC
#include <BRepAdaptor_Curve.hxx>
#include <CPnts_AbscissaPoint.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
