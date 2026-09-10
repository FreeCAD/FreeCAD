// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>

#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Trsf.hxx>

#include <Base/Exception.h>
#include <Mod/Part/App/FeatureExtrusion.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureIntersectionCurve.h"

using namespace Surface;

// Static property registration is provided by the shared FreeCAD macro.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
PROPERTY_SOURCE(Surface::IntersectionCurve, Part::Feature)

IntersectionCurve::IntersectionCurve()
{
    ADD_PROPERTY_TYPE(Curve1, (nullptr), "Intersection", App::Prop_None, "First sketch or wire");
    ADD_PROPERTY_TYPE(Curve2, (nullptr), "Intersection", App::Prop_None, "Second sketch or wire");
    ADD_PROPERTY_TYPE(
        Direction1,
        (0.0, 0.0, 0.0),
        "Intersection",
        App::Prop_None,
        "First extrusion direction in document coordinates; zero uses the profile normal"
    );
    ADD_PROPERTY_TYPE(
        Direction2,
        (0.0, 0.0, 0.0),
        "Intersection",
        App::Prop_None,
        "Second extrusion direction in document coordinates; zero uses the profile normal"
    );
    Curve1.setScope(App::LinkScope::Global);
    Curve2.setScope(App::LinkScope::Global);
}

short IntersectionCurve::mustExecute() const
{
    if (Curve1.isTouched() || Curve2.isTouched() || Direction1.isTouched() || Direction2.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

namespace
{
Part::TopoShape profileShape(const App::PropertyLink& link)
{
    const auto shape = Part::Feature::getTopoShape(
        link.getValue(),
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (shape.isNull() || !TopExp_Explorer(shape.getShape(), TopAbs_EDGE).More()
        || TopExp_Explorer(shape.getShape(), TopAbs_FACE).More()) {
        throw Base::ValueError("Select sketches or wires containing edges and no faces.");
    }
    return shape;
}

gp_Vec extrusionDirection(const App::PropertyLink& link, const App::PropertyVector& property)
{
    auto direction = property.getValue();
    if (direction.Length() <= Precision::Confusion()) {
        direction = Part::Extrusion::calculateShapeNormal(link);
    }
    gp_Vec result(direction.x, direction.y, direction.z);
    if (result.Magnitude() <= Precision::Confusion()) {
        throw Base::ValueError(
            "Cannot determine an extrusion direction; set Direction1 or Direction2."
        );
    }
    return result.Normalized();
}

Part::TopoShape extrude(const Part::TopoShape& profile, const gp_Vec& direction, double extent)
{
    gp_Trsf shift;
    shift.SetTranslation(direction * -extent);
    Part::TopoShape shifted;
    shifted.makeElementTransform(profile, shift);
    return shifted.makeElementPrism(direction * (extent + extent));
}
}  // namespace

App::DocumentObjectExecReturn* IntersectionCurve::execute()
{
    // Clear a previous result so a failed recompute cannot display an obsolete curve.
    Shape.setValue(TopoDS_Shape());
    try {
        if (!Curve1.getValue() || !Curve2.getValue() || Curve1.getValue() == Curve2.getValue()) {
            throw Base::ValueError("Two different sketches or wires are required.");
        }
        const auto first = profileShape(Curve1);
        const auto second = profileShape(Curve2);
        const auto direction1 = extrusionDirection(Curve1, Direction1);
        const auto direction2 = extrusionDirection(Curve2, Direction2);
        const double sine = direction1.Crossed(direction2).Magnitude();
        if (sine <= Precision::Angular()) {
            throw Base::ValueError("The extrusion directions must not be parallel.");
        }

        Bnd_Box bounds;
        BRepBndLib::Add(first.getShape(), bounds);
        BRepBndLib::Add(second.getShape(), bounds);
        // For p + t*d1 = q + u*d2, |t| and |u| <= |q-p| / sin(angle).
        // This bounds both extrusions even for distant profiles and oblique directions.
        constexpr double toleranceMargin = 10.0;
        const double extent = (std::max(bounds.CornerMin().Distance(bounds.CornerMax()), 1.0) / sine)
            + (toleranceMargin * Precision::Confusion());
        const std::vector<Part::TopoShape> surfaces {
            extrude(first, direction1, extent),
            extrude(second, direction2, extent),
        };
        BRepAlgoAPI_Section section;
        section.Init1(surfaces.front().getShape());
        section.Init2(surfaces.back().getShape());
        section.Approximation(true);
        section.Build();
        if (!section.IsDone()) {
            throw Base::ValueError("The intersection calculation failed.");
        }
        if (!TopExp_Explorer(section.Shape(), TopAbs_EDGE).More()) {
            throw Base::ValueError("The extruded curves do not intersect in a curve.");
        }
        // Preserve all branches, joining connected edges into wires.
        Part::TopoShape result;
        result.makeElementShape(section, surfaces, Part::OpCodes::Section);
        Shape.setValue(result.makeElementWires());
        return StdReturn;
    }
    catch (const Standard_Failure& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }
    catch (const Base::Exception& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }
}
