// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Kustaa Nyholm <kustaa.nyholm@sparetimelabs.com>
// SPDX-FileCopyrightText: 2025 Joao Matos
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
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec4f.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/details/SoCubeDetail.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/SoPrimitiveVertex.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <limits>
#include <numbers>

#include "SoNaviCube.h"

using namespace Gui;

SO_NODE_SOURCE(SoNaviCube);

namespace
{

}  // namespace

void SoNaviCube::initClass()
{
    SO_NODE_INIT_CLASS(SoNaviCube, SoShape, "Shape");
}

SoNaviCube::SoNaviCube()
{
    SO_NODE_CONSTRUCTOR(SoNaviCube);
    SO_NODE_ADD_FIELD(size, (1.0F));
    SO_NODE_ADD_FIELD(opacity, (1.0F));
    SO_NODE_ADD_FIELD(borderWidth, (1.0F));
    SO_NODE_ADD_FIELD(showCoordinateSystem, (TRUE));
    SO_NODE_ADD_FIELD(baseColor, (SbColor(0.886F, 0.909F, 0.937F)));
    SO_NODE_ADD_FIELD(baseAlpha, (1.0F));
    SO_NODE_ADD_FIELD(emphaseColor, (SbColor(0.0F, 0.4F, 0.8F)));
    SO_NODE_ADD_FIELD(emphaseAlpha, (1.0F));
    SO_NODE_ADD_FIELD(hiliteColor, (SbColor(0.666F, 0.886F, 1.0F)));
    SO_NODE_ADD_FIELD(hiliteAlpha, (1.0F));
    SO_NODE_ADD_FIELD(axisXColor, (SbColor(1.0F, 0.0F, 0.0F)));
    SO_NODE_ADD_FIELD(axisYColor, (SbColor(0.0F, 1.0F, 0.0F)));
    SO_NODE_ADD_FIELD(axisZColor, (SbColor(0.0F, 0.0F, 1.0F)));
    SO_NODE_ADD_FIELD(hiliteId, (static_cast<int>(PickId::None)));
    SO_NODE_ADD_FIELD(cameraOrientation, (SbRotation()));
    SO_NODE_ADD_FIELD(cameraIsOrthographic, (TRUE));
    SO_NODE_ADD_FIELD(viewportRect, (SbVec4f(0.0F, 0.0F, 0.0F, 0.0F)));
}

void SoNaviCube::setChamfer(float chamfer)
{
    float clamped = std::clamp(chamfer, 0.05F, 0.18F);
    if (std::abs(clamped - m_Chamfer) > std::numeric_limits<float>::epsilon()) {
        m_Chamfer = clamped;
        m_GeometryDirty = true;
        touch();
    }
}

const SoNaviCube::FaceMap& SoNaviCube::faces() const
{
    ensureGeometry();
    return m_Faces;
}

const SoNaviCube::Face* SoNaviCube::faceForId(PickId id) const
{
    ensureGeometry();
    auto it = m_Faces.find(id);
    if (it != m_Faces.end()) {
        return &it->second;
    }
    return nullptr;
}

const SoNaviCube::FaceMap& SoNaviCube::buttonFaces() const
{
    ensureGeometry();
    return m_ButtonFaces;
}

const SoNaviCube::Face* SoNaviCube::buttonFaceForId(PickId id) const
{
    ensureGeometry();
    auto it = m_ButtonFaces.find(id);
    if (it != m_ButtonFaces.end()) {
        return &it->second;
    }
    return nullptr;
}

const SoNaviCube::LabelMap& SoNaviCube::labelSlots() const
{
    ensureGeometry();
    return m_LabelSlots;
}

const SoNaviCube::LabelSlot* SoNaviCube::labelSlotForId(PickId id) const
{
    ensureGeometry();
    auto it = m_LabelSlots.find(id);
    if (it != m_LabelSlots.end()) {
        return &it->second;
    }
    return nullptr;
}

void SoNaviCube::setLabelTexture(PickId id, unsigned int textureId)
{
    ensureGeometry();
    m_LabelSlots[id].textureId = textureId;
}

