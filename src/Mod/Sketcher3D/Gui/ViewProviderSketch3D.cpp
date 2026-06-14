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


#include "PreCompiled.h"

#include <QMenu>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Precision.hxx>

#include <Base/Console.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Inventor/SoAutoZoomTranslation.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolBarManager.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandler3D.h"
#include "SnapManager3D.h"
#include "TaskDlgEditSketch3D.h"
#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)

namespace
{
inline QStringList editModeToolbarNames()
{
    return {QStringLiteral("Sketcher3D Edit")};
}

inline QStringList nonEditModeToolbarNames()
{
    return {
        QStringLiteral("Structure"),
        QStringLiteral("Sketcher"),
    };
}

constexpr const char* kEditingWorkbench = "SketcherWorkbench";

void setDiffuseColor(SoMaterial* material, const Color3f& color)
{
    material->diffuseColor.setValue(color[0], color[1], color[2]);
}

void setEmissiveColor(SoMaterial* material, const Color3f& color, float scale = 1.0F)
{
    material->emissiveColor.setValue(color[0] * scale, color[1] * scale, color[2] * scale);
}

SoSeparator* buildAxisHandle(std::array<SoMaterial*, 3>& axisMaterials)
{
    struct AxisDef
    {
        SbVec3f dir;
        SbVec3f side;
        const char* name;
    };
    const AxisDef axes[3] = {
        {{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, "Sketcher3DAxisHandleX"},
        {{0.0F, 1.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, "Sketcher3DAxisHandleY"},
        {{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F}, "Sketcher3DAxisHandleZ"},
    };

    const int kSegments = 3;

    auto* handle = new SoSeparator();
    handle->setName("Sketcher3DAxisHandle");

    for (std::size_t i = 0; i < 3; ++i) {
        auto* axisSep = new SoSeparator();
        axisSep->setName(axes[i].name);

        auto* material = new SoMaterial();
        setDiffuseColor(material, kInactiveAxisHandleColor);
        setEmissiveColor(material, kInactiveAxisHandleColor, kInactiveAxisHandleEmissiveScale);
        material->transparency.setValue(kAxisHandleTransparency);
        axisSep->addChild(material);
        axisMaterials[i] = material;

        auto* style = new SoDrawStyle();
        style->lineWidth.setValue(kAxisHandleLineWidth);
        axisSep->addChild(style);

        SbVec3f end = axes[i].dir * kAxisHandleLength;
        SbVec3f arrowBase = end - axes[i].dir * kAxisHandleArrowLength;
        SbVec3f sideVec = axes[i].side * (kAxisHandleArrowLength * kArrowHeadHalfWidthFactor);

        auto* coords = new SoCoordinate3();
        coords->point.setNum(6);
        coords->point.set1Value(0, SbVec3f(0.0F, 0.0F, 0.0F));
        coords->point.set1Value(1, end);
        coords->point.set1Value(2, end);
        coords->point.set1Value(3, arrowBase + sideVec);
        coords->point.set1Value(4, end);
        coords->point.set1Value(5, arrowBase - sideVec);
        axisSep->addChild(coords);

        auto* lineSet = new SoLineSet();
        lineSet->numVertices.setNum(kSegments);
        for (int s = 0; s < kSegments; ++s) {
            lineSet->numVertices.set1Value(s, 2);
        }
        axisSep->addChild(lineSet);

        handle->addChild(axisSep);
    }

    return handle;
}

SbVec3f planeNormal(ViewProviderSketch3D::ActivePlane p)
{
    switch (p) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            return {1.0F, 0.0F, 0.0F};
        case ViewProviderSketch3D::ActivePlane::ZX:
            return {0.0F, 1.0F, 0.0F};
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            return {0.0F, 0.0F, 1.0F};
    }
}

float planeSizeFromCursor(
    ViewProviderSketch3D::ActivePlane plane,
    const Base::Vector3d& base,
    const Base::Vector3d& cursor
)
{
    double dx = std::abs(cursor.x - base.x);
    double dy = std::abs(cursor.y - base.y);
    double dz = std::abs(cursor.z - base.z);

    double mouseSize = 0.0;
    switch (plane) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            mouseSize = std::max(dy, dz);
            break;
        case ViewProviderSketch3D::ActivePlane::ZX:
            mouseSize = std::max(dx, dz);
            break;
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            mouseSize = std::max(dx, dy);
            break;
    }

    return std::max(kAxisHandleLength, (float)mouseSize);
}

void writePlaneQuadCoords(SoCoordinate3* coords, ViewProviderSketch3D::ActivePlane p, float size)
{
    coords->point.setNum(kPlaneQuadVertexCount);
    SbVec3f* pts = coords->point.startEditing();
    switch (p) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            pts[0] = {0.0F, -size, -size};
            pts[1] = {0.0F, size, -size};
            pts[2] = {0.0F, size, size};
            pts[3] = {0.0F, -size, size};
            break;
        case ViewProviderSketch3D::ActivePlane::ZX:
            pts[0] = {-size, 0.0F, -size};
            pts[1] = {size, 0.0F, -size};
            pts[2] = {size, 0.0F, size};
            pts[3] = {-size, 0.0F, size};
            break;
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            pts[0] = {-size, -size, 0.0F};
            pts[1] = {size, -size, 0.0F};
            pts[2] = {size, size, 0.0F};
            pts[3] = {-size, size, 0.0F};
            break;
    }
    coords->point.finishEditing();
}

}  // namespace

