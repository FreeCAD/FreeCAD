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

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPad.h"

class Ui_TaskPadParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 


class TaskPadParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPadParameters(ViewProviderPad *PadView,bool newObj=false,QWidget *parent = 0);
    ~TaskPadParameters();

    double getOffset(void) const;
    void saveHistory(void);
    void apply();

private Q_SLOTS:
    void onLengthChanged(double);
    void onMidplane(bool);
    void onReversed(bool);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onModeChanged(int);
    void onButtonFace(const bool pressed = true);
    void onFaceName(const QString& text);

protected:
    void changeEvent(QEvent *e);

private:
    int getMode(void) const;
    double getLength(void) const;
    double getLength2(void) const;
    bool   getReversed(void) const;
    bool   getMidplane(void) const;
    QString getFaceName(void) const;
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI(int index);

private:
    QWidget* proxy;
    Ui_TaskPadParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgPadParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgPadParameters(ViewProviderPad *PadView,bool newObj=false);
    ~TaskDlgPadParameters();

    ViewProviderPad* getPadView() const
    { return static_cast<ViewProviderPad*>(vp); }


public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)

protected:
    TaskPadParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
