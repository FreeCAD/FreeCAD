/***************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale <dahale.a.p@gmail.com>                         *
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

#ifndef GUI_TASKVIEW_TaskFemConstraintOnBoundary_H
#define GUI_TASKVIEW_TaskFemConstraintOnBoundary_H

#include <QObject>

#include <Gui/Selection.h>
#include <Gui/Widgets.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Fem/FemGlobal.h>

#include "TaskFemConstraint.h"


namespace FemGui {

/** @brief Taskbox for FEM constraints that apply on subsets of the domain boundary
 *
 *  @detail Convenience superclass for taskboxes setting certain constraints
 *  that apply on subsets of the boundary (faces/edges/vertices), where one or
 *  more boundary entities need to be selected.
 */
class TaskFemConstraintOnBoundary : public TaskFemConstraint
{
    Q_OBJECT

public:
    TaskFemConstraintOnBoundary(ViewProviderFemConstraint *ConstraintView, QWidget *parent = nullptr, const char* pixmapname = "");
    ~TaskFemConstraintOnBoundary();

protected Q_SLOTS:
    void onButtonToggled(QAbstractButton *button, bool checked);
    virtual void addToSelection() = 0;
    virtual void removeFromSelection() = 0;

protected:
    enum class SelectionChangeModes {none, refAdd, refRemove};
    virtual void onSelectionChanged(const Gui::SelectionChanges&) override;
    virtual void clearButtons(const SelectionChangeModes notThis) = 0;

protected:
    enum SelectionChangeModes selChangeMode;
    Gui::ButtonGroup *buttonGroup;
};

} // namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintOnBoundary_H
