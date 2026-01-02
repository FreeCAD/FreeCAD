// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef GUI_TASKVIEW_TaskAssemblyMessages_H
#define GUI_TASKVIEW_TaskAssemblyMessages_H

#include <Gui/TaskView/TaskSolverMessages.h>


namespace AssemblyGui
{

class ViewProviderAssembly;

class TaskAssemblyMessages: public Gui::TaskSolverMessages
{
    Q_OBJECT

public:
    explicit TaskAssemblyMessages(ViewProviderAssembly* vp);
    ~TaskAssemblyMessages() override;

private:
    void onLabelStatusLinkClicked(const QString&) override;

    void updateToolTip(const QString& link) override;

protected:
    ViewProviderAssembly* vp;
};

}  // namespace AssemblyGui

#endif  // GUI_TASKVIEW_TaskAssemblyMessages_H
