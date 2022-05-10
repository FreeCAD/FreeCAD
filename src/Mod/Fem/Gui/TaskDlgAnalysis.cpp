/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#endif

#include "TaskDlgAnalysis.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/TaskView/TaskSelectLinkProperty.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>

#include <Mod/Fem/App/FemAnalysis.h>

#include "TaskAnalysisInfo.h"
#include "TaskDriver.h"


using namespace FemGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgAnalysis::TaskDlgAnalysis(Fem::FemAnalysis* obj)
    : TaskDialog(), FemAnalysis(obj)
{
    driver  = new TaskDriver(obj);
    info    = new TaskAnalysisInfo(obj);

    Content.push_back(driver);
    Content.push_back(info);
}

TaskDlgAnalysis::~TaskDlgAnalysis()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgAnalysis::open()
{
    //select->activate();
    //Edge2TaskObject->execute();
    //param->setEdgeAndClusterNbr(Edge2TaskObject->NbrOfEdges,Edge2TaskObject->NbrOfCluster);

}

bool TaskDlgAnalysis::accept()
{
    //try {
    //    FemSetNodesObject->Nodes.setValues(param->tempSet);
    //    FemSetNodesObject->recompute();
    //    //Gui::Document* doc = Gui::Application::Instance->activeDocument();
    //    //if(doc)
    //    //    doc->resetEdit();
    //    param->MeshViewProvider->resetHighlightNodes();
    //    FemSetNodesObject->Label.setValue(name->name);
    //    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    //    return true;
    //}
    //catch (const Base::Exception& e) {
    //    Base::Console().Warning("TaskDlgAnalysis::accept(): %s\n", e.what());
    //}

    return false;
}

bool TaskDlgAnalysis::reject()
{
    //FemSetNodesObject->execute();
    //    //Gui::Document* doc = Gui::Application::Instance->activeDocument();
    //    //if(doc)
    //    //    doc->resetEdit();
    //param->MeshViewProvider->resetHighlightNodes();
    //Gui::Command::abortCommand();
    //Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}

void TaskDlgAnalysis::helpRequested()
{

}

#include "moc_TaskDlgAnalysis.cpp"
