// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>
#include <gp_Pln.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

namespace PartDesign
{
struct ThinProfileGraph
{
    std::vector<Part::TopoShape> chains;
    TopTools_IndexedMapOfShape freeVertices;
};

/** Split real intersections and decompose the graph into oriented, nonbranching
 * chains. No projection is performed, so the same graph supports spatial webs. */
ThinProfileGraph makeThinProfileGraph(const Part::TopoShape& profile, long tag);

/** Construct material regions in an explicit support plane, without extruding.
 * Distances are nonnegative; A is left of an oriented open chain, or outside
 * a closed loop. B is the opposite side. The plane and input share a frame.
 * This construction is independent of Pad/Pocket growth and reversal.
 */
Part::TopoShape makeThinProfile(
    const Part::TopoShape& profile,
    const gp_Pln& plane,
    double sideA,
    double sideB,
    Part::JoinType join,
    long tag,
    bool roundEnds = false
);

}  // namespace PartDesign
