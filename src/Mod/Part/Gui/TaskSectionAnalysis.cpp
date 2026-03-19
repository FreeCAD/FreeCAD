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

#include <cmath>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSlider>
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
#include <Gui/View3DInventorViewer.h>
#include <Gui/Widgets.h>
#include <App/Material.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include "TaskSectionAnalysis.h"
#include "ViewProviderSectionAnalysis.h"


using namespace PartGui;

// -----------------------------------------------------------------------
// SectionAnalysisWidget
// -----------------------------------------------------------------------

SectionAnalysisWidget::SectionAnalysisWidget(Part::SectionAnalysis* feat, QWidget* parent)
    : QWidget(parent)
    , feature(feat)
{
    setupUi();
    setupConnections();
    updateSliderRange();

    // Enable hatching by default
    onHatchToggled(true);
}

SectionAnalysisWidget::~SectionAnalysisWidget() = default;

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
    else if (std::abs(n.x) < 1e-6 && std::abs(std::abs(n.y) - 1.0) < 1e-6
             && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(1);
    }
    else if (std::abs(std::abs(n.x) - 1.0) < 1e-6 && std::abs(n.y) < 1e-6
             && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(2);
    }
    else {
        presetCombo->setCurrentIndex(4);  // Custom
    }

    // Normal spinboxes
    planeLayout->addWidget(new QLabel(tr("Normal X:"), this), 1, 0);
    normalX = new QDoubleSpinBox(this);
    normalX->setRange(-1.0, 1.0);
    normalX->setDecimals(4);
    normalX->setSingleStep(0.1);
    normalX->setValue(n.x);
    planeLayout->addWidget(normalX, 1, 1);

    planeLayout->addWidget(new QLabel(tr("Normal Y:"), this), 2, 0);
    normalY = new QDoubleSpinBox(this);
    normalY->setRange(-1.0, 1.0);
    normalY->setDecimals(4);
    normalY->setSingleStep(0.1);
    normalY->setValue(n.y);
    planeLayout->addWidget(normalY, 2, 1);

    planeLayout->addWidget(new QLabel(tr("Normal Z:"), this), 3, 0);
    normalZ = new QDoubleSpinBox(this);
    normalZ->setRange(-1.0, 1.0);
    normalZ->setDecimals(4);
    normalZ->setSingleStep(0.1);
    normalZ->setValue(n.z);
    planeLayout->addWidget(normalZ, 3, 1);

    // Enable/disable normal spinboxes based on preset
    bool isCustom = (presetCombo->currentIndex() == 4);
    normalX->setEnabled(isCustom);
    normalY->setEnabled(isCustom);
    normalZ->setEnabled(isCustom);

    // Angle adjustments (tilt the plane from the preset orientation)
    planeLayout->addWidget(new QLabel(tr("X Angle:"), this), 4, 0);
    angleX = new QDoubleSpinBox(this);
    angleX->setRange(-90.0, 90.0);
    angleX->setDecimals(1);
    angleX->setSingleStep(1.0);
    angleX->setSuffix(QString::fromUtf8("\xc2\xb0"));  // degree sign
    angleX->setValue(0.0);
    planeLayout->addWidget(angleX, 4, 1);

    planeLayout->addWidget(new QLabel(tr("Z Angle:"), this), 5, 0);
    angleZ = new QDoubleSpinBox(this);
    angleZ->setRange(-90.0, 90.0);
    angleZ->setDecimals(1);
    angleZ->setSingleStep(1.0);
    angleZ->setSuffix(QString::fromUtf8("\xc2\xb0"));
    angleZ->setValue(0.0);
    planeLayout->addWidget(angleZ, 5, 1);

    // Offset
    planeLayout->addWidget(new QLabel(tr("Offset:"), this), 6, 0);
    offsetSpin = new Gui::QuantitySpinBox(this);
    offsetSpin->setUnit(Base::Unit::Length);
    offsetSpin->setRange(-1e9, 1e9);
    offsetSpin->setSingleStep(1.0);
    offsetSpin->setValue(feature->PlaneOffset.getValue());
    planeLayout->addWidget(offsetSpin, 6, 1);

    // Offset slider
    offsetSlider = new QSlider(Qt::Horizontal, this);
    offsetSlider->setRange(0, 1000);
    offsetSlider->setValue(500);
    planeLayout->addWidget(offsetSlider, 7, 0, 1, 2);

    // Flip
    flipCheck = new QCheckBox(tr("Flip Direction"), this);
    flipCheck->setChecked(feature->FlipCut.getValue());
    planeLayout->addWidget(flipCheck, 8, 0, 1, 2);

    mainLayout->addWidget(planeGroup);

    // --- Appearance group ---
    auto* appearGroup = new QGroupBox(tr("Appearance"), this);
    auto* appearLayout = new QGridLayout(appearGroup);

    appearLayout->addWidget(new QLabel(tr("Section Color:"), this), 0, 0);
    sectionColorBtn = new Gui::ColorButton(this);
    sectionColorBtn->setColor(QColor(204, 76, 51));  // default reddish-orange
    appearLayout->addWidget(sectionColorBtn, 0, 1);

    hatchCheck = new QCheckBox(tr("Show Hatching"), this);
    hatchCheck->setChecked(true);
    appearLayout->addWidget(hatchCheck, 1, 0, 1, 2);

    mainLayout->addWidget(appearGroup);

    // Update View checkbox
    updateViewCheck = new QCheckBox(tr("Update View"), this);
    updateViewCheck->setChecked(true);
    mainLayout->addWidget(updateViewCheck);

    mainLayout->addStretch();
}

