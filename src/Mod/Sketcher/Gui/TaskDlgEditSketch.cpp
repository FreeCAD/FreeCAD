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

#ifndef _PreComp_
# include <boost_bind_bind.hpp>
#endif

#include "TaskDlgEditSketch.h"
#include "ViewProviderSketch.h"
#include <Gui/Command.h>

using namespace SketcherGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgEditSketch::TaskDlgEditSketch(ViewProviderSketch *sketchView)
    : TaskDialog(),sketchView(sketchView)
{
    assert(sketchView);
    sketchView->toolSettings = new TaskSketcherTool(sketchView);
    Constraints = new TaskSketcherConstraints(sketchView);
    Elements = new TaskSketcherElements(sketchView);
    General = new TaskSketcherGeneral(sketchView);
    Messages = new TaskSketcherMessages(sketchView);
    SolverAdvanced = new TaskSketcherSolverAdvanced(sketchView);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    setEscapeButtonEnabled(hGrp->GetBool("LeaveSketchWithEscape", true));

    Content.push_back(sketchView->toolSettings);
    Content.push_back(Messages);

    if (hGrp->GetBool("ShowSolverAdvancedWidget",false)) {
        Content.push_back(SolverAdvanced);
    }

    Content.push_back(General);
    Content.push_back(Constraints);
    Content.push_back(Elements);

    if (!hGrp->GetBool("ExpandedMessagesWidget",true))
        Messages->hideGroupBox();
    if (!hGrp->GetBool("ExpandedSolverAdvancedWidget",false))
        SolverAdvanced->hideGroupBox();
    if (!hGrp->GetBool("ExpandedEditControlWidget",false))
        General->hideGroupBox();
    if (!hGrp->GetBool("ExpandedConstraintsWidget",true))
        Constraints->hideGroupBox();
    if (!hGrp->GetBool("ExpandedElementsWidget",true))
        Elements->hideGroupBox();
}

TaskDlgEditSketch::~TaskDlgEditSketch()
{
    // to make sure to delete the advanced solver panel
    // it must be part to the 'Content' array
    std::vector<QWidget*>::iterator it = std::find(Content.begin(), Content.end(), SolverAdvanced);
    if (it == Content.end())
        Content.push_back(SolverAdvanced);
}

//==== calls from the TaskView ===============================================================


void TaskDlgEditSketch::open()
{

}

void TaskDlgEditSketch::clicked(int)
{

}

bool TaskDlgEditSketch::accept()
{
    return true;
}

bool TaskDlgEditSketch::reject()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    hGrp->SetBool("ExpandedMessagesWidget",Messages->isGroupVisible());
    hGrp->SetBool("ExpandedSolverAdvancedWidget",SolverAdvanced->isGroupVisible());
    hGrp->SetBool("ExpandedEditControlWidget",General->isGroupVisible());
    hGrp->SetBool("ExpandedConstraintsWidget",Constraints->isGroupVisible());
    hGrp->SetBool("ExpandedElementsWidget",Elements->isGroupVisible());

    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.getDocument('%s').resetEdit()", document.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument('%s').recompute()", document.c_str());

    return true;
}


#include "moc_TaskDlgEditSketch.cpp"
