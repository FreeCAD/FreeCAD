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
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>


#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/QuantitySpinBox.h>
#include <QSignalBlocker>

#include <numbers>

#include <Precision.hxx>

#include <Base/Console.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewParams.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Widgets.h>
#include <App/Material.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include <Base/Converter.h>
#include <Gui/Utilities.h>
#include <Inventor/draggers/SoDragger.h>
#include <Gui/InputHint.h>
#include <Gui/MainWindow.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Gui/Inventor/Draggers/SoLinearDragger.h>
#include <Gui/Inventor/Draggers/SoRotationDragger.h>

#include "TaskSectionAnalysis.h"
#include "ViewProviderSectionAnalysis.h"


using namespace PartGui;

// -----------------------------------------------------------------------
// SectionAnalysisWidget
// -----------------------------------------------------------------------

namespace
{
/// Smallest length or cosine still worth dividing by.
///
/// Deliberately not Precision::Confusion(): that is a modelling tolerance in
/// millimetres, and these quantities are dimensionless - a unit vector's length
/// and the cosine of an angle. Borrowing a length tolerance for them reads as a
/// geometric claim that is not being made.
constexpr double minUnitMagnitude = 1e-10;

/// True when `normal` points along `axis`, either way round.
///
/// Spelled out because the alternative - three chained comparisons of each
/// component against a tolerance - says how the test is done rather than what it
/// asks, and reads the same for all three axes.
bool isAlignedWith(const Base::Vector3d& normal, const Base::Vector3d& axis)
{
    constexpr double alignmentTolerance = 1e-6;
    return std::abs(std::abs(normal * axis) - 1.0) < alignmentTolerance;
}

/// Work out which way the camera is looking
Base::Vector3d cameraViewDirection()
{
    auto* mdiView = qobject_cast<Gui::View3DInventor*>(Gui::Application::Instance->activeView());
    if (!mdiView) {
        return -Base::Vector3d::UnitZ;
    }
    float vx = 0.0F;
    float vy = 0.0F;
    float vz = 0.0F;
    mdiView->getViewer()->getViewDirection().getValue(vx, vy, vz);
    return Base::Vector3d(vx, vy, vz);
}


/// Helps in picking initial preset.
/// Returns a normal vector that is aligned with axis and points in the same
/// direction as the camera's view vector, unless current is already aligned
/// with axis, in which case it is returned (possibly negated to match the
/// camera's view direction). 
Base::Vector3d presetNormal(const Base::Vector3d& axis, const Base::Vector3d& current)
{
    if (isAlignedWith(current, axis)) {
        return (current * axis < 0.0) ? -axis : axis;
    }
    return (cameraViewDirection() * axis < 0.0) ? axis : -axis;
}
}  // namespace


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
    setupGizmos();
    onHatchToggled(true);
}

