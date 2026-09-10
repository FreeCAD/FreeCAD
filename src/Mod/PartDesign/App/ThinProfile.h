// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>
#include <TopTools_IndexedMapOfShape.hxx>

namespace PartDesign
{
struct ThinProfileGraph
{
    std::vector<Part::TopoShape> chains;
    TopTools_IndexedMapOfShape freeVertices;
};

/** Split real intersections and decompose the graph into oriented, nonbranching
 * chains. No projection is performed, so profile geometry stays unchanged. */
ThinProfileGraph makeThinProfileGraph(const Part::TopoShape& profile, long tag);

}  // namespace PartDesign
