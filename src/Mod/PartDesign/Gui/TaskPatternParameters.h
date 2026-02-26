// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include "TaskTransformedParameters.h"
#include "ViewProviderTransformed.h"


class QTimer;
class Ui_TaskPatternParameters;

namespace PartGui
{
class PatternParametersWidget;
}

namespace PartDesignGui
{

class TaskMultiTransformParameters;

class TaskPatternParameters: public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    explicit TaskPatternParameters(ViewProviderTransformed* TransformedView, QWidget* parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskPatternParameters(TaskMultiTransformParameters* parentTask, QWidget* parameterWidget);
    ~TaskPatternParameters() override;

    void apply() override;

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private Q_SLOTS:
    void onUpdateViewTimer();
    // Slot to handle reference selection request from the widget
    void onParameterWidgetRequestReferenceSelection();
    void onParameterWidgetRequestReferenceSelection2();
    // Slot to handle parameter changes from the widget
    void onParameterWidgetParametersChanged();
    // Update view signal (might be redundant now)
    void onUpdateView(bool on) override;


private:
    void setupParameterUI(QWidget* widget) override;
    void retranslateParameterUI(QWidget* widget) override;

    void updateUI();
    void kickUpdateViewTimer() const;

    void bindProperties();

    // Task-specific logic remains
    void showOriginAxes(bool show);
    void enterReferenceSelectionMode();
    void exitReferenceSelectionMode();  // Ensure this clears gates etc.

    PartGui::PatternParametersWidget* parametersWidget = nullptr;
    PartGui::PatternParametersWidget* parametersWidget2 = nullptr;

    PartGui::PatternParametersWidget* activeDirectionWidget = nullptr;

    std::unique_ptr<Ui_TaskPatternParameters> ui;
    QTimer* updateViewTimer = nullptr;
};


/// simulation dialog for the TaskView
class TaskDlgLinearPatternParameters: public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgLinearPatternParameters(ViewProviderTransformed* LinearPatternView);
};

}  // namespace PartDesignGui