SectionAnalysisWidget::~SectionAnalysisWidget()
{
    // The hint belongs to the panel, so it has to go when the panel does
    hideDraggerHints();
}

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
    // Shown when the plane matches no preset, so the box reads as "none of
    // these" rather than as an empty control that failed to populate.
    presetCombo->setPlaceholderText(tr("(no preset)"));
    planeLayout->addWidget(presetCombo, 0, 1);

    // Detect current preset from normal
    const Base::Vector3d n = feature->PlaneNormal.getValue();
    if (isAlignedWith(n, Base::Vector3d::UnitZ)) {
        presetCombo->setCurrentIndex(static_cast<int>(Preset::XY));
    }
    else if (isAlignedWith(n, Base::Vector3d::UnitY)) {
        presetCombo->setCurrentIndex(static_cast<int>(Preset::XZ));
    }
    else if (isAlignedWith(n, Base::Vector3d::UnitX)) {
        presetCombo->setCurrentIndex(static_cast<int>(Preset::YZ));
    }
    else {
        // Any tilt bakes itself into PlaneNormal, so a reopened section usually
        // matches no preset. Blank says that honestly; the tilt boxes still work,
        // taking the current normal as their base.
        presetCombo->setCurrentIndex(-1);
    }

    // Tilting starts from wherever the plane points as the panel opens.
    const double openLen = n.Length();
    angleBaseNormal = (openLen > minUnitMagnitude) ? n / openLen : Base::Vector3d::UnitZ;

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

    applyPresetAngleLabels(static_cast<Preset>(presetCombo->currentIndex()));

    // Offset along the normal. The arrow gizmo edits this box rather than the
    // feature, so dragging and typing go through exactly the same path.
    auto* offsetLabel = new QLabel(tr("Offset:"), this);
    planeLayout->addWidget(offsetLabel, 3, 0);
    offsetSpin = new Gui::QuantitySpinBox(this);
    offsetSpin->setUnit(Base::Unit::Length);
    offsetSpin->setRange(-1.0e7, 1.0e7);
    offsetSpin->setSingleStep(1.0);
    offsetSpin->setValue(feature->PlaneOffset.getValue());
    planeLayout->addWidget(offsetSpin, 3, 1);

    flipCheck = new QCheckBox(tr("Flip Direction"), this);
    flipCheck->setChecked(feature->FlipCut.getValue());
    planeLayout->addWidget(flipCheck, 4, 0, 1, 2);

    mainLayout->addWidget(planeGroup);

    // --- Appearance group ---
    auto* appearGroup = new QGroupBox(tr("Appearance"), this);
    auto* appearLayout = new QGridLayout(appearGroup);

    appearLayout->addWidget(new QLabel(tr("Section Color:"), this), 0, 0);
    sectionColorBtn = new Gui::ColorButton(this);
    {
        // Reflect the section's actual colour (each new section gets a distinct
        // default colour assigned at creation) rather than a fixed swatch.
        const App::Material& curMat = viewProvider->ShapeAppearance[0];
        sectionColorBtn->setColor(
            QColor::fromRgbF(curMat.diffuseColor.r, curMat.diffuseColor.g, curMat.diffuseColor.b)
        );
    }
    appearLayout->addWidget(sectionColorBtn, 0, 1);

    hatchCheck = new QCheckBox(tr("Show Hatching"), this);
    hatchCheck->setChecked(viewProvider->ShowHatching.getValue());
    appearLayout->addWidget(hatchCheck, 1, 0, 1, 2);

    autoHideHatchCheck = new QCheckBox(tr("Fade Hatching When Zoomed Out"), this);
    autoHideHatchCheck->setToolTip(
        tr("Fade the hatching out, then stop drawing it, once the lines are too\n"
           "close together on screen to be told apart")
    );
    autoHideHatchCheck->setChecked(viewProvider->AutoHideHatching.getValue());
    appearLayout->addWidget(autoHideHatchCheck, 4, 0, 1, 2);

    perSolidColorCheck = new QCheckBox(tr("Per-Body Colors"), this);
    perSolidColorCheck->setChecked(viewProvider->PerBodyColors.getValue());
    // Nothing to tell apart below two bodies, and while it is on the single
    // colour picker would overwrite the per-body colours it assigns
    // Counted from the source recursion rather than read off SourceParts, which
    // Display mode leaves empty by design - so this was permanently greyed out
    // in the default mode. It still showed ticked, because PerBodyColors is a
    // separate view property the cap reads directly, so setting it from the
    // property editor worked while the panel insisted it could not be changed.
    perSolidColorCheck->setEnabled(
        Part::SectionAnalysis::distinctSourceParts(feature->Source.getValues(), feature).size() > 1
    );
    if (!perSolidColorCheck->isEnabled()) {
        perSolidColorCheck->setToolTip(tr("The section only comes from a single body"));
    }
    sectionColorBtn->setEnabled(!perSolidColorCheck->isChecked());
    appearLayout->addWidget(perSolidColorCheck, 2, 0, 1, 2);

    ghostCheck = new QCheckBox(tr("Show Removed Material"), this);
    ghostCheck->setToolTip(
        tr("Draw the material the section removes faintly, so the\n"
           "cross-sections have something to sit in")
    );
    ghostCheck->setChecked(viewProvider->ShowRemovedMaterial.getValue());
    // Row and span given, like every other row here. The one argument overload
    // is QLayout's, and on a grid it drops the widget in a cell of its own
    // choosing spanning a single column - which is neither where this reads as
    // going nor the full width its siblings get.
    appearLayout->addWidget(ghostCheck, 5, 0, 1, 2);

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


