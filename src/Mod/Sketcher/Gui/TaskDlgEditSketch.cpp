/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
    ToolSettings = new TaskSketcherTool(sketchView);
    Constraints = new TaskSketcherConstraints(sketchView);
    Elements = new TaskSketcherElements(sketchView);
    Messages = new TaskSketcherMessages(sketchView);
    SolverAdvanced = new TaskSketcherSolverAdvanced(sketchView);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    setEscapeButtonEnabled(hGrp->GetBool("LeaveSketchWithEscape", true));

    Content.push_back(ToolSettings);
    Content.push_back(Messages);

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
        std::bind(&SketcherGui::TaskDlgEditSketch::slotToolChanged, this, sp::_1));

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

void TaskDlgEditSketch::clicked(int)
{}

bool TaskDlgEditSketch::accept()
{
    return true;
}

bool TaskDlgEditSketch::reject()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrp->SetBool("ExpandedMessagesWidget", Messages->isGroupVisible());
    hGrp->SetBool("ExpandedSolverAdvancedWidget", SolverAdvanced->isGroupVisible());
    hGrp->SetBool("ExpandedConstraintsWidget", Constraints->isGroupVisible());
    hGrp->SetBool("ExpandedElementsWidget", Elements->isGroupVisible());

    if (sketchView && sketchView->getSketchMode() != ViewProviderSketch::STATUS_NONE) {
        sketchView->purgeHandler();
    }

    std::string document = getDocumentName();  // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui,
                            "Gui.getDocument('%s').resetEdit()",
                            document.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.getDocument('%s').recompute()",
                            document.c_str());

    return true;
}

QDialogButtonBox::StandardButtons TaskDlgEditSketch::getStandardButtons() const
{
    return QDialogButtonBox::Close;
}

void TaskDlgEditSketch::autoClosedOnClosedView()
{
    // Make sure the edit mode is exited when the view is closed.
    reject();
}

#include "moc_TaskDlgEditSketch.cpp"
