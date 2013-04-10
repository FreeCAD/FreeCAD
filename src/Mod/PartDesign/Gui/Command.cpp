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
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopExp.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <QMessageBox>
#endif

#include <sstream>
#include <algorithm>

#include <App/DocumentObjectGroup.h>
#include <App/Plane.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>
#include <Gui/Tree.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumFeature.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/Gui/SketchOrientationDialog.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>

#include "FeaturePickDialog.h"
#include "Workbench.h"

using namespace std;


const char* BasePlaneNames[3] = {"Body_PlaneXY", "Body_PlaneYZ", "Body_PlaneXZ"};

//===========================================================================
// PartDesign_Body
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignBody);

CmdPartDesignBody::CmdPartDesignBody()
  : Command("PartDesign_Body")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Create body");
    sToolTipText  = QT_TR_NOOP("Create a new body feature");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Body";
}

void CmdPartDesignBody::activated(int iMsg)
{
    openCommand("Add a body feature");
    std::string FeatName = getUniqueObjectName("Body");

    // add the standard planes at the root of the feature tree
    // first check if they already exist
    // FIXME: If the user renames them, they won't be found...
    bool found = false;
    std::vector<App::DocumentObject*> planes = getDocument()->getObjectsOfType(App::Plane::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator p = planes.begin(); p != planes.end(); p++) {
        for (unsigned i = 0; i < 3; i++) {
            if (strcmp(BasePlaneNames[i], (*p)->getNameInDocument()) == 0) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        // Add the planes ...
        doCommand(Doc,"App.activeDocument().addObject('App::Plane','%s')", BasePlaneNames[0]);
        doCommand(Doc,"App.activeDocument().ActiveObject.Label = 'XY-Plane'");
        doCommand(Doc,"App.activeDocument().addObject('App::Plane','%s')", BasePlaneNames[1]);
        doCommand(Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
        doCommand(Doc,"App.activeDocument().ActiveObject.Label = 'YZ-Plane'");
        doCommand(Doc,"App.activeDocument().addObject('App::Plane','%s')", BasePlaneNames[2]);
        doCommand(Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),90))");
        doCommand(Doc,"App.activeDocument().ActiveObject.Label = 'XZ-Plane'");
        // ... and put them in the 'Origin' group
        doCommand(Doc,"App.activeDocument().addObject('App::DocumentObjectGroup','Origin')");
        for (unsigned i = 0; i < 3; i++)
            doCommand(Doc,"App.activeDocument().Origin.addObject(App.activeDocument().getObject('%s'))", BasePlaneNames[i]);
        // TODO: Fold the group (is that possible through the Python interface?)
    }

    // add the Body feature itself, and make it active
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Body','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tip = None",FeatName.c_str());
    doCommand(Doc,"import PartDesignGui");
    doCommand(Gui,"PartDesignGui.setActivePart(App.ActiveDocument.ActiveObject)");
    // Make the "Create sketch" prompt appear in the task panel
    doCommand(Gui,"Gui.Selection.clearSelection()");
    doCommand(Gui,"Gui.Selection.addSelection(App.ActiveDocument.ActiveObject)");

    updateActive();
}

bool CmdPartDesignBody::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_MoveTip
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMoveTip);

CmdPartDesignMoveTip::CmdPartDesignMoveTip()
  : Command("PartDesign_MoveTip")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Insert here");
    sToolTipText  = QT_TR_NOOP("Move insert point to selected feature");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_MoveTip";
}

void CmdPartDesignMoveTip::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;
    App::DocumentObject* selFeature = features.front();

    if (!pcActiveBody->hasFeature(selFeature)) {
        // Switch to other body
        pcActiveBody = PartDesign::Body::findBodyOf(selFeature);
        if (pcActiveBody != NULL)
            Gui::Command::doCommand(Gui::Command::Gui,"PartDesignGui.setActivePart(App.activeDocument().%s)",
                                    pcActiveBody->getNameInDocument());
    }

    App::DocumentObject* oldTip = pcActiveBody->Tip.getValue();
    doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", oldTip->getNameInDocument());
    App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature();
    if (prevSolidFeature != NULL)
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());

    openCommand("Move insert point to selected feature");
    doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(), selFeature->getNameInDocument());

    // Adjust visibility to show only the Tip feature and (if the Tip feature is not solid) the solid feature prior to the Tip
    doCommand(Gui,"Gui.activeDocument().show(\"%s\")", selFeature->getNameInDocument());
    prevSolidFeature = pcActiveBody->getPrevSolidFeature();
    if ((prevSolidFeature != NULL) && !PartDesign::Body::isSolidFeature(selFeature))
        doCommand(Gui,"Gui.activeDocument().show(\"%s\")", prevSolidFeature->getNameInDocument());
}

