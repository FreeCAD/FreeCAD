// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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


#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <Base/Exception.h>

#include "FeatureFlex.h"
#include "Deformation.h"

#include <BRepAdaptor_Curve.hxx>
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

using namespace Part;

PROPERTY_SOURCE(Part::Flex, Part::Feature)

App::PropertyIntegerConstraint::Constraints Flex::sampleRange = {10, 100, 1};

Flex::Flex()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Flex", App::Prop_None, "Shape to deform");
    ADD_PROPERTY_TYPE(Pitch, (10.0), "Flex", App::Prop_None, "Pitch for the twist deformation");
    ADD_PROPERTY_TYPE(Samples, (10), "Flex", App::Prop_None, "Samples count for geometry approximation");
    Samples.setConstraints(&sampleRange);
}


short Flex::mustExecute() const
{
    if (Base.isTouched() || Pitch.isTouched() || Samples.isTouched()) {
        return 1;
    }
    return 0;
}

Flex::FlexParameters Flex::computeFinalParameters() const
{
    Flex::FlexParameters result;
    result.pitch = Pitch.getValue();
    result.samples = Samples.getValue();

    return result;
}

TopoShape Flex::FlexShape(const TopoShape& source, const Flex::FlexParameters& params)
{
    TopoShape result;

    return twist(source, params);
}


TopoShape Flex::twist(const TopoShape& source, const Flex::FlexParameters& params)
{
    double pitch = params.pitch;
    const TopoDS_Shape shape = source.getShape();

    auto func = [pitch](gp_Pnt pt) {
        return Deformation::twistAlongX(pt, pitch);
    };

    TopoShape transTopo;
    try {
        auto result = Deformation::deform(shape, func, params.samples);
        transTopo.setShape(result);
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on twist\n");
        return transTopo;
    }
    return transTopo;
}

App::DocumentObjectExecReturn* Flex::execute()
{
    //    Base::Console().message("FS::execute()\n");
    App::DocumentObject* link = Base.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }

    try {
        Flex::FlexParameters params = computeFinalParameters();
        TopoShape result = FlexShape(
            Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform),
            params
        );
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
