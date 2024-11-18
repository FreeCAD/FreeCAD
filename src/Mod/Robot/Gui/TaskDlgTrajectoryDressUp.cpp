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

#include <Gui/Application.h>
#include <Gui/Document.h>

#include "TaskDlgTrajectoryDressUp.h"


using namespace RobotGui;

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTrajectoryDressUp::TaskDlgTrajectoryDressUp(Robot::TrajectoryDressUpObject* obj)
    : TaskDialog()
    , pcObject(obj)
{
    param = new TaskTrajectoryDressUpParameter(obj);

    Content.push_back(param);
}

//==== calls from the TaskView ===============================================================


void TaskDlgTrajectoryDressUp::open()
{}

void TaskDlgTrajectoryDressUp::clicked(int button)
{
    if (QDialogButtonBox::Apply == button) {
        // transfer the values to the object
        param->writeValues();
        // May throw an exception which we must handle here
        pcObject->recomputeFeature();
    }
}

bool TaskDlgTrajectoryDressUp::accept()
{
    param->writeValues();
    pcObject->recomputeFeature();

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        doc->resetEdit();
    }
    return true;
}

bool TaskDlgTrajectoryDressUp::reject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        doc->resetEdit();
    }
    return true;
}

void TaskDlgTrajectoryDressUp::helpRequested()
{}


#include "moc_TaskDlgTrajectoryDressUp.cpp"
