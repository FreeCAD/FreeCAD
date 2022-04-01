/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <cfloat>
# include <memory>
# include <QMessageBox>
# include <Precision.hxx>
# include <QApplication>
# include <QMessageBox>

# include <Inventor/SbString.h>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludePropertyExternal.h>

#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "ViewProviderSketch.h"
#include "SketchRectangularArrayDialog.h"
#include "Utils.h"

#include <BRepAdaptor_Curve.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HCurve.hxx>
#endif
#include <BRepClass_FaceClassifier.hxx>
#include <Mod/Part/App/BRepOffsetAPI_MakeOffsetFix.h>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <TopoDS.hxx>

#include "DrawSketchDefaultWidgetHandler.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isSketcherAcceleratorActive(Gui::Document *doc, bool actsOnSelection)
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            auto mode = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode();
            if (mode == ViewProviderSketch::STATUS_NONE ||
                mode == ViewProviderSketch::STATUS_SKETCH_UseHandler) {
                if (!actsOnSelection)
                    return true;
                else if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0)
                    return true;
            }
        }
    }

    return false;
}

void ActivateAcceleratorHandler(Gui::Document *doc, DrawSketchHandler *handler)
{
    std::unique_ptr<DrawSketchHandler> ptr(handler);
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());
            vp->purgeHandler();
            vp->activateHandler(ptr.release());
        }
    }
}

// ================================================================================

// Close Shape Command
DEF_STD_CMD_A(CmdSketcherCloseShape)

CmdSketcherCloseShape::CmdSketcherCloseShape()
    :Command("Sketcher_CloseShape")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Close shape");
    sToolTipText    = QT_TR_NOOP("Produce a closed shape by tying the end point "
                                 "of one element with the next element's starting point");
    sWhatsThis      = "Sketcher_CloseShape";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CloseShape";
    sAccel          = "Z, W";
    eType           = ForEdit;
}

void CmdSketcherCloseShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two edges from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two edges from the sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    int GeoIdFirst = -1;
    int GeoIdLast = -1;

    // undo command open
    openCommand(QT_TRANSLATE_NOOP("Command", "Add coincident constraint"));
    // go through the selected subelements
    for (size_t i=0; i < (SubNames.size() - 1); i++) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge" &&
            SubNames[i+1].size() > 4 && SubNames[i+1].substr(0,4) == "Edge") {

            int GeoId1 = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;
            int GeoId2 = std::atoi(SubNames[i+1].substr(4,4000).c_str()) - 1;

            if (GeoIdFirst == -1)
                GeoIdFirst = GeoId1;

            GeoIdLast = GeoId2;

            const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
            if ((geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId()) ||
                (geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                      QObject::tr("One selected edge is not connectable"));
                abortCommand();
                return;
            }

            // Check for the special case of closing a shape with two lines to avoid overlap
            if (SubNames.size() == 2 &&
                geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Closing a shape formed by exactly two lines makes no sense."));
                abortCommand();
                return;
            }

            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "addConstraint(Sketcher.Constraint('Coincident', %d, %d, %d, %d)) ",
                                  GeoId1, static_cast<int>(Sketcher::PointPos::end), GeoId2, static_cast<int>(Sketcher::PointPos::start));
        }
    }

    // Close Last Edge with First Edge
    Gui::cmdAppObjectArgs(selection[0].getObject(),
                          "addConstraint(Sketcher.Constraint('Coincident', %d, %d, %d, %d)) ",
                          GeoIdLast, static_cast<int>(Sketcher::PointPos::end), GeoIdFirst, static_cast<int>(Sketcher::PointPos::start));

    // finish the transaction and update, and clear the selection (convenience)
    commitCommand();
    tryAutoRecompute(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherCloseShape::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Connect Edges Command
DEF_STD_CMD_A(CmdSketcherConnect)

CmdSketcherConnect::CmdSketcherConnect()
    :Command("Sketcher_ConnectLines")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Connect edges");
    sToolTipText    = QT_TR_NOOP("Tie the end point of the element with next element's starting point");
    sWhatsThis      = "Sketcher_ConnectLines";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ConnectLines";
    sAccel          = "Z, J";
    eType           = ForEdit;
}

void CmdSketcherConnect::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two edges from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two edges from the sketch."));
        return;
    }
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // undo command open
    openCommand(QT_TRANSLATE_NOOP("Command", "Add coincident constraint"));

    // go through the selected subelements
    for (unsigned int i=0; i<(SubNames.size()-1); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge" &&
            SubNames[i+1].size() > 4 && SubNames[i+1].substr(0,4) == "Edge") {

            int GeoId1 = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;
            int GeoId2 = std::atoi(SubNames[i+1].substr(4,4000).c_str()) - 1;

            const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
            if ((geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId()) ||
                (geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                      QObject::tr("One selected edge is not connectable"));
                abortCommand();
                return;
            }

            Gui::cmdAppObjectArgs(selection[0].getObject(),"addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                GeoId1,static_cast<int>(Sketcher::PointPos::end),GeoId2,static_cast<int>(Sketcher::PointPos::start));
        }
    }

    // finish the transaction and update, and clear the selection (convenience)
    commitCommand();
    tryAutoRecompute(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherConnect::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Select Constraints of selected elements
DEF_STD_CMD_A(CmdSketcherSelectConstraints)

CmdSketcherSelectConstraints::CmdSketcherSelectConstraints()
    :Command("Sketcher_SelectConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select associated constraints");
    sToolTipText    = QT_TR_NOOP("Select the constraints associated with the selected geometrical elements");
    sWhatsThis      = "Sketcher_SelectConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectConstraints";
    sAccel          = "Z, K";
    eType           = ForEdit;
}

void CmdSketcherSelectConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    getSelection().clearSelection();

    std::vector<std::string> constraintSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            // push all the constraints
            int i = 0;
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it,++i)
            {
                if ((*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId) {
                    constraintSubNames.push_back(Sketcher::PropertyConstraintList::getConstraintName(i));
                }
            }
        }
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);

}

bool CmdSketcherSelectConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

// Select Origin
DEF_STD_CMD_A(CmdSketcherSelectOrigin)

CmdSketcherSelectOrigin::CmdSketcherSelectOrigin()
    :Command("Sketcher_SelectOrigin")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select origin");
    sToolTipText    = QT_TR_NOOP("Select the local origin point of the sketch");
    sWhatsThis      = "Sketcher_SelectOrigin";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectOrigin";
    sAccel          = "Z, O";
    eType           = ForEdit;
}

void CmdSketcherSelectOrigin::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();
//    ViewProviderSketch * vp = static_cast<ViewProviderSketch *>(Gui::Application::Instance->getViewProvider(docobj));
//    Sketcher::SketchObject* Obj = vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "RootPoint";

    if(Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
        Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectOrigin::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

// Select Vertical Axis
DEF_STD_CMD_A(CmdSketcherSelectVerticalAxis)

CmdSketcherSelectVerticalAxis::CmdSketcherSelectVerticalAxis()
    :Command("Sketcher_SelectVerticalAxis")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select vertical axis");
    sToolTipText    = QT_TR_NOOP("Select the local vertical axis of the sketch");
    sWhatsThis      = "Sketcher_SelectVerticalAxis";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectVerticalAxis";
    sAccel          = "Z, V";
    eType           = ForEdit;
}

void CmdSketcherSelectVerticalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "V_Axis";

    if(Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
      Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
      Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectVerticalAxis::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

// Select Horizontal Axis
DEF_STD_CMD_A(CmdSketcherSelectHorizontalAxis)

CmdSketcherSelectHorizontalAxis::CmdSketcherSelectHorizontalAxis()
    :Command("Sketcher_SelectHorizontalAxis")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select horizontal axis");
    sToolTipText    = QT_TR_NOOP("Select the local horizontal axis of the sketch");
    sWhatsThis      = "Sketcher_SelectHorizontalAxis";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectHorizontalAxis";
    sAccel          = "Z, H";
    eType           = ForEdit;
}

void CmdSketcherSelectHorizontalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    ss << "H_Axis";

    if(Gui::Selection().isSelected(doc_name.c_str(), obj_name.c_str(), ss.str().c_str()))
      Gui::Selection().rmvSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
    else
      Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
}

bool CmdSketcherSelectHorizontalAxis::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectRedundantConstraints)

CmdSketcherSelectRedundantConstraints::CmdSketcherSelectRedundantConstraints()
    :Command("Sketcher_SelectRedundantConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select redundant constraints");
    sToolTipText    = QT_TR_NOOP("Select redundant constraints");
    sWhatsThis      = "Sketcher_SelectRedundantConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectRedundantConstraints";
    sAccel          = "Z, P, R";
    eType           = ForEdit;
}

void CmdSketcherSelectRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solverredundant = vp->getSketchObject()->getLastRedundant();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;

    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverredundant.begin();itc != solverredundant.end(); ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectRedundantConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectMalformedConstraints)

CmdSketcherSelectMalformedConstraints::CmdSketcherSelectMalformedConstraints()
    :Command("Sketcher_SelectMalformedConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select malformed constraints");
    sToolTipText    = QT_TR_NOOP("Select malformed constraints");
    sWhatsThis      = "Sketcher_SelectMalformedConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectMalformedConstraints";
    sAccel          = "Z, P, M";
    eType           = ForEdit;
}

void CmdSketcherSelectMalformedConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solvermalformed = vp->getSketchObject()->getLastMalformedConstraints();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solvermalformed.begin();itc != solvermalformed.end(); ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectMalformedConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectPartiallyRedundantConstraints)

CmdSketcherSelectPartiallyRedundantConstraints::CmdSketcherSelectPartiallyRedundantConstraints()
    :Command("Sketcher_SelectPartiallyRedundantConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select partially redundant constraints");
    sToolTipText    = QT_TR_NOOP("Select partially redundant constraints");
    sWhatsThis      = "Sketcher_SelectPartiallyRedundantConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectPartiallyRedundantConstraints";
    sAccel          = "Z, P, P";
    eType           = ForEdit;
}

void CmdSketcherSelectPartiallyRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solverpartiallyredundant = vp->getSketchObject()->getLastPartiallyRedundant();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverpartiallyredundant.begin();itc != solverpartiallyredundant.end(); ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectPartiallyRedundantConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectConflictingConstraints)

CmdSketcherSelectConflictingConstraints::CmdSketcherSelectConflictingConstraints()
    :Command("Sketcher_SelectConflictingConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select conflicting constraints");
    sToolTipText    = QT_TR_NOOP("Select conflicting constraints");
    sWhatsThis      = "Sketcher_SelectConflictingConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectConflictingConstraints";
    sAccel          = "Z, P, C";
    eType           = ForEdit;
}

void CmdSketcherSelectConflictingConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();
    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solverconflicting = vp->getSketchObject()->getLastConflicting();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    std::vector<std::string> constraintSubNames;
    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for (std::vector< int >::const_iterator itc= solverconflicting.begin();itc != solverconflicting.end(); ++itc) {
            if ((*itc) - 1 == i) {
                constraintSubNames.push_back(Sketcher::PropertyConstraintList::getConstraintName(i));
                break;
            }
        }
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
}

bool CmdSketcherSelectConflictingConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectElementsAssociatedWithConstraints)

CmdSketcherSelectElementsAssociatedWithConstraints::CmdSketcherSelectElementsAssociatedWithConstraints()
    :Command("Sketcher_SelectElementsAssociatedWithConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select associated geometry");
    sToolTipText    = QT_TR_NOOP("Select the geometrical elements associated with the selected constraints");
    sWhatsThis      = "Sketcher_SelectElementsAssociatedWithConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectElementsAssociatedWithConstraints";
    sAccel          = "Z, E";
    eType           = ForEdit;
}

void CmdSketcherSelectElementsAssociatedWithConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    std::vector<std::string> elementSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle constraints
        if (it->size() > 10 && it->substr(0,10) == "Constraint") {
            int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);

            if(ConstrId < static_cast<int>(vals.size())){
                if(vals[ConstrId]->First!=GeoEnum::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->FirstPos)
                    {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->First + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->First,vals[ConstrId]->FirstPos);
                            if(vertex>-1)
                                ss << "Vertex" <<  vertex + 1;
                            break;
                    }
                    elementSubNames.push_back(ss.str());
                }

                if(vals[ConstrId]->Second!=GeoEnum::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->SecondPos)
                    {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->Second + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Second,vals[ConstrId]->SecondPos);
                            if(vertex>-1)
                                ss << "Vertex" << vertex + 1;
                            break;
                    }

                    elementSubNames.push_back(ss.str());
                }

                if(vals[ConstrId]->Third!=GeoEnum::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->ThirdPos)
                    {
                        case Sketcher::PointPos::none:
                            ss << "Edge" << vals[ConstrId]->Third + 1;
                            break;
                        case Sketcher::PointPos::start:
                        case Sketcher::PointPos::end:
                        case Sketcher::PointPos::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Third,vals[ConstrId]->ThirdPos);
                            if(vertex>-1)
                                ss << "Vertex" <<  vertex + 1;
                            break;
                    }

                    elementSubNames.push_back(ss.str());
                }
            }
        }
    }

    if (elementSubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No constraint selected"),
                             QObject::tr("At least one constraint must be selected"));
    }
    else {
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), elementSubNames);
    }

}

bool CmdSketcherSelectElementsAssociatedWithConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSelectElementsWithDoFs)

CmdSketcherSelectElementsWithDoFs::CmdSketcherSelectElementsWithDoFs()
:Command("Sketcher_SelectElementsWithDoFs")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Select unconstrained DoF");
    sToolTipText    = QT_TR_NOOP("Select geometrical elements where the solver still detects unconstrained degrees of freedom.");
    sWhatsThis      = "Sketcher_SelectElementsWithDoFs";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectElementsWithDoFs";
    sAccel          = "Z, F";
    eType           = ForEdit;
}

