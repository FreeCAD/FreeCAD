// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cstring>

#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRepLib_FindSurface.hxx>
#include <Bnd_Box.hxx>
#include <Geom_Plane.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>

#include <Base/Exception.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureIntersectionCurve.h"

using namespace Surface;

// Static property registration is provided by the shared FreeCAD macro.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
PROPERTY_SOURCE(Surface::IntersectionCurve, Part::Feature)

IntersectionCurve::IntersectionCurve()
{
    ADD_PROPERTY_TYPE(Curve1, (nullptr), "Intersection", App::Prop_None, "First shape or subelement");
    ADD_PROPERTY_TYPE(Curve2, (nullptr), "Intersection", App::Prop_None, "Second shape or subelement");
    ADD_PROPERTY_TYPE(
        Mode,
        (0L),
        "Intersection",
        App::Prop_None,
        "Automatic extrudes two whole curve profiles and otherwise intersects shapes directly; "
        "Direct intersects the input geometry; ExtrudedProfiles extrudes two curve profiles"
    );
    static const char* modes[] = {"Automatic", "Direct", "ExtrudedProfiles", nullptr};
    Mode.setEnums(modes);
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
    if (Curve1.isTouched() || Curve2.isTouched() || Mode.isTouched() || Direction1.isTouched()
        || Direction2.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

void IntersectionCurve::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* typeName,
    App::Property* prop
)
{
    if ((prop == &Curve1 || prop == &Curve2) && std::strcmp(typeName, "App::PropertyLink") == 0) {
        App::PropertyLink oldLink;
        oldLink.setContainer(this);
        oldLink.Restore(reader);
        static_cast<App::PropertyLinkSub*>(prop)->setValue(oldLink.getValue());
    }
    else {
        Part::Feature::handleChangedPropertyType(reader, typeName, prop);
    }
}

namespace
{
const char* subName(const App::PropertyLinkSub& link)
{
    const auto& subs = link.getSubValues();
    if (subs.size() > 1) {
        throw Base::ValueError("Each input must reference one whole shape or one subelement.");
    }
    return subs.empty() ? nullptr : subs.front().c_str();
}

Part::TopoShape inputShape(const App::PropertyLinkSub& link)
{
    const auto shape = Part::Feature::getTopoShape(
        link.getValue(),
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
            | Part::ShapeOption::NeedSubElement | Part::ShapeOption::DontSimplifyCompound,
        subName(link)
    );
    if (shape.isNull() || !shape.hasSubShape(TopAbs_VERTEX)) {
        throw Base::ValueError("Select nonempty shapes or subelements.");
    }
    return shape;
}

bool isProfile(const Part::TopoShape& shape)
{
    return shape.hasSubShape(TopAbs_EDGE) && !shape.hasSubShape(TopAbs_FACE);
}

gp_Vec extrusionDirection(
    const App::PropertyLinkSub& link,
    const Part::TopoShape& profile,
    const App::PropertyVector& property
)
{
    auto direction = property.getValue();
    if (direction.Length() <= Precision::Confusion()) {
        Base::Matrix4D matrix;
        App::DocumentObject* owner = nullptr;
        Part::Feature::getShape(
            link.getValue(),
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
                | Part::ShapeOption::NeedSubElement,
            subName(link),
            &matrix,
            &owner
        );
        if (owner && owner->isDerivedFrom<Part::Part2DObject>()) {
            Base::Rotation(matrix).multVec(Base::Vector3d(0, 0, 1), direction);
        }
        else {
            BRepLib_FindSurface plane(profile.getShape(), -1, true);
            if (!plane.Found()) {
                throw Base::ValueError(
                    "Cannot determine an extrusion direction; set Direction1 or Direction2."
                );
            }
            const auto normal = Handle(Geom_Plane)::DownCast(plane.Surface())->Pln().Axis().Direction();
            direction = Base::Vector3d(normal.X(), normal.Y(), normal.Z());
        }
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
        if (!Curve1.getValue() || !Curve2.getValue()) {
            throw Base::ValueError("Two shapes or subelements are required.");
        }
        auto first = inputShape(Curve1);
        auto second = inputShape(Curve2);
        if (first.getShape().IsSame(second.getShape())) {
            throw Base::ValueError("Two different shapes or subelements are required.");
        }
        const bool profiles = isProfile(first) && isProfile(second);
        if (Mode.getValue() == 2 && !profiles) {
            throw Base::ValueError("ExtrudedProfiles requires two curves containing no faces.");
        }
        const bool wholeProfiles = profiles && (!subName(Curve1) || !*subName(Curve1))
            && (!subName(Curve2) || !*subName(Curve2));
        if ((Mode.getValue() == 0 && wholeProfiles) || Mode.getValue() == 2) {
            const auto direction1 = extrusionDirection(Curve1, first, Direction1);
            const auto direction2 = extrusionDirection(Curve2, second, Direction2);
            const double sine = direction1.Crossed(direction2).Magnitude();
            if (sine <= Precision::Angular()) {
                throw Base::ValueError("The extrusion directions must not be parallel.");
            }
            Bnd_Box bounds;
            BRepBndLib::Add(first.getShape(), bounds);
            BRepBndLib::Add(second.getShape(), bounds);
            // For p + t*d1 = q + u*d2, |t| and |u| <= |q-p| / sin(angle).
            constexpr double toleranceMargin = 10.0;
            const double extent = (std::max(bounds.CornerMin().Distance(bounds.CornerMax()), 1.0)
                                   / sine)
                + (toleranceMargin * Precision::Confusion());
            first = extrude(first, direction1, extent);
            second = extrude(second, direction2, extent);
        }
        BRepAlgoAPI_Section section;
        section.Init1(first.getShape());
        section.Init2(second.getShape());
        section.Approximation(true);
        section.Build();
        if (!section.IsDone()) {
            throw Base::ValueError("The intersection calculation failed.");
        }
        Part::TopoShape result;
        result.makeElementShape(section, {first, second}, Part::OpCodes::Section);
        if (!result.hasSubShape(TopAbs_VERTEX)) {
            throw Base::ValueError("The inputs do not intersect.");
        }
        if (result.hasSubShape(TopAbs_EDGE)) {
            // Preserve disconnected wires and isolated intersection points.
            std::vector<Part::TopoShape> pieces {result.makeElementWires()};
            for (TopExp_Explorer vertex(result.getShape(), TopAbs_VERTEX, TopAbs_EDGE); vertex.More();
                 vertex.Next()) {
                pieces.push_back(
                    result.getSubTopoShape(TopAbs_VERTEX, result.findShape(vertex.Current()))
                );
            }
            if (pieces.size() > 1) {
                Part::TopoShape compound;
                compound.makeElementCompound(pieces);
                result = compound;
            }
            else {
                result = pieces.front();
            }
        }
        Shape.setValue(result);
        return StdReturn;
    }
    catch (const Standard_Failure& error) {
        // OCCT 7.6 (Ubuntu) does not provide Standard_Failure::what().
        return new App::DocumentObjectExecReturn(error.GetMessageString());
    }
    catch (const Base::Exception& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }
}
