// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <vector>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFVec3f.h>

class SoGLRenderAction;


namespace PartGui
{

/// Per-solid hatching overlay node.
///
/// Renders section face triangles with per-solid texture matrix rotation,
/// producing different hatching angles for each body in the cross-section.
/// Uses GL multiply-blend to darken the existing per-body colors without
/// replacing them.
class SoFCStencilCap : public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(PartGui::SoFCStencilCap);

public:
    static void initClass();
    SoFCStencilCap();

    void GLRender(SoGLRenderAction* action) override;

    /// Hatching on/off.
    SoSFBool hatchEnabled;

    /// Hatching texture projection axes (same as SoTextureCoordinatePlane).
    SoSFVec3f hatchDirS;
    SoSFVec3f hatchDirT;

    /// Set section face tessellation data for per-solid hatching.
    void setSectionFaces(const SbVec3f* verts, int numVerts,
                         const int32_t* indices, int numIndices,
                         const int32_t* partIdx, int numParts,
                         const std::vector<long>& solidFaceCounts);

protected:
    ~SoFCStencilCap() override;

private:
    void renderPerSolidHatch();

    // Per-solid hatching data (copied from faceset at update time)
    struct SolidRange { int indexStart; int indexCount; };
    std::vector<SolidRange> solidRanges;
    std::vector<SbVec3f> sectionVerts;
    std::vector<int32_t> sectionIndices;

    /// Hatch texture GL name (created once, reused).
    unsigned int hatchTexId = 0;
    bool hatchTexCreated = false;
    void ensureHatchTexture();
};

}  // namespace PartGui