bool CmdPartDesignMoveTip::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Sketch
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
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();

    // No PartDesign feature without Body past FreeCAD 0.13
    if(!pcActiveBody) return;

    Gui::SelectionFilter SketchFilter("SELECT Sketcher::SketchObject COUNT 1");
    Gui::SelectionFilter FaceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    Gui::SelectionFilter PlaneFilter1 ("SELECT App::Plane COUNT 1");
    Gui::SelectionFilter PlaneFilter2 ("SELECT PartDesign::Plane COUNT 1");

    if (SketchFilter.match()) {
        Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(SketchFilter.Result[0][0].getObject());
        openCommand("Edit Sketch");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",Sketch->getNameInDocument());
    }
    else if (FaceFilter.match() || PlaneFilter1.match() || PlaneFilter2.match()) {
        // get the selected object
        std::string supportString;

        if (FaceFilter.match()) {
            Part::Feature *part = static_cast<Part::Feature*>(FaceFilter.Result[0][0].getObject());
            // FIXME: Reject or warn about feature that is outside of active body, and feature
            // that comes after the current insert point (Tip)
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

            supportString = FaceFilter.Result[0][0].getAsPropertyLinkSubString();
        } else {
            if (PlaneFilter1.match())
                supportString = PlaneFilter1.Result[0][0].getAsPropertyLinkSubString();
            else
                supportString = PlaneFilter2.Result[0][0].getAsPropertyLinkSubString();
        }

        // create Sketch on Face
        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand("Create a Sketch on Face");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());        
        doCommand(Doc,"App.activeDocument().%s.Support = %s",FeatName.c_str(),supportString.c_str());
        doCommand(Doc,"App.activeDocument().%s.insertFeature(App.activeDocument().%s)",
                       pcActiveBody->getNameInDocument(), FeatName.c_str());
        doCommand(Gui,"App.activeDocument().recompute()");  // recompute the sketch placement based on its support
        //doCommand(Gui,"Gui.activeDocument().activeView().setCamera('%s')",cam.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }
    else {
        // Get a valid plane from the user
        std::vector<PartDesignGui::FeaturePickDialog::featureStatus> status;
        std::vector<App::DocumentObject*> planes = getDocument()->getObjectsOfType(App::Plane::getClassTypeId());
        std::vector<App::DocumentObject*> planestmp = getDocument()->getObjectsOfType(PartDesign::Plane::getClassTypeId());
        planes.insert(planes.end(), planestmp.begin(), planestmp.end());

        unsigned validPlanes = 0;
        std::vector<App::DocumentObject*>::const_iterator firstValidPlane = planes.end();

        for (std::vector<App::DocumentObject*>::iterator p = planes.begin(); p != planes.end(); p++) {
            // Check whether this plane is a base plane
            bool base = false;
            for (unsigned i = 0; i < 3; i++) {
                if (strcmp(BasePlaneNames[i], (*p)->getNameInDocument()) == 0) {
                    status.push_back(PartDesignGui::FeaturePickDialog::basePlane);
                    if (firstValidPlane == planes.end())
                        firstValidPlane = p;
                    validPlanes++;
                    base = true;
                    break;
                }
            }
            if (base) continue;

            // Check whether this plane belongs to the active body
            PartDesign::Body* body = PartDesignGui::getBody();
            if (!body->hasFeature(*p)) {
                status.push_back(PartDesignGui::FeaturePickDialog::otherBody);
                continue;
            } else {
                if (body->isAfterTip(*p))
                    status.push_back(PartDesignGui::FeaturePickDialog::afterTip);
                continue;
            }

            // All checks passed - found a valid plane
            if (firstValidPlane == planes.end())
                firstValidPlane = p;
            validPlanes++;
            status.push_back(PartDesignGui::FeaturePickDialog::validFeature);
        }

        if (validPlanes == 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid planes in this document"),
                QObject::tr("Please create a plane first or select a face to sketch on"));
            return;
        }

        // If there is more than one possibility, show dialog and let user pick plane
        if (validPlanes > 1) {
            PartDesignGui::FeaturePickDialog Dlg(planes, status);
            if ((Dlg.exec() != QDialog::Accepted) || (planes = Dlg.getFeatures()).empty())
                return; // Cancelled or nothing selected
            firstValidPlane = planes.begin();
        }

        App::Plane* plane = static_cast<App::Plane*>(*firstValidPlane);
        Base::Vector3d p = plane->Placement.getValue().getPosition();
        Base::Rotation r = plane->Placement.getValue().getRotation();

        std::string FeatName = getUniqueObjectName("Sketch");
        std::string supportString = std::string("(App.activeDocument().") + plane->getNameInDocument() + ", [])";

        openCommand("Create a new Sketch");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Support = %s",FeatName.c_str(),supportString.c_str());
        doCommand(Doc,"App.activeDocument().%s.Placement = App.Placement(App.Vector(%f,%f,%f),App.Rotation(%f,%f,%f,%f))",FeatName.c_str(),p.x,p.y,p.z,r[0],r[1],r[2],r[3]);
        doCommand(Doc,"App.activeDocument().%s.insertFeature(App.activeDocument().%s)",
                       pcActiveBody->getNameInDocument(), FeatName.c_str());
        //doCommand(Gui,"Gui.activeDocument().activeView().setCamera('%s')",cam.c_str());
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
// Common utility functions for SketchBased features
//===========================================================================