ViewProviderSketch3D::ViewProviderSketch3D()
{
    PointSize.setValue(5.0F);
    LineWidth.setValue(2.0F);
}

ViewProviderSketch3D::~ViewProviderSketch3D() = default;

Sketcher3D::Sketch3DObject* ViewProviderSketch3D::getSketch3DObject() const
{
    return static_cast<Sketcher3D::Sketch3DObject*>(getObject());
}

void ViewProviderSketch3D::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (!taskPanel) {
        return;
    }
    auto* sketch = getSketch3DObject();
    if (!sketch) {
        return;
    }
    if (prop == &sketch->Geometry || prop == &sketch->Constraints) {
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit 3D Sketch"), receiver, member);
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSketch3D::setEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }

    if (Gui::Control().activeDialog()) {
        Base::Console().warning(
            "Sketcher3D: cannot enter edit mode while another task dialog is open.\n"
        );
        return false;
    }

    Gui::Selection().clearSelection();
    Gui::Selection().rmvPreselect();

    oldWb = Gui::Command::assureWorkbench(kEditingWorkbench);

    // save non edit toolbar state and switch to edit mode toolbars
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::SaveState
    );
    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::ForceAvailable
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::ForceHidden
    );

    ensurePlaneOverlay();
    ensureSnapMarker();
    snapManager = std::make_unique<SnapManager3D>(*this);

    auto* dlg = new TaskDlgEditSketch3D(this);
    Gui::Control().showDialog(dlg);

    return true;
}

void ViewProviderSketch3D::unsetEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        return;
    }

    purgeHandler();
    snapManager.reset();

    if (snapMarker) {
        pcRoot->removeChild(snapMarker);
        snapMarker->unref();
        snapMarker = nullptr;
        snapMarkerSwitch = nullptr;
        snapMarkerXf = nullptr;
    }

    if (planeOverlay) {
        pcRoot->removeChild(planeOverlay);
        planeOverlay->unref();
        planeOverlay = nullptr;
        planeOverlayTranslation = nullptr;
        planeOverlayCoords = nullptr;
        axisHandleMaterials = {};
        planeOverlaySize = kPlaneOverlaySize;
    }

    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );

    Gui::Control().closeDialog();

    // restore the workbench that was active before setEdit.
    if (!oldWb.empty()) {
        Gui::Command::assureWorkbench(oldWb.c_str());
        oldWb.clear();
    }
}

void ViewProviderSketch3D::activateHandler(std::unique_ptr<DrawSketchHandler3D> h)
{
    purgeHandler();
    handler = std::move(h);
    if (handler) {
        handler->activate(this);
    }
}

void ViewProviderSketch3D::purgeHandler()
{
    if (handler) {
        handler->quit();
        handler.reset();
    }
}

