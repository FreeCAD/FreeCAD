// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <QColor>
#include <QPointF>

#include <vector>

class SbViewportRegion;
class SoDrawStyle;
class SoLineSet;
class SoMaterial;
class SoNode;
class SoSwitch;
class SoVertexProperty;

namespace Gui
{

namespace Inventor
{
class SoFCScreenSpaceGroup;
}

/**
 * Retained polygon overlay owned by a 3D viewer.
 *
 * Points are stored in logical widget coordinates. The viewer prepares the
 * retained scene root for the current physical viewport before traversing it.
 */
class PolygonOverlay
{
public:
    PolygonOverlay();
    ~PolygonOverlay();

    PolygonOverlay(const PolygonOverlay&) = delete;
    PolygonOverlay& operator=(const PolygonOverlay&) = delete;

    void setPoints(const std::vector<QPointF>& points);
    void clearPoints();
    void addPoint(const QPointF& point);
    void popPoint();

    void setClosed(bool closed);
    void setCloseStippled(bool stippled);
    void setColor(const QColor& color);
    void setLineWidth(float width);
    void setVisible(bool visible);

    /** Prepare the retained geometry for traversal in the supplied viewport. */
    void prepareGeometry(const SbViewportRegion& viewport, qreal devicePixelRatio);

    /** Return the retained scene root owned by this overlay. */
    SoNode* sceneRoot() const;

private:
    void updateGeometry(const SbViewportRegion& viewport, qreal scale);
    static void setMaterialColor(SoMaterial* material, const QColor& color);

    std::vector<QPointF> logicalPoints;
    QColor lineColor {Qt::white};
    float lineWidth {2.0F};
    bool closed {true};
    bool closeStippled {false};

    Inventor::SoFCScreenSpaceGroup* root {nullptr};
    SoSwitch* visibilitySwitch {nullptr};
    SoSwitch* closingLineSwitch {nullptr};
    SoMaterial* material {nullptr};
    SoMaterial* closingLineMaterial {nullptr};
    SoDrawStyle* drawStyle {nullptr};
    SoDrawStyle* closingLineStyle {nullptr};
    SoVertexProperty* vertices {nullptr};
    SoVertexProperty* closingLineVertices {nullptr};
    SoLineSet* lineSet {nullptr};
    SoLineSet* closingLineSet {nullptr};
};

}  // namespace Gui
