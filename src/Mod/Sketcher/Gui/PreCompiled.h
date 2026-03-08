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

// standard
#include <cmath>
#include <cstdlib>

// STL
#include <algorithm>
#include <bitset>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <vector>

// Boost
#include <boost/core/ignore_unused.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// OpenCasCade
#include <BRep_Tool.hxx>
#include <GC_MakeEllipse.hxx>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

// Qt
#include <Gui/QtAll.h>

#include <QWidgetAction>

// all of Inventor
#include <Gui/InventorAll.h>
