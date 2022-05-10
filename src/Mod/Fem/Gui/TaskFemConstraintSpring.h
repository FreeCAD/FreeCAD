/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Preslav Aleksandrov <preslav.aleksandrov@protonmail.com>     *
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

#ifndef GUI_TASKVIEW_TaskFemConstraintSpring_H
#define GUI_TASKVIEW_TaskFemConstraintSpring_H

#include <QObject>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintSpring.h"


class Ui_TaskFemConstraintSpring;

namespace FemGui {
class TaskFemConstraintSpring : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintSpring(ViewProviderFemConstraintSpring *ConstraintView,QWidget *parent = nullptr);
    ~TaskFemConstraintSpring();
    const std::string getReferences() const;
    double get_normalStiffness()const;
    double get_tangentialStiffness()const;

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
    Ui_TaskFemConstraintSpring* ui;

};

class TaskDlgFemConstraintSpring : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintSpring(ViewProviderFemConstraintSpring *ConstraintView);
    void open();
    bool accept();
    bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintSpring_H