void SectionAnalysisWidget::setupGizmos()
{
    // Respect the user's preference, exactly as the other features do
    if (!Gui::GizmoContainer::isEnabled()) {
        return;
    }

    // One handle per degree of freedom the plane actually has: slide along the
    // normal, and tilt about the two axes lying in it. Each is bound to the box
    // that already owns that number.
    offsetGizmo = new Gui::LinearGizmo(offsetSpin);
    tiltGizmo1 = new Gui::RotationGizmo(angle1Spin);
    tiltGizmo2 = new Gui::RotationGizmo(angle2Spin);

    // Emphatically NOT automaticOrientation: that re-aims the arc's rotation
    // axis at the camera on every view change, which is right for a handle that
    // spins about its own pointer (Pad's taper angle) but wrong here. Each of
    // our arcs turns the plane about one fixed in-plane axis, the one its spin
    // box owns, and letting the camera redefine that axis makes the handle
    // rotate about something arbitrary.
    tiltGizmo1->automaticOrientation = false;
    tiltGizmo2->automaticOrientation = false;

    gizmoContainer = Gui::GizmoContainer::create({offsetGizmo, tiltGizmo1, tiltGizmo2}, viewProvider);

    // Dont show the blue line. 
    // The SoArrowBase negative-height bug makes it point the wrong way, and the arrowhead is already on the other end.
    offsetGizmo->getDraggerContainer()->getDragger()->baseGeomVisible = false;

    // After create(), because initDragger() applies the theme colours and would
    // otherwise overwrite these. Two arcs in one colour are indistinguishable,
    // so each takes the colour of the axis it turns about.
    auto axisColor = [](unsigned long packed) {
        SbColor colour;
        float transparency = 0.0F;
        colour.setPackedValue(packed, transparency);
        return colour;
    };
    const auto* viewParams = Gui::ViewParams::instance();
    tiltGizmo1->getDraggerContainer()->color.setValue(axisColor(viewParams->getAxisXColor()));
    tiltGizmo1->getDraggerContainer()->getDragger()->color = axisColor(viewParams->getAxisXColor());
    tiltGizmo2->getDraggerContainer()->color.setValue(axisColor(viewParams->getAxisYColor()));
    tiltGizmo2->getDraggerContainer()->getDragger()->color = axisColor(viewParams->getAxisYColor());

    // A released handle needs one placement to settle; whether a drag is in
    // progress is asked of the draggers themselves, not tracked here.
    auto placeOnRelease = [this](SoDragger* dragger) {
        if (dragger) {
            dragger->addFinishCallback(
                [](void* data, SoDragger*) {
                    static_cast<SectionAnalysisWidget*>(data)->setGizmoPositions();
                },
                this
            );
        }
    };
    placeOnRelease(offsetGizmo->getDraggerContainer()->getDragger());
    placeOnRelease(tiltGizmo1->getDraggerContainer()->getDragger());
    placeOnRelease(tiltGizmo2->getDraggerContainer()->getDragger());

    // RotationGizmo defaults to 1 degree steps, which reads as jerky when you
    // are aiming a section plane by eye. Match the spin box's own 0.1 degree
    // step so dragging and typing have the same resolution.
    constexpr double tiltStepDegrees = 0.1;
    for (Gui::RotationGizmo* tilt : {tiltGizmo1, tiltGizmo2}) {
        tilt->getDraggerContainer()->getDragger()->rotationIncrement = tiltStepDegrees
            * std::numbers::pi / 180.0;
    }

    setGizmoPositions();

    // Paired with the hideDraggerHints() in the destructor. There are handles on
    // screen from here on, and the modifier that makes them drag finely is not
    // discoverable without being told.
    showDraggerHints();
}


