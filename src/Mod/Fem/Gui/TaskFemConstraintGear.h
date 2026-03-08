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

#pragma once

#include "TaskFemConstraintBearing.h"
#include "ViewProviderFemConstraintGear.h"

namespace FemGui
{

class TaskFemConstraintGear: public TaskFemConstraintBearing
{
    Q_OBJECT

public:
    explicit TaskFemConstraintGear(
        ViewProviderFemConstraint* ConstraintView,
        QWidget* parent = nullptr,
        const char* pixmapname = "FEM_ConstraintGear"
    );

    double getDiameter() const;
    double getForce() const;
    double getForceAngle() const;
    const std::string getDirectionName() const;
    const std::string getDirectionObject() const;
    bool getReverse() const;

private Q_SLOTS:
    void onDiameterChanged(double dia);
    void onForceChanged(double force);
    void onForceAngleChanged(double angle);
    void onButtonDirection(const bool pressed = true);
    void onCheckReversed(bool);

protected:
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintGear: public TaskDlgFemConstraintBearing
{
    Q_OBJECT

public:
    TaskDlgFemConstraintGear() = default;
    explicit TaskDlgFemConstraintGear(ViewProviderFemConstraintGear* ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace FemGui
