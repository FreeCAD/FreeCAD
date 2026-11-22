// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <QEvent>


#include <Gui/BitmapFactory.h>

#include "TaskSketcherCreateCommands.h"


using namespace Gui::TaskView;

TaskSketcherCreateCommands::TaskSketcherCreateCommands(QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Appearance"), true, parent)
{
    Gui::Selection().Attach(this);
}

TaskSketcherCreateCommands::~TaskSketcherCreateCommands()
{
    // delete ui;
    Gui::Selection().Detach(this);
}

void TaskSketcherCreateCommands::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
}

/// @cond DOXERR
void TaskSketcherCreateCommands::OnChange(
    Gui::SelectionSingleton::SubjectType& rCaller,
    Gui::SelectionSingleton::MessageType Reason
)
{
    Q_UNUSED(rCaller);
    if (Reason.Type == SelectionChanges::AddSelection || Reason.Type == SelectionChanges::RmvSelection
        || Reason.Type == SelectionChanges::SetSelection
        || Reason.Type == SelectionChanges::ClrSelection) {
    }
}
/// @endcond DOXERR


#include "moc_TaskSketcherCreateCommands.cpp"
