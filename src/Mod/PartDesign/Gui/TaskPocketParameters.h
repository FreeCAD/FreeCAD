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

#ifndef GUI_TASKVIEW_TaskPocketParameters_H
#define GUI_TASKVIEW_TaskPocketParameters_H

#include "TaskExtrudeParameters.h"
#include "ViewProviderPocket.h"


namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {


class TaskPocketParameters : public TaskExtrudeParameters
{
    Q_OBJECT

public:
    explicit TaskPocketParameters(ViewProviderPocket *PocketView, QWidget *parent = nullptr, bool newObj=false);
    ~TaskPocketParameters() override;

    void apply() override;

private:
    void onModeChanged(int index) override;
    void translateModeList(int index) override;
    void updateUI(int index) override;

private:
    double oldLength;
};

/// simulation dialog for the TaskView
class TaskDlgPocketParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgPocketParameters(ViewProviderPocket *PocketView);

    ViewProviderPocket* getPocketView() const
    { return static_cast<ViewProviderPocket*>(vp); }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
