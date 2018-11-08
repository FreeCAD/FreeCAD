/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef GUI_TASKVIEW_TaskAssemblyConstraints_H
#define GUI_TASKVIEW_TaskAssemblyConstraints_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include "ViewProviderConstraint.h"
#include <opendcm/core.hpp>
#include <Solver/Solver.h>
#include "ui_TaskAssemblyConstraints.h"

namespace App {
class Property;
}

namespace AssemblyGui {
  
class TaskAssemblyConstraints : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskAssemblyConstraints(ViewProviderConstraint* vp);
    ~TaskAssemblyConstraints();

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    
public Q_SLOTS:
    void on_constraint_selection(bool clicked);
    void on_value_change(double val);
    void on_orientation_selection(bool clicked);
    void on_solutionspace_selection(bool clicked);
    void on_clear_first();
    void on_clear_second();

private:
    QWidget* proxy;
    Ui::TaskAssemblyConstraints* ui;
    ViewProviderConstraint* view;
    
    void setOrientation(dcm::Direction);
    dcm::Direction getOrientation();
    void setSolutionSpace(dcm::SolutionSpace d);
    dcm::SolutionSpace getSolutionSpace();
    void setPossibleConstraints();
    void setPossibleOptions();
    bool isCombination(boost::shared_ptr<Geometry3D> g1, boost::shared_ptr<Geometry3D> g2, dcm::geometry::types t1, dcm::geometry::types t2);
};

} //namespace AssemblyGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
