// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>
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

#pragma once

#include <FCGlobal.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbRotation.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFVec4f.h>
#include <Inventor/nodes/SoShape.h>

#include <cstdint>
#include <array>
#include <vector>

class SoTexture2;
class SoSeparator;
class SoSwitch;
class SoMaterial;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoPointSet;
class SoDrawStyle;
class SoFaceSet;
class SoPolygonOffset;
class SoDepthBuffer;
class SoShapeHints;
class SoCamera;
class SoOrthographicCamera;
class SoPerspectiveCamera;
class SoTransform;
class SoVertexProperty;
namespace Gui
{

/**
 * Placeholder Coin node for the navigation cube overlay.
 *
 * Rendering responsibilities will be migrated from NaviCubeImplementation
 * into this node in subsequent steps.
 */
class GuiExport SoNaviCube: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoNaviCube);

public:
    static void initClass();
    SoNaviCube();

    //! Cube edge length in viewer units (wired up by the controller later).
    SoSFFloat size;

    enum class PickId
    {
        None,
        Front,
        Top,
        Right,
        Rear,
        Bottom,
        Left,
        FrontTop,
        FrontBottom,
        FrontRight,
        FrontLeft,
        RearTop,
        RearBottom,
        RearRight,
        RearLeft,
        TopRight,
        TopLeft,
        BottomRight,
        BottomLeft,
        FrontTopRight,
        FrontTopLeft,
        FrontBottomRight,
        FrontBottomLeft,
        RearTopRight,
        RearTopLeft,
        RearBottomRight,
        RearBottomLeft,
        ArrowNorth,
        ArrowSouth,
        ArrowEast,
        ArrowWest,
        ArrowRight,
        ArrowLeft,
        Backside,
        Home,
        ViewMenu
    };

    void setChamfer(float chamfer);
    void setLabelImage(PickId id, const SbVec2s& size, int numComponents, const unsigned char* pixels);
    void clearLabelTextures();
    [[nodiscard]] PickId pickAt(const SbVec2s& point) const;