// Take a list of Part2DObjects and erase those which are not eligible for creating a
// SketchBased feature.
 const unsigned validateSketches(std::vector<App::DocumentObject*>& sketches,
                                 std::vector<PartDesignGui::FeaturePickDialog::featureStatus>& status,
                                 std::vector<App::DocumentObject*>::iterator& firstValidSketch)
{    
    // TODO: If the user previously opted to allow multiple use of sketches or use of sketches from other bodies,
    // then count these as valid sketches!
    unsigned validSketches = 0;
    firstValidSketch = sketches.end();

    for (std::vector<App::DocumentObject*>::iterator s = sketches.begin(); s != sketches.end(); s++) {
        //Base::Console().Error("Checking sketch %s\n", (*s)->getNameInDocument());
        // Check whether this sketch is already being used by another feature
        // Body features don't count...
        std::vector<App::DocumentObject*> inList = (*s)->getInList();
        std::vector<App::DocumentObject*>::iterator o = inList.begin();
        while (o != inList.end()) {
            //Base::Console().Error("Inlist: %s\n", (*o)->getNameInDocument());
            if ((*o)->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId()))
                o = inList.erase(o);
            else
                ++o;
        }
        if (inList.size() > 0) {
            status.push_back(PartDesignGui::FeaturePickDialog::isUsed);
            continue;
        }

        // Check whether this sketch belongs to the active body
        PartDesign::Body* body = PartDesignGui::getBody();
        if (!body->hasFeature(*s)) {
            status.push_back(PartDesignGui::FeaturePickDialog::otherBody);
            continue;
        }

        // Check whether the sketch shape is valid
        Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(*s);
        const TopoDS_Shape& shape = sketch->Shape.getValue();
        if (shape.IsNull()) {
            status.push_back(PartDesignGui::FeaturePickDialog::invalidShape);
            continue;
        }

        // count free wires
        int ctWires=0;
        TopExp_Explorer ex;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
            ctWires++;
        }
        if (ctWires == 0) {
            status.push_back(PartDesignGui::FeaturePickDialog::noWire);
            continue;
        }

        // All checks passed - found a valid sketch
        if (firstValidSketch == sketches.end())
            firstValidSketch = s;
        validSketches++;
        status.push_back(PartDesignGui::FeaturePickDialog::validFeature);
    }

    return validSketches;
}

