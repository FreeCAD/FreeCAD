// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost/core/ignore_unused.hpp>
#include <QMessageBox>
#include <QTimer>
#include <QMenu>
#include <vector>
#include <sstream>
#include <iostream>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/sensors/SoSensor.h>
#endif

#include <chrono>

#include <App/Link.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>

#include <Base/Tools.h>

#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/MDIView.h>
#include <Gui/SoFCCSysDragger.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewParams.h>

#include <Mod/Assembly/App/AssemblyLink.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/JointGroup.h>
#include <Mod/Assembly/App/ViewGroup.h>
#include <Mod/Assembly/App/BomGroup.h>
#include <Mod/PartDesign/App/Body.h>

#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyPy.h"

#include <Gui/Utilities.h>


using namespace Assembly;
using namespace AssemblyGui;

void printPlacement(Base::Placement plc, const char* name)
{
    Base::Vector3d pos = plc.getPosition();
    Base::Vector3d axis;
    double angle;
    Base::Rotation rot = plc.getRotation();
    rot.getRawValue(axis, angle);
    Base::Console().Warning(
        "placement %s : position (%.1f, %.1f, %.1f) - axis (%.1f, %.1f, %.1f) angle %.1f\n",
        name,
        pos.x,
        pos.y,
        pos.z,
        axis.x,
        axis.y,
        axis.z,
        angle);
}

PROPERTY_SOURCE(AssemblyGui::ViewProviderAssembly, Gui::ViewProviderPart)

ViewProviderAssembly::ViewProviderAssembly()
    : SelectionObserver(false)
    , dragMode(DragMode::None)
    , canStartDragging(false)
    , partMoving(false)
    , enableMovement(true)
    , moveOnlyPreselected(false)
    , moveInCommand(true)
    , ctrlPressed(false)
    , lastClickTime(0)
    , jointVisibilitiesBackup({})
    , docsToMove({})
{}

ViewProviderAssembly::~ViewProviderAssembly() = default;

QIcon ViewProviderAssembly::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Geoassembly.svg");
}

void ViewProviderAssembly::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);

    QAction* act = menu->addAction(QObject::tr("Active object"));
    act->setCheckable(true);
    act->setChecked(isActivePart());
    func->trigger(act, [this]() {
        this->doubleClicked();
    });

    ViewProviderDragger::setupContextMenu(menu, receiver, member);  // NOLINT
}

bool ViewProviderAssembly::doubleClicked()
{
    if (isInEditMode()) {
        // Part is already 'Active' so we exit edit mode.
        // Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
        getDocument()->resetEdit();
    }
    else {
        // assure the Assembly workbench
        if (App::GetApplication()
                .GetUserParameter()
                .GetGroup("BaseApp")
                ->GetGroup("Preferences")
                ->GetGroup("Mod/Assembly")
                ->GetBool("SwitchToWB", true)) {
            Gui::Command::assureWorkbench("AssemblyWorkbench");
        }

        // Part is not 'Active' so we enter edit mode to make it so.
        getDocument()->setEdit(this);
    }

    Gui::Selection().clearSelection();
    return true;
}

bool ViewProviderAssembly::canDragObject(App::DocumentObject* obj) const
{
    // The user should not be able to drag the joint group out of the assembly
    if (!obj || obj->getTypeId() == Assembly::JointGroup::getClassTypeId()) {
        return false;
    }

    return true;
}

bool ViewProviderAssembly::canDragObjectToTarget(App::DocumentObject* obj,
                                                 App::DocumentObject* target) const
{
    // If a solid is removed from the assembly, its joints need to be removed.
    bool prompted = false;
    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());

    // If target is null then it's being dropped on a doc.
    if (target && assemblyPart->hasObject(target)) {
        // If the obj stays in assembly then its ok.
        return true;
    }

    // Combine the joints and groundedJoints vectors into one for simplicity.
    std::vector<App::DocumentObject*> allJoints = assemblyPart->getJoints();
    std::vector<App::DocumentObject*> groundedJoints = assemblyPart->getGroundedJoints();
    allJoints.insert(allJoints.end(), groundedJoints.begin(), groundedJoints.end());

    for (auto joint : allJoints) {
        // getLinkObjFromProp returns nullptr if the property doesn't exist.
        App::DocumentObject* obj1 = AssemblyObject::getObjFromRef(joint, "Reference1");
        App::DocumentObject* obj2 = AssemblyObject::getObjFromRef(joint, "Reference2");
        App::DocumentObject* part1 = assemblyPart->getMovingPartFromRef(joint, "Reference1");
        App::DocumentObject* part2 = assemblyPart->getMovingPartFromRef(joint, "Reference2");
        App::DocumentObject* obj3 = AssemblyObject::getObjFromProp(joint, "ObjectToGround");
        if (obj == obj1 || obj == obj2 || obj == part1 || obj == part2 || obj == obj3) {
            if (!prompted) {
                prompted = true;
                QMessageBox msgBox;
                msgBox.setText(tr("The object is associated to one or more joints."));
                msgBox.setInformativeText(
                    tr("Do you want to move the object and delete associated joints?"));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                int ret = msgBox.exec();

                if (ret == QMessageBox::No) {
                    return false;
                }
            }
            Gui::Command::doCommand(Gui::Command::Gui,
                                    "App.activeDocument().removeObject('%s')",
                                    joint->getNameInDocument());
        }
    }

    return true;
}

