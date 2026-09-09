// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>

namespace PartDesign
{
// Surface-first thin extrusion for side profiles and spatial edge chains.
// All geometry and the direction are expressed in the profile's local frame.
Part::TopoShape makeThinSurfaceExtrusion(
    const std::vector<Part::TopoShape>& edges,
    const gp_Dir& direction,
    double length,
    double thickness,
    int side,
    bool reversed,
    Part::JoinType join,
    bool intersection,
    long tag
);

}  // namespace PartDesign
