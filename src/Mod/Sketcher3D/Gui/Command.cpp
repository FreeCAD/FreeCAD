// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>


namespace Sketcher3DGui
{

DEF_STD_CMD_A(CmdSketcher3DCreateSketch)

CmdSketcher3DCreateSketch::CmdSketcher3DCreateSketch()
    : Command("Sketcher3D_CreateSketch")
{
    sAppModule = "Sketcher3D";
    sGroup = QT_TR_NOOP("Sketcher3D");
    sMenuText = QT_TR_NOOP("Create 3D sketch");
    sToolTipText = QT_TR_NOOP("Creates a new 3D sketch");
    sWhatsThis = "Sketcher3D_CreateSketch";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_NewSketch";
}

void CmdSketcher3DCreateSketch::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    std::string FeatName = getUniqueObjectName("Sketch3D");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create a new 3D sketch"));
    doCommand(
        Doc,
        "App.activeDocument().addObject('Sketcher3D::Sketch3DObject', '%s')",
        FeatName.c_str()
    );
    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    commitCommand();
}

bool CmdSketcher3DCreateSketch::isActive()
{
    return hasActiveDocument();
}

}  // namespace Sketcher3DGui


void CreateSketcher3DCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new Sketcher3DGui::CmdSketcher3DCreateSketch());
}
