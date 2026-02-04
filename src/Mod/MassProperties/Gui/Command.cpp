// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <QApplication>

#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/MainWindow.h>

#include <App/Application.h>
#include <App/Document.h>
#include "TaskMassProperties.h"

DEF_STD_CMD_A(StdCmdMassProperties)

StdCmdMassProperties::StdCmdMassProperties()
    : Command("Std_MassProperties")
{
    sGroup = "MassProperties";
    sMenuText = QT_TR_NOOP("Mass Properties");
    sToolTipText = QT_TR_NOOP("Calculate mass properties of selected objects");
    sWhatsThis = "Std_MassProperties";
    sStatusTip = QT_TR_NOOP("Calculate mass properties of selected objects");
    sPixmap = "PropertiesIcon";
}

void StdCmdMassProperties::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    
    MassPropertiesGui::TaskMassProperties* task = new MassPropertiesGui::TaskMassProperties();
    task->setDocumentName(this->getDocument()->getName());
    Gui::Control().showDialog(task);
}

bool StdCmdMassProperties::isActive()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return false;
    }
    return Gui::Control().activeDialog() == nullptr;
}

void CreateMassPropertiesCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    auto cmd = new StdCmdMassProperties();
    cmd->initAction();
    rcCmdMgr.addCommand(cmd);
}
