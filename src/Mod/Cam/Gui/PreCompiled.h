/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
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

#undef QT_NO_CAST_FROM_ASCII

// Importing of App classes
#ifdef FC_OS_WIN32
# define MeshExport    __declspec(dllimport)
# define PartExport    __declspec(dllimport)
# define PartGuiExport __declspec(dllimport)
# define CamExport     __declspec(dllimport)
# define CamGuiExport  __declspec(dllexport)
#else // for Linux
# define MeshExport
# define PartExport
# define PartGuiExport
# define CamExport
# define CamGuiExport
#endif

#ifdef _MSC_VER
# pragma warning(disable : 4290)
#endif

#ifdef _PreComp_
// standard
#include <iostream>
//#include <stdio.h>
#include <assert.h>
//#include <io.h>
//#include <fcntl.h>
//#include <ctype.h>

// STL
#include <vector>
#include <list>
#include <map>
#include <string>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <bitset>

#include <Python.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// Xerces
#include <xercesc/util/XercesDefs.hpp>

// OpenCasCade Base
#include <Standard_Failure.hxx>

// OpenCascade View
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <BRepBndLib.hxx>
#include <gp_Pnt.hxx>
#include <gp_Sphere.hxx>
#include <gp_Trsf.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <BRep_Tool.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Bnd_Box.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLProp_SLProps.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <TopExp.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <Poly_Polygon3D.hxx>
#include <BRepMesh.hxx>



// Qt Toolkit
#ifndef __Qt4All__
# include <Gui/Qt4All.h>
#endif

// Inventor
#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#include <float.h>


#endif
#endif