void CmdSketcherSelectElementsWithDoFs::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getSelection().clearSelection();
    Gui::Document * doc= getActiveGuiDocument();
    ReleaseHandler(doc);
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    auto geos = Obj->getInternalGeometry();

    std::vector<std::string> elementSubNames;

    auto testselectvertex = [&Obj, &ss, &elementSubNames](int geoId, PointPos pos) {
        ss.str(std::string());

        int vertex = Obj->getVertexIndexGeoPos(geoId, pos);
        if (vertex > -1) {
            ss << "Vertex" <<  vertex + 1;

            elementSubNames.push_back(ss.str());
        }
    };

    auto testselectedge = [&ss, &elementSubNames](int geoId) {
        ss.str(std::string());

        ss << "Edge" <<  geoId + 1;
        elementSubNames.push_back(ss.str());
    };

    int geoid = 0;

    for (auto geo : geos) {
        if(geo) {
            if(geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                    geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                if (solvext->getGeometry() == Sketcher::SolverGeometryExtension::NotFullyConstraint) {
                    // Coded for consistency with getGeometryWithDependentParameters, read the comments
                    // on that function
                    if (solvext->getEdge() == SolverGeometryExtension::Dependent)
                        testselectedge(geoid);
                    if (solvext->getStart() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::start);
                    if (solvext->getEnd() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::end);
                    if (solvext->getMid() == SolverGeometryExtension::Dependent)
                        testselectvertex(geoid, Sketcher::PointPos::mid);
                }
            }
        }

        geoid++;
    }

    if (!elementSubNames.empty()) {
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), elementSubNames);
    }

}

bool CmdSketcherSelectElementsWithDoFs::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherRestoreInternalAlignmentGeometry)

CmdSketcherRestoreInternalAlignmentGeometry::CmdSketcherRestoreInternalAlignmentGeometry()
    :Command("Sketcher_RestoreInternalAlignmentGeometry")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Show/hide internal geometry");
    sToolTipText    = QT_TR_NOOP("Show all internal geometry or hide unused internal geometry");
    sWhatsThis      = "Sketcher_RestoreInternalAlignmentGeometry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Element_Ellipse_All";
    sAccel          = "Z, I";
    eType           = ForEdit;
}

void CmdSketcherRestoreInternalAlignmentGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    // Return GeoId of the SubName only if it is an edge
    auto getEdgeGeoId = [&Obj](const std::string& SubName) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(SubName, Obj, GeoId, PosId);
        if (PosId == Sketcher::PointPos::none)
            return GeoId;
        else
            return (int)GeoEnum::GeoUndef;
    };

    // Tells if the geometry with given GeoId has internal geometry
    auto noInternalGeo = [&Obj](const auto& GeoId) {
        const Part::Geometry *geo = Obj->getGeometry(GeoId);
        bool hasInternalGeo = geo &&
            (geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
             geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
             geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
             geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
             geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId());
        return !hasInternalGeo; // so it's removed
    };

    std::vector<int> SubGeoIds(SubNames.size());
    std::transform(SubNames.begin(), SubNames.end(), SubGeoIds.begin(), getEdgeGeoId);

    // Handle highest GeoIds first to minimize GeoIds changing
    // TODO: this might not completely resolve GeoIds changing
    std::sort(SubGeoIds.begin(), SubGeoIds.end(), std::greater<int>());
    // Keep unique
    SubGeoIds.erase(std::unique(SubGeoIds.begin(), SubGeoIds.end()), SubGeoIds.end());

    // Only for supported types and keep unique
    SubGeoIds.erase(std::remove_if(SubGeoIds.begin(), SubGeoIds.end(), noInternalGeo),
                    SubGeoIds.end());

    // go through the selected subelements
    for (const auto& GeoId : SubGeoIds) {
        int currentgeoid = Obj->getHighestCurveIndex();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Exposing Internal Geometry"));
            Gui::cmdAppObjectArgs(Obj, "exposeInternalGeometry(%d)", GeoId);

            int aftergeoid = Obj->getHighestCurveIndex();

            if(aftergeoid == currentgeoid) { // if we did not expose anything, deleteunused
                Gui::cmdAppObjectArgs(Obj, "deleteUnusedInternalGeometry(%d)", GeoId);
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(Obj));

            return;
        }

        Gui::Command::commitCommand();
        tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(Obj));
    }
}

bool CmdSketcherRestoreInternalAlignmentGeometry::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherSymmetry)

CmdSketcherSymmetry::CmdSketcherSymmetry()
    :Command("Sketcher_Symmetry")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Symmetry");
    sToolTipText    = QT_TR_NOOP("Creates symmetric geometry with respect to the last selected line or point");
    sWhatsThis      = "Sketcher_Symmetry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Symmetry";
    sAccel          = "Z, S";
    eType           = ForEdit;
}

void CmdSketcherSymmetry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    SketcherGui::ReleaseHandler(doc);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::PointPos::none;
    const Part::Geometry *LastGeo;
    typedef enum { invalid = -1, line = 0, point = 1 } GeoType;

    GeoType lastgeotype = invalid;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if ((it->size() > 4 && it->substr(0,4) == "Edge") ||
            (it->size() > 12 && it->substr(0,12) == "ExternalEdge")) {

            if (it->substr(0,4) == "Edge") {
                LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
                LastPointPos = Sketcher::PointPos::none;
            }
            else {
                LastGeoId = -std::atoi(it->substr(12,4000).c_str()) - 2;
                LastPointPos = Sketcher::PointPos::none;
            }

            // reference can be external or non-external
            LastGeo = Obj->getGeometry(LastGeoId);
            // Only for supported types
            if (LastGeo->getTypeId() == Part::GeomLineSegment::getClassTypeId())
                lastgeotype = line;
            else
                lastgeotype = invalid;

            // lines to make symmetric (only non-external)
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);

            if (Obj->getGeometry(GeoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::PointPos::start;
                lastgeotype = point;

                // points to make symmetric
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    bool lastvertexoraxis = false;
    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex") {
        int VtId = std::atoi(SubNames.rbegin()->substr(6,4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (Obj->getGeometry(GeoId)->getTypeId() != Part::GeomPoint::getClassTypeId()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
            lastgeotype = point;
            lastvertexoraxis = true;
        }
    }
    // check if last selected element is horizontal axis
    else if (SubNames.rbegin()->size() == 6 && SubNames.rbegin()->substr(0,6) == "H_Axis") {
        LastGeoId = Sketcher::GeoEnum::HAxis;
        LastPointPos = Sketcher::PointPos::none;
        lastgeotype = line;
        lastvertexoraxis = true;
    }
    // check if last selected element is vertical axis
    else if (SubNames.rbegin()->size() == 6 && SubNames.rbegin()->substr(0,6) == "V_Axis") {
        LastGeoId = Sketcher::GeoEnum::VAxis;
        LastPointPos = Sketcher::PointPos::none;
        lastgeotype = line;
        lastvertexoraxis = true;
    }
    // check if last selected element is the root point
    else if (SubNames.rbegin()->size() == 9 && SubNames.rbegin()->substr(0,9) == "RootPoint") {
        LastGeoId = Sketcher::GeoEnum::RtPnt;
        LastPointPos = Sketcher::PointPos::start;
        lastgeotype = point;
        lastvertexoraxis = true;
    }

    if (geoids == 0 || (geoids == 1 && LastGeoId >= 0 && !lastvertexoraxis)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("A symmetric construction requires "
                        "at least two geometric elements, "
                        "the last geometric element being the reference "
                        "for the symmetry construction."));
        return;
    }

    if (lastgeotype == invalid) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The last element must be a point "
                        "or a line serving as reference "
                        "for the symmetry construction."));
        return;
    }

    std::string geoIdList = stream.str();

    // missing cases:
    // 1- Last element is an edge, and is V or H axis
    // 2- Last element is a point GeomPoint
    // 3- Last element is a point (Vertex)

    if (LastGeoId >= 0 && !lastvertexoraxis) {
        // if LastGeoId was added remove the last element
        int index = geoIdList.rfind(',');
        index = geoIdList.rfind(',', index-1);
        geoIdList.resize(index);
    }
    else {
        int index = geoIdList.rfind(',');
        geoIdList.resize(index);
    }

    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create symmetric geometry"));

    try{
        Gui::cmdAppObjectArgs(Obj,
                              "addSymmetric(%s, %d, %d)",
                              geoIdList.c_str(), LastGeoId, static_cast<int>(LastPointPos));
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        Gui::Command::abortCommand();
    }
    tryAutoRecomputeIfNotSolve(Obj);
}

bool CmdSketcherSymmetry::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

class SketcherCopy : public Gui::Command {
public:
    enum Op {
        Copy,
        Clone,
        Move
    };
    SketcherCopy(const char* name);
    void activate(SketcherCopy::Op op);
    virtual void activate() = 0;
};

// TODO: replace XPM cursor with SVG file
static const char *cursor_createcopy[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "................................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    "................................",
    ".+++++...+++++..................",
    "................................",
    ".......+........................",
    ".......+..............###.......",
    ".......+..............###.......",
    ".......+..............###.......",
    ".......+..............###.......",
    "......................###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###..............###.......",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    "................................",
    "................................",
    "................................",
    "................................"};

class DrawSketchHandlerCopy: public DrawSketchHandler
{
public:
    DrawSketchHandlerCopy(string geoidlist, int origingeoid,
                          Sketcher::PointPos originpos, int nelements,
                          SketcherCopy::Op op)
    : Mode(STATUS_SEEK_First)
    , snapMode(SnapMode::Free)
    , geoIdList(geoidlist)
    , Origin()
    , OriginGeoId(origingeoid)
    , OriginPos(originpos)
    , nElements(nelements)
    , Op(op)
    , EditCurve(2)
    {
    }

    virtual ~DrawSketchHandlerCopy(){}
    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_End
    };

    enum class SnapMode {
        Free,
        Snap5Degree
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {

             if(QApplication::keyboardModifiers() == Qt::ControlModifier)
                    snapMode = SnapMode::Snap5Degree;
                else
                    snapMode = SnapMode::Free;

            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).Angle();

            Base::Vector2d endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI/36)) * M_PI/36;
                endpoint = EditCurve[0] + length * Base::Vector2d(cos(angle),sin(angle));
            }

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(endpoint, text);

            EditCurve[1] = endpoint;
            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr1, endpoint, Base::Vector2d(0.0, 0.0), AutoConstraint::VERTEX)) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d) override
    {
        if (Mode == STATUS_SEEK_First) {
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End)
        {
            Base::Vector2d vector = EditCurve[1] - EditCurve[0];
            unsetCursor();
            resetPositionText();

            int currentgeoid = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getHighestCurveIndex();
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Copy/clone/move geometry"));

            try{
                if (Op != SketcherCopy::Move) {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addCopy(%s, App.Vector(%f, %f, 0), %s)",
                                          geoIdList.c_str(), vector.x, vector.y,
                                          (Op == SketcherCopy::Clone ? "True" : "False"));
                }
                else {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addMove(%s, App.Vector(%f, %f, 0))",
                                          geoIdList.c_str(), vector.x, vector.y);
                }
                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();
            }

            if (Op != SketcherCopy::Move) {
                // add auto constraints for the destination copy
                if (sugConstr1.size() > 0) {
                    createAutoConstraints(sugConstr1, currentgeoid+nElements, OriginPos);
                    sugConstr1.clear();
                }
            }
            else {
                if (sugConstr1.size() > 0) {
                    createAutoConstraints(sugConstr1, OriginGeoId, OriginPos);
                    sugConstr1.clear();
                }
            }

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
            EditCurve.clear();
            drawEdit(EditCurve);

            // no code after this line, Handler gets deleted in ViewProvider
            sketchgui->purgeHandler();
        }
        return true;
    }
private:
    virtual void activated() override
    {
        setCursor(QPixmap(cursor_createcopy), 7, 7);
        Origin = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getPoint(OriginGeoId, OriginPos);
        EditCurve[0] = Base::Vector2d(Origin.x, Origin.y);
    }
protected:
    SelectMode Mode;
    SnapMode snapMode;
    string geoIdList;
    Base::Vector3d Origin;
    int OriginGeoId;
    Sketcher::PointPos OriginPos;
    int nElements;
    SketcherCopy::Op Op;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1;
};

/*---- SketcherCopy definition ----*/
SketcherCopy::SketcherCopy(const char* name): Command(name)
{}

void SketcherCopy::activate(SketcherCopy::Op op)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::PointPos::none;
    const Part::Geometry *LastGeo = nullptr;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::PointPos::none;
            LastGeo = Obj->getGeometry(LastGeoId);
            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::PointPos::start;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex") {
        int VtId = std::atoi(SubNames.rbegin()->substr(6,4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (Obj->getGeometry(GeoId)->getTypeId() != Part::GeomPoint::getClassTypeId()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
        }
    }

    if (geoids < 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("A copy requires at least one selected non-external geometric element"));
        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the center point)
    if (LastPointPos == Sketcher::PointPos::none) {
        if (LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            LastPointPos = Sketcher::PointPos::mid;
        }
        else {
            LastPointPos = Sketcher::PointPos::start;
        }
    }

    // Ask the user if they want to clone or to simple copy
/*
    int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Dimensional/Geometric constraints"),
                                    QObject::tr("Do you want to clone the object, i.e. substitute dimensional constraints by geometric constraints?"),
                                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        clone = true;
    }
    else if (ret == QMessageBox::Cancel) {
    // do nothing
    return;
    }
*/

    ActivateAcceleratorHandler(getActiveGuiDocument(),
                               new DrawSketchHandlerCopy(geoIdList, LastGeoId, LastPointPos, geoids, op));
}


class CmdSketcherCopy : public SketcherCopy
{
public:
    CmdSketcherCopy();
    virtual ~CmdSketcherCopy(){}
    virtual const char* className() const
    { return "CmdSketcherCopy"; }
    virtual void activate();
protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
};

CmdSketcherCopy::CmdSketcherCopy()
    :SketcherCopy("Sketcher_Copy")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Copy");
    sToolTipText    = QT_TR_NOOP("Creates a simple copy of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Copy";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Copy";
    sAccel          = "Z, C";
    eType           = ForEdit;
}

void CmdSketcherCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Copy);
}


void CmdSketcherCopy::activate()
{
    SketcherCopy::activate(SketcherCopy::Copy);
}

