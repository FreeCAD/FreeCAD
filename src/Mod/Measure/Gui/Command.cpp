/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>

#include "TaskMeasure.h"


//===========================================================================
// Std_Measure
// this is the Unified Measurement Facility Measure command
//===========================================================================


DEF_STD_CMD_A(StdCmdMeasure)

StdCmdMeasure::StdCmdMeasure()
    : Command("Std_Measure")
{
    sGroup = "Measure";
    sMenuText = QT_TR_NOOP("&Measure");
    sToolTipText = QT_TR_NOOP("Measure a feature");
    sWhatsThis = "Std_Measure";
    sStatusTip = QT_TR_NOOP("Measure a feature");
    sPixmap = "umf-measurement";
}

void StdCmdMeasure::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskMeasure* task = new Gui::TaskMeasure();
    Gui::Control().showDialog(task);
}


bool StdCmdMeasure::isActive()
{
    return true;
}




void CreateMeasureCommands() {
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    auto cmd = new StdCmdMeasure();
    cmd->initAction();
    rcCmdMgr.addCommand(cmd);

}
