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
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Document.h>

#include <Mod/Assembly/App/Product.h>
#include <Mod/Assembly/App/ProductRef.h>
#include <Mod/Assembly/App/ConstraintGroup.h>
//#include <Mod/Assembly/Gui/TaskDlgAssemblyConstraints.h>


using namespace std;

extern Assembly::Item* ActiveAsmObject;

// Helper methods ===========================================================

Assembly::ConstraintGroup* getConstraintGroup(Assembly::Product* Asm)
{
    Assembly::ConstraintGroup* ConstGrp = 0;

    std::vector<App::DocumentObject*> Ano = Asm->Items.getValues();

    for(std::vector<App::DocumentObject*>::const_iterator it = Ano.begin(); it != Ano.end(); ++it) {
        if((*it)->getTypeId().isDerivedFrom(Assembly::ConstraintGroup::getClassTypeId())) {
            ConstGrp = static_cast<Assembly::ConstraintGroup*>(*it);
            break;
        }
    }

    return ConstGrp;
}

bool getConstraintPrerequisits(Assembly::Product** Asm, Assembly::ConstraintGroup** ConstGrp)
{
    if(!ActiveAsmObject || !ActiveAsmObject->getTypeId().isDerivedFrom(Assembly::ProductRef::getClassTypeId())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Assembly"),
                             QObject::tr("You need a active (blue) Assembly to insert a Constraint. Please create a new one or make one active (double click)."));
        return true;
    }

    *Asm = static_cast<Assembly::Product*>(ActiveAsmObject);

    // find the Constraint group of the active Assembly
    *ConstGrp = getConstraintGroup(*Asm);

    // if it hasen't aleardy one, create one:
    if(!*ConstGrp) {
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().addObject('Assembly::ConstraintGroup','ConstraintGroup')");
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().ActiveObject.Label = 'ConstraintGroup'");
        Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.Annotations = App.activeDocument().%s.Annotations + [App.activeDocument().ActiveObject]", (*Asm)->getNameInDocument(), (*Asm)->getNameInDocument());

    }

    // find now
    *ConstGrp = getConstraintGroup(*Asm);

    if(!*ConstGrp)
        throw Base::RuntimeError("Could not create Assembly::ConstraintGroup in active Assembly");

    // return with no error
    return false;

}

std::string asSubLinkString(Assembly::ProductRef* part, std::string element)
{
    std::string buf;
    buf += "(App.ActiveDocument.";
    buf += part->getNameInDocument();
    buf += ",['";
    buf += element;
    buf += "'])";
    return buf;
}

//===========================================================================

DEF_STD_CMD(CmdAssemblyConstraint);

CmdAssemblyConstraint::CmdAssemblyConstraint()
    : Command("Assembly_Constraint")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint");
    sToolTipText    = QT_TR_NOOP("Add arbitrary constraints to the assembly");
    sWhatsThis      = "Assembly_Constraint";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintGeneral";
}


void CmdAssemblyConstraint::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() > 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("Only two geometries supported by constraints"));
    //    return;
    //};

    //std::stringstream typestr1, typestr2;
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1, part2;
    //if(objs.size()>=1) {
    //    part1 = Asm->getContainingPart(objs[0].getObject());
    //    //checking the parts is enough, both or non!
    //    if(!part1.first) {
    //        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                             QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //        return;
    //    };
    //    typestr1 << "App.activeDocument().ActiveObject.First = " << asSubLinkString(part1.first, objs[0].getSubNames()[0]);
    //}
    //if(objs.size()>=2) {
    //    part2 = Asm->getContainingPart(objs[1].getObject());
    //    //checking the parts is enough, both or non!
    //    if(!part2.first) {
    //        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                             QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //        return;
    //    };
    //    typestr2 << "App.activeDocument().ActiveObject.Second = " << asSubLinkString(part2.first, objs[1].getSubNames()[0]);
    //}


    ////check if this is the right place for the constraint
    //if(part1.first && part2.first && (part1.second == part2.second) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Distance");
    //std::string ConstrName = getUniqueObjectName("Constraint");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //if(objs.size()>=1)
    //  doCommand(Doc, typestr1.str().c_str());
    //if(objs.size()>=2)
    //  doCommand(Doc, typestr2.str().c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();

    //Gui::Selection().clearCompleteSelection();
}


