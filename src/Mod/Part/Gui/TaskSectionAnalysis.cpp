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

#include <algorithm>
#include <cmath>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/QuantitySpinBox.h>
#include <Base/Console.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewParams.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Widgets.h>
#include <App/Material.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include <Gui/Inventor/Draggers/SoRotationDragger.h>

#include "TaskSectionAnalysis.h"
#include "ViewProviderSectionAnalysis.h"


using namespace PartGui;

// -----------------------------------------------------------------------
// SectionAnalysisWidget
// -----------------------------------------------------------------------

SectionAnalysisWidget::SectionAnalysisWidget(
    Part::SectionAnalysis* feat,
    ViewProviderSectionAnalysis* vp,
    QWidget* parent
)
    : QWidget(parent)
    , feature(feat)
    , viewProvider(vp)
{
    setupUi();
    setupConnections();
    updateSliderRange();

    // Enable hatching by default
    onHatchToggled(true);
}

SectionAnalysisWidget::~SectionAnalysisWidget() = default;

ViewProviderSectionAnalysis* SectionAnalysisWidget::getViewProvider() const
{
    return viewProvider;
}


Part::SectionAnalysis* SectionAnalysisWidget::getObject() const
{
    return feature;
}

void SectionAnalysisWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    // --- Cutting Plane group ---
    auto* planeGroup = new QGroupBox(tr("Cutting Plane"), this);
    auto* planeLayout = new QGridLayout(planeGroup);

    // Preset combo
    planeLayout->addWidget(new QLabel(tr("Preset:"), this), 0, 0);
    presetCombo = new QComboBox(this);
    presetCombo->addItem(tr("XY Plane (Z normal)"));
    presetCombo->addItem(tr("XZ Plane (Y normal)"));
    presetCombo->addItem(tr("YZ Plane (X normal)"));
    presetCombo->addItem(tr("View Direction"));
    presetCombo->addItem(tr("Custom"));
    planeLayout->addWidget(presetCombo, 0, 1);

    // Detect current preset from normal
    Base::Vector3d n = feature->PlaneNormal.getValue();
    if (std::abs(n.x) < 1e-6 && std::abs(n.y) < 1e-6 && std::abs(std::abs(n.z) - 1.0) < 1e-6) {
        presetCombo->setCurrentIndex(0);
    }
    else if (std::abs(n.x) < 1e-6 && std::abs(std::abs(n.y) - 1.0) < 1e-6 && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(1);
    }
    else if (std::abs(std::abs(n.x) - 1.0) < 1e-6 && std::abs(n.y) < 1e-6 && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(2);
    }
    else {
        presetCombo->setCurrentIndex(4);  // Custom
    }

    // Normal spinboxes (hidden — internal state, angles/presets are the user interface)
    normalX = new QDoubleSpinBox(this);
    normalX->setRange(-1.0, 1.0);
    normalX->setDecimals(4);
    normalX->setValue(n.x);
    normalX->setVisible(false);

    normalY = new QDoubleSpinBox(this);
    normalY->setRange(-1.0, 1.0);
    normalY->setDecimals(4);
    normalY->setValue(n.y);
    normalY->setVisible(false);

    normalZ = new QDoubleSpinBox(this);
    normalZ->setRange(-1.0, 1.0);
    normalZ->setDecimals(4);
    normalZ->setValue(n.z);
    normalZ->setVisible(false);

    // Angle adjustments (tilt the plane from the preset orientation)
    angleLabel1 = new QLabel(tr("X Angle:"), this);
    planeLayout->addWidget(angleLabel1, 1, 0);
    angle1Spin = new Gui::QuantitySpinBox(this);
    angle1Spin->setUnit(Base::Unit::Angle);
    angle1Spin->setRange(-90.0, 90.0);
    angle1Spin->setSingleStep(0.1);
    angle1Spin->setValue(0.0);
    planeLayout->addWidget(angle1Spin, 1, 1);

    angleLabel2 = new QLabel(tr("Z Angle:"), this);
    planeLayout->addWidget(angleLabel2, 2, 0);
    angle2Spin = new Gui::QuantitySpinBox(this);
    angle2Spin->setUnit(Base::Unit::Angle);
    angle2Spin->setRange(-90.0, 90.0);
    angle2Spin->setSingleStep(0.1);
    angle2Spin->setValue(0.0);
    planeLayout->addWidget(angle2Spin, 2, 1);

    // Set angle labels and enable state based on initial preset
    {
        int idx = presetCombo->currentIndex();
        if (idx == 0) {
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Y Angle:"));
        }
        else if (idx == 1) {
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
        }
        else if (idx == 2) {
            angleLabel1->setText(tr("Y Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
        }
        else {
            angleLabel1->setText(tr("Angle 1:"));
            angleLabel2->setText(tr("Angle 2:"));
            angle1Spin->setEnabled(false);
            angle2Spin->setEnabled(false);
        }
    }

    // Offset
    planeLayout->addWidget(new QLabel(tr("Distance:"), this), 3, 0);
    offsetSpin = new Gui::QuantitySpinBox(this);
    offsetSpin->setUnit(Base::Unit::Length);
    offsetSpin->setRange(-1e9, 1e9);
    offsetSpin->setSingleStep(0.01);
    offsetSpin->setValue(feature->PlaneOffset.getValue());
    planeLayout->addWidget(offsetSpin, 3, 1);

    // Offset slider
    offsetSlider = new QSlider(Qt::Horizontal, this);
    offsetSlider->setRange(0, 1000);
    offsetSlider->setValue(500);
    planeLayout->addWidget(offsetSlider, 4, 0, 1, 2);

    // Flip
    flipCheck = new QCheckBox(tr("Flip Direction"), this);
    flipCheck->setChecked(feature->FlipCut.getValue());
    planeLayout->addWidget(flipCheck, 5, 0, 1, 2);

    mainLayout->addWidget(planeGroup);

    // --- Appearance group ---
    auto* appearGroup = new QGroupBox(tr("Appearance"), this);
    auto* appearLayout = new QGridLayout(appearGroup);

    appearLayout->addWidget(new QLabel(tr("Section Color:"), this), 0, 0);
    sectionColorBtn = new Gui::ColorButton(this);
    sectionColorBtn->setColor(QColor(204, 76, 51));  // default reddish-orange
    appearLayout->addWidget(sectionColorBtn, 0, 1);

    hatchCheck = new QCheckBox(tr("Show Hatching"), this);
    hatchCheck->setChecked(viewProvider->ShowHatching.getValue());
    appearLayout->addWidget(hatchCheck, 1, 0, 1, 2);

    perSolidColorCheck = new QCheckBox(tr("Per-Body Colors"), this);
    perSolidColorCheck->setChecked(viewProvider->PerBodyColors.getValue());
    appearLayout->addWidget(perSolidColorCheck, 2, 0, 1, 2);

    showPlaneCheck = new QCheckBox(tr("Show Cutting Plane"), this);
    showPlaneCheck->setChecked(true);
    appearLayout->addWidget(showPlaneCheck, 3, 0, 1, 2);

    mainLayout->addWidget(appearGroup);

    // Update View checkbox
    updateViewCheck = new QCheckBox(tr("Update View"), this);
    updateViewCheck->setChecked(true);
    mainLayout->addWidget(updateViewCheck);

    mainLayout->addStretch();
}

void SectionAnalysisWidget::setupConnections()
{
    connect(
        presetCombo,
        qOverload<int>(&QComboBox::activated),
        this,
        &SectionAnalysisWidget::onPresetChanged
    );
    connect(
        normalX,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onNormalXChanged
    );
    connect(
        normalY,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onNormalYChanged
    );
    connect(
        normalZ,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onNormalZChanged
    );
    connect(
        angle1Spin,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onAngle1Changed
    );
    connect(
        angle2Spin,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onAngle2Changed
    );
    connect(
        offsetSpin,
        qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this,
        &SectionAnalysisWidget::onOffsetChanged
    );
    connect(offsetSlider, &QSlider::valueChanged, this, &SectionAnalysisWidget::onSliderMoved);
    connect(flipCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onFlipToggled);
    connect(sectionColorBtn, &Gui::ColorButton::changed, this, [this]() {
        onSectionColorChanged(sectionColorBtn->color());
    });
    connect(hatchCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onHatchToggled);
    connect(perSolidColorCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onPerSolidColorToggled);
    connect(showPlaneCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onShowPlaneToggled);
    connect(updateViewCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onUpdateViewToggled);
}

void SectionAnalysisWidget::updateSliderRange()
{
    App::DocumentObject* source = feature->Source.getValue();
    if (!source) {
        return;
    }

    TopoDS_Shape sourceShape = Part::Feature::getShape(
        source,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (sourceShape.IsNull()) {
        return;
    }

    Bnd_Box bbox;
    BRepBndLib::Add(sourceShape, bbox);
    if (bbox.IsVoid()) {
        return;
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    // Project bounding box onto the normal direction
    Base::Vector3d n = feature->PlaneNormal.getValue();
    double len = n.Length();
    if (len < 1e-10) {
        return;
    }
    n = n / len;

    // Find min/max projection
    double corners[8][3] = {
        {xmin, ymin, zmin},
        {xmax, ymin, zmin},
        {xmin, ymax, zmin},
        {xmax, ymax, zmin},
        {xmin, ymin, zmax},
        {xmax, ymin, zmax},
        {xmin, ymax, zmax},
        {xmax, ymax, zmax}
    };

    double projMin = 1e20, projMax = -1e20;
    for (auto& corner : corners) {
        double proj = corner[0] * n.x + corner[1] * n.y + corner[2] * n.z;
        projMin = std::min(projMin, proj);
        projMax = std::max(projMax, proj);
    }

    // Shift so the spinbox is always [0, range].  The gizmo only does
    // positive values, so we stash projMin and add it back ourselves.
    offsetBase = projMin;
    sliderMin = 0.0;
    sliderMax = projMax - projMin;

    offsetSpin->setRange(sliderMin, sliderMax);

    // Map current PlaneOffset to shifted spinbox value
    double val = feature->PlaneOffset.getValue() - offsetBase;
    offsetSpin->blockSignals(true);
    offsetSpin->setValue(val);
    offsetSpin->blockSignals(false);

    int sliderPos = 0;
    if (sliderMax > sliderMin) {
        sliderPos = static_cast<int>(val / sliderMax * 1000.0);
        sliderPos = std::clamp(sliderPos, 0, 1000);
    }
    offsetSlider->blockSignals(true);
    offsetSlider->setValue(sliderPos);
    offsetSlider->blockSignals(false);
}

void SectionAnalysisWidget::onPresetChanged(int index)
{
    Base::Vector3d normal;
    switch (index) {
        case 0:
            normal = Base::Vector3d(0, 0, 1);
            break;
        case 1:
            normal = Base::Vector3d(0, 1, 0);
            break;
        case 2:
            normal = Base::Vector3d(1, 0, 0);
            break;
        case 3: {
            // View Direction: get camera direction
            auto* mdiView = qobject_cast<Gui::View3DInventor*>(
                Gui::Application::Instance->activeView()
            );
            if (mdiView) {
                SbVec3f vd = mdiView->getViewer()->getViewDirection();
                float vx, vy, vz;
                vd.getValue(vx, vy, vz);
                normal = Base::Vector3d(-vx, -vy, -vz);  // face toward camera
            }
            else {
                normal = Base::Vector3d(0, 0, 1);
            }
            break;
        }
        default:
            // Custom: no base axis for angle decomposition
            angle1Spin->setEnabled(false);
            angle2Spin->setEnabled(false);
            return;
    }

    // Enable angle spinboxes for axis-aligned presets
    angle1Spin->setEnabled(true);
    angle2Spin->setEnabled(true);

    // Update angle labels for the selected preset
    switch (index) {
        case 0:  // XY (Z normal)
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Y Angle:"));
            break;
        case 1:  // XZ (Y normal)
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
            break;
        case 2:  // YZ (X normal)
            angleLabel1->setText(tr("Y Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
            break;
        default:
            angleLabel1->setText(tr("Angle 1:"));
            angleLabel2->setText(tr("Angle 2:"));
            break;
    }

    // Reset angles when switching presets
    angle1Spin->blockSignals(true);
    angle2Spin->blockSignals(true);
    angle1Spin->setValue(0.0);
    angle2Spin->setValue(0.0);
    angle1Spin->blockSignals(false);
    angle2Spin->blockSignals(false);

    // Auto-flip for presets where default normal faces away from typical viewing
    // View Direction (3) doesn't need flip since we already face toward camera
    bool needFlip = (index == 1);  // XZ plane (Y normal) typically faces away
    flipCheck->blockSignals(true);
    flipCheck->setChecked(needFlip);
    flipCheck->blockSignals(false);
    feature->FlipCut.setValue(needFlip);

    normalX->blockSignals(true);
    normalY->blockSignals(true);
    normalZ->blockSignals(true);
    normalX->setValue(normal.x);
    normalY->setValue(normal.y);
    normalZ->setValue(normal.z);
    normalX->blockSignals(false);
    normalY->blockSignals(false);
    normalZ->blockSignals(false);

    feature->PlaneNormal.setValue(normal);

    // Center the offset on the bounding box
    App::DocumentObject* source = feature->Source.getValue();
    if (source) {
        TopoDS_Shape sourceShape = Part::Feature::getShape(
            source,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        if (!sourceShape.IsNull()) {
            Bnd_Box bbox;
            BRepBndLib::Add(sourceShape, bbox);
            if (!bbox.IsVoid()) {
                double xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                Base::Vector3d center((xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2);
                double centerProj = center.x * normal.x + center.y * normal.y + center.z * normal.z;
                feature->PlaneOffset.setValue(centerProj);
            }
        }
    }

    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onAngle1Changed(double /*val*/)
{
    applyAngles();
}

void SectionAnalysisWidget::onAngle2Changed(double /*val*/)
{
    applyAngles();
}

void SectionAnalysisWidget::applyAngles()
{
    // Get base normal from preset, preserving the sign from the current normal
    // (e.g., if the feature was created with (-1,0,0), keep the negative sign)
    Base::Vector3d curN = feature->PlaneNormal.getValue();
    Base::Vector3d baseNormal;
    int preset = presetCombo->currentIndex();
    switch (preset) {
        case 0: {
            double sign = (curN.z < 0) ? -1.0 : 1.0;
            baseNormal = Base::Vector3d(0, 0, sign);
            break;
        }
        case 1: {
            double sign = (curN.y < 0) ? -1.0 : 1.0;
            baseNormal = Base::Vector3d(0, sign, 0);
            break;
        }
        case 2: {
            double sign = (curN.x < 0) ? -1.0 : 1.0;
            baseNormal = Base::Vector3d(sign, 0, 0);
            break;
        }
        case 3:  // View Direction — use current normal as base
        default:
            baseNormal = Base::Vector3d(normalX->value(), normalY->value(), normalZ->value());
            break;
    }

    // Negate X angle to match the gizmo arc drag direction
    double a1 = -angle1Spin->value().getValue() * M_PI / 180.0;
    double a2 = angle2Spin->value().getValue() * M_PI / 180.0;

    // Build two tangent axes perpendicular to the base normal.
    // We rotate around these tangent axes (not global X/Z).
    Base::Vector3d tangent1, tangent2;
    if (std::abs(baseNormal.z) > 0.9) {
        // Z normal → tangent axes are X and Y
        tangent1 = Base::Vector3d(1, 0, 0);
        tangent2 = Base::Vector3d(0, 1, 0);
    }
    else if (std::abs(baseNormal.y) > 0.9) {
        // Y normal → tangent axes are X and Z
        tangent1 = Base::Vector3d(1, 0, 0);
        tangent2 = Base::Vector3d(0, 0, 1);
    }
    else {
        // X normal → tangent axes are Y and Z
        tangent1 = Base::Vector3d(0, 1, 0);
        tangent2 = Base::Vector3d(0, 0, 1);
    }

    // Rodrigues' rotation: rotate baseNormal around tangent1 by a1, then around tangent2 by a2
    auto rodrigues = [](const Base::Vector3d& v, const Base::Vector3d& k, double theta) {
        double ct = std::cos(theta);
        double st = std::sin(theta);
        return v * ct + k.Cross(v) * st + k * (k * v) * (1.0 - ct);
    };

    Base::Vector3d n = rodrigues(baseNormal, tangent1, a1);
    n = rodrigues(n, tangent2, a2);

    double len = n.Length();
    if (len > 1e-10) {
        n = n / len;
    }

    normalX->blockSignals(true);
    normalY->blockSignals(true);
    normalZ->blockSignals(true);
    normalX->setValue(n.x);
    normalY->setValue(n.y);
    normalZ->setValue(n.z);
    normalX->blockSignals(false);
    normalY->blockSignals(false);
    normalZ->blockSignals(false);

    // When the normal rotates, we must adjust the offset to keep the plane
    // passing through the same geometric point. The plane point is oldN * oldD.
    // New offset = dot(oldPlanePoint, newNormal).
    Base::Vector3d oldN = feature->PlaneNormal.getValue();
    double oldD = feature->PlaneOffset.getValue();
    double oldLen = oldN.Length();
    Base::Vector3d oldPlanePoint = (oldLen > 1e-10) ? (oldN / oldLen) * oldD
                                                    : Base::Vector3d(0, 0, 0);
    double newOffset = oldPlanePoint.x * n.x + oldPlanePoint.y * n.y + oldPlanePoint.z * n.z;

    Base::Console().log(
        "SectionAnalysis: angle change — planePoint (%.2f,%.2f,%.2f) "
        "old offset %.2f → new normal (%.4f,%.4f,%.4f) new offset %.2f\n",
        oldPlanePoint.x,
        oldPlanePoint.y,
        oldPlanePoint.z,
        oldD,
        n.x,
        n.y,
        n.z,
        newOffset
    );

    feature->PlaneNormal.setValue(n);
    feature->PlaneOffset.setValue(newOffset);
    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onNormalXChanged(double val)
{
    Base::Vector3d n(val, normalY->value(), normalZ->value());
    feature->PlaneNormal.setValue(n);
    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onNormalYChanged(double val)
{
    Base::Vector3d n(normalX->value(), val, normalZ->value());
    feature->PlaneNormal.setValue(n);
    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onNormalZChanged(double val)
{
    Base::Vector3d n(normalX->value(), normalY->value(), val);
    feature->PlaneNormal.setValue(n);
    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onOffsetChanged(double val)
{
    feature->PlaneOffset.setValue(val + offsetBase);

    int sliderPos = 0;
    if (sliderMax > 0) {
        sliderPos = static_cast<int>(val / sliderMax * 1000.0);
        sliderPos = std::clamp(sliderPos, 0, 1000);
    }
    offsetSlider->blockSignals(true);
    offsetSlider->setValue(sliderPos);
    offsetSlider->blockSignals(false);

    deferRecompute();
}

void SectionAnalysisWidget::onSliderMoved(int val)
{
    double spinVal = sliderMax * val / 1000.0;

    offsetSpin->blockSignals(true);
    offsetSpin->setValue(spinVal);
    offsetSpin->blockSignals(false);

    feature->PlaneOffset.setValue(spinVal + offsetBase);
    deferRecompute();
}

void SectionAnalysisWidget::onFlipToggled(bool on)
{
    feature->FlipCut.setValue(on);
    recompute();
}

void SectionAnalysisWidget::deferRecompute()
{
    // Delay the expensive OCCT recompute so interactive dragging stays responsive.
    // The visual clip plane + plane quad update instantly via ViewProvider::updateData().
    // The cross-section faces (OCCT boolean) update 300ms after the last change.
    if (!recomputeTimer) {
        recomputeTimer = new QTimer(this);
        recomputeTimer->setSingleShot(true);
        recomputeTimer->setInterval(300);
        connect(recomputeTimer, &QTimer::timeout, this, [this]() { recompute(); });
    }
    recomputeTimer->start();
}

void SectionAnalysisWidget::onSectionColorChanged(const QColor& color)
{
    // Update the section face color on the ViewProvider
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature)
    );
    if (vp) {
        App::Material mat;
        mat.diffuseColor.set(color.redF(), color.greenF(), color.blueF(), 0.0f);
        vp->ShapeAppearance.setValues({mat});
    }
}

void SectionAnalysisWidget::onHatchToggled(bool on)
{
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature)
    );
    if (vp) {
        vp->setHatching(on);
    }
}

void SectionAnalysisWidget::onPerSolidColorToggled(bool on)
{
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature)
    );
    if (vp) {
        vp->setPerSolidColors(on);
    }
}

void SectionAnalysisWidget::onShowPlaneToggled(bool on)
{
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature)
    );
    if (vp) {
        vp->setShowPlane(on);
    }
}

void SectionAnalysisWidget::onUpdateViewToggled(bool on)
{
    if (on) {
        recompute();
    }
}

void SectionAnalysisWidget::recompute()
{
    if (updateViewCheck && updateViewCheck->isChecked()) {
        feature->getDocument()->recomputeFeature(feature);
    }
}

void SectionAnalysisWidget::updateFromFeature()
{
    // Read values from feature and update all controls (without triggering signals).
    // Keeps the current preset — decomposes the normal into angles relative to it.
    Base::Vector3d n = feature->PlaneNormal.getValue();
    bool flip = feature->FlipCut.getValue();

    normalX->blockSignals(true);
    normalY->blockSignals(true);
    normalZ->blockSignals(true);
    flipCheck->blockSignals(true);
    angle1Spin->blockSignals(true);
    angle2Spin->blockSignals(true);

    normalX->setValue(n.x);
    normalY->setValue(n.y);
    normalZ->setValue(n.z);
    flipCheck->setChecked(flip);

    // Decompose the current normal into angles relative to the current preset.
    // This is the exact inverse of the Rodrigues rotation in applyAngles().
    int preset = presetCombo->currentIndex();
    switch (preset) {
        case 0: {  // XY plane (Z normal)
            // Forward: n = (cosA·sinB, sinA, cosA·cosB)
            Base::Vector3d ne = (n.z < 0) ? -n : n;
            double alpha = std::asin(std::clamp(ne.y, -1.0, 1.0));
            double cosA = std::cos(alpha);
            double beta = (cosA > 1e-10) ? std::atan2(ne.x, ne.z) : 0.0;
            angle1Spin->setValue(alpha * 180.0 / M_PI);
            angle2Spin->setValue(beta * 180.0 / M_PI);
            break;
        }
        case 1: {  // XZ plane (Y normal)
            // Forward: n = (-cosA·sinB, cosA·cosB, -sinA)
            Base::Vector3d ne = (n.y < 0) ? -n : n;
            double alpha = std::asin(std::clamp(-ne.z, -1.0, 1.0));
            double cosA = std::cos(alpha);
            double beta = (cosA > 1e-10) ? std::atan2(-ne.x, ne.y) : 0.0;
            angle1Spin->setValue(alpha * 180.0 / M_PI);
            angle2Spin->setValue(beta * 180.0 / M_PI);
            break;
        }
        case 2: {  // YZ plane (X normal)
            // Forward: n = (cosA·cosB, cosA·sinB, sinA)
            Base::Vector3d ne = (n.x < 0) ? -n : n;
            double alpha = std::asin(std::clamp(ne.z, -1.0, 1.0));
            double cosA = std::cos(alpha);
            double beta = (cosA > 1e-10) ? std::atan2(ne.y, ne.x) : 0.0;
            angle1Spin->setValue(alpha * 180.0 / M_PI);
            angle2Spin->setValue(beta * 180.0 / M_PI);
            break;
        }
        default:
            break;  // Custom/View Direction: leave angles as-is
    }

    angle1Spin->blockSignals(false);
    angle2Spin->blockSignals(false);
    normalX->blockSignals(false);
    normalY->blockSignals(false);
    normalZ->blockSignals(false);
    flipCheck->blockSignals(false);

    updateSliderRange();
}

bool SectionAnalysisWidget::accept()
{
    try {
        Gui::cmdAppObjectArgs(
            feature,
            "PlaneNormal = FreeCAD.Vector(%f, %f, %f)",
            normalX->value(),
            normalY->value(),
            normalZ->value()
        );
        double offsetValue = offsetSpin->value().getValue() + offsetBase;
        Gui::cmdAppObjectArgs(feature, "PlaneOffset = %f", offsetValue);
        Gui::cmdAppObjectArgs(feature, "FlipCut = %s", flipCheck->isChecked() ? "True" : "False");

        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        if (!feature->isValid()) {
            throw Base::CADKernelError(feature->getStatusString());
        }

        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        // Commit the transaction opened by the command (Create Section Analysis)
        feature->getDocument()->commitTransaction();
    }
    catch (const Base::Exception& e) {
        // On error, abort and remove the object
        auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
            Gui::Application::Instance->getViewProvider(feature)
        );
        if (vp) {
            vp->hide();
        }
        feature->getDocument()->abortTransaction();
        QMessageBox::warning(
            this,
            tr("Input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    return true;
}

bool SectionAnalysisWidget::reject()
{
    // Remove clipping before undo so the source VP is restored
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature)
    );
    if (vp) {
        vp->hide();
    }

    // Roll back the transaction (removes the SectionAnalysis object if newly created)
    feature->getDocument()->abortTransaction();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();
    return true;
}


// -----------------------------------------------------------------------
// TaskSectionAnalysis
// -----------------------------------------------------------------------

TaskSectionAnalysis::TaskSectionAnalysis(Part::SectionAnalysis* feature, ViewProviderSectionAnalysis* vp)
{
    widget = new SectionAnalysisWidget(feature, vp);
    addTaskBox(Gui::BitmapFactory().pixmap("Part_SectionAnalysis"), widget);
}

TaskSectionAnalysis::~TaskSectionAnalysis() = default;

Part::SectionAnalysis* TaskSectionAnalysis::getObject() const
{
    return widget->getObject();
}


void TaskSectionAnalysis::updateFromFeature()
{
    widget->updateFromFeature();
}

bool TaskSectionAnalysis::accept()
{
    return widget->accept();
}

bool TaskSectionAnalysis::reject()
{
    return widget->reject();
}

#include "moc_TaskSectionAnalysis.cpp"
