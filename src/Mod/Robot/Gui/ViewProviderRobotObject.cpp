/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#ifndef _PreComp_
#include <QFile>
#include <sstream>

#include <Inventor/SbVec3f.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/VRMLnodes/SoVRMLTransform.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/draggers/SoJackDragger.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <App/VRMLObject.h>
#include <Gui/Application.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Robot/App/RobotObject.h>

#include "ViewProviderRobotObject.h"


using namespace Gui;
using namespace RobotGui;

PROPERTY_SOURCE(RobotGui::ViewProviderRobotObject, Gui::ViewProviderGeometryObject)

ViewProviderRobotObject::ViewProviderRobotObject()
{
    ADD_PROPERTY(Manipulator, (0));

    pcRobotRoot = new Gui::SoFCSelection();
    pcRobotRoot->highlightMode = Gui::SoFCSelection::OFF;
    // pcRobotRoot->selectionMode = Gui::SoFCSelection::SEL_OFF;
    // pcRobotRoot->style = Gui::SoFCSelection::BOX;
    pcRobotRoot->ref();

    pcSimpleRoot = new Gui::SoFCSelection();
    pcSimpleRoot->highlightMode = Gui::SoFCSelection::OFF;
    // pcSimpleRoot->selectionMode = Gui::SoFCSelection::SEL_OFF;
    pcSimpleRoot->ref();

    pcOffRoot = new SoGroup();
    pcOffRoot->ref();

    // set nodes for the manipulator outfit
    pcTcpRoot = new SoGroup();
    pcTcpRoot->ref();


    Axis1Node = Axis2Node = Axis3Node = Axis4Node = Axis5Node = Axis6Node = nullptr;
}

ViewProviderRobotObject::~ViewProviderRobotObject()
{
    pcRobotRoot->unref();
    pcSimpleRoot->unref();
    pcOffRoot->unref();
    pcTcpRoot->unref();
}

void ViewProviderRobotObject::setDragger()
{
    assert(!pcDragger);
    pcDragger = new SoJackDragger();
    pcDragger->addMotionCallback(sDraggerMotionCallback, this);
    pcTcpRoot->addChild(pcDragger);

    // set the actual TCP position
    Robot::RobotObject* robObj = static_cast<Robot::RobotObject*>(pcObject);
    Base::Placement loc = robObj->Tcp.getValue();
    SbMatrix M;
    M.setTransform(SbVec3f(loc.getPosition().x, loc.getPosition().y, loc.getPosition().z),
                   SbRotation(loc.getRotation()[0],
                              loc.getRotation()[1],
                              loc.getRotation()[2],
                              loc.getRotation()[3]),
                   SbVec3f(150, 150, 150));
    pcDragger->setMotionMatrix(M);
}

void ViewProviderRobotObject::resetDragger()
{
    assert(pcDragger);
    Gui::coinRemoveAllChildren(pcTcpRoot);
    pcDragger = nullptr;
}

void ViewProviderRobotObject::attach(App::DocumentObject* pcObj)
{
    ViewProviderGeometryObject::attach(pcObj);

    addDisplayMaskMode(pcRobotRoot, "VRML");
    pcRobotRoot->objectName = pcObj->getNameInDocument();
    pcRobotRoot->documentName = pcObj->getDocument()->getName();
    pcRobotRoot->subElementName = "Main";
    pcRobotRoot->addChild(pcTcpRoot);

    addDisplayMaskMode(pcSimpleRoot, "Simple");
    pcSimpleRoot->objectName = pcObj->getNameInDocument();
    pcSimpleRoot->documentName = pcObj->getDocument()->getName();
    pcSimpleRoot->subElementName = "Main";
    pcSimpleRoot->addChild(pcTcpRoot);

    addDisplayMaskMode(pcOffRoot, "Off");
    pcOffRoot->addChild(pcTcpRoot);
}

