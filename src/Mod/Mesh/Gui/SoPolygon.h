// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <vector>

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/nodes/SoShape.h>
#include <Mod/Mesh/MeshGlobal.h>

class SoCoordinate3;
class SoIndexedLineSet;
class SoMaterial;
class SoSeparator;
class SoState;
class SoIRRenderAction;

namespace MeshGui
{

// NOLINTBEGIN
/**
 * \class SoPolygon
 * \brief Draws a closed mesh-editing polygon from active coordinates.
 *
 * Place this node after the coordinate node that supplies its vertices.
 * `startIndex` and `numVertices` select the consecutive coordinate range used
 * by the polygon. The node copies that range into a private retained line-loop
 * graph during rendering, so every renderer traverses identical geometry.
 */
class MeshGuiExport SoPolygon: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoPolygon);

public:
    static void initClass();
    SoPolygon();

    /// First coordinate in the active coordinate element used by the polygon.
    SoSFInt32 startIndex;
    /// Number of consecutive coordinates that form the closed polygon.
    SoSFInt32 numVertices;
    /// Reserved selection state retained for the established node interface.
    SoSFBool highlight;
    /// Whether the polygon is rendered.
    SoSFBool render;

protected:
    ~SoPolygon() override;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void rayPick(SoRayPickAction* action) override;
    void generatePrimitives(SoAction* action) override;

private:
    void clearRenderGeometry();
    /// Synchronize the private retained graph from the active coordinate state.
    bool syncRenderGeometry(SoState* state);
    void updateLineIndices(int32_t vertexCount);
    static void IRRender(SoAction* action, SoNode* node);
    void renderAction(::SoIRRenderAction* action);

    int32_t cachedVertexCount {-1};
    std::vector<int32_t> lineIndices;
    SoSeparator* renderRoot {nullptr};
    SoCoordinate3* renderCoordinates {nullptr};
    SoMaterial* renderMaterial {nullptr};
    SoIndexedLineSet* renderLineSet {nullptr};
};
// NOLINTEND

}  // namespace MeshGui
