/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_PRECOMPILED_H
#define PARTGUI_PRECOMPILED_H

#include <FCConfig.h>

#include <Mod/Part/PartGlobal.h>

// here get the warnings of too long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
# pragma warning( disable : 4251 )
# pragma warning( disable : 4503 )
# pragma warning( disable : 4786 )  // specifier longer then 255 chars
# pragma warning( disable : 4273 )
#endif

#ifdef FC_OS_WIN32
# ifndef NOMINMAX
# define NOMINMAX
# endif
# include <windows.h>
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <cassert>
#include <cfloat>
# include <cmath>
#include <sstream>

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

// OpenCasCade Base
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>

#if OCC_VERSION_HEX < 0x070400
#include <BRepMesh.hxx>
#endif
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Connect.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>

#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Sphere.hxx>
#include <gp_Trsf.hxx>

#include <gce_ErrorType.hxx>
#include <GC_MakeArcOfCircle.hxx>

#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ElementarySurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomLProp_SLProps.hxx>
#include <GeomLib.hxx>
#include <GeomProjLib.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>

#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Result.hxx>
#include <BRepCheck_ListIteratorOfListOfStatus.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepProj_Projection.hxx>
#include <BRepTools_ShapeSet.hxx>

#include <Bnd_Box.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <Precision.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#include <Interface_Static.hxx>

#include <ShapeAnalysis.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeExtend_Explorer.hxx>
#include <ShapeFix_Edge.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wireframe.hxx>

# include <BOPAlgo_ArgumentAnalyzer.hxx>
# include <BOPAlgo_ListOfCheckResult.hxx>

// Boost
#include <boost_signals2.hpp>
#include <boost/bind/bind.hpp>

// Qt Toolkit
#ifndef __QtAll__
# include <Gui/QtAll.h>
#endif

// GL
// Include glext before InventorAll
# ifdef FC_OS_WIN32
#  include <GL/gl.h>
#  include <GL/glext.h>
# else
#  ifdef FC_OS_MACOSX
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#  else
#   ifndef GL_GLEXT_PROTOTYPES
#    define GL_GLEXT_PROTOTYPES 1
#   endif
#   include <GL/gl.h>
#   include <GL/glext.h>
#  endif //FC_OS_MACOSX
# endif //FC_OS_WIN32
// Should come after glext.h to avoid warnings
# include <Inventor/C/glue/gl.h>

#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoNurbsCurve.h>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/engines/SoConcatenate.h>

// Inventor includes OpenGL
#ifndef __InventorAll__
# include <Gui/InventorAll.h>
#endif

#endif  //_PreComp_

#endif // PARTGUI_PRECOMPILED_H