void SoNaviCube::clearLabelTextures()
{
    for (auto& [_, slot] : m_LabelSlots) {
        slot.textureId = 0;
    }
}

void SoNaviCube::renderGL(bool pickMode) const
{
    const SbVec4f& rect = viewportRect.getValue();
    const int viewportX = static_cast<int>(std::lround(rect[0]));
    const int viewportY = static_cast<int>(std::lround(rect[1]));
    const int viewportWidth = static_cast<int>(std::lround(rect[2]));
    const int viewportHeight = static_cast<int>(std::lround(rect[3]));

    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    const float currentOpacity = opacity.getValue();
    const bool showCS = showCoordinateSystem.getValue();
    const bool cameraOrthographic = cameraIsOrthographic.getValue();
    const float borderWidthValue = borderWidth.getValue();
    const ColorWithAlpha base = {baseColor.getValue(), baseAlpha.getValue()};
    const ColorWithAlpha emph = {emphaseColor.getValue(), emphaseAlpha.getValue()};
    const ColorWithAlpha hilite = {hiliteColor.getValue(), hiliteAlpha.getValue()};
    const SbColor axisRGB[3] = {axisXColor.getValue(), axisYColor.getValue(), axisZColor.getValue()};
    const PickId hilitePick = static_cast<PickId>(hiliteId.getValue());
    const SbRotation camOrientation = cameraOrientation.getValue();

    ensureGeometry();

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0F, 1.0F);
    glClearDepth(1.0F);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_LIGHTING);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (pickMode) {
        glDisable(GL_BLEND);
        glShadeModel(GL_FLAT);
        glDisable(GL_DITHER);
        glDisable(GL_POLYGON_SMOOTH);
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0F, 1.0F);
        glEnable(GL_BLEND);
        glShadeModel(GL_SMOOTH);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    constexpr float NEARVAL = 0.1F;
    constexpr float FARVAL = 10.1F;
    if (cameraOrthographic) {
        glOrtho(-2.1, 2.1, -2.1, 2.1, NEARVAL, FARVAL);
    }
    else {
        const float dim = NEARVAL * static_cast<float>(std::tan(std::numbers::pi / 8.0)) * 1.1F;
        glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    SbMatrix mx;
    camOrientation.getValue(mx);
    mx = mx.inverse();
    mx[3][0] = 0.0F;
    mx[3][1] = 0.0F;
    mx[3][2] = -5.1F;
    glLoadMatrixf(reinterpret_cast<const float*>(&mx[0][0]));

    glEnableClientState(GL_VERTEX_ARRAY);

    if (!pickMode && showCS) {
        glLineWidth(borderWidthValue * 2.0F);
        glPointSize(borderWidthValue * 2.0F);
        constexpr float a = -1.1F;
        constexpr float b = -1.05F;
        constexpr float c = 0.5F;
        const float pointData[] = {b, a, a, c, a, a, a, b, a, a, c, a, a, a, b, a, a, c, a, a, a};
        glVertexPointer(3, GL_FLOAT, 0, pointData);

        const float* xColor = axisRGB[0].getValue();
        glColor4f(xColor[0], xColor[1], xColor[2], currentOpacity);
        glDrawArrays(GL_LINES, 0, 2);
        glDrawArrays(GL_POINTS, 0, 2);

        const float* yColor = axisRGB[1].getValue();
        glColor4f(yColor[0], yColor[1], yColor[2], currentOpacity);
        glDrawArrays(GL_LINES, 2, 2);
        glDrawArrays(GL_POINTS, 2, 2);

        const float* zColor = axisRGB[2].getValue();
        glColor4f(zColor[0], zColor[1], zColor[2], currentOpacity);
        glDrawArrays(GL_LINES, 4, 2);
        glDrawArrays(GL_POINTS, 4, 2);
    }

    const auto& baseFaces = faces();
    for (const auto& [pickId, face] : baseFaces) {
        if (face.vertexArray.empty() && face.trianglesArray.empty()) {
            continue;
        }
        if (pickMode) {
            glColor3ub(static_cast<GLubyte>(pickId), 0, 0);
        }
        else {
            const auto& color = hilitePick == pickId ? hilite : base;
            const float* rgb = color.rgb.getValue();
            glColor4f(rgb[0], rgb[1], rgb[2], color.alpha * currentOpacity);
        }
        if (!face.trianglesArray.empty()) {
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.trianglesArray.data()));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(face.trianglesArray.size()));
        }
        else if (!face.vertexArray.empty()) {
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.vertexArray.data()));
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(face.vertexArray.size()));
        }
    }

    if (!pickMode) {
        glLineWidth(borderWidthValue);
        const float* borderRGB = emph.rgb.getValue();
        const float borderAlpha = emph.alpha * currentOpacity;
        for (const auto& [pickId, face] : baseFaces) {
            (void)pickId;
            if (face.vertexArray.empty()) {
                continue;
            }
            glColor4f(borderRGB[0], borderRGB[1], borderRGB[2], borderAlpha);
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.vertexArray.data()));
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(face.vertexArray.size()));
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        static const float texCoords[] = {0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F};
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

        const float* emphRGB = emph.rgb.getValue();
        const float emphAlpha = emph.alpha * currentOpacity;
        glColor4f(emphRGB[0], emphRGB[1], emphRGB[2], emphAlpha);

        const auto& labels = labelSlots();
        for (const auto& [pickId, slot] : labels) {
            (void)pickId;
            if (slot.quad.size() != 4 || slot.textureId == 0) {
                continue;
            }
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(slot.quad.data()));
            glBindTexture(GL_TEXTURE_2D, slot.textureId);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }

    const auto& buttons = buttonFaces();
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& [pickId, face] : buttons) {
        // 1. Allow faces that use explicit triangles to pass through
        if (face.vertexArray.empty() && face.trianglesArray.empty()) {
            continue;
        }

        if (pickMode) {
            glColor3ub(static_cast<GLubyte>(pickId), 0, 0);
        }
        else {
            const auto& color = hilitePick == pickId ? hilite : base;
            const float* rgb = color.rgb.getValue();
            glColor4f(rgb[0], rgb[1], rgb[2], color.alpha * currentOpacity);
        }

        // 2. Draw the filled shape using triangles if available, else fallback to fans
        if (!face.trianglesArray.empty()) {
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.trianglesArray.data()));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(face.trianglesArray.size()));
        }
        else if (!face.vertexArray.empty()) {
            glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.vertexArray.data()));
            glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(face.vertexArray.size()));
        }

        // 3. Draw the outlines
        if (!pickMode) {
            const float* rgb = emph.rgb.getValue();
            glColor4f(rgb[0], rgb[1], rgb[2], emph.alpha * currentOpacity);
            if (!face.lineLoops.empty()) {
                for (const auto& loop : face.lineLoops) {
                    if (loop.empty()) {
                        continue;
                    }
                    glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(loop.data()));
                    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(loop.size()));
                }
            }
            else if (!face.vertexArray.empty()) {
                glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const float*>(face.vertexArray.data()));
                glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(face.vertexArray.size()));
            }
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

