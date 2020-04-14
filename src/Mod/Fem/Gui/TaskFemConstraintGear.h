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


#ifndef GUI_TASKVIEW_TaskFemConstraintGear_H
#define GUI_TASKVIEW_TaskFemConstraintGear_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskFemConstraintBearing.h"
#include "ViewProviderFemConstraintGear.h"

namespace FemGui {

class TaskFemConstraintGear : public TaskFemConstraintBearing
{
    Q_OBJECT

public:
    TaskFemConstraintGear(ViewProviderFemConstraint *ConstraintView,QWidget *parent = 0,
                          const char* pixmapname = "FEM_ConstraintGear");

    double getDiameter(void) const;
    double getForce(void) const;
    double getForceAngle(void) const;
    const std::string getDirectionName(void) const;
    const std::string getDirectionObject(void) const;
    bool getReverse(void) const;

private Q_SLOTS:
    void onDiameterChanged(double dia);
    void onForceChanged(double force);
    void onForceAngleChanged(double angle);
    void onButtonDirection(const bool pressed = true);
    void onCheckReversed(bool);

protected:
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintGear : public TaskDlgFemConstraintBearing
{
    Q_OBJECT

public:
    TaskDlgFemConstraintGear() {}
    TaskDlgFemConstraintGear(ViewProviderFemConstraintGear *ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintGear_H
