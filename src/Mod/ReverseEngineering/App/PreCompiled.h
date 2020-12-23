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

// Exporting of App classes
#ifdef FC_OS_WIN32
# define ReenExport     __declspec(dllexport)
# define PartExport     __declspec(dllimport)
# define MeshExport     __declspec(dllimport)
# define PointsExport   __declspec(dllimport)
#else // for Linux
# define ReenExport
# define PartExport
# define MeshExport
# define PointsExport
#endif

#ifdef _MSC_VER
#   pragma warning(disable : 4181)
#   pragma warning(disable : 4267)
#   pragma warning(disable : 4275)
#   pragma warning(disable : 4305)
#   pragma warning(disable : 4522)
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <bitset>

// OpenCasCade
#include <math_Gauss.hxx>
#include <math_Householder.hxx>
#include <math_Matrix.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <Geom_BSplineSurface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Face.hxx>

#include <Python.h>


#endif
#endif // _PreComp_

