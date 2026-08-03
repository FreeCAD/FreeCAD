// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "TaskExtrudeParameters.h"
#include "ViewProviderPad.h"

class QComboBox;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
}

namespace PartDesignGui
{


class TaskPadParameters: public TaskExtrudeParameters
{
    Q_OBJECT

public:
    explicit TaskPadParameters(ViewProviderPad* PadView, QWidget* parent = nullptr, bool newObj = false);
    ~TaskPadParameters() override;

    void apply() override;

private:
    void onModeChanged(int index, Side side) override;
    void translateModeList(QComboBox* box, int index) override;
    void updateUI(Side side) override;
};

/// simulation dialog for the TaskView
class TaskDlgPadParameters: public TaskDlgExtrudeParameters
{
    Q_OBJECT

public:
    explicit TaskDlgPadParameters(ViewProviderPad* PadView, bool newObj = false);

protected:
    TaskExtrudeParameters* getTaskParameters() override
    {
        return parameters;
    }

private:
    TaskPadParameters* parameters;
};

}  // namespace PartDesignGui