void SoNaviCube::GLRender(SoGLRenderAction* action)
{
    if (!shouldGLRender(action)) {
        return;
    }
    ensureGeometry();
    renderGL(false);
}

void SoNaviCube::generatePrimitives(SoAction*)
{}

void SoNaviCube::computeBBox(SoAction*, SbBox3f& box, SbVec3f& center)
{
    const float halfSize = size.getValue() * 0.5F;
    box.setBounds(SbVec3f(-halfSize, -halfSize, -halfSize), SbVec3f(halfSize, halfSize, halfSize));
    center.setValue(0.0F, 0.0F, 0.0F);
}

void SoNaviCube::ensureGeometry() const
{
    if (!m_GeometryDirty) {
        return;
    }
    rebuildGeometry();
}

void SoNaviCube::rebuildGeometry() const
{
    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float piHalf = pi / 2.0F;

    LabelMap oldLabelSlots;
    oldLabelSlots.swap(m_LabelSlots);

    m_Faces.clear();
    m_ButtonFaces.clear();
    m_LabelSlots.clear();

    const SbVec3f x(1.0F, 0.0F, 0.0F);
    const SbVec3f y(0.0F, 1.0F, 0.0F);
    const SbVec3f z(0.0F, 0.0F, 1.0F);

    // Main faces
    addCubeFace(x, z, FaceType::Main, PickId::Top, 0.0F);
    addCubeFace(x, -y, FaceType::Main, PickId::Front, 0.0F);
    addCubeFace(-y, -x, FaceType::Main, PickId::Left, 0.0F);
    addCubeFace(-x, y, FaceType::Main, PickId::Rear, 0.0F);
    addCubeFace(y, x, FaceType::Main, PickId::Right, 0.0F);
    addCubeFace(x, -z, FaceType::Main, PickId::Bottom, 0.0F);

    // Corner faces
    addCubeFace(-x - y, x - y + z, FaceType::Corner, PickId::FrontTopRight, pi);
    addCubeFace(-x + y, -x - y + z, FaceType::Corner, PickId::FrontTopLeft, pi);
    addCubeFace(x + y, x - y - z, FaceType::Corner, PickId::FrontBottomRight, 0.0F);
    addCubeFace(x - y, -x - y - z, FaceType::Corner, PickId::FrontBottomLeft, 0.0F);
    addCubeFace(x - y, x + y + z, FaceType::Corner, PickId::RearTopRight, pi);
    addCubeFace(x + y, -x + y + z, FaceType::Corner, PickId::RearTopLeft, pi);
    addCubeFace(-x + y, x + y - z, FaceType::Corner, PickId::RearBottomRight, 0.0F);
    addCubeFace(-x - y, -x + y - z, FaceType::Corner, PickId::RearBottomLeft, 0.0F);

    // Edge faces
    addCubeFace(x, z - y, FaceType::Edge, PickId::FrontTop, 0.0F);
    addCubeFace(x, -z - y, FaceType::Edge, PickId::FrontBottom, 0.0F);
    addCubeFace(x, y - z, FaceType::Edge, PickId::RearBottom, pi);
    addCubeFace(x, y + z, FaceType::Edge, PickId::RearTop, pi);
    addCubeFace(z, x + y, FaceType::Edge, PickId::RearRight, piHalf);
    addCubeFace(z, x - y, FaceType::Edge, PickId::FrontRight, piHalf);
    addCubeFace(z, -x - y, FaceType::Edge, PickId::FrontLeft, piHalf);
    addCubeFace(z, y - x, FaceType::Edge, PickId::RearLeft, piHalf);
    addCubeFace(y, z - x, FaceType::Edge, PickId::TopLeft, pi);
    addCubeFace(y, x + z, FaceType::Edge, PickId::TopRight, 0.0F);
    addCubeFace(y, x - z, FaceType::Edge, PickId::BottomRight, 0.0F);
    addCubeFace(y, -z - x, FaceType::Edge, PickId::BottomLeft, pi);

    auto setLabelQuad = [&](PickId pickId, const SbVec3f& xVec, const SbVec3f& zVec) {
        auto& slot = m_LabelSlots[pickId];
        slot.quad.clear();

        SbVec3f negZ = zVec;
        negZ.negate();

        SbVec3f yVec = xVec.cross(negZ);
        SbVec3f x2 = xVec * (1.0F - m_Chamfer * 2.0F);
        SbVec3f y2 = yVec * (1.0F - m_Chamfer * 2.0F);

        slot.quad.reserve(4);
        slot.quad.emplace_back(zVec - x2 - y2);
        slot.quad.emplace_back(zVec + x2 - y2);
        slot.quad.emplace_back(zVec + x2 + y2);
        slot.quad.emplace_back(zVec - x2 + y2);

        auto it = oldLabelSlots.find(pickId);
        if (it != oldLabelSlots.end()) {
            slot.textureId = it->second.textureId;
        }
    };

    setLabelQuad(PickId::Top, x, z);
    setLabelQuad(PickId::Front, x, -y);
    setLabelQuad(PickId::Left, -y, -x);
    setLabelQuad(PickId::Rear, -x, y);
    setLabelQuad(PickId::Right, y, x);
    setLabelQuad(PickId::Bottom, x, -z);

    rebuildButtonFaces();

    m_GeometryDirty = false;
}

