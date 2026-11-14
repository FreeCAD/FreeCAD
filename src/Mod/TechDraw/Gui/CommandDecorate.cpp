// SPDX-License-Identifier: LGPL-2.1-or-later

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

# include <QMessageBox>
#include <algorithm>


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
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DrawGuiUtil.h"
#include "TaskHatchFace.h"
#include "ViewProviderPage.h"
#include "MDIViewPage.h"
#include "CommandHelpers.h"
#include "PreferencesGui.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;

//===========================================================================
// TechDraw_ToggleFrame
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToggleFrame)

CmdTechDrawToggleFrame::CmdTechDrawToggleFrame()
  : Command("TechDraw_ToggleFrame")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Toggle View Frames");
    sToolTipText    = QT_TR_NOOP("Toggles visibility of view frames and vertices");
    sWhatsThis      = "TechDraw_Toggle";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_ToggleFrame";
}

// This is a toggle.  Each press flips the fame state.
// Gui::Action *CmdTechDrawToggleFrame::createAction()
// {
//     Gui::Action *action = Gui::Command::createAction();
//     action->setCheckable(true);
//     action->setChecked(false);

//     return action;
// }

void CmdTechDrawToggleFrame::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    if (PreferencesGui::getViewFrameMode() != ViewFrameMode::Manual) {
        return;
    }

    auto mvp = dynamic_cast<MDIViewPage *>(Gui::getMainWindow()->activeWindow());
    if (!mvp) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No TechDraw Page"),
            QObject::tr("Need a TechDraw Page for this command"));
        return;
    }

    ViewProviderPage* vpp = mvp->getViewProviderPage();
    if (!vpp) {
        return;
    }

    vpp->toggleFrameState();

    // Gui::Action *action = this->getAction();
    // if (action) {
    //     action->setChecked(vpp->getFrameState());
    // }
}

bool CmdTechDrawToggleFrame::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView && PreferencesGui::getViewFrameMode() == ViewFrameMode::Manual);
}

//===========================================================================
// TechDraw_ToggleGrid
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawToggleGrid)

CmdTechDrawToggleGrid::CmdTechDrawToggleGrid()
  : Command("TechDraw_ToggleGrid")
{
    sAppModule   = "TechDraw";
    sGroup       = QT_TR_NOOP("TechDraw");
    sMenuText    = QT_TR_NOOP("Toggle Grid");
    sToolTipText = QT_TR_NOOP("Toggles the grid on the active page");
    sWhatsThis   = "TechDraw_ToggleGrid";
    sStatusTip   = sToolTipText;
}

void CmdTechDrawToggleGrid::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto mvp = dynamic_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (!mvp) {
        return;
    }
    ViewProviderPage* vpp = mvp->getViewProviderPage();
    if (!vpp) {
        return;
    }
    vpp->ShowGrid.setValue(!vpp->ShowGrid.getValue());
}

bool CmdTechDrawToggleGrid::isActive()
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_HatchFace
//===========================================================================
class CmdTechDrawHatchFace: public Gui::Command
{
public:
    explicit CmdTechDrawHatchFace(const char* name = "TechDraw_HatchFace");
    const char* className() const override
    {
        return "CmdTechDrawHatchFace";
    }

protected:
    void activated(int) override;
    bool isActive() override;
};

namespace
{
bool isHatch(const App::DocumentObject* object)
{
    return object
        && (object->isDerivedFrom<TechDraw::DrawHatch>()
            || object->isDerivedFrom<TechDraw::DrawGeomHatch>());
}

TechDraw::DrawViewPart* selectedHatchView(std::vector<Gui::SelectionObject>& selection)
{
    if (selection.size() != 1) {
        return nullptr;
    }
    auto view = dynamic_cast<TechDraw::DrawViewPart*>(selection.front().getObject());
    const auto& faces = selection.front().getSubNames();
    if (!view || !view->findParentPage() || faces.empty()) {
        return nullptr;
    }
    const bool onlyFaces = std::ranges::all_of(faces, [](const auto& face) {
        return TechDraw::DrawUtil::getGeomTypeFromName(face) == "Face"
            && TechDraw::DrawUtil::getIndexFromName(face) >= 0;
    });
    return onlyFaces ? view : nullptr;
}

bool selectedHatch(const std::vector<Gui::SelectionObject>& selection)
{
    return selection.size() == 1 && selection.front().getSubNames().empty()
        && isHatch(selection.front().getObject());
}

bool hasHatchedFaces(TechDraw::DrawViewPart* view, const std::vector<std::string>& faces)
{
    const auto overlaps = [&faces](const auto* hatch) {
        return std::ranges::any_of(hatch->Source.getSubValues(), [&faces](const auto& face) {
            return std::ranges::find(faces, face) != faces.end();
        });
    };
    return std::ranges::any_of(view->getHatches(), overlaps)
        || std::ranges::any_of(view->getGeomHatches(), overlaps);
}
}  // namespace

