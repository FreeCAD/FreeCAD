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


#ifndef GUI_TASKVIEW_TaskRevolutionParameters_H
#define GUI_TASKVIEW_TaskRevolutionParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderRevolution.h"

class Ui_TaskRevolutionParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskRevolutionParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskRevolutionParameters(ViewProviderRevolution *RevolutionView,QWidget *parent = 0);
    ~TaskRevolutionParameters();

    void apply();

private Q_SLOTS:
    void onAngleChanged(double);
    void onAxisChanged(int);
    void onMidplane(bool);
    void onReversed(bool);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) {}
    void changeEvent(QEvent *e);
    const bool updateView() const;
    QString getReferenceAxis(void) const;
    double getAngle(void) const;
    bool getMidplane(void) const;
    bool getReversed(void) const;

private:
    QWidget* proxy;
    Ui_TaskRevolutionParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgRevolutionParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgRevolutionParameters(ViewProviderRevolution *RevolutionView);
    ~TaskDlgRevolutionParameters();

    ViewProviderRevolution* getRevolutionView() const
    { return static_cast<ViewProviderRevolution*>(vp); }

public:    
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

protected:
    TaskRevolutionParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