/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintDistance);

CmdAssemblyConstraintDistance::CmdAssemblyConstraintDistance()
    : Command("Assembly_ConstraintDistance")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Distance...");
    sToolTipText    = QT_TR_NOOP("Set the distance between two selected entities");
    sWhatsThis      = "Assembly_ConstraintDistance";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintDistance";
}


void CmdAssemblyConstraintDistance::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select two geometries on two different parts"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1 = Asm->getContainingPart(objs[0].getObject());
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part2 = Asm->getContainingPart(objs[1].getObject());

    ////checking the parts is enough, both or non!
    //if(!part1.first || !part2.first) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //    return;
    //};

    ////check if this is the right place for the constraint
    //if((part1.second == part2.second) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Distance");
    //std::string ConstrName = getUniqueObjectName("Distance");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Distance'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2.first, objs[1].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();

    //Gui::Selection().clearCompleteSelection();
}

/******************************************************************************************/

DEF_STD_CMD(CmdAssemblyConstraintFix);

CmdAssemblyConstraintFix::CmdAssemblyConstraintFix()
    : Command("Assembly_ConstraintFix")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Fix...");
    sToolTipText    = QT_TR_NOOP("Fix a part in it's rotation and translation");
    sWhatsThis      = "Assembly_ConstraintFix";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintLock";
}


void CmdAssemblyConstraintFix::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 1) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select one part only"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part = Asm->getContainingPart(objs[0].getObject());

    //if(!part.first) {
    //    Base::Console().Message("The selected part need to belong to the active assembly\n");
    //    return;
    //};

    //if(part.second != Asm) {
    //    Base::Console().Message("The selected part need belongs to an subproduct, please add constraint there\n");
    //    return;
    //}

    //openCommand("Insert Constraint Fix");

    //std::string ConstrName = getUniqueObjectName("Fix");

    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Fix'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();
    //
    //Gui::Selection().clearCompleteSelection();
}


/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintAngle);

CmdAssemblyConstraintAngle::CmdAssemblyConstraintAngle()
    : Command("Assembly_ConstraintAngle")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Angle...");
    sToolTipText    = QT_TR_NOOP("Set the angle between two selected entities");
    sWhatsThis      = "Assembly_ConstraintAngle";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintAngle";
}


void CmdAssemblyConstraintAngle::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select two geometries on two different parts"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1 = Asm->getContainingPart(objs[0].getObject());
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part2 = Asm->getContainingPart(objs[1].getObject());

    ////checking the parts is enough, both or non!
    //if(!part1.first || !part2.first) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //    return;
    //};

    ////check if this is the right place for the constraint
    //if(((part1.second == part2.second) && part1.second != Asm) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Angle");
    //std::string ConstrName = getUniqueObjectName("Angle");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Angle'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2.first, objs[1].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();

    //Gui::Selection().clearCompleteSelection();
}


/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintOrientation);

CmdAssemblyConstraintOrientation::CmdAssemblyConstraintOrientation()
    : Command("Assembly_ConstraintOrientation")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint Orientation...");
    sToolTipText    = QT_TR_NOOP("Set the orientation of two selected entities in regard to each other");
    sWhatsThis      = "Assembly_ConstraintOrientation";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintOrientation";
}


void CmdAssemblyConstraintOrientation::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select two geometries on two different parts"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1 = Asm->getContainingPart(objs[0].getObject());
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part2 = Asm->getContainingPart(objs[1].getObject());

    ////checking the parts is enough, both or non!
    //if(!part1.first || !part2.first) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //    return;
    //};

    ////check if this is the right place for the constraint
    //if((part1.second == part2.second) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Orientation");
    //std::string ConstrName = getUniqueObjectName("Orientation");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Orientation'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2.first, objs[1].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();
    //
    //Gui::Selection().clearCompleteSelection();
}

