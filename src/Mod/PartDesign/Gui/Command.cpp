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
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Gui/Tree.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Mod/Part/App/Part2DObject.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/Gui/SketchOrientationDialog.h>
#include <Mod/PartDesign/App/FeatureAdditive.h>
#include <Mod/PartDesign/App/FeatureSubtractive.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include "Workbench.h"
#include "FeaturePickDialog.h"
#include "Workbench.h"

using namespace std;

#include "ReferenceSelection.h"

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
    sPixmap       = "PartDesign_Body_Create_New";
}

void CmdPartDesignBody::activated(int iMsg)
{
    openCommand("Add a body feature");
    std::string FeatName = getUniqueObjectName("Body");

	// first check if Part is already created:
	App::Part *actPart =  getDocument()->Tip.getValue<App::Part *>();
	std::string PartName;
	if(!actPart){
		// if not, creating a part and set it up by calling the appropiated function in Workbench
		PartName = getUniqueObjectName("Part");
		doCommand(Doc,"App.activeDocument().Tip = App.activeDocument().addObject('App::Part','%s')",PartName.c_str());
		doCommand(Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr(PartName.c_str()).toStdString().c_str());
		PartDesignGui::Workbench::setUpPart(dynamic_cast<App::Part *>(getDocument()->getObject(PartName.c_str())));
	}else
		PartName = actPart->getNameInDocument();

    // add the Body feature itself, and make it active
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Body','%s')",FeatName.c_str());
    //doCommand(Doc,"App.activeDocument().%s.Model = []",FeatName.c_str());
    //doCommand(Doc,"App.activeDocument().%s.Tip = None",FeatName.c_str());
    addModule(Gui,"PartDesignGui"); // import the Gui module only once a session
    doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PDBODYKEY, FeatName.c_str());
    // Make the "Create sketch" prompt appear in the task panel
    doCommand(Gui,"Gui.Selection.clearSelection()");
    doCommand(Gui,"Gui.Selection.addSelection(App.ActiveDocument.%s)", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.ActiveDocument.%s)",PartName.c_str(),FeatName.c_str());

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
    App::DocumentObject* selFeature;

    if (features.empty()) {
        // Insert at the beginning of this body
        selFeature = NULL;
    } else {
        selFeature = features.front();
        if (selFeature->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId())) {
            // Insert at the beginning of this body
            selFeature = NULL;
        } else if (!pcActiveBody->hasFeature(selFeature)) {
            // Switch to other body
            pcActiveBody = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(selFeature));
            if (pcActiveBody != NULL)
                Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s',App.activeDocument().%s)",
                                        PDBODYKEY, pcActiveBody->getNameInDocument());
            else
                return;
        }
    }

    openCommand("Move insert point to selected feature");
    App::DocumentObject* oldTip = pcActiveBody->Tip.getValue();
    if (oldTip != NULL) {
        if (!oldTip->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", oldTip->getNameInDocument());
        App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature();
        if (prevSolidFeature != NULL)
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
    }

    if (selFeature == NULL) {
        doCommand(Doc,"App.activeDocument().%s.Tip = None", pcActiveBody->getNameInDocument());
    } else {
        doCommand(Doc,"App.activeDocument().%s.Tip = App.activeDocument().%s",pcActiveBody->getNameInDocument(), selFeature->getNameInDocument());

        // Adjust visibility to show only the Tip feature and (if the Tip feature is not solid) the solid feature prior to the Tip
        doCommand(Gui,"Gui.activeDocument().show(\"%s\")", selFeature->getNameInDocument());
        App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature();
        if ((prevSolidFeature != NULL) && !PartDesign::Body::isSolidFeature(selFeature))
            doCommand(Gui,"Gui.activeDocument().show(\"%s\")", prevSolidFeature->getNameInDocument());
    }

    // TOOD: Hide all datum features after the Tip feature? But the user might have already hidden some and wants to see
    // others, so we would have to remember their state somehow
}

bool CmdPartDesignMoveTip::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_DuplicateSelection
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignDuplicateSelection);

