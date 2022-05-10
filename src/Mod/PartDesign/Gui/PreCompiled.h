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


#ifndef __PRECOMPILED_GUI__
#define __PRECOMPILED_GUI__

#include <FCConfig.h>


#ifdef _MSC_VER
#   pragma warning(disable : 4005)
#endif

#ifdef _PreComp_

// standard
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

// Boost
#include <boost/bind/bind.hpp>

// OCC
#include <Standard_math.hxx>
#include <Standard_Version.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepAdaptor_Surface.hxx>

// STL
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <bitset>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif


// Qt Toolkit
#ifndef __QtAll__
# include <Gui/QtAll.h>
#endif

#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

// Inventor
#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#endif // _PreComp_
#endif // __PRECOMPILED_GUI__
