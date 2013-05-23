/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef GUI_TASKVIEW_TaskGrooveParameters_H
#define GUI_TASKVIEW_TaskGrooveParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderGroove.h"
#include "TaskSketchBasedParameters.h"

class Ui_TaskGrooveParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskGrooveParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskGrooveParameters(ViewProviderGroove *GrooveView,QWidget *parent = 0);
    ~TaskGrooveParameters();

    void getReferenceAxis(App::DocumentObject *&obj, std::vector<std::string> &sub) const;
    double  getAngle(void) const;
    bool   getMidplane(void) const;
    bool   getReversed(void) const;

private Q_SLOTS:
    void onAngleChanged(double);
    void onAxisChanged(int);
    void onMidplane(bool);
    void onReversed(bool);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void changeEvent(QEvent *e);

private:
    void updateUI();

private:
    QWidget* proxy;
    Ui_TaskGrooveParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgGrooveParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgGrooveParameters(ViewProviderGroove *GrooveView);
    ~TaskDlgGrooveParameters();

    ViewProviderGroove* getGrooveView() const
    { return static_cast<ViewProviderGroove*>(vp); }

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

protected:
    TaskGrooveParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
