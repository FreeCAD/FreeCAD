// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QApplication>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "TaskMeasure.h"
#include "TaskMassProperties.h"

namespace
{
/// Whether the active 3D view has geometry to measure and is not in an edit mode.
bool canMeasure()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType<App::GeoFeature>() == 0) {
        return false;
    }

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom<Gui::View3DInventor>()) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }
    return false;
}
}  // namespace

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
    sToolTipText = QT_TR_NOOP("Measures a feature");
    sWhatsThis = "Std_Measure";
    sStatusTip = QT_TR_NOOP("Measures a feature");
    sPixmap = "umf-measurement";
}

void StdCmdMeasure::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    MeasureGui::TaskMeasure* task = new MeasureGui::TaskMeasure();
    task->setDocumentName(this->getDocument()->getName());
    Gui::Control().showDialog(task);
}

bool StdCmdMeasure::isActive()
{
    return canMeasure();
}

void CreateMeasureCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    auto cmd = new StdCmdMeasure();
    cmd->initAction();
    rcCmdMgr.addCommand(cmd);
}

DEF_STD_CMD_A(StdCmdMassProperties)

StdCmdMassProperties::StdCmdMassProperties()
    : Command("Std_MassProperties")
{
    sGroup = "MassProperties";
    sMenuText = QT_TR_NOOP("Mass Properties");
    sToolTipText = QT_TR_NOOP("Calculates mass properties of selected objects");
    sWhatsThis = "Std_MassProperties";
    sStatusTip = sToolTipText;
    sPixmap = "MassPropertiesIcon";
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
    return canMeasure();
}

//===========================================================================
// Std_MeasureGroup
// Measure button with Mass Properties in its drop-down
//===========================================================================

class StdCmdMeasureGroup: public Gui::GroupCommand
{
public:
    StdCmdMeasureGroup()
        : GroupCommand("Std_MeasureGroup")
    {
        sGroup = "Measure";
        sMenuText = QT_TR_NOOP("Measure");
        sToolTipText = QT_TR_NOOP("Measures features or calculates mass properties");
        sWhatsThis = "Std_MeasureGroup";
        sStatusTip = sToolTipText;
        sPixmap = "umf-measurement";

        setCheckable(false);
        setRememberLast(false);

        addCommand("Std_Measure");
        addCommand("Std_MassProperties");
    }

    const char* className() const override
    {
        return "StdCmdMeasureGroup";
    }

protected:
    bool isActive() override
    {
        return canMeasure();
    }
};

void CreateMassPropertiesCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    auto cmd = new StdCmdMassProperties();
    cmd->initAction();
    rcCmdMgr.addCommand(cmd);

    rcCmdMgr.addCommand(new StdCmdMeasureGroup());
}
