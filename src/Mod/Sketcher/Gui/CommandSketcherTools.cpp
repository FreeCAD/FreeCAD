/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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
# include <QMessageBox>
# include <Precision.hxx>
# include <QApplication>
# include <QMessageBox>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>

#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"
#include "SketchRectangularArrayDialog.h"
#include "CommandConstraints.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isSketcherAcceleratorActive(Gui::Document *doc, bool actsOnSelection )
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            if (static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode() == ViewProviderSketch::STATUS_NONE) {
                if (!actsOnSelection)
                    return true;
                else if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0)
                    return true;
            }
        }
    }

    return false;
}

void ActivateAcceleratorHandler(Gui::Document *doc,DrawSketchHandler *handler)
{
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom
           (SketcherGui::ViewProviderSketch::getClassTypeId())) {

            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());
            vp->purgeHandler();
            vp->activateHandler(handler);
        }
    }
}

// Close Shape Command
DEF_STD_CMD_A(CmdSketcherCloseShape);

CmdSketcherCloseShape::CmdSketcherCloseShape()
    :Command("Sketcher_CloseShape")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Close Shape");
    sToolTipText    = QT_TR_NOOP("Produce closed shape by Link end point of element with next elements' starting point");
    sWhatsThis      = "Sketcher_CloseShape";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CloseShape";
    sAccel          = "CTRL+SHIFT+S";
    eType           = ForEdit;
}

void CmdSketcherCloseShape::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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

    int GeoIdFirst=-1;
    int GeoIdLast=-1;

    // undo command open
    openCommand("add coincident constraint");
    // go through the selected subelements
    for (unsigned int i=0; i<(SubNames.size()-1); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge" &&
            SubNames[i+1].size() > 4 && SubNames[i+1].substr(0,4) == "Edge") {

            int GeoId1 = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;
            int GeoId2 = std::atoi(SubNames[i+1].substr(4,4000).c_str()) - 1;

            if(GeoIdFirst==-1)
              GeoIdFirst=GeoId1;

            GeoIdLast=GeoId2;

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

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,Sketcher::end,GeoId2,Sketcher::start);
        }
    }

    // Close Last Edge with First Edge
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
        selection[0].getFeatName(),GeoIdLast,Sketcher::end,GeoIdFirst,Sketcher::start);

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherCloseShape::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}


// Connect Edges Command
DEF_STD_CMD_A(CmdSketcherConnect);

CmdSketcherConnect::CmdSketcherConnect()
    :Command("Sketcher_ConnectLines")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Connect Edges");
    sToolTipText    = QT_TR_NOOP("Link end point of element with next elements' starting point");
    sWhatsThis      = "Sketcher_ConnectLines";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ConnectLines";
    sAccel          = "CTRL+SHIFT+K";
    eType           = ForEdit;
}

void CmdSketcherConnect::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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
    openCommand("add coincident constraint");

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

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,Sketcher::end,GeoId2,Sketcher::start);
        }
    }

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConnect::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

// Select Constraints of selected elements
DEF_STD_CMD_A(CmdSketcherSelectConstraints);

CmdSketcherSelectConstraints::CmdSketcherSelectConstraints()
    :Command("Sketcher_SelectConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Constraints");
    sToolTipText    = QT_TR_NOOP("Select the constraints associated to the selected elements");
    sWhatsThis      = "Sketcher_SelectConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectConstraints";
    sAccel          = "CTRL+SHIFT+C";
    eType           = ForEdit;
}

void CmdSketcherSelectConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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

    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            // push all the constraints
            int i = 0;
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it,++i) {
                if ( (*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId){
                  Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), Sketcher::PropertyConstraintList::getConstraintName(i).c_str());
                }
            }
        }
    }
}

bool CmdSketcherSelectConstraints::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

// Select Origin
DEF_STD_CMD_A(CmdSketcherSelectOrigin);

