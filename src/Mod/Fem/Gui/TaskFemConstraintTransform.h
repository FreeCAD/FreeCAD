/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *            Ofentse Kgoa <kgoaot@eskom.co.za>                            *
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

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintTransform.h"


class Ui_TaskFemConstraintTransform;

namespace FemGui
{
class TaskFemConstraintTransform: public TaskFemConstraint
{
    Q_OBJECT

public:
    explicit TaskFemConstraintTransform(
        ViewProviderFemConstraintTransform* ConstraintView,
        QWidget* parent = nullptr
    );
    ~TaskFemConstraintTransform() override;
    const std::string getReferences() const override;
    Base::Rotation getRotation() const;
    std::string get_transform_type() const;
    static std::string getSurfaceReferences(const std::string showConstr);

private Q_SLOTS:
    void onReferenceDeleted();
    void Rect();
    void Cyl();
    void addToSelection();
    void removeFromSelection();
    void xAxisChanged(double x);
    void yAxisChanged(double y);
    void zAxisChanged(double z);
    void angleChanged(double a);

protected:
    void changeEvent(QEvent* e) override;
    const QString makeText(const App::DocumentObject* obj) const;

private:
    void updateUI();
    std::unique_ptr<Ui_TaskFemConstraintTransform> ui;
};

class TaskDlgFemConstraintTransform: public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    explicit TaskDlgFemConstraintTransform(ViewProviderFemConstraintTransform* ConstraintView);
    bool accept() override;
};

}  // namespace FemGui