bool ViewProviderAssembly::setEdit(int mode)
{
    if (mode == ViewProvider::Default) {
        // Set the part as 'Activated' ie bold in the tree.
        Gui::Command::doCommand(Gui::Command::Gui,
                                "appDoc = App.getDocument('%s')\n"
                                "Gui.getDocument(appDoc).ActiveView.setActiveObject('%s', "
                                "appDoc.getObject('%s'))",
                                this->getObject()->getDocument()->getName(),
                                PARTKEY,
                                this->getObject()->getNameInDocument());

        // When we set edit, we update the grounded joints placements to support :
        // - If user transformed the grounded object
        // - For nested assemblies where the grounded object moves around.
        auto* assembly = static_cast<AssemblyObject*>(getObject());
        assembly->updateGroundedJointsPlacements();

        setDragger();

        attachSelection();

        return true;
    }

    return ViewProviderPart::setEdit(mode);
}

void ViewProviderAssembly::unsetEdit(int mode)
{
    if (mode == ViewProvider::Default) {
        canStartDragging = false;
        partMoving = false;
        docsToMove.clear();

        unsetDragger();

        detachSelection();

        // Check if the view is still active before trying to deactivate the assembly.
        auto activeView = getDocument()->getActiveView();
        if (!activeView) {
            return;
        }

        // Set the part as not 'Activated' ie not bold in the tree.
        Gui::Command::doCommand(Gui::Command::Gui,
                                "appDoc = App.getDocument('%s')\n"
                                "Gui.getDocument(appDoc).ActiveView.setActiveObject('%s', None)",
                                this->getObject()->getDocument()->getName(),
                                PARTKEY);
        return;
    }

    ViewProviderPart::unsetEdit(mode);
}

void ViewProviderAssembly::setDragger()
{
    // Create the dragger coin object
    assert(!asmDragger);
    asmDragger = new Gui::SoFCCSysDragger();
    asmDragger->setAxisColors(Gui::ViewParams::instance()->getAxisXColor(),
                              Gui::ViewParams::instance()->getAxisYColor(),
                              Gui::ViewParams::instance()->getAxisZColor());
    asmDragger->draggerSize.setValue(Gui::ViewParams::instance()->getDraggerScale());

    asmDraggerSwitch = new SoSwitch(SO_SWITCH_NONE);
    asmDraggerSwitch->addChild(asmDragger);

    pcRoot->insertChild(asmDraggerSwitch, 0);
    asmDraggerSwitch->ref();
    asmDragger->ref();
}

void ViewProviderAssembly::unsetDragger()
{
    pcRoot->removeChild(asmDraggerSwitch);
    asmDragger->unref();
    asmDragger = nullptr;
    asmDraggerSwitch->unref();
    asmDraggerSwitch = nullptr;
}

void ViewProviderAssembly::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    ViewProviderPart::setEditViewer(viewer, ModNum);

    if (asmDragger && viewer) {
        asmDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());
    }
}

bool ViewProviderAssembly::isInEditMode() const
{
    return asmDragger != nullptr;
}

App::DocumentObject* ViewProviderAssembly::getActivePart() const
{
    auto activeView = getDocument()->getActiveView();
    if (!activeView) {
        return nullptr;
    }

    return activeView->getActiveObject<App::DocumentObject*>(PARTKEY);
}

bool ViewProviderAssembly::keyPressed(bool pressed, int key)
{
    if (key == SoKeyboardEvent::ESCAPE) {
        if (isInEditMode()) {
            if (Gui::Control().activeDialog()) {
                return true;
            }

            ParameterGrp::handle hPgr = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Assembly");

            return !hPgr->GetBool("LeaveEditWithEscape", true);
        }
    }

    if (key == SoKeyboardEvent::LEFT_CONTROL || key == SoKeyboardEvent::RIGHT_CONTROL) {
        ctrlPressed = pressed;
    }

    return false;  // handle all other key events
}

bool ViewProviderAssembly::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    try {
        return tryMouseMove(cursorPos, viewer);
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning("%s\n", e.what());
        return false;
    }
}

