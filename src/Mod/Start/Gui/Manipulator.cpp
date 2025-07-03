// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QCoreApplication>
#include <QCoreApplication>
#include <QLayout>
#endif

#include "Manipulator.h"
#include "StartView.h"

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>

#include <gsl/pointers>

DEF_STD_CMD(CmdStart)

CmdStart::CmdStart()
    : Command("Start_Start")
{
    sAppModule = "Start";
    sGroup = QT_TR_NOOP("Start");
    sMenuText = QT_TR_NOOP("&Start Page");
    sToolTipText = QT_TR_NOOP("Displays the start page");
    sWhatsThis = "Start_Start";
    sStatusTip = sToolTipText;
    sPixmap = "StartCommandIcon";
}

void CmdStart::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    auto mw = Gui::getMainWindow();
    auto existingView = mw->findChild<StartGui::StartView*>(QLatin1String("StartView"));
    if (!existingView) {
        existingView = gsl::owner<StartGui::StartView*>(new StartGui::StartView(mw));
        mw->addWindow(existingView);  // Transfers ownership
    }
    Gui::getMainWindow()->setActiveWindow(existingView);
    existingView->show();
}

void StartGui::Manipulator::modifyMenuBar(Gui::MenuItem* menuBar)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    if (!rcCmdMgr.getCommandByName("Start_Start")) {
        auto newCommand = gsl::owner<CmdStart*>(new CmdStart);
        rcCmdMgr.addCommand(newCommand);  // Transfer ownership
    }

    Gui::MenuItem* helpMenu = menuBar->findItem("&Help");
    Gui::MenuItem* loadStart = new Gui::MenuItem();
    Gui::MenuItem* loadSeparator = new Gui::MenuItem();
    loadStart->setCommand("Start_Start");
    loadSeparator->setCommand("Separator");
    Gui::MenuItem* firstItem = helpMenu->findItem("Std_FreeCADUserHub");
    helpMenu->insertItem(firstItem, loadStart);
    helpMenu->insertItem(firstItem, loadSeparator);
}
