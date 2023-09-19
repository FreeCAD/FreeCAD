/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/Application.h>
#include <Gui/Command.h>

#include "Workbench.h"


using namespace std;

DEF_STD_CMD(CmdStartPage)

CmdStartPage::CmdStartPage()
    : Command("Start_StartPage")
{
    sAppModule = "Start";
    sGroup = QT_TR_NOOP("Start");
    sMenuText = QT_TR_NOOP("Start Page");
    sToolTipText = QT_TR_NOOP("Displays the start page in a browser view");
    sWhatsThis = "Start_StartPage";
    sStatusTip = sToolTipText;
    sPixmap = "StartWorkbench";
}

void CmdStartPage::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    StartGui::Workbench::loadStartPage();
}


void CreateStartCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdStartPage());
}
