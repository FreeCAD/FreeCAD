// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{
double thinExtrusionReach(
    const Part::TopoShape& profile,
    const Part::TopoShape& target,
    const gp_Dir& growth,
    bool extendTarget
);
Part::TopoShape makeThinExtrusionUntil(
    const Part::TopoShape& profile,
    const Part::TopoShape& target,
    const gp_Dir& growth,
    bool extendTarget,
    long tag,
    const Part::TopoShape* selectionProfile = nullptr
);

/** True when the source lies on/in the body, rather than forming a free boundary.
 * Shared by draft construction and draggers; both inputs must share a frame. */
PartDesignExport bool thinRootAtStart(const Part::TopoShape& profile, const Part::TopoShape& body);
Part::TopoShape draftThinExtrusion(
    const Part::TopoShape& tool,
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    const gp_Dir& growth,
    double angle,
    bool holdTop,
    long tag,
    const Part::TopoShape* rootProfile = nullptr,
    const gp_Dir* pullDirection = nullptr,
    bool flipPull = false
);

}  // namespace PartDesign
