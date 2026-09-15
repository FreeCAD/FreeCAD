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

#pragma once

#include <QColor>
#include <QRectF>

class SbViewportRegion;
class SoDrawStyle;
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
 * Retained rubber-band overlay owned by a 3D viewer.
 *
 * Rectangle geometry is stored in logical widget coordinates. The viewer
 * prepares it for the current physical viewport and owns traversal of the
 * retained scene root.
 */
class RubberbandOverlay
{
public:
    explicit RubberbandOverlay();
    ~RubberbandOverlay();

    RubberbandOverlay(const RubberbandOverlay&) = delete;
    RubberbandOverlay& operator=(const RubberbandOverlay&) = delete;

    /** Store the rectangle in logical widget coordinates. */
    void setRectangle(const QRectF& rectangle);

    /** Set the retained rubber-band border color. */
    void setBorderColor(const QColor& color);

    /** Show or hide the retained rubber-band rectangle. */
    void setVisible(bool visible);

    /**
     * Convert retained geometry to the supplied physical viewport.
     *
     * This prepares the scene root for traversal but does not render it.
     */
    void prepareGeometry(const SbViewportRegion& viewport, qreal devicePixelRatio);

    /** Return the retained scene root owned by this overlay. */
    SoNode* sceneRoot() const;

private:
    void updateGeometry(const QRectF& viewportRect, qreal scale);
    static void setMaterialColor(SoMaterial* material, const QColor& color);

    QRectF logicalRectangle;
    Inventor::SoFCScreenSpaceGroup* root {nullptr};
    SoSwitch* visibilitySwitch {nullptr};
    SoMaterial* fillMaterial {nullptr};
    SoMaterial* borderMaterial {nullptr};
    SoDrawStyle* borderStyle {nullptr};
    SoVertexProperty* fillVertices {nullptr};
    SoVertexProperty* borderVertices {nullptr};
};

}  // namespace Gui
