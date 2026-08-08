// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include "TaskDressUpParameters.h"
#include "ViewProviderDefeaturing.h"

class Ui_TaskDefeaturingParameters;

namespace PartDesignGui
{

class TaskDefeaturingParameters: public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDefeaturingParameters(ViewProviderDressUp* DressUpView, QWidget* parent = nullptr);
    ~TaskDefeaturingParameters() override;

    void apply() override;

private Q_SLOTS:
    void onRefDeleted() override;

protected:
    void setButtons(const selectionModes mode) override;
    void changeEvent(QEvent* e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    std::unique_ptr<Ui_TaskDefeaturingParameters> ui;
};

class TaskDlgDefeaturingParameters: public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgDefeaturingParameters(ViewProviderDefeaturing* DressUpView);
    ~TaskDlgDefeaturingParameters() override;

    bool accept() override;
};

}  // namespace PartDesignGui