CmdPartDesignDuplicateSelection::CmdPartDesignDuplicateSelection()
  :Command("PartDesign_DuplicateSelection")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Duplicate selected object");
    sToolTipText    = QT_TR_NOOP("Duplicates the selected object and adds it to the active body");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignDuplicateSelection::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;
    App::DocumentObject* selFeature = features.front();

    if (!pcActiveBody->hasFeature(selFeature)) {
        // NOTE: We assume all selected features will be in the same document
        // Switch to other body
        pcActiveBody = static_cast<PartDesign::Body*>(Part::BodyBase::findBodyOf(selFeature));
        if (pcActiveBody != NULL)
            Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
                                    PDBODYKEY, pcActiveBody->getNameInDocument());
        else
            return;
    }

    std::vector<App::DocumentObject*> beforeFeatures = getDocument()->getObjects();

    openCommand("Duplicate a PartDesign object");
    doCommand(Doc,"FreeCADGui.runCommand('Std_DuplicateSelection')");

    // Find the features that were added
    std::vector<App::DocumentObject*> afterFeatures = getDocument()->getObjects();
    std::vector<App::DocumentObject*> newFeatures;
    std::set_difference(afterFeatures.begin(), afterFeatures.end(), beforeFeatures.begin(), beforeFeatures.end(),
                        std::back_inserter(newFeatures));

    for (std::vector<App::DocumentObject*>::const_iterator f = newFeatures.begin(); f != newFeatures.end(); f++) {
        if (PartDesign::Body::isAllowed(*f)) {
            doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      pcActiveBody->getNameInDocument(), (*f)->getNameInDocument());
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", (*f)->getNameInDocument());
        }
    }

    // Adjust visibility of features
    doCommand(Gui,"Gui.activeDocument().show(\"%s\")", newFeatures.back()->getNameInDocument());
    App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature();
    if ((prevSolidFeature != NULL) && !PartDesign::Body::isSolidFeature(selFeature))
        doCommand(Gui,"Gui.activeDocument().show(\"%s\")", prevSolidFeature->getNameInDocument());
}

bool CmdPartDesignDuplicateSelection::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// PartDesign_MoveFeature
//===========================================================================

DEF_STD_CMD_A(CmdPartDesignMoveFeature);

CmdPartDesignMoveFeature::CmdPartDesignMoveFeature()
  :Command("PartDesign_MoveFeature")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Move object to other body");
    sToolTipText    = QT_TR_NOOP("Moves the selected object to another body");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignMoveFeature::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;

    // Create a list of all bodies in this part
    std::vector<App::DocumentObject*> bodies = getDocument()->getObjectsOfType(Part::BodyBase::getClassTypeId());

    // Ask user to select the target body
    bool ok;
    QStringList items;
    for (std::vector<App::DocumentObject*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
        items.push_back(QString::fromUtf8((*it)->Label.getValue()));
    QString text = QInputDialog::getItem(Gui::getMainWindow(),
        qApp->translate(className(), "Select body"),
        qApp->translate(className(), "Select a body from the list"),
        items, 0, false, &ok);
    if (!ok) return;
    int index = items.indexOf(text);

    PartDesign::Body* target = static_cast<PartDesign::Body*>(bodies[index]);

    openCommand("Move an object");

    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
        // Find body of this feature
        Part::BodyBase* source = PartDesign::Body::findBodyOf(*f);
        if (source == target) continue;
        bool featureIsTip = (source->Tip.getValue() == *f);

        // Remove from source body
        doCommand(Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                      source->getNameInDocument(), (*f)->getNameInDocument());
        // Add to target body (always at the Tip)
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      target->getNameInDocument(), (*f)->getNameInDocument());
        // Recompute to update the shape
        doCommand(Gui,"App.activeDocument().recompute()");

        // Adjust visibility of features
        if (PartDesign::Body::isSolidFeature(*f)) {
            // If we removed the tip of the source body, make the new tip visible
            if (featureIsTip) {
                App::DocumentObject* prevSolidFeature = source->getPrevSolidFeature();
                doCommand(Gui,"Gui.activeDocument().show(\"%s\")", prevSolidFeature->getNameInDocument());
            }

            // Hide old tip and show new tip (the moved feature) of the target body
            App::DocumentObject* prevSolidFeature = target->getPrevSolidFeature();
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
            doCommand(Gui,"Gui.activeDocument().show(\"%s\")", (*f)->getNameInDocument());
        }
    }
}

bool CmdPartDesignMoveFeature::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignMoveFeatureInTree);

CmdPartDesignMoveFeatureInTree::CmdPartDesignMoveFeatureInTree()
  :Command("PartDesign_MoveFeatureInTree")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Move object after other object");
    sToolTipText    = QT_TR_NOOP("Moves the selected object and insert it after another object");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "";
}

