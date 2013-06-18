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
#include <QInputDialog>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include "ui_AlignmentDialog.h"

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

DEF_STD_CMD(CmdAssemblyConstraintDistance);

CmdAssemblyConstraintDistance::CmdAssemblyConstraintDistance()
	:Command("Assembly_ConstraintDistance")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Distance...");
    sToolTipText    = QT_TR_NOOP("Set the distance between two selected entitys");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintDistance";
}


void CmdAssemblyConstraintDistance::activated(int iMsg)
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
    
    bool ok;
    double d = QInputDialog::getDouble(NULL, QObject::tr("Constraint value"),
                                        QObject::tr("Distance:"), 0., -10000., 10000., 2, &ok);
    if(!ok)
      return;
        
    openCommand("Insert Constraint Distance");
    std::string ConstrName = getUniqueObjectName("Distance");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintDistance','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Distance = %f", d);
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
    
    commitCommand();
    updateActive();
      
}

/******************************************************************************************/

DEF_STD_CMD(CmdAssemblyConstraintFix);

CmdAssemblyConstraintFix::CmdAssemblyConstraintFix()
	:Command("Assembly_ConstraintFix")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Fix...");
    sToolTipText    = QT_TR_NOOP("Fix a part in it's rotation and translation");
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
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
      
    commitCommand();
    updateActive();
}

/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintAngle);

CmdAssemblyConstraintAngle::CmdAssemblyConstraintAngle()
	:Command("Assembly_ConstraintAngle")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Angle...");
    sToolTipText    = QT_TR_NOOP("Set the angle between two selected entitys");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintAngle";
}


void CmdAssemblyConstraintAngle::activated(int iMsg)
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
    
    bool ok;
    double d = QInputDialog::getDouble(NULL, QObject::tr("Constraint value"),
                                        QObject::tr("Angle:"), 0., 0., 360., 2, &ok);
    if(!ok)
      return;
        
    openCommand("Insert Constraint Angle");
    std::string ConstrName = getUniqueObjectName("Angle");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintAngle','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Angle = %f", d);
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
    
    commitCommand();
    updateActive();
      
}


/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintOrientation);

CmdAssemblyConstraintOrientation::CmdAssemblyConstraintOrientation()
	:Command("Assembly_ConstraintOrientation")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Orientation...");
    sToolTipText    = QT_TR_NOOP("Set the orientation of two selected entitys in regard to each other");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintOrientation";
}


void CmdAssemblyConstraintOrientation::activated(int iMsg)
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
    
    QStringList items;
    items << QObject::tr("Parallel") << QObject::tr("Perpendicular") << QObject::tr("Equal") << QObject::tr("Opposite");

    bool ok;
    QString item = QInputDialog::getItem(NULL, QObject::tr("Constraint value"),
                                          QObject::tr("Orientation:"), items, 0, false, &ok);
    if (!ok || item.isEmpty())
        return;
            
    openCommand("Insert Constraint Orientation");
    std::string ConstrName = getUniqueObjectName("Orientation");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintOrientation','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Orientation = '%s'", item.toStdString().c_str());
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
    
    commitCommand();
    updateActive();
      
}

/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintCoincidence);

CmdAssemblyConstraintCoincidence::CmdAssemblyConstraintCoincidence()
	:Command("Assembly_ConstraintCoincidence")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint coincidence...");
    sToolTipText    = QT_TR_NOOP("Make the selected entitys coincident");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintCoincidence";
}


void CmdAssemblyConstraintCoincidence::activated(int iMsg)
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
    
    QStringList items;
    items << QObject::tr("Parallel") << QObject::tr("Equal") << QObject::tr("Opposite");

    bool ok;
    QString item = QInputDialog::getItem(NULL, QObject::tr("Constraint value"),
                                          QObject::tr("Orientation:"), items, 0, false, &ok);
    if (!ok || item.isEmpty())
        return;
            
    openCommand("Insert Constraint Coincidence");
    std::string ConstrName = getUniqueObjectName("Coincidence");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintCoincidence','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Orientation = '%s'", item.toStdString().c_str());
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
    
    commitCommand();
    updateActive();
      
}

/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintAlignment);

CmdAssemblyConstraintAlignment::CmdAssemblyConstraintAlignment()
	:Command("Assembly_ConstraintAlignment")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint allignment...");
    sToolTipText    = QT_TR_NOOP("Align the selected entitys");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Assembly_ConstraintAlignment";
}


void CmdAssemblyConstraintAlignment::activated(int iMsg)
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
    
    QStringList items;
    items << QObject::tr("Parallel") << QObject::tr("Equal") << QObject::tr("Opposite");

    QDialog dialog;
    Ui_AlignmentDialog ui;    
    ui.setupUi(&dialog);
    ui.comboBox->addItems(items);
    if( dialog.exec() != QDialog::Accepted )
      return;
            
    openCommand("Insert Constraint Alignment");
    std::string ConstrName = getUniqueObjectName("Alignment");
    doCommand(Doc,"App.activeDocument().addObject('Assembly::ConstraintAlignment','%s')",ConstrName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1, objs[0].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2, objs[1].getSubNames()[0]).c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Orientation = '%s'", ui.comboBox->currentText().toStdString().c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Offset = %f", ui.doubleSpinBox->value());
    doCommand(Doc,"App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]",ConstGrp->getNameInDocument(),ConstGrp->getNameInDocument());
    
    commitCommand();
    updateActive();
      
}

void CreateAssemblyConstraintCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdAssemblyConstraintFix());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintDistance());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintAngle());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintOrientation());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintCoincidence());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintAlignment());
 }
