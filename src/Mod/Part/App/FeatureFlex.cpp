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

#include <Base/Exception.h>

#include "FeatureFlex.h"
#include "Deformation.h"

using namespace Part;

PROPERTY_SOURCE(Part::Flex, Part::Feature)

App::PropertyIntegerConstraint::Constraints Flex::sampleRange = {10, 100, 1};
const char* Flex::ModeEnums[] = {"Bend", "Twist", "Inflate", nullptr};

Flex::Flex()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Flex", App::Prop_None, "Shape to deform");
    ADD_PROPERTY_TYPE(Samples, (10), "Flex", App::Prop_None, "Samples count for geometry approximation");
    Samples.setConstraints(&sampleRange);
    ADD_PROPERTY_TYPE(Mode, (FlexMode::Twist), "Flex", App::Prop_None, "Mode");
    ADD_PROPERTY_TYPE(Origin, (Base::Vector3d(0., 0., 0.)), "Flex", App::Prop_None, "Mode");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1., 0., 0.)), "Flex", App::Prop_None, "Mode");

    ADD_PROPERTY_TYPE(Pitch, (10.0), "Flex", App::Prop_None, "Pitch for the twist deformation");
    ADD_PROPERTY_TYPE(Curve, (nullptr), "Flex", App::Prop_None, "Cruve for the bend deformation");
    ADD_PROPERTY_TYPE(Factor, (1.0), "Flex", App::Prop_None, "Factor for the inflate deformation");
    Mode.setEnums(ModeEnums);
}


short Flex::mustExecute() const
{
    if (Base.isTouched() || Pitch.isTouched() || Samples.isTouched()) {
        return 1;
    }
    return 0;
}

bool Flex::fetchCurveLink(const App::PropertyLinkSub& curveLink, BRepAdaptor_Curve& curve) const
{
    if (!curveLink.getValue()) {
        return false;
    }

    auto linked = curveLink.getValue();

    TopoDS_Shape crvEdge;
    if (!curveLink.getSubValues().empty() && curveLink.getSubValues()[0].length() > 0) {
        crvEdge = Feature::getTopoShape(
                      linked,
                      ShapeOption::NeedSubElement | ShapeOption::ResolveLink | ShapeOption::Transform,
                      curveLink.getSubValues()[0].c_str()
        )
                      .getShape();
    }
    else {
        crvEdge = Feature::getShape(linked, ShapeOption::ResolveLink | ShapeOption::Transform);
    }

    if (crvEdge.IsNull()) {
        throw Base::ValueError("Curve shape is null");
    }
    if (crvEdge.ShapeType() != TopAbs_EDGE) {
        throw Base::TypeError("Curve shape is not an edge");
    }

    curve = BRepAdaptor_Curve(TopoDS::Edge(crvEdge));
    return true;
}

Flex::FlexParameters Flex::computeFinalParameters() const
{
    Flex::FlexParameters result;
    result.samples = Samples.getValue();
    result.mode = Mode.getValue();

    auto orig = Origin.getValue();
    auto dir = Direction.getValue();
    result.coord = gp_Ax3(gp_Pnt(orig.x, orig.y, orig.z), gp_Dir(dir.x, dir.y, dir.z));

    result.pitch = Pitch.getValue();
    result.factor = Factor.getValue();
    if (result.mode == FlexMode::Bend) {
        fetchCurveLink(Curve, result.curve);
    }

    return result;
}

TopoShape Flex::FlexShape(const TopoShape& source, const Flex::FlexParameters& params)
{
    // move to Origin and align to Direction
    gp_Trsf trsf;
    trsf.SetTransformation(gp_Ax3(gp_Pnt(), gp_Dir(1., 0., 0.), gp_Dir(0., 1., 0.)), params.coord);
    trsf.SetTranslationPart(params.coord.Location().XYZ());
    TopoShape toDeform = source.moved(trsf.Inverted());

    TopoShape result;
    switch (params.mode) {
        case FlexMode::Bend:
            result = bend(toDeform, params);
            break;
        case FlexMode::Twist:
            result = twist(toDeform, params);
            break;
        case FlexMode::Inflate:
            result = inflate(toDeform, params);
            break;
        default:
            return source;
    }

    // move back
    result.move(trsf);
    return result;
}

TopoShape Flex::bend(const TopoShape& source, const Flex::FlexParameters& params)
{
    const TopoDS_Shape& shape = source.getShape();

    auto curve = params.curve;
    auto factor = params.factor;
    auto func = [curve, factor](gp_Pnt pt) {
        return Deformation::bendXAlongCurve(pt, curve, factor);
    };

    try {
        return {Deformation::deform(shape, func, params.samples)};
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on bend\n");
        return {};
    }
}

TopoShape Flex::twist(const TopoShape& source, const Flex::FlexParameters& params)
{
    double pitch = params.pitch;
    const TopoDS_Shape& shape = source.getShape();

    auto func = [pitch](gp_Pnt pt) {
        return Deformation::twistAlongX(pt, pitch);
    };

    try {
        return {Deformation::deform(shape, func, params.samples)};
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on twist\n");
        return {};
    }
}

TopoShape Flex::inflate(const TopoShape& source, const Flex::FlexParameters& params)
{
    const TopoDS_Shape& shape = source.getShape();

    auto identity = [](gp_Pnt pt) {
        return pt;
    };

    try {
        return {Deformation::deform(shape, identity, params.samples)};
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on identity\n");
        return {};
    }
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
        if (result.isNull()) {
            return new App::DocumentObjectExecReturn("Null shape");
        }
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
