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
# include <sstream>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "DrawGuiUtil.h"
#include "TaskGeomHatch.h"
#include "TaskHatch.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;

//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

//===========================================================================
// TechDraw_Hatch
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHatch)

CmdTechDrawHatch::CmdTechDrawHatch()
  : Command("TechDraw_Hatch")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Hatch a Face using Image File");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Hatch";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_Hatch";
}

void CmdTechDrawHatch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!_checkSelectionHatch(this)) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    auto partFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if (!partFeat) {
        return;
    }
    const std::vector<std::string> &subNames = selection[0].getSubNames();
    TechDraw::DrawPage* page = partFeat->findParentPage();
    std::string PageName = page->getNameInDocument();
    std::vector<int> selFaces;
    for (auto& s: subNames) {
        int f = TechDraw::DrawUtil::getIndexFromName(s);
        selFaces.push_back(f);
    }

    bool removeOld = false;
    std::vector<TechDraw::DrawHatch*> hatchObjs = partFeat->getHatches();
    for (auto& s: subNames) {                             //all the faces selected in DVP
        int face = TechDraw::DrawUtil::getIndexFromName(s);
        if (TechDraw::DrawHatch::faceIsHatched(face, hatchObjs)) {
            QMessageBox::StandardButton rc =
                    QMessageBox::question(Gui::getMainWindow(), QObject::tr("Replace Hatch?"),
                            QObject::tr("Some Faces in selection are already hatched.  Replace?"));
            if (rc == QMessageBox::StandardButton::NoButton) {
                return;
            }

            removeOld = true;
            break;
        }
    }

    if (removeOld) {
        openCommand(QT_TRANSLATE_NOOP("Command", "Remove old Hatch"));
        std::vector<std::pair< int, TechDraw::DrawHatch*> > toRemove;
        for (auto& h: hatchObjs) {             //all the hatch objects for selected DVP
            std::vector<std::string> hatchSubs = h->Source.getSubValues();
            for (auto& hs: hatchSubs) {        //all the Faces in this hatch object
                int hatchFace = TechDraw::DrawUtil::getIndexFromName(hs);
                std::vector<int>::iterator it = std::find(selFaces.begin(), selFaces.end(), hatchFace);
                if (it != selFaces.end()) {
                    std::pair< int, TechDraw::DrawHatch*> removeItem;
                    removeItem.first = hatchFace;
                    removeItem.second = h;
                    toRemove.push_back(removeItem);
                }
            }
        }
        for (auto& r: toRemove) {
            r.second->removeSub(r.first);
            if (r.second->empty()) {
                doCommand(Doc, "App.activeDocument().removeObject('%s')", r.second->getNameInDocument());
            }
        }
        commitCommand();
    }

    // dialog to fill in hatch values
    Gui::Control().showDialog(new TaskDlgHatch(partFeat, subNames));

    //Horrible hack to force Tree update  ??still required??
    //WF: yes. ViewProvider will not claim children without this!
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
    getDocument()->recompute();
}

bool CmdTechDrawHatch::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_GeometricHatch
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawGeometricHatch)

CmdTechDrawGeometricHatch::CmdTechDrawGeometricHatch()
  : Command("TechDraw_GeometricHatch")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Apply Geometric Hatch to Face");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_GeometricHatch";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_GeometricHatch";
}

void CmdTechDrawGeometricHatch::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    if (!_checkSelectionHatch(this)) {                 //same requirements as hatch - page, DrawViewXXX, face
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    auto objFeat( dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject()) );
    if (!objFeat) {
        return;
    }
    const std::vector<std::string> &subNames = selection[0].getSubNames();
    TechDraw::DrawPage* page = objFeat->findParentPage();
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("GeomHatch");

    // TODO: the structured label for GeomHatch (and Hatch) should be retired.
//    std::stringstream featLabel;
//    featLabel << FeatName << "FX" << TechDraw::DrawUtil::getIndexFromName(subNames.at(0));

    openCommand(QT_TRANSLATE_NOOP("Command", "Create GeomHatch"));
    doCommand(Doc, "App.activeDocument().addObject('TechDraw::DrawGeomHatch', '%s')", FeatName.c_str());
