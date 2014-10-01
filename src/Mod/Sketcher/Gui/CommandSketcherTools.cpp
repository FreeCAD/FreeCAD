/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com	     *
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
#endif

#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"

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

    updateActive();

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
    updateActive();

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
    std::stringstream ss;
    
    getSelection().clearSelection();
    
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            
            // push all the constraints
            int i=1;
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it,++i) {
                if ( (*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId){
                  ss.str(std::string());
                  ss << "Constraint" << i;
                  Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
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
    std::stringstream ss;
    
    // get the needed lists and objects
    const std::vector< int > &solverredundant = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getRedundant();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
       
    getSelection().clearSelection();
    
    // push the constraints
    int i=1;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverredundant.begin();itc != solverredundant.end(); ++itc) {
            if ( (*itc) == i){
                ss.str(std::string());
                ss << "Constraint" << i;
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
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
    const std::vector< int > &solverconflicting = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getConflicting();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
    
    getSelection().clearSelection();
    
    // push the constraints
    int i=1;
    for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();it != vals.end(); ++it,++i) {
        for(std::vector< int >::const_iterator itc= solverconflicting.begin();itc != solverconflicting.end(); ++itc) {
            if ( (*itc) == i){
                ss.str(std::string());
                ss << "Constraint" << i;
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
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
            int ConstrId = std::atoi(it->substr(10,4000).c_str()) - 1;
            
            if(ConstrId<vals.size()){
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
}
