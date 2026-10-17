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

#include <Gui/TaskView/TaskView.h>

#include "Sketcher3DToolWidget.h"

namespace Sketcher3DGui
{

/// show the active tool's parameter.
class TaskSketcher3DTool: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskSketcher3DTool();
    ~TaskSketcher3DTool() override;

    void setToolWidget(std::unique_ptr<Sketcher3DToolWidget> widget);
    void clearToolWidget();
    Sketcher3DToolWidget* toolWidget() const
    {
        return widget.get();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    std::unique_ptr<Sketcher3DToolWidget> widget;
};

}  // namespace Sketcher3DGui
