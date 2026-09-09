// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <Geom_Curve.hxx>
#include <TopoDS_Edge.hxx>

namespace PartDesign
{
struct ThinEndpointJet
{
    gp_Pnt point;
    gp_Vec tangent;  // Unit tangent pointing away from the source edge.
    gp_Vec velocity;
    gp_Vec acceleration;
};

/** Evaluate the located endpoint, accepting a limiting tangent only for C1. */
ThinEndpointJet thinEndpointJet(const TopoDS_Edge& edge, bool after, bool requireC2);

/** Independent quintic continuation: preserves the source's endpoint jet and
 * becomes C2 with a straight tangent tail. The returned parameter range is [0,1]. */
Handle(Geom_Curve) makeThinC2Transition(const ThinEndpointJet& jet, double reach);

/** Direction toward the nearest usable body boundary in the profile plane.
 * Reject directions that collapse a whole construction edge. The optional
 * sweepProfile includes extensions without changing the original profile center.
 * Symmetric nearest contacts are averaged; ambiguity requires an override. */
gp_Dir thinDirectionTowardBody(
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    const gp_Dir& normal,
    const Part::TopoShape* sweepProfile = nullptr
);

// Shared construction operations: no dependency on a document feature or UI.

/** Constant direction from the profile's length-weighted center to the nearest
 * point on reference geometry. Both shapes must be in the same coordinate frame.
 * Reject zero or ambiguous directions rather than choosing an arbitrary side. */
gp_Dir thinDirectionTowardReference(const Part::TopoShape& profile, const Part::TopoShape& reference);

/** Intrinsic direction of a line, conic axis, planar normal or axial surface.
 * Compound/wire/shell references must define one common axis direction.
 * Points and arbitrary curves/surfaces have no intrinsic direction. */
gp_Dir thinReferenceAxis(const Part::TopoShape& reference);

/** Extend free endpoints with tangent or C2 continuation. With throughBody,
 * overrun centerline contact for subsequent full-thickness solid trimming. */
PartDesignExport Part::TopoShape extendThinProfile(
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    bool naturalCurve,
    long tag,
    bool throughBody = false
);

/** Reject a retained wall that still reaches an artificial overrun cap. */
void checkThinExtensionBoundary(
    const Part::TopoShape& wall,
    const Part::TopoShape& extendedProfile,
    const gp_Dir& normal,
    const gp_Dir& growth
);

/** Add semicylindrical caps at degree-one endpoints, in the extrusion frame. */
Part::TopoShape roundThinExtrusionEnds(
    const Part::TopoShape& tool,
    const Part::TopoShape& profile,
    const gp_Dir& direction,
    double length,
    double sideA,
    double sideB,
    bool reversed,
    long tag
);

/** Retain only the portion adjacent to the source, before the next body
 * boundary. Reject a footprint that escapes the target; never return an
 * arbitrary finite wall when a next-boundary construction is requested.
 * With requireTermination=false, the requested finite depth is also a valid
 * end condition: retain source-side material even when it ends before the body.
 */
Part::TopoShape trimThinExtrusionToBoundary(
    const Part::TopoShape& tool,
    const Part::TopoShape& body,
    const Part::TopoShape& source,
    const gp_Vec& travel,
    long tag,
    bool requireTermination = true
);

/** Classify root/exposed edges from their relation to the original body and
 * generated material, then apply history-aware fillets. */
Part::TopoShape finishThinExtrusion(
    const Part::TopoShape& result,
    const Part::TopoShape& body,
    const Part::TopoShape& tool,
    double rootRadius,
    double exposedRadius,
    long tag
);

}  // namespace PartDesign
