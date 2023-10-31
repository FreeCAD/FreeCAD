/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *   Based on Force constraint by Jan Rheinländer                          *
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

#ifndef GUI_TASKVIEW_TaskFemConstraintInitialTemperature_H
#define GUI_TASKVIEW_TaskFemConstraintInitialTemperature_H

#include <QObject>
#include <memory>

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintInitialTemperature.h"


class Ui_TaskFemConstraintInitialTemperature;

namespace FemGui
{
class TaskFemConstraintInitialTemperature: public TaskFemConstraint
{
    Q_OBJECT

public:
    explicit TaskFemConstraintInitialTemperature(
        ViewProviderFemConstraintInitialTemperature* ConstraintView,
        QWidget* parent = nullptr);
    ~TaskFemConstraintInitialTemperature() override;
    std::string get_temperature() const;

protected:
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_TaskFemConstraintInitialTemperature> ui;
};

class TaskDlgFemConstraintInitialTemperature: public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    explicit TaskDlgFemConstraintInitialTemperature(
        ViewProviderFemConstraintInitialTemperature* ConstraintView);
    void open() override;
    bool accept() override;
    bool reject() override;
};

}  // namespace FemGui

#endif  // GUI_TASKVIEW_TaskFemConstraintInitialTemperature_H
