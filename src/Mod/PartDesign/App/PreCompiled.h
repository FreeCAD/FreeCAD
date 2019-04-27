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
# define PartDesignExport __declspec(dllexport)
# define PartExport  __declspec(dllimport)
# define MeshExport     __declspec(dllimport)
#else // for Linux
# define PartDesignExport
# define PartExport
# define MeshExport
#endif

#ifdef _MSC_VER
// disable warning triggered by use of Part::FaceMaker
// see forum thread "Warning C4275 non-dll class used as base for dll class"
// http://forum.freecadweb.org/viewtopic.php?f=10&t=17542
#   pragma warning( disable : 4275)
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

// OpenCasCade =====================================================================================
#include <Mod/Part/App/OpenCascadeAll.h>

// Apart from the Part OpenCascadeAll, I need:
# include <GProp_GProps.hxx>
# include <BRepGProp.hxx>
# include <BRepProj_Projection.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepAlgo.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <BRepOffsetAPI_DraftAngle.hxx>
# include <GeomAPI_IntSS.hxx>


#include <Python.h>

#endif // _PreComp_
#endif