bool CmdSketcherCopy::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

class CmdSketcherClone : public SketcherCopy
{
public:
    CmdSketcherClone();
    virtual ~CmdSketcherClone(){}
    virtual const char* className() const
    { return "CmdSketcherClone"; }
    virtual void activate();
protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
};

CmdSketcherClone::CmdSketcherClone()
    :SketcherCopy("Sketcher_Clone")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Clone");
    sToolTipText    = QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Clone";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Clone";
    sAccel          = "Z, L";
    eType           = ForEdit;
}

void CmdSketcherClone::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Clone);
}

void CmdSketcherClone::activate()
{
    SketcherCopy::activate(SketcherCopy::Clone);
}

bool CmdSketcherClone::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

class CmdSketcherMove : public SketcherCopy
{
public:
    CmdSketcherMove();
    virtual ~CmdSketcherMove(){}
    virtual const char* className() const
    { return "CmdSketcherMove"; }
    virtual void activate();
protected:
    virtual void activated(int iMsg);
    virtual bool isActive(void);
};

CmdSketcherMove::CmdSketcherMove()
    :SketcherCopy("Sketcher_Move")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Move");
    sToolTipText    = QT_TR_NOOP("Moves the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Move";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Move";
    sAccel          = "Z, M";
    eType           = ForEdit;
}

void CmdSketcherMove::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    SketcherCopy::activate(SketcherCopy::Move);
}

void CmdSketcherMove::activate()
{
    SketcherCopy::activate(SketcherCopy::Move);
}

bool CmdSketcherMove::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

// ================================================================================

DEF_STD_CMD_ACL(CmdSketcherCompCopy)

CmdSketcherCompCopy::CmdSketcherCompCopy()
    : Command("Sketcher_CompCopy")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Copy");
    sToolTipText    = QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_CompCopy";
    sStatusTip      = sToolTipText;
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherCompCopy::activated(int iMsg)
{
    if (iMsg<0 || iMsg>2)
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());

    if (iMsg == 0){
        CmdSketcherClone sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->getAccel()));
    }
    else if (iMsg == 1) {
        CmdSketcherCopy sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->getAccel()));
    }
    else if (iMsg == 2) {
        CmdSketcherMove sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(""));
    }
}

Gui::Action * CmdSketcherCompCopy::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* clone = pcAction->addAction(QString());
    clone->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Clone"));
    QAction* copy = pcAction->addAction(QString());
    copy->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Copy"));
    QAction* move = pcAction->addAction(QString());
    move->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Move"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(clone->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    pcAction->setShortcut(QString::fromLatin1(getAccel()));

    return pcAction;
}

void CmdSketcherCompCopy::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* clone = a[0];
    clone->setText(QApplication::translate("Sketcher_CompCopy","Clone"));
    clone->setToolTip(QApplication::translate("Sketcher_Clone","Creates a clone of the geometry taking as reference the last selected point"));
    clone->setStatusTip(QApplication::translate("Sketcher_Clone","Creates a clone of the geometry taking as reference the last selected point"));
    QAction* copy = a[1];
    copy->setText(QApplication::translate("Sketcher_CompCopy","Copy"));
    copy->setToolTip(QApplication::translate("Sketcher_Copy","Creates a simple copy of the geometry taking as reference the last selected point"));
    copy->setStatusTip(QApplication::translate("Sketcher_Copy","Creates a simple copy of the geometry taking as reference the last selected point"));
    QAction* move = a[2];
    move->setText(QApplication::translate("Sketcher_CompCopy","Move"));
    move->setToolTip(QApplication::translate("Sketcher_Move","Moves the geometry taking as reference the last selected point"));
    move->setStatusTip(QApplication::translate("Sketcher_Move","Moves the geometry taking as reference the last selected point"));
}

bool CmdSketcherCompCopy::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

// ================================================================================

// TODO: replace XPM cursor with SVG file
/* XPM */
static const char *cursor_createrectangulararray[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "................................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    "................................",
    ".+++++...+++++..................",
    ".......................###......",
    ".......+...............###......",
    ".......+...............###......",
    ".......+...............###......",
    ".......+......###......###......",
    ".......+......###......###......",
    "..............###......###......",
    "..............###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###...............",
    ".....###......###...............",
    ".....###......###...............",
    ".....###......###...............",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    "................................",
    "................................"};

class DrawSketchHandlerRectangularArray: public DrawSketchHandler
{
public:
    DrawSketchHandlerRectangularArray(string geoidlist, int origingeoid,
                                      Sketcher::PointPos originpos, int nelements, bool clone,
                                      int rows, int cols, bool constraintSeparation,
                                      bool equalVerticalHorizontalSpacing)
        : Mode(STATUS_SEEK_First)
        , snapMode(SnapMode::Free)
        , geoIdList(geoidlist)
        , OriginGeoId(origingeoid)
        , OriginPos(originpos)
        , nElements(nelements)
        , Clone(clone)
        , Rows(rows)
        , Cols(cols)
        , ConstraintSeparation(constraintSeparation)
        , EqualVerticalHorizontalSpacing(equalVerticalHorizontalSpacing)
        , EditCurve(2)
    {
    }

    virtual ~DrawSketchHandlerRectangularArray(){}
    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_End
    };

    enum class SnapMode {
        Free,
        Snap5Degree
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {

            if(QApplication::keyboardModifiers() == Qt::ControlModifier)
                    snapMode = SnapMode::Snap5Degree;
                else
                    snapMode = SnapMode::Free;

            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).Angle();

            Base::Vector2d endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI/36)) * M_PI/36;
                endpoint = EditCurve[0] + length * Base::Vector2d(cos(angle),sin(angle));
            }

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(endpoint, text);

            EditCurve[1] = endpoint;
            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr1, endpoint, Base::Vector2d(0.0, 0.0), AutoConstraint::VERTEX))
            {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }

        }
        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d) override
    {
        if (Mode == STATUS_SEEK_First) {
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            Base::Vector2d vector = EditCurve[1] - EditCurve[0];
            unsetCursor();
            resetPositionText();

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create copy of geometry"));

            try {
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addRectangularArray(%s, App.Vector(%f, %f, 0), %s, %d, %d, %s, %f)",
                                      geoIdList.c_str(), vector.x, vector.y,
                                      (Clone ? "True" : "False"),
                                      Cols, Rows,
                                      (ConstraintSeparation ? "True" : "False"),
                                      (EqualVerticalHorizontalSpacing ? 1.0 : 0.5));
                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();
            }

            // add auto constraints for the destination copy
            if (sugConstr1.size() > 0) {
                createAutoConstraints(sugConstr1, OriginGeoId+nElements, OriginPos);
                sugConstr1.clear();
            }
            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            EditCurve.clear();
            drawEdit(EditCurve);

            // no code after this line, Handler is deleted in ViewProvider
            sketchgui->purgeHandler();
        }
        return true;
    }
private:
    virtual void activated() override
    {
        setCursor(QPixmap(cursor_createrectangulararray), 7, 7);
        Origin = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getPoint(OriginGeoId, OriginPos);
        EditCurve[0] = Base::Vector2d(Origin.x, Origin.y);
    }
protected:
    SelectMode Mode;
    SnapMode snapMode;
    string geoIdList;
    Base::Vector3d Origin;
    int OriginGeoId;
    Sketcher::PointPos OriginPos;
    int nElements;
    bool Clone;
    int Rows;
    int Cols;
    bool ConstraintSeparation;
    bool EqualVerticalHorizontalSpacing;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1;
};

DEF_STD_CMD_A(CmdSketcherRectangularArray)

CmdSketcherRectangularArray::CmdSketcherRectangularArray()
    :Command("Sketcher_RectangularArray")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Rectangular array");
    sToolTipText    = QT_TR_NOOP("Creates a rectangular array pattern of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_RectangularArray";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_RectangularArray";
    sAccel          = "Z, A";
    eType           = ForEdit;
}

void CmdSketcherRectangularArray::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::PointPos::none;
    const Part::Geometry *LastGeo = nullptr;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::PointPos::none;
            LastGeo = Obj->getGeometry(LastGeoId);

            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::PointPos::start;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex") {
        int VtId = std::atoi(SubNames.rbegin()->substr(6,4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (Obj->getGeometry(GeoId)->getTypeId() != Part::GeomPoint::getClassTypeId()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
        }
    }

    if (geoids < 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("A copy requires at least one selected non-external geometric element"));
        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the center point)
    if (LastPointPos == Sketcher::PointPos::none) {
        if (LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            LastPointPos = Sketcher::PointPos::mid;
        }
        else {
            LastPointPos = Sketcher::PointPos::start;
        }
    }

    // Pop-up asking for values
    SketchRectangularArrayDialog slad;

    if (slad.exec() == QDialog::Accepted) {
        ActivateAcceleratorHandler(getActiveGuiDocument(),
            new DrawSketchHandlerRectangularArray(geoIdList, LastGeoId, LastPointPos, geoids, slad.Clone,
                                                  slad.Rows, slad.Cols, slad.ConstraintSeparation,
                                                  slad.EqualVerticalHorizontalSpacing));
    }
}

bool CmdSketcherRectangularArray::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}



// Translate / rectangular pattern tool =======================================================
//Todo: Add 2 more paramters to the tool widget so that we can use 8. Adding angles of translation vectors.

class DrawSketchHandlerTranslate;

using DrawSketchHandlerTranslateBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerTranslate,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*PNumToolwidgetparameters =*/6,
    /*PNumToolwidgetCheckboxes =*/ 1,
    /*PNumToolwidgetComboboxes =*/ 1>;



class DrawSketchHandlerTranslate : public DrawSketchHandlerTranslateBase
{
public:
    enum class ConstructionMethod {
        LinearArray,
        RectangularArray
    };

    DrawSketchHandlerTranslate(std::vector<int> listOfGeoIds)
        :
        constructionMethod(ConstructionMethod::LinearArray)
        , snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , firstTranslationVector(Base::Vector3d(0., 0., 0.))
        , secondTranslationVector(Base::Vector3d(0., 0., 0.))
        , deleteOriginal(false)
        , cloneConstraints(false)
        , numberOfCopies(0)
        , secondNumberOfCopies(1)

        {}
    virtual ~DrawSketchHandlerTranslate() {}


    enum class SnapMode {
        Free,
        Snap5Degree
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            referencePoint = onSketchPos;
            if (snapMode == SnapMode::Snap5Degree) {
                getSnapPoint(referencePoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            double length = (onSketchPos - referencePoint).Length();
            double angle = (onSketchPos - referencePoint).Angle();
            firstTranslationPoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                if (getSnapPoint(firstTranslationPoint)) {
                    angle = (firstTranslationPoint - referencePoint).Angle();
                }
                else {
                    angle = round(angle / (M_PI / 36)) * M_PI / 36;
                    firstTranslationPoint = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));
                }
            }

            firstTranslationVector.x = (firstTranslationPoint - referencePoint).x;
            firstTranslationVector.y = (firstTranslationPoint - referencePoint).y;

            //Draw geometries
            generateTranslatedGeos(false);

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(firstTranslationPoint, text);
        }
        break;
        case SelectMode::SeekThird:
        {
            double length = (onSketchPos - referencePoint).Length();
            double angle = (onSketchPos - referencePoint).Angle();
            secondTranslationPoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                if (getSnapPoint(secondTranslationPoint)) {
                    angle = (secondTranslationPoint - referencePoint).Angle();
                }
                else {
                    angle = round(angle / (M_PI / 36)) * M_PI / 36;
                    secondTranslationPoint = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));
                }
            }

            secondTranslationVector.x = (secondTranslationPoint - referencePoint).x;
            secondTranslationVector.y = (secondTranslationPoint - referencePoint).y;

            //Draw geometries
            generateTranslatedGeos(false);

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(secondTranslationPoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateTranslatedGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Translate";
    }

    virtual QString getCrosshairCursorString() const override {
        return QString::fromLatin1("Sketcher_Translate");
    }

    //reimplement because linear array is 2 steps while rectangular array is 3 steps
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (state() == SelectMode::SeekSecond && constructionMethod == ConstructionMethod::LinearArray) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

