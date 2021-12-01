/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_TASKVIEW_TaskFemConstraintBearing_H
#define GUI_TASKVIEW_TaskFemConstraintBearing_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintBearing.h"

#include <QKeyEvent>

class Ui_TaskFemConstraintBearing;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace FemGui {

class TaskFemConstraintBearing : public TaskFemConstraint
{
    Q_OBJECT

public:
    TaskFemConstraintBearing(ViewProviderFemConstraint *ConstraintView, QWidget *parent = 0,
                             const char* pixmapname = "FEM_ConstraintBearing");
    virtual ~TaskFemConstraintBearing();

    double getDistance(void) const;
    virtual const std::string getReferences() const;
    const std::string getLocationName(void) const;
    const std::string getLocationObject(void) const;
    bool getAxial(void) const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void onDistanceChanged(double l);
    void onButtonLocation(const bool pressed = true);
    void onCheckAxial(bool);

protected:
    bool event(QEvent *e);
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

protected:
    Ui_TaskFemConstraintBearing* ui;

};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintBearing : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintBearing() {}
    TaskDlgFemConstraintBearing(ViewProviderFemConstraintBearing *ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintBearing_H
