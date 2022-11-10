/***************************************************************************
 *   Copyright (c) 2022 Peter McB                                          *
 *                                                                         *
 *   based on: TaskDlgCreateNodeSet.cpp                                    *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>


#include "TaskDlgCreateElementSet.h"
#include "ViewProviderFemMesh.h"

//#include <Gui/TaskView/TaskSelectLinkProperty.h>
//#include <Mod/Fem/App/FemMeshObject.h>

using namespace FemGui;


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgCreateElementSet::TaskDlgCreateElementSet(Fem::FemSetElementNodesObject *obj)
    : TaskDialog(),FemSetElementNodesObject(obj)
{
    name    = new TaskObjectName(obj);
    param   = new TaskCreateElementSet(obj);

    Content.push_back(name);
    Content.push_back(param);
}

TaskDlgCreateElementSet::~TaskDlgCreateElementSet()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgCreateElementSet::open()
{
    //select->activate();
    //Edge2TaskObject->execute();
    //param->setEdgeAndClusterNbr(Edge2TaskObject->NbrOfEdges,Edge2TaskObject->NbrOfCluster);

}

bool TaskDlgCreateElementSet::accept()
{
    try {

        FemSetElementNodesObject->Elements.setValues(param->elementTempSet);
        FemSetElementNodesObject->recomputeFeature();

        param->MeshViewProvider->resetHighlightNodes();
        FemSetElementNodesObject->Label.setValue(name->name);
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

        return true;
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning("TaskDlgCreateElementSet::accept(): %s\n", e.what());
    }

    return false;
}

bool TaskDlgCreateElementSet::reject()
{
    FemSetElementNodesObject->execute();
    param->MeshViewProvider->resetHighlightNodes();
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}

void TaskDlgCreateElementSet::helpRequested()
{

}

#include "moc_TaskDlgCreateElementSet.cpp"
