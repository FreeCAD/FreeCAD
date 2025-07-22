/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *                                                                         *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
 ***************************************************************************/

#include "PreCompiled.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>

#include "TaskDlgCreateElementSet.h"
#include "ViewProviderFemMesh.h"


using namespace FemGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgCreateElementSet::TaskDlgCreateElementSet(Fem::FemSetElementNodesObject* obj)
    : TaskDialog()
    , FemSetElementNodesObject(obj)
{
    name = new TaskObjectName(obj);
    param = new TaskCreateElementSet(obj);

    Content.push_back(name);
    Content.push_back(param);
}

TaskDlgCreateElementSet::~TaskDlgCreateElementSet()
{}

//==== calls from the TaskView ===============================================================


void TaskDlgCreateElementSet::open()
{
    // select->activate();
    // Edge2TaskObject->execute();
    // param->setEdgeAndClusterNbr(Edge2TaskObject->NbrOfEdges,Edge2TaskObject->NbrOfCluster);
}

bool TaskDlgCreateElementSet::accept()
{
    try {
        FemSetElementNodesObject->Elements.setValues(param->elementTempSet);
        FemSetElementNodesObject->recomputeFeature();
        param->MeshViewProvider->resetHighlightNodes();
        FemSetElementNodesObject->Label.setValue(name->name);
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");

        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().warning("TaskDlgCreateElementSet::accept(): %s\n", e.what());
    }

    return false;
}

bool TaskDlgCreateElementSet::reject()
{
    FemSetElementNodesObject->execute();
    param->MeshViewProvider->resetHighlightNodes();
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");

    return true;
}

void TaskDlgCreateElementSet::helpRequested()
{}

#include "moc_TaskDlgCreateElementSet.cpp"
