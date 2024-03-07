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

#include <Gui/Application.h>
#include <Gui/Command.h>

#include <3rdParty/GSL/include/gsl/pointers>

#include "Workbench.h"


using namespace std;

DEF_STD_CMD(CmdCleanStart)

CmdCleanStart::CmdCleanStart()
    : Command("CleanStart_CleanStart")
{
    sAppModule = "CleanStart";
    sGroup = QT_TR_NOOP("CleanStart");
    sMenuText = QT_TR_NOOP("CleanStart");
    sToolTipText = QT_TR_NOOP("Displays the CleanStart in an MDI view");
    sWhatsThis = "CleanStart_CleanStart";
    sStatusTip = sToolTipText;
    sPixmap = "CleanStartWorkbench";
}

void CmdCleanStart::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    CleanStartGui::Workbench::loadCleanStart();
}


void CreateCleanStartCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    auto newCommand = gsl::owner<CmdCleanStart*>(new CmdCleanStart);
    rcCmdMgr.addCommand(newCommand);  // Transfer ownership
}