CmdSketcherSelectOrigin::CmdSketcherSelectOrigin()
    :Command("Sketcher_SelectOrigin")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Origin");
    sToolTipText    = QT_TR_NOOP("Select the origin point");
    sWhatsThis      = "Sketcher_SelectOrigin";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectOrigin";
    sAccel          = "CTRL+SHIFT+O";
    eType           = ForEdit;
}

void CmdSketcherSelectOrigin::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();

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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

// Select Vertical Axis
DEF_STD_CMD_A(CmdSketcherSelectVerticalAxis);

CmdSketcherSelectVerticalAxis::CmdSketcherSelectVerticalAxis()
    :Command("Sketcher_SelectVerticalAxis")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Vertical Axis");
    sToolTipText    = QT_TR_NOOP("Select the vertical axis");
    sWhatsThis      = "Sketcher_SelectVerticalAxis";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectVerticalAxis";
    sAccel          = "CTRL+SHIFT+V";
    eType           = ForEdit;
}

void CmdSketcherSelectVerticalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();

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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

// Select Horizontal Axis
DEF_STD_CMD_A(CmdSketcherSelectHorizontalAxis);

CmdSketcherSelectHorizontalAxis::CmdSketcherSelectHorizontalAxis()
    :Command("Sketcher_SelectHorizontalAxis")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Horizontal Axis");
    sToolTipText    = QT_TR_NOOP("Select the horizontal axis");
    sWhatsThis      = "Sketcher_SelectHorizontalAxis";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectHorizontalAxis";
    sAccel          = "CTRL+SHIFT+H";
    eType           = ForEdit;
}

void CmdSketcherSelectHorizontalAxis::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();

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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

DEF_STD_CMD_A(CmdSketcherSelectRedundantConstraints);

CmdSketcherSelectRedundantConstraints::CmdSketcherSelectRedundantConstraints()
    :Command("Sketcher_SelectRedundantConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Redundant Constraints");
    sToolTipText    = QT_TR_NOOP("Select Redundant Constraints");
    sWhatsThis      = "Sketcher_SelectRedundantConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectRedundantConstraints";
    sAccel          = "CTRL+SHIFT+R";
    eType           = ForEdit;
}

void CmdSketcherSelectRedundantConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solverredundant = vp->getSketchObject()->getLastRedundant();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverredundant.begin();itc != solverredundant.end(); ++itc) {
            if ( (*itc) - 1 == i){
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), Sketcher::PropertyConstraintList::getConstraintName(i).c_str());
                break;
            }
        }


    }
}

bool CmdSketcherSelectRedundantConstraints::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

DEF_STD_CMD_A(CmdSketcherSelectConflictingConstraints);

CmdSketcherSelectConflictingConstraints::CmdSketcherSelectConflictingConstraints()
    :Command("Sketcher_SelectConflictingConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Conflicting Constraints");
    sToolTipText    = QT_TR_NOOP("Select Conflicting Constraints");
    sWhatsThis      = "Sketcher_SelectConflictingConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectConflictingConstraints";
    sAccel          = "CTRL+SHIFT+E";
    eType           = ForEdit;
}

void CmdSketcherSelectConflictingConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    // get the needed lists and objects
    const std::vector< int > &solverconflicting = vp->getSketchObject()->getLastConflicting();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    // push the constraints
    int i = 0;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverconflicting.begin();itc != solverconflicting.end(); ++itc) {
            if ( (*itc) - 1 == i){
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), Sketcher::PropertyConstraintList::getConstraintName(i).c_str());
                break;
            }
        }
    }
}

bool CmdSketcherSelectConflictingConstraints::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

DEF_STD_CMD_A(CmdSketcherSelectElementsAssociatedWithConstraints);

CmdSketcherSelectElementsAssociatedWithConstraints::CmdSketcherSelectElementsAssociatedWithConstraints()
    :Command("Sketcher_SelectElementsAssociatedWithConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select Elements associated with constraints");
    sToolTipText    = QT_TR_NOOP("Select Elements associated with constraints");
    sWhatsThis      = "Sketcher_SelectElementsAssociatedWithConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectElementsAssociatedWithConstraints";
    sAccel          = "CTRL+SHIFT+E";
    eType           = ForEdit;
}