void prepareSketchBased(Gui::Command* cmd, const std::string& which, Part::Part2DObject*& sketch, std::string& FeatName)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if (!pcActiveBody) return;

    // Get a valid sketch from the user
    // First check selections
    FeatName = ""; // Empty string means prepareSketchBased() was not successful
    std::vector<PartDesignGui::FeaturePickDialog::featureStatus> status;
    std::vector<App::DocumentObject*>::iterator firstValidSketch;
    std::vector<App::DocumentObject*> sketches = cmd->getSelection().getObjectsOfType(Part::Part2DObject::getClassTypeId());
    // Next let the user choose from a list of all eligible objects    
    unsigned validSketches = validateSketches(sketches, status, firstValidSketch);
    if (validSketches == 0) {
        status.clear();
        sketches = cmd->getDocument()->getObjectsOfType(Part::Part2DObject::getClassTypeId());
        validSketches = validateSketches(sketches, status, firstValidSketch);
        if (validSketches == 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid sketches in this document"),
                QObject::tr("Please create a sketch or 2D object first."));
            return;
        }
    }
    // If there is more than one selection/possibility, show dialog and let user pick sketch
    if (validSketches > 1) {
        PartDesignGui::FeaturePickDialog Dlg(sketches, status);
        if ((Dlg.exec() != QDialog::Accepted) || (sketches = Dlg.getFeatures()).empty())
            return; // Cancelled or nothing selected
        firstValidSketch = sketches.begin();
    }

    sketch = static_cast<Part::Part2DObject*>(*firstValidSketch);

    FeatName = cmd->getUniqueObjectName(which.c_str());

    cmd->openCommand((std::string("Make ") + which).c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().addObject(\"PartDesign::%s\",\"%s\")",
                   which.c_str(), FeatName.c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.Sketch = App.activeDocument().%s",
                   FeatName.c_str(), sketch->getNameInDocument());
    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.insertFeature(App.activeDocument().%s)",
                   pcActiveBody->getNameInDocument(), FeatName.c_str());
}

