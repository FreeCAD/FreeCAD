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

#include "PreCompiled.h"

#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShape.h>

#include "SectionCapHarvest.h"


using namespace PartGui;
using Part::SectionCap::Segment;

namespace
{

/// Append one triangle, transformed into world coordinates by wherever the
/// traversal-er currently is.
void appendTriangle(
    Part::SectionCap::TriangleSoup& soup,
    const SbMatrix& toWorld,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    const SoPrimitiveVertex* v3
)
{
    const SoPrimitiveVertex* verts[3] = {v1, v2, v3};
    const int base = static_cast<int>(soup.points.size());
    for (const auto* vertex : verts) {
        SbVec3f world;
        toWorld.multVecMatrix(vertex->getPoint(), world);
        soup.points.emplace_back(world[0], world[1], world[2]);
    }
    soup.indices.push_back(base);
    soup.indices.push_back(base + 1);
    soup.indices.push_back(base + 2);
}

void triangleCB(
    void* userdata,
    SoCallbackAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    const SoPrimitiveVertex* v3
)
{
    auto* soup = static_cast<Part::SectionCap::TriangleSoup*>(userdata);
    appendTriangle(*soup, action->getModelMatrix(), v1, v2, v3);
}

/// Where the split-by-part walk keeps its place.
///
/// **stack**  rather than a single index, because owners can nest - a link to a
/// container inside another part. The innermost owner is the right answer, and
/// popping on the way out restores the enclosing one.
struct SplitHarvest
{
    const SectionCapHarvest::PartOwners* owners = nullptr;
    std::vector<Part::SectionCap::TriangleSoup>* soups = nullptr;
    std::vector<std::size_t> stack;
};

SoCallbackAction::Response enterNodeCB(void* userdata, SoCallbackAction*, const SoNode* node)
{
    auto* ctx = static_cast<SplitHarvest*>(userdata);
    const auto it = ctx->owners->find(node);
    if (it != ctx->owners->end()) {
        ctx->stack.push_back(it->second);
    }
    return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response leaveNodeCB(void* userdata, SoCallbackAction*, const SoNode* node)
{
    auto* ctx = static_cast<SplitHarvest*>(userdata);
    const auto it = ctx->owners->find(node);
    if (it == ctx->owners->end()) {
        return SoCallbackAction::CONTINUE;
    }

    // Pop only what this node pushed. The pair is balanced under an ordinary
    // traversal, but anything that prunes or aborts between them would leave the
    // stack deeper than the tree, and every triangle after that would be
    // attributed to the wrong part - quietly, and only on some models.
    if (!ctx->stack.empty() && ctx->stack.back() == it->second) {
        ctx->stack.pop_back();
    }
    return SoCallbackAction::CONTINUE;
}

void splitTriangleCB(
    void* userdata,
    SoCallbackAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    const SoPrimitiveVertex* v3
)
{
    auto* ctx = static_cast<SplitHarvest*>(userdata);
    if (ctx->stack.empty()) {
        // Decoration - an origin, a datum, anything outside the parts we were
        // given. There is nothing to attribute it to, so it is not section
        // geometry.
        return;
    }
    const std::size_t owner = ctx->stack.back();
    if (owner >= ctx->soups->size()) {
        return;
    }
    appendTriangle((*ctx->soups)[owner], action->getModelMatrix(), v1, v2, v3);
}

}  // namespace


Part::SectionCap::TriangleSoup SectionCapHarvest::fromSceneGraph(SoNode* node)
{
    Part::SectionCap::TriangleSoup soup;
    if (!node) {
        return soup;
    }

    SoCallbackAction action(SbViewportRegion(1024, 1024));
    action.addTriangleCallback(SoShape::getClassTypeId(), triangleCB, &soup);
    action.apply(node);

    return soup;
}


void SectionCapHarvest::fromSceneGraph(
    SoNode* node,
    const PartOwners& owners,
    std::vector<Part::SectionCap::TriangleSoup>& soups
)
{
    if (!node || owners.empty() || soups.empty()) {
        return;
    }

    SplitHarvest context;
    context.owners = &owners;
    context.soups = &soups;

    SoCallbackAction action(SbViewportRegion(1024, 1024));
    // Registered on SoSeparator because that is what ViewProvider::getRoot()
    // returns, and Coin matches subclasses too - the same way a callback on
    // SoShape catches every kind of shape.
    action.addPreCallback(SoSeparator::getClassTypeId(), enterNodeCB, &context);
    action.addPostCallback(SoSeparator::getClassTypeId(), leaveNodeCB, &context);
    action.addTriangleCallback(SoShape::getClassTypeId(), splitTriangleCB, &context);
    action.apply(node);
}
