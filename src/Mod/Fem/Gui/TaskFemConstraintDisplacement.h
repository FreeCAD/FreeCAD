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

#pragma once

#include <QObject>
#include <memory>

#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskView.h>

#include "TaskFemConstraint.h"
#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintDisplacement.h"


class Ui_TaskFemConstraintDisplacement;

namespace FemGui
{
class TaskFemConstraintDisplacement: public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    explicit TaskFemConstraintDisplacement(
        ViewProviderFemConstraintDisplacement* ConstraintView,
        QWidget* parent = nullptr
    );
    ~TaskFemConstraintDisplacement() override;

    const std::string getReferences() const override;
    std::string get_spinxDisplacement() const;
    std::string get_spinyDisplacement() const;
    std::string get_spinzDisplacement() const;
    std::string get_spinxRotation() const;
    std::string get_spinyRotation() const;
    std::string get_spinzRotation() const;
    std::string get_xFormula() const;
    std::string get_yFormula() const;
    std::string get_zFormula() const;
    bool get_dispxfree() const;
    bool get_hasDispXFormula() const;
    bool get_dispyfree() const;
    bool get_hasDispYFormula() const;
    bool get_dispzfree() const;
    bool get_hasDispZFormula() const;
    bool get_rotxfree() const;
    bool get_rotyfree() const;
    bool get_rotzfree() const;
    bool get_useFlowSurfaceForce() const;

private Q_SLOTS:
    void onReferenceDeleted();
    void formulaX(bool);
    void formulaY(bool);
    void formulaZ(bool);
    void flowForce(bool);
    void formulaRotx(bool);
    void formulaRoty(bool);
    void formulaRotz(bool);

    void addToSelection() override;
    void removeFromSelection() override;

protected:
    void changeEvent(QEvent* e) override;
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    void updateUI();
    std::unique_ptr<Ui_TaskFemConstraintDisplacement> ui;
};

class TaskDlgFemConstraintDisplacement: public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    explicit TaskDlgFemConstraintDisplacement(ViewProviderFemConstraintDisplacement* ConstraintView);
    bool accept() override;
};

}  // namespace FemGui
