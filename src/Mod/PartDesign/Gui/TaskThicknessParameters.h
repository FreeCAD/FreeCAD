// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#pragma once

#include <Gui/Inventor/Draggers/Gizmo.h>

#include "TaskDressUpParameters.h"
#include "ViewProviderThickness.h"

class Ui_TaskThicknessParameters;

namespace Gui
{
class LinearGizmo;
class GizmoContainer;
}  // namespace Gui

namespace PartDesign
{
class Thickness;
}

namespace PartDesignGui
{

class TaskThicknessParameters: public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskThicknessParameters(ViewProviderDressUp* DressUpView, QWidget* parent = nullptr);
    ~TaskThicknessParameters() override;

    void apply() override;

    double getValue() const;
    bool getReversed() const;
    bool getIntersection() const;
    int getMode() const;
    int getJoinType() const;

private Q_SLOTS:
    void onValueChanged(double angle);
    void onModeChanged(int mode);
    void onJoinTypeChanged(int join);
    void onReversedChanged(bool on);
    void onIntersectionChanged(bool on);
    void onRefDeleted() override;

protected:
    void setButtons(const selectionModes mode) override;
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void addContainerWidget();
    void initControls();
    void setupConnections();
    PartDesign::Thickness* onBeforeChange();
    void onAfterChange(PartDesign::Thickness* obj);

private:
    std::unique_ptr<Ui_TaskThicknessParameters> ui;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* linearGizmo = nullptr;
    void setupGizmos(ViewProviderDressUp* vp);
    void setGizmoPositions();
};

/// simulation dialog for the TaskView
class TaskDlgThicknessParameters: public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgThicknessParameters(ViewProviderThickness* ThicknessView);
    ~TaskDlgThicknessParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace PartDesignGui
