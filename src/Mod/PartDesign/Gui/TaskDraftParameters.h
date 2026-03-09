// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
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


#pragma once

#include "TaskDressUpParameters.h"
#include "ViewProviderDraft.h"

class Ui_TaskDraftParameters;

namespace Gui
{
class RotationGizmo;
class GizmoContainer;
}  // namespace Gui

namespace PartDesignGui
{

class TaskDraftParameters: public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDraftParameters(ViewProviderDressUp* DressUpView, QWidget* parent = nullptr);
    ~TaskDraftParameters() override;

    void apply() override;

    double getAngle() const;
    bool getReversed() const;
    const std::vector<std::string> getFaces() const;
    void getPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    void getLine(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

private Q_SLOTS:
    void onAngleChanged(double angle);
    void onReversedChanged(bool reversed);
    void onButtonPlane(bool checked);
    void onButtonLine(bool checked);
    void onRefDeleted() override;

protected:
    void setButtons(selectionModes mode) override;
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    std::unique_ptr<Ui_TaskDraftParameters> ui;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::RotationGizmo* angleGizmo = nullptr;
    void setupGizmos(ViewProvider* vp);
    void setGizmoPositions();
};

/// simulation dialog for the TaskView
class TaskDlgDraftParameters: public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgDraftParameters(ViewProviderDraft* DraftView);
    ~TaskDlgDraftParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace PartDesignGui
