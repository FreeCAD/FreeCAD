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
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <QApplication>
# include <QInputDialog>
# include <TopExp_Explorer.hxx>
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/SelectionFilter.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/Gui/SketchOrientationDialog.h>



using namespace std;

extern PartDesign::Body *ActivePartObject;

//===========================================================================
// Helper for Body
//===========================================================================

PartDesign::Body *getBody(void)
{
    if(!ActivePartObject){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
            QObject::tr("To do PartDesign you need a active Body object in the documnt. Please make one active or create one. If you have a document with PartDesign objects without Body, use the transfer methode in PartDesign to put it into a Body."));
    }
    return ActivePartObject;

}


//===========================================================================
// Part_Pad
//===========================================================================

/* Sketch commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignNewSketch);

CmdPartDesignNewSketch::CmdPartDesignNewSketch()
  :Command("PartDesign_NewSketch")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create sketch");
    sToolTipText    = QT_TR_NOOP("Create a new sketch");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_NewSketch";
}


void CmdPartDesignNewSketch::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = getBody();

    // No PartDesign feature without Body past FreeCAD 0.13
    if(!pcActiveBody) return;

    Gui::SelectionFilter SketchFilter("SELECT Sketcher::SketchObject COUNT 1");
    Gui::SelectionFilter FaceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1");

    if (SketchFilter.match()) {
        Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(SketchFilter.Result[0][0].getObject());
        openCommand("Edit Sketch");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",Sketch->getNameInDocument());
    }
    else if (FaceFilter.match()) {
        // get the selected object
        Part::Feature *part = static_cast<Part::Feature*>(FaceFilter.Result[0][0].getObject());
        Base::Placement ObjectPos = part->Placement.getValue();
        const std::vector<std::string> &sub = FaceFilter.Result[0][0].getSubNames();
        if (sub.size() > 1){
            // No assert for wrong user input!
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Several sub-elements selected"),
                QObject::tr("You have to select a single face as support for a sketch!"));
            return;
        }
        // get the selected sub shape (a Face)
        const Part::TopoShape &shape = part->Shape.getValue();
        TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
        const TopoDS_Face& face = TopoDS::Face(sh);
        if (face.IsNull()){
            // No assert for wrong user input!
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No support face selected"),
                QObject::tr("You have to select a face as support for a sketch!"));
            return;
        }

        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane){
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No planar support"),
                QObject::tr("You need a planar face as support for a sketch!"));
            return;
        }

        std::string supportString = FaceFilter.Result[0][0].getAsPropertyLinkSubString();

        // create Sketch on Face
        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand("Create a Sketch on Face");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Model = App.activeDocument().%s.Model + [App.activeDocument().%s]",pcActiveBody->getNameInDocument(),pcActiveBody->getNameInDocument(),FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(),FeatName.c_str());
        doCommand(Gui,"App.activeDocument().%s.Support = %s",FeatName.c_str(),supportString.c_str());
        doCommand(Gui,"App.activeDocument().recompute()");  // recompute the sketch placement based on its support
        //doCommand(Gui,"Gui.activeDocument().activeView().setCamera('%s')",cam.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }
    else {
        // ask user for orientation
        SketcherGui::SketchOrientationDialog Dlg;

        if (Dlg.exec() != QDialog::Accepted)
            return; // canceled
        Base::Vector3d p = Dlg.Pos.getPosition();
        Base::Rotation r = Dlg.Pos.getRotation();

        // do the right view direction
        std::string camstring;
        switch(Dlg.DirType){
            case 0:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA \\n position 0 0 87 \\n orientation 0 0 1  0 \\n nearDistance -112.88701 \\n farDistance 287.28702 \\n aspectRatio 1 \\n focalDistance 87 \\n height 143.52005 }";
                break;
            case 1:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA \\n position 0 0 -87 \\n orientation -1 0 0  3.1415927 \\n nearDistance -112.88701 \\n farDistance 287.28702 \\n aspectRatio 1 \\n focalDistance 87 \\n height 143.52005 }";
                break;
            case 2:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA\\n  position 0 -87 0 \\n  orientation -1 0 0  4.712389\\n  nearDistance -112.88701\\n  farDistance 287.28702\\n  aspectRatio 1\\n  focalDistance 87\\n  height 143.52005\\n\\n}";
                break;
            case 3:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA\\n  position 0 87 0 \\n  orientation 0 0.70710683 0.70710683  3.1415927\\n  nearDistance -112.88701\\n  farDistance 287.28702\\n  aspectRatio 1\\n  focalDistance 87\\n  height 143.52005\\n\\n}";
                break;
            case 4:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA\\n  position 87 0 0 \\n  orientation 0.57735026 0.57735026 0.57735026  2.0943952 \\n  nearDistance -112.887\\n  farDistance 287.28699\\n  aspectRatio 1\\n  focalDistance 87\\n  height 143.52005\\n\\n}";
                break;
            case 5:
                camstring = "#Inventor V2.1 ascii \\n OrthographicCamera {\\n viewportMapping ADJUST_CAMERA\\n  position -87 0 0 \\n  orientation -0.57735026 0.57735026 0.57735026  4.1887903 \\n  nearDistance -112.887\\n  farDistance 287.28699\\n  aspectRatio 1\\n  focalDistance 87\\n  height 143.52005\\n\\n}";
                break;
        }
        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand("Create a new Sketch");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Placement = App.Placement(App.Vector(%f,%f,%f),App.Rotation(%f,%f,%f,%f))",FeatName.c_str(),p.x,p.y,p.z,r[0],r[1],r[2],r[3]);
        doCommand(Doc,"App.activeDocument().%s.Model = App.activeDocument().%s.Model + [App.activeDocument().%s]",pcActiveBody->getNameInDocument(),pcActiveBody->getNameInDocument(),FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(),FeatName.c_str());
        doCommand(Gui,"Gui.activeDocument().activeView().setCamera('%s')",camstring.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }

}

bool CmdPartDesignNewSketch::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// PartDesign_Pad
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPad);

CmdPartDesignPad::CmdPartDesignPad()
  : Command("PartDesign_Pad")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Pad");
    sToolTipText  = QT_TR_NOOP("Pad a selected sketch");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pad";
}

void CmdPartDesignPad::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = getBody();

    unsigned int n = getSelection().countObjectsOfType(Part::Part2DObject::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a sketch or 2D object."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Pad");

    std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Part::Part2DObject::getClassTypeId());
    Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(Sel.front());
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    if (shape.IsNull()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is empty."));
        return;
    }

    // count free wires
    int ctWires=0;
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        ctWires++;
    }
    if (ctWires == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is not a wire."));
        return;
    }

    App::DocumentObject* support = sketch->Support.getValue();

    openCommand("Make Pad");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Pad\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Model = App.activeDocument().%s.Model + [App.activeDocument().%s]",pcActiveBody->getNameInDocument(),pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Sketch = App.activeDocument().%s",FeatName.c_str(),sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Length = 10.0",FeatName.c_str());
    updateActive();
    if (isActiveObjectValid()) {
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",sketch->getNameInDocument());
        if (support)
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",support->getNameInDocument());
    }
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    //commitCommand();
    adjustCameraPosition();

    if (support) {
        copyVisual(FeatName.c_str(), "ShapeColor", support->getNameInDocument());
        copyVisual(FeatName.c_str(), "LineColor", support->getNameInDocument());
        copyVisual(FeatName.c_str(), "PointColor", support->getNameInDocument());
    }
}

bool CmdPartDesignPad::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Pocket
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPocket);

CmdPartDesignPocket::CmdPartDesignPocket()
  : Command("PartDesign_Pocket")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Pocket");
    sToolTipText  = QT_TR_NOOP("create a pocket with the selected sketch");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pocket";
}

void CmdPartDesignPocket::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = getBody();

    unsigned int n = getSelection().countObjectsOfType(Part::Part2DObject::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a sketch or 2D object."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Pocket");

    std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Part::Part2DObject::getClassTypeId());
    Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(Sel.front());
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    if (shape.IsNull()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is empty."));
        return;
    }

    // count free wires
    int ctWires=0;
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        ctWires++;
    }
    if (ctWires == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is not a wire."));
        return;
    }
    App::DocumentObject* support = sketch->Support.getValue();
    if (support == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Support"),
            QObject::tr("The sketch has to have a support for the pocket feature.\nCreate the sketch on a face."));
        return;
    }

    openCommand("Make Pocket");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Pocket\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Model = App.activeDocument().%s.Model + [App.activeDocument().%s]",pcActiveBody->getNameInDocument(),pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Sketch = App.activeDocument().%s",FeatName.c_str(),sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Length = 5.0",FeatName.c_str());
    updateActive();
    if (isActiveObjectValid()) {
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",sketch->getNameInDocument());
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",support->getNameInDocument());
    }
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", support->getNameInDocument());
    copyVisual(FeatName.c_str(), "LineColor", support->getNameInDocument());
    copyVisual(FeatName.c_str(), "PointColor", support->getNameInDocument());
}

bool CmdPartDesignPocket::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Revolution
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignRevolution);

CmdPartDesignRevolution::CmdPartDesignRevolution()
  : Command("PartDesign_Revolution")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Revolution");
    sToolTipText  = QT_TR_NOOP("Revolve a selected sketch");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Revolution";
}

void CmdPartDesignRevolution::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = getBody();

    unsigned int n = getSelection().countObjectsOfType(Part::Part2DObject::getClassTypeId());
    if (n != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select a sketch or 2D object."));
        return;
    }

    std::string FeatName = getUniqueObjectName("Revolution");

    std::vector<App::DocumentObject*> Sel = getSelection().getObjectsOfType(Part::Part2DObject::getClassTypeId());
    Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(Sel.front());
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    if (shape.IsNull()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is empty."));
        return;
    }

    // count free wires
    int ctWires=0;
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        ctWires++;
    }
    if (ctWires == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The shape of the selected object is not a wire."));
        return;
    }

    App::DocumentObject* support = sketch->Support.getValue();

    openCommand("Make Revolution");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Revolution\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Model = App.activeDocument().%s.Model + [App.activeDocument().%s]",pcActiveBody->getNameInDocument(),pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(),FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Sketch = App.activeDocument().%s",FeatName.c_str(),sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.ReferenceAxis = (App.activeDocument().%s,['V_Axis'])",
                                                                             FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Angle = 360.0",FeatName.c_str());
    updateActive();
    if (isActiveObjectValid()) {
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",sketch->getNameInDocument());
        if (support)
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",support->getNameInDocument());
    }
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    if (support) {
        copyVisual(FeatName.c_str(), "ShapeColor", support->getNameInDocument());
        copyVisual(FeatName.c_str(), "LineColor", support->getNameInDocument());
        copyVisual(FeatName.c_str(), "PointColor", support->getNameInDocument());
    }
}

bool CmdPartDesignRevolution::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Fillet
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignFillet);

CmdPartDesignFillet::CmdPartDesignFillet()
  :Command("PartDesign_Fillet")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Fillet");
    sToolTipText  = QT_TR_NOOP("Make a fillet on an edge, face or body");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Fillet";
}

void CmdPartDesignFillet::activated(int iMsg)
{
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face or body. Only one body is allowed."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("Fillet works only on parts"));
        return;
    }
    std::string SelString = selection[0].getAsPropertyLinkSubString();
    std::string FeatName = getUniqueObjectName("Fillet");

    openCommand("Make Fillet");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Fillet\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",selection[0].getFeatName());
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "LineColor",  selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "PointColor", selection[0].getFeatName());
}

bool CmdPartDesignFillet::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Chamfer
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignChamfer);

CmdPartDesignChamfer::CmdPartDesignChamfer()
  :Command("PartDesign_Chamfer")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Chamfer");
    sToolTipText  = QT_TR_NOOP("Chamfer the selected edges of a shape");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Chamfer";
}

void CmdPartDesignChamfer::activated(int iMsg)
{
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face or body. Only one body is allowed."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("Chamfer works only on parts"));
        return;
    }
    std::string SelString = selection[0].getAsPropertyLinkSubString();
    std::string FeatName = getUniqueObjectName("Chamfer");

    openCommand("Make Chamfer");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Chamfer\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",selection[0].getFeatName());
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "LineColor",  selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "PointColor", selection[0].getFeatName());
}

bool CmdPartDesignChamfer::isActive(void)
{
    return hasActiveDocument();
}


void CreatePartDesignCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignPad());
    rcCmdMgr.addCommand(new CmdPartDesignPocket());
    rcCmdMgr.addCommand(new CmdPartDesignRevolution());
    rcCmdMgr.addCommand(new CmdPartDesignFillet());
    rcCmdMgr.addCommand(new CmdPartDesignNewSketch());
    rcCmdMgr.addCommand(new CmdPartDesignChamfer());
 }
