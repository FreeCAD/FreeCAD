/***************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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
# include <QApplication>
# include <QMessageBox>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "DrawGuiUtil.h"
#include "ViewProviderDrawingView.h"


using namespace TechDrawGui;
using namespace TechDraw;

void execStackTop(Gui::Command* cmd);
void execStackBottom(Gui::Command* cmd);
void execStackUp(Gui::Command* cmd);
void execStackDown(Gui::Command* cmd);

//===========================================================================
// TechDraw_StackGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawStackGroup)

CmdTechDrawStackGroup::CmdTechDrawStackGroup()
  : Command("TechDraw_StackGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Adjust stacking order of views");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_StackGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawStackGroup::activated(int iMsg)
{
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:
            execStackTop(this);
            break;
        case 1:
            execStackBottom(this);
            break;
        case 2:
            execStackUp(this);
            break;
        case 3:
            execStackDown(this);
            break;
        default:
            Base::Console().Message("CMD::StackGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawStackGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_StackTop"));
    p1->setObjectName(QString::fromLatin1("TechDraw_StackTop"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_StackTop"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_StackBottom"));
    p2->setObjectName(QString::fromLatin1("TechDraw_StackBottom"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_StackBottom"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_StackUp"));
    p3->setObjectName(QString::fromLatin1("TechDraw_StackUp"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_StackUp"));
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_StackDown"));
    p4->setObjectName(QString::fromLatin1("TechDraw_StackDown"));
    p4->setWhatsThis(QString::fromLatin1("TechDraw_StackDown"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawStackGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawStackGroup","Stack Top"));
    arc1->setToolTip(QApplication::translate("TechDraw_StackTop","Move view to top of stack"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawStackGroup","Stack Bottom"));
    arc2->setToolTip(QApplication::translate("TechDraw_StackBottom","Move view to bottom of stack"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawStackGroup","Stack Up"));
    arc3->setToolTip(QApplication::translate("TechDraw_StackUp","Move view up one level"));
    arc3->setStatusTip(arc3->toolTip());
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("CmdTechDrawStackGroup","Stack Down"));
    arc4->setToolTip(QApplication::translate("TechDraw_StackDown","Move view down one level"));
    arc4->setStatusTip(arc4->toolTip());
}

bool CmdTechDrawStackGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_StackTop
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawStackTop)

CmdTechDrawStackTop::CmdTechDrawStackTop()
  : Command("TechDraw_StackTop")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Move view to top of stack");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_StackTop";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_StackTop";
}

void CmdTechDrawStackTop::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execStackTop(this);
}

bool CmdTechDrawStackTop::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execStackTop(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = cmd->getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (!views.empty()) {
        for (auto& v: views) {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(
                                    cmd->getDocument())->getViewProvider(dv);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vp);
            if (vpdv) {
                vpdv->stackTop();
            }
        }
    }
}

//===========================================================================
// TechDraw_StackBottom
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawStackBottom)

CmdTechDrawStackBottom::CmdTechDrawStackBottom()
  : Command("TechDraw_StackBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Move view to bottom of stack");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_StackBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_StackBottom";
}

void CmdTechDrawStackBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execStackBottom(this);
}

bool CmdTechDrawStackBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execStackBottom(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = cmd->getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (!views.empty()) {
        for (auto& v: views) {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(
                                    cmd->getDocument())->getViewProvider(dv);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vp);
            if (vpdv) {
                vpdv->stackBottom();
            }
        }
    }
}

//===========================================================================
// TechDraw_StackUp
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawStackUp)

CmdTechDrawStackUp::CmdTechDrawStackUp()
  : Command("TechDraw_StackUp")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Move view up one level");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_StackUp";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_StackUp";
}

void CmdTechDrawStackUp::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execStackUp(this);
}

bool CmdTechDrawStackUp::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execStackUp(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = cmd->getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (!views.empty()) {
        for (auto& v: views) {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(
                                    cmd->getDocument())->getViewProvider(dv);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vp);
            if (vpdv) {
                vpdv->stackUp();
            }
        }
    }
}

//===========================================================================
// TechDraw_StackDown
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawStackDown)

CmdTechDrawStackDown::CmdTechDrawStackDown()
  : Command("TechDraw_StackDown")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Move view down one level");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_StackDown";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_StackDown";
}

void CmdTechDrawStackDown::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execStackDown(this);
}

bool CmdTechDrawStackDown::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execStackDown(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> views = cmd->getSelection().getObjectsOfType(TechDraw::DrawView::getClassTypeId());
    if (!views.empty()) {
        for (auto& v: views) {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(
                                    cmd->getDocument())->getViewProvider(dv);
            ViewProviderDrawingView* vpdv = static_cast<ViewProviderDrawingView*>(vp);
            if (vpdv) {
                vpdv->stackDown();
            }
        }
    }
}

void CreateTechDrawCommandsStack(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawStackGroup());
    rcCmdMgr.addCommand(new CmdTechDrawStackTop());
    rcCmdMgr.addCommand(new CmdTechDrawStackBottom());
    rcCmdMgr.addCommand(new CmdTechDrawStackUp());
    rcCmdMgr.addCommand(new CmdTechDrawStackDown());
}