void CmdPartDesignMoveFeatureInTree::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if(!pcActiveBody) return;

    std::vector<App::DocumentObject*> features = getSelection().getObjectsOfType(Part::Feature::getClassTypeId());
    if (features.empty()) return;

    // Create a list of all features in this body
    std::vector<App::DocumentObject*> model = pcActiveBody->Model.getValues();

    // Ask user to select the target feature
    bool ok;
    QStringList items;
    for (std::vector<App::DocumentObject*>::iterator it = model.begin(); it != model.end(); ++it)
        items.push_back(QString::fromUtf8((*it)->Label.getValue()));
    QString text = QInputDialog::getItem(Gui::getMainWindow(),
        qApp->translate(className(), "Select feature"),
        qApp->translate(className(), "Select a feature from the list"),
        items, 0, false, &ok);
    if (!ok) return;
    int index = items.indexOf(text);
    PartDesign::Feature* target = static_cast<PartDesign::Feature*>(model[index]);

    openCommand("Move an object inside tree");

    // Set insert point at the selected feature
    App::DocumentObject* oldTip = pcActiveBody->Tip.getValue();
    Gui::Selection().clearSelection();
    if (target != NULL)
        Gui::Selection().addSelection(target->getDocument()->getName(), target->getNameInDocument());
    Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");

    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
        if (*f == target) continue;

        // Remove and re-insert the feature from the Body
        // Note: If the tip was moved then the new tip will be at the moved position, that is, at the same
        // feature as before!
        doCommand(Doc,"App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                      pcActiveBody->getNameInDocument(), (*f)->getNameInDocument());
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                      pcActiveBody->getNameInDocument(), (*f)->getNameInDocument());
    }

    // Recompute to update the shape
    doCommand(Gui,"App.activeDocument().recompute()");
    // Set insert point where it was before
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
    Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
    Gui::Selection().clearSelection();
}

bool CmdPartDesignMoveFeatureInTree::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// PartDesign_Datum
//===========================================================================

