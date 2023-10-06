/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QAction>
#include <QApplication>
#endif

#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>

//===============================================================================
// PartCmdSelectFilter (dropdown toolbar button for Vertex, Edge & Face Selection)
//===============================================================================

DEF_STD_CMD_ACL(PartCmdSelectFilter)

PartCmdSelectFilter::PartCmdSelectFilter()
  : Command("Part_SelectFilter")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Selection filter");
    sToolTipText  = QT_TR_NOOP("Change the Selection filter");
    sStatusTip    = QT_TR_NOOP("Change the Selection filter");
    sWhatsThis    = "Part_SelectFilter";
    sPixmap       = "selection-filter";
    eType         = Alter3DView;
}

void PartCmdSelectFilter::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0) {
        rcCmdMgr.runCommandByName("Part_VertexSelection");
    }
    else if (iMsg==1) {
        rcCmdMgr.runCommandByName("Part_EdgeSelection");
    }
    else if (iMsg==2) {
        rcCmdMgr.runCommandByName("Part_FaceSelection");
    }
    else if (iMsg==3) {
        rcCmdMgr.runCommandByName("Part_RemoveSelectionGate");
    }
    else {
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> act = pcAction->actions();

    assert(iMsg < act.size());
    pcAction->setIcon(act[iMsg]->icon());
}

Gui::Action * PartCmdSelectFilter::createAction()
{
    auto pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("vertex-selection"));
    cmd0->setShortcut(QKeySequence(QString::fromUtf8("X,S")));
    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("edge-selection"));
    cmd1->setShortcut(QKeySequence(QString::fromUtf8("E,S")));
    QAction* cmd2 = pcAction->addAction(QString());
    cmd2->setIcon(Gui::BitmapFactory().iconFromTheme("face-selection"));
    cmd2->setShortcut(QKeySequence(QString::fromUtf8("F,S")));
    QAction* cmd3 = pcAction->addAction(QString());
    cmd3->setIcon(Gui::BitmapFactory().iconFromTheme("clear-selection"));
    cmd3->setShortcut(QKeySequence(QString::fromUtf8("C,S")));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(Gui::BitmapFactory().iconFromTheme("selection-filter"));
    int defaultId = 3;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void PartCmdSelectFilter::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }

    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> act = pcAction->actions();

    Gui::Command* vertexSelection = rcCmdMgr.getCommandByName("Part_VertexSelection");
    if (vertexSelection) {
        QAction* cmd0 = act[0];
        cmd0->setText(QApplication::translate("View_SelectionFilter", vertexSelection->getMenuText()));
        cmd0->setToolTip(QApplication::translate("View_SelectionFilter", vertexSelection->getToolTipText()));
        cmd0->setStatusTip(QApplication::translate("View_SelectionFilter", vertexSelection->getStatusTip()));
    }

    Gui::Command* edgeSelection = rcCmdMgr.getCommandByName("Part_EdgeSelection");
    if (edgeSelection) {
        QAction* cmd1 = act[1];
        cmd1->setText(QApplication::translate("View_SelectionFilter", edgeSelection->getMenuText()));
        cmd1->setToolTip(QApplication::translate("View_SelectionFilter", edgeSelection->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("View_SelectionFilter", edgeSelection->getStatusTip()));
    }

    Gui::Command* faceSelection = rcCmdMgr.getCommandByName("Part_FaceSelection");
    if (faceSelection) {
        QAction* cmd1 = act[2];
        cmd1->setText(QApplication::translate("View_SelectionFilter", faceSelection->getMenuText()));
        cmd1->setToolTip(QApplication::translate("View_SelectionFilter", faceSelection->getToolTipText()));
        cmd1->setStatusTip(QApplication::translate("View_SelectionFilter", faceSelection->getStatusTip()));
    }

    Gui::Command* removeSelection = rcCmdMgr.getCommandByName("Part_RemoveSelectionGate");
    if (removeSelection) {
        QAction* cmd2 = act[3];
        cmd2->setText(QApplication::translate("View_SelectionFilter", removeSelection->getMenuText()));
        cmd2->setToolTip(QApplication::translate("View_SelectionFilter", removeSelection->getToolTipText()));
        cmd2->setStatusTip(QApplication::translate("View_SelectionFilter", removeSelection->getStatusTip()));
    }
}