public:
    ConstructionMethod constructionMethod;
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d referencePoint, firstTranslationPoint, secondTranslationPoint;
    Base::Vector3d firstTranslationVector, secondTranslationVector;

    bool deleteOriginal, cloneConstraints;
    int numberOfCopies, secondNumberOfCopies, firstCurveCreated;

    void generateTranslatedGeos(bool onReleaseButton) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = 1;
        }
        else {
            deleteOriginal = 0;
        }

        //Generate geos
        std::vector<Part::Geometry*> geometriesToAdd;
        for (int k = 0; k < secondNumberOfCopies; k++) {
            for (int i = 0; i <= numberOfCopiesToMake; i++) {
                if (!(k == 0 && i == 0)) {
                    for (size_t j = 0; j < listOfGeoIds.size(); j++) {
                        Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
                        GeometryFacade::setConstruction(geo, GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));
                        if (geo->getTypeId() == Part::GeomConic::getClassTypeId()) {
                            Part::GeomConic* conic = static_cast<Part::GeomConic*>(geo);
                            conic->setCenter(conic->getCenter() + firstTranslationVector * i + secondTranslationVector * k);
                            geometriesToAdd.push_back(conic);
                        }
                        else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                            line->setPoints(line->getStartPoint() + firstTranslationVector * i + secondTranslationVector * k,
                                line->getEndPoint() + firstTranslationVector * i + secondTranslationVector * k);
                            geometriesToAdd.push_back(line);
                        }
                        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                            std::vector<Base::Vector3d> poles = bSpline->getPoles();
                            for (size_t p = 0; p < poles.size(); p++) {
                                poles[p] = poles[p] + firstTranslationVector * i + secondTranslationVector * k;
                            }
                            bSpline->setPoles(poles);
                            geometriesToAdd.push_back(bSpline);
                        }
                    }
                }
            }
        }

        if (!onReleaseButton) {
            //Add the line to show angle
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(firstTranslationPoint.x, firstTranslationPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            if (secondTranslationVector.Length() > Precision::Confusion()) {
                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
                p2 = Base::Vector3d(secondTranslationPoint.x, secondTranslationPoint.y, 0.);
                line2->setPoints(p1, p2);
                geometriesToAdd.push_back(line2);
            }

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Translate"));
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constrains
            const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
            std::vector< Constraint* > newconstrVals(vals);
            std::vector<int> geoIdsWhoAlreadyHasEqual = {}; //avoid applying equal several times if cloning distanceX and distanceY of the same part.

            std::vector< Sketcher::Constraint* >::const_iterator itEnd = vals.end(); //we need vals.end before adding any constraints
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != itEnd; ++it) {
                int firstIndex = indexInVec(listOfGeoIds, (*it)->First);
                int secondIndex = indexInVec(listOfGeoIds, (*it)->Second);
                int thirdIndex = indexInVec(listOfGeoIds, (*it)->Third);

                if (((*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Perpendicular)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                Constraint* constNew = (*it)->copy();
                                constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Third = firstCurveCreated + thirdIndex + listOfGeoIds.size() * i + listOfGeoIds.size() * numberOfCopiesToMake * k;
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::Angle
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == GeoEnum::GeoUndef) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                Constraint* constNew = (*it)->copy();
                                constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                if (deleteOriginal || !cloneConstraints) {
                                    Constraint* constNew = (*it)->copy();
                                    constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                                else { //Clone constraint mode !
                                    Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;// first is already (*it)->First
                                    constNew->isDriving = true;
                                    constNew->Second = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) { //only line length because we can't apply equality between points.
                    for (int k = 0; k < secondNumberOfCopies; k++) {
                        for (int i = 0; i <= numberOfCopiesToMake; i++) {
                            if (!(k == 0 && i == 0)) {
                                if (deleteOriginal || !cloneConstraints || (*it)->First != (*it)->Second) {
                                    Constraint* constNew = (*it)->copy();
                                    constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    newconstrVals.push_back(constNew);
                                }
                                else if (indexInVec(geoIdsWhoAlreadyHasEqual, firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k) == -1) { //Clone constraint mode !
                                    Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;
                                    constNew->isDriving = true;
                                    constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * (i - 1) + listOfGeoIds.size() * (numberOfCopiesToMake + 1) * k;
                                    geoIdsWhoAlreadyHasEqual.push_back(constNew->Second);
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                    }
                }
            }
            if (newconstrVals.size() > vals.size())
                Obj->Constraints.setValues(std::move(newconstrVals));

            if (deleteOriginal) {
                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }
            Gui::Command::commitCommand();

            sketchgui->getSketchObject()->solve(true);
            sketchgui->draw(false, false); // Redraw
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    int indexInVec(std::vector<int> vec, int elem)
    {
        if (elem == GeoEnum::GeoUndef) {
            return GeoEnum::GeoUndef;
        }
        for (size_t i = 0; i < vec.size(); i++)
        {
            if (vec[i] == elem)
            {
                return i;
            }
        }
        return -1;
    }
};


DEF_STD_CMD_A(CmdSketcherTranslate)

CmdSketcherTranslate::CmdSketcherTranslate()
    : Command("Sketcher_Translate")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Translate geometries");
    sToolTipText = QT_TR_NOOP("Translate selected geometries n times, enable creation of rectangular patterns.");
    sWhatsThis = "Sketcher_Translate";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Translate";
    sAccel = "W";
    eType = ForEdit;
}

void CmdSketcherTranslate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (!SubNames.empty()) {
        Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
            // only handle non-external edges
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int geoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                if (geoId >= 0) {
                    listOfGeoIds.push_back(geoId);
                }
            }
            else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
                // only if it is a GeomPoint
                int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
                int geoId;
                Sketcher::PointPos PosId;
                Obj->getGeoVertexIndex(VtId, geoId, PosId);
                if (Obj->getGeometry(geoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                    if (geoId >= 0) {
                        listOfGeoIds.push_back(geoId);
                    }
                }
            }
        }
    }

    getSelection().clearSelection();

    ActivateAcceleratorHandler(getActiveGuiDocument(), new DrawSketchHandlerTranslate(listOfGeoIds));
}

bool CmdSketcherTranslate::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// Rotate / circular pattern tool =======================================================
//TODO DrawSketchDefaultWidgetHandler is not a template!
class DrawSketchHandlerRotate;

using DrawSketchHandlerRotateBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerRotate,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*PNumToolwidgetparameters =*/4,
    /*PNumToolwidgetCheckboxes =*/ 1,
    /*PNumToolwidgetComboboxes =*/ 0>;

class DrawSketchHandlerRotate : public DrawSketchHandlerRotateBase
{
public:
    DrawSketchHandlerRotate(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , cloneConstraints(false)
        , numberOfCopies(0) {}
    virtual ~DrawSketchHandlerRotate() {}

    enum class SnapMode {
        Free,
        Snap
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            centerPoint = onSketchPos;
            if (snapMode == SnapMode::Snap) {
                getSnapPoint(centerPoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            length = (onSketchPos - centerPoint).Length();
            startAngle = (onSketchPos - centerPoint).Angle();

            startPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(startPoint)) {
                    startAngle = (startPoint - centerPoint).Angle();
                }
                else {
                    startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                    startPoint = centerPoint + length * Base::Vector2d(cos(startAngle), sin(startAngle));
                }
            }

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, startAngle * 180 / M_PI);
            setPositionText(startPoint, text);

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x, centerPoint.y, 0.), Base::Vector3d(startPoint.x, startPoint.y, 0.));
            geometriesToAdd.push_back(line);
            drawEdit(geometriesToAdd);

        }
        break;
        case SelectMode::SeekThird:
        {
            endAngle = (onSketchPos - centerPoint).Angle();
            endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(endpoint)) {
                    endAngle = (endpoint - centerPoint).Angle();
                }
                else {
                    endAngle = round(endAngle / (M_PI / 36)) * M_PI / 36;
                    endpoint = centerPoint + length * Base::Vector2d(cos(endAngle), sin(endAngle));
                }
            }
            else {
                endpoint = centerPoint + length * Base::Vector2d(cos(endAngle), sin(endAngle));
            }
            double angle1 = atan2(endpoint.y - centerPoint.y,
                endpoint.x - centerPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            totalAngle = abs(angle1 - totalAngle) < abs(angle2 - totalAngle) ? angle1 : angle2;

            //generate the copies
            generateRotatedGeos(/*CreateGeos*/ false);
            sketchgui->draw(false, false); // Redraw

            SbString text;
            text.sprintf(" (%d copies, %.1fdeg)", numberOfCopies, totalAngle * 180 / M_PI);
            setPositionText(endpoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateRotatedGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Rotate";
    }

    virtual QString getCrosshairCursorString() const override {
        return QString::fromLatin1("Sketcher_Rotate");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

public:
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d centerPoint, startPoint, endpoint;

    bool deleteOriginal, cloneConstraints;
    double length, startAngle, endAngle, totalAngle, individualAngle;
    int numberOfCopies, firstCurveCreated;

    void generateRotatedGeos(bool onReleaseButton) {
        int numberOfCopiesToMake = numberOfCopies;
        if (numberOfCopies == 0) {
            numberOfCopiesToMake = 1;
            deleteOriginal = 1;
        }
        else {
            deleteOriginal = 0;
        }

        individualAngle = totalAngle / numberOfCopiesToMake;

        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        //Generate geos
        std::vector<Part::Geometry*> geometriesToAdd;
        for (int i = 1; i <= numberOfCopiesToMake; i++) {
            for (size_t j = 0; j < listOfGeoIds.size(); j++) {
                Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
                GeometryFacade::setConstruction(geo, GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));
                if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                    Part::GeomCircle* circle = static_cast<Part::GeomCircle*>(geo);
                    circle->setCenter(getRotatedPoint(circle->getCenter(), centerPoint, individualAngle * i));
                    geometriesToAdd.push_back(circle);
                }
                else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                    Part::GeomArcOfCircle* arcOfCircle = static_cast<Part::GeomArcOfCircle*>(geo);
                    arcOfCircle->setCenter(getRotatedPoint(arcOfCircle->getCenter(), centerPoint, individualAngle * i));
                    double arcStartAngle, arcEndAngle;
                    arcOfCircle->getRange(arcStartAngle, arcEndAngle, /*emulateCCWXY=*/true);
                    arcOfCircle->setRange(arcStartAngle + individualAngle * i, arcEndAngle + individualAngle * i, /*emulateCCWXY=*/true);
                    geometriesToAdd.push_back(arcOfCircle);
                }
                else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                    line->setPoints(getRotatedPoint(line->getStartPoint(), centerPoint, individualAngle * i),
                        getRotatedPoint(line->getEndPoint(), centerPoint, individualAngle * i));
                    geometriesToAdd.push_back(line);
                }
                else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                    Part::GeomEllipse* ellipse = static_cast<Part::GeomEllipse*>(geo);
                    ellipse->setCenter(getRotatedPoint(ellipse->getCenter(), centerPoint, individualAngle * i));
                    ellipse->setMajorAxisDir(getRotatedPoint(ellipse->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(ellipse);
                }
                else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                    Part::GeomArcOfEllipse* arcOfEllipse = static_cast<Part::GeomArcOfEllipse*>(geo);
                    arcOfEllipse->setCenter(getRotatedPoint(arcOfEllipse->getCenter(), centerPoint, individualAngle * i));
                    arcOfEllipse->setMajorAxisDir(getRotatedPoint(arcOfEllipse->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(arcOfEllipse);
                }
                else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                    Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<Part::GeomArcOfHyperbola*>(geo);
                    arcOfHyperbola->setCenter(getRotatedPoint(arcOfHyperbola->getCenter(), centerPoint, individualAngle * i));
                    arcOfHyperbola->setMajorAxisDir(getRotatedPoint(arcOfHyperbola->getMajorAxisDir(), Base::Vector2d(0., 0.), individualAngle * i));
                    geometriesToAdd.push_back(arcOfHyperbola);
                }
                else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                    Part::GeomArcOfParabola* arcOfParabola = static_cast<Part::GeomArcOfParabola*>(geo);

                    arcOfParabola->setCenter(getRotatedPoint(arcOfParabola->getCenter(), centerPoint, individualAngle * i));
                    arcOfParabola->setAngleXU(arcOfParabola->getAngleXU() + individualAngle * i);
                    geometriesToAdd.push_back(arcOfParabola);
                }
                else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                    std::vector<Base::Vector3d> poles = bSpline->getPoles();
                    for (size_t p = 0; p < poles.size(); p++) {
                        poles[p] = getRotatedPoint(poles[p], centerPoint, individualAngle * i);
                    }
                    bSpline->setPoles(poles);
                    geometriesToAdd.push_back(bSpline);
                }
            }
        }

        if (!onReleaseButton) {
            //Add the lines to show angle
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(centerPoint.x, centerPoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(startPoint.x, startPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
            p1 = Base::Vector3d(centerPoint.x, centerPoint.y, 0.);
            p2 = Base::Vector3d(endpoint.x, endpoint.y, 0.);
            line2->setPoints(p1, p2);
            geometriesToAdd.push_back(line2);

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Rotate"));
            Obj->addGeometry(std::move(geometriesToAdd));

            const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
            std::vector< Constraint* > newconstrVals(vals);
            std::vector<int> geoIdsWhoAlreadyHasEqual = {}; //avoid applying equal several times if cloning distanceX and distanceY of the same part.

            std::vector< Sketcher::Constraint* >::const_iterator itEnd = vals.end(); //we need vals.end before adding any constraints
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != itEnd; ++it) {
                int firstIndex = indexInVec(listOfGeoIds, (*it)->First);
                int secondIndex = indexInVec(listOfGeoIds, (*it)->Second);
                int thirdIndex = indexInVec(listOfGeoIds, (*it)->Third);

                if (((*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Perpendicular)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        Constraint* constNew = (*it)->copy();
                        constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                        constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                        constNew->Third = firstCurveCreated + thirdIndex + listOfGeoIds.size() * i;
                        newconstrVals.push_back(constNew);
                    }
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == GeoEnum::GeoUndef) {
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        Constraint* constNew = (*it)->copy();
                        constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                        constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                        newconstrVals.push_back(constNew);
                    }
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        if (deleteOriginal || !cloneConstraints) {
                            Constraint* constNew = (*it)->copy();
                            constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                        else {
                            Constraint* constNew = (*it)->copy();
                            constNew->Type = Sketcher::Equal;// first is already (*it)->First
                            constNew->isDriving = true;
                            constNew->Second = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) { //only line length because we can't apply equality between points.
                    for (int i = 0; i < numberOfCopiesToMake; i++) {
                        if ((deleteOriginal || !cloneConstraints) && (*it)->Type == Sketcher::Distance) {
                            Constraint* constNew = (*it)->copy();
                            constNew->First = firstCurveCreated + firstIndex + listOfGeoIds.size() * i;
                            constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                            newconstrVals.push_back(constNew);
                        }
                        else if ((*it)->First == (*it)->Second && indexInVec(geoIdsWhoAlreadyHasEqual, firstCurveCreated + secondIndex + listOfGeoIds.size() * i) == -1) {
                            Constraint* constNew = (*it)->copy();
                            constNew->Type = Sketcher::Equal;// first is already (*it)->First
                            constNew->isDriving = true;
                            constNew->Second = firstCurveCreated + secondIndex + listOfGeoIds.size() * i;
                            geoIdsWhoAlreadyHasEqual.push_back(constNew->Second);
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
            }
            if (newconstrVals.size() > vals.size())
                Obj->Constraints.setValues(std::move(newconstrVals));

            if (deleteOriginal) {
                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }
            Gui::Command::commitCommand();

            sketchgui->getSketchObject()->solve(true);
            sketchgui->draw(false, false); // Redraw
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    int indexInVec(std::vector<int> vec, int elem)
    {
        if (elem == GeoEnum::GeoUndef) {
            return GeoEnum::GeoUndef;
        }
        for (size_t i = 0; i < vec.size(); i++)
        {
            if (vec[i] == elem)
            {
                return i;
            }
        }
        return -1;
    }

    Base::Vector3d getRotatedPoint(Base::Vector3d pointToRotate, Base::Vector2d centerPoint, double angle) {
        Base::Vector2d pointToRotate2D;
        pointToRotate2D.x = pointToRotate.x;
        pointToRotate2D.y = pointToRotate.y;

        double initialAngle = (pointToRotate2D - centerPoint).Angle();
        double lengthToCenter = (pointToRotate2D - centerPoint).Length();

        pointToRotate2D = centerPoint + lengthToCenter * Base::Vector2d(cos(angle + initialAngle), sin(angle + initialAngle));


        pointToRotate.x = pointToRotate2D.x;
        pointToRotate.y = pointToRotate2D.y;

        return pointToRotate;
    }
};

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rotate", "x of center"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rotate", "y of center"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rotate", "Total angle"));
    toolWidget->configureParameterUnit(WParameter::Third, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rotate", "Number of copies"));
    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_rotate", "Clone constraints"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Rotate_1", "Select the center of the rotation."));
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    auto dHandler = static_cast<DrawSketchHandlerRotate*>(handler);
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->totalAngle = value * M_PI / 180;
        break;
    case WParameter::Fourth:
        dHandler->numberOfCopies = floor(abs(value));
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
        toolWidget->setParameterFocus(WParameter::First);
        toolWidget->setNoticeText(QApplication::translate("Rotate_1", "Select the center of the rotation."));
        break;
    case SelectMode::SeekSecond:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Rotate_2", "Select a first point that will define the rotation angle with the next point."));
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Fifth);
        toolWidget->setNoticeText(QApplication::translate("Rotate_3", "Select the second point that will determine the rotation angle."));
        break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex)
    auto dHandler = static_cast<DrawSketchHandlerRotate*>(handler);
    dHandler->cloneConstraints = value;

    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::doOverrideSketchPosition(Base::Vector2d& onSketchPos) {
    auto dHandler = static_cast<DrawSketchHandlerRotate*>(handler);

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = max(1.0, (onSketchPos - dHandler->centerPoint).Length()); //avoid nul length
            dHandler->startAngle = 0.0;
            dHandler->totalAngle = toolWidget->getParameter(WParameter::Third) * M_PI / 180;
            onSketchPos = dHandler->centerPoint + Base::Vector2d(dHandler->length, 0.0);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = max(1.0, (onSketchPos - dHandler->centerPoint).Length()); //avoid nul length
            dHandler->startAngle = 0.0;
            dHandler->endAngle = toolWidget->getParameter(WParameter::Third) * M_PI / 180;
            dHandler->totalAngle = dHandler->endAngle;
            onSketchPos = dHandler->centerPoint + Base::Vector2d(cos(dHandler->totalAngle), sin(dHandler->totalAngle)) * dHandler->length;
        }
    }
    break;
    default:
        break;
    }
    prevCursorPosition = onSketchPos;
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::updateVisualValues(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekThird:
    {
        auto dHandler = static_cast<DrawSketchHandlerRotate*>(handler);
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->totalAngle * 180 / M_PI);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::SeekThird);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerRotateBase::ToolWidgetManager::addConstraints() {
    //none
}

