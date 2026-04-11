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

#include <QObject>
#include <memory>

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintBearing.h"


class Ui_TaskFemConstraintBearing;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
}

namespace FemGui
{

class TaskFemConstraintBearing: public TaskFemConstraint
{
    Q_OBJECT

public:
    explicit TaskFemConstraintBearing(
        ViewProviderFemConstraint* ConstraintView,
        QWidget* parent = nullptr,
        const char* pixmapname = "FEM_ConstraintBearing"
    );
    ~TaskFemConstraintBearing() override;

    double getDistance() const;
    const std::string getReferences() const override;
    const std::string getLocationName() const;
    const std::string getLocationObject() const;
    bool getAxial() const;

private Q_SLOTS:
    void onReferenceDeleted();
    void onDistanceChanged(double l);
    void onButtonLocation(const bool pressed = true);
    void onCheckAxial(bool);

protected:
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

protected:
    std::unique_ptr<Ui_TaskFemConstraintBearing> ui;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintBearing: public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintBearing() = default;
    explicit TaskDlgFemConstraintBearing(ViewProviderFemConstraintBearing* ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace FemGui
