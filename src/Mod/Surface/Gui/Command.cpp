/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
 *                      Balázs Bámer                                       *
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
# include <sstream>
# include <QString>
# include <QDir>
# include <QFileInfo>
# include <QLineEdit>
# include <QPointer>
# include <Standard_math.hxx>
# include <TopoDS_Shape.hxx>
# include <TopExp_Explorer.hxx>
# include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Console.h>
#include <App/Document.h>
//#include <Gui/Application.h>
//#include <Gui/Command.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>

// Nate's stuff

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// CmdSurfaceFILLING THIS IS THE SURFACE FILLING COMMAND
//===========================================================================
DEF_STD_CMD(CmdSurfaceFilling);

CmdSurfaceFilling::CmdSurfaceFilling()
  :Command("Surface_Filling")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Surface Filling function");
    sToolTipText  = QT_TR_NOOP("Fills a series of boundary curves, constraint curves and verticies with a surface.");
    sWhatsThis    = QT_TR_NOOP("Surface Filling function");
    sStatusTip    = QT_TR_NOOP("Surface Filling function");
    sPixmap       = "Filling.svg";
    sAccel        = "CTRL+H";
}

void CmdSurfaceFilling::activated(int iMsg)
{
    Base::Console().Message("Hello, World!\n");
}

//===========================================================================
// CmdSurfaceCut THIS IS THE SURFACE CUT COMMAND
//===========================================================================
DEF_STD_CMD(CmdSurfaceCut);

CmdSurfaceCut::CmdSurfaceCut()
  :Command("Surface_Cut")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Surface Cut function");
    sToolTipText  = QT_TR_NOOP("Cuts a Shape with another Shape.\nReturns a modified version of the first shape");
    sWhatsThis    = QT_TR_NOOP("Surface Cut function");
    sStatusTip    = QT_TR_NOOP("Surface Cut function");
    sPixmap       = "Cut.svg";
    sAccel        = "CTRL+H";
}

void CmdSurfaceCut::activated(int iMsg)
{
/*    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    bool askUser = false;
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }
        }
    }

    std::string FeatName = getUniqueObjectName("Cut");
    std::string BaseName  = Sel[0].getFeatName();
    std::string ToolName  = Sel[1].getFeatName();

    openCommand("Part Cut");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Cut\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    copyVisual(FeatName.c_str(), "ShapeColor", BaseName.c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", BaseName.c_str());
    updateActive();
    commitCommand();*/
}

// my stuff

//===========================================================================
// Surface_Bezier
//===========================================================================
DEF_STD_CMD_A(CmdSurfaceBezier);

CmdSurfaceBezier::CmdSurfaceBezier()
  :Command("Surface_Bezier")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Bezier");
    sToolTipText  = QT_TR_NOOP("Creates a surface from 2, 3 or 4 Bezier curves");
    sWhatsThis    = "Surface_Bezier";
    sStatusTip    = sToolTipText;
    sPixmap       = "BezSurf";
}

void CmdSurfaceBezier::activated(int iMsg)
{
    // TODO filter class type
    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0/*, Part::Feature::getClassTypeId()*/);
    
    // TODO check if input feature count is between 2 and 4
    /*if (Sel.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two shapes or more, please."));
        return;
    }*/

    bool askUser = false;
    std::string FeatName = getUniqueObjectName("BezierSurface");
    std::stringstream bezListCmd;
    // std::vector<std::string> tempSelNames;
    bezListCmd << "FreeCAD.ActiveDocument.ActiveObject.aBezList = [";
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
	// TODO check object types
        /*if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }*/
            bezListCmd << "(App.activeDocument()." << it->getFeatName() << ", \'Edge1\'),";
         //   tempSelNames.push_back(it->getFeatName());
        //}
    }
    bezListCmd << "]";

    openCommand("Create Bezier surface");
    doCommand(Doc,"FreeCAD.ActiveDocument.addObject(\"Surface::BezSurf\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "FreeCAD.ActiveDocument.ActiveObject.filltype=1"); // TODO ask filltype from user and check it
    runCommand(Doc, bezListCmd.str().c_str());
    updateActive();
    commitCommand();
}

bool CmdSurfaceBezier::isActive(void)
{
    return true; //TODO check availability getSelection().countObjectsOfType(Part::Feature::getClassTypeId())>=2;
}

void CreateSurfaceCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdSurfaceFilling());
    rcCmdMgr.addCommand(new CmdSurfaceCut());
    rcCmdMgr.addCommand(new CmdSurfaceBezier());
}
