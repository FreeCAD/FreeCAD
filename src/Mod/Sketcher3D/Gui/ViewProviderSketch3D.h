// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <array>
#include <memory>
#include <string>

#include <QCoreApplication>

#include <Base/Vector3D.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

class QMenu;
class SbVec2s;
class SoCoordinate3;
class SoMaterial;
class SoSeparator;
class SoSwitch;
class SoTranslation;

namespace Sketcher3D
{
class Sketch3DObject;
}

namespace Sketcher3DGui
{

class DrawSketchHandler3D;
class SnapManager3D;
class TaskSketcher3DTool;

using Color3f = std::array<float, 3>;

constexpr float kAxisHandleLength = 10.0F;
constexpr float kPlaneOverlaySize = kAxisHandleLength;
constexpr float kPlaneTransparency = 0.88F;
constexpr float kAxisHandleArrowLength = 1.8F;
constexpr float kAxisHandleLineWidth = 1.6F;
constexpr float kAxisHandleTransparency = 0.35F;
constexpr float kArrowHeadHalfWidthFactor = 0.55F;
constexpr float kActiveAxisHandleEmissiveScale = 0.18F;
constexpr float kInactiveAxisHandleEmissiveScale = 0.06F;
constexpr int kPlaneQuadVertexCount = 4;
constexpr float kSnapMarkerRadius = 0.6F;

constexpr Color3f kActiveAxisHandleColor {1.0F, 0.05F, 0.05F};
constexpr Color3f kInactiveAxisHandleColor {0.36F, 0.40F, 0.48F};
constexpr Color3f kPlaneOverlayColor {0.25F, 0.45F, 0.90F};
constexpr Color3f kSnapMarkerDiffuseColor {1.0F, 0.55F, 0.0F};
constexpr Color3f kSnapMarkerEmissiveColor {0.6F, 0.3F, 0.0F};

class Sketcher3DGuiExport ViewProviderSketch3D: public PartGui::ViewProviderPart
{
    Q_DECLARE_TR_FUNCTIONS(Sketcher3DGui::ViewProviderSketch3D)
    PROPERTY_HEADER_WITH_OVERRIDE(Sketcher3DGui::ViewProviderSketch3D);

public:
    /// Cardinal workplane. UI aid only, geometry
    /// is always stored and solved in full 3D.
    enum class ActivePlane : int
    {
        XY = 0,
        YZ = 1,
        ZX = 2,
    };

    ViewProviderSketch3D();
    ~ViewProviderSketch3D() override;

    ActivePlane getActivePlane() const
    {
        return activePlane;
    }
    const Base::Vector3d& getPlaneBase() const
    {
        return planeBase;
    }
    /// move the workplane anchor point.
    void setPlaneBase(const Base::Vector3d& base);
    /// Switch to the next plane (XY ->YZ -> ZX -> XY).
    void cyclePlane();

    void setTaskPanel(TaskSketcher3DTool* panel)
    {
        taskPanel = panel;
    }
    TaskSketcher3DTool* getTaskPanel() const
    {
        return taskPanel;
    }

    void updateData(const App::Property* prop) override;

    Sketcher3D::Sketch3DObject* getSketch3DObject() const;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    const char* getTransactionText() const override
    {
        return nullptr;
    }

    /// Install a creation tool. Purges any previously active handler.
    void activateHandler(std::unique_ptr<DrawSketchHandler3D> handler);
    /// Tear down the active handler.
    void purgeHandler();
    DrawSketchHandler3D* getHandler() const
    {
        return handler.get();
    }

    // Event forwarding from the viewer while in edit mode.
    bool mouseButtonPressed(
        int button,
        bool pressed,
        const SbVec2s& cursorPos,
        const Gui::View3DInventorViewer* viewer
    ) override;
    bool mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer) override;
    bool keyPressed(bool pressed, int key) override;

    const Sketcher3D::GeoElementId3D& getSnapTarget() const
    {
        return snapTarget;
    }

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

private:
    /// Project screen space cursor onto the active sketch workplane.
    /// Falls back to the focal plane if the camera ray is parallel to the workplane.
    Base::Vector3d projectToSketchPlane(
        const SbVec2s& cursorPx,
        const Gui::View3DInventorViewer* viewer
    ) const;
    std::string getPickedSubName(const SbVec2s& cursorPx, const Gui::View3DInventorViewer* viewer) const;

    /// Resolve a snap target for the given cursor position. Updates snapTarget
    /// and returns the (possibly snapped) world position.
    Base::Vector3d applySnap(
        const Base::Vector3d& raw,
        const SbVec2s& cursorPos,
        const Gui::View3DInventorViewer* viewer
    );

    void ensurePlaneOverlay();
    void updatePlaneOverlay();
    void redrawPlaneQuad();
    void updatePlaneOverlayTranslation();
    void updatePlaneOverlaySize(const Base::Vector3d& cursorPos);
    void updateAxisHandleColors();

    void ensureSnapMarker();
    void hideSnapMarker();
    void showSnapMarker(const Base::Vector3d& pos);

    std::unique_ptr<DrawSketchHandler3D> handler;
    TaskSketcher3DTool* taskPanel {nullptr};

    ActivePlane activePlane {ActivePlane::XY};
    Base::Vector3d planeBase {0.0, 0.0, 0.0};
    SoSeparator* planeOverlay {nullptr};
    SoTranslation* planeOverlayTranslation {nullptr};
    SoCoordinate3* planeOverlayCoords {nullptr};
    float planeOverlaySize {0.0F};
    std::array<SoMaterial*, 3> axisHandleMaterials {};

    std::unique_ptr<SnapManager3D> snapManager;
    SoSeparator* snapMarker {nullptr};
    SoSwitch* snapMarkerSwitch {nullptr};
    SoTranslation* snapMarkerXf {nullptr};
    Sketcher3D::GeoElementId3D snapTarget {};

    /// Workbench name from before setEdit.same as partdesign
    std::string oldWb;
};

}  // namespace Sketcher3DGui