void CmdSketcherSelectElementsAssociatedWithConstraints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    Sketcher::SketchObject* Obj= vp->getSketchObject();

    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    getSelection().clearSelection();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    int selected=0;

    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle constraints
        if (it->size() > 10 && it->substr(0,10) == "Constraint") {
            int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);

            if(ConstrId < static_cast<int>(vals.size())){
                if(vals[ConstrId]->First!=Constraint::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->FirstPos)
                    {
                        case Sketcher::none:
                            ss << "Edge" << vals[ConstrId]->First + 1;
                            break;
                        case Sketcher::start:
                        case Sketcher::end:
                        case Sketcher::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->First,vals[ConstrId]->FirstPos);
                            if(vertex>-1)
                                ss << "Vertex" <<  vertex + 1;
                            break;
                    }

                    Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
                    selected++;
                }

                if(vals[ConstrId]->Second!=Constraint::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->SecondPos)
                    {
                        case Sketcher::none:
                            ss << "Edge" << vals[ConstrId]->Second + 1;
                            break;
                        case Sketcher::start:
                        case Sketcher::end:
                        case Sketcher::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Second,vals[ConstrId]->SecondPos);
                            if(vertex>-1)
                                ss << "Vertex" << vertex + 1;
                            break;
                    }

                    Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
                    selected++;
                }

                if(vals[ConstrId]->Third!=Constraint::GeoUndef){
                    ss.str(std::string());

                    switch(vals[ConstrId]->ThirdPos)
                    {
                        case Sketcher::none:
                            ss << "Edge" << vals[ConstrId]->Third + 1;
                            break;
                        case Sketcher::start:
                        case Sketcher::end:
                        case Sketcher::mid:
                            int vertex = Obj->getVertexIndexGeoPos(vals[ConstrId]->Third,vals[ConstrId]->ThirdPos);
                            if(vertex>-1)
                                ss << "Vertex" <<  vertex + 1;
                            break;
                    }

                    Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
                    selected++;
                }
            }
        }
    }

    if ( selected == 0 ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No constraint selected"),
                                     QObject::tr("At least one constraint must be selected"));
    }
}

bool CmdSketcherSelectElementsAssociatedWithConstraints::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

DEF_STD_CMD_A(CmdSketcherSelectElementsWithDoFs);

CmdSketcherSelectElementsWithDoFs::CmdSketcherSelectElementsWithDoFs()
:Command("Sketcher_SelectElementsWithDoFs")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Select solver DoFs");
    sToolTipText    = QT_TR_NOOP("Select elements where the solver still detects unconstrained degrees of freedom.");
    sWhatsThis      = "Sketcher_SelectElementsWithDoFs";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_SelectElementsWithDoFs";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherSelectElementsWithDoFs::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    getSelection().clearSelection();

    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    Sketcher::SketchObject* Obj= vp->getSketchObject();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();

    std::stringstream ss;

    auto geos = Obj->getInternalGeometry();

    // Solver parameter detection algorithm only works for Dense QR with full pivoting. If we are using Sparse QR, we
    // have to re-solve using Dense QR.
    GCS::QRAlgorithm curQRAlg = Obj->getSolvedSketch().getQRAlgorithm();

    if(curQRAlg == GCS::EigenSparseQR) {
        Obj->getSolvedSketch().setQRAlgorithm(GCS::EigenDenseQR);
        Obj->solve(false);
    }


    auto testselectvertex = [&Obj,&ss,&doc_name,&obj_name](int geoId, PointPos pos){
        ss.str(std::string());

        if(Obj->getSolvedSketch().hasDependentParameters(geoId, pos)) {
            int vertex = Obj->getVertexIndexGeoPos(geoId, pos);
            if(vertex>-1) {
                ss << "Vertex" <<  vertex + 1;

                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
            }
        }
    };

    auto testselectedge = [&Obj,&ss,&doc_name,&obj_name](int geoId){
        ss.str(std::string());

        if(Obj->getSolvedSketch().hasDependentParameters(geoId, Sketcher::none)) {
            ss << "Edge" <<  geoId + 1;
            Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
        }
    };

    int geoid = 0;

    for(auto geo : geos) {
        if(geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            testselectvertex(geoid, Sketcher::start);
        }
        else if(geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            testselectvertex(geoid, Sketcher::start);
            testselectvertex(geoid, Sketcher::end);
            testselectedge(geoid);
        }
        else if(geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ) {
            testselectvertex(geoid, Sketcher::mid);
            testselectedge(geoid);
        }
        else if(geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
            testselectvertex(geoid, Sketcher::start);
            testselectvertex(geoid, Sketcher::end);
            testselectvertex(geoid, Sketcher::mid);
            testselectedge(geoid);
        }

        geoid++;
    }

    if(curQRAlg == GCS::EigenSparseQR) {
        Obj->getSolvedSketch().setQRAlgorithm(GCS::EigenSparseQR);
    }

}