bool ViewProviderSketch3D::mouseButtonPressed(
    int button,
    bool pressed,
    const SbVec2s& cursorPos,
    const Gui::View3DInventorViewer* viewer
)
{
    if (!handler || !pressed) {
        return false;
    }

    if (button == SoMouseButtonEvent::BUTTON1) {
        Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
        Base::Vector3d p = applySnap(raw, cursorPos, viewer);
        hideSnapMarker();
        return handler->pressButton(p);
    }

    return false;
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    if (!handler) {
        return false;
    }
    Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
    updatePlaneOverlaySize(raw);
    Base::Vector3d p = applySnap(raw, cursorPos, viewer);
    if (snapTarget.isValid()) {
        showSnapMarker(p);
    }
    else {
        hideSnapMarker();
    }
    return handler->mouseMove(p);
}

bool ViewProviderSketch3D::keyPressed(bool pressed, int key)
{
    if (key == SoKeyboardEvent::ESCAPE) {
        if (handler) {
            if (!pressed) {
                handler->keyPressed(key);
            }
            return true;
        }
        return false;
    }

    if (!pressed) {
        return false;
    }
    if (key == SoKeyboardEvent::TAB) {
        cyclePlane();
        return true;
    }
    if (!handler) {
        return false;
    }
    return handler->keyPressed(key);
}

