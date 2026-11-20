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

#include <FCConfig.h>

#include <algorithm>

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <GL/gl.h>
#endif

#include <Inventor/SbColor.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/misc/SoState.h>

#include <Gui/Inventor/So3DAnnotation.h>
#include <Gui/ViewParams.h>

#include "SoBrepHiddenSelection.h"

using namespace PartGui;

namespace
{
// Paths queued in a frame whose delayed paths were never rendered (an aborted render) stay in
// the list until the same path is queued again. Drop everything if it ever grows this large.
constexpr std::size_t MaxPendingPaths = 256;

// How far the hidden pass is brought forward, as a share of the depth range.
constexpr float DepthBias = 1.0e-5F;
}  // namespace

bool HiddenSelectionPass::isEnabled()
{
    return Gui::ViewParams::instance()->getShowHiddenSelection();
}

HiddenSelectionPass::PathKey HiddenSelectionPass::makeKey(const SoPath* path)
{
    PathKey key;
    const auto* full = static_cast<const SoFullPath*>(path);
    const int length = full->getLength();
    key.nodes.reserve(length);
    key.indices.reserve(length);
    for (int i = 0; i < length; ++i) {
        key.nodes.push_back(full->getNode(i));
        key.indices.push_back(i > 0 ? full->getIndex(i) : 0);
    }
    return key;
}

bool HiddenSelectionPass::matches(const PathKey& key, const SoPath* path)
{
    const auto* full = static_cast<const SoFullPath*>(path);
    const int length = full->getLength();
    if (length != static_cast<int>(key.nodes.size())) {
        return false;
    }
    // Compare from the tail: paths to different nodes usually differ there first.
    for (int i = length - 1; i >= 0; --i) {
        if (key.nodes[i] != full->getNode(i) || (i > 0 && key.indices[i] != full->getIndex(i))) {
            return false;
        }
    }
    return true;
}

void HiddenSelectionPass::schedule(SoGLRenderAction* action)
{
    if (!action || action->isRenderingDelayedPaths()) {
        // Coin does not allow adding delayed paths while it renders them.
        return;
    }
    if (Gui::SoDelayedAnnotationsElement::isProcessingDelayedPaths || Gui::So3DAnnotation::render) {
        // FreeCAD's own annotation pass clears the depth buffer first, so a hidden pass queued
        // from there would test against the annotations only.
        return;
    }
    if (!isEnabled()) {
        return;
    }

    const SoPath* current = action->getCurPath();
    if (!current) {
        return;
    }

    // A path queued in an earlier frame and never consumed is replaced, not duplicated.
    pending.erase(
        std::remove_if(
            pending.begin(),
            pending.end(),
            [current](const PathKey& key) { return matches(key, current); }
        ),
        pending.end()
    );
    if (pending.size() >= MaxPendingPaths) {
        pending.clear();
    }

    pending.push_back(makeKey(current));
    // addDelayedPath() also invalidates the render caches above this node, as it must:
    // a cached render would skip this call in later frames.
    action->addDelayedPath(current->copy());
}

bool HiddenSelectionPass::isHiddenPass(SoGLRenderAction* action)
{
    if (pending.empty() || !action || !action->isRenderingDelayedPaths()) {
        return false;
    }
    const SoPath* current = action->getCurPath();
    if (!current) {
        return false;
    }
    auto it = std::find_if(pending.begin(), pending.end(), [current](const PathKey& key) {
        return matches(key, current);
    });
    if (it == pending.end()) {
        return false;
    }
    pending.erase(it);
    return true;
}

void HiddenSelectionPass::applyDepthState(SoState* state)
{
    // The far end of the depth range is pulled in a hair, which brings every fragment of the
    // hidden pass slightly towards the viewer. Where the selection is itself what is in front,
    // its fragments then land on or just before their own depth and fail the test, instead of
    // passing here and there on rounding differences between two renders of the same geometry.
    SoDepthBufferElement::set(
        state,
        TRUE,
        FALSE,
        SoDepthBufferElement::GREATER,
        SbVec2f(0.0F, 1.0F - DepthBias)
    );
}

void HiddenSelectionPass::renderDashedLines(
    SoGLRenderAction* action,
    SoNode* node,
    const int32_t* indices,
    int numIndices,
    const SbColor& color,
    float lineWidth
)
{
    if (!action || !node || !indices || numIndices < 2) {
        return;
    }

    SoState* state = action->getState();
    state->push();

    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoTextureEnabledElement::set(state, node, false);
    SoMaterialBindingElement::set(state, SoMaterialBindingElement::OVERALL);
    SoOverrideElement::setMaterialBindingOverride(state, node, true);
    applyDepthState(state);
    SoLineWidthElement::set(state, node, std::max(lineWidth, 1.0F));
    SoLinePatternElement::set(state, node, LinePattern);

    SoLazyElement::setEmissive(state, &color);
    const uint32_t packedColor = color.getPackedValue(0.0F);
    SoLazyElement::setPacked(state, node, 1, &packedColor, false);

    // Send the material and light model; depth, width and pattern are sent as they are set.
    SoMaterialBundle mb(action);
    mb.sendFirst();

    // Drawn with plain GL rather than through an SoIndexedLineSet: Coin renders long line sets
    // as vertex arrays of separate GL_LINES segments, which restarts the stipple pattern at
    // every segment and makes finely tessellated curves look solid.
    const auto* coords = static_cast<const SoGLCoordinateElement*>(
        SoCoordinateElement::getInstance(state)
    );
    const int numCoords = coords->getNum();
    bool inStrip = false;
    for (int i = 0; i < numIndices; ++i) {
        const int32_t index = indices[i];
        if (index < 0 || index >= numCoords) {
            if (inStrip) {
                glEnd();
                inStrip = false;
            }
            continue;
        }
        if (!inStrip) {
            glBegin(GL_LINE_STRIP);
            inStrip = true;
        }
        coords->send(index);
    }
    if (inStrip) {
        glEnd();
    }

    state->pop();
}