void ViewProviderRobotObject::setDisplayMode(const char* ModeName)
{
    if (strcmp("VRML", ModeName) == 0) {
        setDisplayMaskMode("VRML");
    }
    if (strcmp("Simple", ModeName) == 0) {
        setDisplayMaskMode("Simple");
    }
    if (strcmp("Off", ModeName) == 0) {
        setDisplayMaskMode("Off");
    }
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderRobotObject::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("VRML");
    StrList.emplace_back("Simple");
    StrList.emplace_back("Off");
    return StrList;
}

void ViewProviderRobotObject::onChanged(const App::Property* prop)
{
    if (prop == &Manipulator) {
        if (Manipulator.getValue()) {
            if (!this->pcDragger) {
                this->setDragger();
            }
        }
        else {
            if (this->pcDragger) {
                this->resetDragger();
            }
        }
    }
    else {
        ViewProviderGeometryObject::onChanged(prop);
    }
}

void ViewProviderRobotObject::updateData(const App::Property* prop)
{
    Robot::RobotObject* robObj = static_cast<Robot::RobotObject*>(pcObject);
    if (prop == &robObj->RobotVrmlFile) {
        // read also from file
        const char* filename = robObj->RobotVrmlFile.getValue();
        QString fn = QString::fromUtf8(filename);
        QFile file(fn);
        SoInput in;
        Gui::coinRemoveAllChildren(pcRobotRoot);
        if (!fn.isEmpty() && file.open(QFile::ReadOnly)) {
            QByteArray buffer = file.readAll();
            in.setBuffer((void*)buffer.constData(), buffer.length());
            SoSeparator* node = SoDB::readAll(&in);
            if (node) {
                pcRobotRoot->addChild(node);
            }
            pcRobotRoot->addChild(pcTcpRoot);
        }
        // search for the connection points +++++++++++++++++++++++++++++++++++++++++++++++++
        Axis1Node = Axis2Node = Axis3Node = Axis4Node = Axis5Node = Axis6Node = nullptr;
        SoSearchAction searchAction;
        SoPath* path;

        // Axis 1
        searchAction.setName("FREECAD_AXIS1");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis1Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        // Axis 2
        searchAction.setName("FREECAD_AXIS2");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis2Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        // Axis 3
        searchAction.setName("FREECAD_AXIS3");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis3Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        // Axis 4
        searchAction.setName("FREECAD_AXIS4");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis4Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        // Axis 5
        searchAction.setName("FREECAD_AXIS5");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis5Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        // Axis 6
        searchAction.setName("FREECAD_AXIS6");
        searchAction.setInterest(SoSearchAction::FIRST);
        searchAction.setSearchingAll(false);
        searchAction.apply(pcRobotRoot);
        path = searchAction.getPath();
        if (path) {
            SoNode* node = path->getTail();
            if (node && node->getTypeId() == SoVRMLTransform::getClassTypeId()) {
                Axis6Node = static_cast<SoVRMLTransform*>(node);
            }
        }
        if (Axis1Node) {
            Axis1Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis1.getValue() * (M_PI / 180));
        }
        if (Axis2Node) {
            Axis2Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis2.getValue() * (M_PI / 180));
        }
        if (Axis3Node) {
            Axis3Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis3.getValue() * (M_PI / 180));
        }
        if (Axis4Node) {
            Axis4Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis4.getValue() * (M_PI / 180));
        }
        if (Axis5Node) {
            Axis5Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis5.getValue() * (M_PI / 180));
        }
        if (Axis6Node) {
            Axis6Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis6.getValue() * (M_PI / 180));
        }
    }
    else if (prop == &robObj->Axis1) {
        if (Axis1Node) {
            Axis1Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis1.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Axis2) {
        if (Axis2Node) {
            Axis2Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis2.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Axis3) {
        if (Axis3Node) {
            Axis3Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis3.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Axis4) {
        if (Axis4Node) {
            Axis4Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis4.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Axis5) {
        if (Axis5Node) {
            Axis5Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis5.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Axis6) {
        if (Axis6Node) {
            Axis6Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0),
                                         robObj->Axis6.getValue() * (M_PI / 180));
            if (toolShape) {
                toolShape->setTransformation(
                    (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
            }
        }
    }
    else if (prop == &robObj->Tcp) {
        Base::Placement loc = robObj->Tcp.getValue();
        SbMatrix M;
        M.setTransform(SbVec3f(loc.getPosition().x, loc.getPosition().y, loc.getPosition().z),
                       SbRotation(loc.getRotation()[0],
                                  loc.getRotation()[1],
                                  loc.getRotation()[2],
                                  loc.getRotation()[3]),
                       SbVec3f(150, 150, 150));
        if (pcDragger) {
            pcDragger->setMotionMatrix(M);
        }
        if (toolShape) {
            toolShape->setTransformation(
                (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
        }
        // pcTcpTransform->translation =
        // SbVec3f(loc.getPosition().x,loc.getPosition().y,loc.getPosition().z);
        // pcTcpTransform->rotation =
        // SbRotation(loc.getRotation()[0],loc.getRotation()[1],loc.getRotation()[2],loc.getRotation()[3]);
    }
    else if (prop == &robObj->ToolShape) {
        App::DocumentObject* o = robObj->ToolShape.getValue<App::DocumentObject*>();

        if (o
            && (o->isDerivedFrom(Part::Feature::getClassTypeId())
                || o->isDerivedFrom(App::VRMLObject::getClassTypeId()))) {
            // Part::Feature *p = dynamic_cast<Part::Feature *>(o);
            toolShape = Gui::Application::Instance->getViewProvider(o);
            toolShape->setTransformation(
                (robObj->Tcp.getValue() * (robObj->ToolBase.getValue().inverse())).toMatrix());
        }
        else {
            toolShape = nullptr;
        }
    }
}
void ViewProviderRobotObject::setAxisTo(float A1,
                                        float A2,
                                        float A3,
                                        float A4,
                                        float A5,
                                        float A6,
                                        const Base::Placement& Tcp)
{
    Robot::RobotObject* robObj = static_cast<Robot::RobotObject*>(pcObject);

    if (Axis1Node) {
        // FIXME Ugly hack for the wrong transformation of the Kuka 500 robot VRML the minus sign on
        // Axis 1
        Axis1Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A1 * (M_PI / 180));
    }
    if (Axis2Node) {
        Axis2Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A2 * (M_PI / 180));
    }
    if (Axis3Node) {
        Axis3Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A3 * (M_PI / 180));
    }
    if (Axis4Node) {
        Axis4Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A4 * (M_PI / 180));
    }
    if (Axis5Node) {
        Axis5Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A5 * (M_PI / 180));
    }
    if (Axis6Node) {
        Axis6Node->rotation.setValue(SbVec3f(0.0, 1.0, 0.0), A6 * (M_PI / 180));
    }
    // update tool position
    if (toolShape) {
        toolShape->setTransformation((Tcp * (robObj->ToolBase.getValue().inverse())).toMatrix());
    }
}

void ViewProviderRobotObject::sDraggerMotionCallback(void* data, SoDragger* dragger)
{
    static_cast<ViewProviderRobotObject*>(data)->DraggerMotionCallback(dragger);
}

void ViewProviderRobotObject::DraggerMotionCallback(SoDragger* dragger)
{
    float q0, q1, q2, q3;

    Robot::RobotObject* robObj = static_cast<Robot::RobotObject*>(pcObject);
    Base::Placement Tcp = robObj->Tcp.getValue();
    const SbMatrix& M = dragger->getMotionMatrix();
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scaleFactor;
    SbRotation scaleOrientation;
    SbVec3f center(Tcp.getPosition().x, Tcp.getPosition().y, Tcp.getPosition().z);
    M.getTransform(translation, rotation, scaleFactor, scaleOrientation);
    rotation.getValue(q0, q1, q2, q3);
    // Base::Console().Message("M %f %f %f\n", M[3][0], M[3][1], M[3][2]);
    Base::Rotation rot(q0, q1, q2, q3);
    Base::Vector3d pos(translation[0], translation[1], translation[2]);
    robObj->Tcp.setValue(Base::Placement(pos, rot));
}
