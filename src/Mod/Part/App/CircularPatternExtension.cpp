// SPDX-License-Identifier: LGPL-2.1-or-later

#include "CircularPatternExtension.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <BRepAdaptor_Curve.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <App/Datums.h>
#include <App/DocumentObject.h>
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "Part2DObject.h"
#include "PartFeature.h"
#include "TopoShape.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::CircularPatternExtension, App::DocumentObjectExtension)

const App::PropertyIntegerConstraint::Constraints CircularPatternExtension::intNumberCircles = {
    2,
    std::numeric_limits<int>::max(),
    1
};
const App::PropertyIntegerConstraint::Constraints CircularPatternExtension::intSymmetry = {
    1,
    std::numeric_limits<int>::max(),
    1
};

CircularPatternExtension::CircularPatternExtension()
{
    initExtensionType(CircularPatternExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(
        Axis,
        (nullptr),
        "CircularPattern",
        App::Prop_None,
        "The axis and center of the circular pattern."
    );
    EXTENSION_ADD_PROPERTY_TYPE(RadialDistance,
                                (50.0),
                                "CircularPattern",
                                App::Prop_None,
                                "Distance between consecutive concentric circles.");
    EXTENSION_ADD_PROPERTY_TYPE(
        TangentialDistance,
        (25.0),
        "CircularPattern",
        App::Prop_None,
        "Approximate distance between consecutive elements on the same circle."
    );
    EXTENSION_ADD_PROPERTY_TYPE(NumberCircles,
                                (3),
                                "CircularPattern",
                                App::Prop_None,
                                "Number of concentric circles, including the original element.");
    NumberCircles.setConstraints(&intNumberCircles);
    EXTENSION_ADD_PROPERTY_TYPE(
        Symmetry,
        (1),
        "CircularPattern",
        App::Prop_None,
        "Number of symmetry divisions used to round each circle's element count."
    );
    Symmetry.setConstraints(&intSymmetry);
}

std::list<gp_Trsf> CircularPatternExtension::calculateTransformations() const
{
    const double radialDistance = RadialDistance.getValue();
    const double tangentialDistance = TangentialDistance.getValue();
    const int circleCount = NumberCircles.getValue();
    const int symmetry = std::max(1, static_cast<int>(Symmetry.getValue()));

    if (radialDistance <= Precision::Confusion()) {
        throw Base::ValueError("Radial distance must be greater than zero");
    }
    if (tangentialDistance <= Precision::Confusion()) {
        throw Base::ValueError("Tangential distance must be greater than zero");
    }
    if (circleCount < 2) {
        throw Base::ValueError("At least two concentric circles are required");
    }

    const gp_Ax2 axis = getRotation();
    const gp_Pnt center = axis.Location();

    gp_Dir lead(0.0, 1.0, 0.0);
    if (std::abs(axis.Direction().X()) <= Precision::Confusion()
        && std::abs(axis.Direction().Z()) <= Precision::Confusion()) {
        lead = gp_Dir(1.0, 0.0, 0.0);
    }

    gp_Vec radialDirection = gp_Vec(axis.Direction()).Crossed(gp_Vec(lead));
    radialDirection.Normalize();

    std::list<gp_Trsf> transformations {gp_Trsf()};
    const double fullCircle = 2.0 * std::acos(-1.0);
    for (int circle = 1; circle < circleCount; ++circle) {
        const double radius = circle * radialDistance;
        int elementCount =
            static_cast<int>(std::floor(fullCircle * radius / tangentialDistance));
        elementCount = elementCount / symmetry * symmetry;
        if (elementCount == 0) {
            continue;
        }

        gp_Trsf translation;
        translation.SetTranslation(radialDirection * radius);
        const double angularStep = fullCircle / elementCount;

        for (int element = 0; element < elementCount; ++element) {
            gp_Trsf rotation;
            rotation.SetRotation(gp_Ax1(center, axis.Direction()), element * angularStep);
            rotation.Multiply(translation);
            transformations.push_back(rotation);
        }
    }

    return transformations;
}

gp_Ax2 CircularPatternExtension::getRotation() const
{
    App::DocumentObject* reference = Axis.getValue();
    const auto subnames = Axis.getSubValues();
    const std::string subname = subnames.empty() ? std::string() : subnames.front();

    if (!reference) {
        if (subname == "X_Axis") {
            return gp_Ax2(gp_Pnt(), gp_Dir(1.0, 0.0, 0.0));
        }
        if (subname == "Y_Axis") {
            return gp_Ax2(gp_Pnt(), gp_Dir(0.0, 1.0, 0.0));
        }
        return gp_Ax2();
    }

    gp_Pnt base;
    gp_Dir direction;
    if (auto* sketch = freecad_cast<Part::Part2DObject*>(reference)) {
        Base::Axis axis;
        if (subname == "H_Axis") {
            axis = sketch->getAxis(Part::Part2DObject::H_Axis);
        }
        else if (subname == "V_Axis") {
            axis = sketch->getAxis(Part::Part2DObject::V_Axis);
        }
        else {
            axis = sketch->getAxis(Part::Part2DObject::N_Axis);
        }
        axis *= sketch->Placement.getValue();
        base = gp_Pnt(axis.getBase().x, axis.getBase().y, axis.getBase().z);
        direction =
            gp_Dir(axis.getDirection().x, axis.getDirection().y, axis.getDirection().z);
    }
    else if (auto* line = freecad_cast<App::Line*>(reference)) {
        const Base::Vector3d point = line->getBasePoint();
        const Base::Vector3d vector = line->getDirection();
        base = gp_Pnt(point.x, point.y, point.z);
        direction = gp_Dir(vector.x, vector.y, vector.z);
    }
    else if (auto* feature = freecad_cast<Part::Feature*>(reference)) {
        TopoDS_Edge edge = TopoDS::Edge(feature->Shape.getShape().getSubShape(subname.c_str()));
        BRepAdaptor_Curve curve(edge);
        if (curve.GetType() == GeomAbs_Line) {
            base = curve.Line().Location();
            direction = curve.Line().Direction();
        }
        else if (curve.GetType() == GeomAbs_Circle) {
            base = curve.Circle().Location();
            direction = curve.Circle().Axis().Direction();
        }
        else {
            throw Base::TypeError("Rotation edge must be a straight line, circle or arc of circle");
        }
    }
    else {
        throw Base::TypeError("Axis reference must be edge of a feature or datum line");
    }

    Base::Placement placement;
    if (auto* object = getExtendedObject()) {
        if (auto* property =
                freecad_cast<App::PropertyPlacement*>(object->getPropertyByName("Placement"))) {
            placement = property->getValue();
        }
    }
    const Base::Placement inversePlacement = placement.inverse();
    Base::Vector3d rotationAxis;
    double angle;
    inversePlacement.getRotation().getValue(rotationAxis, angle);
    gp_Trsf inverse;
    inverse.SetRotation(
        gp_Ax1(gp_Pnt(), gp_Dir(rotationAxis.x, rotationAxis.y, rotationAxis.z)), angle
    );
    inverse.SetTranslationPart(gp_Vec(inversePlacement.getPosition().x,
                                      inversePlacement.getPosition().y,
                                      inversePlacement.getPosition().z));
    base.Transform(inverse);
    direction.Transform(inverse);
    return gp_Ax2(base, direction);
}

short CircularPatternExtension::extensionMustExecute()
{
    if (Axis.isTouched() || RadialDistance.isTouched() || TangentialDistance.isTouched()
        || NumberCircles.isTouched() || Symmetry.isTouched()) {
        return 1;
    }
    return 0;
}
