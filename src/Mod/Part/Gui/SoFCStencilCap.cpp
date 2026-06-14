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

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif

#include <cmath>
#include <cstring>

#include <Inventor/actions/SoGLRenderAction.h>

#include "SoFCStencilCap.h"


using namespace PartGui;

// -----------------------------------------------------------------------
// GL helpers — isolated for future renderer migration (Vulkan/Metal/etc.)
// -----------------------------------------------------------------------

namespace
{

/// Create a small tileable diagonal-line texture (16x16 RGB).
/// Returns the GL texture ID.  The texture uses GL_REPEAT wrapping
/// so a single tile covers any area.
GLuint createHatchTexture()
{
    const int sz = 16;
    const int lineWidth = 1;

    unsigned char img[sz * sz * 3];
    std::memset(img, 255, sizeof(img));

    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int idx = (y * sz + x) * 3;
            if (((x + y) % sz) < lineWidth) {
                img[idx] = 76;
                img[idx + 1] = 76;
                img[idx + 2] = 76;
            }
        }
    }

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sz, sz, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    return texId;
}

/// Set up GL state for multiply-blend hatching overlay.
/// After this call, drawing white-textured geometry darkens the
/// framebuffer where the texture has dark pixels.
void beginHatchOverlay(GLuint texId, const float dirS[4], const float dirT[4])
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);

    // Multiply blend: result = framebuffer * texture
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Auto-generate texture coordinates via object-linear projection
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, dirS);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, dirT);
}

void endHatchOverlay()
{
    glPopAttrib();
}

/// Rotate the GL texture matrix by `angleDeg` degrees around Z.
void pushTextureRotation(float angleDeg)
{
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
}

void popTextureRotation()
{
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/// Draw indexed triangle fans from a coordIndex array (uses -1 as separator).
void drawIndexedFaces(const int32_t* indices, int count)
{
    for (int i = 0; i < count;) {
        int start = i;
        while (i < count && indices[i] >= 0) {
            i++;
        }
        int vertCount = i - start;
        if (vertCount >= 3) {
            glDrawElements(GL_TRIANGLE_FAN, vertCount, GL_UNSIGNED_INT, &indices[start]);
        }
        i++;  // skip -1
    }
}

}  // anonymous namespace


// -----------------------------------------------------------------------
// SoFCStencilCap — Coin3D node for per-solid hatching overlay
// -----------------------------------------------------------------------

SO_NODE_SOURCE(SoFCStencilCap)

SoFCStencilCap::SoFCStencilCap()
{
    SO_NODE_CONSTRUCTOR(SoFCStencilCap);

    SO_NODE_ADD_FIELD(hatchEnabled, (true));
    SO_NODE_ADD_FIELD(hatchDirS, (SbVec3f(1, 0, 0)));
    SO_NODE_ADD_FIELD(hatchDirT, (SbVec3f(0, 1, 0)));
}

SoFCStencilCap::~SoFCStencilCap()
{
    if (hatchTexCreated) {
        glDeleteTextures(1, &hatchTexId);
    }
}

void SoFCStencilCap::initClass()
{
    SO_NODE_INIT_CLASS(SoFCStencilCap, SoNode, "Node");
}

void SoFCStencilCap::ensureHatchTexture()
{
    if (!hatchTexCreated) {
        hatchTexId = createHatchTexture();
        hatchTexCreated = true;
    }
}

void SoFCStencilCap::setSectionFaces(
    const SbVec3f* verts,
    int numVerts,
    const int32_t* indices,
    int numIndices,
    const int32_t* partIdx,
    int numParts,
    const std::vector<long>& solidFaceCounts
)
{
    sectionVerts.assign(verts, verts + numVerts);
    sectionIndices.assign(indices, indices + numIndices);

    // Compute per-solid index ranges using partIndex + solidFaceCounts
    solidRanges.clear();
    int faceStart = 0;

    // Build a map: partIndex face -> coordIndex offset
    std::vector<int> faceCoordStart;
    faceCoordStart.push_back(0);
    int currentPi = 0;
    int currentTri = 0;
    for (int i = 0; i < numIndices && currentPi < numParts; i++) {
        if (indices[i] < 0) {
            currentTri++;
            if (currentTri >= partIdx[currentPi]) {
                currentPi++;
                currentTri = 0;
                faceCoordStart.push_back(i + 1);
            }
        }
    }

    for (size_t s = 0; s < solidFaceCounts.size(); s++) {
        int numFaces = solidFaceCounts[s];
        int piStart = faceStart;
        int piEnd = faceStart + numFaces;

        int cStart = (piStart < (int)faceCoordStart.size()) ? faceCoordStart[piStart] : numIndices;
        int cEnd = (piEnd < (int)faceCoordStart.size()) ? faceCoordStart[piEnd] : numIndices;

        solidRanges.push_back({cStart, cEnd - cStart});
        faceStart += numFaces;
    }
}

void SoFCStencilCap::renderPerSolidHatch()
{
    if (solidRanges.empty() || sectionVerts.empty() || sectionIndices.empty()) {
        return;
    }

    int nSolids = static_cast<int>(solidRanges.size());
    if (nSolids <= 1) {
        return;  // single solid — Coin3D handles it
    }

    ensureHatchTexture();
    if (!hatchTexCreated) {
        return;
    }

    SbVec3f dirS = hatchDirS.getValue();
    SbVec3f dirT = hatchDirT.getValue();
    GLfloat planeS[] = {dirS[0], dirS[1], dirS[2], 0.0f};
    GLfloat planeT[] = {dirT[0], dirT[1], dirT[2], 0.0f};

    beginHatchOverlay(hatchTexId, planeS, planeT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, sectionVerts.data());

    float angleStep = 180.0f / nSolids;

    // Skip solid 0 — Coin3D rendered it with the standard hatching angle.
    // Only overdraw solids 1+ with rotated angles.
    for (int s = 1; s < nSolids; s++) {
        const auto& range = solidRanges[s];
        if (range.indexCount <= 0) {
            continue;
        }

        float angle = s * angleStep;
        if (angle != 0.0f) {
            pushTextureRotation(angle);
        }

        drawIndexedFaces(&sectionIndices[range.indexStart], range.indexCount);

        if (angle != 0.0f) {
            popTextureRotation();
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);

    endHatchOverlay();
}

void SoFCStencilCap::GLRender(SoGLRenderAction* action)
{
    (void)action;

    if (!solidRanges.empty() && hatchEnabled.getValue()) {
        renderPerSolidHatch();
    }
}
