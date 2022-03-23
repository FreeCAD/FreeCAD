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


#ifndef GUI_TASKVIEW_TaskFemConstraintDisplacement_H
#define GUI_TASKVIEW_TaskFemConstraintDisplacement_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Base/Quantity.h>

#include "TaskFemConstraint.h"
#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintDisplacement.h"

#include <QObject>
#include <Base/Console.h>
#include <App/DocumentObject.h>
#include <QKeyEvent>

class Ui_TaskFemConstraintDisplacement;

namespace FemGui {
class TaskFemConstraintDisplacement : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView, QWidget *parent = nullptr);
    ~TaskFemConstraintDisplacement();
    const std::string getReferences() const;
    double get_spinxDisplacement()const;
    double get_spinyDisplacement()const;
    double get_spinzDisplacement()const;
    double get_spinxRotation()const;
    double get_spinyRotation()const;
    double get_spinzRotation()const;
    bool get_dispxfix()const;
    bool get_dispxfree()const;
    bool get_dispyfix()const;
    bool get_dispyfree()const;
    bool get_dispzfix()const;
    bool get_dispzfree()const;
    bool get_rotxfix()const;
    bool get_rotxfree()const;
    bool get_rotyfix()const;
    bool get_rotyfree()const;
    bool get_rotzfix()const;
    bool get_rotzfree()const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void x_changed(double);
    void y_changed(double);
    void z_changed(double);
    void x_rot(double);
    void y_rot(double);
    void z_rot(double);
    void fixx(int);
    void freex(int);
    void fixy(int);
    void freey(int);
    void fixz(int);
    void freez(int);
    void rotfixx(int);
    void rotfreex(int);
    void rotfixy(int);
    void rotfreey(int);
    void rotfixz(int);
    void rotfreez(int);

    void addToSelection();
    void removeFromSelection();

protected:
    bool event(QEvent *e);
    void changeEvent(QEvent *e);
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateUI();
    Ui_TaskFemConstraintDisplacement* ui;

};

class TaskDlgFemConstraintDisplacement : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView);
    void open();
    bool accept();
    bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintDisplacement_H
