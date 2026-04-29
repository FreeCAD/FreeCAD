// SPDX-License-Identifier: LGPL-2.1-or-later

/*************************************************************************** \
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            * \
 *                                                                         * \
 *   This file is part of the FreeCAD CAx development system.              * \
 *                                                                         * \
 *   This library is free software; you can redistribute it and/or         * \
 *   modify it under the terms of the GNU Library General Public           * \
 *   License as published by the Free Software Foundation; either          * \
 *   version 2 of the License, or (at your option) any later version.      * \
 *                                                                         * \
 *   This library  is distributed in the hope that it will be useful,      * \
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * \
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * \
 *   GNU Library General Public License for more details.                  * \
 *                                                                         * \
 *   You should have received a copy of the GNU Library General Public     * \
 *   License along with this library; see the file COPYING.LIB. If not,    * \
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         * \
 *   Suite 330, Boston, MA  02111-1307, USA                                * \
 *                                                                         * \
 ***************************************************************************/

#pragma once

#include <Mod/Part/PartGlobal.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomConvert.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Geom_SphericalSurface.hxx>
#include <ShapeAnalysis_CanonicalRecognition.hxx>
#include <ShapeFix_Solid.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_ShellSewing.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Shell.hxx>
#include <gp_Pnt.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

namespace Part
{

class PartExport Canonizer
{
public:
    Canonizer() = default;

    static Handle(Geom_Curve) getSimplestCurve(const TopoDS_Edge& shape, float tol);
    static Handle(Geom_Surface) getSimplestSurface(const TopoDS_Face& shape, float tol);

    /*
     * Return an edge based on a canonical curve if possible
     * else the initial edge
     * Canonical curves are Circle, Ellipse and Line
     */
    static TopoDS_Edge canonize(const TopoDS_Edge& edge, float tol);

    /*
     * Base the edges of the wire on canonical curves if possible
     */
    static TopoDS_Wire canonize(const TopoDS_Wire& wire, float tol);

    /*
     * Return a face based on a canonical surface if possible
     * else on the initial face surface
     * All the return face edges are also based on canonical curves if possible
     * Canonical surfaces are Cone, Cylinder, Plane and Sphere
     */
    static TopoDS_Face canonize(const TopoDS_Face& face, float tol);
};

}  // namespace Part