bool ViewProviderAssembly::tryMouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    if (!isInEditMode()) {
        return false;
    }

    // Initialize or cancel the dragging of parts
    if (canStartDragging) {
        canStartDragging = false;

        if (enableMovement && getSelectedObjectsWithinAssembly()) {
            initMove(cursorPos, viewer);
        }
    }

    // Do the dragging of parts
    if (partMoving) {
        Base::Vector3d newPos, newPosRot;
        if (dragMode == DragMode::RotationOnPlane) {
            SbVec3f vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
            newPosRot = Base::Vector3d(vec[0], vec[1], vec[2]);
        }
        else if (dragMode == DragMode::TranslationOnAxis) {
            Base::Vector3d zAxis = jcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., 1.));
            Base::Vector3d pos = jcsGlobalPlc.getPosition();
            SbVec3f axisCenter(pos.x, pos.y, pos.z);
            SbVec3f axis(zAxis.x, zAxis.y, zAxis.z);
            SbVec3f vec = viewer->getPointOnLine(cursorPos, axisCenter, axis);
            newPos = Base::Vector3d(vec[0], vec[1], vec[2]);
        }
        else if (dragMode == DragMode::TranslationOnAxisAndRotationOnePlane) {
            SbVec3f vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
            newPosRot = Base::Vector3d(vec[0], vec[1], vec[2]);

            Base::Vector3d zAxis = jcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., 1.));
            Base::Vector3d pos = jcsGlobalPlc.getPosition();
            SbVec3f axisCenter(pos.x, pos.y, pos.z);
            SbVec3f axis(zAxis.x, zAxis.y, zAxis.z);
            vec = viewer->getPointOnLine(cursorPos, axisCenter, axis);
            newPos = Base::Vector3d(vec[0], vec[1], vec[2]);
        }
        else if (dragMode == DragMode::TranslationOnPlane) {
            SbVec3f vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
            newPos = Base::Vector3d(vec[0], vec[1], vec[2]);
        }
        else {
            SbVec3f vec = viewer->getPointOnFocalPlane(cursorPos);
            newPos = Base::Vector3d(vec[0], vec[1], vec[2]);
        }


        for (auto& objToMove : docsToMove) {
            App::DocumentObject* obj = objToMove.obj;
            auto* propPlacement =
                dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
            if (propPlacement) {
                Base::Placement plc = objToMove.plc;
                // Base::Console().Warning("newPos %f %f %f\n", newPos.x, newPos.y, newPos.z);

                if (dragMode == DragMode::RotationOnPlane) {
                    Base::Vector3d center = jcsGlobalPlc.getPosition();
                    Base::Vector3d norm =
                        jcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., -1.));
                    double angle =
                        (newPosRot - center).GetAngleOriented(initialPositionRot - center, norm);
                    // Base::Console().Warning("angle %f\n", angle);
                    Base::Rotation zRotation = Base::Rotation(Base::Vector3d(0., 0., 1.), angle);
                    Base::Placement rotatedGlovalJcsPlc =
                        jcsGlobalPlc * Base::Placement(Base::Vector3d(), zRotation);
                    Base::Placement jcsPlcRelativeToPart = plc.inverse() * jcsGlobalPlc;
                    plc = rotatedGlovalJcsPlc * jcsPlcRelativeToPart.inverse();
                }
                else if (dragMode == DragMode::TranslationOnAxis) {
                    Base::Vector3d pos = plc.getPosition() + (newPos - initialPosition);
                    plc.setPosition(pos);
                }
                else if (dragMode == DragMode::TranslationOnAxisAndRotationOnePlane) {
                    Base::Vector3d pos = plc.getPosition() + (newPos - initialPosition);
                    plc.setPosition(pos);

                    Base::Placement newJcsGlobalPlc = jcsGlobalPlc;
                    newJcsGlobalPlc.setPosition(jcsGlobalPlc.getPosition()
                                                + (newPos - initialPosition));

                    Base::Vector3d center = newJcsGlobalPlc.getPosition();
                    Base::Vector3d norm =
                        newJcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., -1.));

                    Base::Vector3d projInitialPositionRot =
                        initialPositionRot.ProjectToPlane(newJcsGlobalPlc.getPosition(), norm);
                    boost::ignore_unused(projInitialPositionRot);
                    double angle =
                        (newPosRot - center).GetAngleOriented(initialPositionRot - center, norm);
                    // Base::Console().Warning("angle %f\n", angle);
                    Base::Rotation zRotation = Base::Rotation(Base::Vector3d(0., 0., 1.), angle);
                    Base::Placement rotatedGlovalJcsPlc =
                        newJcsGlobalPlc * Base::Placement(Base::Vector3d(), zRotation);
                    Base::Placement jcsPlcRelativeToPart = plc.inverse() * newJcsGlobalPlc;
                    plc = rotatedGlovalJcsPlc * jcsPlcRelativeToPart.inverse();
                }
                else if (dragMode == DragMode::TranslationOnPlane) {
                    Base::Vector3d pos = plc.getPosition() + (newPos - initialPosition);
                    plc.setPosition(pos);
                }
                else {  // DragMode::Translation
                    Base::Vector3d delta = newPos - prevPosition;

                    Base::Vector3d pos = propPlacement->getValue().getPosition() + delta;
                    // Base::Vector3d pos = newPos + (plc.getPosition() - initialPosition);
                    plc.setPosition(pos);
                }
                propPlacement->setValue(plc);
            }
        }

        prevPosition = newPos;

        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Assembly");
        bool solveOnMove = hGrp->GetBool("SolveOnMove", true);
        if (solveOnMove && dragMode != DragMode::TranslationNoSolve) {
            // assemblyPart->solve(/*enableRedo = */ false, /*updateJCS = */ false);
            assemblyPart->doDragStep();
        }
        else {
            assemblyPart->redrawJointPlacements(assemblyPart->getJoints());
        }
    }
    return false;
}

