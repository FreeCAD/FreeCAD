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
#include <Gui/DlgCheckableMessageBox.h>


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
    *part << "Assembly_Constraint";
    *part << "Assembly_ConstraintFix";
    *part << "Assembly_ConstraintDistance";
    *part << "Assembly_ConstraintOrientation";
    *part << "Assembly_ConstraintAngle";
    *part << "Assembly_ConstraintCoincidence";
    *part << "Assembly_ConstraintAlignment";
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
    *asmCmd << "Assembly_ConstraintFix"
	    << "Assembly_ConstraintDistance"
	    << "Assembly_ConstraintOrientation"
	    << "Assembly_ConstraintAngle"
	    << "Assembly_ConstraintCoincidence"
	    << "Assembly_ConstraintAlignment"
            << "Separator"
            << "Assembly_AddNewPart"
            << "Assembly_AddNewComponent"
            << "Assembly_AddExistingComponent";

    Gui::MenuItem* impCmd = new Gui::MenuItem();
    root->insertItem(asmCmd, impCmd);
    impCmd->setCommand("&Import");
    *impCmd << "Assembly_Import";


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
    // show a warning about the Alpha state of FreeCAD Assembly
    Gui::Dialog::DlgCheckableMessageBox::showMessage(
        QString::fromLatin1("Assembly warning"), 
        QString::fromLatin1(
        "<h2>The <b>Assembly</b> module of FreeCAD is in <b>Alpha state</b>! </h2>"
        "Use for <b>testing purpose only!</b> The object structure is still changing.<br>"
        "You might not be able to load your work in a newer Version of FreeCAD. <br><br>"
        "For further information see the Assembly project page:<br>"
        " <a href=\"http://www.freecadweb.org/wiki/index.php?title=Assembly_project\">http://www.freecadweb.org/wiki/index.php?title=Assembly_project</a> <br>"
        "Or the Assembly dedicated portion of our forum:<br>"
        " <a href=\"http://forum.freecadweb.org/viewforum.php?f=20&sid=2a1a326251c44576f450739e4a74c37d\">http://forum.freecadweb.org/</a> <br>"
                            ),
        false,
        QString::fromLatin1("Don't tell me again, I know the risk!")
                                                    );

    // now we should have a document! 
    assert(doc);

    if(doc->countObjects()==0){
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Assembly::ItemAssembly','Assembly')");
        Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(App.activeDocument().Assembly)");
    }

    Gui::Control().showModelView();
}

void Workbench::deactivated()
{
    Gui::Command::doCommand(Gui::Command::Doc,"AssemblyGui.setActiveAssembly(None)");

    Gui::Workbench::deactivated();
    removeTaskWatcher();

}

