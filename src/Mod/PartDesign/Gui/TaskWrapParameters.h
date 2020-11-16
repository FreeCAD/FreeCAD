/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_TASKVIEW_TaskWrapParameters_H
#define GUI_TASKVIEW_TaskWrapParameters_H

#include <boost/signals2.hpp>

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderWrap.h"
#include "TaskFeatureParameters.h"

class Ui_TaskWrapParameters;

namespace PartDesignGui {

class TaskWrapParameters : public PartDesignGui::TaskFeatureParameters
{
    Q_OBJECT

public:
    TaskWrapParameters(ViewProviderWrap *vp, QWidget *parent = 0);
    ~TaskWrapParameters();

private Q_SLOTS:
    void onTypeChanged(int);
    void onFrozenChanged();

protected:
    void refresh();
    void setupUI();

private:
    QWidget* proxy;
    Ui_TaskWrapParameters* ui;
    boost::signals2::scoped_connection connRecompute;
    bool busy = false;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