protected:
    ~SoNaviCube() override;

    void GLRender(SoGLRenderAction* action) override;
    void generatePrimitives(SoAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;

private:
    enum class CubeFaceKind
    {
        Main,
        Edge,
        Corner
    };

    struct LabelSlot
    {
        std::vector<SbVec3f> quad;
        ::SoTexture2* texture {nullptr};
    };

    struct ButtonHitRect
    {
        bool active {false};
        float left {0.0F};
        float top {0.0F};
        float right {0.0F};
        float bottom {0.0F};
    };

    void renderCoin(SoGLRenderAction* action);
    void ensureSceneGraph() const;
    void rebuildSceneGraph() const;
    void resetSceneGraph() const;
    void buildCamerasAndStyle() const;
    void buildCubeSection() const;
    void buildAxisSection(SoSeparator* cubeGroup) const;
    void buildButtonsSection() const;
    struct RenderParams;
    [[nodiscard]] RenderParams makeRenderParams() const;
    void updateSceneGraph(const RenderParams& params) const;
    void updateCameraAndTransform(const RenderParams& params) const;
    void updateCube(const RenderParams& params) const;
    void updateEdges(const RenderParams& params) const;
    void updateAxes(const RenderParams& params) const;
    void updateButtons(const RenderParams& params) const;
    void updateLabels(const RenderParams& params) const;
    void updateSceneGraph() const;
    void beginOverlayPass(
        SoGLRenderAction* action,
        const RenderParams& params,
        int viewportX,
        int viewportY,
        int viewportWidth,
        int viewportHeight
    );
    void renderOverlayScene(SoGLRenderAction* action);
    void ensureGeometry() const;
    void rebuildGeometry() const;
    void addCubeFace(const SbVec3f& x, const SbVec3f& z, CubeFaceKind kind, PickId pickId) const;
    void rebuildButtonFaces() const;
    void addButtonFace(PickId pickId) const;

private:
    static constexpr size_t kPickIdCount = static_cast<size_t>(PickId::ViewMenu) + 1;

    static constexpr size_t pickIndex(PickId id)
    {
        return static_cast<size_t>(id);
    }

    struct AxisNodes
    {
        SoSeparator* sep {nullptr};
        SoMaterial* material {nullptr};
        SoDrawStyle* drawStyle {nullptr};
        SoVertexProperty* vertexProperty {nullptr};
        SoIndexedLineSet* line {nullptr};
        SoPointSet* points {nullptr};
    };

    struct LabelNodes
    {
        SoSeparator* sep {nullptr};
        SoMaterial* material {nullptr};
        SoTexture2* texture {nullptr};
        SoVertexProperty* vertexProperty {nullptr};
        SoFaceSet* face {nullptr};
    };

    struct ButtonNodes
    {
        SoSeparator* sep {nullptr};
        SoMaterial* fillMaterial {nullptr};
        SoMaterial* outlineMaterial {nullptr};
        SoDrawStyle* outlineDrawStyle {nullptr};
        SoSwitch* coordsSwitch {nullptr};
        SoVertexProperty* vertexOrtho {nullptr};
        SoVertexProperty* vertexPersp {nullptr};
        SoIndexedFaceSet* fill {nullptr};
        SoIndexedLineSet* outline {nullptr};
    };

    mutable SoSeparator* sceneRoot {nullptr};
    mutable SoSwitch* cameraSwitch {nullptr};
    mutable SoOrthographicCamera* orthoCamera {nullptr};
    mutable SoPerspectiveCamera* perspCamera {nullptr};
    mutable SoTransform* rootTransform {nullptr};
    mutable SoSwitch* axisSwitch {nullptr};
    mutable AxisNodes axisNodes[3];
    mutable SoSeparator* cubeSep {nullptr};
    mutable SoMaterial* cubeMaterial {nullptr};
    mutable SoVertexProperty* cubeVertexProperty {nullptr};
    mutable SoIndexedFaceSet* cubeFaces {nullptr};
    mutable SoSeparator* edgeSep {nullptr};
    mutable SoMaterial* edgeMaterial {nullptr};
    mutable SoDrawStyle* edgeDrawStyle {nullptr};
    mutable SoVertexProperty* edgeVertexProperty {nullptr};
    mutable SoIndexedLineSet* edges {nullptr};
    mutable SoSeparator* labelsSep {nullptr};
    mutable SoSeparator* buttonsSep {nullptr};
    mutable std::array<LabelNodes, kPickIdCount> labelNodes;
    mutable std::array<ButtonNodes, kPickIdCount> buttonNodes;

    struct StyleState
    {
        int lastHiliteFaceIndex {-1};
        PickId lastHilitePick {PickId::None};
        PickId lastButtonsHilitePick {PickId::None};
        bool buttonDirty {true};
        bool labelDirty {true};
        bool axisDirty {true};
        SbColor buttonsBaseRgb {0.0F, 0.0F, 0.0F};
        SbColor buttonsHiliteRgb {0.0F, 0.0F, 0.0F};
        SbColor buttonsOutlineRgb {0.0F, 0.0F, 0.0F};
        float buttonsBaseTr {0.0F};
        float buttonsHiliteTr {0.0F};
        float buttonsOutlineTr {0.0F};
        float buttonsLineWidth {0.0F};
        SbColor labelsRgb {0.0F, 0.0F, 0.0F};
        float labelsTr {0.0F};
        std::array<SbColor, 3> axisRgb {{
            SbColor(0.0F, 0.0F, 0.0F),
            SbColor(0.0F, 0.0F, 0.0F),
            SbColor(0.0F, 0.0F, 0.0F),
        }};
        float axisTr {0.0F};
        float axisBw {0.0F};
        bool showCS {false};
    };

    mutable StyleState style;
    mutable bool sceneDirty {true};
    float chamfer {0.12F};
    mutable bool geometryDirty {true};
    mutable std::array<std::vector<SbVec3f>, kPickIdCount> faces;
    mutable std::array<std::vector<SbVec3f>, kPickIdCount> buttonOverlayVerts;
    mutable std::array<std::vector<int>, kPickIdCount> buttonTriangleIndices;
    mutable std::array<std::vector<std::int32_t>, kPickIdCount> buttonOutlineIndices;
    mutable std::array<ButtonHitRect, kPickIdCount> buttonHitRects;
    mutable std::vector<SbVec3f> cubeCoordsData;
    mutable std::vector<std::int32_t> cubeCoordIndexData;
    mutable std::vector<SbVec3f> edgeCoordsData;
    mutable std::vector<std::int32_t> edgeCoordIndexData;
    mutable std::array<LabelSlot, kPickIdCount> labelSlots;
    int labelTextureCount {0};

public:
    SoSFFloat opacity;
    SoSFFloat borderWidth;
    SoSFBool showCoordinateSystem;
    SoSFColor baseColor;
    SoSFFloat baseAlpha;
    SoSFColor emphaseColor;
    SoSFFloat emphaseAlpha;
    SoSFColor hiliteColor;
    SoSFFloat hiliteAlpha;
    SoSFColor axisXColor;
    SoSFColor axisYColor;
    SoSFColor axisZColor;
    SoSFInt32 hiliteId;
    SoSFRotation cameraOrientation;
    SoSFBool cameraIsOrthographic;
    SoSFVec4f viewportRect;
};

}  // namespace Gui
