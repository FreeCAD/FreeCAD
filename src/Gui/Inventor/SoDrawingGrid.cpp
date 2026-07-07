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

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoLineSet.h>
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
    SO_NODE_INIT_CLASS(SoDrawingGrid, SoFCScreenSpaceGroup, "SoFCScreenSpaceGroup");
}

SoDrawingGrid::SoDrawingGrid()
{
    SO_NODE_CONSTRUCTOR(SoDrawingGrid);

    setCoordinateSpace(CoordinateSpace::ClipSpace);
    setBaseColorLightModel(true);
    setTexturesEnabled(false);
    setMultiTexturesEnabled(false);
    setDepthBuffer(false, false, SoDepthBufferElement::ALWAYS);

    constexpr float gridChannel = 10.0f / 255.0f;
    auto* color = new SoBaseColor;
    color->rgb.setValue(gridChannel, gridChannel, gridChannel);
    addChild(color);

    m_VertexProperty = new SoVertexProperty;
    m_LineSet = new SoLineSet;
    m_LineSet->vertexProperty.setValue(m_VertexProperty);
    addChild(m_LineSet);
}

SoDrawingGrid::~SoDrawingGrid()
{
    m_VertexProperty = nullptr;
    m_LineSet = nullptr;
}

void SoDrawingGrid::ensureGeometry(SoState* state)
{
    if (!state || !m_VertexProperty || !m_LineSet) {
        return;
    }

    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    const SbVec2s viewportSize = vp.getViewportSizePixels();
    if (viewportSize[0] <= 0 || viewportSize[1] <= 0) {
        return;
    }
    if (viewportSize == m_CachedViewportSize && m_VertexProperty->vertex.getNum() > 0) {
        return;
    }

    m_CachedViewportSize = viewportSize;

    constexpr int numX = 20;
    const float aspectRatio = static_cast<float>(viewportSize[0])
        / static_cast<float>(viewportSize[1]);
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

void SoDrawingGrid::GLRenderBelowPath(SoGLRenderAction* action)
{
    if (!action) {
        return;
    }

    if (action->isRenderingDelayedPaths()) {
        ensureGeometry(action->getState());
        inherited::GLRenderBelowPath(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void SoDrawingGrid::GLRenderInPath(SoGLRenderAction* action)
{
    if (!action) {
        return;
    }

    if (action->isRenderingDelayedPaths()) {
        ensureGeometry(action->getState());
        inherited::GLRenderInPath(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        action->addDelayedPath(action->getCurPath()->copy());
    }
}

void SoDrawingGrid::GLRenderOffPath(SoGLRenderAction*)
{}

void SoDrawingGrid::doAction(SoAction* action)
{
    inherited::doAction(action);
}