void SoNaviCube::addCubeFace(const SbVec3f& x, const SbVec3f& z, FaceType type, PickId pickId, float rotZ) const
{
    auto& face = m_Faces[pickId];
    face.type = type;
    face.vertexArray.clear();

    SbVec3f y = x.cross(-z);

    SbVec3f xN = x;
    SbVec3f yN = y;
    SbVec3f zN = z;
    xN.normalize();
    yN.normalize();
    zN.normalize();

    SbMatrix R(xN[0], yN[0], zN[0], 0, xN[1], yN[1], zN[1], 0, xN[2], yN[2], zN[2], 0, 0, 0, 0, 1);

    face.rotation = (SbRotation(R) * SbRotation(SbVec3f(0, 0, 1), rotZ)).inverse();

    if (type == FaceType::Corner) {
        SbVec3f xC = x * m_Chamfer;
        SbVec3f yC = y * m_Chamfer;
        SbVec3f zC = z * (1.0F - 2.0F * m_Chamfer);

        face.vertexArray.reserve(6);
        face.vertexArray.emplace_back(zC - xC * 2.0F);
        face.vertexArray.emplace_back((zC - xC) - yC);
        face.vertexArray.emplace_back((zC + xC) - yC);
        face.vertexArray.emplace_back(zC + xC * 2.0F);
        face.vertexArray.emplace_back((zC + xC) + yC);
        face.vertexArray.emplace_back((zC - xC) + yC);
    }
    else if (type == FaceType::Edge) {
        SbVec3f x4 = x * (1.0F - m_Chamfer * 4.0F);
        SbVec3f yE = y * m_Chamfer;
        SbVec3f zE = z * (1.0F - m_Chamfer);

        face.vertexArray.reserve(4);
        face.vertexArray.emplace_back((zE - x4) - yE);
        face.vertexArray.emplace_back((zE + x4) - yE);
        face.vertexArray.emplace_back((zE + x4) + yE);
        face.vertexArray.emplace_back((zE - x4) + yE);
    }
    else if (type == FaceType::Main) {
        SbVec3f x2 = x * (1.0F - m_Chamfer * 2.0F);
        SbVec3f y2 = y * (1.0F - m_Chamfer * 2.0F);
        SbVec3f x4 = x * (1.0F - m_Chamfer * 4.0F);
        SbVec3f y4 = y * (1.0F - m_Chamfer * 4.0F);

        face.vertexArray.reserve(8);
        face.vertexArray.emplace_back((z - x2) - y4);
        face.vertexArray.emplace_back((z - x4) - y2);
        face.vertexArray.emplace_back((z + x4) - y2);
        face.vertexArray.emplace_back((z + x2) - y4);
        face.vertexArray.emplace_back((z + x2) + y4);
        face.vertexArray.emplace_back((z + x4) + y2);
        face.vertexArray.emplace_back((z - x4) + y2);
        face.vertexArray.emplace_back((z - x2) + y4);
    }
}