CmdTechDrawHatchFace::CmdTechDrawHatchFace(const char* name)
    : Gui::Command(name)
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Hatch Face");
    sToolTipText = QT_TR_NOOP("Applies an SVG or PAT hatch pattern to the selected faces");
    sWhatsThis = "TechDraw_HatchFace";
    sStatusTip = sToolTipText;
    sPixmap = "actions/TechDraw_Hatch";
}

void CmdTechDrawHatchFace::activated(int)
{
    if (Gui::Control().activeDialog()) {
        return;
    }
    auto selection = getSelection().getSelectionEx();
    if (selectedHatch(selection)) {
        Gui::Control().showDialog(new TaskDlgHatchFace(selection.front().getObject()));
        return;
    }
    auto view = selectedHatchView(selection);
    if (!view) {
        QMessageBox::information(
            Gui::getMainWindow(),
            QObject::tr("Incorrect Selection"),
            QObject::tr("The selection must contain only faces from one TechDraw view.")
        );
        return;
    }
    const auto& faces = selection.front().getSubNames();
    if (hasHatchedFaces(view, faces)
        && QMessageBox::question(
               Gui::getMainWindow(),
               QObject::tr("Replace Hatch?"),
               QObject::tr("Some faces in the selection are already hatched. Replace existing hatches?"),
               QMessageBox::Yes | QMessageBox::No
           ) != QMessageBox::Yes) {
        return;
    }
    Gui::Control().showDialog(new TaskDlgHatchFace(view, faces));
}

bool CmdTechDrawHatchFace::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }
    auto selection = getSelection().getSelectionEx();
    return selectedHatch(selection) || selectedHatchView(selection);
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
    sMenuText     = QT_TR_NOOP("Bitmap Image");
    sToolTipText  = QT_TR_NOOP("Inserts a bitmap from a file into the current page");
    sWhatsThis    = "TechDraw_Image";
    sStatusTip    = QT_TR_NOOP("Insert bitmap from a file into a page");
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
    const Gui::FileDialog::FilterList filterList {
        {QObject::tr("Image files"), {"*.jpg", "*.jpeg", "*.png", "*.bmp"}},
        Gui::FileDialog::Filter::AllFiles(),
    };
    QString fileName = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(),
        QObject::tr("Select an image file"),
        Preferences::defaultSymbolDir(),
        filterList);
    if (fileName.isEmpty()) {
        return;
    }

    std::string FeatName = getUniqueObjectName("Image");
    auto filespec = DU::cleanFilespecBackslash(
        Base::Tools::escapeEncodeFilename(fileName.toStdString()));

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Image"));
    doCommand(Doc, "App.activeDocument().addObject('TechDraw::DrawViewImage', '%s')", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('DrawViewImage', 'Image', '%s')",
              FeatName.c_str(), FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.ImageFile = '%s'", FeatName.c_str(), filespec.c_str());

    auto baseView = CommandHelpers::firstViewInSelection(this);
    if (baseView) {
        auto baseName = baseView->getNameInDocument();
        doCommand(Doc, "App.activeDocument().%s.Owner = App.activeDocument().%s",
                  FeatName.c_str(), baseName);
    }

    doCommand(Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawImage::isActive()
{
    return DrawGuiUtil::needPage(this);
}

void CreateTechDrawCommandsDecorate()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawHatchFace());
    // Keep saved shortcuts and macros working through the unified implementation.
    rcCmdMgr.addCommand(new CmdTechDrawHatchFace("TechDraw_Hatch"));
    rcCmdMgr.addCommand(new CmdTechDrawHatchFace("TechDraw_GeometricHatch"));
    rcCmdMgr.addCommand(new CmdTechDrawImage());
    rcCmdMgr.addCommand(new CmdTechDrawToggleFrame());
    rcCmdMgr.addCommand(new CmdTechDrawToggleGrid());

//    rcCmdMgr.addCommand(new CmdTechDrawLeaderLine());
//    rcCmdMgr.addCommand(new CmdTechDrawRichTextAnnotation());
}
