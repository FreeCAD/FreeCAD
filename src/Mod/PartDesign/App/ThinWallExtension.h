// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ThinProfile.h"

namespace PartDesign
{

/** Extend selected edges' free endpoints with C1 or C2 continuity across a finite extrusion, then
 * retain the source-connected material bounded by the existing body. All
 * inputs share the feature-local frame. No sampled approximation of the wall
 * is used; the Boolean retains the body's actual surfaces. An empty selection
 * extends all free endpoints. Selected edges must be contained in the profile.
 * Subtractive construction retains source-connected material inside the body.
 * C2 uses the shared rib spline continuation followed by a tangent tail. */
Part::TopoShape makeBodyBoundedThinWall(
    const Part::TopoShape& profile,
    const gp_Pln& plane,
    const Part::TopoShape& body,
    const gp_Vec& travel,
    double sideA,
    double sideB,
    Part::JoinType join,
    double draftAngle,
    bool holdTop,
    long tag,
    const std::vector<Part::TopoShape>& extensionEdges = {},
    bool roundEnds = false,
    const Part::TopoShape* terminationFace = nullptr,
    bool curvatureContinuous = false,
    bool subtractive = false
);

}  // namespace PartDesign
