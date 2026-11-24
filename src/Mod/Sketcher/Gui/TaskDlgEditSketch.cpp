// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
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


#include <Gui/Command.h>

#include "TaskDlgEditSketch.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;

namespace sp = std::placeholders;

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgEditSketch::TaskDlgEditSketch(ViewProviderSketch* sketchView)
    : TaskDialog()
    , sketchView(sketchView)
{
    assert(sketchView);
    roleOnEscape = QDialogButtonBox::ButtonRole::AcceptRole;

    ToolSettings = new TaskSketcherTool(sketchView);
    Constraints = new TaskSketcherConstraints(sketchView);
    Elements = new TaskSketcherElements(sketchView);
    Messages = new TaskSketcherMessages(sketchView);
    SolverAdvanced = new TaskSketcherSolverAdvanced(sketchView);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher"
    );
    setEscapeButtonEnabled(hGrp->GetBool("LeaveSketchWithEscape", true));
    setAutoCloseOnResetEdit(true);

    Content.push_back(Messages);
    Content.push_back(ToolSettings);

    if (hGrp->GetBool("ShowSolverAdvancedWidget", false)) {
        Content.push_back(SolverAdvanced);
    }

    Content.push_back(Constraints);
    Content.push_back(Elements);

    if (!hGrp->GetBool("ExpandedMessagesWidget", true)) {
        Messages->hideGroupBox();
    }
    if (!hGrp->GetBool("ExpandedSolverAdvancedWidget", false)) {
        SolverAdvanced->hideGroupBox();
    }
    if (!hGrp->GetBool("ExpandedConstraintsWidget", true)) {
        Constraints->hideGroupBox();
    }
    if (!hGrp->GetBool("ExpandedElementsWidget", true)) {
        Elements->hideGroupBox();
    }

    connectionToolSettings = sketchView->registerToolChanged(
        std::bind(&SketcherGui::TaskDlgEditSketch::slotToolChanged, this, sp::_1)
    );

    ToolSettings->setHidden(true);

    associateToObject3dView(sketchView->getObject());
}

TaskDlgEditSketch::~TaskDlgEditSketch()
{
    // to make sure to delete the advanced solver panel
    // it must be part to the 'Content' array
    if (const auto it = std::ranges::find(Content, SolverAdvanced); it == Content.end()) {
        Content.push_back(SolverAdvanced);
    }

    connectionToolSettings.disconnect();
}

void TaskDlgEditSketch::slotToolChanged(const std::string& toolname)
{
    bool widgetvisible = false;

    if (toolname != "DSH_None") {
        widgetvisible = sketchView->toolManager.isWidgetVisible();

        ToolSettings->toolChanged(toolname);
    }

    ToolSettings->setHidden(!widgetvisible);
}

//==== calls from the TaskView ===============================================================


void TaskDlgEditSketch::open()
{}

void TaskDlgEditSketch::closed()
{}

void TaskDlgEditSketch::clicked(int)
{}

bool TaskDlgEditSketch::reject()
{
    ViewProviderSketch* view = sketchView;
    std::string document = getDocumentName();  // needed because resetEdit() deletes this instance
    view->editingCancelled = true;
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document.c_str());
    view->editingCancelled = false;

    return true;
}

bool TaskDlgEditSketch::accept()
{
    std::string document = getDocumentName();  // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.getDocument('%s').recompute()", document.c_str());

    return true;
}

void TaskDlgEditSketch::saveDialogState() const
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher"
    );
    hGrp->SetBool("ExpandedMessagesWidget", Messages->isGroupVisible());
    hGrp->SetBool("ExpandedSolverAdvancedWidget", SolverAdvanced->isGroupVisible());
    hGrp->SetBool("ExpandedConstraintsWidget", Constraints->isGroupVisible());
    hGrp->SetBool("ExpandedElementsWidget", Elements->isGroupVisible());
}

QDialogButtonBox::StandardButtons TaskDlgEditSketch::getStandardButtons() const
{
    return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
}

void TaskDlgEditSketch::autoClosedOnResetEdit()
{
    saveDialogState();
}

void TaskDlgEditSketch::autoClosedOnClosedView()
{
    // Make sure the edit mode is exited when the view is closed.
    reject();
}

#include "moc_TaskDlgEditSketch.cpp"
