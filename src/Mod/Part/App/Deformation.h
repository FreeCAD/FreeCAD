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
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
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
#include <Precision.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Shell.hxx>
#include <ShapeFix_Solid.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_ShellSewing.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Shell.hxx>
#include <gp_Pnt.hxx>
#include <gp_Quaternion.hxx>

namespace Part
{

class PartExport Deformation
{
public:
    Deformation() = default;

    static gp_Pnt twist(gp_Pnt from, double pitch, gp_Vec direction, gp_Pnt origin);
    static gp_Pnt twistAlongX(gp_Pnt from, double pitch, gp_Pnt origin);
    static gp_Pnt twistAlongY(gp_Pnt from, double pitch, gp_Pnt origin);
    static gp_Pnt twistAlongZ(gp_Pnt from, double pitch, gp_Pnt origin);

    static gp_Pnt bendAlongCurve(
        gp_Pnt from,
        const BRepAdaptor_Curve& curve,
        double factor,
        gp_Vec direction
    );
    static gp_Pnt bendXAlongCurve(gp_Pnt from, const BRepAdaptor_Curve& curve, double factor);

    template<typename T>
    static T deform(const T& shape, const std::function<gp_Pnt(gp_Pnt)>& deformFunction, int samples);
};

}  // namespace Part
