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

# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/DrawHatch.h>
# include <Mod/TechDraw/App/DrawPage.h>

# include "MDIViewPage.h"
# include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;

//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

//===========================================================================
// TechDraw_NewHatch
//===========================================================================

DEF_STD_CMD(CmdTechDrawNewHatch);

CmdTechDrawNewHatch::CmdTechDrawNewHatch()
  : Command("TechDraw_NewHatch")
{
    sAppModule      = "Drawing";
    sGroup          = QT_TR_NOOP("Drawing");
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
    commitCommand();

    hatch = dynamic_cast<TechDraw::DrawHatch *>(getDocument()->getObject(FeatName.c_str()));
    hatch->Edges.setValues(objs, subs);

    hatch->execute();

    std::vector<App::DocumentObject*> pages = getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    TechDraw::DrawPage *page = dynamic_cast<TechDraw::DrawPage *>(pages.front());
    page->addView(page->getDocument()->getObject(FeatName.c_str()));

    //Horrible hack to force Tree update  ??still required??
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
}

void CreateTechDrawCommandsDecorate(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewHatch());
    //rcCmdMgr.addCommand(new CmdTechDrawHideLabels());
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
