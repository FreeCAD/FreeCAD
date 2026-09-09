// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ThinExtrusion.h"
#include "ThinExtrusionGeometry.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax3.hxx>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Precision.hxx>

namespace PartDesign
{
double thinExtrusionReach(
    const Part::TopoShape& profile,
    const Part::TopoShape& target,
    const gp_Dir& growth,
    bool extendTarget
)
{
    if (target.isNull() || !target.hasSubShape(TopAbs_FACE)) {
        throw Base::ValueError("Select a termination face for the thin extrusion");
    }

    auto bounds = profile.getBoundBoxOptimal();
    double reach;
    const BRepAdaptor_Surface surface(TopoDS::Face(target.getSubShape(TopAbs_FACE, 1)));
    if (!target.hasSubShape(TopAbs_WIRE) || (extendTarget && surface.GetType() == GeomAbs_Plane)) {
        if (surface.GetType() != GeomAbs_Plane) {
            throw Base::ValueError("An unbounded thin-extrusion target must be a plane");
        }
        const auto plane = surface.Plane();
        const double cosine = std::abs(growth.Dot(plane.Axis().Direction()));
        if (cosine <= Precision::Angular()) {
            throw Base::ValueError("Thin extrusion direction is parallel to the target plane");
        }
        double distance = 0;
        for (double x : {bounds.MinX, bounds.MaxX}) {
            for (double y : {bounds.MinY, bounds.MaxY}) {
                for (double z : {bounds.MinZ, bounds.MaxZ}) {
                    distance = std::max(distance, plane.Distance(gp_Pnt(x, y, z)) / cosine);
                }
            }
        }
        reach = 2 * (distance + bounds.CalcDiagonalLength());
    }
    else {
        bounds.Add(target.getBoundBoxOptimal());
        reach = 2 * bounds.CalcDiagonalLength();
    }

    if (!std::isfinite(reach) || reach <= Precision::Confusion()) {
        throw Base::ValueError("Cannot determine a finite reach for the thin extrusion");
    }

    return reach;
}

Part::TopoShape makeThinExtrusionUntil(
    const Part::TopoShape& profile,
    const Part::TopoShape& target,
    const gp_Dir& growth,
    bool extendTarget,
    long tag,
    const Part::TopoShape* selectionProfile
)
{
    auto boundary = target;
    const double reach = thinExtrusionReach(profile, target, growth, extendTarget);
    if (extendTarget) {
        const auto face = TopoDS::Face(target.getSubShape(TopAbs_FACE, 1));
        BRepBuilderAPI_MakeFace support(BRep_Tool::Surface(face), Precision::Confusion());
        boundary = Part::TopoShape(tag, target.Hasher)
                       .makeElementShape(support, {target}, "ThinExtendedTarget");
    }

    const gp_Vec travel = gp_Vec(growth) * reach;
    auto tool = profile.makeElementPrism(travel, "ThinBoundaryPrism");
    if (extendTarget || !boundary.hasSubShape(TopAbs_WIRE)) {
        const BRepAdaptor_Surface surface(TopoDS::Face(boundary.getSubShape(TopAbs_FACE, 1)));
        if (surface.GetType() == GeomAbs_Plane) {
            // The splitter needs a finite trimming tool. Bound a datum/extended
            // plane by the entire prism in its own frame, not world XY.
            const auto plane = surface.Plane();
            gp_Trsf toPlane;
            toPlane.SetTransformation(plane.Position());
            const auto projected = Part::TopoShape(tag, tool.Hasher)
                                       .makeElementTransform(tool, toPlane)
                                       .getBoundBoxOptimal();
            const double margin = std::max(1.0, projected.CalcDiagonalLength());
            BRepBuilderAPI_MakeFace patch(
                plane,
                projected.MinX - margin,
                projected.MaxX + margin,
                projected.MinY - margin,
                projected.MaxY + margin
            );
            boundary = Part::TopoShape(tag, target.Hasher)
                           .makeElementShape(patch, {target}, "ThinTargetPatch");
        }
    }

    // Artificial endpoint extensions may cross behind an oblique target even
    // when the entire real profile is in front of it. Classify using the actual
    // source, not those temporary overshoots.
    return trimThinExtrusionToBoundary(
        tool,
        boundary,
        selectionProfile ? *selectionProfile : profile,
        travel,
        tag
    );
}

bool thinRootAtStart(const Part::TopoShape& profile, const Part::TopoShape& body)
{
    if (body.isNull()) {
        return true;
    }

    BRepAlgoAPI_Common contact(profile.getShape(), body.getShape());
    if (!contact.IsDone()) {
        throw Base::CADKernelError("Cannot determine the thin-wall root for draft");
    }

    GProp_GProps original, touching;
    BRepGProp::LinearProperties(profile.getShape(), original);
    BRepGProp::LinearProperties(contact.Shape(), touching);
    // Endpoint-only contact must not turn a gusset's free boundary into its root.
    return original.Mass() > Precision::Confusion()
        && original.Mass() - touching.Mass()
        <= Precision::Confusion() * profile.countSubShapes(TopAbs_EDGE);
}

Part::TopoShape draftThinExtrusion(
    const Part::TopoShape& tool,
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    const gp_Dir& growth,
    double angle,
    bool holdTop,
    long tag,
    const Part::TopoShape* rootProfile,
    const gp_Dir* pullDirection,
    bool flipPull
)
{
    if (std::abs(angle) <= Precision::Angular()) {
        return tool;
    }
    const auto origin = BRep_Tool::Pnt(TopoDS::Vertex(profile.getSubShape(TopAbs_VERTEX, 1)));
    gp_Trsf toGrowth;
    toGrowth.SetTransformation(gp_Ax3(origin, growth));
    auto measured = Part::TopoShape(tag, tool.Hasher).makeElementTransform(tool, toGrowth);
    const auto bounds = measured.getBoundBoxOptimal();
    // Ignore temporary endpoint overshoots when classifying the actual root.
    const bool rootAtStart = thinRootAtStart(rootProfile ? *rootProfile : profile, body);
    const double neutral = (holdTop == rootAtStart) ? bounds.MaxZ : bounds.MinZ;
    gp_Pln plane(origin.Translated(gp_Vec(growth) * neutral), growth);
    auto pull = pullDirection ? *pullDirection : rootAtStart ? growth : growth.Reversed();
    if (pullDirection) {
        plane = gp_Pln(plane.Location(), pull);
    }
    if (flipPull) {
        pull.Reverse();
    }
    auto reference = profile.makeElementPrism(gp_Vec(growth) * (bounds.MaxZ + 1), "ThinDraftReference");
    // Boundary trimming shares faces/edges with the target body. DraftAngle
    // may update their geometric representations; isolate its input so a
    // subsequent trim still sees the original body geometry.
    auto input = tool.makeElementCopy("ThinDraftInput");
    const auto solids = input.getSubTopoShapes(TopAbs_SOLID);
    if (solids.size() > 1) {
        // Planar profile arrangements produce adjacent solid cells. Drafting
        // those cells separately exposes internal junction faces to DraftAngle
        // and can give an edge more than two incident faces. Draft the union's
        // external boundary instead, retaining disjoint components and history.
        input = Part::TopoShape(tag, tool.Hasher)
                    .makeElementFuse(solids, "ThinDraftUnion")
                    .makeElementRefine("ThinDraftRefine");
    }

    BRepOffsetAPI_DraftAngle builder(input.getShape());

    size_t count = 0;
    for (const auto& face : input.getSubTopoShapes(TopAbs_FACE)) {
        BRepExtrema_DistShapeShape separation(face.getShape(), reference.getShape());
        if (!separation.IsDone()) {
            throw Base::CADKernelError("Cannot identify thin-wall draft faces");
        }

        // Free end caps and start/end sections intersect the reference surface.
        // Offset sidewalls do not. A zero-distance side remains anchored to the
        // source in one-sided thickness mode.
        if (separation.Value() <= Precision::Confusion()) {
            continue;
        }

        builder.Add(TopoDS::Face(face.getShape()), pull, angle, plane);
        if (!builder.AddDone()) {
            throw Base::CADKernelError("Cannot apply the requested draft to every thin-wall face");
        }
        ++count;
    }

    if (!count) {
        throw Base::ValueError("No offset sidewalls available for draft");
    }

    builder.Build();
    if (!builder.IsDone()) {
        throw Base::CADKernelError("Thin-wall draft failed or collapsed the wall");
    }

    auto result
        = Part::TopoShape(tag, tool.Hasher).makeElementShape(builder, {input, profile}, "ThinDraft");
    if (!result.isValid()) {
        throw Base::CADKernelError("Thin-wall draft produced invalid geometry");
    }

    return result;
}
}  // namespace PartDesign
