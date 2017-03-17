/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "CommandConstraints.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isSketcherBSplineActive(Gui::Document *doc, bool actsOnSelection )
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

void ActivateBSplineHandler(Gui::Document *doc,DrawSketchHandler *handler)
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

void ShowRestoreInformationLayer(SketcherGui::ViewProviderSketch* vp, char * visibleelementname)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    
    bool status = hGrp->GetBool(visibleelementname, true);
    
    hGrp->SetBool(visibleelementname, !status);
    
    vp->showRestoreInformationLayer();
}

// Show/Hide BSpline degree
DEF_STD_CMD_A(CmdSketcherBSplineDegree);

CmdSketcherBSplineDegree::CmdSketcherBSplineDegree()
:Command("Sketcher_BSplineDegree")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/Hide B-spline degree");
    sToolTipText    = QT_TR_NOOP("Switches between showing and hiding the degree for all B-splines");
    sWhatsThis      = "Sketcher_BSplineDegree";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineDegree";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherBSplineDegree::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    ShowRestoreInformationLayer(vp, "BSplineDegreeVisible");

}

bool CmdSketcherBSplineDegree::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}

// Show/Hide BSpline polygon
DEF_STD_CMD_A(CmdSketcherBSplinePolygon);

CmdSketcherBSplinePolygon::CmdSketcherBSplinePolygon()
    :Command("Sketcher_BSplinePolygon")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/Hide B-spline control polygon");
    sToolTipText    = QT_TR_NOOP("Switches between showing and hiding the control polygons for all B-splines");
    sWhatsThis      = "Sketcher_BSplinePolygon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplinePolygon";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherBSplinePolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    ShowRestoreInformationLayer(vp, "BSplineControlPolygonVisible");

}

bool CmdSketcherBSplinePolygon::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}

// Show/Hide BSpline comb
DEF_STD_CMD_A(CmdSketcherBSplineComb);

CmdSketcherBSplineComb::CmdSketcherBSplineComb()
:Command("Sketcher_BSplineComb")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/Hide B-spline curvature comb");
    sToolTipText    = QT_TR_NOOP("Switches between showing and hiding the curvature comb for all B-splines");
    sWhatsThis      = "Sketcher_BSplineComb";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineComb";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherBSplineComb::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
    ShowRestoreInformationLayer(vp, "BSplineCombVisible");
    
}

bool CmdSketcherBSplineComb::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}

//
DEF_STD_CMD_A(CmdSketcherBSplineKnotMultiplicity);

CmdSketcherBSplineKnotMultiplicity::CmdSketcherBSplineKnotMultiplicity()
:Command("Sketcher_BSplineKnotMultiplicity")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/Hide B-spline knot multiplicity");
    sToolTipText    = QT_TR_NOOP("Switches between showing and hiding the knot multiplicity for all B-splines");
    sWhatsThis      = "Sketcher_BSplineKnotMultiplicity";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineKnotMultiplicity";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherBSplineKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::Document * doc= getActiveGuiDocument();
    
    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    
    ShowRestoreInformationLayer(vp, "BSplineKnotMultiplicityVisible");
    
}

bool CmdSketcherBSplineKnotMultiplicity::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}

// Composite drop down menu for show/hide geometry information layer
DEF_STD_CMD_ACLU(CmdSketcherCompBSplineShowHideGeometryInformation);