/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintCoincidence);

CmdAssemblyConstraintCoincidence::CmdAssemblyConstraintCoincidence()
    : Command("Assembly_ConstraintCoincidence")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint coincidence...");
    sToolTipText    = QT_TR_NOOP("Make the selected entities coincident");
    sWhatsThis      = "Assembly_ConstraintCoincidence";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintCoincidence";
}


void CmdAssemblyConstraintCoincidence::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select two geometries on two different parts"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1 = Asm->getContainingPart(objs[0].getObject());
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part2 = Asm->getContainingPart(objs[1].getObject());

    ////checking the parts is enough, both or non!
    //if(!part1.first || !part2.first) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //    return;
    //};

    ////check if this is the right place for the constraint
    //if((part1.second == part2.second) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Coincidence");
    //std::string ConstrName = getUniqueObjectName("Coincidence");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Coincident'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2.first, objs[1].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();
    //
    //Gui::Selection().clearCompleteSelection();
}

/******************************************************************************************/


DEF_STD_CMD(CmdAssemblyConstraintAlignment);

CmdAssemblyConstraintAlignment::CmdAssemblyConstraintAlignment()
    : Command("Assembly_ConstraintAlignment")
{
    sAppModule      = "Assembly";
    sGroup          = QT_TR_NOOP("Assembly");
    sMenuText       = QT_TR_NOOP("Constraint alignment...");
    sToolTipText    = QT_TR_NOOP("Align the selected entities");
    sWhatsThis      = "Assembly_ConstraintAlignment";
    sStatusTip      = sToolTipText;
    sPixmap         = "constraints/Assembly_ConstraintAlignment";
}


void CmdAssemblyConstraintAlignment::activated(int iMsg)
{
    //Assembly::ProductRef* Asm = 0;
    //Assembly::ConstraintGroup* ConstGrp = 0;

    //// retrieve the standard objects needed
    //if(getConstraintPrerequisits(&Asm, &ConstGrp))
    //    return;

    //std::vector<Gui::SelectionObject> objs = Gui::Selection().getSelectionEx();

    //if(objs.size() != 2) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("You need to select two geometries on two different parts"));
    //    return;
    //};

    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part1 = Asm->getContainingPart(objs[0].getObject());
    //std::pair<Assembly::PartRef*, Assembly::ProductRef*> part2 = Asm->getContainingPart(objs[1].getObject());

    ////checking the parts is enough, both or non!
    //if(!part1.first || !part2.first) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts need to belong to the active assembly (active product or one of it's subproducts)"));
    //    return;
    //};

    ////check if this is the right place for the constraint
    //if((part1.second == part2.second) && part1.second != Asm) {
    //    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
    //                         QObject::tr("The selected parts belong both to the same subassembly, please add constraints there"));
    //    return;
    //}

    //openCommand("Insert Constraint Alignment");
    //std::string ConstrName = getUniqueObjectName("Alignment");
    //doCommand(Doc, "App.activeDocument().addObject('Assembly::Constraint','%s')", ConstrName.c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Type = 'Align'");
    //doCommand(Doc, "App.activeDocument().ActiveObject.First = %s", asSubLinkString(part1.first, objs[0].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().ActiveObject.Second = %s", asSubLinkString(part2.first, objs[1].getSubNames()[0]).c_str());
    //doCommand(Doc, "App.activeDocument().%s.Constraints = App.activeDocument().%s.Constraints + [App.activeDocument().ActiveObject]", ConstGrp->getNameInDocument(), ConstGrp->getNameInDocument());

    ////updateActive();
    //doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", ConstrName.c_str());

    //commitCommand();
    //
    //Gui::Selection().clearCompleteSelection();
}

void CreateAssemblyConstraintCommands(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdAssemblyConstraint());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintFix());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintDistance());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintAngle());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintOrientation());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintCoincidence());
    rcCmdMgr.addCommand(new CmdAssemblyConstraintAlignment());
}


