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
#ifndef _PreComp_
#include <QApplication>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "QuickMeasure.h"
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

    MeasureGui::TaskMeasure* task = new MeasureGui::TaskMeasure();
    task->setDocumentName(this->getDocument()->getName());
    Gui::Control().showDialog(task);
}

bool StdCmdMeasure::isActive()
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


class StdCmdQuickMeasure: public Gui::Command
{
public:
    StdCmdQuickMeasure()
        : Command("Std_QuickMeasure")
    {
        sGroup = "Measure";
        sMenuText = QT_TR_NOOP("&Quick measure");
        sToolTipText = QT_TR_NOOP("Toggle quick measure");
        sWhatsThis = "Std_QuickMeasure";
        sStatusTip = QT_TR_NOOP("Toggle quick measure");
        accessParameter();
    }
    ~StdCmdQuickMeasure() override = default;
    StdCmdQuickMeasure(const StdCmdQuickMeasure&) = delete;
    StdCmdQuickMeasure(StdCmdQuickMeasure&&) = delete;
    StdCmdQuickMeasure& operator=(const StdCmdQuickMeasure&) = delete;
    StdCmdQuickMeasure& operator=(StdCmdQuickMeasure&&) = delete;

    const char* className() const override
    {
        return "StdCmdQuickMeasure";
    }

protected:
    void activated(int iMsg) override
    {
        if (parameter.isValid()) {
            parameter->SetBool("EnableQuickMeasure", iMsg > 0);
        }

        if (iMsg == 0) {
            if (quickMeasure) {
                quickMeasure->print(QString());
            }
            quickMeasure.reset();
        }
        else {
            quickMeasure = std::make_unique<MeasureGui::QuickMeasure>(QApplication::instance());
        }
    }
    Gui::Action* createAction() override
    {
        Gui::Action* action = Gui::Command::createAction();
        action->setCheckable(true);
        action->setChecked(parameter->GetBool("EnableQuickMeasure", true));
        return action;
    }
    void accessParameter()
    {
        // clang-format off
        parameter = App::GetApplication().GetUserParameter().
                    GetGroup("BaseApp/Preferences/Mod/Measure");
        // clang-format on
    }

private:
    std::unique_ptr<MeasureGui::QuickMeasure> quickMeasure;
    ParameterGrp::handle parameter;
};

void CreateMeasureCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    auto cmd = new StdCmdMeasure();
    cmd->initAction();
    rcCmdMgr.addCommand(cmd);
    rcCmdMgr.addCommand(new StdCmdQuickMeasure);
}