void finishSketchBased(const Gui::Command* cmd, const Part::Part2DObject* sketch, const std::string& FeatName)
{
    App::DocumentObjectGroup* grp = sketch->getGroup();
    if (grp) {
        cmd->doCommand(cmd->Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
        cmd->doCommand(cmd->Doc,"App.activeDocument().%s.removeObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),sketch->getNameInDocument());
    }

    cmd->updateActive();
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if (cmd->isActiveObjectValid()) {
        cmd->doCommand(cmd->Gui,"Gui.activeDocument().hide(\"%s\")", sketch->getNameInDocument());
        if (pcActiveBody != NULL) {
            App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature(NULL, false);
            if (prevSolidFeature != NULL)
                cmd->doCommand(cmd->Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
        }
    }
    // #0001721: use '0' as edit value to avoid switching off selection in
    // ViewProviderGeometryObject::setEditViewer
    cmd->doCommand(cmd->Gui,"Gui.activeDocument().setEdit('%s', 0)", FeatName.c_str());
    cmd->doCommand(cmd->Gui,"Gui.Selection.clearSelection()");
    cmd->doCommand(cmd->Gui,"Gui.Selection.addSelection(App.ActiveDocument.ActiveObject)");

    if (pcActiveBody) {
        cmd->copyVisual(FeatName.c_str(), "ShapeColor", pcActiveBody->getNameInDocument());
        cmd->copyVisual(FeatName.c_str(), "LineColor", pcActiveBody->getNameInDocument());
        cmd->copyVisual(FeatName.c_str(), "PointColor", pcActiveBody->getNameInDocument());
    }
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
    sWhatsThis    = "PartDesign_Pad";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pad";
}

void CmdPartDesignPad::activated(int iMsg)
{
    Part::Part2DObject* sketch;
    std::string FeatName;
    prepareSketchBased(this, "Pad", sketch, FeatName);
    if (FeatName.empty()) return;

    // specific parameters for Pad
    doCommand(Doc,"App.activeDocument().%s.Length = 10.0",FeatName.c_str());

    finishSketchBased(this, sketch, FeatName);
    adjustCameraPosition();
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
    sToolTipText  = QT_TR_NOOP("Create a pocket with the selected sketch");
    sWhatsThis    = "PartDesign_Pocket";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Pocket";
}

void CmdPartDesignPocket::activated(int iMsg)
{
    Part::Part2DObject* sketch;
    std::string FeatName;
    prepareSketchBased(this, "Pocket", sketch, FeatName);
    if (FeatName.empty()) return;

    doCommand(Doc,"App.activeDocument().%s.Length = 5.0",FeatName.c_str());

    finishSketchBased(this, sketch, FeatName);
    adjustCameraPosition();
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
    sWhatsThis    = "PartDesign_Revolution";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Revolution";
}

void CmdPartDesignRevolution::activated(int iMsg)
{
    Part::Part2DObject* sketch;
    std::string FeatName;
    prepareSketchBased(this, "Revolution", sketch, FeatName);
    if (FeatName.empty()) return;

    doCommand(Doc,"App.activeDocument().%s.ReferenceAxis = (App.activeDocument().%s,['V_Axis'])",
                                                                             FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Angle = 360.0",FeatName.c_str());
    PartDesign::Revolution* pcRevolution = static_cast<PartDesign::Revolution*>(getDocument()->getObject(FeatName.c_str()));
    if (pcRevolution && pcRevolution->suggestReversed())
        doCommand(Doc,"App.activeDocument().%s.Reversed = 1",FeatName.c_str());

    finishSketchBased(this, sketch, FeatName);
    adjustCameraPosition();
}

bool CmdPartDesignRevolution::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Groove
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignGroove);

CmdPartDesignGroove::CmdPartDesignGroove()
  : Command("PartDesign_Groove")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Groove");
    sToolTipText  = QT_TR_NOOP("Groove a selected sketch");
    sWhatsThis    = "PartDesign_Groove";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Groove";
}

void CmdPartDesignGroove::activated(int iMsg)
{
    Part::Part2DObject* sketch;
    std::string FeatName;
    prepareSketchBased(this, "Groove", sketch, FeatName);
    if (FeatName.empty()) return;

    doCommand(Doc,"App.activeDocument().%s.ReferenceAxis = (App.activeDocument().%s,['V_Axis'])",
                                                                             FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Angle = 360.0",FeatName.c_str());
    PartDesign::Groove* pcGroove = static_cast<PartDesign::Groove*>(getDocument()->getObject(FeatName.c_str()));
    if (pcGroove && pcGroove->suggestReversed())
        doCommand(Doc,"App.activeDocument().%s.Reversed = 1",FeatName.c_str());

    finishSketchBased(this, sketch, FeatName);
    adjustCameraPosition();
}

bool CmdPartDesignGroove::isActive(void)
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
    sWhatsThis    = "PartDesign_Fillet";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Fillet";
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
            QObject::tr("Fillet works only on parts."));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    const Part::TopoShape& TopShape = base->Shape.getShape();
    if (TopShape._Shape.IsNull()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Shape of selected part is empty."));
        return;
    }

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(TopShape._Shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(TopShape._Shape, TopAbs_EDGE, mapOfEdges);

    std::vector<std::string> SubNames = std::vector<std::string>(selection[0].getSubNames());

    unsigned int i = 0;

    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) == "Edge") {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if(los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        else if(aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));

            TopTools_IndexedMapOfShape mapOfFaces;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfFaces);

            for(int j = 1; j <= mapOfFaces.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfFaces.FindKey(j));

                int id = mapOfEdges.FindIndex(edge);

                std::stringstream buf;
                buf << "Edge";
                buf << id;

                if(std::find(SubNames.begin(),SubNames.end(),buf.str()) == SubNames.end())
                {
                    SubNames.push_back(buf.str());
                }

            }

            SubNames.erase(SubNames.begin()+i);
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }

    if (SubNames.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("No fillet possible on selected faces/edges."));
        return;
    }

    std::string SelString;
    SelString += "(App.";
    SelString += "ActiveDocument";//getObject()->getDocument()->getName();
    SelString += ".";
    SelString += selection[0].getFeatName();
    SelString += ",[";
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        SelString += "\"";
        SelString += *it;
        SelString += "\"";
        if(it != --SubNames.end())
            SelString += ",";
    }
    SelString += "])";

    std::string FeatName = getUniqueObjectName("Fillet");

    openCommand("Make Fillet");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Fillet\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Gui,"Gui.Selection.clearSelection()");
    doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",selection[0].getFeatName());
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    App::DocumentObjectGroup* grp = base->getGroup();
    if (grp) {
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
    }

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
    sWhatsThis    = "PartDesign_Chamfer";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Chamfer";
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
            QObject::tr("Chamfer works only on parts."));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    const Part::TopoShape& TopShape = base->Shape.getShape();

    if (TopShape._Shape.IsNull()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Shape of selected part is empty."));
        return;
    }

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(TopShape._Shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(TopShape._Shape, TopAbs_EDGE, mapOfEdges);

    std::vector<std::string> SubNames = std::vector<std::string>(selection[0].getSubNames());

    unsigned int i = 0;

    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.size() > 4 && aSubName.substr(0,4) == "Edge") {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if(los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        else if(aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));

            TopTools_IndexedMapOfShape mapOfFaces;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfFaces);

            for(int j = 1; j <= mapOfFaces.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfFaces.FindKey(j));

                int id = mapOfEdges.FindIndex(edge);

                std::stringstream buf;
                buf << "Edge";
                buf << id;

                if(std::find(SubNames.begin(),SubNames.end(),buf.str()) == SubNames.end())
                {
                    SubNames.push_back(buf.str());
                }

            }

            SubNames.erase(SubNames.begin()+i);
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }

    if (SubNames.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("No chamfer possible on selected faces/edges."));
        return;
    }

    std::string SelString;
    SelString += "(App.";
    SelString += "ActiveDocument";//getObject()->getDocument()->getName();
    SelString += ".";
    SelString += selection[0].getFeatName();
    SelString += ",[";
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        SelString += "\"";
        SelString += *it;
        SelString += "\"";
        if(it != --SubNames.end())
            SelString += ",";
    }
    SelString += "])";

    std::string FeatName = getUniqueObjectName("Chamfer");

    openCommand("Make Chamfer");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Chamfer\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Gui,"Gui.Selection.clearSelection()");
    doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",selection[0].getFeatName());
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    App::DocumentObjectGroup* grp = base->getGroup();
    if (grp) {
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "LineColor",  selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "PointColor", selection[0].getFeatName());
}