bool CmdSketcherSelectElementsWithDoFs::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

DEF_STD_CMD_A(CmdSketcherRestoreInternalAlignmentGeometry);

CmdSketcherRestoreInternalAlignmentGeometry::CmdSketcherRestoreInternalAlignmentGeometry()
    :Command("Sketcher_RestoreInternalAlignmentGeometry")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/hide internal geometry");
    sToolTipText    = QT_TR_NOOP("Show all internal geometry / hide unused internal geometry");
    sWhatsThis      = "Sketcher_RestoreInternalAlignmentGeometry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Element_Ellipse_All";
    sAccel          = "CTRL+SHIFT+E";
    eType           = ForEdit;
}

void CmdSketcherRestoreInternalAlignmentGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

    getSelection().clearSelection();

    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if ( (it->size() > 4 && it->substr(0,4) == "Edge") ||
             (it->size() > 12 && it->substr(0,12) == "ExternalEdge")) {
            int GeoId;
            if(it->substr(0,4) == "Edge")
               GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            else
               GeoId = -std::atoi(it->substr(12,4000).c_str()) - 2;

            const Part::Geometry *geo = Obj->getGeometry(GeoId);
            // Only for supported types
            if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ) {

                int currentgeoid = Obj->getHighestCurveIndex();

                try {
                    Gui::Command::openCommand("Exposing Internal Geometry");
                    Gui::Command::doCommand(Gui::Command::Doc,
                        "App.ActiveDocument.%s.exposeInternalGeometry(%d)",
                        Obj->getNameInDocument(),
                        GeoId);

                    int aftergeoid = Obj->getHighestCurveIndex();

                    if(aftergeoid == currentgeoid) { // if we did not expose anything, deleteunused
                        Gui::Command::doCommand(Gui::Command::Doc,
                            "App.ActiveDocument.%s.deleteUnusedInternalGeometry(%d)",
                            Obj->getNameInDocument(),
                            GeoId);
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
    }
}

bool CmdSketcherRestoreInternalAlignmentGeometry::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

DEF_STD_CMD_A(CmdSketcherSymmetry);

CmdSketcherSymmetry::CmdSketcherSymmetry()
    :Command("Sketcher_Symmetry")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Symmetry");
    sToolTipText    = QT_TR_NOOP("Creates symmetric geometry with respect to the last selected line or point");
    sWhatsThis      = "Sketcher_Symmetry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Symmetry";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherSymmetry::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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
    Sketcher::PointPos LastPointPos = Sketcher::none;
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
                LastPointPos = Sketcher::none;
            }
            else {
                LastGeoId = -std::atoi(it->substr(12,4000).c_str()) - 2;
                LastPointPos = Sketcher::none;
            }

            // reference can be external or non-external
            LastGeo = Obj->getGeometry(LastGeoId);
            // Only for supported types
            if(LastGeo->getTypeId() == Part::GeomLineSegment::getClassTypeId())
                lastgeotype = line;
            else
                lastgeotype = invalid;

            // lines to make symmetric (only non-external)
            if(LastGeoId>=0) {
                geoids++;
                stream << LastGeoId << ",";
            }
        }
        else if(it->size() > 6 && it->substr(0,6) == "Vertex"){
            // only if it is a GeomPoint
            int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);

            if (Obj->getGeometry(GeoId)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                LastGeoId = GeoId;
                LastPointPos = Sketcher::start;
                lastgeotype = point;

                // points to make symmetric
                if(LastGeoId>=0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    bool lastvertexoraxis=false;
    // check if last selected element is a Vertex, not being a GeomPoint
    if(SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex"){
        int VtId = std::atoi(SubNames.rbegin()->substr(6,4000).c_str()) - 1;
        int GeoId;
        Sketcher::PointPos PosId;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
        if (Obj->getGeometry(GeoId)->getTypeId() != Part::GeomPoint::getClassTypeId()) {
            LastGeoId = GeoId;
            LastPointPos = PosId;
            lastgeotype = point;
            lastvertexoraxis=true;
        }
    }
    // check if last selected element is horizontal axis
    else if(SubNames.rbegin()->size() == 6 && SubNames.rbegin()->substr(0,6) == "H_Axis"){
        LastGeoId = Sketcher::GeoEnum::HAxis;
        LastPointPos = Sketcher::none;
        lastgeotype = line;
        lastvertexoraxis=true;
    }
    // check if last selected element is vertical axis
    else if(SubNames.rbegin()->size() == 6 && SubNames.rbegin()->substr(0,6) == "V_Axis"){
        LastGeoId = Sketcher::GeoEnum::VAxis;
        LastPointPos = Sketcher::none;
        lastgeotype = line;
        lastvertexoraxis=true;
    }
    // check if last selected element is the root point
    else if(SubNames.rbegin()->size() == 9 && SubNames.rbegin()->substr(0,9) == "RootPoint"){
        LastGeoId = Sketcher::GeoEnum::RtPnt;
        LastPointPos = Sketcher::start;
        lastgeotype = point;
        lastvertexoraxis=true;
    }

    if ( geoids == 0 || (geoids == 1 && LastGeoId>=0 && !lastvertexoraxis) ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("A symmetric construction requires at least two geometric elements, the last geometric element being the reference for the symmetry construction."));
        return;
    }

    if ( lastgeotype == invalid ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("The last element must be a point or a line serving as reference for the symmetry construction."));
        return;
    }

    std::string geoIdList = stream.str();

    // missing cases:
    // 1- Last element is an edge, and is V or H axis
    // 2- Last element is a point GeomPoint
    // 3- Last element is a point (Vertex)

    if(LastGeoId>=0 && !lastvertexoraxis) {
        // if LastGeoId was added remove the last element
        int index = geoIdList.rfind(',');
        index = geoIdList.rfind(',',index-1);
        geoIdList.resize(index);
    }
    else {
        int index = geoIdList.rfind(',');
        geoIdList.resize(index);
    }

    geoIdList.insert(0,1,'[');
    geoIdList.append(1,']');

    Gui::Command::openCommand("Create Symmetric geometry");

    try{
        Gui::Command::doCommand(
            Gui::Command::Doc, "App.ActiveDocument.%s.addSymmetric(%s,%d,%d)",
            Obj->getNameInDocument(), geoIdList.c_str(), LastGeoId, LastPointPos
        );

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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}


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

