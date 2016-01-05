/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com	   *
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
#endif

# include <QMessageBox>

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

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isSketcherAcceleratorActive(Gui::Document *doc, bool actsOnSelection )
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            if (dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
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
            
            SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());
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
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

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
    
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    int GeoIdFirst=-1;
    int GeoIdLast=-1;
    
    // undo command open
    openCommand("add coincident constraint");
    // go through the selected subelements
    for (unsigned int i=0; i<(SubNames.size()-1); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge" &&
            SubNames[i+1].size() > 4 && SubNames[i+1].substr(0,4) == "Edge"	) {

            int GeoId1 = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;
            int GeoId2 = std::atoi(SubNames[i+1].substr(4,4000).c_str()) - 1;

            if(GeoIdFirst==-1)
              GeoIdFirst=GeoId1;

            GeoIdLast=GeoId2;

            const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
            if ((geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId()	) ||
                (geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())	) {
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

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
    
    if(autoRecompute)
        Gui::Command::updateActive();

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
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

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
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // undo command open
    openCommand("add coincident constraint");

    // go through the selected subelements
    for (unsigned int i=0; i<(SubNames.size()-1); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge" &&
            SubNames[i+1].size() > 4 && SubNames[i+1].substr(0,4) == "Edge"	) {

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
    
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
    
    if(autoRecompute)
        Gui::Command::updateActive();

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
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
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
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
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
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
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
        Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
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
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
    Sketcher::SketchObject* Obj= vp->getSketchObject();
        
    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    
    // get the needed lists and objects
    const std::vector< int > &solverredundant = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getSketchObject()->getLastRedundant();
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
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
    Sketcher::SketchObject* Obj= vp->getSketchObject();
        
    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;
    
    // get the needed lists and objects
    const std::vector< int > &solverconflicting = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getSketchObject()->getLastConflicting();
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
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    
    
    
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
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

DEF_STD_CMD_A(CmdSketcherRestoreInternalAlignmentGeometry);

CmdSketcherRestoreInternalAlignmentGeometry::CmdSketcherRestoreInternalAlignmentGeometry()
    :Command("Sketcher_RestoreInternalAlignmentGeometry")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/hide internal geometry");
    sToolTipText    = QT_TR_NOOP("Show all internal geometry / hide unused internal geometry");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Element_Ellipse_All";
    sAccel          = "CTRL+SHIFT+E";
    eType           = ForEdit;
}

void CmdSketcherRestoreInternalAlignmentGeometry::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

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
            if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId() || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                // First we search what has to be restored
                bool major=false;
                bool minor=false;
                bool focus1=false;
                bool focus2=false;
                
                int majorelementindex=-1;
                int minorelementindex=-1;
                int focus1elementindex=-1;
                int focus2elementindex=-1;
                
                const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
                
                for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                        it != vals.end(); ++it) {
                    if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
                    {
                        switch((*it)->AlignmentType){
                            case Sketcher::EllipseMajorDiameter:
                                major=true;
                                majorelementindex=(*it)->First;
                                break;
                            case Sketcher::EllipseMinorDiameter:
                                minor=true;
                                minorelementindex=(*it)->First;
                                break;
                            case Sketcher::EllipseFocus1: 
                                focus1=true;
                                focus1elementindex=(*it)->First;
                                break;
                            case Sketcher::EllipseFocus2: 
                                focus2=true;
                                focus2elementindex=(*it)->First;
                                break;
                            default:
                                break;
                        }
                    }
                }
                
                if(major && minor && focus1 && focus2)
                {
                    // Hide unused geometry here
                    int majorconstraints=0; // number of constraints associated to the geoid of the major axis
                    int minorconstraints=0;
                    int focus1constraints=0;
                    int focus2constraints=0;
                    
                    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                        it != vals.end(); ++it) {
                        
                        if((*it)->Second == majorelementindex || (*it)->First == majorelementindex || (*it)->Third == majorelementindex)
                            majorconstraints++;
                        else if((*it)->Second == minorelementindex || (*it)->First == minorelementindex || (*it)->Third == minorelementindex)
                            minorconstraints++;
                        else if((*it)->Second == focus1elementindex || (*it)->First == focus1elementindex || (*it)->Third == focus1elementindex)
                            focus1constraints++;
                        else if((*it)->Second == focus2elementindex || (*it)->First == focus2elementindex || (*it)->Third == focus2elementindex)
                            focus2constraints++;
                    }
                    // those with less than 2 constraints must be removed
                    if(majorconstraints>=2 && minorconstraints>=2 && focus1constraints>=2 && focus2constraints>=2)
                        return; // nothing to delete
                    
                    App::Document* doc = App::GetApplication().getActiveDocument();
                    
                    if (!doc) return;
                    
                    doc->openTransaction("Delete");
                    
                    if(majorconstraints<2) {
                        ss.str(std::string());
                        ss << "Edge" << majorelementindex + 1;
                        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());                   
                    }
                    
                    if(minorconstraints<2) {
                        ss.str(std::string());
                        ss << "Edge" << minorelementindex + 1;
                        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());                   
                    }
                    
                    if(focus1constraints<2) {
                        ss.str(std::string());
                        int vertex = Obj->getVertexIndexGeoPos(focus1elementindex,Sketcher::start);
                        if(vertex>-1){
                            ss << "Vertex" <<  vertex + 1;
                            Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());                  
                        }
                    }
                    
                    if(focus2constraints<2) {
                        ss.str(std::string());
                        int vertex = Obj->getVertexIndexGeoPos(focus2elementindex,Sketcher::start);
                        if(vertex>-1){
                            ss << "Vertex" <<  vertex + 1;
                            Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());                  
                        }
                    }
                    
                    SketcherGui::ViewProviderSketch* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());

                    if (vp) {
                        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
                        vp->onDelete(sel[0].getSubNames());
                    }
                    
                    
                    doc->commitTransaction();
                    return;
                }
                
                Gui::Command::openCommand("Expose ellipse internal geometry");
                
                int currentgeoid= Obj->getHighestCurveIndex();
                int incrgeo= 0;
                
                Base::Vector3d center;
                double majord;
                double minord;
                Base::Vector3d majdir;
                
                if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId()){
                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo);
                    
                    center=ellipse->getCenter();
                    majord=ellipse->getMajorRadius();
                    minord=ellipse->getMinorRadius();
                    majdir=ellipse->getMajorAxisDir();
                }
                else {
                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo);
                    
                    center=aoe->getCenter();
                    majord=aoe->getMajorRadius();
                    minord=aoe->getMinorRadius();
                    majdir=aoe->getMajorAxisDir();
                }

                Base::Vector3d mindir = Base::Vector3d(-majdir.y, majdir.x, 0.0);
                
                Base::Vector3d majorpositiveend = center + majord * majdir;
                Base::Vector3d majornegativeend = center - majord * majdir;
                Base::Vector3d minorpositiveend = center + minord * mindir;
                Base::Vector3d minornegativeend = center - minord * mindir;
                
                double df= sqrt(majord*majord-minord*minord);
                
                Base::Vector3d focus1P = center + df * majdir;
                Base::Vector3d focus2P = center - df * majdir;
                
                try{
                    if(!major)
                    {
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Line(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),True)",
                        Obj->getNameInDocument(),
                        majorpositiveend.x,majorpositiveend.y,majornegativeend.x,majornegativeend.y); // create line for major axis
                        
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMajorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),currentgeoid+incrgeo+1,GeoId); // constrain major axis
                        incrgeo++;
                    }
                    if(!minor)
                    {                       
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Line(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),True)",
                        Obj->getNameInDocument(),
                        minorpositiveend.x,minorpositiveend.y,minornegativeend.x,minornegativeend.y); // create line for minor axis
                        
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMinorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),currentgeoid+incrgeo+1,GeoId); // constrain minor axis
                        incrgeo++;
                    }
                    if(!focus1)
                    {
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                            Obj->getNameInDocument(),
                            focus1P.x,focus1P.y);
                        
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus1',%d,%d,%d)) ",
                        selection[0].getFeatName(),currentgeoid+incrgeo+1,Sketcher::start,GeoId); // constrain major axis
                        incrgeo++;
                    }
                    if(!focus2)
                    {
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                            Obj->getNameInDocument(),
                            focus2P.x,focus2P.y);
                        
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus2',%d,%d,%d)) ",
                        Obj->getNameInDocument(),currentgeoid+incrgeo+1,Sketcher::start,GeoId); // constrain major axis     
                    }
                    
                    Gui::Command::commitCommand();
                    
                    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
                    
                    if(autoRecompute)
                        Gui::Command::updateActive();
                
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                    Gui::Command::abortCommand();
                    
                    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
                    
                    if(autoRecompute)
                        Gui::Command::updateActive();
                }
    
            } // if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) 
            else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Currently internal geometry is only supported for ellipse and arc of ellipse. The last selected element must be an ellipse or an arc of ellipse."));
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Symmetry";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherSymmetry::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select elements from a single sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;

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
        LastGeoId = -1;
        LastPointPos = Sketcher::none;
        lastgeotype = line;
        lastvertexoraxis=true;
    }
    // check if last selected element is vertical axis
    else if(SubNames.rbegin()->size() == 6 && SubNames.rbegin()->substr(0,6) == "V_Axis"){
        LastGeoId = -2;
        LastPointPos = Sketcher::none;
        lastgeotype = line;
        lastvertexoraxis=true;
    }
    // check if last selected element is the root point
    else if(SubNames.rbegin()->size() == 9 && SubNames.rbegin()->substr(0,9) == "RootPoint"){
        LastGeoId = -1;
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

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

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

    if(autoRecompute)
        Gui::Command::updateActive();
    else
        Obj->solve();
}