void SectionAnalysisWidget::showDraggerHints()
{
    if (!Gui::GizmoContainer::isEnabled() || !Gui::GizmoContainer::isCoarseSnapEnabled()) {
        return;
    }

    const Gui::InputHint::UserInput key = Gui::GizmoContainer::getFineSnapKey();
    const QString message = Gui::GizmoContainer::isCoarseByDefault() ? tr("%1 fine dragging")
                                                                     : tr("%1 coarse dragging");

    Gui::getMainWindow()->showHints({{
        .message = message,
        .sequences = {{key}},
    }});
}


void SectionAnalysisWidget::hideDraggerHints()
{
    Gui::getMainWindow()->hideHints();
}


bool SectionAnalysisWidget::anyGizmoDragging() const
{
    const SoDragger* draggers[] = {
        offsetGizmo ? offsetGizmo->getDraggerContainer()->getDragger() : nullptr,
        tiltGizmo1 ? tiltGizmo1->getDraggerContainer()->getDragger() : nullptr,
        tiltGizmo2 ? tiltGizmo2->getDraggerContainer()->getDragger() : nullptr,
    };
    for (const SoDragger* dragger : draggers) {
        if (dragger && dragger->isActive.getValue()) {
            return true;
        }
    }
    return false;
}


void SectionAnalysisWidget::setGizmoPositions()
{
    // guard against the case when gizmos arent there yet or already mid-dragging
    if (!gizmoContainer || !offsetGizmo || anyGizmoDragging()) {
        return;
    }

    Base::Vector3d normal;
    double offset = 0.0;
    if (!feature->cutPlane(normal, offset)) {
        return;
    }

    // Anchored on the geometry rather than on the plane's closest approach to
    // the world origin, which on an imported assembly is nowhere near the model.
    // The box comes from the view provider so the handles and the plane quad are
    // placed from the same one - measuring it twice let them drift apart.
    // Without a box there is no hint worth having: draggerAnchor would fall back
    // to the plane's closest approach to the world origin, which is the very
    // thing this exists to avoid.
    Base::Vector3d hint(0, 0, 0);
    double diagonal = 0.0;
    if (!viewProvider || !viewProvider->sourceBounds(hint, diagonal)) {
        return;
    }
    const Base::Vector3d onPlane = Part::SectionAnalysis::draggerAnchor(normal, offset, hint);

    // Stood off towards the side the section is looked at from, so the handles
    // are not buried in the cap. Proportional to the model, since the handles
    // themselves are screen sized.
    constexpr double standOff = 0.02;
    const Base::Vector3d tip = onPlane + normal * (diagonal * standOff);

    // By tip, not by base: the gizmo carries the offset as its own translation,
    // so placing the base on the plane counted it twice.
    offsetGizmo->setMultFactor(feature->FlipCut.getValue() ? -1.0 : 1.0);
    offsetGizmo->setDraggerTip(tip, normal);

    // Both arcs on the arrow, never below it.
    // They are told apart within the plane, by putting their
    // pivots on the two in-plane axes a quarter turn from each other.
    tiltGizmo1->placeOverLinearGizmo(offsetGizmo);
    tiltGizmo2->placeOverLinearGizmo(offsetGizmo);

    // placeOverLinearGizmo turns automaticOrientation back on (Gizmo.cpp:439),
    // which lets the camera overwrite the arc's rotation axis. Right for Pad,
    // wrong here - our arcs must turn about the axis their spin box owns.
    tiltGizmo1->automaticOrientation = false;
    tiltGizmo2->automaticOrientation = false;

    // Each arc turns about the axis its own spin box turns about, in the frame
    // the angles are expressed in. Taking these from the current normal instead
    // would move the axes as the plane tilts, so the handles would drift and
    // jump after every change.
    Base::Vector3d baseNormal;
    Base::Vector3d tangent1;
    Base::Vector3d tangent2;
    angleReferenceFrame(baseNormal, tangent1, tangent2);

    // Pivot first: setPointerDirection overwrites the container rotation,
    // setArcNormalDirection composes onto it.
    auto placeArc =
        [](Gui::RotationGizmo* arc, const Base::Vector3d& pivot, const Base::Vector3d& axis) {
            auto* container = arc->getDraggerContainer();
            container->setPointerDirection(Base::convertTo<SbVec3f>(pivot));
            container->setArcNormalDirection(Base::convertTo<SbVec3f>(axis));
        };

    // Only the pivot is projected into the current plane. The rotation axis
    // stays the world axis its spin box is labelled with, so "X Angle" keeps
    // meaning X; the pivot is only where the handle sits, and a world axis stops
    // lying in the plane as soon as the plane is tilted - which left the handles
    // poking through it at any non-zero angle. At zero tilt this is a no-op.
    auto inPlane = [&normal](const Base::Vector3d& axis) {
        const Base::Vector3d projected = axis - normal * (axis * normal);
        const double len = projected.Length();
        return (len > minUnitMagnitude) ? projected / len : axis;
    };

    // Negated: applyAngles() turns by -angle1, so the arc must face the other way.
    placeArc(tiltGizmo1, inPlane(tangent2), -tangent1);
    placeArc(tiltGizmo2, inPlane(tangent1), tangent2);

    const bool tiltable = angle1Spin->isEnabled();
    tiltGizmo1->setVisibility(tiltable);
    tiltGizmo2->setVisibility(tiltable);

    gizmoContainer->calculateScaleAndOrientation();
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
        // arrow gizmo moves the plane
        [this](double value) {
            feature->PlaneOffset.setValue(value);
            setGizmoPositions();
            recompute();
        }
    );
    connect(flipCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onFlipToggled);
    connect(sectionColorBtn, &Gui::ColorButton::changed, this, [this]() {
        onSectionColorChanged(sectionColorBtn->color());
    });
    connect(hatchCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onHatchToggled);
    connect(autoHideHatchCheck, &QCheckBox::toggled, this, [this](bool on) {
        viewProvider->AutoHideHatching.setValue(on);
    });
    connect(perSolidColorCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onPerSolidColorToggled);
    connect(ghostCheck, &QCheckBox::toggled, this, [this](bool on) {
        viewProvider->ShowRemovedMaterial.setValue(on);
    });
    connect(showPlaneCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onShowPlaneToggled);
    connect(updateViewCheck, &QCheckBox::toggled, this, &SectionAnalysisWidget::onUpdateViewToggled);
}

