/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"

#include <App/PropertyContainer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Base/Tools.h>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <gp_Circ.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

#include "MeasureDistance.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureDistance, Measure::MeasureBase)


MeasureDistance::MeasureDistance()
{
    ADD_PROPERTY_TYPE(Element1,
                      (nullptr),
                      "Measurement",
                      App::Prop_None,
                      "First element of the measurement");
    Element1.setScope(App::LinkScope::Global);
    Element1.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Element2,
                      (nullptr),
                      "Measurement",
                      App::Prop_None,
                      "Second element of the measurement");
    Element2.setScope(App::LinkScope::Global);
    Element2.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Distance,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance between the two elements");
    Distance.setUnit(Base::Unit::Length);

    ADD_PROPERTY_TYPE(DistanceX,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in X direction");
    DistanceX.setUnit(Base::Unit::Length);
    ADD_PROPERTY_TYPE(DistanceY,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in Y direction");
    DistanceY.setUnit(Base::Unit::Length);
    ADD_PROPERTY_TYPE(DistanceZ,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in Z direction");
    DistanceZ.setUnit(Base::Unit::Length);

    ADD_PROPERTY_TYPE(Position1,
                      (Base::Vector3d(0.0, 0.0, 0.0)),
                      "Measurement",
                      App::Prop_Hidden,
                      "Position1");
    ADD_PROPERTY_TYPE(Position2,
                      (Base::Vector3d(0.0, 1.0, 0.0)),
                      "Measurement",
                      App::Prop_Hidden,
                      "Position2");
}

MeasureDistance::~MeasureDistance() = default;


bool MeasureDistance::isValidSelection(const App::MeasureSelection& selection)
{

    if (selection.size() != 2) {
        return false;
    }

    for (auto element : selection) {
        auto type = App::MeasureManager::getMeasureElementType(element);

        if (type == App::MeasureElementType::INVALID) {
            return false;
        }

        if (type != App::MeasureElementType::POINT && type != App::MeasureElementType::LINE
            && type != App::MeasureElementType::LINESEGMENT
            && type != App::MeasureElementType::CIRCLE && type != App::MeasureElementType::ARC
            && type != App::MeasureElementType::CURVE && type != App::MeasureElementType::PLANE
            && type != App::MeasureElementType::CYLINDER) {
            return false;
        }
    }
    return true;
}

bool MeasureDistance::isPrioritizedSelection(const App::MeasureSelection& selection)
{

    (void)selection;

    // TODO: Check if Elements ar parallel
    // if (selection.size() == 2) {
    //     return true;
    // }

    return false;
}


void MeasureDistance::parseSelection(const App::MeasureSelection& selection)
{

    assert(selection.size() >= 2);

    auto element1 = selection.at(0);
    auto objT1 = element1.object;
    App::DocumentObject* ob1 = objT1.getObject();
    const std::vector<std::string> elems1 = {objT1.getSubName()};
    Element1.setValue(ob1, elems1);

    auto element2 = selection.at(1);
    auto objT2 = element2.object;
    App::DocumentObject* ob2 = objT2.getObject();
    const std::vector<std::string> elems2 = {objT2.getSubName()};
    Element2.setValue(ob2, elems2);
}


bool MeasureDistance::getShape(App::PropertyLinkSub* prop, TopoDS_Shape& rShape)
{

    App::DocumentObject* ob = prop->getValue();
    std::vector<std::string> subs = prop->getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return false;
    }

    std::string subName = subs.at(0);
    App::SubObjectT subject {ob, subName.c_str()};

    auto info = getMeasureInfo(subject);

    if (!info || !info->valid) {
        return false;
    }
    auto distanceInfo = std::dynamic_pointer_cast<Part::MeasureDistanceInfo>(info);

    rShape = distanceInfo->getShape();
    return true;
}

bool MeasureDistance::distanceCircleCircle(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    auto circle1 = asCircle(shape1);
    auto circle2 = asCircle(shape2);
    if (!circle1.IsNull() && !circle2.IsNull()) {
        const gp_Pnt p1 = circle1->Location();
        const gp_Pnt p2 = circle2->Location();
        setValues(p1, p2);
        return true;
    }

    return false;
}

void MeasureDistance::distanceGeneric(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    // Calculate the extrema
    BRepExtrema_DistShapeShape measure(shape1, shape2);
    if (!measure.IsDone() || measure.NbSolution() < 1) {
        throw Base::RuntimeError("Could not get extrema");
    }

    const gp_Pnt p1 = measure.PointOnShape1(1);
    const gp_Pnt p2 = measure.PointOnShape2(1);
    setValues(p1, p2);
}

void MeasureDistance::setValues(const gp_Pnt& p1, const gp_Pnt& p2)
{
    Position1.setValue(p1.X(), p1.Y(), p1.Z());
    Position2.setValue(p2.X(), p2.Y(), p2.Z());

    const gp_Pnt delta = p2.XYZ() - p1.XYZ();
    Distance.setValue(p1.Distance(p2));
    DistanceX.setValue(std::fabs(delta.X()));
    DistanceY.setValue(std::fabs(delta.Y()));
    DistanceZ.setValue(std::fabs(delta.Z()));
}

App::DocumentObjectExecReturn* MeasureDistance::execute()
{

    App::DocumentObject* ob1 = Element1.getValue();
    std::vector<std::string> subs1 = Element1.getSubValues();

    App::DocumentObject* ob2 = Element2.getValue();
    std::vector<std::string> subs2 = Element2.getSubValues();

    if (!ob1 || !ob1->isValid() || !ob2 || !ob2->isValid()) {
        return new App::DocumentObjectExecReturn("Submitted object(s) is not valid");
    }

    if (subs1.empty() || subs2.empty()) {
        return new App::DocumentObjectExecReturn("No geometry element picked");
    }

    // Get both shapes from geometry handler
    TopoDS_Shape shape1;
    if (!this->getShape(&Element1, shape1)) {
        return new App::DocumentObjectExecReturn("Could not get shape");
    }

    TopoDS_Shape shape2;
    if (!this->getShape(&Element2, shape2)) {
        return new App::DocumentObjectExecReturn("Could not get shape");
    }

    if (distanceCircleCircle(shape1, shape2)) {
        return DocumentObject::StdReturn;
    }

    distanceGeneric(shape1, shape2);
    return DocumentObject::StdReturn;
}

void MeasureDistance::onChanged(const App::Property* prop)
{

    if (prop == &Element1 || prop == &Element2) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn* ret = recompute();
            delete ret;
        }
    }
    DocumentObject::onChanged(prop);
}

