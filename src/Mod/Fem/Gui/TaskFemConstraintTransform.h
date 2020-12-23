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

#ifndef GUI_TASKVIEW_TaskFemConstraintTransform_H
#define GUI_TASKVIEW_TaskFemConstraintTransform_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Base/Quantity.h>

#include "TaskFemConstraint.h"
#include "ViewProviderFemConstraintTransform.h"

#include <QObject>
#include <Base/Console.h>
#include <App/DocumentObject.h>
#include <QListWidgetItem>

class Ui_TaskFemConstraintTransform;

namespace FemGui {
class TaskFemConstraintTransform : public TaskFemConstraint
{
    Q_OBJECT

public:
    TaskFemConstraintTransform(ViewProviderFemConstraintTransform *ConstraintView,QWidget *parent = 0);
    ~TaskFemConstraintTransform();
    const std::string getReferences() const;
    double get_X_rot()const;
    double get_Y_rot()const;
    double get_Z_rot()const;
    std::string get_transform_type(void) const;
    static std::string getSurfaceReferences(const std::string showConstr);

private Q_SLOTS:
    void onReferenceDeleted(void);
    void Rect();
    void Cyl();
    void addToSelection();
    void removeFromSelection();
    void setSelection(QListWidgetItem* item);
    void x_Changed(int x);
    void y_Changed(int y);
    void z_Changed(int z);

protected:
    void changeEvent(QEvent *e);
    const QString makeText(const App::DocumentObject* obj) const;

private:
    //void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();
    Ui_TaskFemConstraintTransform* ui;
};

class TaskDlgFemConstraintTransform : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintTransform(ViewProviderFemConstraintTransform *ConstraintView);
    void open();
    bool accept();
    bool reject();
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintTransform_H
