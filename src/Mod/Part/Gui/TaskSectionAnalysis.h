// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
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

#include <memory>

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

class QComboBox;
class QCheckBox;
class QLabel;

namespace Gui
{
class QuantitySpinBox;
class ColorButton;
}  // namespace Gui

namespace Part
{
class SectionAnalysis;
}

namespace PartGui
{

class ViewProviderSectionAnalysis;

class SectionAnalysisWidget: public QWidget
{
    Q_OBJECT

public:
    explicit SectionAnalysisWidget(
        Part::SectionAnalysis* feature,
        ViewProviderSectionAnalysis* vp,
        QWidget* parent = nullptr
    );
    ~SectionAnalysisWidget() override;

    bool accept();
    bool reject();
    Part::SectionAnalysis* getObject() const;
    ViewProviderSectionAnalysis* getViewProvider() const;
    void updateFromFeature();

private:
    void setupUi();
    void setupConnections();
    void onPresetChanged(int index);
    void onAngle1Changed(double val);
    void onAngle2Changed(double val);
    void applyAngles();
    void onFlipToggled(bool on);
    void onSectionColorChanged(const QColor& color);
    void onHatchToggled(bool on);
    void onPerSolidColorToggled(bool on);
    void onShowPlaneToggled(bool on);
    void onUpdateViewToggled(bool on);
    void recompute();

    Part::SectionAnalysis* feature;
    ViewProviderSectionAnalysis* viewProvider;
    QComboBox* presetCombo = nullptr;
    QLabel* angleLabel1 = nullptr;
    QLabel* angleLabel2 = nullptr;
    Gui::QuantitySpinBox* angle1Spin = nullptr;
    Gui::QuantitySpinBox* angle2Spin = nullptr;
    QCheckBox* flipCheck = nullptr;
    Gui::ColorButton* sectionColorBtn = nullptr;
    QCheckBox* hatchCheck = nullptr;
    QCheckBox* perSolidColorCheck = nullptr;
    QCheckBox* showPlaneCheck = nullptr;
    QCheckBox* updateViewCheck = nullptr;
};

class TaskSectionAnalysis: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskSectionAnalysis(Part::SectionAnalysis* feature, ViewProviderSectionAnalysis* vp);
    ~TaskSectionAnalysis() override;

    bool accept() override;
    bool reject() override;
    Part::SectionAnalysis* getObject() const;
    void updateFromFeature();

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    SectionAnalysisWidget* widget;
};

}  // namespace PartGui
