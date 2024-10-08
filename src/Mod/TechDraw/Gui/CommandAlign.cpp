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

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include <Gui/Action.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIView.h"
#include "DrawGuiUtil.h"
#include "ViewProviderViewPart.h"


using namespace TechDrawGui;
using namespace TechDraw;

//===========================================================================
// TechDraw_AlignVertexesVertically
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAlignVertexesVertically)

CmdTechDrawAlignVertexesVertically::CmdTechDrawAlignVertexesVertically()
  : Command("TechDraw_AlignVertexesVertically")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Align vertexes vertically by view rotation");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_AlignGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawAlignVertexesVertically::activated(int iMsg)
{
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(
        getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId())[0]
    );

    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(dvp->getDocument());
    if (!guiDoc) {
        return;
    }

    Gui::ViewProvider* gvp = guiDoc->getViewProvider(dvp);
    auto vpdvp = static_cast<TechDrawGui::ViewProviderViewPart*>(gvp);
    if (!vpdvp) {
        return;
    }
    TechDrawGui::QGIView* view = vpdvp->getQView();

    std::vector<std::string> vertexNames = getSelection().getSelectionEx()[0].getSubNames();
    std::vector<int> vertexIndexes = DrawUtil::getIndexFromName(vertexNames);

    std::vector<QGIVertex*> vertexes = view->getObjects<QGIVertex*>(vertexIndexes);
    if(vertexes.size() != 2) {
        Base::Console().Error(QObject::tr("You must select 2 vertexes\n").toStdString().c_str());
        return;
    }
    QGIVertex* v1 = vertexes.front();
    QGIVertex* v2 = vertexes.back();
    DrawGuiUtil::rotateToAlignVertically(v1, v2);
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
    sMenuText       = QT_TR_NOOP("Align vertexes horizontally by view rotation");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_AlignGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawAlignVertexesHorizontally::activated(int iMsg)
{
    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(
        getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId())[0]
    );

    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(dvp->getDocument());
    if (!guiDoc) {
        return;
    }

    Gui::ViewProvider* gvp = guiDoc->getViewProvider(dvp);
    auto vpdvp = static_cast<TechDrawGui::ViewProviderViewPart*>(gvp);
    if (!vpdvp) {
        return;
    }
    TechDrawGui::QGIView* view = vpdvp->getQView();

    std::vector<std::string> vertexNames = getSelection().getSelectionEx()[0].getSubNames();
    std::vector<int> vertexIndexes = DrawUtil::getIndexFromName(vertexNames);

    std::vector<QGIVertex*> vertexes = view->getObjects<QGIVertex*>(vertexIndexes);
    if(vertexes.size() != 2) {
        Base::Console().Error(QObject::tr("You must select 2 vertexes\n").toStdString().c_str());
        return;
    }
    QGIVertex* v1 = vertexes.front();
    QGIVertex* v2 = vertexes.back();
    DrawGuiUtil::rotateToAlignHorizontally(v1, v2);
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