bool CmdSketcherSymmetry::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
}

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
        DrawSketchHandlerCopy(string geoidlist, int origingeoid, Sketcher::PointPos originpos, int nelements, bool clone)
        : Mode(STATUS_SEEK_First)
        , geoIdList(geoidlist)
        , Origin()
        , OriginGeoId(origingeoid)
        , OriginPos(originpos)
        , nElements(nelements)
        , Clone(clone)
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
            EditCurve[0] = Base::Vector2D(Origin.x,Origin.y);
        }
        
        virtual void mouseMove(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_SEEK_First) {
                float length = (onSketchPos - EditCurve[0]).Length();
                float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2D(1.f,0.f));
                SbString text;
                text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
                setPositionText(onSketchPos, text);
                
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2D(0.0,0.0),AutoConstraint::VERTEX)) {
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }
 
            }
            applyCursor();
        }
        
        virtual bool pressButton(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_SEEK_First){
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);                
                Mode = STATUS_End;
            }

            return true;
        }
        
        virtual bool releaseButton(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_End){
                
                Base::Vector2D vector = EditCurve[1]-EditCurve[0];
                
                unsetCursor();
                resetPositionText();
                
                Gui::Command::openCommand("Create copy of geometry");
                
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
                
                try{
                    Gui::Command::doCommand(
                        Gui::Command::Doc, "App.ActiveDocument.%s.addCopy(%s,App.Vector(%f,%f,0),%s)",
                                            sketchgui->getObject()->getNameInDocument(), 
                                            geoIdList.c_str(), vector.fX, vector.fY,
                                            (Clone?"True":"False")); 
                    
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
                
                if(autoRecompute)
                    Gui::Command::updateActive();
                else
                    static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();
                
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
        std::vector<Base::Vector2D> EditCurve;
        std::vector<AutoConstraint> sugConstr1;
    };

class SketcherCopy : public Gui::Command {
public:    
    SketcherCopy(const char* name);
    void activate(bool clone);
};

SketcherCopy::SketcherCopy(const char* name): Command(name)
{}

void SketcherCopy::activate(bool clone)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
    
    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }
    
    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;
    
    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::none;
    const Part::Geometry *LastGeo = 0;
    
    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;
    
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if ( it->size() > 4 && it->substr(0,4) == "Edge" ) {           
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::none;
            
            LastGeo = Obj->getGeometry(LastGeoId);
            
            // lines to copy
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
                // points to copy
                if(LastGeoId>=0) {
                    geoids++;       
                    stream << LastGeoId << ",";
                }       
            }
        }
    }
    
    // check if last selected element is a Vertex, not being a GeomPoint
    if(SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex"){
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
    if(LastPointPos == Sketcher::none){
        if( LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId() ) {
            LastPointPos = Sketcher::mid;    
            }
            else {
                LastPointPos = Sketcher::start;    
            }  
    }
    
    // Ask the user if he wants to clone or to simple copy
    /*int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Dimensional/Geometric constraints"),
                                    QObject::tr("Do you want to clone the object, i.e. substitute dimensional constraints by geometric constraints?"),
                                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    // use an equality constraint
    if (ret == QMessageBox::Yes) {
        clone = true;
    }
    else if (ret == QMessageBox::Cancel) {
        // do nothing
        return;
    }*/
    
    ActivateAcceleratorHandler(getActiveGuiDocument(),new DrawSketchHandlerCopy(geoIdList, LastGeoId, LastPointPos, geoids, clone));    
}
        
