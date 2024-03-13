/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#ifndef PATH_PRECOMPILED_H
#define PATH_PRECOMPILED_H

#include <FCConfig.h>

// Exporting of App classes
#ifdef FC_OS_WIN32
# define PathExport  __declspec(dllexport)
//# define RobotExport __declspec(dllexport) uncomment this to use KDL
# define PartExport __declspec(dllimport)
#else // for Linux
# define PathExport
//# define RobotExport uncomment this to use KDL
# define PartExport
#endif

#ifdef _MSC_VER
#pragma warning( disable : 5208 )
#endif

#ifdef _PreComp_

// standard
#include <cinttypes>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Boost
#include <boost/geometry.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/range/adaptor/transformed.hpp>

//OCC
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepLib.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepLib_MakeFace.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <Geom_Ellipse.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Wire.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#endif // _PreComp_
#endif