static const char *cursor_createcopy[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    "......................###.......",
    "......................###.......",
    "......................###.......",
    "......................###.......",
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
    "................................",
    "................................",
    "................................",
    "................................"};

    class DrawSketchHandlerCopy: public DrawSketchHandler
    {
    public:
        DrawSketchHandlerCopy(string geoidlist, int origingeoid, Sketcher::PointPos originpos, int nelements, SketcherCopy::Op op)
        : Mode(STATUS_SEEK_First)
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

        virtual void activated(ViewProviderSketch *sketchgui)
        {
            setCursor(QPixmap(cursor_createcopy),7,7);
            Origin = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getPoint(OriginGeoId, OriginPos);
            EditCurve[0] = Base::Vector2d(Origin.x,Origin.y);
        }

        virtual void mouseMove(Base::Vector2d onSketchPos)
        {
            if (Mode==STATUS_SEEK_First) {
                float length = (onSketchPos - EditCurve[0]).Length();
                float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2d(1.f,0.f));
                SbString text;
                text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.0,0.0),AutoConstraint::VERTEX)) {
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }

            }
            applyCursor();
        }

        virtual bool pressButton(Base::Vector2d onSketchPos)
        {
            if (Mode==STATUS_SEEK_First){
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                Mode = STATUS_End;
            }

            return true;
        }

        virtual bool releaseButton(Base::Vector2d onSketchPos)
        {
            Q_UNUSED(onSketchPos);
            if (Mode==STATUS_End){

                Base::Vector2d vector = EditCurve[1]-EditCurve[0];

                unsetCursor();
                resetPositionText();

                int currentgeoid = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getHighestCurveIndex();

                Gui::Command::openCommand("Copy/clone/move geometry");

                try{
                    if( Op != SketcherCopy::Move) {

                        Gui::Command::doCommand(
                            Gui::Command::Doc, "App.ActiveDocument.%s.addCopy(%s,App.Vector(%f,%f,0),%s)",
                                            sketchgui->getObject()->getNameInDocument(),
                                            geoIdList.c_str(), vector.x, vector.y,
                                            (Op == SketcherCopy::Clone?"True":"False"));
                    }
                    else {
                        Gui::Command::doCommand(
                            Gui::Command::Doc, "App.ActiveDocument.%s.addMove(%s,App.Vector(%f,%f,0))",
                            sketchgui->getObject()->getNameInDocument(),
                            geoIdList.c_str(), vector.x, vector.y);
                    }

                    Gui::Command::commitCommand();
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                    Gui::Command::abortCommand();
                }

                if( Op != SketcherCopy::Move) {
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
                sketchgui->drawEdit(EditCurve);

                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
            return true;
        }
    protected:
        SelectMode Mode;
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
    Sketcher::PointPos LastPointPos = Sketcher::none;
    const Part::Geometry *LastGeo = 0;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::none;
            LastGeo = Obj->getGeometry(LastGeoId);
            // lines to copy
            if (LastGeoId>=0) {
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
                LastPointPos = Sketcher::start;
                // points to copy
                if (LastGeoId>=0) {
                    geoids++;
                    stream << LastGeoId << ",";
                }
            }
        }
    }

    // check if last selected element is a Vertex, not being a GeomPoint
    if (SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex"){
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
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                QObject::tr("A copy requires at least one selected non-external geometric element"));
        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0,1,'[');
    geoIdList.append(1,']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the center point)
    if (LastPointPos == Sketcher::none) {
        if (LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            LastPointPos = Sketcher::mid;
            }
            else {
                LastPointPos = Sketcher::start;
            }
    }

    // Ask the user if he wants to clone or to simple copy
    /*int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Dimensional/Geometric constraints"),
        *                                   QObject::tr("Do you want to clone the object, i.e. substitute dimensional constraints by geometric constraints?"),
        *                                   QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        *   // use an equality constraint
        *   if (ret == QMessageBox::Yes) {
        *       clone = true;
}
else if (ret == QMessageBox::Cancel) {
    // do nothing
    return;
}*/

    ActivateAcceleratorHandler(getActiveGuiDocument(),new DrawSketchHandlerCopy(geoIdList, LastGeoId, LastPointPos, geoids, op));
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
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Copy");
    sToolTipText    = QT_TR_NOOP("Creates a simple copy of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Copy";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Copy";
    sAccel          = "";
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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

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
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Clone");
    sToolTipText    = QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Clone";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Clone";
    sAccel          = "";
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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
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
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Move");
    sToolTipText    = QT_TR_NOOP("Moves the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_Move";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Move";
    sAccel          = "CTRL+M";
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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

DEF_STD_CMD_ACL(CmdSketcherCompCopy);

CmdSketcherCompCopy::CmdSketcherCompCopy()
: Command("Sketcher_CompCopy")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Copy");
    sToolTipText    = QT_TR_NOOP("Creates a clone of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_CompCopy";
    sStatusTip      = sToolTipText;
    sAccel          = "CTRL+C";
    eType           = ForEdit;
}

void CmdSketcherCompCopy::activated(int iMsg)
{
    if (iMsg<0 || iMsg>2)
        return;

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());

    if (iMsg==0){
        CmdSketcherClone sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->sAccel));
    }
    else if (iMsg==1) {
        CmdSketcherCopy sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(this->sAccel));
    }
    else if (iMsg==2) {
        CmdSketcherMove sc;
        sc.activate();
        pcAction->setShortcut(QString::fromLatin1(""));
    }
    else
        return;
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

    pcAction->setShortcut(QString::fromLatin1(sAccel));

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