bool ViewProviderAssembly::mouseButtonPressed(int Button,
                                              bool pressed,
                                              const SbVec2s& cursorPos,
                                              const Gui::View3DInventorViewer* viewer)
{
    Q_UNUSED(cursorPos);
    Q_UNUSED(viewer);

    if (!isInEditMode()) {
        return false;
    }

    // Left Mouse button ****************************************************
    if (Button == 1) {
        if (pressed && !getDraggerVisibility()) {
            // Check for double-click
            auto now = std::chrono::steady_clock::now();
            long nowMillis =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                    .count();
            if (nowMillis - lastClickTime < 500) {
                auto* joint = getSelectedJoint();
                if (joint) {
                    // Double-click detected
                    // We start by clearing selection such that the second click selects the joint
                    // and not the assembly.
                    Gui::Selection().clearSelection();
                    // singleShot timer to make sure this happens after the release of the click.
                    // Else the release will trigger a removeSelection of what
                    // doubleClickedIn3dView adds to the selection.
                    QTimer::singleShot(50, [this]() {
                        doubleClickedIn3dView();
                    });
                    return true;
                }
            }
            // First click detected
            lastClickTime = nowMillis;

            canStartDragging = true;
        }
        else {  // Button 1 released
            // release event is not received when user click on a part for selection.
            // So we use SelectionObserver to know if something got selected.

            canStartDragging = false;
            if (partMoving) {
                endMove();
                return true;
            }
        }
    }

    return false;
}

void ViewProviderAssembly::doubleClickedIn3dView()
{
    // Double clicking on a joint should start editing it.
    auto* joint = getSelectedJoint();

    if (joint) {
        std::string obj_name = joint->getNameInDocument();
        std::string doc_name = joint->getDocument()->getName();

        std::string cmd = "import JointObject\n"
                          "obj = App.getDocument('"
            + doc_name + "').getObject('" + obj_name
            + "')\n"
              "Gui.Control.showDialog(JointObject.TaskAssemblyCreateJoint(0, obj))";

        Gui::Command::runCommand(Gui::Command::App, cmd.c_str());
    }
}

bool ViewProviderAssembly::canDragObjectIn3d(App::DocumentObject* obj) const
{
    if (!obj) {
        return false;
    }

    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());

    // Check if the selected object is a child of the assembly
    if (!assemblyPart->hasObject(obj, true)) {
        // hasObject does not detect LinkElements (see
        // https://github.com/FreeCAD/FreeCAD/issues/16113) the following block can be removed if
        // the issue is fixed :
        auto* linkEl = dynamic_cast<App::LinkElement*>(obj);
        if (linkEl) {
            auto* linkGroup = linkEl->getLinkGroup();
            if (assemblyPart->hasObject(linkGroup, true)) {
                return true;
            }
        }

        return false;
    }

    auto* propPlacement =
        dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
    if (!propPlacement) {
        return false;
    }

    // We have to exclude Grounded joints as they happen to have a Placement prop
    auto* propLink = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("ObjectToGround"));
    if (propLink) {
        return false;
    }

    // We have to exclude grounded objects as they should not move.
    if (assemblyPart->isPartGrounded(obj)) {
        return false;
    }
    return true;
}

App::DocumentObject* ViewProviderAssembly::getSelectedJoint()
{
    auto sel = Gui::Selection().getSelectionEx("", App::DocumentObject::getClassTypeId());

    if (sel.size() == 1) {  // Handle double click only if only one obj selected.
        App::DocumentObject* obj = sel[0].getObject();
        if (obj) {
            auto* prop =
                dynamic_cast<App::PropertyBool*>(obj->getPropertyByName("EnableLengthMin"));
            if (prop) {
                return obj;
            }
        }
    }
    return nullptr;
}

