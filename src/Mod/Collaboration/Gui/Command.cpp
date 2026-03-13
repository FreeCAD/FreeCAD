// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Ondsel Pieter Hijma <info@pieterhijma.net>          *
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

#include <Base/Console.h>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>

#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>

#include <Mod/Collaboration/App/Topic.h>
#include <Mod/Collaboration/Gui/ViewProviderTopic.h>

using Topic = Collaboration::Topic;
using ViewProviderTopic = CollaborationGui::ViewProviderTopic;

FC_LOG_LEVEL_INIT("CollaborationCommand", true, true)

//===========================================================================
// Std_Topic
//===========================================================================
DEF_STD_CMD_A(CmdCollaborationTopic)

CmdCollaborationTopic::CmdCollaborationTopic()
    : Command("Collaboration_Topic")
{
    sGroup = QT_TR_NOOP("Tools");
    sMenuText = QT_TR_NOOP("Topic");
    sToolTipText = QT_TR_NOOP("Create a topic");
    sStatusTip = sToolTipText;
    sWhatsThis = "Collaboration_Topic";
    sPixmap = "Tree_Annotation";
    eType = AlterDoc;
}

static App::DocumentObjectGroup* getTopicGroup(App::Document* doc)
{
    auto* obj = doc->getObject("Topics");
    if (!obj) {
        obj = doc->addObject("App::DocumentObjectGroup", "Topics");
    }
    return freecad_cast<App::DocumentObjectGroup*>(obj);
}

void CmdCollaborationTopic::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    auto* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Topic"));

    App::DocumentObjectGroup* group = getTopicGroup(doc);
    if (!group) {
        abortCommand();
        return;
    }

    auto* obj = group->addObject("Collaboration::Topic", "Topic");
    if (!obj) {
        abortCommand();
        return;
    }

    // Set basic properties
    auto* topic = freecad_cast<Topic*>(obj);
    topic->LabelText.setValue(std::vector<std::string> {"Topic"});

    auto selEx = Gui::Selection().getSelectionEx(
        nullptr,
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::NoResolve,
        true
    );
    if (selEx.empty()) {
        doCommand(
            Gui,
            "App.ActiveDocument.getObject('%s').ViewObject.DisplayMode = '%s'",
            topic->getNameInDocument(),
            ViewProviderTopic::NoneDisplayMode.c_str()
        );
        commitCommand();
        return;
    }

    // The base position is the origin at first as fallback.
    Base::Vector3d basePos;

    auto finish = [&]() {
        topic->BasePosition.setValue(basePos);
        const auto textPos = basePos + Base::Vector3d(10, 10, 10);
        topic->TextPosition.setValue(textPos);

        commitCommand();
        doCommand(Gui, "Gui.ActiveDocument.getObject('%s').doubleClicked()", topic->getNameInDocument());
        doc->recompute();
    };

    // Use the first selected item in active 3D view
    const Gui::SelectionObject& sel = selEx.front();

    const auto& picked = sel.getPickedPoints();
    // More precise base position from picked points.
    if (!picked.empty()) {
        basePos = picked.front();
    }

    const App::DocumentObject* selected = sel.getObject();
    auto* selectedObj = const_cast<App::DocumentObject*>(selected);
    if (selectedObj == nullptr) {
        finish();
        return;
    }

    auto* placementProp = selectedObj->getPlacementProperty();
    if (placementProp == nullptr) {
        finish();
        return;
    }

    // We use placement type Geometry, so we use the placement of the geometry.
    // Base position will act as an offset and is therefore reset to (0,0,0)
    basePos = Base::Vector3d(0, 0, 0);
    topic->Geometry.setValue(selectedObj, sel.getSubNames());
    topic->PlacementType.setValue("Geometry");
    finish();
}

bool CmdCollaborationTopic::isActive()
{
    return (Gui::Control().activeDialog() == nullptr) && Gui::Application::Instance->activeDocument();
}

void createCollaborationCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdCollaborationTopic());
}
