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
# include <Standard_Version.hxx>
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

// Show/Hide B-spline degree
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

// Show/Hide B-spline polygon
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

// Show/Hide B-spline comb
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
    c1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineDegree"));
    QAction* c2 = pcAction->addAction(QString());
    c2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplinePolygon"));
    QAction* c3 = pcAction->addAction(QString());
    c3->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineComb"));
    QAction* c4 = pcAction->addAction(QString());
    c4->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineKnotMultiplicity"));

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
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

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

    tryAutoRecomputeIfNotSolve(Obj);
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
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand("Increase degree");
    
    bool ignored=false;

    for (unsigned int i=0; i<SubNames.size(); i++ ) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0,4) == "Edge") {

            int GeoId = std::atoi(SubNames[i].substr(4,4000).c_str()) - 1;

            const Part::Geometry * geo = Obj->getGeometry(GeoId);

            if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {

                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.increaseBSplineDegree(%d) ",
                                        selection[0].getFeatName(),GeoId);

                // add new control points
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.exposeInternalGeometry(%d)",
                                        selection[0].getFeatName(),
                                        GeoId);
            }
            else {
                ignored=true;
            }
        }
    }

    if(ignored) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("At least one of the selected objects was not a B-Spline and was ignored."));
    }

    commitCommand();

    tryAutoRecomputeIfNotSolve(Obj);

    getSelection().clearSelection();
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
    sMenuText       = QT_TR_NOOP("Increase knot multiplicity");
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
    
    #if OCC_VERSION_HEX < 0x060900
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong OCE/OCC version"),
                         QObject::tr("This version of OCE/OCC does not support knot operation. You need 6.9.0 or higher"));
    return;
    #endif
    
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }
    
    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    
    if(SubNames.size()>1) {
        // Check that only one object is selected, as we need only one object to get the new GeoId after multiplicity change
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("The selection comprises more than one item. Please select just one knot."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand("Increase knot multiplicity");

    bool applied = false;
    bool notaknot = true;
    
    boost::uuids::uuid bsplinetag;

    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[0], Obj, GeoId, PosId);

    if(isSimpleVertex(Obj, GeoId, PosId)) {

        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->First == GeoId && (*it)->AlignmentType == Sketcher::BSplineKnotPoint)
            {
                bsplinetag = Obj->getGeometry((*it)->Second)->getTag();
                
                notaknot = false;
                
                try {
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.modifyBSplineKnotMultiplicity(%d,%d,%d) ",
                        selection[0].getFeatName(),(*it)->Second, (*it)->InternalAlignmentIndex + 1, 1);

                    applied = true;

                    // Warning: GeoId list might have changed as the consequence of deleting pole circles and
                    // particularly B-spline GeoID might have changed.

                }
                catch (const Base::CADKernelError& e) {
                    e.ReportException();
                    if(e.getTranslatable()) {
                        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("CAD Kernel Error"),
                                            QObject::tr(e.getMessage().c_str()));
                    }
                    getSelection().clearSelection();
                }
                catch (const Base::Exception& e) {
                    e.ReportException();
                    if(e.getTranslatable()) {
                        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Input Error"),
                                            QObject::tr(e.getMessage().c_str()));
                    }

                    getSelection().clearSelection();
                }

                break; // we have already found our knot.

            }

        }

    }
    
    if(notaknot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("None of the selected elements is a knot of a B-spline"));
    }
    
    if(applied)
    {
        // find new geoid for B-spline as GeoId might have changed
        const std::vector< Part::Geometry * > &gvals = Obj->getInternalGeometry();
        
        int ngeoid = 0;
        bool ngfound = false;
        
        for (std::vector<Part::Geometry *>::const_iterator geo = gvals.begin(); geo != gvals.end(); geo++, ngeoid++) {
            if ((*geo) && (*geo)->getTag() == bsplinetag) {
                ngfound = true;
                break;
            }
        }
        
        
        if(ngfound) {
            try {
                // add internalalignment for new pole
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.exposeInternalGeometry(%d)",
                                        selection[0].getFeatName(),
                                        ngeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                getSelection().clearSelection();
            }
            
        }
    }
    
    if(!applied) {
        abortCommand();
    }
    else {
        commitCommand();
    }

    tryAutoRecomputeIfNotSolve(Obj);

    getSelection().clearSelection();
    
}

bool CmdSketcherIncreaseKnotMultiplicity::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), true );
}

DEF_STD_CMD_A(CmdSketcherDecreaseKnotMultiplicity);

