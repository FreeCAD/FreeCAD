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


DeformExpr::DeformExpr(const std::string& xFunc, const std::string& yFunc, const std::string& zFunc)
{
    symTable.add_constant("pi", std::numbers::pi);
    symTable.add_constant("e", std::numbers::e);

    symTable.add_variable("x", valX);
    symTable.add_variable("y", valY);
    symTable.add_variable("z", valZ);

    xexpr.register_symbol_table(symTable);
    yexpr.register_symbol_table(symTable);
    zexpr.register_symbol_table(symTable);

    setExpr(xFunc, yFunc, zFunc);
}

void DeformExpr::setExpr(const std::string& xFunc, const std::string& yFunc, const std::string& zFunc)
{
    parser_t parser;
    if (!parser.compile(xFunc, xexpr)) {
        throw Base::ValueError("Unable to parse the X function: " + parser.error());
    }
    if (!parser.compile(yFunc, yexpr)) {
        throw Base::ValueError("Unable to parse the Y function: " + parser.error());
    }
    if (!parser.compile(zFunc, zexpr)) {
        throw Base::ValueError("Unable to parse the Z function: " + parser.error());
    }
}

void DeformExpr::setValues(const double vx, const double vy, const double vz)
{
    // symTable.get_variable("x")->ref() = vx;
    // symTable.get_variable("y")->ref() = vy;
    // symTable.get_variable("z")->ref() = vz;
    valX = vx;
    valY = vy;
    valZ = vz;
}

double DeformExpr::x(const double vx, const double vy, const double vz)
{
    setValues(vx, vy, vz);
    return xexpr.value();
}

double DeformExpr::y(const double vx, const double vy, const double vz)
{
    setValues(vx, vy, vz);
    return yexpr.value();
}

double DeformExpr::z(const double vx, const double vy, const double vz)
{
    setValues(vx, vy, vz);
    return zexpr.value();
}

PROPERTY_SOURCE(Part::Flex, Part::Feature)

App::PropertyIntegerConstraint::Constraints Flex::sampleRange = {10, 100, 1};
const char* Flex::ModeEnums[] = {"Bend", "Twist", "UserDefined", nullptr};

namespace
{
FlexMode getMode(const char* strMode)
{
    if (strcmp(strMode, "Bend") == 0) {
        return FlexMode::Bend;
    }
    if (strcmp(strMode, "Twist") == 0) {
        return FlexMode::Twist;
    }
    return FlexMode::UserDefined;
}
}  // namespace

Flex::Flex()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Flex", App::Prop_None, "Shape to deform");
    ADD_PROPERTY_TYPE(Samples, (10), "Flex", App::Prop_None, "Samples count for geometry approximation");
    Samples.setConstraints(&sampleRange);
    ADD_PROPERTY_TYPE(Mode, (1), "Flex", App::Prop_None, "Mode");
    ADD_PROPERTY_TYPE(Origin, (Base::Vector3d(0., 0., 0.)), "Flex", App::Prop_None, "Mode");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1., 0., 0.)), "Flex", App::Prop_None, "Mode");

    ADD_PROPERTY_TYPE(Pitch, (10.0), "Flex", App::Prop_None, "Pitch for the twist deformation");
    ADD_PROPERTY_TYPE(Curve, (nullptr), "Flex", App::Prop_None, "Cruve for the bend deformation");
    ADD_PROPERTY_TYPE(Factor, (1.0), "Flex", App::Prop_None, "Factor for the inflate deformation");
    Mode.setEnums(ModeEnums);

    ADD_PROPERTY_TYPE(xFunc, ("x"), "Flex", App::Prop_None, "Deform function for x result component");
    ADD_PROPERTY_TYPE(yFunc, ("y"), "Flex", App::Prop_None, "Deform function for y result component");
    ADD_PROPERTY_TYPE(zFunc, ("z"), "Flex", App::Prop_None, "Deform function for z result component");
    // initialize hidden statuses
    onChanged(&Mode);
}


short Flex::mustExecute() const
{
    if (Base.isTouched() || Pitch.isTouched() || Samples.isTouched()) {
        return 1;
    }
    return 0;
}