//    doCommand(Doc, "App.activeDocument().%s.Label = '%s'", FeatName.c_str(), featLabel.str().c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('DrawGeomHatch', 'GeomHatch', '%s')",
              FeatName.c_str(), FeatName.c_str());

    auto geomhatch( static_cast<TechDraw::DrawGeomHatch *>(getDocument()->getObject(FeatName.c_str())) );
    geomhatch->Source.setValue(objFeat, subNames);
    Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(getDocument())->getViewProvider(geomhatch);
    TechDrawGui::ViewProviderGeomHatch* hvp = dynamic_cast<TechDrawGui::ViewProviderGeomHatch*>(vp);
    if (!hvp) {
        return;
    }

    // dialog to fill in hatch values
    Gui::Control().showDialog(new TaskDlgGeomHatch(geomhatch, hvp, true));

    commitCommand();

    //Horrible hack to force Tree update  ??still required??
    double x = objFeat->X.getValue();
    objFeat->X.setValue(x);
    getDocument()->recompute();
}

bool CmdTechDrawGeometricHatch::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Image
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawImage)

CmdTechDrawImage::CmdTechDrawImage()
  : Command("TechDraw_Image")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Bitmap Image");
    sToolTipText  = QT_TR_NOOP("Insert Bitmap from a file into a page");
    sWhatsThis    = "TechDraw_Image";
    sStatusTip    = QT_TR_NOOP("Insert Bitmap from a file into a page");
    sPixmap       = "actions/TechDraw_Image";
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
    QString fileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QString::fromUtf8(QT_TR_NOOP("Select an Image File")),
        QString(),
        QString::fromUtf8(QT_TR_NOOP("Image files (*.jpg *.jpeg *.png *.bmp);;All files (*)")));
    if (fileName.isEmpty()) {
        return;
    }

    std::string FeatName = getUniqueObjectName("Image");
    fileName = Base::Tools::escapeEncodeFilename(fileName);
    openCommand(QT_TRANSLATE_NOOP("Command", "Create Image"));
    doCommand(Doc, "App.activeDocument().addObject('TechDraw::DrawViewImage', '%s')", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('DrawViewImage', 'Image', '%s')",
              FeatName.c_str(), FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.ImageFile = '%s'", FeatName.c_str(), fileName.toUtf8().constData());
    doCommand(Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawImage::isActive()
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_ToggleFrame
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToggleFrame)

CmdTechDrawToggleFrame::CmdTechDrawToggleFrame()
  : Command("TechDraw_ToggleFrame")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Turn View Frames On/Off");
    sToolTipText    = QT_TR_NOOP("Turn View Frames On/Off");
    sWhatsThis      = "TechDraw_Toggle";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_ToggleFrame";
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
    ViewProviderPage* vpp = dynamic_cast<ViewProviderPage*>(vp);

    if (!vpp) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No TechDraw Page"),
            QObject::tr("Need a TechDraw Page for this command"));
        return;
    }
    vpp->toggleFrameState();
}

bool CmdTechDrawToggleFrame::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}


void CreateTechDrawCommandsDecorate()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawHatch());
    rcCmdMgr.addCommand(new CmdTechDrawGeometricHatch());
    rcCmdMgr.addCommand(new CmdTechDrawImage());
    rcCmdMgr.addCommand(new CmdTechDrawToggleFrame());
//    rcCmdMgr.addCommand(new CmdTechDrawLeaderLine());
//    rcCmdMgr.addCommand(new CmdTechDrawRichTextAnnotation());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

bool _checkSelectionHatch(Gui::Command* cmd) {
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.empty()) {
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
        QObject::tr("No Faces to hatch in this selection"));
        return false;
    }
    std::string gType = TechDraw::DrawUtil::getGeomTypeFromName(SubNames.at(0));
    if (!(gType == "Face")) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
        QObject::tr("No Faces to hatch in this selection"));
        return false;
    }

    return true;
}