CmdSketcherDecreaseKnotMultiplicity::CmdSketcherDecreaseKnotMultiplicity()
:Command("Sketcher_BSplineDecreaseKnotMultiplicity")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Decrease multiplicity");
    sToolTipText    = QT_TR_NOOP("Decreases the multiplicity of the selected knot of a B-spline");
    sWhatsThis      = "Sketcher_BSplineDecreaseKnotMultiplicity";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_BSplineDecreaseKnotMultiplicity";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherDecreaseKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    #if OCC_VERSION_HEX < 0x060900
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong OCE/OCC version"),
                         QObject::tr("This version of OCE/OCC does not support knot operation. You need 6.9.0 or higher"));
    return;
    #endif
    
    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }
    
    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    
    if(SubNames.size()>1) {
        // Check that only one object is selected, as we need only one object to get the new GeoId after multiplicity change
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("The selection comprises more than one item. Please select just one knot."));
        return;
    }
    
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    
    openCommand("Decrease knot multiplicity");
    
    bool applied = false;
    bool notaknot = true;
    
    boost::uuids::uuid bsplinetag;
    
    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[0], Obj, GeoId, PosId);
    
    if(isSimpleVertex(Obj, GeoId, PosId)) {
        
        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
        
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->First == GeoId && (*it)->AlignmentType == Sketcher::BSplineKnotPoint)
            {
                bsplinetag = Obj->getGeometry((*it)->Second)->getTag();
                
                notaknot = false;
                
                try {
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.modifyBSplineKnotMultiplicity(%d,%d,%d) ",
                                            selection[0].getFeatName(),(*it)->Second, (*it)->InternalAlignmentIndex + 1, -1);
                    
                    applied = true;
                    
                    // Warning: GeoId list might have changed as the consequence of deleting pole circles and
                    // particularly B-spline GeoID might have changed.
                    
                }
                catch (const Base::Exception& e) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Error"),
                                         QObject::tr(getStrippedPythonExceptionString(e).c_str()));
                    getSelection().clearSelection();
                }
                
                break; // we have already found our knot.
                
            }
        }
        
    }
    
    if(notaknot){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("None of the selected elements is a knot of a B-spline"));
    }    
    
    if(applied)
    {
        // find new geoid for B-spline as GeoId might have changed
        const std::vector< Part::Geometry * > &gvals = Obj->getInternalGeometry();
        
        int ngeoid = 0;
        bool ngfound = false;
        
        for (std::vector<Part::Geometry *>::const_iterator geo = gvals.begin(); geo != gvals.end(); geo++, ngeoid++) {
            if ((*geo) && (*geo)->getTag() == bsplinetag) {
                ngfound = true;
                break;
            }
        }
            

        if(ngfound) {
            try {
                // add internalalignment for new pole
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.exposeInternalGeometry(%d)",
                                        selection[0].getFeatName(),
                                        ngeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                getSelection().clearSelection();
            }

        }
    }
    
    if(!applied) {
        abortCommand();
    }
    else {
        commitCommand();
    }

    tryAutoRecomputeIfNotSolve(Obj);

    getSelection().clearSelection();
}

bool CmdSketcherDecreaseKnotMultiplicity::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), true );
}


// Composite drop down for knot increase/decrease
DEF_STD_CMD_ACLU(CmdSketcherCompModifyKnotMultiplicity);

CmdSketcherCompModifyKnotMultiplicity::CmdSketcherCompModifyKnotMultiplicity()
: Command("Sketcher_CompModifyKnotMultiplicity")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Modify knot multiplicity");
    sToolTipText    = QT_TR_NOOP("Modifies the multiplicity of the selected knot of a B-spline");    
    sWhatsThis      = "Sketcher_CompModifyKnotMultiplicity";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdSketcherCompModifyKnotMultiplicity::activated(int iMsg)
{
    
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    
    Gui::Command * cmd;
    
    if (iMsg==0)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineIncreaseKnotMultiplicity");
    else if (iMsg==1)
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineDecreaseKnotMultiplicity");
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

Gui::Action * CmdSketcherCompModifyKnotMultiplicity::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* c1 = pcAction->addAction(QString());
    c1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineIncreaseKnotMultiplicity"));
    QAction* c2 = pcAction->addAction(QString());
    c2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineDecreaseKnotMultiplicity"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(c1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompModifyKnotMultiplicity::languageChange()
{
    Command::languageChange();
    
    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();
    
    QAction* c1 = a[0];
    c1->setText(QApplication::translate("CmdSketcherCompModifyKnotMultiplicity","Increase knot multiplicity"));
    c1->setToolTip(QApplication::translate("Sketcher_BSplineIncreaseKnotMultiplicity","Increases the multiplicity of the selected knot of a B-spline"));
    c1->setStatusTip(QApplication::translate("Sketcher_BSplineIncreaseKnotMultiplicity","Increases the multiplicity of the selected knot of a B-spline"));
    QAction* c2 = a[1];
    c2->setText(QApplication::translate("CmdSketcherCompModifyKnotMultiplicity","Decrease knot multiplicity"));
    c2->setToolTip(QApplication::translate("Sketcher_BSplineDecreaseKnotMultiplicity","Decreases the multiplicity of the selected knot of a B-spline"));
    c2->setStatusTip(QApplication::translate("Sketcher_BSplineDecreaseKnotMultiplicity","Decreases the multiplicity of the selected knot of a B-spline"));

}

void CmdSketcherCompModifyKnotMultiplicity::updateAction(int /*mode*/)
{
}

bool CmdSketcherCompModifyKnotMultiplicity::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
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
    rcCmdMgr.addCommand(new CmdSketcherDecreaseKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherCompModifyKnotMultiplicity());
}
