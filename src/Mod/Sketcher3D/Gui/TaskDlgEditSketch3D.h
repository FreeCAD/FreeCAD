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

#include <Gui/TaskView/TaskDialog.h>


namespace Sketcher3DGui
{

class ViewProviderSketch3D;
class TaskSketcher3DMessages;
class TaskSketcher3DTool;
class TaskSketcher3DConstraints;
class TaskSketcher3DElements;

class TaskDlgEditSketch3D: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgEditSketch3D(ViewProviderSketch3D* sketchView);
    ~TaskDlgEditSketch3D() override;

    ViewProviderSketch3D* getSketchView() const
    {
        return sketchView;
    }

    TaskSketcher3DTool* getToolPanel() const
    {
        return toolPanel;
    }

    void open() override;
    bool accept() override;
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }
    void autoClosedOnClosedView() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override;

private:
    ViewProviderSketch3D* sketchView {nullptr};
    TaskSketcher3DMessages* messagesPanel {nullptr};
    TaskSketcher3DTool* toolPanel {nullptr};
    TaskSketcher3DConstraints* constraintsPanel {nullptr};
    TaskSketcher3DElements* elementsPanel {nullptr};
};

}  // namespace Sketcher3DGui