DEF_STD_CMD_A(CmdSketcherRotate)

CmdSketcherRotate::CmdSketcherRotate()
    : Command("Sketcher_Rotate")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rotate geometries");
    sToolTipText = QT_TR_NOOP("Rotate selected geometries n times, enable creation of circular patterns.");
    sWhatsThis = "Sketcher_Rotate";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Rotate";
    sAccel = "B";
    eType = ForEdit;
}

void CmdSketcherRotate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (!SubNames.empty()) {
        Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
            // only handle non-external edges
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int geoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                if (geoId >= 0) {
                    listOfGeoIds.push_back(geoId);
                }
            }
            else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
                // only if it is a GeomPoint
                int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
                int geoId;
                Sketcher::PointPos PosId;
                Obj->getGeoVertexIndex(VtId, geoId, PosId);
                if (Obj->getGeometry(geoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                    if (geoId >= 0) {
                        listOfGeoIds.push_back(geoId);
                    }
                }
            }
        }
    }

    getSelection().clearSelection();

    ActivateAcceleratorHandler(getActiveGuiDocument(), new DrawSketchHandlerRotate(listOfGeoIds));
}

bool CmdSketcherRotate::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// Scale tool =====================================================================

class DrawSketchHandlerScale;

using DrawSketchHandlerScaleBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerScale,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*PNumToolwidgetparameters =*/3,
    /*PNumToolwidgetCheckboxes =*/ 1,
    /*PNumToolwidgetComboboxes =*/ 0>;

class DrawSketchHandlerScale : public DrawSketchHandlerScaleBase
{
public:
    DrawSketchHandlerScale(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false) {}
    virtual ~DrawSketchHandlerScale() {}

    enum class SnapMode {
        Free,
        Snap,
        Snap5Degree
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            referencePoint = onSketchPos;
            if (snapMode == SnapMode::Snap5Degree) {
                getSnapPoint(referencePoint);
            }
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            refLength = (onSketchPos - referencePoint).Length();

            startPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(startPoint)) {
                    refLength = (startPoint - referencePoint).Length();
                }
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(referencePoint.x, referencePoint.y, 0.), Base::Vector3d(startPoint.x, startPoint.y, 0.));
            geometriesToAdd.push_back(line);
            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1f)", refLength);
            setPositionText(startPoint, text);
        }
        break;
        case SelectMode::SeekThird:
        {
            length = (onSketchPos - referencePoint).Length();

            endPoint = onSketchPos;

            if (snapMode == SnapMode::Snap) {
                if (getSnapPoint(endPoint)) {
                    length = (endPoint - referencePoint).Length();
                }
            }

            scaleFactor = length / refLength;

            //generate the copies
            generateScaledGeos(/*CreateGeos*/ false);

            SbString text;
            text.sprintf(" (%.1f)", length);
            setPositionText(endPoint, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        generateScaledGeos(/*CreateGeos*/ true);

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Scale";
    }

    virtual QString getCrosshairCursorString() const override {
        return QString::fromLatin1("Sketcher_Scale");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;
    }

public:
    SnapMode snapMode;
    std::vector<int> listOfGeoIds;
    Base::Vector2d referencePoint, startPoint, endPoint;
    bool deleteOriginal;
    double refLength, length, scaleFactor;
    int firstCurveCreated;
    Base::Vector2d centerPoint;

    void generateScaledGeos(bool onReleaseButton) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        std::vector<Part::Geometry*> geometriesToAdd;
        for (size_t j = 0; j < listOfGeoIds.size(); j++) {
            Part::Geometry* geo = Obj->getGeometry(listOfGeoIds[j])->copy();
            GeometryFacade::setConstruction(geo, GeometryFacade::getConstruction(Obj->getGeometry(listOfGeoIds[j])));

            if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                Part::GeomCircle* circle = static_cast<Part::GeomCircle*>(geo);
                circle->setRadius(circle->getRadius() * scaleFactor);
                circle->setCenter(getScaledPoint(circle->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(circle);
            }
            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                Part::GeomArcOfCircle* arcOfCircle = static_cast<Part::GeomArcOfCircle*>(geo);
                arcOfCircle->setRadius(arcOfCircle->getRadius() * scaleFactor);
                arcOfCircle->setCenter(getScaledPoint(arcOfCircle->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfCircle);
            }
            else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                Part::GeomLineSegment* line = static_cast<Part::GeomLineSegment*>(geo);
                line->setPoints(getScaledPoint(line->getStartPoint(), referencePoint, scaleFactor),
                    getScaledPoint(line->getEndPoint(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(line);
            }
            else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                Part::GeomEllipse* ellipse = static_cast<Part::GeomEllipse*>(geo);
                ellipse->setMajorRadius(ellipse->getMajorRadius() * scaleFactor);
                ellipse->setMinorRadius(ellipse->getMinorRadius() * scaleFactor);
                ellipse->setCenter(getScaledPoint(ellipse->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(ellipse);
            }
            else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                Part::GeomArcOfEllipse* arcOfEllipse = static_cast<Part::GeomArcOfEllipse*>(geo);
                arcOfEllipse->setMajorRadius(arcOfEllipse->getMajorRadius() * scaleFactor);
                arcOfEllipse->setMinorRadius(arcOfEllipse->getMinorRadius() * scaleFactor);
                arcOfEllipse->setCenter(getScaledPoint(arcOfEllipse->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfEllipse);
            }
            else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<Part::GeomArcOfHyperbola*>(geo);
                arcOfHyperbola->setMajorRadius(arcOfHyperbola->getMajorRadius() * scaleFactor);
                arcOfHyperbola->setMinorRadius(arcOfHyperbola->getMinorRadius() * scaleFactor);
                arcOfHyperbola->setCenter(getScaledPoint(arcOfHyperbola->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfHyperbola);
            }
            else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                Part::GeomArcOfParabola* arcOfParabola = static_cast<Part::GeomArcOfParabola*>(geo);
                //Todo: Problem with scale parabola end points.
                arcOfParabola->setFocal(arcOfParabola->getFocal() * scaleFactor);
                arcOfParabola->setCenter(getScaledPoint(arcOfParabola->getCenter(), referencePoint, scaleFactor));
                geometriesToAdd.push_back(arcOfParabola);
            }
            else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Part::GeomBSplineCurve* bSpline = static_cast<Part::GeomBSplineCurve*>(geo);
                std::vector<Base::Vector3d> poles = bSpline->getPoles();
                for (size_t p = 0; p < poles.size(); p++) {
                    poles[p] = getScaledPoint(poles[p], referencePoint, scaleFactor);
                }
                bSpline->setPoles(poles);
                geometriesToAdd.push_back(bSpline);
            }
        }

        if (!onReleaseButton) {
            //Add the lines to show lengths
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            Base::Vector3d p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            Base::Vector3d p2 = Base::Vector3d(startPoint.x, startPoint.y, 0.);
            line->setPoints(p1, p2);
            geometriesToAdd.push_back(line);

            Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
            p1 = Base::Vector3d(referencePoint.x, referencePoint.y, 0.);
            p2 = Base::Vector3d(endPoint.x, endPoint.y, 0.);
            line2->setPoints(p1, p2);
            geometriesToAdd.push_back(line2);

            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            //Creates geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Scale"));
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constraints
            const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
            std::vector< Constraint* > newconstrVals(vals);
            std::vector<int> geoIdsWhoAlreadyHasEqual = {}; //avoid applying equal several times if cloning distanceX and distanceY of the same part.

            std::vector< Sketcher::Constraint* >::const_iterator itEnd = vals.end(); //we need vals.end before adding any constraints
            for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != itEnd; ++it) {
                int firstIndex = indexInVec(listOfGeoIds, (*it)->First);
                int secondIndex = indexInVec(listOfGeoIds, (*it)->Second);
                int thirdIndex = indexInVec(listOfGeoIds, (*it)->Third);

                if (((*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Perpendicular)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex >= 0) {
                    Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    constNew->Third = firstCurveCreated + thirdIndex;
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Coincident
                    || (*it)->Type == Sketcher::Tangent
                    || (*it)->Type == Sketcher::Symmetric
                    || (*it)->Type == Sketcher::Perpendicular
                    || (*it)->Type == Sketcher::Parallel
                    || (*it)->Type == Sketcher::Equal
                    || (*it)->Type == Sketcher::PointOnObject)
                    && firstIndex >= 0 && secondIndex >= 0 && thirdIndex == GeoEnum::GeoUndef) {
                    Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Radius
                    || (*it)->Type == Sketcher::Diameter)
                    && firstIndex >= 0) {
                    Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->setValue(constNew->getValue() * scaleFactor);
                    newconstrVals.push_back(constNew);
                }
                else if (((*it)->Type == Sketcher::Distance
                    || (*it)->Type == Sketcher::DistanceX
                    || (*it)->Type == Sketcher::DistanceY)
                    && firstIndex >= 0 && secondIndex >= 0) {
                    Constraint* constNew = (*it)->copy();
                    constNew->First = firstCurveCreated + firstIndex;
                    constNew->Second = firstCurveCreated + secondIndex;
                    constNew->setValue(constNew->getValue() * scaleFactor);
                    newconstrVals.push_back(constNew);
                }
            }
            if (newconstrVals.size() > vals.size())
                Obj->Constraints.setValues(std::move(newconstrVals));

            if (deleteOriginal) {
                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }

            Gui::Command::commitCommand();
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    int indexInVec(std::vector<int> vec, int elem)
    {
        if (elem == GeoEnum::GeoUndef) {
            return GeoEnum::GeoUndef;
        }
        for (size_t i = 0; i < vec.size(); i++)
        {
            if (vec[i] == elem)
            {
                return i;
            }
        }
        return -1;
    }

    Base::Vector3d getScaledPoint(Base::Vector3d pointToScale, Base::Vector2d referencePoint, double scaleFactor) {
        Base::Vector2d pointToScale2D;
        pointToScale2D.x = pointToScale.x;
        pointToScale2D.y = pointToScale.y;
        pointToScale2D = (pointToScale2D - referencePoint) * scaleFactor + referencePoint;

        pointToScale.x = pointToScale2D.x;
        pointToScale.y = pointToScale2D.y;

        return pointToScale;
    }
};

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_scale", "x of reference"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_scale", "y of reference"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_scale", "Scale factor"));

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_scale", "Keep original geometries"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Scale_1", "Select the reference point of the scale."));
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    auto dHandler = static_cast<DrawSketchHandlerScale*>(handler);
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->scaleFactor = value;
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
        toolWidget->setParameterFocus(WParameter::First);
        toolWidget->setNoticeText(QApplication::translate("Scale_1", "Select the center of the rotation."));
        break;
    case SelectMode::SeekSecond:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Scale_2", "Select a point where distance from this point to reference point represent the reference length."));
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Third);
        toolWidget->setNoticeText(QApplication::translate("Scale_3", "Select a point where distance from this point to reference point represent the length defining scale factor (scale factor = length / reference length)."));
        break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex)
    auto dHandler = static_cast<DrawSketchHandlerScale*>(handler);
    dHandler->deleteOriginal = value;

    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::doOverrideSketchPosition(Base::Vector2d& onSketchPos) {
    auto dHandler = static_cast<DrawSketchHandlerScale*>(handler);

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->scaleFactor = toolWidget->getParameter(WParameter::Third);
            onSketchPos = dHandler->referencePoint + Base::Vector2d(1.0, 0.0); //just in case mouse is at referencePoint to avoid 0. length
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->scaleFactor = toolWidget->getParameter(WParameter::Third);
            dHandler->startPoint = dHandler->referencePoint + Base::Vector2d(1.0, 0.0);
            dHandler->endPoint = dHandler->referencePoint + Base::Vector2d(dHandler->scaleFactor, 0.0);

            onSketchPos = dHandler->endPoint;
        }
    }
    break;
    default:
        break;
    }
    prevCursorPosition = onSketchPos;
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::updateVisualValues(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekThird:
    {
        auto dHandler = static_cast<DrawSketchHandlerScale*>(handler);
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->scaleFactor);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::SeekThird);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Third)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerScaleBase::ToolWidgetManager::addConstraints() {
    //none
}


