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

#ifndef GUI_TASKVIEW_TaskFemConstraintFixed_H
#define GUI_TASKVIEW_TaskFemConstraintFixed_H

#include <QObject>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintFixed.h"


class Ui_TaskFemConstraintFixed;

namespace FemGui {
class TaskFemConstraintFixed : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView,QWidget *parent = nullptr);
    ~TaskFemConstraintFixed();
    const std::string getReferences() const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void addToSelection();
    void removeFromSelection();

protected:
    bool event(QEvent *e);
    void changeEvent(QEvent *e);
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateUI();
    Ui_TaskFemConstraintFixed* ui;

};

class TaskDlgFemConstraintFixed : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintFixed(ViewProviderFemConstraintFixed *ConstraintView);
    void open();
    bool accept();
    bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintFixed_H
