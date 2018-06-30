/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>

#include <Mod/Assembly/App/Product.h>


using namespace std;

extern Assembly::Item *ActiveAsmObject;


//===========================================================================

DEF_STD_CMD(CmdAssemblyAddNewPart);

CmdAssemblyAddNewPart::CmdAssemblyAddNewPart()
	:Command("Assembly_AddNewPart")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Add new Part");
    sToolTipText    = QT_TR_NOOP("Add a new Part into the active Assembly");
    sWhatsThis      = "Assembly_AddNewPart";
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_Add_New_Part.svg";
}


void CmdAssemblyAddNewPart::activated(int iMsg)
{
    Assembly::Product *dest = 0;

    unsigned int n = getSelection().countObjectsOfType(Assembly::Product::getClassTypeId());
    if (n >= 1) {
        std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Assembly::Product::getClassTypeId());
        dest = dynamic_cast<Assembly::Product*>(Sel.front());
    }else if(ActiveAsmObject && ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
        dest = dynamic_cast<Assembly::Product*>(ActiveAsmObject);
    }else {

        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active or selected assembly"),
                             QObject::tr("You need a active or selected assembly to insert a part in."));
	return;
    }

    openCommand("Insert Part");
    // need the help of the Part module to set up a Part
    addModule(App,"PartDesign");
    addModule(Gui,"PartDesignGui");
    addModule(Gui,"AssemblyGui");

    std::string PartName = getUniqueObjectName("Part");
    std::string ProductName = dest->getNameInDocument();
    std::string RefName = getUniqueObjectName((PartName + "-1").c_str());

    doCommand(Doc,"App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
	doCommand(Doc,"App.activeDocument().addObject('Assembly::ProductRef','%s')",RefName.c_str());
	doCommand(Doc,"App.activeDocument().%s.Items = App.activeDocument().%s.Items + [App.activeDocument().%s]",ProductName.c_str(),ProductName.c_str(),RefName.c_str());
	doCommand(Doc,"App.activeDocument().%s.Item = App.activeDocument().%s",RefName.c_str(),PartName.c_str());
    doCommand(Doc,"AssemblyGui.setActiveAssembly(App.activeDocument().%s)",ProductName.c_str());
    
    // create a PartDesign Part for now, can be later any kind of Part or an empty one
    doCommand(Gui::Command::Doc,"PartDesignGui.setUpPart(App.activeDocument().%s)",PartName.c_str());

    this->updateActive();
}

//===========================================================================

DEF_STD_CMD(CmdAssemblyAddNewComponent);

CmdAssemblyAddNewComponent::CmdAssemblyAddNewComponent()
	:Command("Assembly_AddNewComponent")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Add new Assembly");
    sToolTipText    = QT_TR_NOOP("Add a new Subassembly into the active Assembly");
    sWhatsThis      = "Assembly_AddNewComponent";
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_Assembly_Create_New";
}


void CmdAssemblyAddNewComponent::activated(int iMsg)
{
    Assembly::Product *dest = 0;

    unsigned int n = getSelection().countObjectsOfType(Assembly::Product::getClassTypeId());
    if (n >= 1) {
        std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Assembly::Product::getClassTypeId());
        dest = dynamic_cast<Assembly::Product*>(Sel.front());
    }else if(ActiveAsmObject && ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
        dest = dynamic_cast<Assembly::Product*>(ActiveAsmObject);
    }else {

        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active or selected assembly"),
                             QObject::tr("You need a active or selected assembly to insert a component in."));
	    return;
    }

    openCommand("Insert Component");
    std::string NewProdName = getUniqueObjectName("Product");
    std::string ProductName = dest->getNameInDocument();
    std::string RefName = getUniqueObjectName((NewProdName + "-1").c_str());

    doCommand(Doc,"App.activeDocument().addObject('Assembly::Product','%s')",NewProdName.c_str());
	doCommand(Doc,"App.activeDocument().addObject('Assembly::ProductRef','%s')",RefName.c_str());
	doCommand(Doc,"App.activeDocument().%s.Items = App.activeDocument().%s.Items + [App.activeDocument().%s]",ProductName.c_str(),ProductName.c_str(),RefName.c_str());
	doCommand(Doc,"App.activeDocument().%s.Item = App.activeDocument().%s",RefName.c_str(),NewProdName.c_str());

}

//===========================================================================

