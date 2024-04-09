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
#include <QMessageBox>
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

#include <App/Link.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MDIView.h>
#include <Gui/SoFCCSysDragger.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewParams.h>

#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyUtils.h>
#include <Mod/Assembly/App/JointGroup.h>
#include <Mod/PartDesign/App/Body.h>

#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyPy.h"


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
    , jointVisibilityBackup(false)
    , ctrlPressed(false)
    , docsToMove({})
{}

ViewProviderAssembly::~ViewProviderAssembly() = default;

QIcon ViewProviderAssembly::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Geoassembly.svg");
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

    return true;
}

bool ViewProviderAssembly::canDragObject(App::DocumentObject* obj) const
{
    // The user should not be able to drag the joint group out of the assembly
    if (!obj || obj->getTypeId() == Assembly::JointGroup::getClassTypeId()) {
        return false;
    }

    // else if a solid is removed, remove associated joints if any.
    bool prompted = false;
    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
    std::vector<App::DocumentObject*> joints = assemblyPart->getJoints();

    // Combine the joints and groundedJoints vectors into one for simplicity.
    std::vector<App::DocumentObject*> allJoints = assemblyPart->getJoints();
    std::vector<App::DocumentObject*> groundedJoints = assemblyPart->getGroundedJoints();
    allJoints.insert(allJoints.end(), groundedJoints.begin(), groundedJoints.end());

    Gui::Command::openCommand(tr("Delete associated joints").toStdString().c_str());
    for (auto joint : allJoints) {
        // getLinkObjFromProp returns nullptr if the property doesn't exist.
        App::DocumentObject* obj1 = AssemblyObject::getObjFromNameProp(joint, "Object1", "Part1");
        App::DocumentObject* obj2 = AssemblyObject::getObjFromNameProp(joint, "Object2", "Part2");
        App::DocumentObject* part1 = AssemblyObject::getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* part2 = AssemblyObject::getLinkObjFromProp(joint, "Part2");
        App::DocumentObject* obj3 = AssemblyObject::getLinkObjFromProp(joint, "ObjectToGround");
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
    Gui::Command::commitCommand();

    return true;
}

bool ViewProviderAssembly::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);

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

void ViewProviderAssembly::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
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
}

