/***************************************************************************
 *   Copyright (c) 2017 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include <QApplication>
#endif

#include <App/GroupExtension.h>
#include <App/Document.h>

#include "Command.h"
#include "ActiveObjectList.h"
#include "Application.h"
#include "Document.h"
#include "MDIView.h"
#include "ViewProviderDocumentObject.h"
#include "Selection.h"

using namespace Gui;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// Std_Part
//===========================================================================
DEF_STD_CMD_A(StdCmdPart)

StdCmdPart::StdCmdPart()
  : Command("Std_Part")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("New Part");
    sToolTipText  = QT_TR_NOOP("Creates a part, which is a general-purpose container to group objects so they "
                               "act as a unit in the 3D view. It is intended to arrange objects that have a part "
                               "TopoShape, like part primitives, Part Design bodies, and other parts.");
    sWhatsThis    = "Std_Part";
    sStatusTip    = sToolTipText;
    sPixmap       = "Geofeaturegroup";
}

void StdCmdPart::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add a part"));
    std::string FeatName = getUniqueObjectName("Part");

    std::string PartName;
    PartName = getUniqueObjectName("Part");
    doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
    // TODO We really must set label ourselves? (2015-08-17, Fat-Zer)
    doCommand(Doc,"App.activeDocument().%s.Label = '%s'", PartName.c_str(),
            QObject::tr(PartName.c_str()).toUtf8().data());

    doCommand(Doc,
    "selected_objects = Gui.Selection.getSelection()\n"
    "if len(selected_objects) > 1:\n"
    "    for obj in selected_objects:\n"
    "        # Add subobjects if obj is a container\n"
    "        if hasattr(obj, 'OutList') and len(obj.OutList) > 0:\n"
    "            for child in obj.OutList:\n"
    "                App.activeDocument().%s.addObject(child)\n"
    "        App.activeDocument().%s.addObject(obj)\n",
    PartName.c_str(), PartName.c_str());

    doCommand(Gui::Command::Gui, "Gui.activateView('Gui::View3DInventor', True)\n"
                                 "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
            PARTKEY, PartName.c_str());

    updateActive();
}

bool StdCmdPart::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Std_Group
//===========================================================================
DEF_STD_CMD_A(StdCmdGroup)

StdCmdGroup::StdCmdGroup()
  : Command("Std_Group")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("New Group");
    sToolTipText = QT_TR_NOOP("Creates a group, which is a general-purpose container to group objects in the "
                              "tree view, regardless of their data type. It is a simple folder to organize "
                              "the objects in a model.");
    sWhatsThis    = "Std_Group";
    sStatusTip    = sToolTipText;
    sPixmap       = "folder";
}

void StdCmdGroup::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add a group"));

    std::string GroupName;
    GroupName = getUniqueObjectName("Group");
    QString label = QApplication::translate("Std_Group", "Group");

    // create a group
    doCommand(Doc,"group = App.activeDocument().addObject('App::DocumentObjectGroup','%s')",GroupName.c_str());
    doCommand(Doc,"group.Label = '%s'", label.toUtf8().data());
    doCommand(Doc,"App.activeDocument().Tip = group");

    // try to add the group to any active object that supports grouping (has GroupExtension)
    if (auto* activeDoc = Gui::Application::Instance->activeDocument()) {
        if (auto* activeView = activeDoc->getActiveView()) {
            // find the first active object with GroupExtension
            if (auto* activeObj = activeView->getActiveObjectWithExtension(
                    App::GroupExtension::getExtensionClassTypeId())) {
                doCommand(Doc,
                          "active_obj = App.activeDocument().getObject('%s')\n"
                          "if active_obj and active_obj.allowObject(group):\n"
                          "    active_obj.Group += [group]",
                          activeObj->getNameInDocument());
            }
        }
    } // if we have no active object, group will be added to root doc

    commitCommand();

    Gui::Document* gui = Application::Instance->activeDocument();
    App::Document* app = gui->getDocument();
    ViewProvider* vp = gui->getViewProvider(app->getActiveObject());
    if (vp && vp->isDerivedFrom<ViewProviderDocumentObject>())
        gui->signalScrollToObject(*static_cast<ViewProviderDocumentObject*>(vp));
}

bool StdCmdGroup::isActive()
{
    return hasActiveDocument();
}

//===========================================================================
// Std_VarSet
//===========================================================================
DEF_STD_CMD_A(StdCmdVarSet)

StdCmdVarSet::StdCmdVarSet()
  : Command("Std_VarSet")
{
    sGroup        = "Structure";
    sMenuText     = QT_TR_NOOP("Variable Set");
    sToolTipText  = QT_TR_NOOP("Creates a variable set, which is an object that maintains a set of properties to be used as variables");
    sWhatsThis    = "Std_VarSet";
    sStatusTip    = sToolTipText;
    sPixmap       = "VarSet";
}

void StdCmdVarSet::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    openCommand(QT_TRANSLATE_NOOP("Command", "Add a variable set"));

    std::string VarSetName;
    VarSetName = getUniqueObjectName("VarSet");
    doCommand(Doc,"App.activeDocument().addObject('App::VarSet','%s')",VarSetName.c_str());

    // add the varset to a group if it is selected
    auto sels = Selection().getSelectionEx(nullptr, App::DocumentObject::getClassTypeId(),
        ResolveMode::OldStyleElement, true);
    if (sels.size() == 1) {
        App::DocumentObject* obj = sels[0].getObject();
        auto group = obj->getExtension<App::GroupExtension>();
        if (group) {
            Gui::Document* docGui = Application::Instance->activeDocument();
            App::Document* doc = docGui->getDocument();
            group->addObject(doc->getObject(VarSetName.c_str()));
        }
    }
    commitCommand();

    doCommand(Doc, "App.ActiveDocument.getObject('%s').ViewObject.doubleClicked()", VarSetName.c_str());
}

bool StdCmdVarSet::isActive()
{
    return hasActiveDocument();
}

namespace Gui {

void CreateStructureCommands()
{
    CommandManager &rcCmdMgr = Application::Instance->commandManager();

    rcCmdMgr.addCommand(new StdCmdPart());
    rcCmdMgr.addCommand(new StdCmdGroup());
    rcCmdMgr.addCommand(new StdCmdVarSet());
}

} // namespace Gui
