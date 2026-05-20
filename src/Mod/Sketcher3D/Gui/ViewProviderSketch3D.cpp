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

#include <cmath>
#include <memory>

#include <QMenu>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <Base/Console.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolBarManager.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Part/App/TopoShape.h>
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

constexpr float kPlaneHalfSize = 50.0F;
constexpr float kPlaneTransparency = 0.75F;
constexpr const char* kEditingWorkbench = "SketcherWorkbench";

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

void writePlaneQuadCoords(SoCoordinate3* coords,
                          ViewProviderSketch3D::ActivePlane p,
                          const Base::Vector3d& base)
{
    const float s = kPlaneHalfSize;
    const float bx = static_cast<float>(base.x);
    const float by = static_cast<float>(base.y);
    const float bz = static_cast<float>(base.z);
    coords->point.setNum(4);
    SbVec3f* pts = coords->point.startEditing();
    switch (p) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            pts[0] = {bx, by - s, bz - s};
            pts[1] = {bx, by + s, bz - s};
            pts[2] = {bx, by + s, bz + s};
            pts[3] = {bx, by - s, bz + s};
            break;
        case ViewProviderSketch3D::ActivePlane::ZX:
            pts[0] = {bx - s, by, bz - s};
            pts[1] = {bx + s, by, bz - s};
            pts[2] = {bx + s, by, bz + s};
            pts[3] = {bx - s, by, bz + s};
            break;
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            pts[0] = {bx - s, by - s, bz};
            pts[1] = {bx + s, by - s, bz};
            pts[2] = {bx + s, by + s, bz};
            pts[3] = {bx - s, by + s, bz};
            break;
    }
    coords->point.finishEditing();
}

const char* planeLabel(ViewProviderSketch3D::ActivePlane p)
{
    switch (p) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            return "YZ";
        case ViewProviderSketch3D::ActivePlane::ZX:
            return "ZX";
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            return "XY";
    }
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
        planeOverlayCoords = nullptr;
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
        const Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
        Base::Vector3d p = raw;
        snapTarget = {};
        if (snapManager) {
            const std::string pickedSubName = getPickedSubName(cursorPos, viewer);
            p = snapManager->snap(raw, pickedSubName, snapTarget);
        }
        updateSnapMarker(false, p);  // hide on click; next move will re-show
        return handler->pressButton(p);
    }

    return false;
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    if (!handler) {
        return false;
    }
    const Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
    Base::Vector3d p = raw;
    bool showMarker = false;
    snapTarget = {};
    if (snapManager) {
        const std::string pickedSubName = getPickedSubName(cursorPos, viewer);
        p = snapManager->snap(raw, pickedSubName, snapTarget);
        showMarker = snapTarget.isValid();
    }
    updateSnapMarker(showMarker, p);
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
    const int next = (static_cast<int>(activePlane) + 1) % 3;
    activePlane = static_cast<ActivePlane>(next);
    updatePlaneOverlay();
    if (taskPanel) {
        taskPanel->refresh();
    }
    Base::Console().message("Sketcher3D: active plane = %s\n", planeLabel(activePlane));
}

void ViewProviderSketch3D::setPlaneBase(const Base::Vector3d& base)
{
    planeBase = base;
    updatePlaneOverlay();
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

    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    planeOverlay->addChild(pick);

    auto* material = new SoMaterial();
    material->diffuseColor.setValue(0.25F, 0.45F, 0.90F);
    material->transparency.setValue(kPlaneTransparency);
    planeOverlay->addChild(material);

    auto* style = new SoDrawStyle();
    style->style.setValue(SoDrawStyle::FILLED);
    planeOverlay->addChild(style);

    planeOverlayCoords = new SoCoordinate3();
    writePlaneQuadCoords(planeOverlayCoords, activePlane, planeBase);
    planeOverlay->addChild(planeOverlayCoords);

    auto* face = new SoFaceSet();
    face->numVertices.setNum(1);
    face->numVertices.set1Value(0, 4);
    planeOverlay->addChild(face);

    pcRoot->addChild(planeOverlay);
}

void ViewProviderSketch3D::updatePlaneOverlay()
{
    if (!planeOverlayCoords) {
        ensurePlaneOverlay();
        return;
    }
    writePlaneQuadCoords(planeOverlayCoords, activePlane, planeBase);
}

Base::Vector3d ViewProviderSketch3D::projectToSketchPlane(
    const SbVec2s& cursorPx,
    const Gui::View3DInventorViewer* viewer
) const
{
    SbVec3f near;
    SbVec3f far;
    viewer->projectPointToLine(cursorPx, near, far);

    const SbVec3f n = planeNormal(activePlane);
    const SbVec3f base(
        static_cast<float>(planeBase.x),
        static_cast<float>(planeBase.y),
        static_cast<float>(planeBase.z)
    );
    const SbVec3f dir = far - near;
    const float denom = dir.dot(n);
    if (std::abs(denom) < 1e-9F) {
        SbVec3f fp = viewer->getPointOnFocalPlane(cursorPx);
        return Base::Vector3d(fp[0], fp[1], fp[2]);
    }
    const float t = (base - near).dot(n) / denom;
    const SbVec3f p = near + dir * t;
    return Base::Vector3d(p[0], p[1], p[2]);
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
    material->diffuseColor.setValue(1.0F, 0.55F, 0.0F);
    material->emissiveColor.setValue(0.6F, 0.3F, 0.0F);
    content->addChild(material);

    snapMarkerXf = new SoTranslation();
    snapMarkerXf->translation.setValue(0.0F, 0.0F, 0.0F);
    content->addChild(snapMarkerXf);

    auto* sphere = new SoSphere();
    sphere->radius.setValue(0.6F);
    content->addChild(sphere);

    snapMarkerSwitch->addChild(content);

    pcRoot->addChild(snapMarker);
}

void ViewProviderSketch3D::updateSnapMarker(bool visible, const Base::Vector3d& pos)
{
    if (!snapMarkerSwitch || !snapMarkerXf) {
        return;
    }
    if (visible) {
        snapMarkerXf->translation.setValue(
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
        snapMarkerSwitch->whichChild = 0;
    }
    else {
        snapMarkerSwitch->whichChild = SO_SWITCH_NONE;
    }
}
