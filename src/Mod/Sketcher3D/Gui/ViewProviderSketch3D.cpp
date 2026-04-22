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

#include <QMenu>

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

#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolBarManager.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandler3D.h"
#include "TaskDlgEditSketch3D.h"
#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)

namespace
{
inline QStringList editModeToolbarNames() {
    return {QStringLiteral("Sketcher3D Edit")};
}

inline QStringList nonEditModeToolbarNames() {
    return {
        QStringLiteral("Structure"),
        QStringLiteral("Sketcher"),
    };
}

constexpr float kPlaneHalfSize = 50.0F;
constexpr float kPlaneTransparency = 0.75F;

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

SoSeparator* buildPlaneQuad(ViewProviderSketch3D::ActivePlane p, const Base::Vector3d& base)
{
    auto* sep = new SoSeparator();
    sep->setName("Sketcher3DPlaneOverlay");

    // Clicks should pass through the plane hint to pick actual geometry.
    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    sep->addChild(pick);

    auto* material = new SoMaterial();
    material->diffuseColor.setValue(0.25F, 0.45F, 0.90F);
    material->transparency.setValue(kPlaneTransparency);
    sep->addChild(material);

    auto* style = new SoDrawStyle();
    style->style.setValue(SoDrawStyle::FILLED);
    sep->addChild(style);

    const float s = kPlaneHalfSize;
    const float bx = static_cast<float>(base.x);
    const float by = static_cast<float>(base.y);
    const float bz = static_cast<float>(base.z);
    auto* coords = new SoCoordinate3();
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
    sep->addChild(coords);

    auto* face = new SoFaceSet();
    face->numVertices.setNum(1);
    face->numVertices.set1Value(0, 4);
    sep->addChild(face);

    return sep;
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
    PointSize.setValue(8.0F);
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

    rebuildPlaneOverlay();

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

    if (planeOverlay) {
        pcRoot->removeChild(planeOverlay);
        planeOverlay->unref();
        planeOverlay = nullptr;
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
        Base::Vector3d p = projectToSketchPlane(cursorPos, viewer);
        return handler->pressButton(p);
    }
    if (button == SoMouseButtonEvent::BUTTON2) {
        purgeHandler();
        return true;
    }
    return false;
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    if (!handler) {
        return false;
    }
    Base::Vector3d p = projectToSketchPlane(cursorPos, viewer);
    return handler->mouseMove(p);
}

bool ViewProviderSketch3D::keyPressed(bool pressed, int key)
{
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
    rebuildPlaneOverlay();
    Base::Console().message("Sketcher3D: active plane = %s\n", planeLabel(activePlane));
}

void ViewProviderSketch3D::setPlaneBase(const Base::Vector3d& base)
{
    planeBase = base;
    rebuildPlaneOverlay();
}

void ViewProviderSketch3D::rebuildPlaneOverlay()
{
    if (planeOverlay) {
        pcRoot->removeChild(planeOverlay);
        planeOverlay->unref();
        planeOverlay = nullptr;
    }
    planeOverlay = buildPlaneQuad(activePlane, planeBase);
    planeOverlay->ref();
    pcRoot->addChild(planeOverlay);
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