DEF_STD_CMD_A(CmdSketcherScale)

CmdSketcherScale::CmdSketcherScale()
    : Command("Sketcher_Scale")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Scale geometries");
    sToolTipText = QT_TR_NOOP("Scale selected geometries.");
    sWhatsThis = "Sketcher_Scale";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Scale";
    sAccel = "S";
    eType = ForEdit;
}

void CmdSketcherScale::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (!SubNames.empty()) {
        Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
            // only handle non-external edges
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int geoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                if (geoId >= 0) {
                    listOfGeoIds.push_back(geoId);
                }
            }
            else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
                // only if it is a GeomPoint
                int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
                int geoId;
                Sketcher::PointPos PosId;
                Obj->getGeoVertexIndex(VtId, geoId, PosId);
                if (Obj->getGeometry(geoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                    if (geoId >= 0) {
                        listOfGeoIds.push_back(geoId);
                    }
                }
            }
        }
    }

    getSelection().clearSelection();

    ActivateAcceleratorHandler(getActiveGuiDocument(), new DrawSketchHandlerScale(listOfGeoIds));
}

bool CmdSketcherScale::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// Offset tool =====================================================================

class DrawSketchHandlerOffset;

using DrawSketchHandlerOffsetBase = DrawSketchDefaultWidgetHandler< DrawSketchHandlerOffset,
    StateMachines::OneSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*PNumToolwidgetparameters =*/1,
    /*PNumToolwidgetCheckboxes =*/ 2,
    /*PNumToolwidgetComboboxes =*/ 1>;

class DrawSketchHandlerOffset : public DrawSketchHandlerOffsetBase
{
public:
    DrawSketchHandlerOffset(std::vector<int> listOfGeoIds)
        : snapMode(SnapMode::Free)
        , listOfGeoIds(listOfGeoIds)
        , deleteOriginal(false)
        , offsetLengthSet(false)
        , offsetConstraint(false)
        , offsetLength(1) {}

    virtual ~DrawSketchHandlerOffset() {}

    enum class SnapMode {
        Free,
        Snap
    };

    enum class JoinMode {
        Arc,
        Tangent,
        Intersection
    };

    enum class ModeEnums {
        Skin,
        Pipe,
        RectoVerso
    };

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            endpoint = onSketchPos;
            if (snapMode == SnapMode::Snap) {
                getSnapPoint(endpoint);
            }

            if (!offsetLengthSet) {
                findOffsetLength();
            }

            SbString text;
            text.sprintf(" (%.1f)", offsetLength);
            setPositionText(endpoint, text);

