// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include <Mod/Part/App/SectionCap.h>
#include <Mod/Part/PartGlobal.h>

class SoNode;

namespace PartGui
{

/// Sectioning what the 3D view is actually drawing.
///
/// Asking Coin rather than OCCT is what lets the section cover everything
/// visible: a Part solid, a mesh, a link, a whole assembly. They all end up as
/// triangles in the scene graph, and none of them needs a B-rep. It is also the
/// only way to be sure the cap lines up with what is on screen, since it is
/// built from the very triangles being drawn.
namespace SectionCapHarvest
{

/// Every triangle below `node`, in the coordinates the scene graph places them.
///
/// Walking the scene graph is expensive - Coin regenerates primitives as it
/// goes, and on an assembly that is most of a second, far more than slicing the
/// triangles afterwards. So when the plane moves repeatedly over unchanged
/// geometry, harvest once with this and slice the result, rather than walking
/// the graph again for every new plane.
PartExport Part::SectionCap::TriangleSoup fromSceneGraph(SoNode* node);

/// Which part a triangle belongs to: the view provider root of each part,
/// mapped to its index. Instances of one part share a root node and so share an
/// index, which is the same rule execute() applies when it dedups SourceParts.
using PartOwners = std::unordered_map<const SoNode*, std::size_t>;

/// The same walk, but splitting the triangles by the part they came from and
/// appending them into `soups` - which the caller sizes, so several source roots
/// can be harvested into one set of parts.
///
/// The traversal still starts from the top, because that is the only place the
/// accumulated transforms are right. Harvesting each part's own root instead
/// would return triangles in whatever local frame its container happens to
/// impose, and an assembly's placements would simply be missing. So the parts
/// are told apart during the one walk, by tracking which owner the traversal is
/// currently inside, rather than by walking each of them separately.
///
/// Triangles under no listed owner are dropped: they are decoration the section
/// has no part to attribute them to.
PartExport void fromSceneGraph(
    SoNode* node,
    const PartOwners& owners,
    std::vector<Part::SectionCap::TriangleSoup>& soups
);

}  // namespace SectionCapHarvest
}  // namespace PartGui