void ViewProviderSketch3D::cyclePlane()
{
    activePlane = static_cast<ActivePlane>((static_cast<int>(activePlane) + 1) % 3);
    redrawPlaneQuad();
    updateAxisHandleColors();
    if (taskPanel) {
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::setPlaneBase(const Base::Vector3d& base)
{
    planeBase = base;
    updatePlaneOverlayTranslation();
    if (taskPanel) {
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::ensurePlaneOverlay()
{
    if (planeOverlay) {
        updatePlaneOverlay();
        return;
    }

    planeOverlay = new SoSeparator();
    planeOverlay->setName("Sketcher3DPlaneOverlay");
    planeOverlay->ref();
    planeOverlay->renderCaching = SoSeparator::OFF;

    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    planeOverlay->addChild(pick);

    planeOverlayTranslation = new SoTranslation();
    planeOverlay->addChild(planeOverlayTranslation);

    auto* quadSep = new SoSeparator();
    quadSep->setName("Sketcher3DPlaneQuad");

    auto* material = new SoMaterial();
    setDiffuseColor(material, kPlaneOverlayColor);
    material->transparency.setValue(kPlaneTransparency);
    quadSep->addChild(material);

    auto* style = new SoDrawStyle();
    style->style.setValue(SoDrawStyle::FILLED);
    quadSep->addChild(style);

    planeOverlayCoords = new SoCoordinate3();
    quadSep->addChild(planeOverlayCoords);

    auto* face = new SoFaceSet();
    face->numVertices.setNum(1);
    face->numVertices.set1Value(0, kPlaneQuadVertexCount);
    quadSep->addChild(face);

    planeOverlay->addChild(quadSep);

    auto* handleSep = new SoSeparator();
    handleSep->setName("Sketcher3DAxisHandleAutoZoom");
    handleSep->addChild(new Gui::SoAutoZoomTranslation());

    handleSep->addChild(buildAxisHandle(axisHandleMaterials));

    planeOverlay->addChild(handleSep);

    pcRoot->addChild(planeOverlay);
    updatePlaneOverlay();
}

void ViewProviderSketch3D::updatePlaneOverlay()
{
    if (!planeOverlay) {
        return;
    }
    redrawPlaneQuad();
    updatePlaneOverlayTranslation();
    updateAxisHandleColors();
}

void ViewProviderSketch3D::redrawPlaneQuad()
{
    if (!planeOverlayCoords) {
        return;
    }
    writePlaneQuadCoords(planeOverlayCoords, activePlane, planeOverlaySize);
}

void ViewProviderSketch3D::updatePlaneOverlayTranslation()
{
    if (!planeOverlayTranslation) {
        return;
    }
    planeOverlayTranslation->translation.setValue(planeBase.x, planeBase.y, planeBase.z);
}

void ViewProviderSketch3D::updatePlaneOverlaySize(const Base::Vector3d& cursorPos)
{
    float updatedSize = planeSizeFromCursor(activePlane, planeBase, cursorPos);
    if (std::abs(planeOverlaySize - updatedSize) <= Precision::Confusion()) {
        return;
    }

    planeOverlaySize = updatedSize;
    redrawPlaneQuad();
}

void ViewProviderSketch3D::updateAxisHandleColors()
{
    int normalAxisIdx = (static_cast<int>(activePlane) + 2) % 3;

    for (int i = 0; i < 3; ++i) {
        SoMaterial* m = axisHandleMaterials[i];
        if (!m) {
            continue;
        }
        bool active = (i != normalAxisIdx);
        const Color3f& c = active ? kActiveAxisHandleColor : kInactiveAxisHandleColor;
        float em = active ? kActiveAxisHandleEmissiveScale : kInactiveAxisHandleEmissiveScale;
        setDiffuseColor(m, c);
        setEmissiveColor(m, c, em);
        m->transparency.setValue(kAxisHandleTransparency);
    }
}

Base::Vector3d ViewProviderSketch3D::projectToSketchPlane(
    const SbVec2s& cursorPx,
    const Gui::View3DInventorViewer* viewer
) const
{
    SbVec3f rayStart;
    SbVec3f rayEnd;
    viewer->projectPointToLine(cursorPx, rayStart, rayEnd);

    SbPlane plane(planeNormal(activePlane), SbVec3f(planeBase.x, planeBase.y, planeBase.z));

    SbVec3f hit;
    if (!plane.intersect(SbLine(rayStart, rayEnd), hit)) {
        // Camera ray is parallel to the workplane.
        SbVec3f fp = viewer->getPointOnFocalPlane(cursorPx);
        return Base::Vector3d(fp[0], fp[1], fp[2]);
    }
    return Base::Vector3d(hit[0], hit[1], hit[2]);
}

Base::Vector3d ViewProviderSketch3D::applySnap(
    const Base::Vector3d& raw,
    const SbVec2s& cursorPos,
    const Gui::View3DInventorViewer* viewer
)
{
    if (!snapManager) {
        snapTarget = {};
        return raw;
    }
    std::string pickedSubName = getPickedSubName(cursorPos, viewer);
    return snapManager->snap(raw, pickedSubName, snapTarget);
}

std::string ViewProviderSketch3D::getPickedSubName(
    const SbVec2s& cursorPx,
    const Gui::View3DInventorViewer* viewer
) const
{
    if (!viewer) {
        return {};
    }

    std::unique_ptr<SoPickedPoint> point(getPointOnRay(cursorPx, viewer));
    if (!point) {
        return {};
    }

    std::string subName;
    if (!getElementPicked(point.get(), subName)) {
        return {};
    }
    return subName;
}

void ViewProviderSketch3D::ensureSnapMarker()
{
    if (snapMarker) {
        return;
    }
    snapMarker = new SoSeparator();
    snapMarker->setName("Sketcher3DSnapMarker");
    snapMarker->ref();
    snapMarker->renderCaching = SoSeparator::OFF;

    snapMarkerSwitch = new SoSwitch();
    snapMarkerSwitch->whichChild = SO_SWITCH_NONE;
    snapMarker->addChild(snapMarkerSwitch);

    auto* content = new SoSeparator();

    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    content->addChild(pick);

    auto* material = new SoMaterial();
    setDiffuseColor(material, kSnapMarkerDiffuseColor);
    setEmissiveColor(material, kSnapMarkerEmissiveColor);
    content->addChild(material);

    snapMarkerXf = new SoTranslation();
    content->addChild(snapMarkerXf);

    auto* sphere = new SoSphere();
    sphere->radius.setValue(kSnapMarkerRadius);
    content->addChild(sphere);

    snapMarkerSwitch->addChild(content);

    pcRoot->addChild(snapMarker);
}

void ViewProviderSketch3D::hideSnapMarker()
{
    if (snapMarkerSwitch) {
        snapMarkerSwitch->whichChild = SO_SWITCH_NONE;
    }
}

void ViewProviderSketch3D::showSnapMarker(const Base::Vector3d& pos)
{
    if (!snapMarkerSwitch || !snapMarkerXf) {
        return;
    }
    snapMarkerXf->translation.setValue(pos.x, pos.y, pos.z);
    snapMarkerSwitch->whichChild = 0;
}