bool CmdPartDesignChamfer::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Draft
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignDraft);

CmdPartDesignDraft::CmdPartDesignDraft()
  :Command("PartDesign_Draft")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Draft");
    sToolTipText  = QT_TR_NOOP("Make a draft on a face");
    sWhatsThis    = "PartDesign_Draft";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Draft";
}

void CmdPartDesignDraft::activated(int iMsg)
{
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.size() < 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more faces."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("Draft works only on parts."));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    const Part::TopoShape& TopShape = base->Shape.getShape();
    if (TopShape._Shape.IsNull()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Shape of selected Part is empty."));
        return;
    }

    std::vector<std::string> SubNames = std::vector<std::string>(selection[0].getSubNames());
    unsigned int i = 0;

    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if(aSubName.size() > 4 && aSubName.substr(0,4) == "Face") {
            // Check for valid face types
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));
            BRepAdaptor_Surface sf(face);
            if ((sf.GetType() != GeomAbs_Plane) && (sf.GetType() != GeomAbs_Cylinder) && (sf.GetType() != GeomAbs_Cone))
                SubNames.erase(SubNames.begin()+i);
        } else {
            // empty name or any other sub-element
            SubNames.erase(SubNames.begin()+i);
        }

        i++;
    }

    if (SubNames.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("No draft possible on selected faces."));
        return;
    }

    std::string SelString;
    SelString += "(App.";
    SelString += "ActiveDocument";
    SelString += ".";
    SelString += selection[0].getFeatName();
    SelString += ",[";
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        SelString += "\"";
        SelString += *it;
        SelString += "\"";
        if(it != --SubNames.end())
            SelString += ",";
    }
    SelString += "])";

    std::string FeatName = getUniqueObjectName("Draft");

    // We don't create any defaults for neutral plane and pull direction, but Draft::execute()
    // will choose them.
    // Note: When the body feature is there, the best thing would be to get pull direction and
    // neutral plane from the preceding feature in the tree. Or even store them as default in
    // the Body feature itself
    openCommand("Make Draft");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Draft\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Doc,"App.activeDocument().%s.Angle = %f",FeatName.c_str(), 1.5);
    updateActive();
    if (isActiveObjectValid()) {
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",selection[0].getFeatName());
    }
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    App::DocumentObjectGroup* grp = base->getGroup();
    if (grp) {
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "LineColor",  selection[0].getFeatName());
    copyVisual(FeatName.c_str(), "PointColor", selection[0].getFeatName());
}

bool CmdPartDesignDraft::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Common functions for all Transformed features
//===========================================================================

