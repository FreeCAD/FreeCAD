// SPDX-License-Identifier: LGPL-2.1-or-later

#include "CircularPatternExtension.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include <BRepAdaptor_Curve.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Quaternion.hxx>

#include <App/Datums.h>
#include <App/DocumentObject.h>
#include <Base/Axis.h>
#include <Base/Exception.h>
#include <Base/Placement.h>

#include "Part2DObject.h"
#include "PartFeature.h"
#include "PatternConstants.h"
#include "TopoShape.h"
#include "Tools.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::CircularPatternExtension, App::DocumentObjectExtension)

const App::PropertyIntegerConstraint::Constraints CircularPatternExtension::intNumberCircles
    = {1, std::numeric_limits<int>::max(), 1};
const App::PropertyIntegerConstraint::Constraints CircularPatternExtension::intSymmetry
    = {1, std::numeric_limits<int>::max(), 1};

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
    EXTENSION_ADD_PROPERTY_TYPE(
        RadialDistance,
        (50.0),
        "CircularPattern",
        App::Prop_None,
        "Distance between consecutive concentric circles."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        TangentialDistance,
        (25.0),
        "CircularPattern",
        App::Prop_None,
        "Approximate distance between consecutive elements on the same circle."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        NumberCircles,
        (3),
        "CircularPattern",
        App::Prop_None,
        "Number of concentric circles, including the original element."
    );
    NumberCircles.setConstraints(&intNumberCircles);
    EXTENSION_ADD_PROPERTY_TYPE(
        Symmetry,
        (1),
        "CircularPattern",
        App::Prop_None,
        "Number of symmetry divisions used to round each circle's element count."
    );
    Symmetry.setConstraints(&intSymmetry);
    EXTENSION_ADD_PROPERTY_TYPE(
        StartAtShape,
        (false),
        "CircularPattern",
        App::Prop_None,
        "Place the existing shape in the first circle, else use the axis' origin as center."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        BinormalAxis,
        (nullptr),
        "CircularPattern",
        App::Prop_None,
        "The axis representing the binormal of the first transformation on the first circle."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        FlipBinormalAxis,
        (false),
        "CircularPattern",
        App::Prop_None,
        "Flip the binormal axis' direction."
    );
}

bool CircularPatternExtension::identityTransformSuppressed() const
{
    return false;
}