CmdSketcherCompBSplineShowHideGeometryInformation::CmdSketcherCompBSplineShowHideGeometryInformation()
: Command("Sketcher_CompBSplineShowHideGeometryInformation")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/hide B-spline information layer");
    sToolTipText    = QT_TR_NOOP("Show/hide B-spline information layer");
    sWhatsThis      = "Sketcher_CompBSplineShowHideGeometryInformation";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdSketcherCompBSplineShowHideGeometryInformation::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    Gui::Command * cmd;
    
    if (iMsg==0)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineDegree");
    else if (iMsg==1)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplinePolygon");
    else if (iMsg==2)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineComb");
    else if (iMsg==3)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineKnotMultiplicity");
    else
        return;
    
    cmd->invoke(0);
    
    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();
    
    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompBSplineShowHideGeometryInformation::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);
    
    QAction* c1 = pcAction->addAction(QString());
    c1->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplineDegree"));
    QAction* c2 = pcAction->addAction(QString());
    c2->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplinePolygon"));
    QAction* c3 = pcAction->addAction(QString());
    c3->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplineComb"));
    QAction* c4 = pcAction->addAction(QString());
    c4->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplineKnotMultiplicity"));

    _pcAction = pcAction;
    languageChange();
    
    pcAction->setIcon(c2->icon());
    int defaultId = 1;
    pcAction->setProperty("defaultAction", QVariant(defaultId));
    
    return pcAction;
}

void CmdSketcherCompBSplineShowHideGeometryInformation::languageChange()
{
    Command::languageChange();
    
    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();
    
    QAction* c1 = a[0];
    c1->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation","Show/Hide B-spline degree"));
    c1->setToolTip(QApplication::translate("Sketcher_BSplineDegree","Switches between showing and hiding the degree for all B-splines"));
    c1->setStatusTip(QApplication::translate("Sketcher_BSplineDegree","Switches between showing and hiding the degree for all B-splines"));
    QAction* c2 = a[1];
    c2->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation","Show/Hide B-spline control polygon"));
    c2->setToolTip(QApplication::translate("Sketcher_BSplinePolygon","Switches between showing and hiding the control polygons for all B-splines"));
    c2->setStatusTip(QApplication::translate("Sketcher_BSplinePolygon","Switches between showing and hiding the control polygons for all B-splines"));
    QAction* c3 = a[2];
    c3->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation","Show/Hide B-spline curvature comb"));
    c3->setToolTip(QApplication::translate("Sketcher_BSplineComb","Switches between showing and hiding the curvature comb for all B-splines"));
    c3->setStatusTip(QApplication::translate("Sketcher_BSplineComb","Switches between showing and hiding the curvature comb for all B-splines"));
    QAction* c4 = a[3];
    c4->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation","Show/Hide B-spline knot multiplicity"));
    c4->setToolTip(QApplication::translate("Sketcher_BSplineKnotMultiplicity","Switches between showing and hiding the knot multiplicity for all B-splines"));
    c4->setStatusTip(QApplication::translate("Sketcher_BSplineKnotMultiplicity","Switches between showing and hiding the knot multiplicity for all B-splines"));
}

void CmdSketcherCompBSplineShowHideGeometryInformation::updateAction(int /*mode*/)
{
}

bool CmdSketcherCompBSplineShowHideGeometryInformation::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}

// Convert to NURB
DEF_STD_CMD_A(CmdSketcherConvertToNURB);

CmdSketcherConvertToNURB::CmdSketcherConvertToNURB()
:Command("Sketcher_BSplineConvertToNURB")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Convert Geometry to B-spline");
    sToolTipText    = QT_TR_NOOP("Converts the given Geometry to a B-spline");
    sWhatsThis      = "Sketcher_ConvertToNURB";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineApproximate";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherConvertToNURB::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    bool nurbsized = false;
    
    openCommand("Convert to NURBS");

    for (unsigned int i=0; i<SubNames.size(); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge") {

            int GeoId = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.convertToNURBS(%d) ",
                                    selection[0].getFeatName(),GeoId);
            
            nurbsized = true;
        }
        else if (SubNames[i].size() > 12 && SubNames[i].substr(0,12) == "ExternalEdge") {
            
            int GeoId = - (std::atoi(SubNames[i].substr(12,4000).c_str()) + 2);
            
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.convertToNURBS(%d) ",
                                    selection[0].getFeatName(),GeoId);
            
            nurbsized = true;
        }
        
        
    }
    
    if(!nurbsized) {
        abortCommand();
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("None of the selected elements is an edge."));
    }
    else {
        commitCommand();
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

    if (autoRecompute)
        Gui::Command::updateActive();
    else
        Obj->solve();

}

