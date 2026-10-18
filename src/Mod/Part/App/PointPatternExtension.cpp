// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PointPatternExtension.h"

#include <vector>

#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>

#include <Base/Exception.h>

#include "PartFeature.h"
#include "TopoShape.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::PointPatternExtension, App::DocumentObjectExtension)

PointPatternExtension::PointPatternExtension()
{
    initExtensionType(PointPatternExtension::getExtensionClassTypeId());
    EXTENSION_ADD_PROPERTY_TYPE(
        PointObject,
        (nullptr),
        "PointPattern",
        App::Prop_None,
        "Sketch or shape object whose vertices define the pattern positions."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        OriginPointIndex,
        (0),
        "PointPattern",
        App::Prop_None,
        "The origin point index, used for the identity transformation."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        RelativeToFirst,
        (false),
        "PointPattern",
        App::Prop_None,
        "Moves all transformations relative to the first one."
    );
    OriginPointIndex.setStatus(App::Property::Hidden, RelativeToFirst.getValue());
}

void PointPatternExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &RelativeToFirst) {
        OriginPointIndex.setStatus(App::Property::Hidden, RelativeToFirst.getValue());
    }
    App::DocumentObjectExtension::extensionOnChanged(prop);
}

bool PointPatternExtension::identityTransformSuppressed() const
{
    return RelativeToFirst.getValue();
}

std::list<gp_Trsf> PointPatternExtension::calculateTransformations() const
{
    if (!PointObject.getValue()) {
        return {};
    }

    auto* feature = freecad_cast<Part::Feature*>(PointObject.getValue());
    if (!feature) {
        throw Base::TypeError("Point object must reference an object with a shape");
    }

    const Part::TopoShape shape = feature->Shape.getShape();
    if (shape.isNull()) {
        throw Base::ValueError("Point object shape is empty");
    }

    std::vector<gp_Pnt> points;
    for (const auto& vertexShape : shape.getSubShapes(TopAbs_VERTEX)) {
        const gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(vertexShape));
        bool duplicate = false;
        for (const auto& existing : points) {
            if (point.Distance(existing) <= Precision::Confusion()) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            points.push_back(point);
        }
    }

    if (points.empty()) {
        throw Base::ValueError("Point object does not contain vertices");
    }

    const int originIndex = RelativeToFirst.getValue() ? 0 : OriginPointIndex.getValue();
    if (originIndex < 0 || static_cast<std::size_t>(originIndex) >= points.size()) {
        throw Base::ValueError("Origin point index is out of range");
    }

    const gp_Pnt origin = points[static_cast<std::size_t>(originIndex)];
    std::list<gp_Trsf> transformations;

    for (std::size_t offset = 0; offset < points.size(); ++offset) {
        const std::size_t pointIndex = (static_cast<std::size_t>(originIndex) + offset)
            % points.size();

        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(origin, points[pointIndex]));
        transformations.push_back(transform);
    }

    if (RelativeToFirst.getValue() && !transformations.empty()) {
        const gp_Vec originVector(origin.X(), origin.Y(), origin.Z());

        for (gp_Trsf& transform : transformations) {
            gp_Vec translation(transform.TranslationPart());
            translation.Add(originVector);
            transform.SetTranslationPart(translation);
        }
    }

    return transformations;
}

short PointPatternExtension::extensionMustExecute()
{
    return PointObject.isTouched() || OriginPointIndex.isTouched() || RelativeToFirst.isTouched();
}
