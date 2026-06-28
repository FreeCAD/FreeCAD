// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <FCConfig.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoLineDetail.h>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include "SoDrawingGrid.h"


using namespace Gui::Inventor;

/*
from pivy import coin
grid=coin.SoType.fromName("SoDrawingGrid").createInstance()
Gui.ActiveDocument.ActiveView.getSceneGraph().addChild(grid)
*/

SO_NODE_SOURCE(SoDrawingGrid)

void SoDrawingGrid::initClass()
{
    SO_NODE_INIT_CLASS(SoDrawingGrid, SoShape, "Shape");
}

SoDrawingGrid::SoDrawingGrid()
{
    SO_NODE_CONSTRUCTOR(SoDrawingGrid);

    m_Root = new SoSeparator;
    m_Root->ref();

    constexpr float legacyGridChannel = 10.0f / 255.0f;
    auto* color = new SoBaseColor;
    color->rgb.setValue(legacyGridChannel, legacyGridChannel, legacyGridChannel);
    m_Root->addChild(color);

    m_VertexProperty = new SoVertexProperty;
    m_LineSet = new SoLineSet;
    m_LineSet->vertexProperty.setValue(m_VertexProperty);
    m_Root->addChild(m_LineSet);
}

SoDrawingGrid::~SoDrawingGrid()
{
    if (m_Root) {
        m_Root->unref();
        m_Root = nullptr;
    }
    m_VertexProperty = nullptr;
    m_LineSet = nullptr;
}

void SoDrawingGrid::renderGrid(SoGLRenderAction* action)
{
    if (!shouldGLRender(action)) {
        return;
    }

    SoState* state = action->getState();
    state->push();
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

    ensureGeometry(state);

    SoModelMatrixElement::set(state, this, SbMatrix::identity());
    SoViewingMatrixElement::set(state, this, SbMatrix::identity());
    SoProjectionMatrixElement::set(state, this, SbMatrix::identity());

    SoDepthBufferElement::set(state, FALSE, FALSE, SoDepthBufferElement::ALWAYS, SbVec2f(0.0f, 1.0f));
    SoGLTextureEnabledElement::set(state, this, FALSE);
    SoMultiTextureEnabledElement::set(state, this, FALSE);

    if (m_Root) {
        m_Root->GLRender(action);
    }

    state->pop();
}

void SoDrawingGrid::ensureGeometry(SoState* state)
{
    if (!state || !m_VertexProperty || !m_LineSet) {
        return;
    }

    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    const SbVec2s viewportSize = vp.getViewportSizePixels();
    if (viewportSize == m_CachedViewportSize && m_VertexProperty->vertex.getNum() > 0) {
        return;
    }

    m_CachedViewportSize = viewportSize;

    constexpr int numX = 20;
    const float aspectRatio = vp.getViewportAspectRatio();
    int numY = static_cast<int>(static_cast<float>(numX) / aspectRatio);
    numY = std::max(1, numY);

    const int verticalLineCount = 2 * numX;
    const int horizontalLineCount = 2 * numY;
    const int lineCount = verticalLineCount + horizontalLineCount;

    std::vector<SbVec3f> vertices;
    vertices.reserve(lineCount * 2);

    for (int i = -numX; i < numX; i++) {
        const float x = static_cast<float>(i) / static_cast<float>(numX);
        vertices.emplace_back(x, -1.0f, 0.0f);
        vertices.emplace_back(x, 1.0f, 0.0f);
    }

    for (int i = -numY; i < numY; i++) {
        const float y = static_cast<float>(i) / static_cast<float>(numY);
        vertices.emplace_back(-1.0f, y, 0.0f);
        vertices.emplace_back(1.0f, y, 0.0f);
    }

    m_VertexProperty->vertex.setValues(0, static_cast<int>(vertices.size()), vertices.data());

    std::vector<int32_t> numVertices;
    numVertices.assign(lineCount, 2);
    m_LineSet->numVertices.setValues(0, static_cast<int>(numVertices.size()), numVertices.data());
}

void SoDrawingGrid::GLRender(SoGLRenderAction* action)
{
    // renderGrid(action);
    // return;
    switch (action->getCurPathCode()) {
        case SoAction::NO_PATH:
        case SoAction::BELOW_PATH:
            this->GLRenderBelowPath(action);
            break;
        case SoAction::OFF_PATH:
            // do nothing. Separator will reset state.
            break;
        case SoAction::IN_PATH:
            this->GLRenderInPath(action);
            break;
    }
}

void SoDrawingGrid::GLRenderBelowPath(SoGLRenderAction* action)
{
    // inherited::GLRenderBelowPath(action);
    // return;
    if (action->isRenderingDelayedPaths()) {
        renderGrid(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void SoDrawingGrid::GLRenderInPath(SoGLRenderAction* action)
{
    // inherited::GLRenderInPath(action);
    // return;
    if (action->isRenderingDelayedPaths()) {
        renderGrid(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void SoDrawingGrid::GLRenderOffPath(SoGLRenderAction*)
{}

void SoDrawingGrid::generatePrimitives(SoAction* action)
{
    SoState* state = action ? action->getState() : nullptr;
    if (!state || !m_LineSet) {
        return;
    }

    state->push();
    ensureGeometry(state);
    SoModelMatrixElement::set(state, this, SbMatrix::identity());
    SoViewingMatrixElement::set(state, this, SbMatrix::identity());
    SoProjectionMatrixElement::set(state, this, SbMatrix::identity());

    const int numLines = m_LineSet->numVertices.getNum();
    const int32_t* counts = m_LineSet->numVertices.getValues(0);
    const int numVerts = m_VertexProperty->vertex.getNum();
    const SbVec3f* verts = m_VertexProperty->vertex.getValues(0);

    SoLineDetail lineDetail;
    SoPrimitiveVertex pv;

    int vIndex = 0;
    for (int lineIdx = 0; lineIdx < numLines; lineIdx++) {
        const int count = counts[lineIdx];
        if (count < 2) {
            vIndex += count;
            continue;
        }
        if (vIndex + count > numVerts) {
            break;
        }

        lineDetail.setLineIndex(lineIdx);
        lineDetail.setPartIndex(0);

        beginShape(action, LINE_STRIP, &lineDetail);
        for (int i = 0; i < count; i++) {
            pv.setPoint(verts[vIndex + i]);
            shapeVertex(&pv);
        }
        endShape();

        vIndex += count;
    }
    state->pop();
}

void SoDrawingGrid::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    (void)action;
    // Overlay grid rendered in clip/screen space: do not contribute a world-space bbox.
    box.makeEmpty();
    center.setValue(0.0f, 0.0f, 0.0f);
}