bool ViewProviderAssembly::getSelectedObjectsWithinAssembly(bool addPreselection, bool onlySolids)
{
    // check the current selection, and check if any of the selected objects are within this
    // App::Part
    //  If any, put them into the vector docsToMove and return true.
    //  Get the document
    docsToMove.clear();

    // Get the assembly object for this ViewProvider
    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());

    if (!assemblyPart) {
        return false;
    }

    if (!moveOnlyPreselected) {
        for (auto& selObj : Gui::Selection().getSelectionEx("",
                                                            App::DocumentObject::getClassTypeId(),
                                                            Gui::ResolveMode::NoResolve)) {
            // getSubNames() returns ["Body001.Pad.Face14", "Body002.Pad.Face7"]
            //  if you have several objects within the same assembly selected.

            std::vector<std::string> objsSubNames = selObj.getSubNames();
            for (auto& subNamesStr : objsSubNames) {
                std::vector<std::string> subNames = Base::Tools::splitSubName(subNamesStr);
                if (subNames.empty()) {
                    continue;
                }
                if (onlySolids && subNames.back() != "") {
                    continue;
                }

                App::DocumentObject* selRoot = selObj.getObject();
                App::DocumentObject* obj = assemblyPart->getObjFromRef(selRoot, subNamesStr);
                if (!obj) {
                    // In case of sub-assembly, the jointgroup would trigger the dragger.
                    continue;
                }
                App::DocumentObject* part =
                    assemblyPart->getMovingPartFromRef(selRoot, subNamesStr);

                if (!canDragObjectIn3d(part)) {
                    continue;
                }

                auto* pPlc =
                    dynamic_cast<App::PropertyPlacement*>(part->getPropertyByName("Placement"));

                MovingObject movingObj(part, pPlc->getValue(), selRoot, subNamesStr);

                docsToMove.emplace_back(movingObj);
            }
        }
    }

    // This function is called before the selection is updated. So if a user click and drag a part
    // it is not selected at that point. So we need to get the preselection too.
    if (addPreselection && Gui::Selection().hasPreselection()) {

        // Base::Console().Warning("Gui::Selection().getPreselection().pSubName %s\n",
        //                         Gui::Selection().getPreselection().pSubName);

        App::DocumentObject* selRoot = Gui::Selection().getPreselection().Object.getObject();
        std::string sub = Gui::Selection().getPreselection().pSubName;

        App::DocumentObject* obj = assemblyPart->getMovingPartFromRef(selRoot, sub);
        if (canDragObjectIn3d(obj)) {

            bool alreadyIn = false;
            for (auto& movingObj : docsToMove) {
                App::DocumentObject* obji = movingObj.obj;
                if (obji == obj) {
                    alreadyIn = true;
                    break;
                }
            }

            if (!alreadyIn) {
                auto* pPlc =
                    dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
                if (!ctrlPressed && !moveOnlyPreselected) {
                    Gui::Selection().clearSelection();
                    docsToMove.clear();
                }
                MovingObject movingObj(obj, pPlc->getValue(), selRoot, sub);

                docsToMove.emplace_back(movingObj);
            }
        }
    }

    return !docsToMove.empty();
}

ViewProviderAssembly::DragMode ViewProviderAssembly::findDragMode()
{
    auto addPartsToMove = [&](const std::vector<Assembly::ObjRef>& refs) {
        for (auto& partRef : refs) {
            auto* pPlc =
                dynamic_cast<App::PropertyPlacement*>(partRef.obj->getPropertyByName("Placement"));
            if (pPlc) {
                App::DocumentObject* selRoot = partRef.ref->getValue();
                if (!selRoot) {
                    continue;
                }
                std::vector<std::string> subs = partRef.ref->getSubValues();
                if (subs.empty()) {
                    continue;
                }

                docsToMove.emplace_back(partRef.obj, pPlc->getValue(), selRoot, subs[0]);
            }
        }
    };

    if (docsToMove.size() == 1) {
        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        std::string pName;
        movingJoint = assemblyPart->getJointOfPartConnectingToGround(docsToMove[0].obj, pName);

        if (!movingJoint) {
            // In this case the user is moving an object that is not grounded
            // Then we want to also move other parts that may be connected to it.
            // In particular for case of flexible subassemblies or it looks really weird
            std::vector<Assembly::ObjRef> connectedParts =
                assemblyPart->getDownstreamParts(docsToMove[0].obj, movingJoint);

            addPartsToMove(connectedParts);
            return DragMode::TranslationNoSolve;
        }

        JointType jointType = AssemblyObject::getJointType(movingJoint);
        if (jointType == JointType::Fixed) {
            // If fixed joint we need to find the upstream joint to find move mode.
            // For example : Gnd -(revolute)- A -(fixed)- B : if user try to move B, then we should
            // actually move A
            auto* upPart =
                assemblyPart->getUpstreamMovingPart(docsToMove[0].obj, movingJoint, pName);
            if (!movingJoint) {
                return DragMode::Translation;
            }
            docsToMove.clear();
            if (!upPart) {
                return DragMode::None;
            }

            auto* pPlc =
                dynamic_cast<App::PropertyPlacement*>(upPart->getPropertyByName("Placement"));
            if (pPlc) {
                auto* ref = dynamic_cast<App::PropertyXLinkSub*>(
                    movingJoint->getPropertyByName(pName.c_str()));

                App::DocumentObject* selRoot = ref->getValue();
                if (!selRoot) {
                    return DragMode::None;
                }
                std::vector<std::string> subs = ref->getSubValues();
                if (subs.empty()) {
                    return DragMode::None;
                }

                docsToMove.emplace_back(upPart, pPlc->getValue(), selRoot, subs[0]);
            }

            jointType = AssemblyObject::getJointType(movingJoint);
        }

        const char* plcPropName = (pName == "Reference1") ? "Placement1" : "Placement2";

        // jcsPlc is relative to the Object
        jcsPlc = App::GeoFeature::getPlacementFromProp(movingJoint, plcPropName);

        // Make jcsGlobalPlc relative to the origin of the doc
        auto* ref =
            dynamic_cast<App::PropertyXLinkSub*>(movingJoint->getPropertyByName(pName.c_str()));
        if (!ref) {
            return DragMode::Translation;
        }
        auto* obj = assemblyPart->getObjFromRef(movingJoint, pName.c_str());
        Base::Placement global_plc = App::GeoFeature::getGlobalPlacement(obj, ref);
        jcsGlobalPlc = global_plc * jcsPlc;

        // Add downstream parts so that they move together
        std::vector<Assembly::ObjRef> downstreamParts =
            assemblyPart->getDownstreamParts(docsToMove[0].obj, movingJoint);
        addPartsToMove(downstreamParts);

        if (jointType == JointType::Revolute) {
            return DragMode::RotationOnPlane;
        }
        else if (jointType == JointType::Slider) {
            return DragMode::TranslationOnAxis;
        }
        else if (jointType == JointType::Cylindrical) {
            return DragMode::TranslationOnAxisAndRotationOnePlane;
        }
        else if (jointType == JointType::Ball) {
            // return DragMode::Ball;
        }
        else if (jointType == JointType::Distance) {
            //  depends on the type of distance. For example plane-plane:
            DistanceType distanceType = AssemblyObject::getDistanceType(movingJoint);
            if (distanceType == DistanceType::PlanePlane || distanceType == DistanceType::Other) {
                return DragMode::TranslationOnPlane;
            }
        }
    }
    return DragMode::Translation;
}

