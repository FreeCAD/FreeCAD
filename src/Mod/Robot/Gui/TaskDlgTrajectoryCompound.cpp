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
#include <QApplication>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/TaskView/TaskSelectLinkProperty.h>

#include "TaskDlgTrajectoryCompound.h"


using namespace RobotGui;

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTrajectoryCompound::TaskDlgTrajectoryCompound(Robot::TrajectoryCompound* obj)
    : TaskDialog()
    , TrajectoryCompound(obj)
{
    select = new Gui::TaskView::TaskSelectLinkProperty("SELECT Robot::TrajectoryObject COUNT 1..",
                                                       &(obj->Source));

    Content.push_back(select);
}

//==== calls from the TaskView ===============================================================


void TaskDlgTrajectoryCompound::open()
{
    select->activate();
}


bool TaskDlgTrajectoryCompound::accept()
{
    if (select->isSelectionValid()) {
        select->accept();
        TrajectoryCompound->execute();
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            doc->resetEdit();
        }
        return true;
    }
    else {
        QApplication::beep();
    }

    return false;
}

bool TaskDlgTrajectoryCompound::reject()
{
    select->reject();
    TrajectoryCompound->execute();
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        doc->resetEdit();
    }
    return true;
}

void TaskDlgTrajectoryCompound::helpRequested()
{}


#include "moc_TaskDlgTrajectoryCompound.cpp"