const QString getReferenceString(Gui::Command* cmd)
{
    QString referenceString;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if(!pcActiveBody) return QString::fromAscii("");

    Gui::SelectionFilter GeometryFilter("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    Gui::SelectionFilter DatumFilter   ("SELECT PartDesign::Plane COUNT 1");
    Gui::SelectionFilter EdgeFilter    ("SELECT Part::Feature SUBELEMENT Edge COUNT 1");
    Gui::SelectionFilter LineFilter    ("SELECT PartDesign::Line COUNT 1");
    Gui::SelectionFilter VertexFilter  ("SELECT Part::Feature SUBELEMENT Vertex COUNT 1");
    Gui::SelectionFilter PointFilter   ("SELECT PartDesign::Point COUNT 1");
    Gui::SelectionFilter PlaneFilter   ("SELECT App::Plane COUNT 1");


    if (EdgeFilter.match())
        GeometryFilter = EdgeFilter;
    else if (VertexFilter.match())
        GeometryFilter = VertexFilter;

    if (LineFilter.match())
        DatumFilter = LineFilter;
    else if (PointFilter.match())
        DatumFilter = PointFilter;
    else if (PlaneFilter.match())
        DatumFilter = PlaneFilter;

    if (GeometryFilter.match() || DatumFilter.match()) {
        // get the selected object
        if (GeometryFilter.match()) {
            Part::Feature *part = static_cast<Part::Feature*>(GeometryFilter.Result[0][0].getObject());
            // FIXME: Reject or warn about feature that is outside of active body, and feature
            // that comes after the current insert point (Tip)
            const std::vector<std::string> &sub = GeometryFilter.Result[0][0].getSubNames();
            referenceString = QString::fromAscii("[");

            for (int r = 0; r != sub.size(); r++) {
                // get the selected sub shape
                const Part::TopoShape &shape = part->Shape.getValue();
                TopoDS_Shape sh = shape.getSubShape(sub[r].c_str());
                if (!sh.IsNull()) {
                    referenceString += QString::fromAscii(r == 0 ? "" : ",") +
                                QString::fromAscii("(App.activeDocument().") + QString::fromAscii(part->getNameInDocument()) +
                                QString::fromAscii(",'") + QString::fromStdString(sub[r]) + QString::fromAscii("')");
                }
            }

            referenceString += QString::fromAscii("]");
            if (referenceString.length() == 2) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No sub shape selected"),
                    QObject::tr("You have to select a face, edge, vertex or plane to define a datum feature!"));
                return QString::fromAscii("");
            }

            return referenceString;
        } else {
            Part::Feature *part = static_cast<Part::Feature*>(DatumFilter.Result[0][0].getObject());
            return QString::fromAscii("[(App.activeDocument().") + QString::fromAscii(part->getNameInDocument()) +
                    QString::fromAscii(",'')]");
        }
    }
    else {
        // Get a valid datum feature from the user
        std::vector<PartDesignGui::FeaturePickDialog::featureStatus> status;
        std::vector<App::DocumentObject*> refs = cmd->getDocument()->getObjectsOfType(App::Plane::getClassTypeId());
        std::vector<App::DocumentObject*> refstmp = cmd->getDocument()->getObjectsOfType(PartDesign::Plane::getClassTypeId());
        refs.insert(refs.end(), refstmp.begin(), refstmp.end());
        refstmp = cmd->getDocument()->getObjectsOfType(PartDesign::Line::getClassTypeId());
        refs.insert(refs.end(), refstmp.begin(), refstmp.end());
        refstmp = cmd->getDocument()->getObjectsOfType(PartDesign::Point::getClassTypeId());
        refs.insert(refs.end(), refstmp.begin(), refstmp.end());

        unsigned validRefs = 0;
        std::vector<App::DocumentObject*> chosenRefs;

        for (std::vector<App::DocumentObject*>::iterator r = refs.begin(); r != refs.end(); r++) {
            // Check whether this reference is a base plane
            bool base = false;
            for (unsigned i = 0; i < 3; i++) {
                if (strcmp(App::Part::BaseplaneTypes[i], (*r)->getNameInDocument()) == 0) {
                    status.push_back(PartDesignGui::FeaturePickDialog::basePlane);
                    if (chosenRefs.empty())
                        chosenRefs.push_back(*r);
                    validRefs++;
                    base = true;
                    break;
                }
            }
            if (base) continue;

            // Check whether this reference belongs to the active body
            PartDesign::Body* body = PartDesignGui::getBody();
            if (!body->hasFeature(*r)) {
                status.push_back(PartDesignGui::FeaturePickDialog::otherBody);
                continue;
            } else if (body->isAfterTip(*r)) {
                status.push_back(PartDesignGui::FeaturePickDialog::afterTip);
                continue;
            }

            // All checks passed - found a valid reference
            if (chosenRefs.empty())
                chosenRefs.push_back(*r);
            validRefs++;
            status.push_back(PartDesignGui::FeaturePickDialog::validFeature);
        }

        if (validRefs == 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid references in this document"),
                QObject::tr("Please select a datum feature, or a face, edge or vertex"));
            return QString::fromAscii("");
        }

        // If there is more than one possibility, show dialog and let user pick references
        if (validRefs > 1) {
            PartDesignGui::FeaturePickDialog Dlg(refs, status);
            if ((Dlg.exec() != QDialog::Accepted) || (chosenRefs = Dlg.getFeatures()).empty())
                return QString::fromAscii(""); // Cancelled or nothing selected
            if (chosenRefs.size() > 3)
                Base::Console().Warning("You have chosen more than three references for a datum feature. The extra references are being ignored");
        }

        // TODO: Allow user to choose front or back of the plane

        referenceString = QString::fromAscii("[");
        for (int i = 0; i < chosenRefs.size(); i++) {
            referenceString += QString::fromAscii(i == 0 ? "" : ",") +
                    QString::fromAscii("(App.activeDocument().") + QString::fromUtf8(chosenRefs[i]->getNameInDocument()) +
                    QString::fromAscii(",'front')");
        }

        referenceString += QString::fromAscii("]");
        return referenceString;
    }
}

/* Datum feature commands =======================================================*/

DEF_STD_CMD_A(CmdPartDesignPlane);

CmdPartDesignPlane::CmdPartDesignPlane()
  :Command("PartDesign_Plane")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum plane");
    sToolTipText    = QT_TR_NOOP("Create a new datum plane");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Plane";
}

void CmdPartDesignPlane::activated(int iMsg)
{
    // create Datum plane
    std::string FeatName = getUniqueObjectName("DatumPlane");
    QString refStr = getReferenceString(this);
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();

    openCommand("Create a datum plane");
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Plane','%s')",FeatName.c_str());
    if (refStr.length() > 0)
        doCommand(Doc,"App.activeDocument().%s.References = %s",FeatName.c_str(),refStr.toStdString().c_str());
    doCommand(Doc,"App.activeDocument().%s.Offset = 0.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Angle = 0.0",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                   pcActiveBody->getNameInDocument(), FeatName.c_str());
    doCommand(Gui,"App.activeDocument().recompute()");  // recompute the feature based on its references
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

}

bool CmdPartDesignPlane::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignLine);

