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

#include <cstring>

#include <Inventor/actions/SoGLRenderAction.h>

#include "SoFCStencilCap.h"


using namespace PartGui;

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
    if (hatchTexCreated) {
        return;
    }

    const int sz = 256;
    const int spacing = 64;
    const int lineWidth = 1;

    // RGB texture: white = pass-through, gray = darken
    unsigned char* img = new unsigned char[sz * sz * 3];
    std::memset(img, 255, sz * sz * 3);

    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            int idx = (y * sz + x) * 3;
            int diag = (x + y) % spacing;
            if (diag < lineWidth) {
                img[idx] = 76;
                img[idx + 1] = 76;
                img[idx + 2] = 76;
            }
        }
    }

    glGenTextures(1, &hatchTexId);
    glBindTexture(GL_TEXTURE_2D, hatchTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sz, sz, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

    delete[] img;
    hatchTexCreated = true;
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

    ensureHatchTexture();
    if (!hatchTexCreated) {
        return;
    }

    int nSolids = static_cast<int>(solidRanges.size());
    if (nSolids <= 1) {
        return;  // single solid — Coin3D handles it fine
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    // Multiply blend: framebuffer_color * texture_color
    // White texture pixels = no change, dark pixels = darken existing color
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, hatchTexId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Enable texture coordinate auto-generation (object linear)
    SbVec3f dirS = hatchDirS.getValue();
    SbVec3f dirT = hatchDirT.getValue();
    GLfloat planeS[] = {dirS[0], dirS[1], dirS[2], 0.0f};
    GLfloat planeT[] = {dirT[0], dirT[1], dirT[2], 0.0f};
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, planeS);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, planeT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, sectionVerts.data());

    float angleStep = 180.0f / nSolids;

    // Skip solid 0 — Coin3D already rendered it with the standard hatching angle.
    // Only overdraw solids 1+ with their rotated angles.
    for (int s = 1; s < nSolids; s++) {
        const auto& range = solidRanges[s];
        if (range.indexCount <= 0) {
            continue;
        }

        float angle = s * angleStep;
        if (angle != 0.0f) {
            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glRotatef(angle, 0.0f, 0.0f, 1.0f);
            glMatrixMode(GL_MODELVIEW);
        }

        // Draw this solid's triangles
        const int32_t* idx = &sectionIndices[range.indexStart];
        int count = range.indexCount;
        for (int i = 0; i < count;) {
            int start = i;
            while (i < count && idx[i] >= 0) {
                i++;
            }
            int vertCount = i - start;
            if (vertCount >= 3) {
                glDrawElements(GL_TRIANGLE_FAN, vertCount, GL_UNSIGNED_INT, &idx[start]);
            }
            i++;  // skip -1
        }

        if (angle != 0.0f) {
            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glPopAttrib();
}

void SoFCStencilCap::GLRender(SoGLRenderAction* action)
{
    (void)action;

    // Per-solid hatching overlay: render section faces with rotated textures
    if (!solidRanges.empty() && hatchEnabled.getValue()) {
        renderPerSolidHatch();
    }
}