void SectionAnalysisWidget::setupConnections()
{
    connect(presetCombo, qOverload<int>(&QComboBox::activated),
            this, &SectionAnalysisWidget::onPresetChanged);
    connect(normalX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &SectionAnalysisWidget::onNormalXChanged);
    connect(normalY, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &SectionAnalysisWidget::onNormalYChanged);
    connect(normalZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &SectionAnalysisWidget::onNormalZChanged);
    connect(angleX, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &SectionAnalysisWidget::onAngleXChanged);
    connect(angleZ, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &SectionAnalysisWidget::onAngleZChanged);
    connect(offsetSpin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &SectionAnalysisWidget::onOffsetChanged);
    connect(offsetSlider, &QSlider::valueChanged,
            this, &SectionAnalysisWidget::onSliderMoved);
    connect(flipCheck, &QCheckBox::toggled,
            this, &SectionAnalysisWidget::onFlipToggled);
    connect(sectionColorBtn, &Gui::ColorButton::changed,
            this, [this]() { onSectionColorChanged(sectionColorBtn->color()); });
    connect(hatchCheck, &QCheckBox::toggled,
            this, &SectionAnalysisWidget::onHatchToggled);
    connect(updateViewCheck, &QCheckBox::toggled,
            this, &SectionAnalysisWidget::onUpdateViewToggled);
}

