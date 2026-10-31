// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#pragma once

#include <fastsignals/signal.h>

#include <Gui/TaskView/TaskView.h>

class QAction;
class QListWidget;
class QListWidgetItem;

namespace Sketcher3DGui
{

class ViewProviderSketch3D;

class TaskSketcher3DConstraints: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskSketcher3DConstraints(ViewProviderSketch3D* sketchView);
    ~TaskSketcher3DConstraints() override;

    void refresh();

private Q_SLOTS:
    void onDeleteTriggered();

private:
    ViewProviderSketch3D* sketchView {nullptr};
    QListWidget* constraintsList {nullptr};
    QAction* deleteAction {nullptr};
    fastsignals::connection connectionConstraintsChanged;
};

}  // namespace Sketcher3DGui