std::list<gp_Trsf> CircularPatternExtension::calculateTransformations() const
{
    const double radialDistance = RadialDistance.getValue();
    const double tangentialDistance = TangentialDistance.getValue();
    int circleCount = NumberCircles.getValue();
    const int symmetry = std::max(1, static_cast<int>(Symmetry.getValue()));

    if (!std::isfinite(radialDistance) || radialDistance <= Precision::Confusion()) {
        throw Base::ValueError("Radial distance must be greater than zero");
    }
    if (!std::isfinite(tangentialDistance) || tangentialDistance <= Precision::Confusion()) {
        throw Base::ValueError("Tangential distance must be greater than zero");
    }
    if (circleCount < 1) {
        throw Base::ValueError("At least one concentric circles is required");
    }
    if (circleCount > PatternConstants::MaximumOccurrences) {
        throw Base::ValueError(
            "Circular pattern would create more than "
            + std::to_string(PatternConstants::MaximumOccurrences) + " circles"
        );
    }

    const gp_Ax2 axis = getRotation(Axis);
    const gp_Ax2 binormalAxis = getRotation(BinormalAxis);

    const gp_Pnt center = axis.Location();
    const gp_Vec axisDirection(axis.Direction());
    const gp_Vec binormalDirection(binormalAxis.Direction());

    gp_Vec radialDirection = binormalDirection
        - axisDirection.Multiplied(binormalDirection.Dot(axisDirection));

    if (radialDirection.SquareMagnitude() <= Precision::SquareConfusion()) {
        throw Base::ValueError("Tangent axis must not be parallel to the pattern axis");
    }

    if (FlipBinormalAxis.getValue()) {
        radialDirection.Reverse();
    }
    radialDirection.Normalize();

    gp_Dir lead(0.0, 1.0, 0.0);
    if (std::abs(axis.Direction().X()) <= Precision::Confusion()
        && std::abs(axis.Direction().Z()) <= Precision::Confusion()) {
        lead = gp_Dir(1.0, 0.0, 0.0);
    }

    std::list<gp_Trsf> transformations;

    // If StartAtShape is false, the original object is the center,
    // and we start creating rings from radius = radialDistance.
    // If true, the original object is part of the first ring (radius = 0).
    int startCircle = !StartAtShape.getValue() ? 1 : 0;
    int endCircle = !StartAtShape.getValue() ? circleCount : circleCount - 1;

    if (includeIdentityTransformation) {
        transformations.emplace_back();
    }

    const double fullCircle = 2.0 * std::acos(-1.0);

    for (int circle = startCircle; circle <= endCircle; ++circle) {
        const double ringRadius = circle * radialDistance;

        int elementCount = 0;
        if (circle == 0) {
            // Circle 0 has no circumference. We derive its count from Circle 1
            // to maintain angular alignment across the whole pattern.
            double circle1Radius = 1.0 * radialDistance;
            double circle1Requested = std::floor(fullCircle * circle1Radius / tangentialDistance);
            elementCount = static_cast<int>(circle1Requested);
            elementCount = (elementCount / symmetry) * symmetry;
            if (elementCount == 0) {
                elementCount = 1;  // Ensure at least the identity exists
            }
        }
        else {
            const double requestedCount = std::floor(fullCircle * ringRadius / tangentialDistance);
            if (!std::isfinite(requestedCount)) {
                throw Base::ValueError("Invalid number of occurrences");
            }
            elementCount = static_cast<int>(requestedCount);
            elementCount = (elementCount / symmetry) * symmetry;
        }

        if (elementCount == 0) {
            continue;
        }

        if (transformations.size() + elementCount > PatternConstants::MaximumOccurrences) {
            throw Base::ValueError(
                "Circular pattern created more than "
                + std::to_string(PatternConstants::MaximumOccurrences) + " circles"
            );
        }

        const double angularStep = fullCircle / elementCount;

        for (int element = 0; element < elementCount; ++element) {
            if (circle == 0 && element == 0) {
                transformations.emplace_back();
                continue;
            }

            gp_Trsf translation;
            translation.SetTranslation(radialDirection * ringRadius);

            gp_Trsf rotation;
            rotation.SetRotation(gp_Ax1(center, axis.Direction()), element * angularStep);

            gp_Trsf finalTrsf = rotation;
            finalTrsf.Multiply(translation);

            transformations.push_back(finalTrsf);
        }
    }

    return transformations;
}

gp_Ax2 CircularPatternExtension::getRotation() const
{
    return getRotation(Axis);
}

gp_Ax2 CircularPatternExtension::getRotation(const App::PropertyLinkSub& property) const
{
    App::DocumentObject* reference = property.getValue();
    const auto subnames = property.getSubValues();
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

    if (auto* sketch = freecad_cast<Part::Part2DObject*>(reference); sketch
        && (subname.empty() || subname == "H_Axis" || subname == "V_Axis" || subname == "N_Axis")) {
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

        base = Base::convertTo<gp_Pnt>(axis.getBase());
        direction = Base::convertTo<gp_Dir>(axis.getDirection());
    }
    else if (auto* line = freecad_cast<App::Line*>(reference)) {
        base = Base::convertTo<gp_Pnt>(line->getBasePoint());
        direction = Base::convertTo<gp_Dir>(line->getDirection());
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
            throw Base::TypeError(
                "Rotation edge must be a straight line, circle "
                "or arc of circle"
            );
        }
    }
    else {
        throw Base::TypeError("Axis reference must be edge of a feature or datum line");
    }

    Base::Placement placement;

    if (auto* object = getExtendedObject()) {
        if (auto* property = object->getPropertyByName<App::PropertyPlacement>("Placement")) {
            placement = property->getValue();
        }
    }

    const Base::Placement inversePlacement = placement.inverse();

    Base::Vector3d rotationAxis;
    double angle;
    inversePlacement.getRotation().getValue(rotationAxis, angle);

    gp_Trsf inverse;
    inverse.SetRotation(gp_Ax1(gp_Pnt(), Base::convertTo<gp_Dir>(rotationAxis)), angle);
    inverse.SetTranslationPart(Base::convertTo<gp_Vec>(inversePlacement.getPosition()));

    base.Transform(inverse);
    direction.Transform(inverse);

    return gp_Ax2(base, direction);
}

short CircularPatternExtension::extensionMustExecute()
{
    return Axis.isTouched() || RadialDistance.isTouched() || TangentialDistance.isTouched()
        || NumberCircles.isTouched() || Symmetry.isTouched();
}