void Flex::onChanged(const App::Property* prop)
{
    constexpr auto visible = [](App::Property* prop, bool show) {
        prop->setStatus(App::Property::Hidden, !show);
    };

    if (prop == &Mode) {
        // hide all specific properties and show the needed below
        visible(&Curve, false);
        visible(&Pitch, false);
        visible(&Factor, false);
        visible(&xFunc, false);
        visible(&yFunc, false);
        visible(&zFunc, false);

        switch (getMode(Mode.getValueAsString())) {
            case FlexMode::Twist:
                visible(&Pitch, true);
                break;
            case FlexMode::Bend:
                visible(&Curve, true);
                visible(&Factor, true);
                break;
            case FlexMode::UserDefined:
                visible(&xFunc, true);
                visible(&yFunc, true);
                visible(&zFunc, true);
                break;
        }
    }

    Part::Feature::onChanged(prop);
}

bool Flex::fetchCurveLink(const App::PropertyLinkSub& curveLink, BRepAdaptor_Curve& curve) const
{
    if (!curveLink.getValue()) {
        throw Base::ValueError("No curve selected");
    }

    auto linked = curveLink.getValue();

    TopoDS_Shape crvEdge;
    if (!curveLink.getSubValues().empty() && !curveLink.getSubValues()[0].empty()) {
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
    auto strMode = Mode.getValueAsString();
    result.mode = getMode(strMode);
    switch (result.mode) {
        case FlexMode::Bend:
            result.factor = Factor.getValue();
            fetchCurveLink(Curve, result.curve);
            break;
        case FlexMode::Twist:
            result.pitch = Pitch.getValue();
            break;
        default:
            result.funcExpr->setExpr(xFunc.getStrValue(), yFunc.getStrValue(), zFunc.getStrValue());
    }

    auto orig = Origin.getValue();
    auto dir = Direction.getValue();
    result.coord = gp_Ax3(gp_Pnt(orig.x, orig.y, orig.z), gp_Dir(dir.x, dir.y, dir.z));

    return result;
}

TopoShape Flex::FlexShape(const TopoShape& source, Flex::FlexParameters& params)
{
    TopoShape result(source);
    switch (params.mode) {
        case FlexMode::Bend:
            result.setShape(bend(source, params));
            break;
        case FlexMode::Twist:
            result.setShape(twist(source, params));
            break;
        case FlexMode::UserDefined:
            result.setShape(userDeform(source, params));
            break;
        default:
            break;
    }
    return result;
}

TopoShape Flex::bend(const TopoShape& source, const Flex::FlexParameters& params)
{
    auto shape = source.getShape();

    auto curve = params.curve;
    auto factor = params.factor;
    auto func = [curve, factor](gp_Pnt pt) {
        return Deformation::bendXAlongCurve(pt, curve, factor);
    };

    try {
        return {Deformation::deform(shape, func, params.samples)};
    }
    catch (Base::Exception& e) {
        throw Base::RuntimeError("FeatureFlex failed on bend\n" + e.getMessage());
    }
}

TopoShape Flex::twist(const TopoShape& source, const Flex::FlexParameters& params)
{
    const double pitch = params.pitch;
    const TopoDS_Shape& shape = source.getShape();
    const auto origin = params.coord.Location();
    const auto axis = params.coord.Direction();

    auto func = [pitch, origin, axis](gp_Pnt pt) {
        return Deformation::twist(pt, pitch, axis, origin);
    };

    try {
        return {Deformation::deform(shape, func, params.samples)};
    }
    catch (Base::Exception& e) {
        throw Base::RuntimeError("FeatureFlex failed on twist\n" + e.getMessage());
    }
}

TopoShape Flex::userDeform(const TopoShape& source, Flex::FlexParameters& params)
{
    const TopoDS_Shape& shape = source.getShape();

    auto* expr = params.funcExpr;

    auto func = [&expr](gp_Pnt pt) {
        gp_Pnt result;
        result.SetX(expr->x(pt.X(), pt.Y(), pt.Z()));
        result.SetY(expr->y(pt.X(), pt.Y(), pt.Z()));
        result.SetZ(expr->z(pt.X(), pt.Y(), pt.Z()));
        return result;
    };

    try {
        auto result = Deformation::deform(shape, func, params.samples);
        return result;
    }
    catch (Base::Exception& e) {
        throw Base::RuntimeError("FeatureFlex failed on userDeform\n" + e.getMessage());
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
        delete params.funcExpr;

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
