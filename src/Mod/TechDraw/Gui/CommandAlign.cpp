/***************************************************************************
 *   Copyright (c) 2024 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
#include <QMessageBox>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Action.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>

#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIView.h"
#include "DrawGuiUtil.h"
#include "ViewProviderViewPart.h"


const auto& getSelection = Gui::Command::getSelection;  // alias
using namespace TechDrawGui;
using namespace TechDraw;

namespace TechDrawGui {
class QGIEdge;
class QGIVertex;
}

namespace {
void incorrectSelection()
{
    QMessageBox::warning(
        Gui::getMainWindow(),
        QObject::tr("Incorrect Selection"),
        QObject::tr("You must select 2 vertices or 1 edge\n")
    );
}

void CmdTechDrawAlignByRotation(const Base::Vector2d& direction)
{
    std::vector<TechDraw::DrawViewPart*> dvps = getSelection().getObjectsOfType<TechDraw::DrawViewPart>();
    if (dvps.size() != 1) {
        incorrectSelection();
        return;
    }
    TechDraw::DrawViewPart* dvp = dvps[0];

    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(dvp->getDocument());
    if (!guiDoc) {
        return;
    }

    Gui::ViewProvider* gvp = guiDoc->getViewProvider(dvp);
    auto vpdvp = static_cast<TechDrawGui::ViewProviderViewPart*>(gvp);
    if (!vpdvp) {
        return;
    }
    QGIView* view = vpdvp->getQView();

    std::vector<std::string> subNames = getSelection().getSelectionEx()[0].getSubNames();
    if(!DrawUtil::isGeomTypeConsistent(subNames)) {
        incorrectSelection();
        return;
    }

    std::vector<int> subIndexes = DrawUtil::getIndexFromName(subNames);
    std::string subType = DrawUtil::getGeomTypeFromName(subNames.at(0));
    if (subType == "Vertex") {
        std::vector<QGIVertex*> vertexes = view->getObjects<QGIVertex*>(subIndexes);
        if (vertexes.size() == 2) {
            QGIVertex* v1 = vertexes.front();
            QGIVertex* v2 = vertexes.back();
            DrawGuiUtil::rotateToAlign(v1, v2, direction);
            dvp->recomputeFeature();
            return;
        }
    }
    else if (subType == "Edge") {
        std::vector<QGIEdge*> edges = view->getObjects<QGIEdge*>(subIndexes);
        if (edges.size() == 1) {
            DrawGuiUtil::rotateToAlign(edges.at(0), direction);
            dvp->recomputeFeature();
            return;
        }
    }

    incorrectSelection();
}
} // anonymous namespace


//===========================================================================
// TechDraw_AlignVertexesVertically
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAlignVertexesVertically)

CmdTechDrawAlignVertexesVertically::CmdTechDrawAlignVertexesVertically()
  : Command("TechDraw_AlignVertexesVertically")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Vertices/Edge Vertically");
    sToolTipText    = QT_TR_NOOP("Aligns the selected vertices or edges vertically to the view rotation");
    sWhatsThis      = "TechDraw_AlignGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawAlignVertexesVertically::activated(int iMsg)
{
    Q_UNUSED(iMsg)

    Base::Vector2d Vertical(0.0, 1.0);
    CmdTechDrawAlignByRotation(Vertical);
}

bool CmdTechDrawAlignVertexesVertically::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}


//===========================================================================
// TechDraw_AlignVertexesHorizontally
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAlignVertexesHorizontally)

CmdTechDrawAlignVertexesHorizontally::CmdTechDrawAlignVertexesHorizontally()
  : Command("TechDraw_AlignVertexesHorizontally")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align Vertices/Edge Horizontally");
    sToolTipText    = QT_TR_NOOP("Aligns the selected vertices or edges horizontally to the view rotation");
    sWhatsThis      = "TechDraw_AlignGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawAlignVertexesHorizontally::activated(int iMsg)
{
    Q_UNUSED(iMsg)

    Base::Vector2d Horizontal(1.0, 0.0);
    CmdTechDrawAlignByRotation(Horizontal);
}

bool CmdTechDrawAlignVertexesHorizontally::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}


void CreateTechDrawCommandsAlign(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawAlignVertexesVertically());
    rcCmdMgr.addCommand(new CmdTechDrawAlignVertexesHorizontally());
}