void prepareTransformed(Gui::Command* cmd, const std::string& which,
                        std::vector<App::DocumentObject*>& features, std::string& FeatName,
                        std::vector<std::string>& selList, std::string& selNames)
{
    // Get a valid original from the user
    // First check selections
    features = cmd->getSelection().getObjectsOfType(PartDesign::Additive::getClassTypeId());
    std::vector<App::DocumentObject*> subtractive = cmd->getSelection().getObjectsOfType(PartDesign::Subtractive::getClassTypeId());
    features.insert(features.end(), subtractive.begin(), subtractive.end());
    // Next create a list of all eligible objects
    if (features.size() == 0) {
        features = cmd->getDocument()->getObjectsOfType(PartDesign::Additive::getClassTypeId());
        subtractive = cmd->getDocument()->getObjectsOfType(PartDesign::Subtractive::getClassTypeId());
        features.insert(features.end(), subtractive.begin(), subtractive.end());
        // If there is more than one selected or eligible object, show dialog and let user pick one
        if (features.size() > 1) {
            std::vector<PartDesignGui::FeaturePickDialog::featureStatus> status;
            for (unsigned i = 0; i < features.size(); i++)
                status.push_back(PartDesignGui::FeaturePickDialog::validFeature);
            PartDesignGui::FeaturePickDialog Dlg(features, status);
            if ((Dlg.exec() != QDialog::Accepted) || (features = Dlg.getFeatures()).empty()) {
                features.clear();
                return; // Cancelled or nothing selected
            }
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid features in this document"),
                QObject::tr("Please create a subtractive or additive feature first."));
            return;
        }
    }

    FeatName = cmd->getUniqueObjectName("Mirrored");

    std::stringstream str;
    str << "App.activeDocument()." << FeatName << ".Originals = [";
    for (std::vector<App::DocumentObject*>::iterator it = features.begin(); it != features.end(); ++it){
        str << "App.activeDocument()." << (*it)->getNameInDocument() << ",";
        selList.push_back((*it)->getNameInDocument());
    }
    str << "]";
    selNames = str.str();
}

//===========================================================================
// PartDesign_Mirrored
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMirrored);

CmdPartDesignMirrored::CmdPartDesignMirrored()
  : Command("PartDesign_Mirrored")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Mirrored");
    sToolTipText  = QT_TR_NOOP("create a mirrored feature");
    sWhatsThis    = "PartDesign_Mirrored";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Mirrored";
}

void CmdPartDesignMirrored::activated(int iMsg)
{
    std::string FeatName, selNames;
    std::vector<App::DocumentObject*> features;
    std::vector<std::string> selList;
    prepareTransformed(this, "Mirrored", features, FeatName, selList, selNames);
    if (features.empty())
        return;

    openCommand("Mirrored");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Mirrored\",\"%s\")",FeatName.c_str());
    // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
    // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
    updateActive(); // Helps to ensure that the object already exists when the next command comes up
    doCommand(Doc,selNames.c_str());
    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.MirrorPlane = (App.activeDocument().%s, [\"V_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());
    for (std::vector<std::string>::iterator it = selList.begin(); it != selList.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", selList.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", selList.front().c_str());
}

bool CmdPartDesignMirrored::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_LinearPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignLinearPattern);

CmdPartDesignLinearPattern::CmdPartDesignLinearPattern()
  : Command("PartDesign_LinearPattern")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("LinearPattern");
    sToolTipText  = QT_TR_NOOP("Create a linear pattern feature");
    sWhatsThis    = "PartDesign_LinearPattern";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_LinearPattern";
}

void CmdPartDesignLinearPattern::activated(int iMsg)
{
    std::string FeatName, selNames;
    std::vector<App::DocumentObject*> features;
    std::vector<std::string> selList;
    prepareTransformed(this, "LinearPattern", features, FeatName, selList, selNames);
    if (features.empty())
        return;

    openCommand("LinearPattern");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::LinearPattern\",\"%s\")",FeatName.c_str());
    updateActive();
    doCommand(Doc,selNames.c_str());
    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.Direction = (App.activeDocument().%s, [\"H_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Length = 100", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());
    for (std::vector<std::string>::iterator it = selList.begin(); it != selList.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    App::DocumentObjectGroup* grp = sketch->getGroup();
    if (grp) {
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.removeObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),sketch->getNameInDocument());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", selList.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", selList.front().c_str());
}

bool CmdPartDesignLinearPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_PolarPattern
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignPolarPattern);

CmdPartDesignPolarPattern::CmdPartDesignPolarPattern()
  : Command("PartDesign_PolarPattern")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("PolarPattern");
    sToolTipText  = QT_TR_NOOP("Create a polar pattern feature");
    sWhatsThis    = "PartDesign_PolarPattern";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_PolarPattern";
}

