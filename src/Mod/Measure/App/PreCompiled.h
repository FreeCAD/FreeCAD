/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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


#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#include <FCConfig.h>

#ifdef FC_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <bitset>

#include <Mod/Part/App/OpenCascadeAll.h>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <BRepGProp.hxx>

#elif defined(FC_OS_WIN32)
#include <windows.h>
#endif // _PreComp_
#endif
