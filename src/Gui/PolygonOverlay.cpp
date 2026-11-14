// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

#include "PolygonOverlay.h"

#include <algorithm>

#include <Inventor/SbViewportRegion.h>

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include "Inventor/SoFCScreenSpaceGroup.h"

namespace Gui
{

void PolygonOverlay::setMaterialColor(SoMaterial* material, const QColor& color)
{
    material->diffuseColor.setValue(
        static_cast<float>(color.redF()),
        static_cast<float>(color.greenF()),
        static_cast<float>(color.blueF())
    );
    material->transparency.setValue(1.0F - static_cast<float>(color.alphaF()));
}

void PolygonOverlay::updateGeometry(const SbViewportRegion& viewport, qreal scale)
{
    const SbVec2s size = viewport.getViewportSizePixels();
    const float width = static_cast<float>(size[0]);
    const float height = static_cast<float>(size[1]);
    const int pointCount = static_cast<int>(logicalPoints.size());
    const bool canClose = closed && pointCount > 1;
    const bool separateClosingLine = canClose && closeStippled;
    const int lineVertexCount = pointCount + (canClose && !separateClosingLine ? 1 : 0);

    for (int i = 0; i < pointCount; ++i) {
        const QPointF& point = logicalPoints[static_cast<size_t>(i)];
        const float x = std::clamp(static_cast<float>(point.x() * scale), 0.0F, width);
        const float y = std::clamp(static_cast<float>(point.y() * scale), 0.0F, height);
        vertices->vertex.set1Value(i, SbVec3f(x, height - y, 0.0F));
    }

    if (canClose && !separateClosingLine) {
        const QPointF& first = logicalPoints.front();
        const float x = std::clamp(static_cast<float>(first.x() * scale), 0.0F, width);
        const float y = std::clamp(static_cast<float>(first.y() * scale), 0.0F, height);
        vertices->vertex.set1Value(pointCount, SbVec3f(x, height - y, 0.0F));
    }
    lineSet->numVertices.setValue(lineVertexCount);

    if (separateClosingLine) {
        const QPointF& first = logicalPoints.front();
        const QPointF& last = logicalPoints.back();
        const float firstX = std::clamp(static_cast<float>(first.x() * scale), 0.0F, width);
        const float firstY = std::clamp(static_cast<float>(first.y() * scale), 0.0F, height);
        const float lastX = std::clamp(static_cast<float>(last.x() * scale), 0.0F, width);
        const float lastY = std::clamp(static_cast<float>(last.y() * scale), 0.0F, height);
        closingLineVertices->vertex.set1Value(0, SbVec3f(lastX, height - lastY, 0.0F));
        closingLineVertices->vertex.set1Value(1, SbVec3f(firstX, height - firstY, 0.0F));
    }
    closingLineSet->numVertices.setValue(separateClosingLine ? 2 : 0);
    closingLineSwitch->whichChild = separateClosingLine ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

PolygonOverlay::PolygonOverlay()
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

    auto* line = new SoSeparator;
    material = new SoMaterial;
    drawStyle = new SoDrawStyle;
    vertices = new SoVertexProperty;
    lineSet = new SoLineSet;
    lineSet->vertexProperty.setValue(vertices);
    lineSet->numVertices.setValue(0);
    line->addChild(material);
    line->addChild(drawStyle);
    line->addChild(lineSet);
    visibilitySwitch->addChild(line);

    auto* closingLine = new SoSeparator;
    closingLineMaterial = new SoMaterial;
    closingLineStyle = new SoDrawStyle;
    closingLineVertices = new SoVertexProperty;
    closingLineSet = new SoLineSet;
    closingLineSet->vertexProperty.setValue(closingLineVertices);
    closingLineSet->numVertices.setValue(0);
    closingLine->addChild(closingLineMaterial);
    closingLine->addChild(closingLineStyle);
    closingLine->addChild(closingLineSet);

    closingLineSwitch = new SoSwitch;
    closingLineSwitch->whichChild = SO_SWITCH_NONE;
    closingLineSwitch->addChild(closingLine);
    visibilitySwitch->addChild(closingLineSwitch);

    setMaterialColor(material, lineColor);
    setMaterialColor(closingLineMaterial, lineColor);
    drawStyle->lineWidth.setValue(lineWidth);
    closingLineStyle->lineWidth.setValue(lineWidth);
    closingLineStyle->linePatternScaleFactor.setValue(2);
    closingLineStyle->linePattern.setValue(0x3F3F);

    root->addChild(visibilitySwitch);
}

PolygonOverlay::~PolygonOverlay()
{
    root->unref();
}

void PolygonOverlay::setPoints(const std::vector<QPointF>& points)
{
    logicalPoints = points;
}

void PolygonOverlay::clearPoints()
{
    logicalPoints.clear();
}

void PolygonOverlay::addPoint(const QPointF& point)
{
    logicalPoints.push_back(point);
}

void PolygonOverlay::popPoint()
{
    if (!logicalPoints.empty()) {
        logicalPoints.pop_back();
    }
}

void PolygonOverlay::setClosed(bool value)
{
    closed = value;
}

void PolygonOverlay::setCloseStippled(bool value)
{
    closeStippled = value;
}

void PolygonOverlay::setColor(const QColor& color)
{
    lineColor = color;
    setMaterialColor(material, lineColor);
    setMaterialColor(closingLineMaterial, lineColor);
}

void PolygonOverlay::setLineWidth(float width)
{
    lineWidth = width;
    drawStyle->lineWidth.setValue(lineWidth);
    closingLineStyle->lineWidth.setValue(lineWidth);
}

void PolygonOverlay::setVisible(bool visible)
{
    visibilitySwitch->whichChild = visible ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}

void PolygonOverlay::prepareGeometry(const SbViewportRegion& viewport, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0) {
        return;
    }

    const SbVec2s size = viewport.getViewportSizePixels();
    if (size[0] <= 0 || size[1] <= 0) {
        return;
    }

    updateGeometry(viewport, devicePixelRatio);
}

SoNode* PolygonOverlay::sceneRoot() const
{
    return root;
}

}  // namespace Gui