bool CmdSketcherConvertToNURB::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), true );
}

// Convert to NURB
DEF_STD_CMD_A(CmdSketcherIncreaseDegree);

CmdSketcherIncreaseDegree::CmdSketcherIncreaseDegree()
:Command("Sketcher_BSplineIncreaseDegree")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Increase degree");
    sToolTipText    = QT_TR_NOOP("Increases the degree of the B-spline");
    sWhatsThis      = "Sketcher_BSplineIncreaseDegree";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineIncreaseDegree";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherIncreaseDegree::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand("Increase degree");

    for (unsigned int i=0; i<SubNames.size(); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge") {

            int GeoId = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.increaseBSplineDegree(%d) ",
                                    selection[0].getFeatName(),GeoId);
        }
    }

    commitCommand();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

    if (autoRecompute)
        Gui::Command::updateActive();
    else
        Obj->solve();

}

bool CmdSketcherIncreaseDegree::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), true );
}

DEF_STD_CMD_A(CmdSketcherIncreaseKnotMultiplicity);

CmdSketcherIncreaseKnotMultiplicity::CmdSketcherIncreaseKnotMultiplicity()
:Command("Sketcher_BSplineIncreaseKnotMultiplicity")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Increase degree");
    sToolTipText    = QT_TR_NOOP("Increases the multiplicity of the selected knot of a B-spline");
    sWhatsThis      = "Sketcher_BSplineIncreaseKnotMultiplicity";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineIncreaseKnotMultiplicity";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherIncreaseKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    
    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }
    
    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand("Increase knot multiplicity");
    
    bool applied = false;
    
    for (unsigned int i=0; i<SubNames.size(); i++ ) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(SubNames[i], Obj, GeoId, PosId);

        if(isSimpleVertex(Obj, GeoId, PosId)) {

            const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
                if((*it)->Type == Sketcher::InternalAlignment && (*it)->First == GeoId && (*it)->AlignmentType == Sketcher::BSplineKnotPoint)
                {
                    try {
                        Gui::Command::doCommand(
                            Doc,"App.ActiveDocument.%s.modifyBSplineKnotMultiplicity(%d,%d,%d) ",
                            selection[0].getFeatName(),(*it)->Second, (*it)->InternalAlignmentIndex + 1, 1);

                        applied = true;
                        
                        // Warning: GeoId list might have changed as the consequence of deleting pole circles and
                        // particularly bspline GeoID might have changed.
                    }
                    catch (const Base::Exception& e) {
                        Base::Console().Error("%s\n", e.what());
                    }
                    
                    break; // we applied to a knot, because the constraints have changed in the meanwhile, this loop is now invalid, so we exit.
                   
                }
            }

        }
    }
    
    if(!applied) {
        abortCommand();
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                         QObject::tr("None of the selected elements is a knot of a bspline or the knot has already reached the maximum multiplicity."));
    }
    else {
        commitCommand();
    }
    
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
    
    if (autoRecompute)
        Gui::Command::updateActive();
    else
        Obj->solve();
    
}

bool CmdSketcherIncreaseKnotMultiplicity::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), true );
}

void CreateSketcherCommandsBSpline(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherBSplineDegree());
    rcCmdMgr.addCommand(new CmdSketcherBSplinePolygon());
    rcCmdMgr.addCommand(new CmdSketcherBSplineComb());
    rcCmdMgr.addCommand(new CmdSketcherBSplineKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherCompBSplineShowHideGeometryInformation());
    rcCmdMgr.addCommand(new CmdSketcherConvertToNURB());
    rcCmdMgr.addCommand(new CmdSketcherIncreaseDegree());
    rcCmdMgr.addCommand(new CmdSketcherIncreaseKnotMultiplicity());
}
