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
#include <Mod/TechDraw/App/DrawCrosshatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/Gui/QGVPage.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;


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
    sMenuText       = QT_TR_NOOP("Hatch a Face using image file");
    sToolTipText    = QT_TR_NOOP("Hatch a Face using image file");
    sWhatsThis      = "TechDraw_NewHatch";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-hatch";
}

void CmdTechDrawNewHatch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!_checkSelectionHatch(this)) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( objFeat == nullptr ) {
        return;
    }
    const std::vector<std::string> &subNames = selection[0].getSubNames();
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("Hatch");
    std::stringstream featLabel;
    featLabel << FeatName << "F" << TechDraw::DrawUtil::getIndexFromName(subNames.at(0));

    openCommand("Create Hatch");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawHatch','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Label = '%s'",FeatName.c_str(),featLabel.str().c_str());

    auto hatch( static_cast<TechDraw::DrawHatch *>(getDocument()->getObject(FeatName.c_str())) );
    hatch->Source.setValue(objFeat, subNames);
    //should this be: doCommand(Doc,"App..Feat..Source = [(App...%s,%s),(App..%s,%s),...]",objs[0]->getNameInDocument(),subs[0],...);
    //seems very unwieldy

    commitCommand();

    //Horrible hack to force Tree update  ??still required??
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
    getDocument()->recompute();
}

bool CmdTechDrawNewHatch::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_NewCrosshatch
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawNewCrosshatch);

CmdTechDrawNewCrosshatch::CmdTechDrawNewCrosshatch()
  : Command("TechDraw_NewCrosshatch")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Apply geometric hatch to a Face");
    sToolTipText    = QT_TR_NOOP("Apply geometric hatch to a Face");
    sWhatsThis      = "TechDraw_NewCrosshatch";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-crosshatch";
}

void CmdTechDrawNewCrosshatch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!_checkSelectionHatch(this)) {                 //same requirements as hatch - page, DrawViewXXX, face
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if( objFeat == nullptr ) {
        return;
    }
    const std::vector<std::string> &subNames = selection[0].getSubNames();
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("Crosshatch");
    std::stringstream featLabel;
    featLabel << FeatName << "FX" << TechDraw::DrawUtil::getIndexFromName(subNames.at(0));

    openCommand("Create Crosshatch");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawCrosshatch','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Label = '%s'",FeatName.c_str(),featLabel.str().c_str());

    auto crosshatch( static_cast<TechDraw::DrawCrosshatch *>(getDocument()->getObject(FeatName.c_str())) );
    crosshatch->Source.setValue(objFeat, subNames);

    commitCommand();

    //Horrible hack to force Tree update  ??still required??
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
    getDocument()->recompute();
}

bool CmdTechDrawNewCrosshatch::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Image
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawImage);

CmdTechDrawImage::CmdTechDrawImage()
  : Command("TechDraw_Image")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert bitmap image");
    sToolTipText  = QT_TR_NOOP("Inserts a bitmap from a file in the active drawing");
    sWhatsThis    = "TechDraw_Image";
    sStatusTip    = QT_TR_NOOP("Inserts a bitmap from a file in the active drawing");
    sPixmap       = "actions/techdraw-image";
}

void CmdTechDrawImage::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    // Reading an image
    std::string defaultDir = App::Application::getResourceDir();
    QString qDir = QString::fromUtf8(defaultDir.data(),defaultDir.size());
    QString fileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
                                                   QString::fromUtf8(QT_TR_NOOP("Select an Image File")),
                                                   qDir,
                                                   QString::fromUtf8(QT_TR_NOOP("Image (*.png *.jpg *.jpeg)")));

    if (!fileName.isEmpty())
    {
        std::string FeatName = getUniqueObjectName("Image");
        openCommand("Create Image");
        doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewImage','%s')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.ImageFile = '%s'",FeatName.c_str(),fileName.toUtf8().constData());
        doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
        updateActive();
        commitCommand();
    }
}

bool CmdTechDrawImage::isActive(void)
{
    return DrawGuiUtil::needPage(this);
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
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
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
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void CreateTechDrawCommandsDecorate(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawNewHatch());
    rcCmdMgr.addCommand(new CmdTechDrawNewCrosshatch());
    rcCmdMgr.addCommand(new CmdTechDrawImage());
    rcCmdMgr.addCommand(new CmdTechDrawToggleFrame());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

bool _checkSelectionHatch(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.size() == 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("Select a Face first"));
        return false;
    }

    TechDraw::DrawViewPart * objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if(!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
                             QObject::tr("No TechDraw object in selection"));
        return false;
    }

    std::vector<App::DocumentObject*> pages = cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect selection"),
            QObject::tr("Create a page to insert."));
        return false;
    }

    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
        QObject::tr("Can't make a Hatched area from this selection"));
        return false;
    }
    std::string gType = TechDraw::DrawUtil::getGeomTypeFromName(SubNames.at(0));
    if (!(gType == "Face")) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
        QObject::tr("No Face in this selection"));
        return false;
    }

    return true;
}
