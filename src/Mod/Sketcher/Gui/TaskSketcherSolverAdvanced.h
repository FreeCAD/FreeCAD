// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Abdullah Tahiri
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

#include <Gui/TaskView/TaskView.h>


class Ui_TaskSketcherSolverAdvanced;

namespace App
{
class Property;
}

namespace SketcherGui
{

class ViewProviderSketch;

class TaskSketcherSolverAdvanced: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskSketcherSolverAdvanced(ViewProviderSketch* sketchView);
    ~TaskSketcherSolverAdvanced() override;

private:
    void setupConnections();
    void onComboBoxDefaultSolverCurrentIndexChanged(int index);
    void onComboBoxDogLegGaussStepCurrentIndexChanged(int index);
    void onSpinBoxMaxIterValueChanged(int i);
    void onSpinBoxAutoQRAlgoChanged(int i);
    void onCheckBoxAutoQRAlgoStateChanged(int state);
    void onCheckBoxSketchSizeMultiplierStateChanged(int state);
    void onLineEditConvergenceEditingFinished();
    void onComboBoxQRMethodCurrentIndexChanged(int index);
    void onLineEditQRPivotThresholdEditingFinished();
    void onComboBoxRedundantDefaultSolverCurrentIndexChanged(int index);
    void onLineEditRedundantConvergenceEditingFinished();
    void onSpinBoxRedundantSolverMaxIterationsValueChanged(int i);
    void onCheckBoxRedundantSketchSizeMultiplierStateChanged(int state);
    void onComboBoxDebugModeCurrentIndexChanged(int index);
    void onLineEditSolverParam1EditingFinished();
    void onLineEditRedundantSolverParam1EditingFinished();
    void onLineEditSolverParam2EditingFinished();
    void onLineEditRedundantSolverParam2EditingFinished();
    void onLineEditSolverParam3EditingFinished();
    void onLineEditRedundantSolverParam3EditingFinished();
    void onPushButtonDefaultsClicked(bool checked = false);
    void onPushButtonSolveClicked(bool checked = false);

protected:
    void updateDefaultMethodParameters();
    void updateRedundantMethodParameters();
    void updateSketchObject();

protected:
    ViewProviderSketch* sketchView;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskSketcherSolverAdvanced> ui;
};

}  // namespace SketcherGui
