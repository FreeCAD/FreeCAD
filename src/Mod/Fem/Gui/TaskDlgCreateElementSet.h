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


#pragma once

#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Fem/App/FemSetElementNodesObject.h>

#include "TaskCreateElementSet.h"
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
class TaskDlgCreateElementSet: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCreateElementSet(Fem::FemSetElementNodesObject*);
    ~TaskDlgCreateElementSet() override;

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
    TaskCreateElementSet* param;
    TaskObjectName* name;

    Fem::FemSetElementNodesObject* FemSetElementNodesObject;
};


}  // namespace FemGui
