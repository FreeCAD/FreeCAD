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

#ifndef GUI_TASKVIEW_TaskFemConstraintPressure_H
#define GUI_TASKVIEW_TaskFemConstraintPressure_H

#include <QObject>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintPressure.h"


class Ui_TaskFemConstraintPressure;

namespace FemGui {
class TaskFemConstraintPressure : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView,QWidget *parent = nullptr);
    ~TaskFemConstraintPressure();
    const std::string getReferences() const;
    double get_Pressure()const;
    bool get_Reverse()const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void onCheckReverse(bool);
    void addToSelection();
    void removeFromSelection();

protected:
    bool event(QEvent *e);
    void changeEvent(QEvent *e);
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateUI();
    Ui_TaskFemConstraintPressure* ui;

};

class TaskDlgFemConstraintPressure : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView);
    void open();
    bool accept();
    bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintPressure_H
