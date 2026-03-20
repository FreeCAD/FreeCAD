// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
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

#include <memory>

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QSlider;

namespace Gui
{
class QuantitySpinBox;
class ColorButton;
class LinearGizmo;
class RotationGizmo;
class GizmoContainer;
}

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
    explicit SectionAnalysisWidget(Part::SectionAnalysis* feature,
                                   ViewProviderSectionAnalysis* vp,
                                   QWidget* parent = nullptr);
    ~SectionAnalysisWidget() override;

    bool accept();
    bool reject();
    Part::SectionAnalysis* getObject() const;
    ViewProviderSectionAnalysis* getViewProvider() const;
    void updateFromFeature();
    void setupGizmos();
    void updateGizmoPositions();

private:
    void setupUi();
    void setupConnections();
    void updateSliderRange();
    void onPresetChanged(int index);
    void onNormalXChanged(double val);
    void onNormalYChanged(double val);
    void onNormalZChanged(double val);
    void onAngleXChanged(double val);
    void onAngleZChanged(double val);
    void applyAngles();
    void onOffsetChanged(double val);
    void onSliderMoved(int val);
    void onFlipToggled(bool on);
    void onSectionColorChanged(const QColor& color);
    void onHatchToggled(bool on);
    void onUpdateViewToggled(bool on);
    void recompute();

    Part::SectionAnalysis* feature;
    ViewProviderSectionAnalysis* viewProvider;
    QComboBox* presetCombo = nullptr;
    QDoubleSpinBox* normalX = nullptr;
    QDoubleSpinBox* normalY = nullptr;
    QDoubleSpinBox* normalZ = nullptr;
    Gui::QuantitySpinBox* offsetSpin = nullptr;
    QSlider* offsetSlider = nullptr;
    Gui::QuantitySpinBox* angleXSpin = nullptr;
    Gui::QuantitySpinBox* angleZSpin = nullptr;
    QCheckBox* flipCheck = nullptr;
    Gui::ColorButton* sectionColorBtn = nullptr;
    QCheckBox* hatchCheck = nullptr;
    QCheckBox* updateViewCheck = nullptr;
    double sliderMin = -100.0;
    double sliderMax = 100.0;

    // Gizmos — the GizmoContainer owns the gizmo lifetimes
    Gui::LinearGizmo* offsetGizmo = nullptr;
    Gui::RotationGizmo* angleXGizmo = nullptr;
    Gui::RotationGizmo* angleZGizmo = nullptr;
    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
};

class TaskSectionAnalysis: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskSectionAnalysis(Part::SectionAnalysis* feature,
                                 ViewProviderSectionAnalysis* vp);
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
