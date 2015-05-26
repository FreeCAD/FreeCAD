/***************************************************************************
 *   Copyright (c) 2015 Przemo Firszt <przemo@firszt.eu>                   *
 *   Based on Force constraint                                             *
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

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Base/Quantity.h>

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintPressure.h"

class Ui_TaskFemConstraintPressure;

namespace FemGui {
class TaskFemConstraintPressure : public TaskFemConstraint
{
    Q_OBJECT

public:
    TaskFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView,QWidget *parent = 0);
    virtual ~TaskFemConstraintPressure();
    double getPressure(void) const;
    virtual const std::string getReferences() const;
    bool getReverse(void) const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void onPressureChanged(const Base::Quantity & f);
    void onCheckReverse(bool);

protected:
    virtual void changeEvent(QEvent *e);

private:
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();
    Ui_TaskFemConstraintPressure* ui;
};

class TaskDlgFemConstraintPressure : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintPressure(ViewProviderFemConstraintPressure *ConstraintView);
    virtual void open();
    virtual bool accept();
    virtual bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintPressure_H
