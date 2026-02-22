// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/CAM/App/FeaturePathCompound.h>

#include "ViewProviderPathCompound.h"


class Ui_TaskDlgPathCompound;

namespace PathGui
{

/// Widget
class TaskWidgetPathCompound: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskWidgetPathCompound(ViewProviderPathCompound* CompoundView, QWidget* parent = nullptr);
    ~TaskWidgetPathCompound() override;

    std::vector<std::string> getList() const;

protected:
    void changeEvent(QEvent* e) override;

private:
    QWidget* proxy;
    Ui_TaskDlgPathCompound* ui;
};

/// Task Dialog
class PathGuiExport TaskDlgPathCompound: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgPathCompound(PathGui::ViewProviderPathCompound*);
    ~TaskDlgPathCompound() override;

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or rject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user press the help button
    void helpRequested() override;

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

protected:
    PathGui::ViewProviderPathCompound* CompoundView;
    TaskWidgetPathCompound* parameter;
};


}  // namespace PathGui