bool PartCmdSelectFilter::isActive()
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    return view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId());
}


//===========================================================================
// Part_VertexSelection
//===========================================================================
DEF_3DV_CMD(PartCmdVertexSelection)

PartCmdVertexSelection::PartCmdVertexSelection()
  : Command("Part_VertexSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Vertex Selection");
    sToolTipText  = QT_TR_NOOP("Select a Vertex/Vertices");
    sWhatsThis    = "Part_VertexSelection";
    sStatusTip    = QT_TR_NOOP("Select a Vertex/Vertices");
    sPixmap       = "vertex-selection";
    sAccel        = "X, S";
    eType         = Alter3DView;
}

void PartCmdVertexSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.Selection.addSelectionGate('SELECT Part::Feature SUBELEMENT Vertex')");
}


//===========================================================================
// Part_EdgeSelection
//===========================================================================
DEF_3DV_CMD(PartCmdEdgeSelection)

PartCmdEdgeSelection::PartCmdEdgeSelection()
  : Command("Part_EdgeSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Edge Selection");
    sToolTipText  = QT_TR_NOOP("Select Edge(s)");
    sWhatsThis    = "Part_EdgeSelection";
    sStatusTip    = QT_TR_NOOP("Select Edge(s)");
    sPixmap       = "edge-selection";
    sAccel        = "E, S";
    eType         = Alter3DView;
}

void PartCmdEdgeSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.Selection.addSelectionGate('SELECT Part::Feature SUBELEMENT Edge')");
}


//===========================================================================
// Part_FaceSelection
//===========================================================================
DEF_3DV_CMD(PartCmdFaceSelection)

PartCmdFaceSelection::PartCmdFaceSelection()
  : Command("Part_FaceSelection")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("Face Selection");
    sToolTipText  = QT_TR_NOOP("Select Face(s)");
    sWhatsThis    = "Part_FaceSelection";
    sStatusTip    = QT_TR_NOOP("Select Face(s)");
    sPixmap       = "face-selection";
    sAccel        = "F, S";
    eType         = Alter3DView;
}

void PartCmdFaceSelection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.Selection.addSelectionGate('SELECT Part::Feature SUBELEMENT Face')");
}


//===========================================================================
// Part_RemoveSelectionGate
//===========================================================================
DEF_3DV_CMD(PartCmdRemoveSelectionGate)

PartCmdRemoveSelectionGate::PartCmdRemoveSelectionGate()
  : Command("Part_RemoveSelectionGate")
{
    sGroup        = "Standard-View";
    sMenuText     = QT_TR_NOOP("All selection filters cleared");
    sToolTipText  = QT_TR_NOOP("All selection filters cleared");
    sWhatsThis    = "Part_RemoveSelectionGate";
    sStatusTip    = QT_TR_NOOP("All selection filters cleared");
    sPixmap       = "clear-selection";
    sAccel        = "C, S";
    eType         = Alter3DView;
}

void PartCmdRemoveSelectionGate::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    doCommand(Command::Gui,"Gui.Selection.removeSelectionGate()");
}

void CreatePartSelectCommands()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    // NOLINTBEGIN
    rcCmdMgr.addCommand(new PartCmdSelectFilter());
    rcCmdMgr.addCommand(new PartCmdVertexSelection());
    rcCmdMgr.addCommand(new PartCmdEdgeSelection());
    rcCmdMgr.addCommand(new PartCmdFaceSelection());
    rcCmdMgr.addCommand(new PartCmdRemoveSelectionGate());
    // NOLINTEND
}