/* XPM */
static const char *cursor_createrectangulararray[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "................................",
    "................................",
    "................................",
    "................................",
    "................................",
    ".......................###......",
    ".......................###......",
    ".......................###......",
    ".......................###......",
    "..............###......###......",
    "..............###......###......",
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
    "................................",
    "................................",
    "................................",
    "................................"};

    class DrawSketchHandlerRectangularArray: public DrawSketchHandler
    {
    public:
        DrawSketchHandlerRectangularArray(string geoidlist, int origingeoid, Sketcher::PointPos originpos, int nelements, bool clone,
                                          int rows, int cols, bool constraintSeparation,
                                          bool equalVerticalHorizontalSpacing )
            : Mode(STATUS_SEEK_First)
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

        virtual void activated(ViewProviderSketch *sketchgui)
        {
            setCursor(QPixmap(cursor_createrectangulararray),7,7);
            Origin = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getPoint(OriginGeoId, OriginPos);
            EditCurve[0] = Base::Vector2d(Origin.x,Origin.y);
        }

        virtual void mouseMove(Base::Vector2d onSketchPos)
        {
            if (Mode==STATUS_SEEK_First) {
                float length = (onSketchPos - EditCurve[0]).Length();
                float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2d(1.f,0.f));
                SbString text;
                text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.0,0.0),AutoConstraint::VERTEX)) {
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }

            }
            applyCursor();
        }

        virtual bool pressButton(Base::Vector2d onSketchPos)
        {
            if (Mode==STATUS_SEEK_First){
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                Mode = STATUS_End;
            }

            return true;
        }

        virtual bool releaseButton(Base::Vector2d onSketchPos)
        {
            Q_UNUSED(onSketchPos);
            if (Mode==STATUS_End){

                Base::Vector2d vector = EditCurve[1]-EditCurve[0];

                unsetCursor();
                resetPositionText();

                Gui::Command::openCommand("Create copy of geometry");

                try {
                    Gui::Command::doCommand(
                        Gui::Command::Doc, "App.ActiveDocument.%s.addRectangularArray(%s, App.Vector(%f,%f,0),%s,%d,%d,%s,%f)",
                                            sketchgui->getObject()->getNameInDocument(),
                                            geoIdList.c_str(), vector.x, vector.y,
                                            (Clone?"True":"False"),
                                            Cols, Rows,
                                            (ConstraintSeparation?"True":"False"),
                                            (EqualVerticalHorizontalSpacing?1.0:0.5));

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
                sketchgui->drawEdit(EditCurve);

                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
            return true;
        }
    protected:
        SelectMode Mode;
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


DEF_STD_CMD_A(CmdSketcherRectangularArray);

CmdSketcherRectangularArray::CmdSketcherRectangularArray()
:Command("Sketcher_RectangularArray")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Rectangular Array");
    sToolTipText    = QT_TR_NOOP("Creates a rectangular array pattern of the geometry taking as reference the last selected point");
    sWhatsThis      = "Sketcher_RectangularArray";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_RectangularArray";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherRectangularArray::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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
    Sketcher::PointPos LastPointPos = Sketcher::none;
    const Part::Geometry *LastGeo = 0;

    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::none;

            LastGeo = Obj->getGeometry(LastGeoId);

            // lines to copy
            if (LastGeoId>=0) {
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
                LastPointPos = Sketcher::start;
                // points to copy
                if(LastGeoId>=0) {
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

    if ( geoids < 1 ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("A copy requires at least one selected non-external geometric element"));
        return;
    }

    std::string geoIdList = stream.str();

    // remove the last added comma and brackets to make the python list
    int index = geoIdList.rfind(',');
    geoIdList.resize(index);
    geoIdList.insert(0,1,'[');
    geoIdList.append(1,']');

    // if the last element is not a point serving as a reference for the copy process
    // then make the start point of the last element the copy reference (if it exists, if not the center point)
    if (LastPointPos == Sketcher::none) {
        if (LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            LastPointPos = Sketcher::mid;
        }
        else {
            LastPointPos = Sketcher::start;
        }
    }

    // Pop-up asking for values
    SketchRectangularArrayDialog * slad = new SketchRectangularArrayDialog();

    if (slad->exec() == QDialog::Accepted) {
        ActivateAcceleratorHandler(getActiveGuiDocument(),
            new DrawSketchHandlerRectangularArray(geoIdList, LastGeoId, LastPointPos, geoids, slad->Clone,
                                                  slad->Rows, slad->Cols, slad->ConstraintSeparation,
                                                  slad->EqualVerticalHorizontalSpacing));
    }

    delete slad;
}

bool CmdSketcherRectangularArray::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

// Select Origin
DEF_STD_CMD_A(CmdSketcherDeleteAllGeometry);

CmdSketcherDeleteAllGeometry::CmdSketcherDeleteAllGeometry()
:Command("Sketcher_DeleteAllGeometry")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Delete All Geometry");
    sToolTipText    = QT_TR_NOOP("Deletes all the geometry and constraints but external geometry");
    sWhatsThis      = "Sketcher_DeleteAllGeometry";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Element_SelectionTypeInvalid";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherDeleteAllGeometry::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Delete All Geometry"),
                                    QObject::tr("Are you really sure you want to delete all the geometry and constraints?"),
                                    QMessageBox::Yes, QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        getSelection().clearSelection();

        Gui::Document * doc= getActiveGuiDocument();

        SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

        Sketcher::SketchObject* Obj= vp->getSketchObject();

        try {
            Gui::Command::openCommand("Delete All Geometry");
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.deleteAllGeometry()",
                                    Obj->getNameInDocument());

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to delete All Geometry: %s\n", e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

        if(autoRecompute)
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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
}