            //generate the copies
            if (fabs(offsetLength) > Precision::Confusion()) {
                makeOffset(static_cast<int>(joinMode), false, /*MakeGeos*/false);
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {

        if (fabs(offsetLength) > Precision::Confusion()) {
            makeOffset(static_cast<int>(joinMode), false, /*MakeGeos*/true);
        }

        sketchgui->getSketchObject()->solve(true);
        sketchgui->draw(false, false); // Redraw

        sketchgui->purgeHandler();
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Offset";
    }

    virtual QString getCrosshairCursorString() const override {
        return QString::fromLatin1("Sketcher_Offset");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        firstCurveCreated = getHighestCurveIndex() + 1;

        vCC = generatevCC(listOfGeoIds);
        generateSourceWires();
    }

public:
    class CoincidencePointPos
    {
    public:
        Sketcher::PointPos FirstGeoPos;
        Sketcher::PointPos SecondGeoPos;
        Sketcher::PointPos SecondCoincidenceFirstGeoPos;
        Sketcher::PointPos SecondCoincidenceSecondGeoPos;
    };

    SnapMode snapMode;
    JoinMode joinMode;
    std::vector<int> listOfGeoIds;
    std::vector<int> listOfOffsetGeoIds;
    std::vector<std::vector<int>> vCC;
    std::vector<std::vector<int>> vCCO;
    Base::Vector2d endpoint;
    std::vector<TopoDS_Wire> sourceWires;

    bool deleteOriginal, offsetLengthSet, offsetConstraint;
    double offsetLength;
    int firstCurveCreated;

    void makeOffset(short joinType, bool allowOpenResult, bool onReleaseButton) {
        //make offset shape using BRepOffsetAPI_MakeOffset
        TopoDS_Shape offsetShape;
        Part::BRepOffsetAPI_MakeOffsetFix mkOffset(GeomAbs_JoinType(joinType), allowOpenResult);
        for (TopoDS_Wire& w : sourceWires) {
            mkOffset.AddWire(w);
        }
        try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkOffset.Perform(offsetLength);
        }
        catch (Standard_Failure&) {
            throw;
        }
        catch (...) {
            throw Base::CADKernelError("BRepOffsetAPI_MakeOffset has crashed! (Unknown exception caught)");
        }
        offsetShape = mkOffset.Shape();

        if (offsetShape.IsNull())
            throw Base::CADKernelError("makeOffset2D: result of offsetting is null!");

        //Copying shape to fix strange orientation behavior, OCC7.0.0. See bug #2699
        // http://www.freecadweb.org/tracker/view.php?id=2699
        offsetShape = BRepBuilderAPI_Copy(offsetShape).Shape();


        //turn wires/edges of shape into Geometries.
        std::vector<Part::Geometry*> geometriesToAdd;
        listOfOffsetGeoIds.clear();
        TopExp_Explorer expl(offsetShape, TopAbs_EDGE);
        int geoIdToAdd = firstCurveCreated;
        for (; expl.More(); expl.Next(), geoIdToAdd++) {

            const TopoDS_Edge& edge = TopoDS::Edge(expl.Current());
            BRepAdaptor_Curve curve(edge);
            if (curve.GetType() == GeomAbs_Line) {
                double first = curve.FirstParameter();
                if (fabs(first) > 1E99) {
                    first = -10000;
                }

                double last = curve.LastParameter();
                if (fabs(last) > 1E99) {
                    last = +10000;
                }

                gp_Pnt P1 = curve.Value(first);
                gp_Pnt P2 = curve.Value(last);

                Base::Vector3d p1(P1.X(), P1.Y(), P1.Z());
                Base::Vector3d p2(P2.X(), P2.Y(), P2.Z());
                Part::GeomLineSegment* line = new Part::GeomLineSegment();
                line->setPoints(p1, p2);
                GeometryFacade::setConstruction(line, false);
                geometriesToAdd.push_back(line);
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Circle) {
                gp_Circ circle = curve.Circle();
                gp_Pnt cnt = circle.Location();
                gp_Pnt beg = curve.Value(curve.FirstParameter());
                gp_Pnt end = curve.Value(curve.LastParameter());

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    Part::GeomCircle* gCircle = new Part::GeomCircle();
                    gCircle->setRadius(circle.Radius());
                    gCircle->setCenter(Base::Vector3d(cnt.X(), cnt.Y(), cnt.Z()));

                    GeometryFacade::setConstruction(gCircle, false);
                    geometriesToAdd.push_back(gCircle);
                }
                else {
                    Part::GeomArcOfCircle* gArc = new Part::GeomArcOfCircle();
                    Handle(Geom_Curve) hCircle = new Geom_Circle(circle);
                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hCircle, curve.FirstParameter(),
                        curve.LastParameter());
                    gArc->setHandle(tCurve);
                    GeometryFacade::setConstruction(gArc, false);
                    geometriesToAdd.push_back(gArc);
                }
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }
            else if (curve.GetType() == GeomAbs_Ellipse) {

                Base::Console().Warning("hello ellipse\n");
                gp_Elips ellipse = curve.Ellipse();
                //gp_Pnt cnt = ellipse.Location();
                gp_Pnt beg = curve.Value(curve.FirstParameter());
                gp_Pnt end = curve.Value(curve.LastParameter());

                if (beg.SquareDistance(end) < Precision::Confusion()) {
                    Part::GeomEllipse* gEllipse = new Part::GeomEllipse();
                    Handle(Geom_Ellipse) hEllipse = new Geom_Ellipse(ellipse);
                    gEllipse->setHandle(hEllipse);
                    GeometryFacade::setConstruction(gEllipse, false);
                    geometriesToAdd.push_back(gEllipse);
                }
                else {
                    Base::Console().Warning("hello arc ellipse\n");
                    Handle(Geom_Curve) hEllipse = new Geom_Ellipse(ellipse);
                    Handle(Geom_TrimmedCurve) tCurve = new Geom_TrimmedCurve(hEllipse, curve.FirstParameter(), curve.LastParameter());
                    Part::GeomArcOfEllipse* gArc = new Part::GeomArcOfEllipse();
                    gArc->setHandle(tCurve);
                    GeometryFacade::setConstruction(gArc, false);
                    geometriesToAdd.push_back(gArc);
                }
                listOfOffsetGeoIds.push_back(geoIdToAdd);
            }

        }


        if (!onReleaseButton) {
            //Draw geos
            drawEdit(geometriesToAdd);
        }
        else {
            Base::Console().Warning("hello onrelease \n");
            //Create geos
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Offset"));
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
            Obj->addGeometry(std::move(geometriesToAdd));

            //Create constraints
            std::stringstream stream;
            stream << "conList = []\n";
            for (size_t i = 0; i < listOfOffsetGeoIds.size() - 1; i++) {
                Base::Console().Warning("create constraint i : %d\n", i);
                for (size_t j = i + 1; j < listOfOffsetGeoIds.size(); j++) {
                    //There's a bug with created arcs. They end points seems to swap at some point... Here we make coincidences based on lengths. So they must change after.
                    //here we check for coincidence on all geometries. It's far from ideal. We should check only the geometries that were inside a wire next to each other.
                    Base::Vector3d firstStartPoint, firstEndPoint, secondStartPoint, secondEndPoint;
                    if (getFirstSecondPoints(listOfOffsetGeoIds[i], firstStartPoint, firstEndPoint) && getFirstSecondPoints(listOfOffsetGeoIds[j], secondStartPoint, secondEndPoint)) {
                        bool create = false;
                        int posi = 1;
                        int posj = 1;

                        if ((firstStartPoint - secondStartPoint).Length() < Precision::Confusion()) {
                            create = true;
                        }
                        else if ((firstStartPoint - secondEndPoint).Length() < Precision::Confusion()) {
                            create = true;
                            posi = 1;
                            posj = 2;
                        }
                        else if ((firstEndPoint - secondStartPoint).Length() < Precision::Confusion()) {
                            create = true;
                            posi = 2;
                            posj = 1;
                        }
                        else if ((firstEndPoint - secondEndPoint).Length() < Precision::Confusion()) {
                            create = true;
                            posi = 2;
                            posj = 2;
                        }
                        if (create) {
                            bool tangent = needTangent(listOfOffsetGeoIds[i], listOfOffsetGeoIds[j], posi, posj);
                            if (tangent) {
                                stream << "conList.append(Sketcher.Constraint('Tangent'," << listOfOffsetGeoIds[i] << "," << posi << ", " << listOfOffsetGeoIds[j] << "," << posj << "))\n";
                            }
                            else {
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << listOfOffsetGeoIds[i] << "," << posi << ", " << listOfOffsetGeoIds[j] << "," << posj << "))\n";
                            }
                        }
                    }
                }
            }

            Base::Console().Warning("after create constrain for \n");
            stream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
            stream << "del conList\n";
            Gui::Command::doCommand(Gui::Command::Doc, stream.str().c_str());
            //We have to doCommand here even if we makeOffsetConstraint later because we'll have to know if there're tangents.

            //Delete original geometries if necessary
            if (deleteOriginal) {

                std::stringstream stream;
                for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
                    stream << listOfGeoIds[j] << ",";
                }
                stream << listOfGeoIds[listOfGeoIds.size() - 1];
                try {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                }
            }
            else {
                Base::Console().Warning("hbefore makeoffsetconstr \n");
                std::stringstream stream2;
                stream2 << "conList = []\n";
                if (offsetConstraint) {
                    vCCO = generatevCC(listOfOffsetGeoIds);
                    makeOffsetConstraint(stream2);
                }
                stream2 << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
                stream2 << "del conList\n";
                Gui::Command::doCommand(Gui::Command::Doc, stream2.str().c_str());
            }

            Gui::Command::commitCommand();
        }
    }

    bool needTangent(int geoId1, int geoId2, int pos1, int pos2) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const Part::Geometry* geo1 = Obj->getGeometry(geoId1);
        const Part::Geometry* geo2 = Obj->getGeometry(geoId2);

        if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() || geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            Base::Vector3d perpendicular1, perpendicular2, p1, p2;
            if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle = static_cast<const Part::GeomArcOfConic*>(geo1);
                p1 = arcOfCircle->getEndPoint();
                if (pos1 == 1) {
                    p1 = arcOfCircle->getStartPoint();
                }
                perpendicular1.x = -(arcOfCircle->getCenter() - p1).y;
                perpendicular1.y = (arcOfCircle->getCenter() - p1).x;
            }
            else if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo1);
                p1 = line->getEndPoint();
                perpendicular1 = line->getStartPoint() - line->getEndPoint();
                if (pos1 == 1) {
                    p1 = line->getStartPoint();
                    perpendicular1 = line->getEndPoint() - line->getStartPoint();
                }
            }
            else { return false; }
            //Todo: add cases for arcOfellipse parabolas hyperbolas bspline
            if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                const Part::GeomArcOfConic* arcOfCircle = static_cast<const Part::GeomArcOfConic*>(geo2);
                p2 = arcOfCircle->getEndPoint();
                if (pos2 == 1) {
                    p2 = arcOfCircle->getStartPoint();
                }
                perpendicular2.x = -(arcOfCircle->getCenter() - p2).y;
                perpendicular2.y = (arcOfCircle->getCenter() - p2).x;
            }
            else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo2);
                p2 = line->getEndPoint();
                perpendicular2 = line->getStartPoint() - line->getEndPoint();
                if (pos2 == 1) {
                    p2 = line->getStartPoint();
                    perpendicular2 = line->getEndPoint() - line->getStartPoint();
                }
            }
            else { return false; }
            //Todo: add cases for arcOfellipse parabolas hyperbolas bspline

            //if lines are parallel
            if ((perpendicular1 % perpendicular2).Length() < Precision::Intersection()) {
                return true;
            }
        }
        return false;
    }

    void makeOffsetConstraint(std::stringstream& stream) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        int newCurveCounter = 0;
        int prevCurveCounter = 0;
        std::vector<Part::Geometry*> geometriesToAdd;
        for (size_t i = 0; i < vCCO.size(); i++) {
            //Check if curve is closed. Note as we use pipe it should always be closed but in case we enable 'Skin' in the future.
            bool isCurveClosed = false;
            if (vCCO[i].size() > 2) {
                CoincidencePointPos cpp = checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
                if (cpp.FirstGeoPos != Sketcher::PointPos::none)
                    isCurveClosed = true;
            }
            else if (vCCO[i].size() == 2) {
                //if only 2 elements, we need to check that they don't close end to end.
                CoincidencePointPos cpp = checkForCoincidence(vCCO[i][0], vCCO[i][vCCO[i].size() - 1]);
                if (cpp.FirstGeoPos != Sketcher::PointPos::none) {
                    if (cpp.SecondCoincidenceFirstGeoPos != Sketcher::PointPos::none) {
                        isCurveClosed = true;
                    }
                }
            }
            bool atLeastOneLine = false;
            bool reRunForFirst = false;
            bool inTangentGroup = false;

            for (size_t j = 0; j < vCCO[i].size(); j++) {

                //Tangent constraint is constraining the offset already. So if there are tangents we should not create the construction lines. Hence the code below.
                bool createLine = true;
                bool forceCreate = false;
                if (!inTangentGroup && (!isCurveClosed || j != 0 || reRunForFirst)) {
                    createLine = true;
                    atLeastOneLine = true;
                }
                else { //include case of j == 0 and closed curve, because if required the line will be made after last.
                    createLine = false;
                }

                if (j + 1 < vCCO[i].size()) {
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j], vCCO[i][j + 1], true);//true is tangentOnly
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        inTangentGroup = true;
                    }
                    else {
                        inTangentGroup = false;
                    }
                }
                else if (j == vCCO[i].size() - 1 && isCurveClosed) {//Case of last geoId for closed curves.
                    CoincidencePointPos ppc = checkForCoincidence(vCCO[i][j], vCCO[i][0], true);
                    if (ppc.FirstGeoPos != Sketcher::PointPos::none) {
                        if (!atLeastOneLine) { //We need at least one line
                            createLine = true;
                            forceCreate = true;
                        }
                    }
                    else {
                        //create line for j = 0. For this we rerun the for at j=0 after this run. With an escape bool.
                        reRunForFirst = true;
                        inTangentGroup = false;
                    }
                }

                Base::Console().Warning("i-j : %d-%d / vCCO[i][j] : %d / / createLine %d / reRunForFirst %d\n", i, j, vCCO[i][j], createLine, reRunForFirst);
                const Part::Geometry* geo = Obj->getGeometry(vCCO[i][j]);
                for (size_t k = 0; k < listOfGeoIds.size(); k++) {
                    //Check if listOfGeoIds[k] is the offseted curve giving curve i-j.
                    const Part::Geometry* geo2 = Obj->getGeometry(listOfGeoIds[k]);

                    if (geo->getTypeId() == Part::GeomCircle::getClassTypeId() && geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                        const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(geo);
                        const Part::GeomCircle* circle2 = static_cast<const Part::GeomCircle*>(geo2);
                        Base::Vector3d p1 = circle->getCenter();
                        Base::Vector3d p2 = circle2->getCenter();
                        if ((p1 - p2).Length() < Precision::Confusion()) {
                            //coincidence of center
                            stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                            //Create line between both circles.
                            Part::GeomLineSegment* line = new Part::GeomLineSegment();
                            p1.x = p1.x + circle->getRadius();
                            p2.x = p2.x + circle2->getRadius();
                            line->setPoints(p1, p2);
                            GeometryFacade::setConstruction(line, true);
                            geometriesToAdd.push_back(line);
                            newCurveCounter++;
                            stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                            stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                            break;
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId() && geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                        //same as circle but 2 lines
                    }
                    else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo);
                        const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);
                        Base::Vector3d p1[2], p2[2];
                        p1[0] = lineSeg1->getStartPoint();
                        p1[1] = lineSeg1->getEndPoint();
                        p2[0] = lineSeg2->getStartPoint();
                        p2[1] = lineSeg2->getEndPoint();
                        //if lines are parallel
                        if (((p1[1] - p1[0]) % (p2[1] - p2[0])).Length() < Precision::Intersection()) {
                            //If the lines are space by offsetLength distance
                            Base::Vector3d projectedP;
                            projectedP.ProjectToLine(p1[0] - p2[0], p2[1] - p2[0]);

                            if ((projectedP).Length() - fabs(offsetLength) < Precision::Confusion()) {
                                if (!forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Parallel'," << vCCO[i][j] << ", " << listOfGeoIds[k] << "))\n";
                                }

                                //We don't need a construction line if the line has a tangent at one end. Unless it's the first line that we're making.
                                if (createLine) {
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    line->setPoints(p1[0], p1[0] + projectedP);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;

                                    stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                        }
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                        //multiple cases because arc join mode creates arcs or circle.
                        const Part::GeomArcOfCircle* arcOfCircle = static_cast<const Part::GeomArcOfCircle*>(geo);
                        Base::Vector3d p1 = arcOfCircle->getCenter();

                        if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle* arcOfCircle2 = static_cast<const Part::GeomArcOfCircle*>(geo2);
                            Base::Vector3d p2 = arcOfCircle2->getCenter();
                            Base::Vector3d p3 = arcOfCircle2->getStartPoint();
                            Base::Vector3d p4 = arcOfCircle2->getEndPoint();

                            if ((p1 - p2).Length() < Precision::Confusion()) {
                                //coincidence of center. Offset arc is the offset of an arc
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ",3))\n";
                                if (createLine) {
                                    //Create line between both circles.
                                    Part::GeomLineSegment* line = new Part::GeomLineSegment();
                                    p1.x = p1.x + arcOfCircle->getRadius();
                                    p2.x = p2.x + arcOfCircle2->getRadius();
                                    line->setPoints(p1, p2);
                                    GeometryFacade::setConstruction(line, true);
                                    geometriesToAdd.push_back(line);
                                    newCurveCounter++;
                                    stream << "conList.append(Sketcher.Constraint('Perpendicular'," << getHighestCurveIndex() + newCurveCounter << ", " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",1, " << vCCO[i][j] << "))\n";
                                    stream << "conList.append(Sketcher.Constraint('PointOnObject'," << getHighestCurveIndex() + newCurveCounter << ",2, " << listOfGeoIds[k] << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p3).Length() < Precision::Confusion()) {
                                //coincidence of center to startpoint. offset arc is created arc join
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 1))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius'," << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }
                            else if ((p1 - p4).Length() < Precision::Confusion()) {
                                //coincidence of center to startpoint
                                stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 2))\n";

                                if (forceCreate) {
                                    stream << "conList.append(Sketcher.Constraint('Radius'," << vCCO[i][j] << ", " << offsetLength << "))\n";
                                }
                                break;
                            }

                        }
                        else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                            || geo2->getTypeId() == Part::GeomArcOfConic::getClassTypeId()
                            || geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            //cases where arc is created by arc join mode.
                            Base::Vector3d p2, p3;

                            if (getFirstSecondPoints(listOfGeoIds[k], p2, p3)) {
                                if (((p1 - p2).Length() < Precision::Confusion()) || ((p1 - p3).Length() < Precision::Confusion())) {
                                    if ((p1 - p2).Length() < Precision::Confusion()) {
                                        //coincidence of center to startpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 1))\n";
                                    }
                                    else if ((p1 - p3).Length() < Precision::Confusion()) {
                                        //coincidence of center to endpoint
                                        stream << "conList.append(Sketcher.Constraint('Coincident'," << vCCO[i][j] << ",3, " << listOfGeoIds[k] << ", 2))\n";
                                    }
                                    break;
                                }
                            }

                        }

                    }
                    else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                        //const Part::GeomArcOfEllipse* arcOfEllipse = static_cast<const Part::GeomArcOfEllipse*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                        //const Part::GeomArcOfHyperbola* arcOfHyperbola = static_cast<const Part::GeomArcOfHyperbola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() && geo2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                        //const Part::GeomArcOfParabola* arcOfParabola = static_cast<const Part::GeomArcOfParabola*>(geo2);
                    }
                    else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() && geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    }

                }
                if (newCurveCounter != prevCurveCounter) {
                    prevCurveCounter = newCurveCounter;
                    if (newCurveCounter != 1) {
                        stream << "conList.append(Sketcher.Constraint('Equal'," << getHighestCurveIndex() + newCurveCounter << ", " << getHighestCurveIndex() + 1 << "))\n";
                    }
                }

                if (reRunForFirst) {
                    if (j != 0) {
                        j = -1;
                    }// j will be incremented to 0 after new loop
                    else {
                        break;
                    }
                }
            }
        }
        if (newCurveCounter != 0) {
            stream << "conList.append(Sketcher.Constraint('Distance'," << getHighestCurveIndex() + 1 << ", " << offsetLength << "))\n";
        }
        Obj->addGeometry(std::move(geometriesToAdd));
    }

    std::vector<std::vector<int>> generatevCC(std::vector<int>& listOfGeo) {
        //This function separates all the selected geometries into separate continuous curves.
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        std::vector<std::vector<int>> vcc;

        for (size_t i = 0; i < listOfGeo.size(); i++) {
            std::vector<int> vecOfGeoIds;
            const Part::Geometry* geo = Obj->getGeometry(listOfGeo[i]);
            if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                vecOfGeoIds.push_back(listOfGeo[i]);
                vcc.push_back(vecOfGeoIds);
            }
            else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                || geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
                || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
                || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                bool inserted = 0;
                int insertedIn = -1;
                //Base::Console().Warning("Inserting : %d\n", listOfGeo[i]);
                for (size_t j = 0; j < vcc.size(); j++) {
                    //Base::Console().Warning("curve : %d\n", j);
                    for (size_t k = 0; k < vcc[j].size(); k++) {
                        //Base::Console().Warning("edge : %d ", vcc[j][k]);
                        CoincidencePointPos pointPosOfCoincidence = checkForCoincidence(listOfGeo[i], vcc[j][k]);
                        if (pointPosOfCoincidence.FirstGeoPos != Sketcher::PointPos::none) {
                            if (inserted && insertedIn != int(j)) {
                                //if it's already inserted in another continuous curve then we need to merge both curves together.
                                //There're 2 cases, it could have been inserted at the end or at the beginning.
                                if (vcc[insertedIn][0] == listOfGeo[i]) {
                                    //Two cases. Either the coincident is at the beginning or at the end.
                                    if (k == 0) {
                                        std::reverse(vcc[j].begin(), vcc[j].end());
                                    }
                                    vcc[j].insert(vcc[j].end(), vcc[insertedIn].begin(), vcc[insertedIn].end());
                                    vcc.erase(vcc.begin() + insertedIn);
                                }
                                else {
                                    if (k != 0) { //ie k is  vcc[j].size()-1
                                        std::reverse(vcc[j].begin(), vcc[j].end());
                                    }
                                    vcc[insertedIn].insert(vcc[insertedIn].end(), vcc[j].begin(), vcc[j].end());
                                    vcc.erase(vcc.begin() + j);
                                }
                                j--;
                                //Base::Console().Warning("Removing vector : %d ", j);
                            }
                            else {
                                //we need to get the curves in the correct order.
                                if (k == vcc[j].size() - 1) {
                                    vcc[j].push_back(listOfGeo[i]);
                                    //Base::Console().Warning("inserted at the end in : %d ", j);
                                }
                                else {
                                    //in this case k should actually be 0.
                                    vcc[j].insert(vcc[j].begin() + k, listOfGeo[i]);
                                    //Base::Console().Warning("inserted after %d in : %d ", k, j);
                                }
                                insertedIn = j;
                                inserted = 1;
                            }
                            //Base::Console().Warning("\n");
                            //printCCeVec();
                            break;
                        }
                    }
                }
                if (!inserted) {
                    vecOfGeoIds.push_back(listOfGeo[i]);
                    vcc.push_back(vecOfGeoIds);
                }
            }
        }
        Base::Console().Warning("vcc.size() : %d\n", vcc.size());
        return vcc;
    }

    void generateSourceWires() {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        for (size_t i = 0; i < vCC.size(); i++) {
            BRepBuilderAPI_MakeWire mkWire;
            for (size_t j = 0; j < vCC[i].size(); j++) {
                Base::Console().Warning("j %d\n", j);
                mkWire.Add(TopoDS::Edge(Obj->getGeometry(vCC[i][j])->toShape()));
            }
            if (BRep_Tool::IsClosed(mkWire.Wire())) {
                Base::Console().Warning("wire %d closed\n", i);
            }
            sourceWires.push_back(mkWire.Wire());
        }
    }

    void findOffsetLength() {
        double newOffsetLength = 1000000000000;
        BRepBuilderAPI_MakeVertex mkVertex({ endpoint.x, endpoint.y, 0.0 });
        TopoDS_Vertex vertex = mkVertex.Vertex();
        for (size_t i = 0; i < sourceWires.size(); i++) {
            BRepExtrema_DistShapeShape distTool(sourceWires[i], vertex);
            if (distTool.IsDone()) {
                double distance = distTool.Value();
                if (distance == min(distance, newOffsetLength)) {
                    newOffsetLength = distance;

                    //find direction
                    if (BRep_Tool::IsClosed(sourceWires[i])) {
                        TopoDS_Face aFace = BRepBuilderAPI_MakeFace(sourceWires[i]);
                        BRepClass_FaceClassifier checkPoint(aFace, { endpoint.x, endpoint.y, 0.0 }, Precision::Confusion());
                        if (checkPoint.State() == TopAbs_IN)
                            newOffsetLength = -newOffsetLength;
                    }
                }
            }
        }

        if (newOffsetLength != 1000000000000) {
            offsetLength = newOffsetLength;
        }
    }

    bool getSnapPoint(Base::Vector2d& snapPoint) {
        int pointGeoId = GeoEnum::GeoUndef;
        Sketcher::PointPos pointPosId = Sketcher::PointPos::none;
        int VtId = getPreselectPoint();
        int CrsId = getPreselectCross();
        if (CrsId == 0) {
            pointGeoId = Sketcher::GeoEnum::RtPnt;
            pointPosId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, pointGeoId, pointPosId);
        }
        if (pointGeoId != GeoEnum::GeoUndef && pointGeoId < firstCurveCreated) {
            //don't want to snap to the point of a geometry which is being previewed!
            auto sk = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());
            snapPoint.x = sk->getPoint(pointGeoId, pointPosId).x;
            snapPoint.y = sk->getPoint(pointGeoId, pointPosId).y;
            return true;
        }
        return false;
    }

    bool getFirstSecondPoints(int geoId, Base::Vector3d& startPoint, Base::Vector3d& endPoint) {
        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId);

        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo);
            startPoint = line->getStartPoint();
            endPoint = line->getEndPoint();
            return true;
        }
        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
            || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            const Part::GeomArcOfConic* arcOfConic = static_cast<const Part::GeomArcOfConic*>(geo);
            startPoint = arcOfConic->getStartPoint();
            endPoint = arcOfConic->getEndPoint();
            return true;
        }
        else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve* bSpline = static_cast<const Part::GeomBSplineCurve*>(geo);
            startPoint = bSpline->getStartPoint();
            endPoint = bSpline->getEndPoint();
            return true;
        }
        return false;
    }

    CoincidencePointPos checkForCoincidence(int GeoId1, int GeoId2, bool tangentOnly = false) {
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        const std::vector< Sketcher::Constraint* >& vals = Obj->Constraints.getValues();
        CoincidencePointPos positions;
        positions.FirstGeoPos = Sketcher::PointPos::none;
        positions.SecondGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceFirstGeoPos = Sketcher::PointPos::none;
        positions.SecondCoincidenceSecondGeoPos = Sketcher::PointPos::none;
        bool firstCoincidenceFound = 0;
        for (std::vector< Sketcher::Constraint* >::const_iterator it = vals.begin(); it != vals.end(); ++it) {
            if ((!tangentOnly && (*it)->Type == Sketcher::Coincident) || (*it)->Type == Sketcher::Tangent) {
                if ((*it)->First == GeoId1 && (*it)->FirstPos != Sketcher::PointPos::mid && (*it)->FirstPos != Sketcher::PointPos::none
                    && (*it)->Second == GeoId2 && (*it)->SecondPos != Sketcher::PointPos::mid && (*it)->SecondPos != Sketcher::PointPos::none) {
                    if (!firstCoincidenceFound) {
                        positions.FirstGeoPos = (*it)->FirstPos;
                        positions.SecondGeoPos = (*it)->SecondPos;
                        firstCoincidenceFound = 1;
                    }
                    else {
                        positions.SecondCoincidenceFirstGeoPos = (*it)->FirstPos;
                        positions.SecondCoincidenceSecondGeoPos = (*it)->SecondPos;
                    }
                }
                else if ((*it)->First == GeoId2 && (*it)->FirstPos != Sketcher::PointPos::mid && (*it)->FirstPos != Sketcher::PointPos::none
                    && (*it)->Second == GeoId1 && (*it)->SecondPos != Sketcher::PointPos::mid && (*it)->SecondPos != Sketcher::PointPos::none) {
                    if (!firstCoincidenceFound) {
                        positions.FirstGeoPos = (*it)->SecondPos;
                        positions.SecondGeoPos = (*it)->FirstPos;
                        firstCoincidenceFound = 1;
                    }
                    else {
                        positions.SecondCoincidenceFirstGeoPos = (*it)->SecondPos;
                        positions.SecondCoincidenceSecondGeoPos = (*it)->FirstPos;
                    }
                }
            }
        }
        return positions;
    }

    Base::Vector2d vec3dTo2d(Base::Vector3d pointToProcess) {
        Base::Vector2d pointToReturn;
        pointToReturn.x = pointToProcess.x;
        pointToReturn.y = pointToProcess.y;
        return pointToReturn;
    }

    //debug only
    void printCCeVec() {
        for (size_t j = 0; j < vCC.size(); j++) {
            Base::Console().Warning("curve %d{", j);
            for (size_t k = 0; k < vCC[j].size(); k++) {
                Base::Console().Warning("%d, ", vCC[j][k]);
            }
            Base::Console().Warning("}\n");
        }
    }
};

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_offset", "Offset length"));

    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_offset", "Delete original geometries"));
    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_offset", "Add offset constraint"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("Offset_1", "Positive offset length is outward, negative inward."));
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::setComboBoxesElements() {
    auto dHandler = static_cast<DrawSketchHandlerOffset*>(handler);
    /*This if is because when the construction mode change by adaptDrawingToComboboxChange, we call reset to change nParameter.
    But doing so also triggers this function which re-initialize the combo box. Meaning that it reset the combobox index to 0.
    The following if enables to setComboBoxesElements only if combobox index is 0 (ie if tool starts for the first time (or if tool returns to mode 0 but that's not a problem then)) */
    if (dHandler->joinMode == DrawSketchHandlerOffset::JoinMode::Arc) {
        std::string str = "Arc";
        std::string str2 = "Intersection";
        QStringList names;
        names << QString::fromStdString(str) << QString::fromStdString(str2);
        toolWidget->setComboboxElements(0, names);
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    auto dHandler = static_cast<DrawSketchHandlerOffset*>(handler);
    switch (parameterindex) {
    case WParameter::First:
        dHandler->offsetLengthSet = true;
        dHandler->offsetLength = value;
        break;
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    auto dHandler = static_cast<DrawSketchHandlerOffset*>(handler);
    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->deleteOriginal = value;
        break;
    case WCheckbox::SecondBox:
        dHandler->offsetConstraint = value;
        break;
    }
    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::adaptDrawingToComboboxChange(int comboboxindex, int value) {
    Q_UNUSED(comboboxindex)
    auto dHandler = static_cast<DrawSketchHandlerOffset*>(handler);

    if (value == 0) {
        dHandler->joinMode = DrawSketchHandlerOffset::JoinMode::Arc;
    }
    else {
        dHandler->joinMode = DrawSketchHandlerOffset::JoinMode::Intersection;
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::doOverrideSketchPosition(Base::Vector2d& onSketchPos) {
    //Too hard to override onsketchpos such that it is at offsetLength from the curve. So we use offsetLengthSet to prevent rewrite of offsetLength.

    prevCursorPosition = onSketchPos;
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::updateVisualValues(Base::Vector2d onSketchPos) {
    Q_UNUSED(onSketchPos)
    auto dHandler = static_cast<DrawSketchHandlerOffset*>(handler);

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, dHandler->offsetLength);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerOffsetBase::ToolWidgetManager::addConstraints() {
    //none
}

DEF_STD_CMD_A(CmdSketcherOffset)

CmdSketcherOffset::CmdSketcherOffset()
    : Command("Sketcher_Offset")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Offset geometries");
    sToolTipText = QT_TR_NOOP("Offset selected geometries.");
    sWhatsThis = "Sketcher_Offset";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Offset";
    sAccel = "O";
    eType = ForEdit;
}

void CmdSketcherOffset::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<int> listOfGeoIds = {};

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (!SubNames.empty()) {
        //Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
            // only handle non-external edges
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int geoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                if (geoId >= 0) {
                    listOfGeoIds.push_back(geoId);
                }
            }
        }
    }

    //getSelection().clearSelection();

    ActivateAcceleratorHandler(getActiveGuiDocument(), new DrawSketchHandlerOffset(listOfGeoIds));
}

bool CmdSketcherOffset::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}