Handle(Geom_Circle) MeasureDistance::asCircle(const TopoDS_Shape& shape) const
{
    if (shape.IsNull()) {
        return {};
    }

    if (shape.ShapeType() == TopAbs_EDGE) {
        return asCircle(TopoDS::Edge(shape));
    }

    if (shape.ShapeType() == TopAbs_WIRE) {
        return asCircle(TopoDS::Wire(shape));
    }

    return {};
}

Handle(Geom_Circle) MeasureDistance::asCircle(const TopoDS_Edge& edge) const
{
    Handle(Geom_Circle) circle;
    BRepAdaptor_Curve adapt(edge);
    if (adapt.GetType() == GeomAbs_Circle) {
        circle = new Geom_Circle(adapt.Circle());
    }

    return circle;
}

Handle(Geom_Circle) MeasureDistance::asCircle(const TopoDS_Wire& wire) const
{
    Handle(Geom_Circle) circle;
    BRepAdaptor_CompCurve adapt(wire);
    if (adapt.GetType() == GeomAbs_Circle) {
        circle = new Geom_Circle(adapt.Circle());
    }

    return circle;
}

//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureDistance::getSubject() const
{
    return {Element1.getValue()};
}


PROPERTY_SOURCE(Measure::MeasureDistanceDetached, Measure::MeasureBase)

MeasureDistanceDetached::MeasureDistanceDetached()
{
    ADD_PROPERTY_TYPE(Distance,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance between the two elements");
    Distance.setUnit(Base::Unit::Length);

    ADD_PROPERTY_TYPE(DistanceX,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in X direction");
    DistanceX.setUnit(Base::Unit::Length);
    ADD_PROPERTY_TYPE(DistanceY,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in Y direction");
    DistanceY.setUnit(Base::Unit::Length);
    ADD_PROPERTY_TYPE(DistanceZ,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Distance in Z direction");
    DistanceZ.setUnit(Base::Unit::Length);

    ADD_PROPERTY_TYPE(Position1,
                      (Base::Vector3d(0.0, 0.0, 0.0)),
                      "Measurement",
                      App::Prop_None,
                      "Position1");
    ADD_PROPERTY_TYPE(Position2,
                      (Base::Vector3d(0.0, 1.0, 0.0)),
                      "Measurement",
                      App::Prop_None,
                      "Position2");
}

MeasureDistanceDetached::~MeasureDistanceDetached() = default;


bool MeasureDistanceDetached::isValidSelection(const App::MeasureSelection& selection)
{
    return selection.size() == 2;
}

void MeasureDistanceDetached::parseSelection(const App::MeasureSelection& selection)
{
    auto sel1 = selection.at(0);
    auto sel2 = selection.at(1);

    Position1.setValue(sel1.pickedPoint);
    Position2.setValue(sel2.pickedPoint);
}


App::DocumentObjectExecReturn* MeasureDistanceDetached::execute()
{
    recalculateDistance();
    return DocumentObject::StdReturn;
}

void MeasureDistanceDetached::recalculateDistance()
{
    auto delta = Position1.getValue() - Position2.getValue();
    Distance.setValue(delta.Length());
    DistanceX.setValue(fabs(delta.x));
    DistanceY.setValue(fabs(delta.y));
    DistanceZ.setValue(fabs(delta.z));
}

void MeasureDistanceDetached::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Position1 || prop == &Position2) {
        recalculateDistance();
    }

    MeasureBase::onChanged(prop);
}


std::vector<App::DocumentObject*> MeasureDistanceDetached::getSubject() const
{
    return {};
}


Base::Type MeasureDistanceType::getClassTypeId()
{
    return Base::Type::badType();
}

Base::Type MeasureDistanceType::getTypeId() const
{
    return Base::Type::badType();
}

void MeasureDistanceType::init()
{
    initSubclass(MeasureDistanceType::classTypeId,
                 "App::MeasureDistance",
                 "App::DocumentObject",
                 &(MeasureDistanceType::create));
}

void* MeasureDistanceType::create()
{
    return new MeasureDistanceDetached();
}

Base::Type MeasureDistanceType::classTypeId = Base::Type::badType();


// Migrate old MeasureDistance Type
void MeasureDistanceDetached::handleChangedPropertyName(Base::XMLReader& reader,
                                                        const char* TypeName,
                                                        const char* PropName)
{
    if (strcmp(PropName, "P1") == 0 && strcmp(TypeName, "App::PropertyVector") == 0) {
        Position1.Restore(reader);
    }
    else if (strcmp(PropName, "P2") == 0 && strcmp(TypeName, "App::PropertyVector") == 0) {
        Position2.Restore(reader);
    }
}
