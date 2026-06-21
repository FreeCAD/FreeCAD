// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
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

#include <Inventor/SbLine.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/events/SoMouseButtonEvent.h>

#include <QObject>

#include <Base/Console.h>

#include <App/PropertyGeo.h>

#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderAnnotation.h>

#include <Mod/Collaboration/App/Topic.h>

#include "ViewProviderTopic.h"


using namespace CollaborationGui;

FC_LOG_LEVEL_INIT("ViewProviderTopic", true, true)

PROPERTY_SOURCE_WITH_EXTENSIONS(CollaborationGui::ViewProviderTopic, Gui::ViewProviderAnnotationLabel);

ViewProviderTopic::ViewProviderTopic()
{
    Gui::ViewProviderGroupExtension::initExtension(this);
}

ViewProviderTopic::~ViewProviderTopic() = default;

std::vector<std::string> ViewProviderTopic::getDisplayModes() const
{
    std::vector<std::string> modes = ViewProviderAnnotationLabel::getDisplayModes();
    modes.emplace_back(NoneDisplayMode);
    return modes;
}

void ViewProviderTopic::setDisplayMode(const char* mode)
{
    if (mode && NoneDisplayMode == mode) {
        setDisplayMaskMode(mode);
        return;
    }

    ViewProviderAnnotationLabel::setDisplayMode(mode);
}

void ViewProviderTopic::labelDoubleClicked()
{
    doubleClicked();
}

bool ViewProviderTopic::doubleClicked()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    Gui::Command::addModule(Gui::Command::Gui, "collaboration.gui.components.task_topic_dialog");
    Gui::Command::doCommand(
        Gui::Command::Gui,
        "Gui.Control.showDialog(collaboration.gui.components.task_topic_dialog.TaskTopicDialog(App."
        "ActiveDocument.getObject('%s')))",
        getObject()->getNameInDocument()
    );
    return true;
}

bool ViewProviderTopic::updateBasePosition(const App::Property* prop)
{
    App::DocumentObject* obj = getObject();
    if (!obj) {
        return false;
    }

    auto* topic = freecad_cast<Collaboration::Topic*>(obj);
    if (!topic) {
        return false;
    }

    Base::Vector3d position = static_cast<const App::PropertyVector*>(prop)->getValue();
    if (auto maybePos = topic->getGeometryPosition();
        maybePos && std::string(topic->PlacementType.getValueAsString()) == "Geometry") {
        Base::Vector3d geomPos = *maybePos;
        position = position + geomPos;
    }
    getBaseTranslation()->translation.setValue(
        static_cast<float>(position.x),
        static_cast<float>(position.y),
        static_cast<float>(position.z)
    );
    return true;
}

void ViewProviderTopic::updateData(const App::Property* prop)
{
    if (prop->is<App::PropertyVector>() && std::string_view(prop->getName()) == "BasePosition"
        && updateBasePosition(prop)) {
        return;
    }

    ViewProviderAnnotationLabel::updateData(prop);
}
