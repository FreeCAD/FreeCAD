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
#endif

#include <App/Link.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/JointGroup.h>
#include <Mod/PartDesign/App/Body.h>

#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyPy.h"


using namespace Assembly;
using namespace AssemblyGui;

PROPERTY_SOURCE(AssemblyGui::ViewProviderAssembly, Gui::ViewProviderPart)

ViewProviderAssembly::ViewProviderAssembly()
    : SelectionObserver(true)
    , canStartDragging(false)
    , partMoving(false)
    , enableMovement(true)
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
        Gui::Application::Instance->activeDocument()->resetEdit();
    }
    else {
        // Part is not 'Active' so we enter edit mode to make it so.
        Gui::Application::Instance->activeDocument()->setEdit(this);
    }

    return true;
}

bool ViewProviderAssembly::canDragObject(App::DocumentObject* obj) const
{
    Base::Console().Warning("ViewProviderAssembly::canDragObject\n");
    if (!obj || obj->getTypeId() == Assembly::JointGroup::getClassTypeId()) {
        Base::Console().Warning("so should be false...\n");
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
        // Assume getLinkObjFromProp can return nullptr if the property doesn't exist.
        App::DocumentObject* obj1 = assemblyPart->getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* obj2 = assemblyPart->getLinkObjFromProp(joint, "Part2");
        App::DocumentObject* obj3 = assemblyPart->getLinkObjFromProp(joint, "ObjectToGround");
        if (obj == obj1 || obj == obj2 || obj == obj3) {
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

    // Remove grounded tag if any. (as it is not done in jointObject.py onDelete)
    std::string label = obj->Label.getValue();

    if (label.size() >= 4 && label.substr(label.size() - 2) == " 🔒") {
        label = label.substr(0, label.size() - 2);
        obj->Label.setValue(label.c_str());
    }

    return true;
}

bool ViewProviderAssembly::setEdit(int ModNum)
{
    // Set the part as 'Activated' ie bold in the tree.
    Gui::Command::doCommand(Gui::Command::Gui,
                            "Gui.ActiveDocument.ActiveView.setActiveObject('%s', "
                            "App.getDocument('%s').getObject('%s'))",
                            PARTKEY,
                            this->getObject()->getDocument()->getName(),
                            this->getObject()->getNameInDocument());

    return true;
}

void ViewProviderAssembly::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    canStartDragging = false;
    partMoving = false;
    docsToMove = {};

    // Check if the view is still active before trying to deactivate the assembly.
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if (!activeDoc) {
        return;
    }
    auto activeView = activeDoc->getActiveView();
    if (!activeView) {
        return;
    }

    // Set the part as not 'Activated' ie not bold in the tree.
    Gui::Command::doCommand(Gui::Command::Gui,
                            "Gui.ActiveDocument.ActiveView.setActiveObject('%s', None)",
                            PARTKEY);
}

bool ViewProviderAssembly::isInEditMode()
{
    App::DocumentObject* activePart = getActivePart();
    if (!activePart) {
        return false;
    }

    return activePart == this->getObject();
}

App::DocumentObject* ViewProviderAssembly::getActivePart()
{
    App::DocumentObject* activePart = nullptr;
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if (!activeDoc) {
        activeDoc = getDocument();
    }
    auto activeView = activeDoc->setActiveView(this);
    if (!activeView) {
        return nullptr;
    }

    activePart = activeView->getActiveObject<App::DocumentObject*>(PARTKEY);
    return activePart;
}

bool ViewProviderAssembly::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    // Initialize or end the dragging of parts
    if (canStartDragging) {
        canStartDragging = false;

        if (enableMovement && getSelectedObjectsWithinAssembly()) {
            SbVec3f vec = viewer->getPointOnFocalPlane(cursorPos);
            Base::Vector3d mousePosition = Base::Vector3d(vec[0], vec[1], vec[2]);

            initMove(mousePosition);
        }
    }

    // Do the dragging of parts
    if (partMoving) {
        SbVec3f vec = viewer->getPointOnFocalPlane(cursorPos);
        Base::Vector3d mousePosition = Base::Vector3d(vec[0], vec[1], vec[2]);
        for (auto& pair : docsToMove) {
            App::DocumentObject* obj = pair.first;
            auto* propPlacement =
                dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
            if (propPlacement) {
                Base::Placement plc = propPlacement->getValue();
                // Base::Console().Warning("transl %f %f %f\n", pair.second.x, pair.second.y,
                // pair.second.z);
                Base::Vector3d pos = mousePosition + pair.second;
                Base::Placement newPlacement = Base::Placement(pos, plc.getRotation());
                propPlacement->setValue(newPlacement);
            }
        }

        auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
        assemblyPart->solve();
    }
    return false;
}