DEF_STD_CMD_A(CmdSketcherDeleteAllConstraints);

CmdSketcherDeleteAllConstraints::CmdSketcherDeleteAllConstraints()
:Command("Sketcher_DeleteAllConstraints")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Delete All Constraints");
    sToolTipText    = QT_TR_NOOP("Deletes all the constraints");
    sWhatsThis      = "Sketcher_DeleteAllConstraints";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Element_SelectionTypeInvalid";
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

        SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

        Sketcher::SketchObject* Obj= vp->getSketchObject();

        try {
            Gui::Command::openCommand("Delete All Constraints");
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.deleteAllConstraints()",
                                    Obj->getNameInDocument());

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to delete All Constraints: %s\n", e.what());
            Gui::Command::abortCommand();
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

        if(autoRecompute)
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
    return isSketcherAcceleratorActive( getActiveGuiDocument(), false );
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
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsAssociatedWithConstraints());
    rcCmdMgr.addCommand(new CmdSketcherSelectElementsWithDoFs());
    rcCmdMgr.addCommand(new CmdSketcherRestoreInternalAlignmentGeometry());
    rcCmdMgr.addCommand(new CmdSketcherSymmetry());
    rcCmdMgr.addCommand(new CmdSketcherCopy());
    rcCmdMgr.addCommand(new CmdSketcherClone());
    rcCmdMgr.addCommand(new CmdSketcherMove());
    rcCmdMgr.addCommand(new CmdSketcherCompCopy());
    rcCmdMgr.addCommand(new CmdSketcherRectangularArray());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllGeometry());
    rcCmdMgr.addCommand(new CmdSketcherDeleteAllConstraints());
}