void CmdPartDesignPolarPattern::activated(int iMsg)
{
    std::string FeatName, selNames;
    std::vector<App::DocumentObject*> features;
    std::vector<std::string> selList;
    prepareTransformed(this, "PolarPattern", features, FeatName, selList, selNames);
    if (features.empty())
        return;

    openCommand("PolarPattern");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::PolarPattern\",\"%s\")",FeatName.c_str());
    updateActive();
    doCommand(Doc,selNames.c_str());
    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.Axis = (App.activeDocument().%s, [\"N_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Angle = 360", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());
    for (std::vector<std::string>::iterator it = selList.begin(); it != selList.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    App::DocumentObjectGroup* grp = sketch->getGroup();
    if (grp) {
        doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.removeObject(App.activeDocument().%s)"
                     ,grp->getNameInDocument(),sketch->getNameInDocument());
    }

    copyVisual(FeatName.c_str(), "ShapeColor", selList.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", selList.front().c_str());
}

bool CmdPartDesignPolarPattern::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Scaled
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignScaled);

CmdPartDesignScaled::CmdPartDesignScaled()
  : Command("PartDesign_Scaled")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("Scaled");
    sToolTipText  = QT_TR_NOOP("Create a scaled feature");
    sWhatsThis    = "PartDesign_Scaled";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_Scaled";
}

void CmdPartDesignScaled::activated(int iMsg)
{
    std::string FeatName, selNames;
    std::vector<App::DocumentObject*> features;
    std::vector<std::string> selList;
    prepareTransformed(this, "Scaled", features, FeatName, selList, selNames);
    if (features.empty())
        return;

    openCommand("Scaled");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::Scaled\",\"%s\")",FeatName.c_str());
    updateActive();
    doCommand(Doc,selNames.c_str());
    doCommand(Doc,"App.activeDocument().%s.Factor = 2", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());
    for (std::vector<std::string>::iterator it = selList.begin(); it != selList.end(); ++it)
        doCommand(Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", selList.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", selList.front().c_str());
}

bool CmdPartDesignScaled::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_MultiTransform
//===========================================================================
DEF_STD_CMD_A(CmdPartDesignMultiTransform);

CmdPartDesignMultiTransform::CmdPartDesignMultiTransform()
  : Command("PartDesign_MultiTransform")
{
    sAppModule    = "PartDesign";
    sGroup        = QT_TR_NOOP("PartDesign");
    sMenuText     = QT_TR_NOOP("MultiTransform");
    sToolTipText  = QT_TR_NOOP("Create a multitransform feature");
    sWhatsThis    = "PartDesign_MultiTransform";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_MultiTransform";
}

void CmdPartDesignMultiTransform::activated(int iMsg)
{
    std::string FeatName, selNames;
    std::vector<App::DocumentObject*> features;
    std::vector<std::string> selList;
    prepareTransformed(this, "MultiTransform", features, FeatName, selList, selNames);
    if (features.empty())
        return;

    openCommand("MultiTransform");
    doCommand(Doc,"App.activeDocument().addObject(\"PartDesign::MultiTransform\",\"%s\")",FeatName.c_str());
    updateActive();
    doCommand(Doc,selNames.c_str());

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    copyVisual(FeatName.c_str(), "ShapeColor", selList.front().c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", selList.front().c_str());
}

bool CmdPartDesignMultiTransform::isActive(void)
{
    return hasActiveDocument();
}


//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignBody());
    rcCmdMgr.addCommand(new CmdPartDesignPad());
    rcCmdMgr.addCommand(new CmdPartDesignPocket());
    rcCmdMgr.addCommand(new CmdPartDesignRevolution());
    rcCmdMgr.addCommand(new CmdPartDesignGroove());
    rcCmdMgr.addCommand(new CmdPartDesignFillet());
    rcCmdMgr.addCommand(new CmdPartDesignDraft());
    rcCmdMgr.addCommand(new CmdPartDesignNewSketch());
    rcCmdMgr.addCommand(new CmdPartDesignChamfer());
    rcCmdMgr.addCommand(new CmdPartDesignMirrored());
    rcCmdMgr.addCommand(new CmdPartDesignLinearPattern());
    rcCmdMgr.addCommand(new CmdPartDesignPolarPattern());
    //rcCmdMgr.addCommand(new CmdPartDesignScaled());
    rcCmdMgr.addCommand(new CmdPartDesignMultiTransform());
    rcCmdMgr.addCommand(new CmdPartDesignMoveTip());
 }
