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
}

std::list<gp_Trsf> PointPatternExtension::calculateTransformations(bool relativeToFirst) const
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

    const gp_Pnt origin = relativeToFirst ? points.front() : gp_Pnt();
    std::list<gp_Trsf> transformations;
    for (const auto& point : points) {
        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(origin, point));
        transformations.push_back(transform);
    }
    return transformations;
}

short PointPatternExtension::extensionMustExecute()
{
    return PointObject.isTouched() ? 1 : 0;
}