CmdPartDesignLine::CmdPartDesignLine()
  :Command("PartDesign_Line")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum line");
    sToolTipText    = QT_TR_NOOP("Create a new datum line");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Line";
}

void CmdPartDesignLine::activated(int iMsg)
{
    // create Datum line
    std::string FeatName = getUniqueObjectName("DatumLine");
    QString refStr = getReferenceString(this);
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();

    openCommand("Create a datum line");
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Line','%s')",FeatName.c_str());
    if (refStr.length() > 0)
        doCommand(Doc,"App.activeDocument().%s.References = %s",FeatName.c_str(),refStr.toStdString().c_str());
    doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                   pcActiveBody->getNameInDocument(), FeatName.c_str());
    doCommand(Gui,"App.activeDocument().recompute()");  // recompute the feature based on its references
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

}

bool CmdPartDesignLine::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_A(CmdPartDesignPoint);

CmdPartDesignPoint::CmdPartDesignPoint()
  :Command("PartDesign_Point")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a datum point");
    sToolTipText    = QT_TR_NOOP("Create a new datum point");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Point";
}

void CmdPartDesignPoint::activated(int iMsg)
{
    // create Datum point
    std::string FeatName = getUniqueObjectName("DatumPoint");
    QString refStr = getReferenceString(this);
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();

    openCommand("Create a datum point");
    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Point','%s')",FeatName.c_str());
    if (refStr.length() > 0)
        doCommand(Doc,"App.activeDocument().%s.References = %s",FeatName.c_str(),refStr.toStdString().c_str());
    doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                   pcActiveBody->getNameInDocument(), FeatName.c_str());
    doCommand(Gui,"App.activeDocument().recompute()");  // recompute the feature based on its references
    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

}

bool CmdPartDesignPoint::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
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
    Gui::SelectionFilter PlaneFilter ("SELECT App::Plane COUNT 1");
    Gui::SelectionFilter PlaneFilter2 ("SELECT PartDesign::Plane COUNT 1");
    if (PlaneFilter2.match())
        PlaneFilter = PlaneFilter2;

    if (SketchFilter.match()) {
        Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(SketchFilter.Result[0][0].getObject());
        openCommand("Edit Sketch");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",Sketch->getNameInDocument());
    }
    else if (FaceFilter.match() || PlaneFilter.match()) {
        // get the selected object
        std::string supportString;
        Part::Feature* feat;

        if (FaceFilter.match()) {
            feat = static_cast<Part::Feature*>(FaceFilter.Result[0][0].getObject());
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
            const Part::TopoShape &shape = feat->Shape.getValue();
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
            feat = static_cast<Part::Feature*>(PlaneFilter.Result[0][0].getObject());
            // TODO: Find out whether the user picked front or back of this plane
            supportString = std::string("(App.activeDocument().") + feat->getNameInDocument() + ", ['front'])";
        }

        if (!pcActiveBody->hasFeature(feat)) {
            bool isBasePlane = false;
            for (unsigned i = 0; i < 3; i++) {
                if (strcmp(App::Part::BaseplaneTypes[i], feat->getNameInDocument()) == 0) {
                    isBasePlane = true;
                    break;
                }
            }
            if (!isBasePlane) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection from other body"),
                    QObject::tr("You have to select a face or plane from the active body!"));
                return;
            }
        } else if (pcActiveBody->isAfterTip(feat)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection from inactive feature"),
                QObject::tr("You have to select a face or plane before the current insert point, or move the insert point"));
            return;
        }

        // create Sketch on Face or Plane
        std::string FeatName = getUniqueObjectName("Sketch");

        openCommand("Create a Sketch on Face");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());        
        doCommand(Doc,"App.activeDocument().%s.Support = %s",FeatName.c_str(),supportString.c_str());
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
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
                if (strcmp(App::Part::BaseplaneTypes[i], (*p)->getNameInDocument()) == 0) {
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
            if (!pcActiveBody->hasFeature(*p)) {
                status.push_back(PartDesignGui::FeaturePickDialog::otherBody);
                continue;
            } else {
                if (pcActiveBody->isAfterTip(*p))
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
        bool reversed = false;
        if (validPlanes > 1) {
            PartDesignGui::FeaturePickDialog Dlg(planes, status);
            if ((Dlg.exec() != QDialog::Accepted) || (planes = Dlg.getFeatures()).empty())
                return; // Cancelled or nothing selected
            firstValidPlane = planes.begin();
            reversed = Dlg.getReverse();
        }

        App::Plane* plane = static_cast<App::Plane*>(*firstValidPlane);        
        std::string FeatName = getUniqueObjectName("Sketch");
        std::string supportString = std::string("(App.activeDocument().") + plane->getNameInDocument() +
                                    ", ['" + (reversed ? "back" : "front") + "'])";

        openCommand("Create a new Sketch");
        doCommand(Doc,"App.activeDocument().addObject('Sketcher::SketchObject','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Support = %s",FeatName.c_str(),supportString.c_str());
        updateActive(); // Make sure the Support's Placement property is updated
        doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
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
// Common utility functions for all features creating solids
//===========================================================================

void finishFeature(const Gui::Command* cmd, const std::string& FeatName, const bool hidePrevSolid = true)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();

    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)",
                   pcActiveBody->getNameInDocument(), FeatName.c_str());

    if (cmd->isActiveObjectValid() && (pcActiveBody != NULL)) {
        App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature(NULL, false);
        if (hidePrevSolid && (prevSolidFeature != NULL))
            cmd->doCommand(cmd->Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
    }
    cmd->updateActive();
    // #0001721: use '0' as edit value to avoid switching off selection in
    // ViewProviderGeometryObject::setEditViewer
    cmd->doCommand(cmd->Gui,"Gui.activeDocument().setEdit('%s', 0)", FeatName.c_str());
    cmd->doCommand(cmd->Gui,"Gui.Selection.clearSelection()");
    //cmd->doCommand(cmd->Gui,"Gui.Selection.addSelection(App.ActiveDocument.ActiveObject)");

    if (pcActiveBody) {
        cmd->copyVisual(FeatName.c_str(), "ShapeColor", pcActiveBody->getNameInDocument());
        cmd->copyVisual(FeatName.c_str(), "LineColor", pcActiveBody->getNameInDocument());
        cmd->copyVisual(FeatName.c_str(), "PointColor", pcActiveBody->getNameInDocument());
    }
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
}