void ViewProviderAssembly::initMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    try {
        tryInitMove(cursorPos, viewer);
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning("%s\n", e.what());
    }
}

void ViewProviderAssembly::tryInitMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    dragMode = findDragMode();
    if (dragMode == DragMode::None) {
        return;
    }

    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
    // When the user drag parts, we switch off all joints visibility and only show the movingjoint
    jointVisibilitiesBackup.clear();
    auto joints = assemblyPart->getJoints();
    for (auto* joint : joints) {
        if (!joint) {
            continue;
        }
        bool visible = joint->Visibility.getValue();
        jointVisibilitiesBackup.push_back({joint, visible});
        if (movingJoint == joint) {
            if (!visible) {
                joint->Visibility.setValue(true);
            }
        }
        else if (visible) {
            joint->Visibility.setValue(false);
        }
    }

    SbVec3f vec;
    if (dragMode == DragMode::RotationOnPlane) {
        vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
        initialPositionRot = Base::Vector3d(vec[0], vec[1], vec[2]);
    }
    else if (dragMode == DragMode::TranslationOnAxis) {
        Base::Vector3d zAxis = jcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., 1.));
        Base::Vector3d pos = jcsGlobalPlc.getPosition();
        SbVec3f axisCenter(pos.x, pos.y, pos.z);
        SbVec3f axis(zAxis.x, zAxis.y, zAxis.z);
        vec = viewer->getPointOnLine(cursorPos, axisCenter, axis);
        initialPosition = Base::Vector3d(vec[0], vec[1], vec[2]);
    }
    else if (dragMode == DragMode::TranslationOnAxisAndRotationOnePlane) {
        vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
        initialPositionRot = Base::Vector3d(vec[0], vec[1], vec[2]);

        Base::Vector3d zAxis = jcsGlobalPlc.getRotation().multVec(Base::Vector3d(0., 0., 1.));
        Base::Vector3d pos = jcsGlobalPlc.getPosition();
        SbVec3f axisCenter(pos.x, pos.y, pos.z);
        SbVec3f axis(zAxis.x, zAxis.y, zAxis.z);
        vec = viewer->getPointOnLine(cursorPos, axisCenter, axis);
        initialPosition = Base::Vector3d(vec[0], vec[1], vec[2]);
    }
    else if (dragMode == DragMode::TranslationOnPlane) {
        vec = viewer->getPointOnXYPlaneOfPlacement(cursorPos, jcsGlobalPlc);
        initialPosition = Base::Vector3d(vec[0], vec[1], vec[2]);
    }
    else {
        vec = viewer->getPointOnFocalPlane(cursorPos);
        initialPosition = Base::Vector3d(vec[0], vec[1], vec[2]);
        prevPosition = initialPosition;
    }

    if (moveInCommand) {
        Gui::Command::openCommand(tr("Move part").toStdString().c_str());
    }
    partMoving = true;

    // prevent selection while moving
    viewer->setSelectionEnabled(false);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly");
    bool solveOnMove = hGrp->GetBool("SolveOnMove", true);
    if (solveOnMove && dragMode != DragMode::TranslationNoSolve) {
        objectMasses.clear();
        for (auto& movingObj : docsToMove) {
            objectMasses.push_back({movingObj.obj, 10.0});
        }

        assemblyPart->setObjMasses(objectMasses);
        std::vector<App::DocumentObject*> dragParts;
        for (auto& movingObj : docsToMove) {
            dragParts.push_back(movingObj.obj);
        }
        assemblyPart->preDrag(dragParts);
    }
    else {
        assemblyPart->redrawJointPlacements(assemblyPart->getJoints());
    }
}

