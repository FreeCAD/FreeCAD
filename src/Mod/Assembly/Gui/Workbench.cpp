/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <qobject.h>
#endif

#include "Workbench.h"
#include <App/Application.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Control.h>
#include <Gui/Command.h>

using namespace AssemblyGui;

/// @namespace AssemblyGui @class Workbench
TYPESYSTEM_SOURCE(AssemblyGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand(QT_TR_NOOP("Assembly"));
    *part << "Assembly_ConstraintAxle";
    *part << "Separator";
    *part << "Assembly_AddNewPart";
    *part << "Assembly_AddNewComponent";
    *part << "Assembly_AddExistingComponent";
     return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");


    Gui::MenuItem* asmCmd = new Gui::MenuItem();
    root->insertItem(item, asmCmd);
    asmCmd->setCommand("&Assembly");
    *asmCmd << "Assembly_ConstraintAxle"
            << "Separator"
            << "Assembly_AddNewPart"
            << "Assembly_AddNewComponent"
            << "Assembly_AddExistingComponent";


    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

void Workbench::activated()
{
    Gui::Workbench::activated();

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Asm[] = {
        "Assembly_AddNewPart",
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Assembly::Item COUNT 1",
        Asm,
        "Assembly tools",
        "Part_Box"
    ));

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();

    App::Document *doc = App::GetApplication().getActiveDocument();
    if(!doc){
        // create a new document
        std::string uniqueName = App::GetApplication().getUniqueDocumentName("Assembly1");
        Gui::Command::doCommand(Gui::Command::Doc,"App.newDocument('%s')",uniqueName.c_str());
        doc = App::GetApplication().getActiveDocument();

    }
    // now we should have a document! 
    assert(doc);

    if(doc->countObjects()==0){
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Assembly::ItemAssembly','Product')");
        Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(App.activeDocument().Product)");
    }

    Gui::Control().showModelView();
}

void Workbench::deactivated()
{
    Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(None)");

    Gui::Workbench::deactivated();
    removeTaskWatcher();

}

