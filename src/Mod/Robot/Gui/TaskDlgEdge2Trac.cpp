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

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include <Gui/TaskView/TaskSelectLinkProperty.h>

#include "TaskDlgEdge2Trac.h"


using namespace RobotGui;

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgEdge2Trac::TaskDlgEdge2Trac(Robot::Edge2TracObject* obj)
    : TaskDialog()
    , Edge2TaskObject(obj)
{
    param = new TaskEdge2TracParameter(obj);
    select =
        new Gui::TaskView::TaskSelectLinkProperty("SELECT Part::Feature SUBELEMENT Edge COUNT 1..",
                                                  &(obj->Source));

    Content.push_back(param);
    Content.push_back(select);
}

//==== calls from the TaskView ===============================================================


void TaskDlgEdge2Trac::open()
{
    select->activate();
    Edge2TaskObject->execute();
    param->setEdgeAndClusterNbr(Edge2TaskObject->NbrOfEdges, Edge2TaskObject->NbrOfCluster);
}

void TaskDlgEdge2Trac::clicked(int button)
{
    try {
        if (QDialogButtonBox::Apply == button) {
            if (select->isSelectionValid()) {
                select->sendSelection2Property();
                // May throw an exception which we must handle here
                Edge2TaskObject->execute();
                param->setEdgeAndClusterNbr(Edge2TaskObject->NbrOfEdges,
                                            Edge2TaskObject->NbrOfCluster);
            }
            else {
                QApplication::beep();
                param->setEdgeAndClusterNbr(0, 0);
            }
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning("TaskDlgEdge2Trac::clicked(): %s\n", e.what());
    }
}

bool TaskDlgEdge2Trac::accept()
{
    try {
        if (select->isSelectionValid()) {
            select->accept();
            Edge2TaskObject->recomputeFeature();
            Gui::Document* doc = Gui::Application::Instance->activeDocument();
            if (doc) {
                doc->resetEdit();
            }
            return true;
        }
        else {
            QApplication::beep();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().Warning("TaskDlgEdge2Trac::accept(): %s\n", e.what());
    }

    return false;
}

bool TaskDlgEdge2Trac::reject()
{
    select->reject();
    Edge2TaskObject->execute();
    return true;
}

void TaskDlgEdge2Trac::helpRequested()
{}


#include "moc_TaskDlgEdge2Trac.cpp"