void finishSketchBased(const Gui::Command* cmd, const Part::Part2DObject* sketch, const std::string& FeatName)
{
    if (cmd->isActiveObjectValid())
        cmd->doCommand(cmd->Gui,"Gui.activeDocument().hide(\"%s\")", sketch->getNameInDocument());

    finishFeature(cmd, FeatName);
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
// Common utility functions for Dressup features
//===========================================================================

void makeChamferOrFillet(Gui::Command* cmd, const std::string& which)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if (!pcActiveBody) 
        return;

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();

    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face or body. Only one body is allowed."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QString::fromStdString(which) + QObject::tr(" works only on parts."));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    if (base != pcActiveBody->getPrevSolidFeature(NULL, true)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong base feature"),
            QObject::tr("Only the current Tip of the active Body can be selected as the base feature"));
        return;
    }

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
        QString::fromStdString(which) + QObject::tr(" not possible on selected faces/edges."));
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

    std::string FeatName = cmd->getUniqueObjectName(which.c_str());

    cmd->openCommand((std::string("Make ") + which).c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().addObject(\"PartDesign::%s\",\"%s\")",which.c_str(), FeatName.c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().%s.Base = %s",FeatName.c_str(),SelString.c_str());
    doCommand(Gui,"Gui.Selection.clearSelection()");
    finishFeature(cmd, FeatName);
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
    makeChamferOrFillet(this, "Fillet");
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
    makeChamferOrFillet(this, "Chamfer");
    doCommand(Gui,"Gui.Selection.clearSelection()");
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
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if (!pcActiveBody) return;

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

    if (base != pcActiveBody->getPrevSolidFeature(NULL, true)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong base feature"),
            QObject::tr("Only the current Tip of the active Body can be selected as the base feature"));
        return;
    }

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

    finishFeature(this, FeatName);
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

    FeatName = cmd->getUniqueObjectName(which.c_str());

    std::stringstream str;
    str << "App.activeDocument()." << FeatName << ".Originals = [";
    for (std::vector<App::DocumentObject*>::iterator it = features.begin(); it != features.end(); ++it){
        str << "App.activeDocument()." << (*it)->getNameInDocument() << ",";
        selList.push_back((*it)->getNameInDocument());
    }
    str << "]";
    selNames = str.str();

    cmd->openCommand((std::string("Make ") + which + " feature").c_str());
    cmd->doCommand(cmd->Doc,"App.activeDocument().addObject(\"PartDesign::%s\",\"%s\")",which.c_str(), FeatName.c_str());
    // FIXME: There seems to be kind of a race condition here, leading to sporadic errors like
    // Exception (Thu Sep  6 11:52:01 2012): 'App.Document' object has no attribute 'Mirrored'
    cmd->updateActive(); // Helps to ensure that the object already exists when the next command comes up
    cmd->doCommand(cmd->Doc,selNames.c_str());
}

