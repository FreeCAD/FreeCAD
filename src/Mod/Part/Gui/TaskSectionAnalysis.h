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

#include <Base/Vector3D.h>

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

class QComboBox;
class QCheckBox;
class QLabel;

namespace Gui
{
class QuantitySpinBox;
class ColorButton;
class GizmoContainer;
class LinearGizmo;
class RotationGizmo;
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
    /// Entries of the preset combo, in order. The index is used to pick the base
    /// normal and to label the two angle boxes, so it is worth naming rather
    /// than reading `case 2:` and counting rows in the constructor. A plane
    /// matching no entry leaves the combo blank, i.e. currentIndex() == -1.
    enum class Preset
    {
        XY = 0,  //!< Z normal
        XZ = 1,  //!< Y normal
        YZ = 2,  //!< X normal
        ViewDirection = 3,
    };

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

private:
    void setupUi();
    void setupConnections();

    /// Build the drag handles, bound to the spin boxes they edit. This is the
    /// same arrangement PartDesign uses for pull-plus-tilt features: the gizmos
    /// drive the spin boxes and the spin boxes drive the feature, so there is
    /// one path into the plane rather than two.
    void setupGizmos();

    /// Tell the user the fine-drag modifier exists.
    ///
    /// The handles snap coarsely by default, which reads as "the resolution is
    /// rough" unless something says otherwise. Every other gizmo-driven task
    /// panel shows this hint, so ours has to as well.
    void showDraggerHints();
    void hideDraggerHints();

    /// Re-place the handles after the plane has moved.
    void setGizmoPositions();

    /// Base orientation and the two axes the angle boxes turn about.
    ///
    /// The single source for both what the angles mean (applyAngles rotates the
    /// base normal about these) and where the tilt handles go (setGizmoPositions
    /// uses them as the arcs' rotation axes). A handle turning about a different
    /// axis than the box it drives is worse than no handle, so both must read the
    /// same frame.
    void angleReferenceFrame(
        Base::Vector3d& baseNormal,
        Base::Vector3d& angle1Axis,
        Base::Vector3d& angle2Axis
    ) const;

    /// Name the two tilt boxes after the axes the given preset turns about.
    void applyPresetAngleLabels(Preset preset);

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

    /// Orientation the tilt angles are measured from, for the presets that have
    /// no fixed axis.
    Base::Vector3d angleBaseNormal {0.0, 0.0, 1.0};

    QComboBox* presetCombo = nullptr;
    QLabel* angleLabel1 = nullptr;
    QLabel* angleLabel2 = nullptr;
    Gui::QuantitySpinBox* offsetSpin = nullptr;
    Gui::QuantitySpinBox* angle1Spin = nullptr;
    Gui::QuantitySpinBox* angle2Spin = nullptr;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* offsetGizmo = nullptr;
    /// True while any handle is being dragged. Re-placing a gizmo mid drag moves
    /// the frame it is projecting the mouse into, so its next reading jumps -
    /// which feeds back through the spin box and runs away.
    ///
    /// Asked of the draggers rather than mirrored in a flag of our own: Coin has
    /// no abort callback, only a finish on mouse release, so a mirrored flag
    /// latches for good if a release is ever missed. This cannot go stale.
    bool anyGizmoDragging() const;
    Gui::RotationGizmo* tiltGizmo1 = nullptr;
    Gui::RotationGizmo* tiltGizmo2 = nullptr;
    QCheckBox* flipCheck = nullptr;
    Gui::ColorButton* sectionColorBtn = nullptr;
    QCheckBox* hatchCheck = nullptr;
    QCheckBox* autoHideHatchCheck = nullptr;
    QCheckBox* perSolidColorCheck = nullptr;
    QCheckBox* ghostCheck = nullptr;
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

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    SectionAnalysisWidget* widget;
};

}  // namespace PartGui