// ================================================================================

DEF_STD_CMD_A(CmdSketcherDeleteAllGeometry)

CmdSketcherDeleteAllGeometry::CmdSketcherDeleteAllGeometry()
    :Command("Sketcher_DeleteAllGeometry")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Delete all geometry");
    sToolTipText    = QT_TR_NOOP("Delete all geometry and constraints in the current sketch, "
                                 "with the exception of external geometry");
    sWhatsThis      = "Sketcher_DeleteAllGeometry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_DeleteGeometry";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherDeleteAllGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Delete All Geometry"),
                                    QObject::tr("Are you really sure you want to delete all geometry and constraints?"),
                                    QMessageBox::Yes, QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        getSelection().clearSelection();
        Gui::Document * doc= getActiveGuiDocument();
        ReleaseHandler(doc);
        SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        Sketcher::SketchObject* Obj= vp->getSketchObject();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Delete all geometry"));
            Gui::cmdAppObjectArgs(Obj, "deleteAllGeometry()");
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to delete all geometry: %s\n", e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute", false);

        if (autoRecompute)
            Gui::Command::updateActive();
        else
            Obj->solve();
    }
    else if (ret == QMessageBox::Cancel) {
        // do nothing
        return;
    }
}

bool CmdSketcherDeleteAllGeometry::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================

DEF_STD_CMD_A(CmdSketcherDeleteAllConstraints)

CmdSketcherDeleteAllConstraints::CmdSketcherDeleteAllConstraints()
    :Command("Sketcher_DeleteAllConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Delete all constraints");
    sToolTipText    = QT_TR_NOOP("Delete all constraints in the sketch");
    sWhatsThis      = "Sketcher_DeleteAllConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_DeleteConstraints";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherDeleteAllConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Delete All Constraints"),
                                    QObject::tr("Are you really sure you want to delete all the constraints?"),
                                    QMessageBox::Yes, QMessageBox::Cancel);

    if (ret == QMessageBox::Yes) {
        getSelection().clearSelection();
        Gui::Document * doc= getActiveGuiDocument();
        ReleaseHandler(doc);
        SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        Sketcher::SketchObject* Obj= vp->getSketchObject();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Delete All Constraints"));
            Gui::cmdAppObjectArgs(Obj, "deleteAllConstraints()");
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to delete All Constraints: %s\n", e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

        if (autoRecompute)
            Gui::Command::updateActive();
        else
            Obj->solve();
    }
    else if (ret == QMessageBox::Cancel) {
        // do nothing
        return;
    }

}

bool CmdSketcherDeleteAllConstraints::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), false);
}

// ================================================================================


DEF_STD_CMD_A(CmdSketcherRemoveAxesAlignment)

CmdSketcherRemoveAxesAlignment::CmdSketcherRemoveAxesAlignment()
    :Command("Sketcher_RemoveAxesAlignment")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Remove axes alignment");
    sToolTipText    = QT_TR_NOOP("Modifies constraints to remove axes alignment while trying to preserve the constraint relationship of the selection");
    sWhatsThis      = "Sketcher_RemoveAxesAlignment";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_RemoveAxesAlignment";
    sAccel          = "Z, R";
    eType           = ForEdit;
}

void CmdSketcherRemoveAxesAlignment::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    getSelection().clearSelection();

    int LastGeoId = 0;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            // lines to copy
            if (LastGeoId >= 0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            if (Obj->getGeometry(GeoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                LastGeoId = GeoId;
                // points to copy
                if (LastGeoId >= 0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    if (geoids < 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Removal of axes alignment requires at least one selected non-external geometric element"));
        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0, 1, '[');
    geoIdList.append(1, ']');

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove Axes Alignment"));

    try {
        Gui::cmdAppObjectArgs(  Obj,
                                "removeAxesAlignment(%s)",
                                geoIdList.c_str());
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        Gui::Command::abortCommand();
    }

    tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(Obj));

}

bool CmdSketcherRemoveAxesAlignment::isActive(void)
{
    return isSketcherAcceleratorActive(getActiveGuiDocument(), true);
}

void CreateSketcherCommandsConstraintAccel(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherCloseShape());
    rcCmdMgr.addCommand(new CmdSketcherConnect());
    rcCmdMgr.addCommand(new CmdSketcherSelectConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectOrigin());
    rcCmdMgr.addCommand(new CmdSketcherSelectVerticalAxis());
    rcCmdMgr.addCommand(new CmdSketcherSelectHorizontalAxis());
    rcCmdMgr.addCommand(new CmdSketcherSelectRedundantConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectConflictingConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectMalformedConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectPartiallyRedundantConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsAssociatedWithConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsWithDoFs());
    rcCmdMgr.addCommand(new CmdSketcherRestoreInternalAlignmentGeometry());
    rcCmdMgr.addCommand(new CmdSketcherRotate());
    rcCmdMgr.addCommand(new CmdSketcherScale());
    rcCmdMgr.addCommand(new CmdSketcherOffset());
    rcCmdMgr.addCommand(new CmdSketcherSymmetry());
    rcCmdMgr.addCommand(new CmdSketcherCopy());
    rcCmdMgr.addCommand(new CmdSketcherClone());
    rcCmdMgr.addCommand(new CmdSketcherMove());
    rcCmdMgr.addCommand(new CmdSketcherCompCopy());
    rcCmdMgr.addCommand(new CmdSketcherRectangularArray());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllGeometry());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllConstraints());
    rcCmdMgr.addCommand(new CmdSketcherRemoveAxesAlignment());
}
