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


#ifndef ROBOTGUI_TaskDlgEdge2Trac_H
#define ROBOTGUI_TaskDlgEdge2Trac_H

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>
#include <Mod/Robot/App/Edge2TracObject.h>

#include "TaskEdge2TracParameter.h"

// forward
namespace Gui { namespace TaskView { class TaskSelectLinkProperty;}}


namespace RobotGui {


/// simulation dialog for the TaskView
class RobotGuiExport TaskDlgEdge2Trac : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgEdge2Trac(Robot::Edge2TracObject *);
    ~TaskDlgEdge2Trac();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or rject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user press the help button 
    virtual void helpRequested();

    /// returns for Close and Help button 
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel; }

protected:
    TaskEdge2TracParameter                *param; 
    Gui::TaskView::TaskSelectLinkProperty *select;
    Robot::Edge2TracObject                *Edge2TaskObject;
};



} //namespace RobotGui

#endif // ROBOTGUI_TASKDLGSIMULATE_H