DEF_STD_CMD(CmdAssemblyAddExistingComponent);

CmdAssemblyAddExistingComponent::CmdAssemblyAddExistingComponent()
	:Command("Assembly_AddExistingComponent")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Add existing Component...");
    sToolTipText    = QT_TR_NOOP("Add a existing Component into the active Assembly, STEP, IGES or BREP");
    sWhatsThis      = "Assembly_AddExistingComponent";
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_Add_Existing_Part";
}


void CmdAssemblyAddExistingComponent::activated(int iMsg)
{
    Assembly::Product *dest = 0;

    unsigned int n = getSelection().countObjectsOfType(Assembly::Product::getClassTypeId());
    if (n >= 1) {
        std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Assembly::Product::getClassTypeId());
        dest = dynamic_cast<Assembly::Product*>(Sel.front());
    }else if(ActiveAsmObject && ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
        dest = dynamic_cast<Assembly::Product*>(ActiveAsmObject);
    }else {
      
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active or selected assembly"),
                             QObject::tr("You need a active or selected assembly to insert a component in."));
	return;
    }

    // asking for file name (only step at the moment) 
    QStringList filter;
    filter << QString::fromAscii("STEP (*.stp *.step)");
    filter << QString::fromAscii("IGES (*.igs *.iges)");
    filter << QString::fromAscii("BREP (*.brp *.brep)");
    filter << QString::fromAscii("Mesh (*.stl *.obj)");
    filter << QString::fromAscii("VRML (*.wrl)");

    QString select;
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")), &select);
    if (!fn.isEmpty()) {
        Gui::WaitCursor wc;
        App::Document* pDoc = getDocument();
        if (!pDoc) return; // no document
        openCommand("Import an Assembly");
        if (select == filter[1] ||
            select == filter[3]) {
            doCommand(Doc, "import ImportGui");
            doCommand(Doc, "ImportGui.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        else {
            doCommand(Doc, "import Part");
            doCommand(Doc, "Part.insert(\"%s\",\"%s\")", (const char*)fn.toUtf8(), pDoc->getName());
        }
        commitCommand();
        this->updateActive();
    }      
}

//===========================================================================

DEF_STD_CMD(CmdAssemblyImport);

CmdAssemblyImport::CmdAssemblyImport()
	:Command("Assembly_Import")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Import assembly...");
    sToolTipText    = QT_TR_NOOP("Import one or more files and create a assembly structure.");
    sWhatsThis      = "Assembly_Import";
    sStatusTip      = sToolTipText;
    sPixmap         = 0;
}


void CmdAssemblyImport::activated(int iMsg)
{
    Assembly::Product *dest = 0;

    unsigned int n = getSelection().countObjectsOfType(Assembly::Product::getClassTypeId());
    if (n >= 1) {
        std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Assembly::Product::getClassTypeId());
        dest = dynamic_cast<Assembly::Product*>(Sel.front());
    }else if(ActiveAsmObject && ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::Product::getClassTypeId())) {
        dest = dynamic_cast<Assembly::Product*>(ActiveAsmObject);
    }

    // asking for file name (only step at the moment) 
    QStringList filter;
    filter << QString::fromAscii("STEP (*.stp *.step)");
    filter << QString::fromAscii("IGES (*.igs *.iges)");
    filter << QString::fromAscii("BREP (*.brp *.brep)");
    filter << QString::fromAscii("Mesh (*.stl *.obj)");
    filter << QString::fromAscii("VRML (*.wrl)");

    QString select;
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), QString(), QString(), filter.join(QLatin1String(";;")), &select);
    if (!fn.isEmpty()) {
        Gui::WaitCursor wc;
        App::Document* pDoc = getDocument();
        if (!pDoc) return; // no document
        openCommand("Import an Assembly");
        if (select == filter[0] ||
            select == filter[1]) {
            doCommand(Doc, "import Import");
            doCommand(Doc, "Import.importAssembly('%s')", (const char*)fn.toUtf8());
        }
        else {
			return;
		}
        commitCommand();
        this->updateActive();
    }      
}
void CreateAssemblyCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdAssemblyAddNewPart());
    rcCmdMgr.addCommand(new CmdAssemblyAddNewComponent());
    rcCmdMgr.addCommand(new CmdAssemblyAddExistingComponent());
    rcCmdMgr.addCommand(new CmdAssemblyImport());
 }
