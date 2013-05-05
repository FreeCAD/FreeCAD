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

#include <Mod/Assembly/App/ItemAssembly.h>
#include <Mod/Assembly/App/ConstraintGroup.h>


using namespace std;

extern Assembly::Item *ActiveAsmObject;

// Helper methods ===========================================================

Assembly::ConstraintGroup * getConstraintGroup(Assembly::ItemAssembly *Asm)
{
    Assembly::ConstraintGroup *ConstGrp = 0;

    std::vector<App::DocumentObject*> Ano = Asm->Annotations.getValues();
    for(std::vector<App::DocumentObject*>::const_iterator it = Ano.begin();it!=Ano.end();++it){
        if((*it)->getTypeId().isDerivedFrom(Assembly::ConstraintGroup::getClassTypeId() )){
            ConstGrp = static_cast<Assembly::ConstraintGroup*>(*it);
            break;
        }
    }
    return ConstGrp;
}

bool getConstraintPrerequisits(Assembly::ItemAssembly **Asm,Assembly::ConstraintGroup **ConstGrp)
{
    if(!ActiveAsmObject || !ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::ItemAssembly::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Assembly"),
                QObject::tr("You need a active (blue) Assembly to insert a Constraint. Please create a new one or make one active (double click)."));
        return true;
    }

    *Asm = static_cast<Assembly::ItemAssembly*>(ActiveAsmObject);

    // find the Constraint group of the active Assembly
    *ConstGrp = getConstraintGroup(*Asm);
    // if it hasen't aleardy one, create one:
    if(!*ConstGrp){
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('Assembly::ConstraintGroup','ConstraintGroup')");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = 'ConstraintGroup'");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Annotations = App.activeDocument().%s.Annotations + [App.activeDocument().ActiveObject]",(*Asm)->getNameInDocument(),(*Asm)->getNameInDocument()); 

    }
    // find now
    *ConstGrp = getConstraintGroup(*Asm);
    if(!*ConstGrp)
        throw Base::Exception("Could not create Assembly::ConstraintGroup in active Assembly");

    // return with no error
    return false;

}

std::string asSubLinkString(Assembly::ItemPart* part, std::string element) {
  std::string buf;
    buf += "(App.ActiveDocument.";
    buf += part->getNameInDocument(); 
    buf += ",['";
    buf += element;
    buf += "'])"; 
    return buf;
}

//===========================================================================

DEF_STD_CMD(CmdAssemblyConstraintAxle);

CmdAssemblyConstraintAxle::CmdAssemblyConstraintAxle()
	:Command("Assembly_ConstraintAxle")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Axle...");
    sToolTipText    = QT_TR_NOOP("set a axle constraint between two objects");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/Axle_constraint";
}


void CmdAssemblyConstraintAxle::activated(int iMsg)
{
    Assembly::ItemAssembly *Asm=0;
    Assembly::ConstraintGroup *ConstGrp=0;

    // retrive the standard objects needed
    if(getConstraintPrerequisits(&Asm,&ConstGrp))
        return;
    
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();
    if(objs.size() != 2) {
        Base::Console().Message("you must select two geometries on two diffrent parts\n");
        return;
    };
    
    Assembly::ItemPart* part1 = Asm->getContainingPart(objs[0].getObject());
    Assembly::ItemPart* part2 = Asm->getContainingPart(objs[1].getObject());
    if(!part1 || !part2) {
        Base::Console().Message("The selected objects need to belong to the active assembly\n");
        return;
    };
        
    openCommand("Insert Constraint Axle");
    std::string ConstrName = getUniqueObjectName("Axle");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintAxis','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().%s.addConstraint(App.activeDocument().ActiveObject)",ConstGrp->getNameInDocument());
      
}

/******************************************************************************************/

DEF_STD_CMD(CmdAssemblyConstraintFix);

CmdAssemblyConstraintFix::CmdAssemblyConstraintFix()
	:Command("Assembly_ConstraintFix")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Fix...");
    sToolTipText    = QT_TR_NOOP("Fixes a part in it's rotation and translation");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintLock";
}


void CmdAssemblyConstraintFix::activated(int iMsg)
{
    Assembly::ItemAssembly *Asm=0;
    Assembly::ConstraintGroup *ConstGrp=0;

    // retrive the standard objects needed
    if(getConstraintPrerequisits(&Asm,&ConstGrp))
        return;
    
    std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();
    if(objs.size() != 1) {
        Base::Console().Message("you must select one part\n");
        return;
    };
    
    Assembly::ItemPart* part = Asm->getContainingPart(objs[0].getObject());
    if(!part) {
        Base::Console().Message("The selected object need to belong to the active assembly\n");
        return;
    };
        
    openCommand("Insert Constraint Fix");
    std::string ConstrName = getUniqueObjectName("Fix");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintFix','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().%s.addConstraint(App.activeDocument().ActiveObject)",ConstGrp->getNameInDocument());
      
}

void CreateAssemblyConstraintCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdAssemblyConstraintFix());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintAxle());
 }