class CmdSketcherCopy : public SketcherCopy
{
public:
    CmdSketcherCopy();
    virtual ~CmdSketcherCopy(){}
    virtual const char* className() const
    { return "CmdSketcherCopy"; }
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Copy";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherCopy::activated(int iMsg)
{
    activate(false);
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Clone";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherClone::activated(int iMsg)
{
    activate(true);
}

bool CmdSketcherClone::isActive(void)
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
    if (iMsg==0){
        CmdSketcherClone sc;
        sc.activate(true);
    }
    else if (iMsg==1) {
        CmdSketcherCopy sc;
        sc.activate(false);
    }
    else
        return;
    
    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();
    
    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
    pcAction->setShortcut(QString::fromLatin1("CTRL+C"));
}

Gui::Action * CmdSketcherCompCopy::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);
    
    QAction* clone = pcAction->addAction(QString());
    clone->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Clone"));
    QAction* copy = pcAction->addAction(QString());
    copy->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Copy"));
    
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
            EditCurve[0] = Base::Vector2D(Origin.x,Origin.y);
        }
        
        virtual void mouseMove(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_SEEK_First) {
                float length = (onSketchPos - EditCurve[0]).Length();
                float angle = (onSketchPos - EditCurve[0]).GetAngle(Base::Vector2D(1.f,0.f));
                SbString text;
                text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
                setPositionText(onSketchPos, text);
                
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);
                if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2D(0.0,0.0),AutoConstraint::VERTEX)) {
                    renderSuggestConstraintsCursor(sugConstr1);
                    return;
                }
                
            }
            applyCursor();
        }
        
        virtual bool pressButton(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_SEEK_First){
                EditCurve[1] = onSketchPos;
                sketchgui->drawEdit(EditCurve);                
                Mode = STATUS_End;
            }
            
            return true;
        }
        
        virtual bool releaseButton(Base::Vector2D onSketchPos)
        {
            if (Mode==STATUS_End){
                
                Base::Vector2D vector = EditCurve[1]-EditCurve[0];
                
                unsetCursor();
                resetPositionText();
                
                Gui::Command::openCommand("Create copy of geometry");
                
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
                
                try{
                    Gui::Command::doCommand(
                        Gui::Command::Doc, "App.ActiveDocument.%s.addRectangularArray(%s, App.Vector(%f,%f,0),%s,%d,%d,%s,%f)",
                                            sketchgui->getObject()->getNameInDocument(), 
                                            geoIdList.c_str(), vector.fX, vector.fY,
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
                
                if(autoRecompute)
                    Gui::Command::updateActive();
                else
                    static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();
                
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
        std::vector<Base::Vector2D> EditCurve;
        std::vector<AutoConstraint> sugConstr1;
    };
    

DEF_STD_CMD_A(CmdSketcherRectangularArray);

CmdSketcherRectangularArray::CmdSketcherRectangularArray()
:Command("Sketcher_RectangularArray")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Rectangular Array");
    sToolTipText    = QT_TR_NOOP("Creates an rectangular array pattern of the geometry taking as reference the last selected point");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_RectangularArray";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherRectangularArray::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
    
    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select elements from a single sketch."));
        return;
    }
    
    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    std::string doc_name = Obj->getDocument()->getName();
    std::string obj_name = Obj->getNameInDocument();
    std::stringstream ss;
    
    getSelection().clearSelection();

    int LastGeoId = 0;
    Sketcher::PointPos LastPointPos = Sketcher::none;
    const Part::Geometry *LastGeo = 0;
    
    // create python command with list of elements
    std::stringstream stream;
    int geoids = 0;
    
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle non-external edges
        if ( it->size() > 4 && it->substr(0,4) == "Edge" ) {           
            LastGeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            LastPointPos = Sketcher::none;
            
            LastGeo = Obj->getGeometry(LastGeoId);
            
            // lines to copy
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
                // points to copy
                if(LastGeoId>=0) {
                    geoids++;       
                    stream << LastGeoId << ",";
                }       
            }
        }
    }
    
    // check if last selected element is a Vertex, not being a GeomPoint
    if(SubNames.rbegin()->size() > 6 && SubNames.rbegin()->substr(0,6) == "Vertex"){
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
    if(LastPointPos == Sketcher::none){
        if( LastGeo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
            LastGeo->getTypeId() == Part::GeomEllipse::getClassTypeId() ) {
            LastPointPos = Sketcher::mid;    
            }
            else {
                LastPointPos = Sketcher::start;    
            }  
    }
    
    // Pop-up asking for values
    SketchRectangularArrayDialog * slad = new SketchRectangularArrayDialog();
        
    if( slad->exec() == QDialog::Accepted )
        ActivateAcceleratorHandler(getActiveGuiDocument(),
                                   new DrawSketchHandlerRectangularArray(geoIdList, LastGeoId, LastPointPos, geoids, slad->Clone,
                                                                    slad->Rows, slad->Cols, slad->ConstraintSeparation,
                                                                    slad->EqualVerticalHorizontalSpacing));
    
    delete slad;
    
}



bool CmdSketcherRectangularArray::isActive(void)
{
    return isSketcherAcceleratorActive( getActiveGuiDocument(), true );
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
    rcCmdMgr.addCommand(new CmdSketcherRestoreInternalAlignmentGeometry());
    rcCmdMgr.addCommand(new CmdSketcherSymmetry());
    rcCmdMgr.addCommand(new CmdSketcherCopy());
    rcCmdMgr.addCommand(new CmdSketcherClone());
    rcCmdMgr.addCommand(new CmdSketcherCompCopy());
    rcCmdMgr.addCommand(new CmdSketcherRectangularArray());
}
