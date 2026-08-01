// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025-2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <numbers>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/lists/SoPickedPointList.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/system/gl.h>

#include "SoNaviCube.h"

using namespace Gui;

SO_NODE_SOURCE(SoNaviCube);

namespace
{

constexpr std::array<SoNaviCube::PickId, 26> kCubeFacePickOrder = {
    SoNaviCube::PickId::Top,
    SoNaviCube::PickId::Front,
    SoNaviCube::PickId::Left,
    SoNaviCube::PickId::Rear,
    SoNaviCube::PickId::Right,
    SoNaviCube::PickId::Bottom,
    SoNaviCube::PickId::FrontTopRight,
    SoNaviCube::PickId::FrontTopLeft,
    SoNaviCube::PickId::FrontBottomRight,
    SoNaviCube::PickId::FrontBottomLeft,
    SoNaviCube::PickId::RearTopRight,
    SoNaviCube::PickId::RearTopLeft,
    SoNaviCube::PickId::RearBottomRight,
    SoNaviCube::PickId::RearBottomLeft,
    SoNaviCube::PickId::FrontTop,
    SoNaviCube::PickId::FrontBottom,
    SoNaviCube::PickId::RearBottom,
    SoNaviCube::PickId::RearTop,
    SoNaviCube::PickId::RearRight,
    SoNaviCube::PickId::FrontRight,
    SoNaviCube::PickId::FrontLeft,
    SoNaviCube::PickId::RearLeft,
    SoNaviCube::PickId::TopLeft,
    SoNaviCube::PickId::TopRight,
    SoNaviCube::PickId::BottomRight,
    SoNaviCube::PickId::BottomLeft
};

constexpr std::array<SoNaviCube::PickId, 6> kLabelPickIds = {
    SoNaviCube::PickId::Top,
    SoNaviCube::PickId::Front,
    SoNaviCube::PickId::Left,
    SoNaviCube::PickId::Rear,
    SoNaviCube::PickId::Right,
    SoNaviCube::PickId::Bottom
};

constexpr std::array<SoNaviCube::PickId, 9> kButtonPickIds = {
    SoNaviCube::PickId::ArrowNorth,
    SoNaviCube::PickId::ArrowSouth,
    SoNaviCube::PickId::ArrowEast,
    SoNaviCube::PickId::ArrowWest,
    SoNaviCube::PickId::ArrowRight,
    SoNaviCube::PickId::ArrowLeft,
    SoNaviCube::PickId::Backside,
    SoNaviCube::PickId::Home,
    SoNaviCube::PickId::ViewMenu
};

constexpr int cubeFaceIndex(SoNaviCube::PickId id)
{
    for (size_t i = 0; i < kCubeFacePickOrder.size(); ++i) {
        if (kCubeFacePickOrder[i] == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/** Restores the OpenGL state touched while clearing the NaviCube overlay depth. */
class ScopedDepthClearState
{
public:
    ScopedDepthClearState()
    {
        scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteMask);
        glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    }

    ScopedDepthClearState(const ScopedDepthClearState&) = delete;
    ScopedDepthClearState& operator=(const ScopedDepthClearState&) = delete;

    ~ScopedDepthClearState() noexcept
    {
        glScissor(
            scissorBox[0],
            scissorBox[1],
            static_cast<GLsizei>(scissorBox[2]),
            static_cast<GLsizei>(scissorBox[3])
        );
        if (scissorEnabled == GL_TRUE) {
            glEnable(GL_SCISSOR_TEST);
        }
        else {
            glDisable(GL_SCISSOR_TEST);
        }
        glDepthMask(depthWriteMask);
        glClearDepth(clearDepth);
    }

private:
    GLboolean scissorEnabled {GL_FALSE};
    GLint scissorBox[4] {};
    GLboolean depthWriteMask {GL_TRUE};
    GLdouble clearDepth {1.0};
};

/** Clears only the depth buffer in the NaviCube viewport, leaving color output untouched. */
void clearOverlayDepth(int viewportX, int viewportY, int viewportWidth, int viewportHeight)
{
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    ScopedDepthClearState state;

    glEnable(GL_SCISSOR_TEST);
    glScissor(
        viewportX,
        viewportY,
        static_cast<GLsizei>(viewportWidth),
        static_cast<GLsizei>(viewportHeight)
    );
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

bool pointInTriangle2D(const SbVec2f& p, const SbVec2f& a, const SbVec2f& b, const SbVec2f& c)
{
    const float px = p[0];
    const float py = p[1];
    const float ax = a[0];
    const float ay = a[1];
    const float bx = b[0];
    const float by = b[1];
    const float cx = c[0];
    const float cy = c[1];

    const float v0x = cx - ax;
    const float v0y = cy - ay;
    const float v1x = bx - ax;
    const float v1y = by - ay;
    const float v2x = px - ax;
    const float v2y = py - ay;

    const float dot00 = v0x * v0x + v0y * v0y;
    const float dot01 = v0x * v1x + v0y * v1y;
    const float dot02 = v0x * v2x + v0y * v2y;
    const float dot11 = v1x * v1x + v1y * v1y;
    const float dot12 = v1x * v2x + v1y * v2y;

    const float denom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(denom) < 1e-18F) {
        return false;
    }
    const float invDenom = 1.0F / denom;
    const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return (u >= 0.0F) && (v >= 0.0F) && (u + v <= 1.0F);
}

bool triangulatePolygon2D(const std::vector<SbVec3f>& verts, std::vector<int>& triIndices)
{
    triIndices.clear();
    const int n = static_cast<int>(verts.size());
    if (n < 3) {
        return false;
    }

    double signedArea2 = 0.0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const double xi = verts[i][0];
        const double yi = verts[i][1];
        const double xj = verts[j][0];
        const double yj = verts[j][1];
        signedArea2 += (xj * yi) - (xi * yj);
    }
    if (std::abs(signedArea2) < 1e-12) {
        return false;
    }
    const bool wantCCW = (signedArea2 > 0.0);

    auto crossZ = [&](int ia, int ib, int ic) {
        const double abx = verts[ib][0] - verts[ia][0];
        const double aby = verts[ib][1] - verts[ia][1];
        const double bcx = verts[ic][0] - verts[ib][0];
        const double bcy = verts[ic][1] - verts[ib][1];
        return abx * bcy - aby * bcx;
    };

    std::vector<int> remaining;
    remaining.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        remaining.push_back(i);
    }

    auto isConvex = [&](int ia, int ib, int ic) {
        const double z = crossZ(ia, ib, ic);
        return wantCCW ? (z > 1e-18) : (z < -1e-18);
    };

    constexpr int MAX_ITERS_MULT = 4;
    int iters = 0;
    while (remaining.size() > 3 && iters < MAX_ITERS_MULT * n) {
        bool clipped = false;
        const size_t m = remaining.size();
        for (size_t i = 0; i < m; ++i) {
            const int ib = remaining[i];
            const int ia = remaining[(i + m - 1) % m];
            const int ic = remaining[(i + 1) % m];

            if (!isConvex(ia, ib, ic)) {
                continue;
            }

            const SbVec2f a(verts[ia][0], verts[ia][1]);
            const SbVec2f b(verts[ib][0], verts[ib][1]);
            const SbVec2f c(verts[ic][0], verts[ic][1]);

            bool anyInside = false;
            for (size_t k = 0; k < m; ++k) {
                const int ip = remaining[k];
                if (ip == ia || ip == ib || ip == ic) {
                    continue;
                }
                const SbVec2f p(verts[ip][0], verts[ip][1]);
                if (pointInTriangle2D(p, a, b, c)) {
                    anyInside = true;
                    break;
                }
            }
            if (anyInside) {
                continue;
            }

            if (wantCCW) {
                triIndices.push_back(ia);
                triIndices.push_back(ib);
                triIndices.push_back(ic);
            }
            else {
                triIndices.push_back(ic);
                triIndices.push_back(ib);
                triIndices.push_back(ia);
            }
            remaining.erase(remaining.begin() + static_cast<std::vector<int>::difference_type>(i));
            clipped = true;
            break;
        }
        if (!clipped) {
            triIndices.clear();
            return false;
        }
        ++iters;
    }

    if (remaining.size() == 3) {
        const int ia = remaining[0];
        const int ib = remaining[1];
        const int ic = remaining[2];
        if (wantCCW) {
            triIndices.push_back(ia);
            triIndices.push_back(ib);
            triIndices.push_back(ic);
        }
        else {
            triIndices.push_back(ic);
            triIndices.push_back(ib);
            triIndices.push_back(ia);
        }
        return true;
    }

    triIndices.clear();
    return false;
}

float toTransparency(float alpha)
{
    alpha = std::clamp(alpha, 0.0F, 1.0F);
    return 1.0F - alpha;
}

bool nearlyEqual(float a, float b, float eps = 1e-6F)
{
    return std::abs(a - b) <= eps;
}

bool nearlyEqual(const SbColor& a, const SbColor& b, float eps = 1e-6F)
{
    return nearlyEqual(a[0], b[0], eps) && nearlyEqual(a[1], b[1], eps)
        && nearlyEqual(a[2], b[2], eps);
}

constexpr float OVERLAY_NEAR = 0.1F;
constexpr float OVERLAY_FAR = 10.1F;
constexpr float OVERLAY_ORTHO_EXTENT = 2.1F;
constexpr float OVERLAY_FOV_SCALE = 1.1F;
constexpr float OVERLAY_CUBE_Z = -5.1F;
constexpr float OVERLAY_BUTTON_Z = -4.0F;  // in front of the cube (cube is centered at z≈-5.1)
constexpr float BACKSIDE_HIT_LEFT = 0.79F;
constexpr float BACKSIDE_HIT_TOP = 0.0F;
constexpr float BACKSIDE_HIT_RIGHT = 1.0F;
constexpr float BACKSIDE_HIT_BOTTOM = 0.16F;

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

SoNaviCube::~SoNaviCube()
{
    if (sceneRoot) {
        sceneRoot->unref();
        sceneRoot = nullptr;
    }
    clearLabelTextures();
}

void SoNaviCube::setChamfer(float chamfer)
{
    float clamped = std::clamp(chamfer, 0.05F, 0.18F);
    if (std::abs(clamped - this->chamfer) > std::numeric_limits<float>::epsilon()) {
        this->chamfer = clamped;
        geometryDirty = true;
        sceneDirty = true;
        touch();
    }
}

void SoNaviCube::setLabelImage(PickId id, const SbVec2s& size, int numComponents, const unsigned char* pixels)
{
    ensureGeometry();

    auto& slot = labelSlots[pickIndex(id)];
    if (!pixels || size[0] <= 0 || size[1] <= 0 || numComponents <= 0) {
        if (slot.texture) {
            slot.texture->unref();
            slot.texture = nullptr;
            if (labelTextureCount > 0) {
                --labelTextureCount;
            }
        }
        sceneDirty = true;
        return;
    }

    if (!slot.texture) {
        slot.texture = new SoTexture2();
        slot.texture->ref();
        ++labelTextureCount;
        slot.texture->wrapS = SoTexture2::CLAMP;
        slot.texture->wrapT = SoTexture2::CLAMP;
        // Match legacy GL behavior: labels are white text in the texture, and we modulate them
        // with the current material color (usually the emphase/border color).
        slot.texture->model = SoTexture2::MODULATE;
        slot.texture->enableCompressedTexture = FALSE;
    }
    else {
        // Keep the model consistent in case the slot was created before this policy.
        slot.texture->model = SoTexture2::MODULATE;
    }

    slot.texture->image.setValue(size, numComponents, pixels, SoSFImage::COPY);
    sceneDirty = true;
}

void SoNaviCube::clearLabelTextures()
{
    for (auto& slot : labelSlots) {
        if (slot.texture) {
            slot.texture->unref();
            slot.texture = nullptr;
        }
    }
    labelTextureCount = 0;
    sceneDirty = true;
}

void SoNaviCube::ensureSceneGraph() const
{
    if (!sceneRoot) {
        sceneRoot = new SoSeparator;
        sceneRoot->ref();
        sceneDirty = true;
    }

    if (sceneDirty) {
        rebuildSceneGraph();
    }
}

void SoNaviCube::resetSceneGraph() const
{
    if (!sceneRoot) {
        return;
    }

    sceneRoot->removeAllChildren();
    for (auto& nodes : labelNodes) {
        nodes = {};
    }
    for (auto& nodes : buttonNodes) {
        nodes = {};
    }
    style.lastHiliteFaceIndex = -1;
    style.lastHilitePick = PickId::None;
    style.buttonDirty = true;
    style.labelDirty = true;
    style.axisDirty = true;
    style.lastButtonsHilitePick = PickId::None;
    style.showCS = false;

    cameraSwitch = nullptr;
    orthoCamera = nullptr;
    perspCamera = nullptr;
    rootTransform = nullptr;
    axisSwitch = nullptr;
    for (AxisNodes& nodes : axisNodes) {
        nodes = {};
    }
    cubeSep = nullptr;
    cubeMaterial = nullptr;
    cubeVertexProperty = nullptr;
    cubeFaces = nullptr;
    edgeSep = nullptr;
    edgeMaterial = nullptr;
    edgeDrawStyle = nullptr;
    edgeVertexProperty = nullptr;
    edges = nullptr;
    labelsSep = nullptr;
    buttonsSep = nullptr;
}

void SoNaviCube::buildCamerasAndStyle() const
{
    if (!sceneRoot) {
        return;
    }

    cameraSwitch = new SoSwitch;
    sceneRoot->addChild(cameraSwitch);

    orthoCamera = new SoOrthographicCamera;
    cameraSwitch->addChild(orthoCamera);

    perspCamera = new SoPerspectiveCamera;
    cameraSwitch->addChild(perspCamera);

    // The overlay uses a fixed projection independent of viewport aspect ratio, matching the
    // legacy GL code path. (Non-square viewports will therefore stretch the overlay similarly.)
    cameraSwitch->whichChild = 0;

    // Force filled rendering for the overlay by default, regardless of viewer draw style.
    {
        auto* drawStyle = new SoDrawStyle;
        drawStyle->style = SoDrawStyle::FILLED;
        sceneRoot->addChild(drawStyle);
    }

    {
        using Mapping = SoCamera::ViewportMapping;
        // Legacy projection assumed a fixed (square) aspect ratio.
        orthoCamera->viewportMapping = Mapping::LEAVE_ALONE;
        perspCamera->viewportMapping = Mapping::LEAVE_ALONE;
        orthoCamera->aspectRatio = 1.0F;
        perspCamera->aspectRatio = 1.0F;

        orthoCamera->position = SbVec3f(0.0F, 0.0F, 0.0F);
        perspCamera->position = SbVec3f(0.0F, 0.0F, 0.0F);
        orthoCamera->orientation = SbRotation();
        perspCamera->orientation = SbRotation();

        orthoCamera->nearDistance = OVERLAY_NEAR;
        orthoCamera->farDistance = OVERLAY_FAR;
        perspCamera->nearDistance = OVERLAY_NEAR;
        perspCamera->farDistance = OVERLAY_FAR;
        orthoCamera->focalDistance = std::abs(OVERLAY_CUBE_Z);
        perspCamera->focalDistance = std::abs(OVERLAY_CUBE_Z);

        // Equivalent to ortho [-2.1..2.1] in Y.
        orthoCamera->height = 2.0F * OVERLAY_ORTHO_EXTENT;

        // Match the legacy frustum: dim = near * tan(pi/8) * scale.
        const float halfAngle = std::atan(
            std::tan(std::numbers::pi_v<float> / 8.0F) * OVERLAY_FOV_SCALE
        );
        perspCamera->heightAngle = 2.0F * halfAngle;
    }
}

void SoNaviCube::buildCubeSection() const
{
    if (!sceneRoot) {
        return;
    }

    auto* cubeGroup = new SoSeparator;
    sceneRoot->addChild(cubeGroup);

    // Ensure the overlay depth behavior is stable regardless of viewer state: after the depth-clear
    // pass we want normal depth testing for the cube/labels/axes (buttons handle their own depth
    // settings separately).
    {
        auto* depth = new SoDepthBuffer;
        depth->test = TRUE;
        depth->write = TRUE;
        depth->function = SoDepthBuffer::LEQUAL;
        depth->range = SbVec2f(0.0F, 1.0F);
        cubeGroup->addChild(depth);
    }

    rootTransform = new SoTransform;
    cubeGroup->addChild(rootTransform);

    buildAxisSection(cubeGroup);

    cubeSep = new SoSeparator;
    cubeGroup->addChild(cubeSep);

    cubeMaterial = new SoMaterial;
    cubeSep->addChild(cubeMaterial);

    // Explicit binding node to match legacy behavior and avoid inheriting upstream binding state.
    {
        auto* binding = new SoMaterialBinding;
        binding->value = SoMaterialBinding::PER_FACE_INDEXED;
        cubeSep->addChild(binding);
    }

    // Match the legacy render ordering: offset filled faces slightly so edges/labels can be drawn
    // "on top" without z-fighting.
    {
        auto* offset = new SoPolygonOffset;
        offset->factor = 1.0F;
        offset->units = 1.0F;
        offset->styles = SoPolygonOffset::FILLED;
        offset->on = TRUE;
        cubeSep->addChild(offset);

        // Ensure consistent front-face orientation and enable solid face culling for the cube.
        auto* hints = new SoShapeHints;
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = SoShapeHints::SOLID;
        hints->faceType = SoShapeHints::CONVEX;
        cubeSep->addChild(hints);
    }

    cubeFaces = new SoIndexedFaceSet;
    cubeSep->addChild(cubeFaces);

    {
        cubeVertexProperty = new SoVertexProperty;
        cubeFaces->vertexProperty = cubeVertexProperty;
        cubeVertexProperty->materialBinding = SoVertexProperty::PER_FACE_INDEXED;
        if (!cubeCoordsData.empty()) {
            cubeVertexProperty->vertex
                .setValues(0, static_cast<int>(cubeCoordsData.size()), cubeCoordsData.data());
        }
        if (!cubeCoordIndexData.empty()) {
            cubeFaces->coordIndex.setValues(
                0,
                static_cast<int>(cubeCoordIndexData.size()),
                cubeCoordIndexData.data()
            );
        }
        const int faceCount = static_cast<int>(kCubeFacePickOrder.size());
        cubeFaces->materialIndex.setNum(faceCount);
        for (int i = 0; i < faceCount; ++i) {
            cubeFaces->materialIndex.set1Value(i, 0);
        }
    }

    edgeSep = new SoSeparator;
    cubeGroup->addChild(edgeSep);

    edgeMaterial = new SoMaterial;
    edgeSep->addChild(edgeMaterial);

    edgeDrawStyle = new SoDrawStyle;
    edgeSep->addChild(edgeDrawStyle);

    edges = new SoIndexedLineSet;
    edgeSep->addChild(edges);

    {
        edgeVertexProperty = new SoVertexProperty;
        edges->vertexProperty = edgeVertexProperty;
        if (!edgeCoordsData.empty()) {
            edgeVertexProperty->vertex
                .setValues(0, static_cast<int>(edgeCoordsData.size()), edgeCoordsData.data());
        }
        if (!edgeCoordIndexData.empty()) {
            edges->coordIndex.setValues(
                0,
                static_cast<int>(edgeCoordIndexData.size()),
                edgeCoordIndexData.data()
            );
        }
    }

    labelsSep = new SoSeparator;
    cubeGroup->addChild(labelsSep);

    // Labels should render slightly "on top" of cube faces and without being affected by any
    // backface culling settings inherited from the axis/cube/edges.
    SoSeparator* labelsGroup = nullptr;
    {
        labelsGroup = new SoSeparator;
        labelsSep->addChild(labelsGroup);

        auto* depth = new SoDepthBuffer;
        depth->test = TRUE;
        depth->write = FALSE;
        depth->function = SoDepthBuffer::LEQUAL;
        labelsGroup->addChild(depth);

        auto* offset = new SoPolygonOffset;
        offset->factor = -1.0F;
        offset->units = -1.0F;
        offset->styles = SoPolygonOffset::FILLED;
        offset->on = TRUE;
        labelsGroup->addChild(offset);

        auto* hints = new SoShapeHints;
        hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        labelsGroup->addChild(hints);
    }

    for (PickId pickId : kLabelPickIds) {
        const auto& slot = labelSlots[pickIndex(pickId)];
        if (slot.quad.size() != 4 || !slot.texture) {
            continue;
        }

        LabelNodes nodes;
        nodes.sep = new SoSeparator;

        nodes.material = new SoMaterial;
        nodes.sep->addChild(nodes.material);

        nodes.texture = slot.texture;
        nodes.sep->addChild(nodes.texture);

        nodes.face = new SoFaceSet;
        nodes.face->numVertices.set1Value(0, 4);
        nodes.vertexProperty = new SoVertexProperty;
        nodes.face->vertexProperty = nodes.vertexProperty;
        nodes.vertexProperty->vertex.setNum(4);
        nodes.vertexProperty->texCoord.setNum(4);
        nodes.vertexProperty->texCoord.set1Value(0, SbVec2f(0.0F, 0.0F));
        nodes.vertexProperty->texCoord.set1Value(1, SbVec2f(1.0F, 0.0F));
        nodes.vertexProperty->texCoord.set1Value(2, SbVec2f(1.0F, 1.0F));
        nodes.vertexProperty->texCoord.set1Value(3, SbVec2f(0.0F, 1.0F));
        for (int i = 0; i < 4; ++i) {
            nodes.vertexProperty->vertex.set1Value(i, slot.quad[static_cast<size_t>(i)]);
        }
        nodes.sep->addChild(nodes.face);

        labelsGroup->addChild(nodes.sep);
        labelNodes[pickIndex(pickId)] = nodes;
    }
}

void SoNaviCube::buildAxisSection(SoSeparator* cubeGroup) const
{
    if (!cubeGroup) {
        return;
    }

    axisSwitch = new SoSwitch;
    cubeGroup->addChild(axisSwitch);

    constexpr float a = -1.1F;
    constexpr float b = -1.05F;
    constexpr float c = 0.5F;
    const SbVec3f axisSegments[3][2] = {
        {SbVec3f(b, a, a), SbVec3f(c, a, a)},
        {SbVec3f(a, b, a), SbVec3f(a, c, a)},
        {SbVec3f(a, a, b), SbVec3f(a, a, c)}
    };

    for (int axis = 0; axis < 3; ++axis) {
        AxisNodes nodes;
        nodes.sep = new SoSeparator;

        nodes.material = new SoMaterial;
        nodes.sep->addChild(nodes.material);

        nodes.drawStyle = new SoDrawStyle;
        nodes.sep->addChild(nodes.drawStyle);

        nodes.vertexProperty = new SoVertexProperty;
        nodes.vertexProperty->vertex.setNum(2);
        nodes.vertexProperty->vertex.set1Value(0, axisSegments[axis][0]);
        nodes.vertexProperty->vertex.set1Value(1, axisSegments[axis][1]);

        nodes.line = new SoIndexedLineSet;
        nodes.line->vertexProperty = nodes.vertexProperty;
        const std::int32_t lineIdx[3] = {0, 1, -1};
        nodes.line->coordIndex.setValues(0, 3, lineIdx);
        nodes.sep->addChild(nodes.line);

        nodes.points = new SoPointSet;
        nodes.points->vertexProperty = nodes.vertexProperty;
        nodes.points->numPoints = 2;
        nodes.sep->addChild(nodes.points);

        axisSwitch->addChild(nodes.sep);
        axisNodes[axis] = nodes;
    }
}

void SoNaviCube::buildButtonsSection() const
{
    if (!sceneRoot) {
        return;
    }

    auto* buttonsGroup = new SoSeparator;
    sceneRoot->addChild(buttonsGroup);

    buttonsSep = new SoSeparator;
    buttonsGroup->addChild(buttonsSep);

    // Buttons are UI controls and should not be depth-tested against the cube. Some button meshes
    // are concave or have inconsistent winding, so we also avoid relying on culling.
    {
        auto* depth = new SoDepthBuffer;
        depth->test = FALSE;
        depth->write = FALSE;
        depth->function = SoDepthBuffer::ALWAYS;
        buttonsSep->addChild(depth);

        auto* hints = new SoShapeHints;
        hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        buttonsSep->addChild(hints);
    }

    SbViewVolume ortho;
    ortho.ortho(
        -OVERLAY_ORTHO_EXTENT,
        OVERLAY_ORTHO_EXTENT,
        -OVERLAY_ORTHO_EXTENT,
        OVERLAY_ORTHO_EXTENT,
        OVERLAY_NEAR,
        OVERLAY_FAR
    );

    SbViewVolume persp;
    const float dim = OVERLAY_NEAR * static_cast<float>(std::tan(std::numbers::pi / 8.0F))
        * OVERLAY_FOV_SCALE;
    persp.frustum(-dim, dim, -dim, dim, OVERLAY_NEAR, OVERLAY_FAR);

    const float buttonPlaneDist = std::abs(OVERLAY_BUTTON_Z);

    for (PickId pickId : kButtonPickIds) {
        const auto& verts = buttonOverlayVerts[pickIndex(pickId)];
        const size_t n = verts.size();
        if (n < 3) {
            continue;
        }

        ButtonNodes nodes;
        nodes.sep = new SoSeparator;

        nodes.fillMaterial = new SoMaterial;
        nodes.sep->addChild(nodes.fillMaterial);

        nodes.coordsSwitch = new SoSwitch;
        nodes.sep->addChild(nodes.coordsSwitch);

        nodes.vertexOrtho = new SoVertexProperty;
        nodes.vertexOrtho->vertex.setNum(static_cast<int>(n));
        nodes.coordsSwitch->addChild(nodes.vertexOrtho);

        nodes.vertexPersp = new SoVertexProperty;
        nodes.vertexPersp->vertex.setNum(static_cast<int>(n));
        nodes.coordsSwitch->addChild(nodes.vertexPersp);

        // Default: match the current camera mode. (updateSceneGraph() keeps it in sync.)
        nodes.coordsSwitch->whichChild = cameraIsOrthographic.getValue() ? 0 : 1;

        for (int i = 0; i < static_cast<int>(n); ++i) {
            const SbVec3f& overlayPoint = verts[static_cast<size_t>(i)];
            const SbVec2f normPoint(overlayPoint[0], 1.0F - overlayPoint[1]);
            nodes.vertexOrtho->vertex.set1Value(i, ortho.getPlanePoint(buttonPlaneDist, normPoint));
            nodes.vertexPersp->vertex.set1Value(i, persp.getPlanePoint(buttonPlaneDist, normPoint));
        }

        nodes.fill = new SoIndexedFaceSet;
        {
            std::vector<std::int32_t> idx;
            const auto& tris = buttonTriangleIndices[pickIndex(pickId)];
            if (tris.size() >= 3) {
                idx.reserve(tris.size() + (tris.size() / 3));
                for (size_t i = 0; i + 2 < tris.size(); i += 3) {
                    idx.push_back(tris[i]);
                    idx.push_back(tris[i + 1]);
                    idx.push_back(tris[i + 2]);
                    idx.push_back(-1);
                }
            }
            else {
                const std::int32_t n32 = static_cast<std::int32_t>(n);
                idx.reserve((n32 - 2) * 4);
                for (std::int32_t i = 1; i + 1 < n32; ++i) {
                    idx.push_back(0);
                    idx.push_back(i);
                    idx.push_back(i + 1);
                    idx.push_back(-1);
                }
            }
            nodes.fill->coordIndex.setValues(0, static_cast<int>(idx.size()), idx.data());
        }
        nodes.sep->addChild(nodes.fill);

        nodes.outlineMaterial = new SoMaterial;
        nodes.sep->addChild(nodes.outlineMaterial);

        nodes.outlineDrawStyle = new SoDrawStyle;
        nodes.sep->addChild(nodes.outlineDrawStyle);

        nodes.outline = new SoIndexedLineSet;
        {
            std::vector<std::int32_t> idx;
            const auto& outlineIdx = buttonOutlineIndices[pickIndex(pickId)];
            if (!outlineIdx.empty()) {
                idx = outlineIdx;
            }
            else {
                idx.reserve(n + 2);
                for (std::int32_t i = 0; i < static_cast<std::int32_t>(n); ++i) {
                    idx.push_back(i);
                }
                idx.push_back(0);
                idx.push_back(-1);
            }
            nodes.outline->coordIndex.setValues(0, static_cast<int>(idx.size()), idx.data());
        }
        nodes.sep->addChild(nodes.outline);

        buttonsSep->addChild(nodes.sep);
        buttonNodes[pickIndex(pickId)] = nodes;
    }
}

void SoNaviCube::rebuildSceneGraph() const
{
    ensureGeometry();

    if (!sceneRoot) {
        return;
    }

    resetSceneGraph();
    buildCamerasAndStyle();
    buildCubeSection();
    buildButtonsSection();

    sceneDirty = false;
}

struct SoNaviCube::RenderParams
{
    float currentOpacity {1.0F};
    float bw {1.0F};
    PickId hilitePick {PickId::None};
    SbColor baseRgb;
    SbColor hiliteRgb;
    SbColor emphRgb;
    float baseTr {0.0F};
    float hiliteTr {0.0F};
    float emphTr {0.0F};
    bool transparentMaterial {false};
    bool transparentTexture {false};
    std::array<SbColor, 3> axisRgb;
    float axisTr {0.0F};
    bool showCS {false};
    bool ortho {true};
    int coordChild {0};
};

SoNaviCube::RenderParams SoNaviCube::makeRenderParams() const
{
    RenderParams params;

    params.currentOpacity = std::clamp(opacity.getValue(), 0.0F, 1.0F);
    params.bw = borderWidth.getValue();
    params.hilitePick = static_cast<PickId>(hiliteId.getValue());

    params.baseRgb = baseColor.getValue();
    params.hiliteRgb = hiliteColor.getValue();
    params.emphRgb = emphaseColor.getValue();

    const float baseA = std::clamp(baseAlpha.getValue(), 0.0F, 1.0F) * params.currentOpacity;
    const float hiliteA = std::clamp(hiliteAlpha.getValue(), 0.0F, 1.0F) * params.currentOpacity;
    const float emphA = std::clamp(emphaseAlpha.getValue(), 0.0F, 1.0F) * params.currentOpacity;
    params.baseTr = toTransparency(baseA);
    params.hiliteTr = toTransparency(hiliteA);
    params.emphTr = toTransparency(emphA);

    params.axisRgb = {axisXColor.getValue(), axisYColor.getValue(), axisZColor.getValue()};
    params.axisTr = toTransparency(params.currentOpacity);
    params.showCS = showCoordinateSystem.getValue();

    params.ortho = cameraIsOrthographic.getValue();
    params.coordChild = params.ortho ? 0 : 1;

    params.transparentMaterial = (opacity.getValue() < 0.999F) || (baseAlpha.getValue() < 0.999F)
        || (emphaseAlpha.getValue() < 0.999F) || (hiliteAlpha.getValue() < 0.999F);
    params.transparentTexture = (labelTextureCount > 0);

    return params;
}

void SoNaviCube::updateSceneGraph(const RenderParams& params) const
{
    updateCameraAndTransform(params);
    updateCube(params);
    updateEdges(params);
    updateAxes(params);
    updateButtons(params);
    updateLabels(params);
}

void SoNaviCube::updateCameraAndTransform(const RenderParams& params) const
{
    if (cameraSwitch) {
        cameraSwitch->whichChild = params.ortho ? 0 : 1;
    }

    if (rootTransform) {
        const SbRotation cam = cameraOrientation.getValue();
        rootTransform->rotation = cam.inverse();
        rootTransform->translation = SbVec3f(0.0F, 0.0F, OVERLAY_CUBE_Z);
    }
}

void SoNaviCube::updateCube(const RenderParams& params) const
{
    if (cubeMaterial) {
        cubeMaterial->diffuseColor.setNum(2);
        cubeMaterial->diffuseColor.set1Value(0, params.baseRgb);
        cubeMaterial->diffuseColor.set1Value(1, params.hiliteRgb);
        cubeMaterial->transparency.setNum(2);
        cubeMaterial->transparency.set1Value(0, params.baseTr);
        cubeMaterial->transparency.set1Value(1, params.hiliteTr);
    }

    if (!cubeFaces) {
        return;
    }

    const int count = static_cast<int>(kCubeFacePickOrder.size());
    if (cubeFaces->materialIndex.getNum() != count) {
        cubeFaces->materialIndex.setNum(count);
        for (int i = 0; i < count; ++i) {
            cubeFaces->materialIndex.set1Value(i, 0);
        }
        style.lastHiliteFaceIndex = -1;
        style.lastHilitePick = PickId::None;
    }

    if (params.hilitePick == style.lastHilitePick) {
        return;
    }

    const int newHiliteIndex = (params.hilitePick == PickId::None)
        ? -1
        : cubeFaceIndex(params.hilitePick);

    if (newHiliteIndex != style.lastHiliteFaceIndex) {
        if (style.lastHiliteFaceIndex >= 0 && style.lastHiliteFaceIndex < count) {
            cubeFaces->materialIndex.set1Value(style.lastHiliteFaceIndex, 0);
        }
        if (newHiliteIndex >= 0 && newHiliteIndex < count) {
            cubeFaces->materialIndex.set1Value(newHiliteIndex, 1);
        }
        style.lastHiliteFaceIndex = newHiliteIndex;
    }
    style.lastHilitePick = params.hilitePick;
}

void SoNaviCube::updateEdges(const RenderParams& params) const
{
    if (edgeMaterial) {
        edgeMaterial->diffuseColor.setValue(params.emphRgb);
        edgeMaterial->transparency = params.emphTr;
    }
    if (edgeDrawStyle) {
        edgeDrawStyle->lineWidth = params.bw;
    }
}

void SoNaviCube::updateAxes(const RenderParams& params) const
{
    bool axisColorsChanged = false;
    if (!style.axisDirty) {
        for (size_t i = 0; i < params.axisRgb.size(); ++i) {
            if (!nearlyEqual(params.axisRgb[i], style.axisRgb[i])) {
                axisColorsChanged = true;
                break;
            }
        }
    }

    const bool axisStyleChanged = style.axisDirty || axisColorsChanged
        || !nearlyEqual(params.axisTr, style.axisTr) || !nearlyEqual(params.bw, style.axisBw)
        || (params.showCS != style.showCS);

    if (!axisStyleChanged) {
        return;
    }

    for (int axis = 0; axis < 3; ++axis) {
        AxisNodes& nodes = axisNodes[axis];
        if (nodes.material) {
            nodes.material->diffuseColor.setValue(params.axisRgb[static_cast<size_t>(axis)]);
            nodes.material->transparency = params.axisTr;
        }
        if (nodes.drawStyle) {
            nodes.drawStyle->lineWidth = params.bw * 2.0F;
            nodes.drawStyle->pointSize = params.bw * 2.0F;
        }
    }

    if (axisSwitch) {
        axisSwitch->whichChild = params.showCS ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }

    style.axisRgb = params.axisRgb;
    style.axisTr = params.axisTr;
    style.axisBw = params.bw;
    style.showCS = params.showCS;
    style.axisDirty = false;
}

void SoNaviCube::updateButtons(const RenderParams& params) const
{
    for (PickId pickId : kButtonPickIds) {
        ButtonNodes& nodes = buttonNodes[pickIndex(pickId)];
        if (nodes.coordsSwitch && nodes.coordsSwitch->whichChild.getValue() != params.coordChild) {
            nodes.coordsSwitch->whichChild = params.coordChild;
        }
    }

    const bool buttonsStyleChanged = style.buttonDirty
        || !nearlyEqual(style.buttonsBaseRgb, params.baseRgb)
        || !nearlyEqual(style.buttonsHiliteRgb, params.hiliteRgb)
        || !nearlyEqual(style.buttonsOutlineRgb, params.emphRgb)
        || !nearlyEqual(style.buttonsBaseTr, params.baseTr)
        || !nearlyEqual(style.buttonsHiliteTr, params.hiliteTr)
        || !nearlyEqual(style.buttonsOutlineTr, params.emphTr)
        || !nearlyEqual(style.buttonsLineWidth, params.bw);

    const auto updateButtonFill = [&](PickId id, bool hilite) {
        ButtonNodes& nodes = buttonNodes[pickIndex(id)];
        if (!nodes.fillMaterial) {
            return;
        }
        nodes.fillMaterial->diffuseColor.setValue(hilite ? params.hiliteRgb : params.baseRgb);
        nodes.fillMaterial->transparency = hilite ? params.hiliteTr : params.baseTr;
    };

    if (buttonsStyleChanged) {
        for (PickId pickId : kButtonPickIds) {
            ButtonNodes& nodes = buttonNodes[pickIndex(pickId)];
            if (nodes.fillMaterial) {
                const bool hilite = (pickId == params.hilitePick);
                nodes.fillMaterial->diffuseColor.setValue(hilite ? params.hiliteRgb : params.baseRgb);
                nodes.fillMaterial->transparency = hilite ? params.hiliteTr : params.baseTr;
            }
            if (nodes.outlineMaterial) {
                nodes.outlineMaterial->diffuseColor.setValue(params.emphRgb);
                nodes.outlineMaterial->transparency = params.emphTr;
            }
            if (nodes.outlineDrawStyle) {
                nodes.outlineDrawStyle->lineWidth = params.bw;
            }
        }
        style.buttonsBaseRgb = params.baseRgb;
        style.buttonsHiliteRgb = params.hiliteRgb;
        style.buttonsOutlineRgb = params.emphRgb;
        style.buttonsBaseTr = params.baseTr;
        style.buttonsHiliteTr = params.hiliteTr;
        style.buttonsOutlineTr = params.emphTr;
        style.buttonsLineWidth = params.bw;
        style.lastButtonsHilitePick = params.hilitePick;
        style.buttonDirty = false;
    }
    else if (params.hilitePick != style.lastButtonsHilitePick) {
        updateButtonFill(style.lastButtonsHilitePick, false);
        updateButtonFill(params.hilitePick, true);
        style.lastButtonsHilitePick = params.hilitePick;
    }
}

void SoNaviCube::updateLabels(const RenderParams& params) const
{
    const bool labelsStyleChanged = style.labelDirty || !nearlyEqual(style.labelsRgb, params.emphRgb)
        || !nearlyEqual(style.labelsTr, params.emphTr);

    if (!labelsStyleChanged) {
        return;
    }

    for (PickId pickId : kLabelPickIds) {
        LabelNodes& nodes = labelNodes[pickIndex(pickId)];
        if (nodes.material) {
            nodes.material->diffuseColor.setValue(params.emphRgb);
            nodes.material->transparency = params.emphTr;
        }
    }
    style.labelsRgb = params.emphRgb;
    style.labelsTr = params.emphTr;
    style.labelDirty = false;
}

void SoNaviCube::updateSceneGraph() const
{
    if (!sceneRoot) {
        return;
    }

    updateSceneGraph(makeRenderParams());
}

void SoNaviCube::beginOverlayPass(
    SoGLRenderAction* action,
    const RenderParams& params,
    int viewportX,
    int viewportY,
    int viewportWidth,
    int viewportHeight
)
{
    if (!action) {
        return;
    }
    SoState* state = action->getState();
    if (!state) {
        return;
    }

    // The viewer scenegraph can have color/transparency overrides active (e.g. selection/highlight).
    // As an overlay, the NaviCube must render with its own colors regardless of such overrides.
    SoOverrideElement::setDiffuseColorOverride(state, this, FALSE);
    SoOverrideElement::setTransparencyOverride(state, this, FALSE);
    SoOverrideElement::setLightModelOverride(state, this, FALSE);
    SoOverrideElement::setMaterialBindingOverride(state, this, FALSE);
    SoOverrideElement::setColorIndexOverride(state, this, FALSE);
    // Some view modes can force wireframe or otherwise override draw style. The legacy GL path
    // always draws filled primitives for buttons/handles, so the Coin backend must ignore any
    // upstream draw-style overrides too (otherwise filled buttons become outlines only).
    SoOverrideElement::setDrawStyleOverride(state, this, FALSE);

    // Avoid inheriting any texture/shader state from the main scene (which can otherwise tint
    // the overlay geometry if texturing is left enabled/bound).
    SoGLTextureEnabledElement::disableAll(state);
    SoLazyElement::setColorMaterial(state, FALSE);
    SoGLShaderProgramElement::enable(state, FALSE);

    // Allow Coin to use vertex arrays/VBOs for retained-mode rendering of the overlay geometry.
    SoShapeStyleElement::setVertexArrayRendering(state, TRUE);

    // The overlay must render immediately (not deferred/sorted by transparency passes), but it
    // still needs to respect its alpha (inactive opacity). For that, keep per-node transparency
    // type as BLEND and advertise transparency when needed.
    SoShapeStyleElement::setTransparentMaterial(state, params.transparentMaterial ? TRUE : FALSE);
    SoShapeStyleElement::setTransparentTexture(state, params.transparentTexture ? TRUE : FALSE);
    SoShapeStyleElement::setTransparencyType(state, SoGLRenderAction::BLEND);
    SoLazyElement::setTransparencyType(state, SoGLRenderAction::BLEND);

    SbViewportRegion vp = SoViewportRegionElement::get(state);
    vp.setViewportPixels(viewportX, viewportY, viewportWidth, viewportHeight);
    SoViewportRegionElement::set(state, vp);

    // Reset depth within the overlay viewport so the NaviCube can self-occlude and render
    // translucency correctly without being affected by whatever the main scene left in the depth
    // buffer.
    clearOverlayDepth(viewportX, viewportY, viewportWidth, viewportHeight);

    // The scissored depth clear is a direct GL operation because Coin currently has no node/API for
    // clearing only an overlay viewport's depth buffer while keeping its GL state cache coherent.
    // Keep Coin's depth element and the actual context state aligned before the retained NaviCube
    // scene renders. Long term, this should move behind a Coin-owned viewport-clear node/API that
    // performs the clear and re-establishes or invalidates the affected GL state itself.
    SoDepthBufferElement::set(state, TRUE, TRUE, SoDepthBufferElement::LEQUAL, SbVec2f(0.0F, 1.0F));
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0, 1.0);