void ViewProviderAssembly::endMove()
{
    docsToMove.clear();
    partMoving = false;
    canStartDragging = false;

    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
    auto joints = assemblyPart->getJoints();
    for (auto pair : jointVisibilitiesBackup) {
        bool visible = pair.first->Visibility.getValue();
        if (visible != pair.second) {
            pair.first->Visibility.setValue(pair.second);
        }
    }

    movingJoint = nullptr;

    // enable selection after the move
    auto* view = dynamic_cast<Gui::View3DInventor*>(getDocument()->getActiveView());
    if (view) {
        view->getViewer()->setSelectionEnabled(true);
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly");
    bool solveOnMove = hGrp->GetBool("SolveOnMove", true);
    if (solveOnMove) {
        assemblyPart->postDrag();
        assemblyPart->setObjMasses({});
    }

    if (moveInCommand) {
        Gui::Command::commitCommand();
    }
}

void ViewProviderAssembly::initMoveDragger()
{
    setDraggerVisibility(true);

    // find the placement for the dragger.
    App::DocumentObject* part = docsToMove[0].obj;

    draggerInitPlc =
        App::GeoFeature::getGlobalPlacement(part, docsToMove[0].rootObj, docsToMove[0].sub);
    std::vector<App::DocumentObject*> listOfObjs;
    std::vector<App::PropertyXLinkSub*> listOfRefs;
    for (auto& movingObj : docsToMove) {
        listOfObjs.push_back(movingObj.obj);
        listOfRefs.push_back(movingObj.ref);
    }
    Base::Vector3d pos = getCenterOfBoundingBox(docsToMove);
    draggerInitPlc.setPosition(pos);

    setDraggerPlacement(draggerInitPlc);
    asmDragger->addMotionCallback(draggerMotionCallback, this);
}

void ViewProviderAssembly::endMoveDragger()
{
    if (getDraggerVisibility()) {
        asmDragger->removeMotionCallback(draggerMotionCallback, this);
        setDraggerVisibility(false);
    }
}

void ViewProviderAssembly::draggerMotionCallback(void* data, SoDragger* d)
{
    boost::ignore_unused(d);
    auto sudoThis = static_cast<ViewProviderAssembly*>(data);

    Base::Placement draggerPlc = sudoThis->getDraggerPlacement();
    Base::Placement movePlc = draggerPlc * sudoThis->draggerInitPlc.inverse();

    for (auto& movingObj : sudoThis->docsToMove) {
        App::DocumentObject* obj = movingObj.obj;

        auto* pPlc = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
        if (pPlc) {
            pPlc->setValue(movePlc * movingObj.plc);
        }
    }
}

void ViewProviderAssembly::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!isInEditMode()) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection
        || msg.Type == Gui::SelectionChanges::ClrSelection
        || msg.Type == Gui::SelectionChanges::RmvSelection) {
        canStartDragging = false;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // If selected object is a single solid show dragger and init dragger move

        if (enableMovement && getSelectedObjectsWithinAssembly(false, true)) {
            initMoveDragger();
        }
    }
    if (msg.Type == Gui::SelectionChanges::ClrSelection
        || msg.Type == Gui::SelectionChanges::RmvSelection) {
        if (enableMovement) {
            endMoveDragger();
        }
    }
}

bool ViewProviderAssembly::onDelete(const std::vector<std::string>& subNames)
{
    // Delete the assembly groups when assembly is deleted
    for (auto obj : getObject()->getOutList()) {
        if (obj->getTypeId() == Assembly::JointGroup::getClassTypeId()
            || obj->getTypeId() == Assembly::ViewGroup::getClassTypeId()
            || obj->getTypeId() == Assembly::BomGroup::getClassTypeId()) {

            // Delete the group content first.
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "doc = App.getDocument(\"%s\")\n"
                                    "objName = \"%s\"\n"
                                    "doc.getObject(objName).removeObjectsFromDocument()\n"
                                    "doc.removeObject(objName)\n",
                                    obj->getDocument()->getName(),
                                    obj->getNameInDocument());
        }
    }

    return ViewProviderPart::onDelete(subNames);
}

