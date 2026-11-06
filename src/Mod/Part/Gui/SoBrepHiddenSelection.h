// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Ryan Kembrey <ryan.FreeCAD@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <vector>

#include <Mod/Part/PartGlobal.h>

class SbColor;
class SoGLRenderAction;
class SoNode;
class SoPath;
class SoState;

namespace PartGui
{

/**
 * Shows the part of a sub-element selection that is hidden behind other geometry.
 *
 * A selected face, edge or vertex that ends up behind other geometry when the view is rotated
 * would otherwise be invisible, so the user can no longer tell what is selected (issue #13981).
 * Like hidden lines on a drawing, the hidden part is drawn through whatever is in front of it:
 * faces as a faint tint of the selection color with a dashed outline, edges dashed, and
 * vertices faded. The visible part of the selection is drawn as before.
 *
 * How it works: a node with a selection draws itself as usual in the normal render pass and
 * calls schedule(), which queues its path as a Coin delayed path. Coin renders delayed paths
 * after all opaque and transparent geometry, with the depth buffer of the whole scene intact.
 * When the node is traversed again from that path, isHiddenPass() returns true and the node
 * draws its selection with the depth test set to GL_GREATER and depth writes off, so only
 * the fragments behind something already drawn reach the screen.
 *
 * Paths are matched exactly, so a node that is rendered in a delayed path for another reason
 * (for example an object shown with OnTopWhenSelected) is not mistaken for the hidden pass.
 */
class PartGuiExport HiddenSelectionPass
{
public:
    /// Transparency of the tint drawn over hidden selected faces.
    static constexpr float FaceTransparency = 0.7F;
    /// Transparency of hidden selected vertices.
    static constexpr float PointTransparency = 0.5F;
    /// Stipple pattern of hidden selected edges and face outlines (4 pixels on, 4 off).
    static constexpr int32_t LinePattern = 0x0F0F;

    HiddenSelectionPass() = default;
    ~HiddenSelectionPass() = default;
    HiddenSelectionPass(const HiddenSelectionPass&) = delete;
    HiddenSelectionPass(HiddenSelectionPass&&) = delete;
    HiddenSelectionPass& operator=(const HiddenSelectionPass&) = delete;
    HiddenSelectionPass& operator=(HiddenSelectionPass&&) = delete;

    /// Whether hidden selections are shown (View/ShowHiddenSelection preference).
    static bool isEnabled();

    /** Queue the hidden pass for the node at the current path.
     *
     *  Call from the node's GLRender() when it has a sub-element selection. Does nothing while
     *  delayed paths or FreeCAD's delayed annotations are being rendered, or when the
     *  preference is off.
     */
    void schedule(SoGLRenderAction* action);

    /** Whether this traversal is the hidden pass queued by schedule().
     *
     *  Returns true at most once per queued pass. When it does, the node must draw only its
     *  hidden selection and return.
     */
    bool isHiddenPass(SoGLRenderAction* action);

    /// Depth state of the hidden pass: test GL_GREATER, no depth writes.
    static void applyDepthState(SoState* state);

    /** Draw dashed lines through the hidden part of the scene.
     *
     *  @param indices polylines in the current coordinates, each terminated by -1
     *  (SoIndexedLineSet::coordIndex style). Each polyline is drawn as one line strip, so the
     *  dash pattern runs on along curves instead of restarting at every short segment.
     */
    static void renderDashedLines(
        SoGLRenderAction* action,
        SoNode* node,
        const int32_t* indices,
        int numIndices,
        const SbColor& color,
        float lineWidth
    );

private:
    /// The nodes and child indices of a path, kept without referencing the nodes: it is only
    /// compared, and holding references would keep this node alive through its own path.
    struct PathKey
    {
        std::vector<const SoNode*> nodes;
        std::vector<int> indices;
    };

    static PathKey makeKey(const SoPath* path);
    static bool matches(const PathKey& key, const SoPath* path);

    std::vector<PathKey> pending;
};

}  // namespace PartGui
