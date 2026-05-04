// SPDX-License-Identifier: LGPL-2.1-or-later

/*************************************************************************** \ \
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            * \ \
 *                                                                         * \ \
 *   This file is part of the FreeCAD CAx development system.              * \ \
 *                                                                         * \ \
 *   This library is free software; you can redistribute it and/or         * \ \
 *   modify it under the terms of the GNU Library General Public           * \ \
 *   License as published by the Free Software Foundation; either          * \ \
 *   version 2 of the License, or (at your option) any later version.      * \ \
 *                                                                         * \ \
 *   This library  is distributed in the hope that it will be useful,      * \ \
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * \ \
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * \ \
 *   GNU Library General Public License for more details.                  * \ \
 *                                                                         * \ \
 *   You should have received a copy of the GNU Library General Public     * \ \
 *   License along with this library; see the file COPYING.LIB. If not,    * \ \
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         * \ \
 *   Suite 330, Boston, MA  02111-1307, USA                                * \ \
 *                                                                         * \ \
 ***************************************************************************/

#pragma once

#include <Mod/Part/PartGlobal.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
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

class PartExport Deformation
{
public:
    Deformation() = default;


    static gp_Pnt twistAlongX(gp_Pnt from, double pitch);
    static gp_Pnt bendXAlongCurve(gp_Pnt from, const BRepAdaptor_Curve& curve, double factor);

    static TopoDS_Edge deform(
        const TopoDS_Edge& edge,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
    static TopoDS_Wire deform(
        const TopoDS_Wire& wire,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
    static TopoDS_Face deform(
        const TopoDS_Face& face,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
    static TopoDS_Shell deform(
        const TopoDS_Shell& shell,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
    static TopoDS_Solid deform(
        const TopoDS_Solid& solid,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
    static TopoDS_Compound deform(
        const TopoDS_Compound& solid,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );

    static TopoDS_Shape deform(
        const TopoDS_Shape& shape,
        const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
        int samples
    );
};

}  // namespace Part
