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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>

#include "SoFCStencilCap.h"


using namespace PartGui;

SO_NODE_SOURCE(SoFCStencilCap)

SoFCStencilCap::SoFCStencilCap()
{
    SO_NODE_CONSTRUCTOR(SoFCStencilCap);

    SO_NODE_ADD_FIELD(capCorner0, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(capCorner1, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(capCorner2, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(capCorner3, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(sectionColor, (SbColor(0.8f, 0.3f, 0.2f)));
    SO_NODE_ADD_FIELD(hatchEnabled, (true));
    SO_NODE_ADD_FIELD(hatchDirS, (SbVec3f(1, 0, 0)));
    SO_NODE_ADD_FIELD(hatchDirT, (SbVec3f(0, 1, 0)));
    // Disabled by default — enable once the OCCT path is made conditional
    SO_NODE_ADD_FIELD(enabled, (false));
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

void SoFCStencilCap::setSources(const std::vector<StencilSource>& sources)
{
    stencilSources = sources;
}

void SoFCStencilCap::ensureHatchTexture()
{
    if (hatchTexCreated) {
        return;
    }

    const int sz = 256;
    const int spacing = 64;
    const int lineWidth = 1;

    // RGB texture, MODULATE mode: white = pass-through, gray = darken
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

void SoFCStencilCap::renderStencilFill(SoGLRenderAction* action)
{
    (void)action;

    // Disable color and depth writes — stencil only
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    // Disable all clip planes so we render full geometry
    GLboolean clipEnabled[6];
    for (int i = 0; i < 6; i++) {
        clipEnabled[i] = glIsEnabled(GL_CLIP_PLANE0 + i);
        if (clipEnabled[i]) {
            glDisable(GL_CLIP_PLANE0 + i);
        }
    }

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    // Pass 1: Back faces — increment stencil on depth pass
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);  // renders back faces
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);

    for (const auto& src : stencilSources) {
        if (!src.coords || !src.faceSet) {
            continue;
        }

        glPushMatrix();
        glMultMatrixf(src.transform[0]);

        const SbVec3f* pts = src.coords->point.getValues(0);
        int numPts = src.coords->point.getNum();
        const int32_t* indices = src.faceSet->coordIndex.getValues(0);
        int numIndices = src.faceSet->coordIndex.getNum();

        if (numPts > 0 && numIndices > 0) {
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, pts);

            // Draw indexed triangles — SoIndexedFaceSet uses -1 as separator
            // We draw triangle fans for each face (typically triangulated already)
            int start = 0;
            for (int i = 0; i <= numIndices; i++) {
                if (i == numIndices || indices[i] < 0) {
                    int count = i - start;
                    if (count >= 3) {
                        glDrawElements(GL_TRIANGLE_FAN, count, GL_UNSIGNED_INT, &indices[start]);
                    }
                    start = i + 1;
                }
            }

            glDisableClientState(GL_VERTEX_ARRAY);
        }

        glPopMatrix();
    }

    // Pass 2: Front faces — decrement stencil on depth pass
    glCullFace(GL_BACK);  // renders front faces
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP);

    for (const auto& src : stencilSources) {
        if (!src.coords || !src.faceSet) {
            continue;
        }

        glPushMatrix();
        glMultMatrixf(src.transform[0]);

        const SbVec3f* pts = src.coords->point.getValues(0);
        int numPts = src.coords->point.getNum();
        const int32_t* indices = src.faceSet->coordIndex.getValues(0);
        int numIndices = src.faceSet->coordIndex.getNum();

        if (numPts > 0 && numIndices > 0) {
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, pts);

            int start = 0;
            for (int i = 0; i <= numIndices; i++) {
                if (i == numIndices || indices[i] < 0) {
                    int count = i - start;
                    if (count >= 3) {
                        glDrawElements(GL_TRIANGLE_FAN, count, GL_UNSIGNED_INT, &indices[start]);
                    }
                    start = i + 1;
                }
            }

            glDisableClientState(GL_VERTEX_ARRAY);
        }

        glPopMatrix();
    }

    glDisable(GL_CULL_FACE);

    // Restore clip planes
    for (int i = 0; i < 6; i++) {
        if (clipEnabled[i]) {
            glEnable(GL_CLIP_PLANE0 + i);
        }
    }

    // Restore writes
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
}

void SoFCStencilCap::renderCapQuad()
{
    SbVec3f c0 = capCorner0.getValue();
    SbVec3f c1 = capCorner1.getValue();
    SbVec3f c2 = capCorner2.getValue();
    SbVec3f c3 = capCorner3.getValue();

    SbColor col = sectionColor.getValue();
    glColor3f(col[0], col[1], col[2]);

    if (hatchEnabled.getValue() && hatchTexCreated) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, hatchTexId);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        // Compute texture coords by projecting cap corners onto the
        // hatching plane frame (same as SoTextureCoordinatePlane).
        SbVec3f dirS = hatchDirS.getValue();
        SbVec3f dirT = hatchDirT.getValue();

        glBegin(GL_QUADS);
        glTexCoord2f(c0.dot(dirS), c0.dot(dirT));
        glVertex3f(c0[0], c0[1], c0[2]);
        glTexCoord2f(c1.dot(dirS), c1.dot(dirT));
        glVertex3f(c1[0], c1[1], c1[2]);
        glTexCoord2f(c2.dot(dirS), c2.dot(dirT));
        glVertex3f(c2[0], c2[1], c2[2]);
        glTexCoord2f(c3.dot(dirS), c3.dot(dirT));
        glVertex3f(c3[0], c3[1], c3[2]);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    else {
        glBegin(GL_QUADS);
        glVertex3f(c0[0], c0[1], c0[2]);
        glVertex3f(c1[0], c1[1], c1[2]);
        glVertex3f(c2[0], c2[1], c2[2]);
        glVertex3f(c3[0], c3[1], c3[2]);
        glEnd();
    }
}

void SoFCStencilCap::GLRender(SoGLRenderAction* action)
{
    if (!enabled.getValue() || stencilSources.empty()) {
        return;
    }

    // Check stencil buffer availability
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (stencilBits == 0) {
        return;  // no stencil buffer — fall back to OCCT path
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Clear stencil
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Enable depth test (needed for correct stencil fill)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // --- Stencil fill pass ---
    renderStencilFill(action);

    // --- Cap rendering pass ---
    // Draw cap quad only where stencil != 0 (inside the solid)
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    // Disable lighting — we want flat section color
    glDisable(GL_LIGHTING);

    ensureHatchTexture();
    renderCapQuad();

    glDisable(GL_STENCIL_TEST);

    glPopAttrib();
}