void ViewProviderAssembly::setDragger()
{
    // Create the dragger coin object
    assert(!asmDragger);
    asmDragger = new Gui::SoFCCSysDragger();
    asmDragger->setAxisColors(Gui::ViewParams::instance()->getAxisXColor(),
                              Gui::ViewParams::instance()->getAxisYColor(),
                              Gui::ViewParams::instance()->getAxisZColor());
    asmDragger->draggerSize.setValue(0.05f);

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
    App::DocumentObject* activePart = getActivePart();
    if (!activePart) {
        return false;
    }

    return activePart == this->getObject();
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


        for (auto& pair : docsToMove) {
            App::DocumentObject* obj = pair.first;
            auto* propPlacement =
                dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
            if (propPlacement) {
                Base::Placement plc = pair.second;
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

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Assembly");
        bool solveOnMove = hGrp->GetBool("SolveOnMove", true);
        if (solveOnMove) {
            auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
            // assemblyPart->solve(/*enableRedo = */ false, /*updateJCS = */ false);
            assemblyPart->doDragStep();
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

    // Left Mouse button ****************************************************
    if (Button == 1) {
        if (pressed && !getDraggerVisibility()) {
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

bool ViewProviderAssembly::getSelectedObjectsWithinAssembly(bool addPreselection, bool onlySolids)
{
    // check the current selection, and check if any of the selected objects are within this
    // App::Part
    //  If any, put them into the vector docsToMove and return true.
    //  Get the document
    docsToMove.clear();

    // Get the assembly object for this ViewProvider
    AssemblyObject* assemblyPart = static_cast<AssemblyObject*>(getObject());

    if (!assemblyPart) {
        return false;
    }

    for (auto& selObj : Gui::Selection().getSelectionEx("",
                                                        App::DocumentObject::getClassTypeId(),
                                                        Gui::ResolveMode::NoResolve)) {
        // getSubNames() returns ["Body001.Pad.Face14", "Body002.Pad.Face7"]
        //  if you have several objects within the same assembly selected.

        std::vector<std::string> objsSubNames = selObj.getSubNames();
        for (auto& subNamesStr : objsSubNames) {
            std::vector<std::string> subNames = parseSubNames(subNamesStr);
            if (subNames.empty()) {
                continue;
            }
            if (onlySolids && subNames.back() != "") {
                continue;
            }

            App::DocumentObject* obj = getObjectFromSubNames(subNames);
            if (!obj) {
                continue;
            }

            // Check if the selected object is a child of the assembly
            if (assemblyPart->hasObject(obj, true)) {
                auto* propPlacement =
                    dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
                if (!propPlacement) {
                    continue;
                }
                // We have to exclude Grounded joints as they happen to have a Placement prop
                auto* propLink =
                    dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("ObjectToGround"));
                if (propLink) {
                    continue;
                }

                docsToMove.emplace_back(obj, propPlacement->getValue());
            }
        }
    }

    // This function is called before the selection is updated. So if a user click and drag a part
    // it is not selected at that point. So we need to get the preselection too.
    if (addPreselection && Gui::Selection().hasPreselection()) {

        // Base::Console().Warning("Gui::Selection().getPreselection().pSubName %s\n",
        //                         Gui::Selection().getPreselection().pSubName);

        std::string subNamesStr = Gui::Selection().getPreselection().pSubName;
        std::vector<std::string> subNames = parseSubNames(subNamesStr);

        App::DocumentObject* preselectedObj = getObjectFromSubNames(subNames);
        if (preselectedObj && assemblyPart->hasObject(preselectedObj, true)) {
            bool alreadyIn = false;
            for (auto& pair : docsToMove) {
                App::DocumentObject* obj = pair.first;
                if (obj == preselectedObj) {
                    alreadyIn = true;
                    break;
                }
            }

            if (!alreadyIn) {
                auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(
                    preselectedObj->getPropertyByName("Placement"));
                if (propPlacement) {
                    // We have to exclude Grounded joints as they happen to have a Placement prop
                    auto* propLink = dynamic_cast<App::PropertyLink*>(
                        preselectedObj->getPropertyByName("ObjectToGround"));
                    if (!propLink) {
                        if (!ctrlPressed) {
                            Gui::Selection().clearSelection();
                            docsToMove.clear();
                        }
                        docsToMove.emplace_back(preselectedObj, propPlacement->getValue());
                    }
                }
            }
        }
    }

    return !docsToMove.empty();
}

std::vector<std::string> ViewProviderAssembly::parseSubNames(std::string& subNamesStr)
{
    std::vector<std::string> subNames;
    std::string subName;
    std::istringstream subNameStream(subNamesStr);
    while (std::getline(subNameStream, subName, '.')) {
        subNames.push_back(subName);
    }

    // Check if the last character of the input string is the delimiter.
    // If so, add an empty string to the subNames vector.
    // Because the last subname is the element name and can be empty.
    if (!subNamesStr.empty() && subNamesStr.back() == '.') {
        subNames.push_back("");  // Append empty string for trailing dot.
    }

    return subNames;
}

App::DocumentObject* ViewProviderAssembly::getObjectFromSubNames(std::vector<std::string>& subNames)
{
    App::Document* appDoc = getObject()->getDocument();

    std::string objName;
    if (subNames.size() < 2) {
        return nullptr;
    }
    else if (subNames.size() == 2) {
        // If two subnames then it can't be a body and the object we want is the first one
        // For example we want box in "box.face1"
        // "assembly.part.box.face1"
        // "p.fcstd.assembly.LinkToPart.box.face1"
        // "p2.fcstd.Part.box."
        return appDoc->getObject(subNames[0].c_str());
    }

    // From here subnames is at least 3 and can be more. There are several cases to consider :
    //  bodyOrLink.pad.face1  -> bodyOrLink should be the moving  entity
    // partOrLink.bodyOrLink.pad.face1  -> partOrLink should be the moving entity
    // partOrLink.box.face1  -> partOrLink should be the moving entity
    // partOrLink1...ParOrLinkn.bodyOrLink.pad.face1    -> partOrLink1 should be the moving entity
    // assembly1.partOrLink1...ParOrLinkn.bodyOrLink.pad.face1 -> partOrLink1 should be the moving
    // entity assembly1.boxOrLink1.face1 -> boxOrLink1 should be the moving entity

    for (auto objName : subNames) {
        App::DocumentObject* obj = appDoc->getObject(objName.c_str());
        if (!obj) {
            continue;
        }

        if (obj->getTypeId().isDerivedFrom(AssemblyObject::getClassTypeId())) {
            continue;
        }
        else if (obj->getTypeId().isDerivedFrom(App::Part::getClassTypeId())
                 || obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            return obj;
        }
        else if (obj->getTypeId().isDerivedFrom(App::Link::getClassTypeId())) {
            App::Link* link = dynamic_cast<App::Link*>(obj);

            App::DocumentObject* linkedObj = link->getLinkedObject(true);
            if (!linkedObj) {
                continue;
            }

            if (linkedObj->getTypeId().isDerivedFrom(App::Part::getClassTypeId())
                || linkedObj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                return obj;
            }
        }
    }

    return nullptr;
}

ViewProviderAssembly::DragMode ViewProviderAssembly::findDragMode()
{
    if (docsToMove.size() == 1) {
        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        std::string partPropName;
        movingJoint =
            assemblyPart->getJointOfPartConnectingToGround(docsToMove[0].first, partPropName);

        if (!movingJoint) {
            return DragMode::Translation;
        }

        JointType jointType = AssemblyObject::getJointType(movingJoint);
        if (jointType == JointType::Fixed) {
            // If fixed joint we need to find the upstream joint to find move mode.
            // For example : Gnd -(revolute)- A -(fixed)- B : if user try to move B, then we should
            // actually move A
            App::DocumentObject* upstreamPart =
                assemblyPart->getUpstreamMovingPart(docsToMove[0].first);
            docsToMove.clear();
            if (!upstreamPart) {
                return DragMode::None;
            }

            auto* propPlacement =
                dynamic_cast<App::PropertyPlacement*>(upstreamPart->getPropertyByName("Placement"));
            if (propPlacement) {
                docsToMove.emplace_back(upstreamPart, propPlacement->getValue());
            }

            movingJoint =
                assemblyPart->getJointOfPartConnectingToGround(docsToMove[0].first, partPropName);
            if (!movingJoint) {
                return DragMode::Translation;
            }
            jointType = AssemblyObject::getJointType(movingJoint);
        }

        const char* plcPropName = (partPropName == "Part1") ? "Placement1" : "Placement2";
        const char* objPropName = (partPropName == "Part1") ? "Object1" : "Object2";

        // jcsPlc is relative to the Object
        jcsPlc = AssemblyObject::getPlacementFromProp(movingJoint, plcPropName);

        // Make jcsGlobalPlc relative to the origin of the doc
        Base::Placement global_plc =
            AssemblyObject::getGlobalPlacement(movingJoint, objPropName, partPropName.c_str());
        jcsGlobalPlc = global_plc * jcsPlc;

        // Add downstream parts so that they move together
        auto downstreamParts = assemblyPart->getDownstreamParts(docsToMove[0].first, movingJoint);
        for (auto part : downstreamParts) {
            auto* propPlacement =
                dynamic_cast<App::PropertyPlacement*>(part->getPropertyByName("Placement"));
            if (propPlacement) {
                docsToMove.emplace_back(part, propPlacement->getValue());
            }
        }

        jointVisibilityBackup = movingJoint->Visibility.getValue();
        if (!jointVisibilityBackup) {
            movingJoint->Visibility.setValue(true);
        }

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
    dragMode = findDragMode();
    if (dragMode == DragMode::None) {
        return;
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


    Gui::Command::openCommand(tr("Move part").toStdString().c_str());
    partMoving = true;

    // prevent selection while moving
    viewer->setSelectionEnabled(false);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly");
    bool solveOnMove = hGrp->GetBool("SolveOnMove", true);
    if (solveOnMove) {
        objectMasses.clear();
        for (auto& pair : docsToMove) {
            objectMasses.push_back({pair.first, 10.0});
        }

        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        assemblyPart->setObjMasses(objectMasses);
        std::vector<App::DocumentObject*> dragParts;
        for (auto& pair : docsToMove) {
            dragParts.push_back(pair.first);
        }
        assemblyPart->preDrag(dragParts);
    }
}

void ViewProviderAssembly::endMove()
{
    docsToMove.clear();
    partMoving = false;
    canStartDragging = false;

    if (movingJoint && !jointVisibilityBackup) {
        movingJoint->Visibility.setValue(false);
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
        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        assemblyPart->postDrag();
        assemblyPart->setObjMasses({});
    }

    Gui::Command::commitCommand();
}

void ViewProviderAssembly::initMoveDragger()
{
    setDraggerVisibility(true);

    // find the placement for the dragger.
    App::DocumentObject* obj = docsToMove[0].first;
    AssemblyObject* assemblyPart = static_cast<AssemblyObject*>(getObject());
    draggerInitPlc = AssemblyObject::getGlobalPlacement(obj, obj);
    std::vector<App::DocumentObject*> listOfObjs;
    for (auto& pair : docsToMove) {
        listOfObjs.push_back(pair.first);
    }
    Base::Vector3d pos = getCenterOfBoundingBox(listOfObjs, listOfObjs);
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
    auto sudoThis = static_cast<ViewProviderAssembly*>(data);

    Base::Placement draggerPlc = sudoThis->getDraggerPlacement();
    Base::Placement movePlc = draggerPlc * sudoThis->draggerInitPlc.inverse();

    for (auto& pair : sudoThis->docsToMove) {
        App::DocumentObject* obj = pair.first;

        auto* propPlc = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
        if (propPlc) {
            propPlc->setValue(movePlc * pair.second);
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
    // Delete the joingroup when assembly is deleted
    for (auto obj : getObject()->getOutList()) {
        if (obj->getTypeId() == Assembly::JointGroup::getClassTypeId()) {
            obj->getDocument()->removeObject(obj->getNameInDocument());
        }
    }

    return ViewProviderPart::onDelete(subNames);
}

void ViewProviderAssembly::setDraggerVisibility(bool val)
{
    asmDraggerSwitch->whichChild = val ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}
bool ViewProviderAssembly::getDraggerVisibility()
{
    return asmDraggerSwitch->whichChild.getValue() == SO_SWITCH_ALL;
}

void ViewProviderAssembly::setDraggerPlacement(Base::Placement plc)
{
    double q0, q1, q2, q3;
    plc.getRotation().getValue(q0, q1, q2, q3);
    Base::Vector3d pos = plc.getPosition();
    asmDragger->rotation.setValue(q0, q1, q2, q3);
    asmDragger->translation.setValue(pos.x, pos.y, pos.z);
}

Base::Placement ViewProviderAssembly::getDraggerPlacement()
{
    Base::Placement plc;
    SbVec3f pos = asmDragger->translation.getValue();
    plc.setPosition(Base::Vector3d(pos[0], pos[1], pos[2]));

    SbVec3f axis;
    float angle;
    asmDragger->rotation.getValue(axis, angle);
    Base::Vector3d axisV = Base::Vector3d(axis[0], axis[1], axis[2]);
    Base::Rotation rot(axisV, angle);
    plc.setRotation(rot);

    return plc;
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
ViewProviderAssembly::getCenterOfBoundingBox(const std::vector<App::DocumentObject*>& objs,
                                             const std::vector<App::DocumentObject*>& parts)
{
    int count = 0;
    Base::Vector3d center;

    for (size_t i = 0; i < objs.size(); ++i) {
        Gui::ViewProvider* viewProvider = Gui::Application::Instance->getViewProvider(objs[i]);
        if (!viewProvider) {
            continue;
        }

        const Base::BoundBox3d& boundingBox = viewProvider->getBoundingBox();
        if (!boundingBox.IsValid()) {
            continue;
        }

        Base::Vector3d bboxCenter = boundingBox.GetCenter();

        if (parts[i] != objs[i]) {
            // bboxCenter does not take into account obj global placement
            Base::Placement plc(bboxCenter, Base::Rotation());
            // Change plc to be relative to the object placement.
            Base::Placement objPlc = AssemblyObject::getPlacementFromProp(objs[i], "Placement");
            plc = objPlc.inverse() * plc;
            // Change plc to be relative to the origin of the document.
            Base::Placement global_plc = AssemblyObject::getGlobalPlacement(objs[i], parts[i]);
            plc = global_plc * plc;
            bboxCenter = plc.getPosition();
        }

        center += bboxCenter;
        ++count;
    }

    if (count > 0) {
        center /= static_cast<double>(count);
    }

    return center;
}