void finishTransformed(Gui::Command* cmd, std::string& FeatName, std::vector<std::string>& selList)
{
    //for (std::vector<std::string>::iterator it = selList.begin(); it != selList.end(); ++it)
    //    cmd->doCommand(cmd->Gui,"Gui.activeDocument().%s.Visibility=False",it->c_str());

    finishFeature(cmd, FeatName);
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

    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.MirrorPlane = (App.activeDocument().%s, [\"V_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());

    finishTransformed(this, FeatName, selList);
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

    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.Direction = (App.activeDocument().%s, [\"H_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Length = 100", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());

    finishTransformed(this, FeatName, selList);
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

    Part::Part2DObject *sketch = (static_cast<PartDesign::SketchBased*>(features.front()))->getVerifiedSketch();
    if (sketch)
        doCommand(Doc,"App.activeDocument().%s.Axis = (App.activeDocument().%s, [\"N_Axis\"])",
                  FeatName.c_str(), sketch->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.Angle = 360", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());

    finishTransformed(this, FeatName, selList);
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

    doCommand(Doc,"App.activeDocument().%s.Factor = 2", FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Occurrences = 2", FeatName.c_str());

    finishTransformed(this, FeatName, selList);
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
    sMenuText     = QT_TR_NOOP("Create MultiTransform");
    sToolTipText  = QT_TR_NOOP("Create a multitransform feature");
    sWhatsThis    = "PartDesign_MultiTransform";
    sStatusTip    = sToolTipText;
    sPixmap       = "PartDesign_MultiTransform";
}

void CmdPartDesignMultiTransform::activated(int iMsg)
{
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
    if (!pcActiveBody) return;

    std::vector<App::DocumentObject*> features;

    // Check if a Transformed feature has been selected, convert it to MultiTransform
    features = getSelection().getObjectsOfType(PartDesign::Transformed::getClassTypeId());
    if (!features.empty()) {
        // Throw out MultiTransform features, we don't want to nest them
        for (std::vector<App::DocumentObject*>::iterator f = features.begin(); f != features.end(); ) {
            if ((*f)->getTypeId().isDerivedFrom(PartDesign::MultiTransform::getClassTypeId()))
                f = features.erase(f);
            else
                f++;
        }

        if (features.empty()) return;
        // Note: If multiple Transformed features were selected, only the first one is used
        PartDesign::Transformed* trFeat = static_cast<PartDesign::Transformed*>(features.front());

        // Move the insert point back one feature
        App::DocumentObject* oldTip = pcActiveBody->Tip.getValue();
        App::DocumentObject* prevFeature = pcActiveBody->getPrevFeature(trFeat);
        Gui::Selection().clearSelection();
        if (prevFeature != NULL)
            Gui::Selection().addSelection(prevFeature->getDocument()->getName(), prevFeature->getNameInDocument());
        openCommand("Convert to MultiTransform feature");
        doCommand(Gui, "FreeCADGui.runCommand('PartDesign_MoveTip')");

        // Remove the Transformed feature from the Body
        doCommand(Doc, "App.activeDocument().%s.removeFeature(App.activeDocument().%s)",
                  pcActiveBody->getNameInDocument(), trFeat->getNameInDocument());

        // Create a MultiTransform feature and move the Transformed feature inside it
        std::string FeatName = getUniqueObjectName("MultiTransform");
        doCommand(Doc, "App.activeDocument().addObject(\"PartDesign::MultiTransform\",\"%s\")", FeatName.c_str());
        doCommand(Doc, "App.activeDocument().%s.Originals = App.activeDocument().%s.Originals", FeatName.c_str(), trFeat->getNameInDocument());
        doCommand(Doc, "App.activeDocument().%s.Originals = []", trFeat->getNameInDocument());
        doCommand(Doc, "App.activeDocument().%s.Transformations = [App.activeDocument().%s]", FeatName.c_str(), trFeat->getNameInDocument());

        // Add the MultiTransform into the Body at the current insert point
        finishFeature(this, FeatName);

        // Restore the insert point
        if (oldTip != trFeat) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
            Gui::Selection().clearSelection();
        } // otherwise the insert point remains at the new MultiTransform, which is fine
    } else {
        std::string FeatName, selNames;
        std::vector<std::string> selList;
        prepareTransformed(this, "MultiTransform", features, FeatName, selList, selNames);
        if (features.empty())
            return;

        PartDesign::Body *pcActiveBody = PartDesignGui::getBody();
        if (!pcActiveBody) 
            return;
        updateActive();
        doCommand(Doc,selNames.c_str());

        // Make sure the user isn't presented with an empty screen because no transformations are defined yet...
        App::DocumentObject* prevSolid = pcActiveBody->getPrevSolidFeature(NULL, true);
        if (prevSolid != NULL) {
            Part::Feature* feat = static_cast<Part::Feature*>(prevSolid);
            doCommand(Doc,"App.activeDocument().%s.Shape = App.activeDocument().%s.Shape",
                      FeatName.c_str(), feat->getNameInDocument());
        }

        finishFeature(this, FeatName);
    }
}

bool CmdPartDesignMultiTransform::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// PartDesign_Boolean
//===========================================================================

/* Boolean commands =======================================================*/
DEF_STD_CMD_A(CmdPartDesignBoolean);

CmdPartDesignBoolean::CmdPartDesignBoolean()
  :Command("PartDesign_Boolean")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Boolean operation");
    sToolTipText    = QT_TR_NOOP("Boolean operation with two or more boies");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "PartDesign_Boolean";
}


void CmdPartDesignBoolean::activated(int iMsg)
{
    Gui::SelectionFilter BodyFilter("SELECT PartDesign::Body COUNT 1..");
    PartDesign::Body* body;
    std::string bodyString("");

    if (BodyFilter.match()) {
        body = static_cast<PartDesign::Body*>(BodyFilter.Result[0][0].getObject());
        std::vector<App::DocumentObject*> bodies;
        std::vector<std::vector<Gui::SelectionObject> >::iterator i = BodyFilter.Result.begin();
        i++;
        for (; i != BodyFilter.Result.end(); i++) {
            for (std::vector<Gui::SelectionObject>::iterator j = i->begin(); j != i->end(); j++) {
                bodies.push_back(j->getObject());
            }
        }
        bodyString = PartDesignGui::getPythonStr(bodies);
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No body selected"),
            QObject::tr("Please select a body for the boolean operation"));
        return;
    }

    openCommand("Create Boolean");

	PartDesign::Body* activeBody = Gui::Application::Instance->activeView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    // Make sure we are working on the selected body
	if (body != activeBody) {
        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(body->getDocument()->getName(), body->Tip.getValue()->getNameInDocument());
        Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
    }

    std::string FeatName = getUniqueObjectName("Boolean");

    doCommand(Doc,"App.activeDocument().addObject('PartDesign::Boolean','%s')",FeatName.c_str());
    if (!bodyString.empty())
        doCommand(Doc,"App.activeDocument().%s.Bodies = %s",FeatName.c_str(),bodyString.c_str());
    finishFeature(this, FeatName, false);
}

bool CmdPartDesignBoolean::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPartDesignBody());
    rcCmdMgr.addCommand(new CmdPartDesignMoveTip());

    rcCmdMgr.addCommand(new CmdPartDesignDuplicateSelection());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeature());
    rcCmdMgr.addCommand(new CmdPartDesignMoveFeatureInTree());

    rcCmdMgr.addCommand(new CmdPartDesignPlane());
    rcCmdMgr.addCommand(new CmdPartDesignLine());
    rcCmdMgr.addCommand(new CmdPartDesignPoint());

    rcCmdMgr.addCommand(new CmdPartDesignNewSketch());

    rcCmdMgr.addCommand(new CmdPartDesignPad());
    rcCmdMgr.addCommand(new CmdPartDesignPocket());
    rcCmdMgr.addCommand(new CmdPartDesignRevolution());
    rcCmdMgr.addCommand(new CmdPartDesignGroove());

    rcCmdMgr.addCommand(new CmdPartDesignFillet());
    rcCmdMgr.addCommand(new CmdPartDesignDraft());    
    rcCmdMgr.addCommand(new CmdPartDesignChamfer());

    rcCmdMgr.addCommand(new CmdPartDesignMirrored());
    rcCmdMgr.addCommand(new CmdPartDesignLinearPattern());
    rcCmdMgr.addCommand(new CmdPartDesignPolarPattern());
    //rcCmdMgr.addCommand(new CmdPartDesignScaled());
    rcCmdMgr.addCommand(new CmdPartDesignMultiTransform());

    rcCmdMgr.addCommand(new CmdPartDesignBoolean());
 }
