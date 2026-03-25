// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
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

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFVec3f.h>

class SoCoordinate3;
class SoIndexedFaceSet;
class SoGLRenderAction;


namespace PartGui
{

/// Geometry source for the stencil fill pass — references a source
/// object's tessellated face data and its world transform.
struct StencilSource
{
    SoCoordinate3* coords = nullptr;      // vertex positions
    SoIndexedFaceSet* faceSet = nullptr;  // triangle indices
    SbMatrix transform;                   // model → world
};

/// GPU-based cross-section cap using the stencil buffer.
///
/// Algorithm:
///   1. Clear stencil to 0
///   2. Disable color/depth writes, disable clip planes
///   3. Render source geometry back-faces: stencil++ on depth pass
///   4. Render source geometry front-faces: stencil-- on depth pass
///   5. Re-enable clip planes, color/depth writes
///   6. Draw cap quad where stencil != 0
///
/// The result is a filled cross-section at the clip plane, without
/// any OCCT boolean computation.
class SoFCStencilCap: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(PartGui::SoFCStencilCap);

public:
    static void initClass();
    SoFCStencilCap();

    void GLRender(SoGLRenderAction* action) override;

    /// Set the source geometry to render during the stencil pass.
    void setSources(const std::vector<StencilSource>& sources);

    /// Cap quad corners (world space).
    SoSFVec3f capCorner0;
    SoSFVec3f capCorner1;
    SoSFVec3f capCorner2;
    SoSFVec3f capCorner3;

    /// Section appearance.
    SoSFColor sectionColor;
    SoSFBool hatchEnabled;

    /// Hatching texture projection axes (same as SoTextureCoordinatePlane).
    SoSFVec3f hatchDirS;
    SoSFVec3f hatchDirT;

    /// Enable/disable stencil capping (allows fallback to OCCT path).
    SoSFBool enabled;

protected:
    ~SoFCStencilCap() override;

private:
    void renderStencilFill(SoGLRenderAction* action);
    void renderCapQuad();

    std::vector<StencilSource> stencilSources;

    /// Hatch texture GL name (created once, reused).
    unsigned int hatchTexId = 0;
    bool hatchTexCreated = false;
    void ensureHatchTexture();
};

}  // namespace PartGui
