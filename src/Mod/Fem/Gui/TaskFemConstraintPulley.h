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

#ifndef GUI_TASKVIEW_TaskFemConstraintPulley_H
#define GUI_TASKVIEW_TaskFemConstraintPulley_H

#include "TaskFemConstraintGear.h"
#include "ViewProviderFemConstraintPulley.h"

namespace FemGui
{

class TaskFemConstraintPulley: public TaskFemConstraintGear
{
    Q_OBJECT

public:
    explicit TaskFemConstraintPulley(ViewProviderFemConstraintPulley* ConstraintView,
                                     QWidget* parent = nullptr);

    double getOtherDiameter() const;
    double getCenterDistance() const;
    double getTensionForce() const;
    double getTorque() const;
    bool getIsDriven() const;

private Q_SLOTS:
    void onOtherDiameterChanged(double dia);
    void onCenterDistanceChanged(double dia);
    void onTensionForceChanged(double force);
    void onCheckIsDriven(bool);

protected:
    void changeEvent(QEvent* e) override;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintPulley: public TaskDlgFemConstraintGear
{
    Q_OBJECT

public:
    explicit TaskDlgFemConstraintPulley(ViewProviderFemConstraintPulley* ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    void open() override;
};

}  // namespace FemGui

#endif  // GUI_TASKVIEW_TaskFemConstraintPulley_H