void SectionAnalysisWidget::updateSliderRange()
{
    App::DocumentObject* source = feature->Source.getValue();
    if (!source) {
        return;
    }

    TopoDS_Shape sourceShape = Part::Feature::getShape(source,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
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
    double corners[8][3] = {{xmin, ymin, zmin}, {xmax, ymin, zmin}, {xmin, ymax, zmin},
                             {xmax, ymax, zmin}, {xmin, ymin, zmax}, {xmax, ymin, zmax},
                             {xmin, ymax, zmax}, {xmax, ymax, zmax}};

    sliderMin = 1e20;
    sliderMax = -1e20;
    for (auto& corner : corners) {
        double proj = corner[0] * n.x + corner[1] * n.y + corner[2] * n.z;
        sliderMin = std::min(sliderMin, proj);
        sliderMax = std::max(sliderMax, proj);
    }

    // Add small margin
    double margin = (sliderMax - sliderMin) * 0.05;
    sliderMin -= margin;
    sliderMax += margin;

    // Update spinbox range
    offsetSpin->setRange(sliderMin, sliderMax);

    // Map current offset to slider position
    double val = feature->PlaneOffset.getValue();
    int sliderPos = 0;
    if (sliderMax > sliderMin) {
        sliderPos = static_cast<int>((val - sliderMin) / (sliderMax - sliderMin) * 1000.0);
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
                Gui::Application::Instance->activeView());
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
            // Custom: just enable the spinboxes
            normalX->setEnabled(true);
            normalY->setEnabled(true);
            normalZ->setEnabled(true);
            return;
    }

    normalX->setEnabled(false);
    normalY->setEnabled(false);
    normalZ->setEnabled(false);

    // Reset angles when switching presets
    angleX->blockSignals(true);
    angleZ->blockSignals(true);
    angleX->setValue(0.0);
    angleZ->setValue(0.0);
    angleX->blockSignals(false);
    angleZ->blockSignals(false);

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
        TopoDS_Shape sourceShape = Part::Feature::getShape(source,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        if (!sourceShape.IsNull()) {
            Bnd_Box bbox;
            BRepBndLib::Add(sourceShape, bbox);
            if (!bbox.IsVoid()) {
                double xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                Base::Vector3d center((xmin + xmax) / 2, (ymin + ymax) / 2,
                                     (zmin + zmax) / 2);
                double centerProj = center.x * normal.x + center.y * normal.y
                    + center.z * normal.z;
                feature->PlaneOffset.setValue(centerProj);
                offsetSpin->blockSignals(true);
                offsetSpin->setValue(centerProj);
                offsetSpin->blockSignals(false);
            }
        }
    }

    updateSliderRange();
    recompute();
}

void SectionAnalysisWidget::onAngleXChanged(double /*val*/)
{
    applyAngles();
}

void SectionAnalysisWidget::onAngleZChanged(double /*val*/)
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

    double a1 = angleX->value() * M_PI / 180.0;
    double a2 = angleZ->value() * M_PI / 180.0;

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
    Base::Vector3d oldPlanePoint = (oldLen > 1e-10) ? (oldN / oldLen) * oldD : Base::Vector3d(0, 0, 0);
    double newOffset = oldPlanePoint.x * n.x + oldPlanePoint.y * n.y + oldPlanePoint.z * n.z;

    Base::Console().log("SectionAnalysis: angle change — planePoint (%.2f,%.2f,%.2f) "
                        "old offset %.2f → new normal (%.4f,%.4f,%.4f) new offset %.2f\n",
                        oldPlanePoint.x, oldPlanePoint.y, oldPlanePoint.z, oldD,
                        n.x, n.y, n.z, newOffset);

    feature->PlaneNormal.setValue(n);
    feature->PlaneOffset.setValue(newOffset);
    offsetSpin->blockSignals(true);
    offsetSpin->setValue(newOffset);
    offsetSpin->blockSignals(false);
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
    feature->PlaneOffset.setValue(val);

    // Update slider
    int sliderPos = 0;
    if (sliderMax > sliderMin) {
        sliderPos = static_cast<int>((val - sliderMin) / (sliderMax - sliderMin) * 1000.0);
        sliderPos = std::clamp(sliderPos, 0, 1000);
    }
    offsetSlider->blockSignals(true);
    offsetSlider->setValue(sliderPos);
    offsetSlider->blockSignals(false);

    recompute();
}

void SectionAnalysisWidget::onSliderMoved(int val)
{
    double offset = sliderMin + (sliderMax - sliderMin) * val / 1000.0;

    offsetSpin->blockSignals(true);
    offsetSpin->setValue(offset);
    offsetSpin->blockSignals(false);

    feature->PlaneOffset.setValue(offset);
    recompute();
}