bool ViewProviderAssembly::mouseButtonPressed(int Button,
                                              bool pressed,
                                              const SbVec2s& cursorPos,
                                              const Gui::View3DInventorViewer* viewer)
{
    // Left Mouse button ****************************************************
    if (Button == 1) {
        if (pressed) {
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

bool ViewProviderAssembly::getSelectedObjectsWithinAssembly()
{
    // check the current selection, and check if any of the selected objects are within this
    // App::Part
    //  If any, put them into the vector docsToMove and return true.
    //  Get the document
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    if (!doc) {
        return false;
    }

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

            App::DocumentObject* obj = getObjectFromSubNames(subNames);
            if (!obj) {
                continue;
            }

            // Check if the selected object is a child of the assembly
            if (assemblyPart->hasObject(obj, true)) {
                auto* propPlacement =
                    dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
                if (propPlacement) {
                    Base::Placement plc = propPlacement->getValue();
                    Base::Vector3d pos = plc.getPosition();
                    docsToMove.emplace_back(obj, pos);
                }
            }
        }
    }

    // This function is called before the selection is updated. So if a user click and drag a part
    // it is not selected at that point. So we need to get the preselection too.
    if (Gui::Selection().hasPreselection()) {

        // Base::Console().Warning("Gui::Selection().getPreselection().pSubName %s\n",
        //                         Gui::Selection().getPreselection().pSubName);

        std::string subNamesStr = Gui::Selection().getPreselection().pSubName;
        std::vector<std::string> subNames = parseSubNames(subNamesStr);

        App::DocumentObject* preselectedObj = getObjectFromSubNames(subNames);
        if (preselectedObj) {
            if (assemblyPart->hasObject(preselectedObj, true)) {
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
                        Base::Placement plc = propPlacement->getValue();
                        Base::Vector3d pos = plc.getPosition();
                        docsToMove.emplace_back(preselectedObj, pos);
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
    return subNames;
}

App::DocumentObject* ViewProviderAssembly::getObjectFromSubNames(std::vector<std::string>& subNames)
{
    App::Document* appDoc = App::GetApplication().getActiveDocument();

    std::string objName;
    if (subNames.size() < 2) {
        return nullptr;
    }
    else if (subNames.size() == 2) {
        // If two subnames then it can't be a body and the object we want is the first one
        // For example we want box in "box.face1"
        return appDoc->getObject(subNames[0].c_str());
    }

    // From here subnames is at least 3 and can be more. There are several cases to consider :
    //  bodyOrLink.pad.face1                                    -> bodyOrLink should be the moving
    //  entity partOrLink.bodyOrLink.pad.face1                         -> partOrLink should be the
    //  moving entity partOrLink.box.face1                                    -> partOrLink should
    //  be the moving entity partOrLink1...ParOrLinkn.bodyOrLink.pad.face1           -> partOrLink1
    //  should be the moving entity assembly1.partOrLink1...ParOrLinkn.bodyOrLink.pad.face1 ->
    //  partOrLink1 should be the moving entity assembly1.boxOrLink1.face1 -> boxOrLink1 should be
    //  the moving entity

    for (auto objName : subNames) {
        App::DocumentObject* obj = appDoc->getObject(objName.c_str());
        if (!obj) {
            continue;
        }

        if (obj->getTypeId().isDerivedFrom(AssemblyObject::getClassTypeId())) {
            continue;
        }
        else if (obj->getTypeId().isDerivedFrom(App::Part::getClassTypeId())
                 || obj->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId())) {
            return obj;
        }
        else if (obj->getTypeId().isDerivedFrom(App::Link::getClassTypeId())) {
            App::Link* link = dynamic_cast<App::Link*>(obj);

            App::DocumentObject* linkedObj = link->getLinkedObject(true);
            if (!linkedObj) {
                continue;
            }

            if (linkedObj->getTypeId().isDerivedFrom(App::Part::getClassTypeId())
                || linkedObj->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId())) {
                return obj;
            }
        }
    }

    // then its neither a part or body or a link to a part or body. So it is something like
    // assembly.box.face1
    objName = subNames[subNames.size() - 2];
    return appDoc->getObject(objName.c_str());
}

void ViewProviderAssembly::initMove(Base::Vector3d& mousePosition)
{
    Gui::Command::openCommand(tr("Move part").toStdString().c_str());
    partMoving = true;

    // prevent selection while moving
    auto* view = dynamic_cast<Gui::View3DInventor*>(
        Gui::Application::Instance->editDocument()->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewerNotConst;
        viewerNotConst = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewerNotConst->setSelectionEnabled(false);
    }

    objectMasses.clear();

    for (auto& pair : docsToMove) {
        pair.second = pair.second - mousePosition;
        objectMasses.push_back({pair.first, 10.0});
    }

    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
    assemblyPart->setObjMasses(objectMasses);
}

void ViewProviderAssembly::endMove()
{
    docsToMove = {};
    partMoving = false;
    canStartDragging = false;

    // enable selection after the move
    auto* view = dynamic_cast<Gui::View3DInventor*>(
        Gui::Application::Instance->editDocument()->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewerNotConst;
        viewerNotConst = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewerNotConst->setSelectionEnabled(true);
    }

    auto* assemblyPart = static_cast<AssemblyObject*>(getObject());
    assemblyPart->setObjMasses({});

    Gui::Command::commitCommand();
}


void ViewProviderAssembly::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection
        || msg.Type == Gui::SelectionChanges::ClrSelection
        || msg.Type == Gui::SelectionChanges::RmvSelection) {
        canStartDragging = false;
    }
}

PyObject* ViewProviderAssembly::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderAssemblyPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}
