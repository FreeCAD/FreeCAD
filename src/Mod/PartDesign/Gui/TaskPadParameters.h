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

#ifndef GUI_TASKVIEW_TaskPadParameters_H
#define GUI_TASKVIEW_TaskPadParameters_H

#include "TaskExtrudeParameters.h"
#include "ViewProviderPad.h"


namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {


class TaskPadParameters : public TaskExtrudeParameters
{
    Q_OBJECT

public:
    explicit TaskPadParameters(ViewProviderPad *PadView, QWidget *parent = nullptr, bool newObj=false);
    ~TaskPadParameters() override;

    void apply() override;

private:
    void onModeChanged(int index) override;
    void translateModeList(int index) override;
    void updateUI(int index) override;
};

/// simulation dialog for the TaskView
class TaskDlgPadParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgPadParameters(ViewProviderPad *PadView, bool newObj=false);

    ViewProviderPad* getPadView() const
    { return static_cast<ViewProviderPad*>(vp); }
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
