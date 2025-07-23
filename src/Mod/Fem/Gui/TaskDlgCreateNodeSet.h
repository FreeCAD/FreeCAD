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

#ifndef ROBOTGUI_TaskDlgCreateNodeSet_H
#define ROBOTGUI_TaskDlgCreateNodeSet_H

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Fem/App/FemSetNodesObject.h>

#include "TaskCreateNodeSet.h"
#include "TaskObjectName.h"


// forward
namespace Gui
{
namespace TaskView
{
class TaskSelectLinkProperty;
}
}  // namespace Gui


namespace FemGui
{


/// simulation dialog for the TaskView
class TaskDlgCreateNodeSet: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgCreateNodeSet(Fem::FemSetNodesObject*);
    ~TaskDlgCreateNodeSet() override;

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user press the help button
    void helpRequested() override;

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

protected:
    TaskCreateNodeSet* param;
    TaskObjectName* name;

    Fem::FemSetNodesObject* FemSetNodesObject;
};


}  // namespace FemGui

#endif  // ROBOTGUI_TASKDLGSIMULATE_H