    // Enforce overlay render state after the depth-clear pass.
    SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
    SoShapeStyleElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoShapeStyleElement::setTransparencyType(state, SoGLRenderAction::BLEND);
    SoLazyElement::enableBlending(state, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SoLazyElement::setVertexOrdering(state, SoLazyElement::CCW);
    SoLazyElement::setTwosideLighting(state, FALSE);
}

void SoNaviCube::renderOverlayScene(SoGLRenderAction* action)
{
    if (!action) {
        return;
    }
    SoState* state = action->getState();
    if (!state) {
        return;
    }
    if (!sceneRoot) {
        return;
    }

    SoTextureQualityElement::set(state, this, 1.0F);
    sceneRoot->GLRender(action);
    SoGLTextureEnabledElement::disableAll(state);
}

void SoNaviCube::renderCoin(SoGLRenderAction* action)
{
    if (!action) {
        return;
    }

    const SbVec4f& rect = viewportRect.getValue();
    const int viewportX = static_cast<int>(std::lround(rect[0]));
    const int viewportY = static_cast<int>(std::lround(rect[1]));
    const int viewportWidth = static_cast<int>(std::lround(rect[2]));
    const int viewportHeight = static_cast<int>(std::lround(rect[3]));

    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    SoState* state = action->getState();
    if (!state) {
        return;
    }

    ensureGeometry();
    ensureSceneGraph();
    const RenderParams params = makeRenderParams();
    updateSceneGraph(params);

    state->push();
    beginOverlayPass(action, params, viewportX, viewportY, viewportWidth, viewportHeight);
    renderOverlayScene(action);
    state->pop();
}

void SoNaviCube::GLRender(SoGLRenderAction* action)
{
    if (!shouldGLRender(action)) {
        return;
    }
    renderCoin(action);
}

void SoNaviCube::computeBBox(SoAction*, SbBox3f& box, SbVec3f& center)
{
    const float halfSize = size.getValue() * 0.5F;
    box.setBounds(SbVec3f(-halfSize, -halfSize, -halfSize), SbVec3f(halfSize, halfSize, halfSize));
    center.setValue(0.0F, 0.0F, 0.0F);
}

SoNaviCube::PickId SoNaviCube::pickAt(const SbVec2s& point) const
{
    ensureGeometry();
    ensureSceneGraph();
    updateSceneGraph();

    const SbVec4f& rect = viewportRect.getValue();
    const float viewportX = rect[0];
    const float viewportY = rect[1];
    const float viewportWidth = rect[2];
    const float viewportHeight = rect[3];
    if (viewportWidth <= 0.0F || viewportHeight <= 0.0F) {
        return PickId::None;
    }

    const SbVec2s localPoint(
        static_cast<short>(static_cast<float>(point[0]) - viewportX),
        static_cast<short>(static_cast<float>(point[1]) - viewportY)
    );
    if (localPoint[0] < 0 || localPoint[1] < 0 || localPoint[0] >= static_cast<short>(viewportWidth)
        || localPoint[1] >= static_cast<short>(viewportHeight)) {
        return PickId::None;
    }

    const SbVec2f overlayPoint(
        (static_cast<float>(localPoint[0]) + 0.5F) / viewportWidth,
        1.0F - ((static_cast<float>(localPoint[1]) + 0.5F) / viewportHeight)
    );
    for (PickId pickId : kButtonPickIds) {
        const ButtonHitRect& rect = buttonHitRects[pickIndex(pickId)];
        if (rect.active && overlayPoint[0] >= rect.left && overlayPoint[0] <= rect.right
            && overlayPoint[1] >= rect.top && overlayPoint[1] <= rect.bottom) {
            return pickId;
        }
    }

    const SbViewportRegion vp(static_cast<int>(viewportWidth), static_cast<int>(viewportHeight));
    SoRayPickAction pick(vp);
    pick.setPoint(localPoint);
    {
        float radius = 1.0F;
        if (const char* env = std::getenv("FREECAD_NAVICUBE_PICK_RADIUS")) {
            char* end = nullptr;
            const float value = std::strtof(env, &end);
            if (end != env && std::isfinite(value) && value >= 0.0F) {
                radius = value;
            }
        }
        pick.setRadius(radius);
    }
    pick.setPickAll(TRUE);
    pick.apply(sceneRoot);

    const SoPickedPointList& hits = pick.getPickedPointList();
    if (hits.getLength() == 0) {
        return PickId::None;
    }

    const auto resolveHit = [&](const SoPickedPoint* hit) -> PickId {
        if (!hit) {
            return PickId::None;
        }

        const SoPath* path = hit->getPath();
        const SoNode* tail = path ? path->getTail() : nullptr;
        if (!tail) {
            return PickId::None;
        }

        for (PickId pickId : kButtonPickIds) {
            const ButtonNodes& nodes = buttonNodes[pickIndex(pickId)];
            if (tail == nodes.fill || tail == nodes.outline) {
                return pickId;
            }
        }

        for (PickId pickId : kLabelPickIds) {
            const LabelNodes& nodes = labelNodes[pickIndex(pickId)];
            if (tail == nodes.face) {
                return pickId;
            }
        }

        if (tail == cubeFaces) {
            const SoDetail* detail = hit->getDetail();
            if (detail && detail->isOfType(SoFaceDetail::getClassTypeId())) {
                const auto* face = static_cast<const SoFaceDetail*>(detail);
                const int faceIndex = face->getFaceIndex();
                if (faceIndex >= 0 && faceIndex < static_cast<int>(kCubeFacePickOrder.size())) {
                    return kCubeFacePickOrder[static_cast<size_t>(faceIndex)];
                }
            }
        }

        return PickId::None;
    };

    // Prefer hits with face details (filled geometry) to avoid selecting edges/lines.
    for (int i = 0; i < hits.getLength(); ++i) {
        const SoPickedPoint* hit = hits[i];
        const SoDetail* detail = hit ? hit->getDetail() : nullptr;
        if (detail && detail->isOfType(SoFaceDetail::getClassTypeId())) {
            const PickId id = resolveHit(hit);
            if (id != PickId::None) {
                return id;
            }
        }
    }

    for (int i = 0; i < hits.getLength(); ++i) {
        const PickId id = resolveHit(hits[i]);
        if (id != PickId::None) {
            return id;
        }
    }

    return PickId::None;
}

void SoNaviCube::generatePrimitives(SoAction* action)
{
    (void)action;
}

void SoNaviCube::ensureGeometry() const
{
    if (!geometryDirty) {
        return;
    }
    rebuildGeometry();
}

void SoNaviCube::rebuildGeometry() const
{
    std::array<LabelSlot, kPickIdCount> oldLabelSlots;
    oldLabelSlots.swap(labelSlots);

    for (auto& verts : faces) {
        verts.clear();
    }
    for (auto& verts : buttonOverlayVerts) {
        verts.clear();
    }
    for (auto& tris : buttonTriangleIndices) {
        tris.clear();
    }
    for (auto& slot : labelSlots) {
        slot.quad.clear();
        slot.texture = nullptr;
    }

    cubeCoordsData.clear();
    cubeCoordIndexData.clear();
    edgeCoordsData.clear();
    edgeCoordIndexData.clear();

    const SbVec3f x(1.0F, 0.0F, 0.0F);
    const SbVec3f y(0.0F, 1.0F, 0.0F);
    const SbVec3f z(0.0F, 0.0F, 1.0F);

    // Main faces
    addCubeFace(x, z, CubeFaceKind::Main, PickId::Top);
    addCubeFace(x, -y, CubeFaceKind::Main, PickId::Front);
    addCubeFace(-y, -x, CubeFaceKind::Main, PickId::Left);
    addCubeFace(-x, y, CubeFaceKind::Main, PickId::Rear);
    addCubeFace(y, x, CubeFaceKind::Main, PickId::Right);
    addCubeFace(x, -z, CubeFaceKind::Main, PickId::Bottom);

    // Corner faces
    addCubeFace(-x - y, x - y + z, CubeFaceKind::Corner, PickId::FrontTopRight);
    addCubeFace(-x + y, -x - y + z, CubeFaceKind::Corner, PickId::FrontTopLeft);
    addCubeFace(x + y, x - y - z, CubeFaceKind::Corner, PickId::FrontBottomRight);
    addCubeFace(x - y, -x - y - z, CubeFaceKind::Corner, PickId::FrontBottomLeft);
    addCubeFace(x - y, x + y + z, CubeFaceKind::Corner, PickId::RearTopRight);
    addCubeFace(x + y, -x + y + z, CubeFaceKind::Corner, PickId::RearTopLeft);
    addCubeFace(-x + y, x + y - z, CubeFaceKind::Corner, PickId::RearBottomRight);
    addCubeFace(-x - y, -x + y - z, CubeFaceKind::Corner, PickId::RearBottomLeft);

    // Edge faces
    addCubeFace(x, z - y, CubeFaceKind::Edge, PickId::FrontTop);
    addCubeFace(x, -z - y, CubeFaceKind::Edge, PickId::FrontBottom);
    addCubeFace(x, y - z, CubeFaceKind::Edge, PickId::RearBottom);
    addCubeFace(x, y + z, CubeFaceKind::Edge, PickId::RearTop);
    addCubeFace(z, x + y, CubeFaceKind::Edge, PickId::RearRight);
    addCubeFace(z, x - y, CubeFaceKind::Edge, PickId::FrontRight);
    addCubeFace(z, -x - y, CubeFaceKind::Edge, PickId::FrontLeft);
    addCubeFace(z, y - x, CubeFaceKind::Edge, PickId::RearLeft);
    addCubeFace(y, z - x, CubeFaceKind::Edge, PickId::TopLeft);
    addCubeFace(y, x + z, CubeFaceKind::Edge, PickId::TopRight);
    addCubeFace(y, x - z, CubeFaceKind::Edge, PickId::BottomRight);
    addCubeFace(y, -z - x, CubeFaceKind::Edge, PickId::BottomLeft);

    // Precompute cube and edge geometry arrays used by the scene graph nodes.
    {
        size_t totalVerts = 0;
        for (PickId pickId : kCubeFacePickOrder) {
            const auto& verts = faces[pickIndex(pickId)];
            if (verts.size() >= 3) {
                totalVerts += verts.size();
            }
        }
        cubeCoordsData.reserve(totalVerts);
        cubeCoordIndexData.reserve(totalVerts + kCubeFacePickOrder.size());

        std::int32_t base = 0;
        for (PickId pickId : kCubeFacePickOrder) {
            const auto& verts = faces[pickIndex(pickId)];
            if (verts.size() < 3) {
                continue;
            }
            for (const SbVec3f& p : verts) {
                cubeCoordsData.push_back(p);
                cubeCoordIndexData.push_back(base++);
            }
            cubeCoordIndexData.push_back(-1);
        }

        size_t edgeVerts = 0;
        size_t edgeSegs = 0;
        for (PickId pickId : kCubeFacePickOrder) {
            const auto& verts = faces[pickIndex(pickId)];
            edgeVerts += verts.size();
            edgeSegs += verts.size() / 2;
        }
        edgeCoordsData.reserve(edgeVerts);
        edgeCoordIndexData.reserve(edgeSegs * 3);

        std::int32_t edgeBase = 0;
        for (PickId pickId : kCubeFacePickOrder) {
            const auto& verts = faces[pickIndex(pickId)];
            const size_t n = verts.size();
            if (n < 2) {
                continue;
            }

            for (const SbVec3f& p : verts) {
                edgeCoordsData.push_back(p);
            }

            for (size_t i = 0; i + 1 < n; i += 2) {
                edgeCoordIndexData.push_back(edgeBase + static_cast<std::int32_t>(i));
                edgeCoordIndexData.push_back(edgeBase + static_cast<std::int32_t>(i + 1));
                edgeCoordIndexData.push_back(-1);
            }
            edgeBase += static_cast<std::int32_t>(n);
        }
    }

    auto setLabelQuad = [&](PickId pickId, const SbVec3f& xVec, const SbVec3f& zVec) {
        auto& slot = labelSlots[pickIndex(pickId)];
        slot.quad.clear();

        SbVec3f negZ = zVec;
        negZ.negate();

        SbVec3f yVec = xVec.cross(negZ);
        SbVec3f x2 = xVec * (1.0F - this->chamfer * 2.0F);
        SbVec3f y2 = yVec * (1.0F - this->chamfer * 2.0F);

        slot.quad.reserve(4);
        slot.quad.emplace_back(zVec - x2 - y2);
        slot.quad.emplace_back(zVec + x2 - y2);
        slot.quad.emplace_back(zVec + x2 + y2);
        slot.quad.emplace_back(zVec - x2 + y2);

        auto& oldSlot = oldLabelSlots[pickIndex(pickId)];
        if (oldSlot.texture) {
            slot.texture = oldSlot.texture;
            oldSlot.texture = nullptr;
        }
    };

    setLabelQuad(PickId::Top, x, z);
    setLabelQuad(PickId::Front, x, -y);
    setLabelQuad(PickId::Left, -y, -x);
    setLabelQuad(PickId::Rear, -x, y);
    setLabelQuad(PickId::Right, y, x);
    setLabelQuad(PickId::Bottom, x, -z);

    rebuildButtonFaces();

    for (auto& slot : oldLabelSlots) {
        if (slot.texture) {
            slot.texture->unref();
            slot.texture = nullptr;
        }
    }

    geometryDirty = false;
    sceneDirty = true;
}

void SoNaviCube::addCubeFace(const SbVec3f& x, const SbVec3f& z, CubeFaceKind kind, PickId pickId) const
{
    auto& verts = faces[pickIndex(pickId)];
    verts.clear();

    SbVec3f y = x.cross(-z);

    if (kind == CubeFaceKind::Corner) {
        SbVec3f xC = x * this->chamfer;
        SbVec3f yC = y * this->chamfer;
        SbVec3f zC = z * (1.0F - 2.0F * this->chamfer);

        verts.reserve(6);
        verts.emplace_back(zC - xC * 2.0F);
        verts.emplace_back((zC - xC) - yC);
        verts.emplace_back((zC + xC) - yC);
        verts.emplace_back(zC + xC * 2.0F);
        verts.emplace_back((zC + xC) + yC);
        verts.emplace_back((zC - xC) + yC);
    }
    else if (kind == CubeFaceKind::Edge) {
        SbVec3f x4 = x * (1.0F - this->chamfer * 4.0F);
        SbVec3f yE = y * this->chamfer;
        SbVec3f zE = z * (1.0F - this->chamfer);

        verts.reserve(4);
        verts.emplace_back((zE - x4) - yE);
        verts.emplace_back((zE + x4) - yE);
        verts.emplace_back((zE + x4) + yE);
        verts.emplace_back((zE - x4) + yE);
    }
    else {
        SbVec3f x2 = x * (1.0F - this->chamfer * 2.0F);
        SbVec3f y2 = y * (1.0F - this->chamfer * 2.0F);
        SbVec3f x4 = x * (1.0F - this->chamfer * 4.0F);
        SbVec3f y4 = y * (1.0F - this->chamfer * 4.0F);

        verts.reserve(8);
        verts.emplace_back((z - x2) - y4);
        verts.emplace_back((z - x4) - y2);
        verts.emplace_back((z + x4) - y2);
        verts.emplace_back((z + x2) - y4);
        verts.emplace_back((z + x2) + y4);
        verts.emplace_back((z + x4) + y2);
        verts.emplace_back((z - x4) + y2);
        verts.emplace_back((z - x2) + y4);
    }
}

void SoNaviCube::rebuildButtonFaces() const
{
    for (auto& verts : buttonOverlayVerts) {
        verts.clear();
    }
    for (auto& tris : buttonTriangleIndices) {
        tris.clear();
    }
    for (auto& outline : buttonOutlineIndices) {
        outline.clear();
    }
    for (auto& rect : buttonHitRects) {
        rect = {};
    }
    addButtonFace(PickId::ArrowNorth);
    addButtonFace(PickId::ArrowSouth);
    addButtonFace(PickId::ArrowEast);
    addButtonFace(PickId::ArrowWest);
    addButtonFace(PickId::ArrowLeft);
    addButtonFace(PickId::ArrowRight);
    addButtonFace(PickId::Backside);
    addButtonFace(PickId::Home);
    addButtonFace(PickId::ViewMenu);
}

void SoNaviCube::addButtonFace(PickId pickId) const
{
    auto& verts = buttonOverlayVerts[pickIndex(pickId)];
    auto& outline = buttonOutlineIndices[pickIndex(pickId)];
    auto& tris = buttonTriangleIndices[pickIndex(pickId)];
    auto& hitRect = buttonHitRects[pickIndex(pickId)];
    verts.clear();
    outline.clear();
    tris.clear();
    hitRect = {};
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

    const auto appendLoop = [&](const std::vector<float>& pts) {
        const auto base = static_cast<std::int32_t>(verts.size());
        const int count = static_cast<int>(pts.size()) / 2;
        verts.reserve(verts.size() + static_cast<size_t>(count));
        for (int i = 0; i < count; ++i) {
            verts.push_back(transform(pts[i * 2], pts[i * 2 + 1]));
            outline.push_back(base + i);
        }
        outline.push_back(base);
        outline.push_back(-1);
        return base;
    };

    const auto appendTriangles = [&](std::int32_t base, const std::initializer_list<int>& idx) {
        tris.reserve(tris.size() + idx.size());
        for (int i : idx) {
            tris.push_back(base + i);
        }
    };

    const auto addPoint = [&](float x, float y) {
        const auto index = static_cast<std::int32_t>(verts.size());
        verts.push_back(transform(x, y));
        return index;
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
            const auto bar = appendLoop({-13.0F, -20.0F, 13.0F, -20.0F, 13.0F, -16.0F, -13.0F, -16.0F});
            appendTriangles(bar, {0, 1, 2, 0, 2, 3});
            const auto triangle = appendLoop({-13.0F, -12.0F, 13.0F, -12.0F, 0.0F, 0.0F});
            appendTriangles(triangle, {0, 1, 2});
            break;
        }
        case PickId::Home: {
            offx = 0.09F;
            offy = 0.09F;
            const auto base = appendLoop({0.0F,   -18.0F, 18.0F,  -6.0F, 12.0F,  -6.0F, 12.0F, 8.0F,
                                          4.0F,   8.0F,   4.0F,   0.0F,  -4.0F,  0.0F,  -4.0F, 8.0F,
                                          -12.0F, 8.0F,   -12.0F, -6.0F, -18.0F, -6.0F});
            const int roofTop = 0;
            const int roofRight = 1;
            const int rightWallTop = 2;
            const int rightWallBottom = 3;
            const int doorRightBottom = 4;
            const int doorRightTop = 5;
            const int doorLeftTop = 6;
            const int doorLeftBottom = 7;
            const int leftWallBottom = 8;
            const int leftWallTop = 9;
            const int roofLeft = 10;
            const int doorRightLintel = addPoint(4.0F, -6.0F) - base;
            const int doorLeftLintel = addPoint(-4.0F, -6.0F) - base;

            // Keep the door as a true cutout: fill the roof, side walls, and lintel separately
            // instead of drawing triangles through the opening.
            appendTriangles(base, {roofLeft,        roofRight,       roofTop,
                                   doorRightLintel, rightWallBottom, rightWallTop,
                                   doorRightLintel, doorRightBottom, rightWallBottom,
                                   leftWallTop,     doorLeftBottom,  doorLeftLintel,
                                   leftWallTop,     leftWallBottom,  doorLeftBottom,
                                   doorLeftLintel,  doorRightTop,    doorRightLintel,
                                   doorLeftLintel,  doorLeftTop,     doorRightTop});
            break;
        }
        case PickId::Backside: {
            offx = 0.80F;
            offy = 0.0F;
            // The icon has two disconnected arrow loops; keep the center gap clickable.
            hitRect
                = {true, BACKSIDE_HIT_LEFT, BACKSIDE_HIT_TOP, BACKSIDE_HIT_RIGHT, BACKSIDE_HIT_BOTTOM};
            const auto loop1 = appendLoop(
                {24.0F, 21.5F, 17.0F, 29.1F, 16.7F, 25.6F, 12.0F, 25.3F, 8.2F,  24.0F, 4.0F,
                 22.0F, 1.2F,  19.0F, 0.0F,  15.0F, 0.0F,  10.0F, 1.5F,  8.1F,  4.4F,  6.1F,
                 8.0F,  5.5F,  14.0F, 4.0F,  14.6F, 9.2F,  10.1F, 10.2F, 6.0F,  12.0F, 3.5F,
                 14.0F, 3.4F,  14.5F, 6.0F,  15.8F, 10.0F, 17.0F, 16.3F, 18.0F, 16.3F, 13.6F}
            );
            appendTriangles(loop1, {0,  1,  2, 0,  2,  20, 0,  20, 21, 2,  20, 3,  20, 19, 3,
                                    3,  19, 4, 19, 18, 4,  4,  18, 5,  18, 17, 5,  5,  17, 6,
                                    17, 16, 6, 6,  16, 7,  16, 15, 7,  7,  15, 8,  15, 14, 8,
                                    8,  14, 9, 14, 13, 9,  9,  13, 10, 10, 13, 11, 11, 13, 12});

            const auto loop2 = appendLoop(
                {18.0F, 6.0F,  22.6F, 0.0F,  22.5F, 3.0F,  27.0F, 3.3F,  31.4F, 4.3F,  35.0F,
                 5.6F,  37.5F, 7.1F,  40.0F, 9.7F,  40.0F, 12.8F, 38.5F, 16.5F, 36.2F, 20.0F,
                 33.0F, 21.9F, 28.3F, 22.9F, 28.7F, 16.0F, 32.8F, 15.0F, 36.4F, 12.9F, 33.8F,
                 10.8F, 30.0F, 9.3F,  26.1F, 8.5F,  22.5F, 8.1F,  22.4F, 10.8F}
            );
            appendTriangles(loop2, {0,  1,  2, 0,  2,  19, 0,  19, 20, 2,  19, 3, 19, 18, 3,
                                    3,  18, 4, 18, 17, 4,  4,  17, 5,  17, 16, 5, 5,  16, 6,
                                    16, 15, 6, 6,  15, 7,  15, 14, 7,  7,  14, 8, 14, 13, 8,
                                    8,  13, 9, 9,  13, 10, 10, 13, 11, 11, 13, 12});
            break;
        }
    }

    if (!pointData.empty()) {
        const int count = static_cast<int>(pointData.size()) / 2;
        verts.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i) {
            verts.push_back(transform(pointData[i * 2], pointData[i * 2 + 1]));
        }
    }

    if (outline.empty() && !verts.empty()) {
        outline.reserve(verts.size() + 2);
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(verts.size()); ++i) {
            outline.push_back(i);
        }
        outline.push_back(0);
        outline.push_back(-1);
    }

    if (tris.empty()) {
        if (triangulatePolygon2D(verts, tris) && (tris.size() % 3 == 0) && tris.size() >= 3) {
            // ok
        }
        else if (verts.size() == 3) {
            tris = {0, 1, 2};
        }
    }
}
