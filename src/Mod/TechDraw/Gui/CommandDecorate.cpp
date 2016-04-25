/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
# include <boost/regex.hpp>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

# include <App/DocumentObject.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/Document.h>
# include <Gui/Selection.h>
# include <Gui/MainWindow.h>
# include <Gui/FileDialog.h>
# include <Gui/ViewProvider.h>

# include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/Gui/QGVPage.h>

# include "MDIViewPage.h"
# include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;

//===========================================================================
// utility routines
//===========================================================================

//TODO: code is duplicated in Command and CommandCreateDims
TechDraw::DrawPage* _findPageCD(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = 0;
    //check if a DrawPage is currently displayed
    Gui::MainWindow* w = Gui::getMainWindow();
    Gui::MDIView* mv = w->activeWindow();
    MDIViewPage* mvp = dynamic_cast<MDIViewPage*>(mv);
    if (mvp) {
        QGVPage* qp = mvp->getQGVPage();
        page = qp->getDrawPage();
    } else {
        //DrawPage not displayed, check Selection and/or Document for a DrawPage
        std::vector<App::DocumentObject*> selPages = cmd->getSelection().getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
        if (selPages.empty()) {                                            //no page in selection
            selPages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
            if (selPages.empty()) {                                        //no page in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No page found"),
                                     QObject::tr("Create a page first."));
                return page;
            } else if (selPages.size() > 1) {                              //multiple pages in document
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                     QObject::tr("Can not determine correct page."));
                return page;
            } else {                                                       //use only page in document
                page = dynamic_cast<TechDraw::DrawPage*>(selPages.front());
            }
        } else if (selPages.size() > 1) {                                  //multiple pages in selection
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Too many pages"),
                                 QObject::tr("Select exactly 1 page."));
            return page;
        } else {                                                           //use only page in selection
            page = dynamic_cast<TechDraw::DrawPage*>(selPages.front());
        }
    }
    return page;
}

//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

//===========================================================================
// TechDraw_NewHatch
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewHatch);

CmdTechDrawNewHatch::CmdTechDrawNewHatch()
  : Command("TechDraw_NewHatch")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert a hatched area into a view");
    sToolTipText    = QT_TR_NOOP("Insert a hatched area into a view");
    sWhatsThis      = "TechDraw_NewHatch";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-hatch";
}

void CmdTechDrawNewHatch::activated(int iMsg)
{
    if (!_checkSelectionHatch(this)) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawHatch *hatch = 0;
    std::string FeatName = getUniqueObjectName("Hatch");

    std::vector<App::DocumentObject *> objs;
    std::vector<std::string> subs;

    std::vector<std::string>::const_iterator itSub = SubNames.begin();
    for (; itSub != SubNames.end(); itSub++) {
        objs.push_back(objFeat);
        subs.push_back((*itSub));
    }

    openCommand("Create Hatch");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawHatch','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.PartView = App.activeDocument().%s",FeatName.c_str(),objFeat->getNameInDocument());

    hatch = dynamic_cast<TechDraw::DrawHatch *>(getDocument()->getObject(FeatName.c_str()));
    hatch->Edges.setValues(objs, subs);
    //should this be: doCommand(Doc,"App..Feat..Edges = [(App...%s,%s),(App..%s,%s),...]",objs[0]->getNameInDocument(),subs[0],...);
    //seems very unwieldy

    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    commitCommand();

    //Horrible hack to force Tree update  ??still required??
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

bool CmdTechDrawNewHatch::isActive(void)
{
    // TODO: Also ensure that there's a part selected?
    return hasActiveDocument();
}

//===========================================================================
// TechDraw_ToggleFrame
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToggleFrame);

CmdTechDrawToggleFrame::CmdTechDrawToggleFrame()
  : Command("TechDraw_ToggleFrame")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Turn View Frames on or off");
    sToolTipText    = QT_TR_NOOP("Turn View Frames on or off");
    sWhatsThis      = "TechDraw_ToggleFrame";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-toggleframe";
}

void CmdTechDrawToggleFrame::activated(int iMsg)
{
    TechDraw::DrawPage* page = _findPageCD(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(page->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(page);
    ViewProviderPage* dvp = dynamic_cast<ViewProviderPage*>(vp);

    if (dvp  && dvp->getMDIViewPage()) {
        dvp->getMDIViewPage()->setFrameState(!dvp->getMDIViewPage()->getFrameState());
    } else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No TechDraw Page"),
            QObject::tr("Need a TechDraw Page for this command"));
        return;
    }
}

bool CmdTechDrawToggleFrame::isActive(void)
{
    // TODO: Also ensure that there's a page displayed?
    return hasActiveDocument();
}

void CreateTechDrawCommandsDecorate(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewHatch());
    rcCmdMgr.addCommand(new CmdTechDrawToggleFrame());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

bool _checkSelectionHatch(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if(!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No Feature in selection"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page to insert."));
        return false;
    }

//        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
//                                                   QObject::tr("Can't make a Hatched area from this selection"));
//        return false;

    //TODO: if selection != set of closed edges, return false
    return true;
}