void SectionAnalysisWidget::onFlipToggled(bool on)
{
    feature->FlipCut.setValue(on);
    recompute();
}

void SectionAnalysisWidget::onSectionColorChanged(const QColor& color)
{
    // Update the section face color on the ViewProvider
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature));
    if (vp) {
        App::Material mat;
        mat.diffuseColor.set(color.redF(), color.greenF(), color.blueF(), 0.0f);
        vp->ShapeAppearance.setValues({mat});
    }
}

void SectionAnalysisWidget::onHatchToggled(bool on)
{
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature));
    if (vp) {
        vp->setHatching(on);
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
    // Read values from feature and update all controls (without triggering signals)
    Base::Vector3d n = feature->PlaneNormal.getValue();
    double d = feature->PlaneOffset.getValue();
    bool flip = feature->FlipCut.getValue();

    normalX->blockSignals(true);
    normalY->blockSignals(true);
    normalZ->blockSignals(true);
    offsetSpin->blockSignals(true);
    flipCheck->blockSignals(true);

    normalX->setValue(n.x);
    normalY->setValue(n.y);
    normalZ->setValue(n.z);
    offsetSpin->setValue(d);
    flipCheck->setChecked(flip);

    // Detect preset
    if (std::abs(n.x) < 1e-6 && std::abs(n.y) < 1e-6 && std::abs(std::abs(n.z) - 1.0) < 1e-6) {
        presetCombo->setCurrentIndex(0);
        normalX->setEnabled(false);
        normalY->setEnabled(false);
        normalZ->setEnabled(false);
    }
    else if (std::abs(n.x) < 1e-6 && std::abs(std::abs(n.y) - 1.0) < 1e-6
             && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(1);
        normalX->setEnabled(false);
        normalY->setEnabled(false);
        normalZ->setEnabled(false);
    }
    else if (std::abs(std::abs(n.x) - 1.0) < 1e-6 && std::abs(n.y) < 1e-6
             && std::abs(n.z) < 1e-6) {
        presetCombo->setCurrentIndex(2);
        normalX->setEnabled(false);
        normalY->setEnabled(false);
        normalZ->setEnabled(false);
    }
    else {
        presetCombo->setCurrentIndex(4);
        normalX->setEnabled(true);
        normalY->setEnabled(true);
        normalZ->setEnabled(true);
    }

    normalX->blockSignals(false);
    normalY->blockSignals(false);
    normalZ->blockSignals(false);
    offsetSpin->blockSignals(false);
    flipCheck->blockSignals(false);

    updateSliderRange();
}

bool SectionAnalysisWidget::accept()
{
    try {
        Gui::cmdAppObjectArgs(feature, "PlaneNormal = FreeCAD.Vector(%f, %f, %f)",
                              normalX->value(), normalY->value(), normalZ->value());
        double offsetValue = offsetSpin->value().getValue();
        Gui::cmdAppObjectArgs(feature, "PlaneOffset = %f", offsetValue);
        Gui::cmdAppObjectArgs(feature, "FlipCut = %s",
                              flipCheck->isChecked() ? "True" : "False");

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
            Gui::Application::Instance->getViewProvider(feature));
        if (vp) {
            vp->hide();
        }
        feature->getDocument()->abortTransaction();
        QMessageBox::warning(
            this, tr("Input error"),
            QCoreApplication::translate("Exception", e.what()));
        return false;
    }

    return true;
}

bool SectionAnalysisWidget::reject()
{
    // Remove clipping before undo so the source VP is restored
    auto* vp = dynamic_cast<PartGui::ViewProviderSectionAnalysis*>(
        Gui::Application::Instance->getViewProvider(feature));
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

TaskSectionAnalysis::TaskSectionAnalysis(Part::SectionAnalysis* feature)
{
    widget = new SectionAnalysisWidget(feature);
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
