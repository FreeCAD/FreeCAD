// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
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

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/misc/SoState.h>

#include "SoFCScreenSpaceGroup.h"


using namespace Gui::Inventor;

SO_NODE_SOURCE(SoFCScreenSpaceGroup)

void SoFCScreenSpaceGroup::initClass()
{
    SO_NODE_INIT_CLASS(SoFCScreenSpaceGroup, SoSeparator, "Separator");
}

SoFCScreenSpaceGroup::SoFCScreenSpaceGroup()
{
    SO_NODE_CONSTRUCTOR(SoFCScreenSpaceGroup);
}

SoFCScreenSpaceGroup::~SoFCScreenSpaceGroup() = default;

void SoFCScreenSpaceGroup::setCoordinateSpace(CoordinateSpace newCoordinateSpace)
{
    if (coordinateSpace == newCoordinateSpace) {
        return;
    }

    coordinateSpace = newCoordinateSpace;
    touch();
}

SoFCScreenSpaceGroup::CoordinateSpace SoFCScreenSpaceGroup::getCoordinateSpace() const
{
    return coordinateSpace;
}

void SoFCScreenSpaceGroup::setBaseColorLightModel(bool enable)
{
    if (useBaseColorLightModel == enable) {
        return;
    }

    useBaseColorLightModel = enable;
    touch();
}

void SoFCScreenSpaceGroup::setTexturesEnabled(bool enable)
{
    if (texturesEnabled == enable) {
        return;
    }

    texturesEnabled = enable;
    touch();
}

void SoFCScreenSpaceGroup::setMultiTexturesEnabled(bool enable)
{
    if (multiTexturesEnabled == enable) {
        return;
    }

    multiTexturesEnabled = enable;
    touch();
}

void SoFCScreenSpaceGroup::setDepthBuffer(
    bool test,
    bool write,
    SoDepthBufferElement::DepthWriteFunction function,
    const SbVec2f& range
)
{
    if (depthTestEnabled == test && depthWriteEnabled == write && depthFunction == function
        && depthRange == range) {
        return;
    }

    depthTestEnabled = test;
    depthWriteEnabled = write;
    depthFunction = function;
    depthRange = range;
    touch();
}

void SoFCScreenSpaceGroup::doAction(SoAction* action)
{
    inherited::doAction(action);
}

void SoFCScreenSpaceGroup::GLRenderBelowPath(SoGLRenderAction* action)
{
    SoState* state = action ? action->getState() : nullptr;
    if (!state) {
        return;
    }

    state->push();
    applyScreenSpaceState(state);
    inherited::GLRenderBelowPath(action);
    state->pop();
}

void SoFCScreenSpaceGroup::GLRenderInPath(SoGLRenderAction* action)
{
    SoState* state = action ? action->getState() : nullptr;
    if (!state) {
        return;
    }

    state->push();
    applyScreenSpaceState(state);
    inherited::GLRenderInPath(action);
    state->pop();
}

void SoFCScreenSpaceGroup::GLRenderOffPath(SoGLRenderAction*)
{
    // Screen-space wrappers only contribute through their retained children
    // when traversed on the active render path.
}

void SoFCScreenSpaceGroup::getBoundingBox(SoGetBoundingBoxAction*)
{
    // Intentionally empty: screen-space overlays do not contribute to the
    // scene's world-space bounds.
}

void SoFCScreenSpaceGroup::rayPick(SoRayPickAction*)
{
    // Intentionally empty: overlays are not part of normal 3D ray picking.
}

void SoFCScreenSpaceGroup::applyScreenSpaceState(SoState* state)
{
    if (!state) {
        return;
    }

    // Screen-space overlays define their own coordinate system and should not
    // inherit the current 3D camera transform.
    SoModelMatrixElement::set(state, this, SbMatrix::identity());
    SoViewingMatrixElement::set(state, this, SbMatrix::identity());

    SbViewVolume viewVolume;
    if (coordinateSpace == CoordinateSpace::ClipSpace) {
        // Keep clip-space geometry untouched while still publishing a matching
        // logical view volume for Coin code that queries it during traversal.
        viewVolume.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
        SoProjectionMatrixElement::set(state, this, SbMatrix::identity());
        SoViewVolumeElement::set(state, this, viewVolume);
    }
    else if (coordinateSpace == CoordinateSpace::ViewportPixels) {
        const SbViewportRegion& viewport = SoViewportRegionElement::get(state);
        const SbVec2s viewportSize = viewport.getViewportSizePixels();
        const float width = viewportSize[0] > 0 ? static_cast<float>(viewportSize[0]) : 1.0f;
        const float height = viewportSize[1] > 0 ? static_cast<float>(viewportSize[1]) : 1.0f;
        viewVolume.ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
        SbMatrix affine;
        SbMatrix projection;
        viewVolume.getMatrices(affine, projection);
        SoProjectionMatrixElement::set(state, this, projection);
        SoViewVolumeElement::set(state, this, viewVolume);
    }
    else {
        viewVolume.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
        SbMatrix affine;
        SbMatrix projection;
        viewVolume.getMatrices(affine, projection);
        SoProjectionMatrixElement::set(state, this, projection);
        SoViewVolumeElement::set(state, this, viewVolume);
    }

    SoDepthBufferElement::set(
        state,
        depthTestEnabled ? TRUE : FALSE,
        depthWriteEnabled ? TRUE : FALSE,
        depthFunction,
        depthRange
    );

    // Most screen-space overlays want stable authored colors rather than
    // lighting derived from the active scene.
    if (useBaseColorLightModel) {
        SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    }

    SoGLTextureEnabledElement::set(state, this, texturesEnabled ? TRUE : FALSE);
    if (multiTexturesEnabled) {
        SoMultiTextureEnabledElement::set(state, this, TRUE);
    }
    else {
        SoMultiTextureEnabledElement::disableAll(state);
    }
}
