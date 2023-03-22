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

#include <QObject>

#include <Gui/Selection.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskFemConstraint.h"
#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintDisplacement.h"


class Ui_TaskFemConstraintDisplacement;

namespace FemGui {
class TaskFemConstraintDisplacement : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    explicit TaskFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView, QWidget *parent = nullptr);
    ~TaskFemConstraintDisplacement() override;
    const std::string getReferences() const override;
    double get_spinxDisplacement()const;
    double get_spinyDisplacement()const;
    double get_spinzDisplacement()const;
    double get_spinxRotation()const;
    double get_spinyRotation()const;
    double get_spinzRotation()const;
    std::string get_xFormula() const;
    std::string get_yFormula() const;
    std::string get_zFormula() const;
    bool get_dispxfix()const;
    bool get_dispxfree()const;
    bool get_hasDispXFormula() const;
    bool get_dispyfix()const;
    bool get_dispyfree()const;
    bool get_hasDispYFormula() const;
    bool get_dispzfix()const;
    bool get_dispzfree()const;
    bool get_hasDispZFormula() const;
    bool get_rotxfix()const;
    bool get_rotxfree()const;
    bool get_rotyfix()const;
    bool get_rotyfree()const;
    bool get_rotzfix()const;
    bool get_rotzfree()const;
    bool get_useFlowSurfaceForce() const;

private Q_SLOTS:
    void onReferenceDeleted();
    void fixx(bool);
    void formulaX(bool);
    void fixy(bool);
    void formulaY(bool);
    void fixz(bool);
    void formulaZ(bool);
    void flowForce(bool);
    void rotfixx(bool);
    void formulaRotx(bool);
    void rotfixy(bool);
    void formulaRoty(bool);
    void rotfixz(bool);
    void formulaRotz(bool);

    void addToSelection() override;
    void removeFromSelection() override;

protected:
    bool event(QEvent *e) override;
    void changeEvent(QEvent *e) override;
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateUI();
    Ui_TaskFemConstraintDisplacement* ui;

};

class TaskDlgFemConstraintDisplacement : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    explicit TaskDlgFemConstraintDisplacement(ViewProviderFemConstraintDisplacement *ConstraintView);
    void open() override;
    bool accept() override;
    bool reject() override;
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintDisplacement_H
