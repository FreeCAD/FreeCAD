// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                     *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "RubberbandOverlay.h"

#include <algorithm>

#include <Inventor/SbViewportRegion.h>

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include "Inventor/SoFCScreenSpaceGroup.h"

namespace Gui
{

void RubberbandOverlay::setMaterialColor(SoMaterial* material, const QColor& color)
{
    material->diffuseColor.setValue(
        static_cast<float>(color.redF()),
        static_cast<float>(color.greenF()),
        static_cast<float>(color.blueF())
    );
    material->transparency.setValue(1.0F - static_cast<float>(color.alphaF()));
}

void RubberbandOverlay::updateGeometry(const QRectF& viewportRect, qreal scale)
{
    const QRectF logical = logicalRectangle.normalized();
    const float width = static_cast<float>(viewportRect.width());
    const float height = static_cast<float>(viewportRect.height());

    const float left = std::clamp(static_cast<float>(logical.left() * scale), 0.0F, width);
    const float right = std::clamp(static_cast<float>(logical.right() * scale), 0.0F, width);
    const float top = std::clamp(static_cast<float>(logical.top() * scale), 0.0F, height);
    const float bottom = std::clamp(static_cast<float>(logical.bottom() * scale), 0.0F, height);
    const float lower = height - bottom;
    const float upper = height - top;

    fillVertices->vertex.set1Value(0, SbVec3f(left, lower, 0.0F));
    fillVertices->vertex.set1Value(1, SbVec3f(left, upper, 0.0F));
    fillVertices->vertex.set1Value(2, SbVec3f(right, upper, 0.0F));
    fillVertices->vertex.set1Value(3, SbVec3f(right, lower, 0.0F));

    borderVertices->vertex.set1Value(0, SbVec3f(left, lower, 0.0F));
    borderVertices->vertex.set1Value(1, SbVec3f(left, upper, 0.0F));
    borderVertices->vertex.set1Value(2, SbVec3f(right, upper, 0.0F));
    borderVertices->vertex.set1Value(3, SbVec3f(right, lower, 0.0F));
    borderVertices->vertex.set1Value(4, SbVec3f(left, lower, 0.0F));
}

RubberbandOverlay::RubberbandOverlay()
{
    root = new Inventor::SoFCScreenSpaceGroup;
    root->ref();
    root->setCoordinateSpace(Inventor::SoFCScreenSpaceGroup::CoordinateSpace::ViewportPixels);
    root->setBaseColorLightModel(true);
    root->setTexturesEnabled(false);
    root->setMultiTexturesEnabled(false);
    root->setDepthBuffer(false, false);

    visibilitySwitch = new SoSwitch;
    visibilitySwitch->whichChild = SO_SWITCH_NONE;

    auto* fill = new SoSeparator;
    fillMaterial = new SoMaterial;
    fillVertices = new SoVertexProperty;
    auto* fillFaces = new SoFaceSet;
    fillFaces->vertexProperty.setValue(fillVertices);
    fillFaces->numVertices.setValue(4);
    fill->addChild(fillMaterial);
    fill->addChild(fillFaces);
    visibilitySwitch->addChild(fill);

    auto* border = new SoSeparator;
    borderMaterial = new SoMaterial;
    borderStyle = new SoDrawStyle;
    borderVertices = new SoVertexProperty;
    auto* borderLines = new SoLineSet;
    borderLines->vertexProperty.setValue(borderVertices);
    borderLines->numVertices.setValue(5);
    border->addChild(borderMaterial);
    border->addChild(borderStyle);
    border->addChild(borderLines);
    visibilitySwitch->addChild(border);

    setMaterialColor(fillMaterial, QColor(255, 255, 255, 128));
    setMaterialColor(borderMaterial, Qt::white);
    borderStyle->lineWidth.setValue(4.0F);
    borderStyle->linePatternScaleFactor.setValue(3);
    borderStyle->linePattern.setValue(0xAAAA);
    root->addChild(visibilitySwitch);
}

RubberbandOverlay::~RubberbandOverlay()
{
    root->unref();
}

void RubberbandOverlay::setRectangle(const QRectF& rectangle)
{
    logicalRectangle = rectangle;
}

void RubberbandOverlay::setBorderColor(const QColor& color)
{
    setMaterialColor(borderMaterial, color);
}

void RubberbandOverlay::setVisible(bool visible)
{
    visibilitySwitch->whichChild = visible ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

void RubberbandOverlay::prepareGeometry(const SbViewportRegion& viewport, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0) {
        return;
    }

    const SbVec2s size = viewport.getViewportSizePixels();
    if (size[0] <= 0 || size[1] <= 0) {
        return;
    }

    const QRectF viewportRect(0.0, 0.0, size[0], size[1]);
    updateGeometry(viewportRect, devicePixelRatio);
}

SoNode* RubberbandOverlay::sceneRoot() const
{
    return root;
}

}  // namespace Gui
