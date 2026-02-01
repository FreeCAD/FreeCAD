/***************************************************************************
 *   Copyright (c) 2017 Kustaa Nyholm  <kustaa.nyholm@sparetimelabs.com>   *
 *   Copyright (c) 2025 Joao Matos                                         *
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

#ifndef GUI_INVENTOR_SONAVICUBE_H
#define GUI_INVENTOR_SONAVICUBE_H

#include <FCGlobal.h>
#include <Inventor/SbColor.h>
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

#include <map>
#include <vector>

namespace Gui
{

/**
 * Coin node for the navigation cube overlay.
 */
class GuiExport SoNaviCube: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoNaviCube);

public:
    static void initClass();
    SoNaviCube();

    //! Cube edge length in viewer units.
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
        DotBackside,
        ViewMenu
    };

    enum class FaceType
    {
        None,
        Main,
        Edge,
        Corner,
        Button
    };

    struct Face
    {
        FaceType type {FaceType::None};
        std::vector<SbVec3f> vertexArray;
        SbRotation rotation;
    };

    using FaceMap = std::map<PickId, Face>;

    struct LabelSlot
    {
        std::vector<SbVec3f> quad;
        unsigned int textureId {0};
    };

    using LabelMap = std::map<PickId, LabelSlot>;

    struct ColorWithAlpha
    {
        SbColor rgb {1.0F, 1.0F, 1.0F};
        float alpha {1.0F};
    };

    void setChamfer(float chamfer);
    [[nodiscard]] float chamfer() const
    {
        return m_Chamfer;
    }
    [[nodiscard]] const FaceMap& faces() const;
    [[nodiscard]] const Face* faceForId(PickId id) const;
    [[nodiscard]] const FaceMap& buttonFaces() const;
    [[nodiscard]] const Face* buttonFaceForId(PickId id) const;
    [[nodiscard]] const LabelMap& labelSlots() const;
    [[nodiscard]] const LabelSlot* labelSlotForId(PickId id) const;
    void setLabelTexture(PickId id, unsigned int textureId);
    void clearLabelTextures();
    void renderGL(bool pickMode) const;

protected:
    ~SoNaviCube() override = default;

    void GLRender(SoGLRenderAction* action) override;
    void generatePrimitives(SoAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;

private:
    void ensureGeometry() const;
    void rebuildGeometry() const;
    void addCubeFace(const SbVec3f& x, const SbVec3f& z, FaceType type, PickId pickId, float rotZ) const;
    void rebuildButtonFaces() const;
    void addButtonFace(PickId pickId, const SbVec3f& direction = SbVec3f(0, 0, 0)) const;

private:
    float m_Chamfer {0.12F};
    mutable bool m_GeometryDirty {true};
    mutable FaceMap m_Faces;
    mutable FaceMap m_ButtonFaces;
    mutable LabelMap m_LabelSlots;

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

#endif  // GUI_INVENTOR_SONAVICUBE_H
