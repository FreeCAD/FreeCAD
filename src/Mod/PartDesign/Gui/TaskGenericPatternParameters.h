/***************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
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

#ifndef GUI_TASKVIEW_TaskGenericPatternParameters_H
#define GUI_TASKVIEW_TaskGenericPatternParameters_H

#include <App/DocumentObject.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskTransformedParameters.h"
#include "ViewProviderGenericPattern.h"

class QTimer;
class Ui_TaskGenericPatternParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskGenericPatternParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskGenericPatternParameters(ViewProviderTransformed *TransformedView, QWidget *parent = 0);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskGenericPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);
    virtual ~TaskGenericPatternParameters();

    virtual void apply();

private Q_SLOTS:
    void onChangedExpression();
    virtual void onUpdateView(bool);

protected:
    virtual void changeEvent(QEvent *e);

private:
    void setupUI();
    void updateUI();

private:
    Ui_TaskGenericPatternParameters* ui;
    boost::shared_ptr<App::Expression> expr;
};


/// simulation dialog for the TaskView
class TaskDlgGenericPatternParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    TaskDlgGenericPatternParameters(ViewProviderGenericPattern *GenericPatternView);
    virtual ~TaskDlgGenericPatternParameters() {}
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
