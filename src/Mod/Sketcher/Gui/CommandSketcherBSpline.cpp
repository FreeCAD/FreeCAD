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

#include "ViewProviderSketch.h"

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

extern bool isPolygonShown;

// Show/Hide BSpline polygon
DEF_STD_CMD_A(CmdSketcherBSplinePolygon);

CmdSketcherBSplinePolygon::CmdSketcherBSplinePolygon()
    :Command("Sketcher_BSplinePolygon")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Show/Hide B-Spline control polygon");
    sToolTipText    = QT_TR_NOOP("Switches between showing and hiding the control polygons for all B-Splines");
    sWhatsThis      = "Sketcher_BSplinePolygon";
    sStatusTip      = sToolTipText;
    
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    if(hGrp->GetBool("BSplinePolygonVisible", true))
        sPixmap         = "Sketcher_BSplinePolygon_on";
    else
        sPixmap         = "Sketcher_BSplinePolygon_off";
    
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherBSplinePolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    Gui::Document * doc= getActiveGuiDocument();

    SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    bool status = hGrp->GetBool("BSplineControlPolygonVisible", true);

    hGrp->SetBool("BSplineControlPolygonVisible", !status);

    vp->showRestoreInformationLayer();

    if(status) {
        if (getAction()) {
            getAction()->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplinePolygon_on"));
        }
    }
    else {
        if (getAction()) {
            getAction()->setIcon(Gui::BitmapFactory().pixmap("Sketcher_BSplinePolygon_off"));
        }
    }
}

bool CmdSketcherBSplinePolygon::isActive(void)
{
    return isSketcherBSplineActive( getActiveGuiDocument(), false );
}




void CreateSketcherCommandsBSpline(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherBSplinePolygon());

}