void SectionAnalysisWidget::applyPresetAngleLabels(Preset preset)
{
    switch (preset) {
        case Preset::XY:
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Y Angle:"));
            break;
        case Preset::XZ:
            angleLabel1->setText(tr("X Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
            break;
        case Preset::YZ:
            angleLabel1->setText(tr("Y Angle:"));
            angleLabel2->setText(tr("Z Angle:"));
            break;
        default:
            // View direction, or no preset at all: the tilt axes are derived
            // from the current normal, so they have no standing names.
            angleLabel1->setText(tr("Angle 1:"));
            angleLabel2->setText(tr("Angle 2:"));
            break;
    }
}

void SectionAnalysisWidget::onPresetChanged(int index)
{
    Base::Vector3d normal;
    const Base::Vector3d curN = feature->PlaneNormal.getValue();
    switch (static_cast<Preset>(index)) {
        case Preset::XY:
            normal = presetNormal(Base::Vector3d::UnitZ, curN);
            break;
        case Preset::XZ:
            normal = presetNormal(Base::Vector3d::UnitY, curN);
            break;
        case Preset::YZ:
            normal = presetNormal(Base::Vector3d::UnitX, curN);
            break;
        case Preset::ViewDirection:
            normal = -cameraViewDirection();  // face toward camera
            break;
        default:
            // Blank combo: nothing to apply, the plane keeps the pose it has.
            applyPresetAngleLabels(static_cast<Preset>(index));
            return;
    }

    applyPresetAngleLabels(static_cast<Preset>(index));

    // Reset angles when switching presets
    {
        const QSignalBlocker blockAngle1(angle1Spin);
        const QSignalBlocker blockAngle2(angle2Spin);
        angle1Spin->setValue(0.0);
        angle2Spin->setValue(0.0);
    }

    feature->PlaneNormal.setValue(normal);
    angleBaseNormal = normal;

    // Center the offset on the combined bounding box of every source, from the
    // same box the plane quad and the handles use.
    Base::Vector3d centre;
    double diagonal = 0.0;
    if (viewProvider && viewProvider->sourceBounds(centre, diagonal)) {
        feature->PlaneOffset.setValue(centre * normal);
    }

    // The offset box is what the arrow gizmo reads, so it has to follow too
    {
        const QSignalBlocker blockOffset(offsetSpin);
        offsetSpin->setValue(feature->PlaneOffset.getValue());
    }

    // A preset moves the plane wholesale - new normal, new offset, new frame -
    // so every handle needs re-placing. Without this they keep the old pose and
    // only snap into place when one of them is grabbed.
    setGizmoPositions();

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

void SectionAnalysisWidget::angleReferenceFrame(
    Base::Vector3d& baseNormal,
    Base::Vector3d& angle1Axis,
    Base::Vector3d& angle2Axis
) const
{
    // The orientation captured when the panel opened or a preset was picked
    // but never used again when changes were made
    baseNormal = angleBaseNormal;

    // Rotate about world axes lying in the plane, not about an arbitrary frame:
    // the angles are meant to read as "tilt about X", and the boxes are labelled
    // that way.
    // Whichever world axis the normal leans on most is the one there is no
    // point turning about; the other two are the tilt axes. Picking the largest
    // component rather than testing against a threshold keeps this right for the
    // presets that are not axis aligned - view direction, or none at all - where a
    // threshold could fall through and hand back an axis nearly parallel to the
    // normal, which is a degenerate pivot.
    const double alongX = std::abs(baseNormal.x);
    const double alongY = std::abs(baseNormal.y);
    const double alongZ = std::abs(baseNormal.z);
    if (alongZ >= alongX && alongZ >= alongY) {
        angle1Axis = Base::Vector3d::UnitX;
        angle2Axis = Base::Vector3d::UnitY;
    }
    else if (alongY >= alongX) {
        angle1Axis = Base::Vector3d::UnitX;
        angle2Axis = Base::Vector3d::UnitZ;
    }
    else {
        angle1Axis = Base::Vector3d::UnitY;
        angle2Axis = Base::Vector3d::UnitZ;
    }
}


void SectionAnalysisWidget::applyAngles()
{
    // Placed again at the end, once the new normal is in the feature
    struct PlaceOnExit
    {
        SectionAnalysisWidget* self;
        ~PlaceOnExit()
        {
            self->setGizmoPositions();
        }
    } placeOnExit {this};

    // Negate X angle to match the gizmo arc drag direction
    double a1 = -angle1Spin->value().getValue() * std::numbers::pi / 180.0;
    double a2 = angle2Spin->value().getValue() * std::numbers::pi / 180.0;

    // Base normal and tilt axes both come from presetFrame, which is the same
    // frame the tilt handles are placed in. A second copy of the preset switch
    // lived here, and a handle turning about a different axis than the box it
    // drives is worse than no handle at all.
    Base::Vector3d baseNormal;
    Base::Vector3d angle1Axis;
    Base::Vector3d angle2Axis;
    angleReferenceFrame(baseNormal, angle1Axis, angle2Axis);

    // Rodrigues' rotation: rotate baseNormal around angle1Axis by a1, then around angle2Axis by a2
    auto rodrigues = [](const Base::Vector3d& v, const Base::Vector3d& k, double theta) {
        double ct = std::cos(theta);
        double st = std::sin(theta);
        return v * ct + k.Cross(v) * st + k * (k * v) * (1.0 - ct);
    };

    Base::Vector3d n = rodrigues(baseNormal, angle1Axis, a1);
    n = rodrigues(n, angle2Axis, a2);

    double len = n.Length();
    if (len > minUnitMagnitude) {
        n = n / len;
    }

    // Keep the plane passing through the same geometric point as it tilts
    Base::Vector3d oldN = feature->PlaneNormal.getValue();
    double oldD = feature->PlaneOffset.getValue();
    double oldLen = oldN.Length();
    Base::Vector3d oldPlanePoint = (oldLen > minUnitMagnitude) ? (oldN / oldLen) * oldD
                                                               : Base::Vector3d(0, 0, 0);
    double newOffset = oldPlanePoint.x * n.x + oldPlanePoint.y * n.y + oldPlanePoint.z * n.z;

    feature->PlaneNormal.setValue(n);
    feature->PlaneOffset.setValue(newOffset);

    // Tilting moves the offset too - the plane is kept through the same point,
    // which is a different distance along the new normal. The arrow gizmo drags
    // this box rather than the feature, so leaving it showing the old number
    // means the next drag starts from a value the plane no longer has.
    {
        const QSignalBlocker blockOffset(offsetSpin);
        offsetSpin->setValue(newOffset);
    }

    recompute();
}

void SectionAnalysisWidget::onFlipToggled(bool on)
{
    feature->FlipCut.setValue(on);
    // Flipping turns the cut frame around, so the handles have to turn with it
    setGizmoPositions();
    recompute();
}

void SectionAnalysisWidget::onSectionColorChanged(const QColor& color)
{
    // Update the section face color on the ViewProvider
    if (viewProvider) {
        App::Material mat;
        mat.diffuseColor.set(color.redF(), color.greenF(), color.blueF(), 0.0f);
        viewProvider->ShapeAppearance.setValues({mat});
    }
}

void SectionAnalysisWidget::onHatchToggled(bool on)
{
    if (viewProvider) {
        viewProvider->setHatching(on);
    }
}

void SectionAnalysisWidget::onPerSolidColorToggled(bool on)
{
    if (viewProvider) {
        viewProvider->setPerSolidColors(on);
    }
    // The single colour would be applied to every face, undoing the per-body one
    sectionColorBtn->setEnabled(!on);
}

void SectionAnalysisWidget::onShowPlaneToggled(bool on)
{
    if (viewProvider) {
        viewProvider->setShowPlane(on);
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

bool SectionAnalysisWidget::accept()
{
    try {
        Base::Vector3d n = feature->PlaneNormal.getValue();
        // %.12g rather than %f: these are doubles, and a recorded macro should
        // reproduce the plane the user actually left rather than a rounding of it.
        Gui::cmdAppObjectArgs(feature, "PlaneNormal = FreeCAD.Vector(%.12g, %.12g, %.12g)", n.x, n.y, n.z);
        Gui::cmdAppObjectArgs(feature, "PlaneOffset = %.12g", feature->PlaneOffset.getValue());
        Gui::cmdAppObjectArgs(feature, "FlipCut = %s", feature->FlipCut.getValue() ? "True" : "False");

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
        if (viewProvider) {
            viewProvider->hide();
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
    if (viewProvider) {
        viewProvider->hide();
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

bool TaskSectionAnalysis::accept()
{
    return widget->accept();
}

bool TaskSectionAnalysis::reject()
{
    return widget->reject();
}

#include "moc_TaskSectionAnalysis.cpp"