void SoNaviCube::rebuildButtonFaces() const
{
    m_ButtonFaces.clear();
    addButtonFace(PickId::ArrowNorth, SbVec3f(-1, 0, 0));
    addButtonFace(PickId::ArrowSouth, SbVec3f(1, 0, 0));
    addButtonFace(PickId::ArrowEast, SbVec3f(0, 1, 0));
    addButtonFace(PickId::ArrowWest, SbVec3f(0, -1, 0));
    addButtonFace(PickId::ArrowLeft, SbVec3f(0, 0, 1));
    addButtonFace(PickId::ArrowRight, SbVec3f(0, 0, -1));
    addButtonFace(PickId::Backside, SbVec3f(0, 1, 0));
    addButtonFace(PickId::Home);
    addButtonFace(PickId::ViewMenu);
}

void SoNaviCube::addButtonFace(PickId pickId, const SbVec3f& direction) const
{
    auto& face = m_ButtonFaces[pickId];
    face.vertexArray.clear();
    face.trianglesArray.clear();
    face.lineLoops.clear();
    float scale = 0.005F;
    float offx = 0.5F;
    float offy = 0.5F;
    std::vector<float> pointData;

    auto transform = [&](float px, float py) {
        float x = px * scale + offx;
        float y = py * scale + offy;
        if (pickId == PickId::ArrowNorth || pickId == PickId::ArrowWest
            || pickId == PickId::ArrowLeft) {
            x = 1.0F - x;
        }
        if (pickId == PickId::ArrowSouth || pickId == PickId::ArrowNorth) {
            return SbVec3f(y, x, 0.0F);
        }
        else {
            return SbVec3f(x, y, 0.0F);
        }
    };

    switch (pickId) {
        default:
            break;
        case PickId::ArrowRight:
        case PickId::ArrowLeft: {
            pointData = {66.6F,  -66.6F, 58.3F,  -74.0F, 49.2F,  -80.3F, 39.4F,  -85.5F, 29.0F,
                         -89.5F, 25.3F,  -78.1F, 34.3F,  -74.3F, 42.8F,  -69.9F, 50.8F,  -64.4F,
                         58.1F,  -58.1F, 53.8F,  -53.8F, 74.7F,  -46.8F, 70.7F,  -70.4F};
            break;
        }
        case PickId::ArrowWest:
        case PickId::ArrowNorth:
        case PickId::ArrowSouth:
        case PickId::ArrowEast: {
            pointData = {100.0F, 0.0F, 80.0F, -18.0F, 80.0F, 18.0F};
            break;
        }
        case PickId::ViewMenu: {
            offx = 0.90F;
            offy = 0.95F;

            // 1. The small bar on top
            std::vector<float> ptsBar = {-13.0F, -20.0F, 13.0F, -20.0F, 13.0F, -16.0F, -13.0F, -16.0F};
            std::vector<SbVec3f> loopBar;
            for (size_t i = 0; i < ptsBar.size(); i += 2) {
                loopBar.push_back(transform(ptsBar[i], ptsBar[i + 1]));
            }
            face.lineLoops.push_back(loopBar);

            face.trianglesArray.push_back(loopBar[0]);
            face.trianglesArray.push_back(loopBar[1]);
            face.trianglesArray.push_back(loopBar[2]);
            face.trianglesArray.push_back(loopBar[0]);
            face.trianglesArray.push_back(loopBar[2]);
            face.trianglesArray.push_back(loopBar[3]);

            // 2. The triangle pointing down
            std::vector<float> ptsTri = {-13.0F, -12.0F, 13.0F, -12.0F, 0.0F, 0.0F};
            std::vector<SbVec3f> loopTri;
            for (size_t i = 0; i < ptsTri.size(); i += 2) {
                loopTri.push_back(transform(ptsTri[i], ptsTri[i + 1]));
            }
            face.lineLoops.push_back(loopTri);

            face.trianglesArray.push_back(loopTri[0]);
            face.trianglesArray.push_back(loopTri[1]);
            face.trianglesArray.push_back(loopTri[2]);
            break;
        }
        case PickId::Home: {
            offx = 0.09F;
            offy = 0.09F;
            std::vector<float> pts = {0.0F,   -18.0F, 18.0F,  -6.0F, 12.0F,  -6.0F, 12.0F, 8.0F,
                                      4.0F,   8.0F,   4.0F,   0.0F,  -4.0F,  0.0F,  -4.0F, 8.0F,
                                      -12.0F, 8.0F,   -12.0F, -6.0F, -18.0F, -6.0F};
            std::vector<SbVec3f> loop;
            for (size_t i = 0; i < pts.size(); i += 2) {
                loop.push_back(transform(pts[i], pts[i + 1]));
            }
            face.lineLoops.push_back(loop);
            face.trianglesArray.push_back(transform(-18, -6));
            face.trianglesArray.push_back(transform(18, -6));
            face.trianglesArray.push_back(transform(0, -18));
            face.trianglesArray.push_back(transform(-12, -6));
            face.trianglesArray.push_back(transform(12, -6));
            face.trianglesArray.push_back(transform(12, 0));
            face.trianglesArray.push_back(transform(-12, -6));
            face.trianglesArray.push_back(transform(12, 0));
            face.trianglesArray.push_back(transform(-12, 0));
            face.trianglesArray.push_back(transform(-12, 0));
            face.trianglesArray.push_back(transform(-4, 0));
            face.trianglesArray.push_back(transform(-4, 8));
            face.trianglesArray.push_back(transform(-12, 0));
            face.trianglesArray.push_back(transform(-4, 8));
            face.trianglesArray.push_back(transform(-12, 8));
            face.trianglesArray.push_back(transform(4, 0));
            face.trianglesArray.push_back(transform(12, 0));
            face.trianglesArray.push_back(transform(12, 8));
            face.trianglesArray.push_back(transform(4, 0));
            face.trianglesArray.push_back(transform(12, 8));
            face.trianglesArray.push_back(transform(4, 8));
            break;
        }
        case PickId::Backside: {
            offx = 0.80F;
            offy = 0.0F;
            std::vector<float> pts1 = {24.,  21.5, 17.,  29.1, 16.7, 25.6, 12.0, 25.3, 8.2,
                                       24.0, 4.,   22.,  1.2,  19.,  0.,   15.,  0.,   10.,
                                       1.5,  8.1,  4.4,  6.1,  8.0,  5.5,  14.,  4.,   14.6,
                                       9.2,  10.1, 10.2, 6.,   12.,  3.5,  14.,  3.4,  14.5,
                                       6.0,  15.8, 10.,  17.,  16.3, 18.,  16.3, 13.6};
            std::vector<SbVec3f> loop1;
            for (size_t i = 0; i < pts1.size(); i += 2) {
                loop1.push_back(transform(pts1[i], pts1[i + 1]));
            }
            face.lineLoops.push_back(loop1);
            std::vector<int> idx1 = {0,  1,  2, 0,  2,  20, 0,  20, 21, 2,  20, 3,  20, 19, 3,
                                     3,  19, 4, 19, 18, 4,  4,  18, 5,  18, 17, 5,  5,  17, 6,
                                     17, 16, 6, 6,  16, 7,  16, 15, 7,  7,  15, 8,  15, 14, 8,
                                     8,  14, 9, 14, 13, 9,  9,  13, 10, 10, 13, 11, 11, 13, 12};
            for (int i : idx1) {
                face.trianglesArray.push_back(loop1[i]);
            }

            std::vector<float> pts2 = {18.,  6.,   22.6, 0.,   22.5, 3.0,  27.,  3.3,  31.4,
                                       4.3,  35.0, 5.6,  37.5, 7.1,  40.,  9.7,  40.,  12.8,
                                       38.5, 16.5, 36.2, 20.,  33.,  21.9, 28.3, 22.9, 28.7,
                                       16.,  32.8, 15.,  36.4, 12.9, 33.8, 10.8, 30.,  9.3,
                                       26.1, 8.5,  22.5, 8.1,  22.4, 10.8};
            std::vector<SbVec3f> loop2;
            for (size_t i = 0; i < pts2.size(); i += 2) {
                loop2.push_back(transform(pts2[i], pts2[i + 1]));
            }
            face.lineLoops.push_back(loop2);
            std::vector<int> idx2 = {0,  1,  2, 0,  2,  19, 0,  19, 20, 2,  19, 3, 19, 18, 3,
                                     3,  18, 4, 18, 17, 4,  4,  17, 5,  17, 16, 5, 5,  16, 6,
                                     16, 15, 6, 6,  15, 7,  15, 14, 7,  7,  14, 8, 14, 13, 8,
                                     8,  13, 9, 9,  13, 10, 10, 13, 11, 11, 13, 12};
            for (int i : idx2) {
                face.trianglesArray.push_back(loop2[i]);
            }
            break;
        }
    }

    if (!pointData.empty()) {
        int count = static_cast<int>(pointData.size()) / 2;
        face.vertexArray.reserve(count);
        for (int i = 0; i < count; i++) {
            face.vertexArray.push_back(transform(pointData[i * 2], pointData[i * 2 + 1]));
        }
    }
    face.type = FaceType::Button;
    face.rotation = SbRotation(direction, 1).inverse();
}
