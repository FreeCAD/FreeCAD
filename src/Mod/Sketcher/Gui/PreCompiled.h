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

// Importing of App classes
#ifdef FC_OS_WIN32
# define SketcherExport    __declspec(dllimport)
# define PartExport        __declspec(dllimport)
# define PartGuiExport     __declspec(dllimport)
# define SketcherGuiExport __declspec(dllexport)
#else // for Linux
# define SketcherExport
# define PartExport
# define PartAppExport
# define PartGuiExport
# define SketcherGuiExport
#endif


#ifdef _MSC_VER
#   pragma warning(disable : 4005)
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <cassert>
#include <cmath>

#include <stdlib.h>

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

// Boost
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#ifdef FC_OS_WIN32
# define NOMINMAX
# include <windows.h>
#endif

// OCC
#include <TopoDS_Shape.hxx>
#include <GC_MakeEllipse.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

// Qt Toolkit
#ifndef __Qt4All__
# include <Gui/Qt4All.h>
#endif

# include <QMessageBox>
#include <qdebug.h>
#include <QString>

// all of Inventor
#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#include <Inventor/sensors/SoSensor.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbTime.h>

// Python
#include <Python.h>


#endif //_PreComp_

#endif // __PRECOMPILED_GUI__