bool ViewProviderAssembly::canDelete(App::DocumentObject* objBeingDeleted) const
{
    bool res = ViewProviderPart::canDelete(objBeingDeleted);
    if (res) {
        // If a component is deleted, then we delete the joints as well.
        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());

        std::vector<App::DocumentObject*> objToDel;
        std::vector<App::DocumentObject*> objsBeingDeleted;
        objsBeingDeleted.push_back(objBeingDeleted);

        auto addSubComponents =
            std::function<void(AssemblyLink*, std::vector<App::DocumentObject*>&)> {};
        addSubComponents = [&](AssemblyLink* asmLink, std::vector<App::DocumentObject*>& objs) {
            std::vector<App::DocumentObject*> assemblyLinkGroup = asmLink->Group.getValues();
            for (auto* obj : assemblyLinkGroup) {
                auto* subAsmLink = dynamic_cast<AssemblyLink*>(obj);
                auto* link = dynamic_cast<App::Link*>(obj);
                if (subAsmLink || link) {
                    if (std::find(objs.begin(), objs.end(), obj) == objs.end()) {
                        objs.push_back(obj);
                        if (subAsmLink && !asmLink->isRigid()) {
                            addSubComponents(subAsmLink, objs);
                        }
                    }
                }
            }
        };

        auto* asmLink = dynamic_cast<Assembly::AssemblyLink*>(objBeingDeleted);
        if (asmLink && !asmLink->isRigid()) {
            addSubComponents(asmLink, objsBeingDeleted);
        }

        for (auto* obj : objsBeingDeleted) {
            // List its joints
            std::vector<App::DocumentObject*> joints = assemblyPart->getJointsOfObj(obj);
            for (auto* joint : joints) {
                objToDel.push_back(joint);
            }
            joints = assemblyPart->getJointsOfPart(obj);
            for (auto* joint : joints) {
                if (std::find(objToDel.begin(), objToDel.end(), joint) == objToDel.end()) {
                    objToDel.push_back(joint);
                }
            }

            // List its grounded joints
            std::vector<App::DocumentObject*> inList = obj->getInList();
            for (auto* parent : inList) {
                if (!parent) {
                    continue;
                }

                if (dynamic_cast<App::PropertyLink*>(parent->getPropertyByName("ObjectToGround"))) {
                    objToDel.push_back(parent);
                }
            }
        }

        // Deletes them.
        for (auto* joint : objToDel) {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.getDocument(\"%s\").removeObject(\"%s\")",
                                    joint->getDocument()->getName(),
                                    joint->getNameInDocument());
        }
    }

    return res;
}

void ViewProviderAssembly::setDraggerVisibility(bool val)
{
    asmDraggerSwitch->whichChild = val ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}
bool ViewProviderAssembly::getDraggerVisibility()
{
    if (!isInEditMode()) {
        return false;
    }

    return asmDraggerSwitch->whichChild.getValue() == SO_SWITCH_ALL;
}

void ViewProviderAssembly::setDraggerPlacement(Base::Placement plc)
{
    asmDragger->rotation.setValue(Base::convertTo<SbRotation>(plc.getRotation()));
    asmDragger->translation.setValue(Base::convertTo<SbVec3f>(plc.getPosition()));
}

Base::Placement ViewProviderAssembly::getDraggerPlacement()
{
    return {Base::convertTo<Base::Vector3d>(asmDragger->translation.getValue()),
            Base::convertTo<Base::Rotation>(asmDragger->rotation.getValue())};
}

Gui::SoFCCSysDragger* ViewProviderAssembly::getDragger()
{
    return asmDragger;
}

PyObject* ViewProviderAssembly::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderAssemblyPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}

// UTILS
Base::Vector3d
ViewProviderAssembly::getCenterOfBoundingBox(const std::vector<MovingObject>& movingObjs)
{
    int count = 0;
    Base::Vector3d center;  // feujhzef

    for (auto& movingObj : movingObjs) {
        Gui::ViewProvider* viewProvider =
            Gui::Application::Instance->getViewProvider(movingObj.obj);
        if (!viewProvider) {
            continue;
        }

        const Base::BoundBox3d& boundingBox = viewProvider->getBoundingBox();
        if (!boundingBox.IsValid()) {
            continue;
        }

        Base::Vector3d bboxCenter = boundingBox.GetCenter();

        // bboxCenter does not take into account obj global placement
        Base::Placement plc(bboxCenter, Base::Rotation());
        // Change plc to be relative to the object placement.
        Base::Placement objPlc = App::GeoFeature::getPlacementFromProp(movingObj.obj, "Placement");
        plc = objPlc.inverse() * plc;
        // Change plc to be relative to the origin of the document.
        Base::Placement global_plc =
            App::GeoFeature::getGlobalPlacement(movingObj.obj, movingObj.rootObj, movingObj.sub);
        plc = global_plc * plc;
        bboxCenter = plc.getPosition();

        center += bboxCenter;
        ++count;
    }

    if (count > 0) {
        center /= static_cast<double>(count);
    }

    return center;
}
