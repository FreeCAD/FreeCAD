/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS.hxx>

#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Gui/Inventor/Draggers/SoRotationDragger.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/Part/App/GizmoHelper.h>
#include <Mod/Part/App/Tools.h>

#include "ui_TaskHoleParameters.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;
using namespace Gui;
namespace sp = std::placeholders;

/* TRANSLATOR PartDesignGui::TaskHoleParameters */

// See Hole::HoleCutType_ISOmetric_Enums
// and Hole::HoleCutType_ISOmetricfine_Enums
#if 0  // needed for Qt's lupdate utility
    qApp->translate("PartDesignGui::TaskHoleParameters", "Counterbore");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Countersink");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Counterdrill");
#endif

TaskHoleParameters::TaskHoleParameters(ViewProviderHole* HoleView, QWidget* parent)
    : TaskSketchBasedParameters(HoleView, parent, "PartDesign_Hole", tr("Hole Parameters"))
    , observer(new Observer(this, getObject<PartDesign::Hole>()))
    , ui(new Ui_TaskHoleParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->ThreadType->addItem(tr("None"), QByteArray("None"));
    ui->ThreadType->addItem(tr("ISO metric regular"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("ISO metric fine"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("UTS coarse"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS fine"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS extra fine"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("ANSI pipes"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("ISO/BSP pipes"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("BSW whitworth"), QByteArray("Other"));
    ui->ThreadType->addItem(tr("BSF whitworth fine"), QByteArray("Other"));
    ui->ThreadType->addItem(tr("ISO tyre valves"), QByteArray("Other"));

    // read values from the hole properties
    auto pcHole = getObject<PartDesign::Hole>();
    bool isNone = std::string(pcHole->ThreadType.getValueAsString()) == "None";
    bool isThreaded = pcHole->Threaded.getValue();

    ui->labelThreading->setHidden(isNone);
    ui->labelHoleType->setHidden(isNone);
    ui->HoleType->setHidden(isNone);
    ui->ThreadSize->setHidden(isNone);
    ui->labelSize->setHidden(isNone);
    ui->ThreadFit->setHidden(isNone || isThreaded);
    ui->labelThreadClearance->setHidden(isNone || isThreaded);

    updateHoleTypeCombo();
    ui->ThreadType->setCurrentIndex(pcHole->ThreadType.getValue());

    ui->ThreadSize->clear();
    std::vector<std::string> cursor = pcHole->ThreadSize.getEnumVector();
    for (const auto& it : cursor) {
        ui->ThreadSize->addItem(tr(it.c_str()));
    }
    ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());

    ui->ThreadClass->clear();
    cursor = pcHole->ThreadClass.getEnumVector();
    for (const auto& it : cursor) {
        ui->ThreadClass->addItem(tr(it.c_str()));
    }
    ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
    // Class is only enabled (sensible) if threaded
    ui->ThreadClass->setEnabled(pcHole->Threaded.getValue());
    ui->ThreadFit->setCurrentIndex(pcHole->ThreadFit.getValue());
    // Fit is only enabled (sensible) if not threaded
    ui->ThreadFit->setEnabled(!pcHole->Threaded.getValue() && pcHole->ThreadType.getValue() != 0L);
    ui->Diameter->setMinimum(pcHole->Diameter.getMinimum());
    ui->Diameter->setValue(pcHole->Diameter.getValue());
    // Diameter is only enabled if ThreadType is None
    if (pcHole->ThreadType.getValue() != 0L) {
        ui->Diameter->setEnabled(false);
    }
    if (pcHole->ThreadDirection.getValue() == 0L) {
        ui->directionRightHand->setChecked(true);
    }
    else {
        ui->directionLeftHand->setChecked(true);
    }
    // ThreadDirection is only sensible if there is a thread
    ui->directionRightHand->setEnabled(pcHole->Threaded.getValue());
    ui->directionLeftHand->setEnabled(pcHole->Threaded.getValue());
    ui->HoleCutType->clear();
    cursor = pcHole->HoleCutType.getEnumVector();
    for (const auto& it : cursor) {
        ui->HoleCutType->addItem(tr(it.c_str()));
    }
    ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());

    ui->HoleCutCustomValues->setChecked(pcHole->HoleCutCustomValues.getValue());
    ui->HoleCutCustomValues->setHidden(pcHole->HoleCutType.getValue() < 4);
    // HoleCutDiameter must not be smaller or equal than the Diameter
    updateHoleCutLimits(pcHole);
    ui->HoleCutDiameter->setValue(pcHole->HoleCutDiameter.getValue());
    ui->HoleCutDiameter->setDisabled(pcHole->HoleCutDiameter.isReadOnly());
    ui->HoleCutDepth->setValue(pcHole->HoleCutDepth.getValue());
    ui->HoleCutDepth->setDisabled(pcHole->HoleCutDepth.isReadOnly());
    ui->HoleCutCountersinkAngle->setMinimum(pcHole->HoleCutCountersinkAngle.getMinimum());
    ui->HoleCutCountersinkAngle->setMaximum(pcHole->HoleCutCountersinkAngle.getMaximum());
    ui->HoleCutCountersinkAngle->setValue(pcHole->HoleCutCountersinkAngle.getValue());
    ui->HoleCutCountersinkAngle->setDisabled(pcHole->HoleCutCountersinkAngle.isReadOnly());

    ui->DepthType->setCurrentIndex(pcHole->DepthType.getValue());
    ui->Depth->setValue(pcHole->Depth.getValue());

    ui->DrillPointAngle->setMinimum(pcHole->DrillPointAngle.getMinimum());
    ui->DrillPointAngle->setMaximum(pcHole->DrillPointAngle.getMaximum());
    ui->DrillPointAngle->setValue(pcHole->DrillPointAngle.getValue());
    ui->DrillForDepth->setChecked(pcHole->DrillForDepth.getValue());

    bool isFlatDrill = pcHole->DrillPoint.getValue() == 0L;
    bool depthIsDimension = std::string(pcHole->DepthType.getValueAsString()) == "Dimension";
    ui->DrillPointAngled->setChecked(!isFlatDrill && depthIsDimension);
    ui->DrillPointAngle->setEnabled(!isFlatDrill && depthIsDimension);
    ui->DrillForDepth->setEnabled(!isFlatDrill && depthIsDimension);

    ui->Tapered->setChecked(pcHole->Tapered.getValue());
    // Angle is only enabled (sensible) if tapered
    ui->TaperedAngle->setEnabled(pcHole->Tapered.getValue());
    ui->TaperedAngle->setMinimum(pcHole->TaperedAngle.getMinimum());
    ui->TaperedAngle->setMaximum(pcHole->TaperedAngle.getMaximum());
    ui->TaperedAngle->setValue(pcHole->TaperedAngle.getValue());
    ui->Reversed->setChecked(pcHole->Reversed.getValue());

    bool isModeled = pcHole->ModelThread.getValue();
    ui->ThreadGroupBox->setVisible(isThreaded);
    ui->UseCustomThreadClearance->setChecked(pcHole->UseCustomThreadClearance.getValue());
    ui->CustomThreadClearance->setValue(pcHole->CustomThreadClearance.getValue());
    ui->ThreadDepthType->setCurrentIndex(pcHole->ThreadDepthType.getValue());
    ui->ThreadDepth->setValue(pcHole->ThreadDepth.getValue());

    ui->CustomClearanceWidget->setVisible(isThreaded && isModeled);
    ui->CustomThreadClearance->setEnabled(ui->UseCustomThreadClearance->isChecked());
    ui->UpdateView->setChecked(false);
    ui->UpdateView->setVisible(isThreaded && isModeled);

    ui->Depth->setEnabled(depthIsDimension);
    ui->ThreadDepthWidget->setVisible(isThreaded && isModeled);

    ui->ThreadDepthDimensionWidget->setVisible(
        std::string(pcHole->ThreadDepthType.getValueAsString()) == "Dimension"
    );

    ui->BaseProfileType->setCurrentIndex(
        PartDesign::Hole::baseProfileOption_bitmaskToIdx(pcHole->BaseProfileType.getValue())
    );

    setCutDiagram();

    // clang-format off
    connect(ui->HoleType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::holeTypeChanged);
    connect(ui->ThreadType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::threadTypeChanged);
    connect(ui->ThreadSize, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::threadSizeChanged);
    connect(ui->ThreadClass, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::threadClassChanged);
    connect(ui->ThreadFit, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::threadFitChanged);
    connect(ui->Diameter, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::threadDiameterChanged);
    connect(ui->directionRightHand, &QRadioButton::clicked,
            this, &TaskHoleParameters::threadDirectionChanged);
    connect(ui->directionLeftHand, &QRadioButton::clicked,
            this, &TaskHoleParameters::threadDirectionChanged);
    connect(ui->HoleCutType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::holeCutTypeChanged);
    connect(ui->HoleCutCustomValues, &QCheckBox::clicked,
            this, &TaskHoleParameters::holeCutCustomValuesChanged);
    connect(ui->HoleCutDiameter, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::holeCutDiameterChanged);
    connect(ui->HoleCutDepth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::holeCutDepthChanged);
    connect(ui->HoleCutCountersinkAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::holeCutCountersinkAngleChanged);
    connect(ui->DepthType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::depthChanged);
    connect(ui->Depth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::depthValueChanged);
    connect(ui->DrillPointAngled, &QCheckBox::toggled,
            this, &TaskHoleParameters::drillPointChanged);
    connect(ui->DrillPointAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::drillPointAngledValueChanged);
    connect(ui->DrillForDepth, &QCheckBox::clicked,
            this, &TaskHoleParameters::drillForDepthChanged);
    connect(ui->Tapered, &QCheckBox::clicked,
            this, &TaskHoleParameters::taperedChanged);
    connect(ui->Reversed, &QCheckBox::clicked,
            this, &TaskHoleParameters::reversedChanged);
    connect(ui->TaperedAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::taperedAngleChanged);
    connect(ui->UpdateView, &QCheckBox::toggled,
            this, &TaskHoleParameters::updateViewChanged);
    connect(ui->UseCustomThreadClearance, &QCheckBox::toggled,
            this, &TaskHoleParameters::useCustomThreadClearanceChanged);
    connect(ui->CustomThreadClearance, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::customThreadClearanceChanged);
    connect(ui->ThreadDepthType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::threadDepthTypeChanged);
    connect(ui->ThreadDepth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskHoleParameters::threadDepthChanged);
    connect(ui->BaseProfileType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskHoleParameters::baseProfileTypeChanged);
    // clang-format on

    ui->Diameter->bind(pcHole->Diameter);
    ui->HoleCutDiameter->bind(pcHole->HoleCutDiameter);
    ui->HoleCutDepth->bind(pcHole->HoleCutDepth);
    ui->HoleCutCountersinkAngle->bind(pcHole->HoleCutCountersinkAngle);
    ui->Depth->bind(pcHole->Depth);
    ui->DrillPointAngle->bind(pcHole->DrillPointAngle);
    ui->TaperedAngle->bind(pcHole->TaperedAngle);
    ui->ThreadDepth->bind(pcHole->ThreadDepth);
    ui->CustomThreadClearance->bind(pcHole->CustomThreadClearance);

    // NOLINTBEGIN
    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(
        std::bind(&TaskHoleParameters::changedObject, this, sp::_1, sp::_2)
    );
    // NOLINTEND

    this->groupLayout()->addWidget(proxy);

    setupGizmos(HoleView);
}

TaskHoleParameters::~TaskHoleParameters() = default;

void TaskHoleParameters::holeTypeChanged(int index)
{
    if (index < 0) {
        return;
    }
    auto pcHole = getObject<PartDesign::Hole>();
    if (!pcHole) {
        return;
    }
    bool isThreaded = getThreaded();
    bool isModeled = getModelThread();

    pcHole->Threaded.setValue(isThreaded);
    pcHole->ModelThread.setValue(isModeled);

    ui->ThreadGroupBox->setVisible(isThreaded);
    // update view not active if modeling threads
    // this will also ensure that the feature is recomputed.
    ui->UpdateView->setVisible(isModeled);
    setUpdateBlocked(isModeled && !(ui->UpdateView->isChecked()));

    // conditional enabling of thread modeling options
    ui->CustomClearanceWidget->setVisible(isModeled);
    ui->CustomThreadClearance->setEnabled(pcHole->UseCustomThreadClearance.getValue());

    ui->ThreadDepthWidget->setVisible(isThreaded && isModeled);
    ui->ThreadDepthDimensionWidget->setVisible(
        std::string(pcHole->ThreadDepthType.getValueAsString()) == "Dimension"
    );

    recomputeFeature();
}

void TaskHoleParameters::updateViewChanged(bool isChecked)
{
    setUpdateBlocked(!isChecked);
    recomputeFeature();
}

void TaskHoleParameters::threadDepthTypeChanged(int index)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadDepthType.setValue(index);
        ui->ThreadDepthDimensionWidget->setVisible(index == 1);
        ui->ThreadDepth->setValue(hole->ThreadDepth.getValue());
        recomputeFeature();
    }
}
void TaskHoleParameters::threadDepthChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadDepth.setValue(value);
        setCutDiagram();
        recomputeFeature();
    }
}

void TaskHoleParameters::useCustomThreadClearanceChanged()
{
    bool isChecked = ui->UseCustomThreadClearance->isChecked();
    ui->CustomThreadClearance->setEnabled(isChecked);
    ui->ThreadClass->setDisabled(isChecked);

    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->UseCustomThreadClearance.setValue(isChecked);
        recomputeFeature();
    }
}

void TaskHoleParameters::customThreadClearanceChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->CustomThreadClearance.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::threadPitchChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadPitch.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::holeCutTypeChanged(int index)
{
    if (index < 0) {
        return;
    }

    auto hole = getObject<PartDesign::Hole>();

    // the HoleCutDepth is something different for countersinks and counterbores
    // therefore reset it, it will be reset to sensible values by setting the new HoleCutType
    hole->HoleCutDepth.setValue(0.0);
    hole->HoleCutType.setValue(index);

    // when holeCutType was changed, reset HoleCutCustomValues to false because it should
    // be a purpose decision to overwrite the normed values
    // we will handle the case that there is no normed value later in this routine
    ui->HoleCutCustomValues->setChecked(false);
    hole->HoleCutCustomValues.setValue(false);

    // recompute to get the info about the HoleCutType properties
    recomputeFeature();

    // apply the result to the widgets
    ui->HoleCutCustomValues->setChecked(hole->HoleCutCustomValues.getValue());

    // HoleCutCustomValues is only enabled for screw definitions
    // we must do this after recomputeFeature() because this gives us the info if
    // the type is a countersink and thus if HoleCutCountersinkAngle can be enabled

    if (hole->HoleCutType.getValue() < 4) {
        ui->HoleCutCustomValues->setHidden(true);
    }
    else {  // screw definition
        // we can have the case that we have no normed values
        // in this case HoleCutCustomValues is read-only AND true
        ui->HoleCutCustomValues->setHidden(false);
        bool isCustom = ui->HoleCutCustomValues->isChecked();
        ui->HoleCutDiameter->setEnabled(isCustom);
        ui->HoleCutDepth->setEnabled(isCustom);
        ui->HoleCutCountersinkAngle->setEnabled(
            isCustom && !hole->HoleCutCountersinkAngle.isReadOnly()
        );
    }
    setCutDiagram();
}
void TaskHoleParameters::baseProfileTypeChanged(int index)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->BaseProfileType.setValue(PartDesign::Hole::baseProfileOption_idxToBitmask(index));
        recomputeFeature();

        setGizmoPositions();
    }
}

void TaskHoleParameters::setCutDiagram()
{
    auto hole = getObject<PartDesign::Hole>();
    const std::string holeCutTypeString = hole->HoleCutType.getValueAsString();
    const std::string threadTypeString = hole->ThreadType.getValueAsString();
    bool isAngled
        = (std::string(hole->DepthType.getValueAsString()) == "Dimension"
           && ui->DrillPointAngled->isChecked());
    bool isCountersink
        = (holeCutTypeString == "Countersink"
           || hole->isDynamicCountersink(threadTypeString, holeCutTypeString));
    bool isCounterbore
        = (holeCutTypeString == "Counterbore"
           || hole->isDynamicCounterbore(threadTypeString, holeCutTypeString));
    bool isCounterdrill = (holeCutTypeString == "Counterdrill");
    bool includeAngle = hole->DrillForDepth.getValue();
    bool isNotCut = holeCutTypeString == "None";

    ui->labelHoleCutDiameter->setHidden(isNotCut);
    ui->labelHoleCutDepth->setHidden(isNotCut);
    ui->HoleCutDiameter->setHidden(isNotCut);
    ui->HoleCutDepth->setHidden(isNotCut);

    std::string baseFileName;
    if (isCounterbore) {
        baseFileName = "hole_counterbore";
        ui->HoleCutCountersinkAngle->setVisible(false);
        ui->labelHoleCutCountersinkAngle->setVisible(false);
    }
    else if (isCountersink) {
        baseFileName = "hole_countersink";
        ui->HoleCutCountersinkAngle->setVisible(true);
        ui->labelHoleCutCountersinkAngle->setVisible(true);
    }
    else if (isCounterdrill) {
        baseFileName = "hole_counterdrill";
        ui->HoleCutCountersinkAngle->setVisible(true);
        ui->labelHoleCutCountersinkAngle->setVisible(true);
    }
    else {
        baseFileName = "hole_none";
        ui->HoleCutCountersinkAngle->setVisible(false);
        ui->labelHoleCutCountersinkAngle->setVisible(false);
    }

    if (isAngled) {
        baseFileName += includeAngle ? "_angled_included" : "_angled";
    }
    else {
        baseFileName += "_flat";
    }

    ui->cutDiagram->setSvg(QString::fromUtf8((":images/" + baseFileName + ".svg").c_str()));
}

void TaskHoleParameters::holeCutCustomValuesChanged()
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->HoleCutCustomValues.setValue(ui->HoleCutCustomValues->isChecked());

        if (ui->HoleCutCustomValues->isChecked()) {
            ui->HoleCutDiameter->setEnabled(true);
            ui->HoleCutDepth->setEnabled(true);
            if (!hole->HoleCutCountersinkAngle.isReadOnly()) {
                ui->HoleCutCountersinkAngle->setEnabled(true);
            }
        }
        else {
            ui->HoleCutDiameter->setEnabled(false);
            ui->HoleCutDepth->setEnabled(false);
            ui->HoleCutCountersinkAngle->setEnabled(false);
        }

        recomputeFeature();
    }
}

void TaskHoleParameters::holeCutDiameterChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->HoleCutDiameter.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::holeCutDepthChanged(double value)
{
    auto hole = getObject<PartDesign::Hole>();
    if (!hole) {
        return;
    }

    std::string HoleCutTypeString = hole->HoleCutType.getValueAsString();

    if (ui->HoleCutCountersinkAngle->isEnabled() && HoleCutTypeString != "Counterdrill") {
        // we have a countersink and recalculate the HoleCutDiameter

        // store current depth
        double DepthDifference = value - hole->HoleCutDepth.getValue();
        // new diameter is the old one + 2 * tan(angle / 2) * DepthDifference
        double newDiameter = hole->HoleCutDiameter.getValue()
            + 2 * tan(Base::toRadians(hole->HoleCutCountersinkAngle.getValue() / 2))
                * DepthDifference;
        // only apply if the result is not smaller than the hole diameter
        if (newDiameter > hole->Diameter.getValue()) {
            hole->HoleCutDiameter.setValue(newDiameter);
            hole->HoleCutDepth.setValue(value);
        }
    }
    else {
        hole->HoleCutDepth.setValue(value);
    }

    recomputeFeature();
}

void TaskHoleParameters::holeCutCountersinkAngleChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->HoleCutCountersinkAngle.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::depthChanged(int index)
{
    auto hole = getObject<PartDesign::Hole>();
    if (!hole) {
        return;
    }
    hole->DepthType.setValue(index);
    recomputeFeature();
    // enabling must be handled after recompute
    bool DepthisDimension = (std::string(hole->DepthType.getValueAsString()) == "Dimension");
    ui->DrillPointAngled->setEnabled(DepthisDimension);
    ui->DrillPointAngle->setEnabled(DepthisDimension);
    ui->DrillForDepth->setEnabled(DepthisDimension);
    setCutDiagram();

    setGizmoPositions();
}

void TaskHoleParameters::depthValueChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->Depth.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::drillPointChanged()
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        bool angled = ui->DrillPointAngled->isChecked();
        hole->DrillPoint.setValue(angled);
        ui->DrillPointAngle->setEnabled(angled);
        ui->DrillForDepth->setEnabled(angled);
        setCutDiagram();
        recomputeFeature();
    }
}

void TaskHoleParameters::drillPointAngledValueChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->DrillPointAngle.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::drillForDepthChanged()
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->DrillForDepth.setValue(ui->DrillForDepth->isChecked());
        recomputeFeature();
    }
    setCutDiagram();
}

void TaskHoleParameters::taperedChanged()
{
    bool checked = ui->Tapered->isChecked();
    ui->TaperedAngle->setEnabled(checked);
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->Tapered.setValue(checked);
        recomputeFeature();
    }
}

void TaskHoleParameters::reversedChanged()
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->Reversed.setValue(ui->Reversed->isChecked());
        recomputeFeature();

        setGizmoPositions();
    }
}

void TaskHoleParameters::taperedAngleChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->TaperedAngle.setValue(value);
        recomputeFeature();
    }
}

void TaskHoleParameters::threadTypeChanged(int index)
{
    if (index < 0) {
        return;
    }

    auto hole = getObject<PartDesign::Hole>();
    if (!hole) {
        return;
    }

    // A typical case is that users change from an ISO profile to another one.
    // When they had e.g. the size "M3" in one profile they expect
    // the same size in the other profile if it exists there.
    // Besides the size also the thread class" and hole cut type are affected.

    // at first check what type class is used
    QByteArray TypeClass = ui->ThreadType->itemData(index).toByteArray();

    // store the current size
    QString ThreadSizeString = ui->ThreadSize->currentText();
    // store the current class
    QString ThreadClassString = ui->ThreadClass->currentText();
    // store the current type
    QString CutTypeString = ui->HoleCutType->currentText();

    // now set the new type, this will reset the comboboxes to item 0
    hole->ThreadType.setValue(index);

    // Threaded checkbox is meaningless if no thread profile is selected.
    bool isNone = std::string(hole->ThreadType.getValueAsString()) == "None";
    bool isThreaded = hole->Threaded.getValue();
    ui->ThreadGroupBox->setHidden(isNone || !isThreaded);
    ui->ThreadSize->setHidden(isNone);
    ui->labelSize->setHidden(isNone);
    ui->labelThreading->setHidden(isNone);
    ui->labelHoleType->setHidden(isNone);
    ui->HoleType->setHidden(isNone);
    ui->ThreadFit->setHidden(isNone || isThreaded);
    ui->labelThreadClearance->setHidden(isNone || isThreaded);

    if (TypeClass == QByteArray("None")) {
        QString noneText = QStringLiteral("-");
        ui->ThreadFit->setItemText(0, noneText);
        ui->ThreadFit->setItemText(1, noneText);
        ui->ThreadFit->setItemText(2, noneText);
    }
    else if (TypeClass == QByteArray("ISO")) {
        ui->ThreadFit->setItemText(
            0,
            tr("Medium", "Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible")
        );
        ui->ThreadFit->setItemText(
            1,
            tr("Fine", "Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible")
        );
        ui->ThreadFit->setItemText(
            2,
            tr("Coarse", "Distance between thread crest and hole wall, use ISO-273 nomenclature or equivalent if possible")
        );
    }
    else if (TypeClass == QByteArray("UTS")) {
        ui->ThreadFit->setItemText(
            0,
            tr("Normal", "Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible")
        );
        ui->ThreadFit->setItemText(
            1,
            tr("Close", "Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible")
        );
        ui->ThreadFit->setItemText(
            2,
            tr("Loose", "Distance between thread crest and hole wall, use ASME B18.2.8 nomenclature or equivalent if possible")
        );
    }
    else {
        ui->ThreadFit->setItemText(0, tr("Normal", "Distance between thread crest and hole wall"));
        ui->ThreadFit->setItemText(1, tr("Close", "Distance between thread crest and hole wall"));
        ui->ThreadFit->setItemText(2, tr("Wide", "Distance between thread crest and hole wall"));
    }

    // Class and cut type
    // the class and cut types are the same for both TypeClass so we don't need to distinguish
    // between ISO and UTS
    int threadClassIndex = ui->ThreadClass->findText(ThreadClassString, Qt::MatchContains);
    if (threadClassIndex > -1) {
        ui->ThreadClass->setCurrentIndex(threadClassIndex);
    }
    int holeCutIndex = ui->HoleCutType->findText(CutTypeString, Qt::MatchContains);
    if (holeCutIndex > -1) {
        ui->HoleCutType->setCurrentIndex(holeCutIndex);
    }

    // we must set the read-only state according to the new HoleCutType
    holeCutTypeChanged(ui->HoleCutType->currentIndex());

    recomputeFeature();
}

void TaskHoleParameters::threadSizeChanged(int index)
{
    if (index < 0) {
        return;
    }

    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadSize.setValue(index);
        recomputeFeature();

        // apply the recompute result to the widgets
        ui->HoleCutCustomValues->setDisabled(hole->HoleCutCustomValues.isReadOnly());
        ui->HoleCutCustomValues->setChecked(hole->HoleCutCustomValues.getValue());
    }
}

void TaskHoleParameters::threadClassChanged(int index)
{
    if (index < 0) {
        return;
    }

    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadClass.setValue(index);
        recomputeFeature();
    }
}

void TaskHoleParameters::threadDiameterChanged(double value)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->Diameter.setValue(value);

        updateHoleCutLimits(hole);

        recomputeFeature();
    }
}

void TaskHoleParameters::threadFitChanged(int index)
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        hole->ThreadFit.setValue(index);
        recomputeFeature();
    }
}

void TaskHoleParameters::threadDirectionChanged()
{
    if (auto hole = getObject<PartDesign::Hole>()) {
        if (sender() == ui->directionRightHand) {
            hole->ThreadDirection.setValue(0L);
        }
        else {
            hole->ThreadDirection.setValue(1L);
        }
        recomputeFeature();
    }
}

void TaskHoleParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskHoleParameters::changedObject(const App::Document&, const App::Property& Prop)
{
    auto hole = getObject<PartDesign::Hole>();
    if (!hole) {
        return;  // happens when aborting the command
    }
    bool ro = Prop.isReadOnly();

    Base::Console().log("Parameter %s was updated\n", Prop.getName());

    auto updateCheckable = [&](QCheckBox* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(ro);
    };

    auto updateRadio = [&](QRadioButton* widget, bool value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setChecked(value);
        widget->setDisabled(ro);
    };

    auto updateComboBox = [&](QComboBox* widget, int value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setCurrentIndex(value);
        widget->setDisabled(ro);
    };

    auto updateSpinBox = [&](Gui::PrefQuantitySpinBox* widget, double value) {
        [[maybe_unused]] QSignalBlocker blocker(widget);
        widget->setValue(value);
        widget->setDisabled(ro);
    };

    if (&Prop == &hole->Threaded || &Prop == &hole->ModelThread) {
        updateHoleTypeCombo();
    }
    else if (&Prop == &hole->ThreadType) {
        ui->ThreadType->setEnabled(true);
        updateComboBox(ui->ThreadType, hole->ThreadType.getValue());

        // Thread type also updates related properties
        auto updateComboBoxItems = [&](QComboBox* widget, const auto& values, int selected) {
            QSignalBlocker blocker(widget);
            widget->clear();
            for (const auto& it : values) {
                widget->addItem(QString::fromStdString(it));
            }
            widget->setCurrentIndex(selected);
        };

        updateComboBoxItems(
            ui->ThreadSize,
            hole->ThreadSize.getEnumVector(),
            hole->ThreadSize.getValue()
        );

        std::vector<std::string> translatedCutTypes;
        for (const auto& it : hole->HoleCutType.getEnumVector()) {
            translatedCutTypes.push_back(tr(it.c_str()).toStdString());
        }
        updateComboBoxItems(ui->HoleCutType, translatedCutTypes, hole->HoleCutType.getValue());

        std::vector<std::string> translatedClassTypes;
        for (const auto& it : hole->ThreadClass.getEnumVector()) {
            translatedClassTypes.push_back(tr(it.c_str()).toStdString());
        }
        updateComboBoxItems(ui->ThreadClass, translatedClassTypes, hole->ThreadClass.getValue());
    }
    else if (&Prop == &hole->ThreadSize) {
        ui->ThreadSize->setEnabled(true);
        updateComboBox(ui->ThreadSize, hole->ThreadSize.getValue());
    }
    else if (&Prop == &hole->ThreadClass) {
        ui->ThreadClass->setEnabled(true);
        updateComboBox(ui->ThreadClass, hole->ThreadClass.getValue());
    }
    else if (&Prop == &hole->ThreadFit) {
        ui->ThreadFit->setEnabled(true);
        updateComboBox(ui->ThreadFit, hole->ThreadFit.getValue());
    }
    else if (&Prop == &hole->Diameter) {
        ui->Diameter->setEnabled(true);
        updateSpinBox(ui->Diameter, hole->Diameter.getValue());
        updateHoleCutLimits(hole);
    }
    else if (&Prop == &hole->ThreadDirection) {
        ui->directionRightHand->setEnabled(true);
        ui->directionLeftHand->setEnabled(true);

        std::string direction(hole->ThreadDirection.getValueAsString());
        updateRadio(ui->directionRightHand, direction == "Right");
        updateRadio(ui->directionLeftHand, direction == "Left");
    }
    else if (&Prop == &hole->HoleCutType) {
        ui->HoleCutType->setEnabled(true);
        updateComboBox(ui->HoleCutType, hole->HoleCutType.getValue());
    }
    else if (&Prop == &hole->HoleCutDiameter) {
        ui->HoleCutDiameter->setEnabled(true);
        updateSpinBox(ui->HoleCutDiameter, hole->HoleCutDiameter.getValue());
    }
    else if (&Prop == &hole->HoleCutDepth) {
        ui->HoleCutDepth->setEnabled(true);
        updateSpinBox(ui->HoleCutDepth, hole->HoleCutDepth.getValue());
    }
    else if (&Prop == &hole->HoleCutCountersinkAngle) {
        ui->HoleCutCountersinkAngle->setEnabled(true);
        updateSpinBox(ui->HoleCutCountersinkAngle, hole->HoleCutCountersinkAngle.getValue());
    }
    else if (&Prop == &hole->DepthType) {
        ui->DepthType->setEnabled(true);
        updateComboBox(ui->DepthType, hole->DepthType.getValue());
    }
    else if (&Prop == &hole->Depth) {
        ui->Depth->setEnabled(true);
        updateSpinBox(ui->Depth, hole->Depth.getValue());
    }
    else if (&Prop == &hole->DrillPoint) {
        ui->DrillPointAngled->setEnabled(true);
        updateCheckable(
            ui->DrillPointAngled,
            hole->DrillPoint.getValueAsString() == std::string("Angled")
        );
    }
    else if (&Prop == &hole->DrillPointAngle) {
        ui->DrillPointAngle->setEnabled(true);
        updateSpinBox(ui->DrillPointAngle, hole->DrillPointAngle.getValue());
    }
    else if (&Prop == &hole->DrillForDepth) {
        ui->DrillForDepth->setEnabled(true);
        updateCheckable(ui->DrillForDepth, hole->DrillForDepth.getValue());
    }
    else if (&Prop == &hole->Tapered) {
        ui->Tapered->setEnabled(true);
        updateCheckable(ui->Tapered, hole->Tapered.getValue());
    }
    else if (&Prop == &hole->TaperedAngle) {
        ui->TaperedAngle->setEnabled(true);
        updateSpinBox(ui->TaperedAngle, hole->TaperedAngle.getValue());
    }
    else if (&Prop == &hole->UseCustomThreadClearance) {
        ui->UseCustomThreadClearance->setEnabled(true);
        updateCheckable(ui->UseCustomThreadClearance, hole->UseCustomThreadClearance.getValue());
    }
    else if (&Prop == &hole->CustomThreadClearance) {
        ui->CustomThreadClearance->setEnabled(true);
        updateSpinBox(ui->CustomThreadClearance, hole->CustomThreadClearance.getValue());
    }
    else if (&Prop == &hole->ThreadDepthType) {
        ui->ThreadDepthType->setEnabled(true);
        updateComboBox(ui->ThreadDepthType, hole->ThreadDepthType.getValue());
    }
    else if (&Prop == &hole->ThreadDepth) {
        ui->ThreadDepth->setEnabled(true);
        updateSpinBox(ui->ThreadDepth, hole->ThreadDepth.getValue());
    }
    else if (&Prop == &hole->BaseProfileType) {
        ui->BaseProfileType->setEnabled(true);
        updateComboBox(
            ui->BaseProfileType,
            PartDesign::Hole::baseProfileOption_bitmaskToIdx(hole->BaseProfileType.getValue())
        );
    }
}

void TaskHoleParameters::updateHoleTypeCombo()
{
    auto hole = getObject<PartDesign::Hole>();
    if (!hole) {
        return;
    }
    [[maybe_unused]] QSignalBlocker blocker(ui->HoleType);
    if (hole->Threaded.getValue()) {
        if (hole->ModelThread.getValue()) {
            ui->HoleType->setCurrentIndex(ModeledThread);
        }
        else {
            ui->HoleType->setCurrentIndex(TapDrill);
        }
    }
    else {
        ui->HoleType->setCurrentIndex(Clearance);
    }
}

void TaskHoleParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Q_UNUSED(msg)
}

bool TaskHoleParameters::getThreaded() const
{
    return ui->HoleType->currentIndex() != Clearance;
}

bool TaskHoleParameters::getModelThread() const
{
    return ui->HoleType->currentIndex() == ModeledThread;
}

long TaskHoleParameters::getThreadType() const
{
    return ui->ThreadType->currentIndex();
}

long TaskHoleParameters::getThreadSize() const
{
    if (ui->ThreadSize->currentIndex() == -1) {
        return 0;
    }
    else {
        return ui->ThreadSize->currentIndex();
    }
}

long TaskHoleParameters::getThreadClass() const
{
    if (ui->ThreadSize->currentIndex() == -1) {
        return 0;
    }
    else {
        return ui->ThreadClass->currentIndex();
    }
}

long TaskHoleParameters::getThreadFit() const
{
    // the fit (clearance) is independent if the hole is threaded or not
    // since an unthreaded hole for a screw can also have a close fit
    return ui->ThreadFit->currentIndex();
}

Base::Quantity TaskHoleParameters::getDiameter() const
{
    return ui->Diameter->value();
}

long TaskHoleParameters::getThreadDirection() const
{
    if (ui->directionRightHand->isChecked()) {
        return 0;
    }
    else {
        return 1;
    }
}

long TaskHoleParameters::getHoleCutType() const
{
    if (ui->HoleCutType->currentIndex() == -1) {
        return 0;
    }
    else {
        return ui->HoleCutType->currentIndex();
    }
}

bool TaskHoleParameters::getHoleCutCustomValues() const
{
    return ui->HoleCutCustomValues->isChecked();
}

Base::Quantity TaskHoleParameters::getHoleCutDiameter() const
{
    return ui->HoleCutDiameter->value();
}

Base::Quantity TaskHoleParameters::getHoleCutDepth() const
{
    return ui->HoleCutDepth->value();
}

Base::Quantity TaskHoleParameters::getHoleCutCountersinkAngle() const
{
    return ui->HoleCutCountersinkAngle->value();
}

long TaskHoleParameters::getDepthType() const
{
    return ui->DepthType->currentIndex();
}

Base::Quantity TaskHoleParameters::getDepth() const
{
    return ui->Depth->value();
}

long TaskHoleParameters::getDrillPoint() const
{
    return ui->DrillPointAngled->isChecked() ? 1 : 0;
}

Base::Quantity TaskHoleParameters::getDrillPointAngle() const
{
    return ui->DrillPointAngle->value();
}

bool TaskHoleParameters::getTapered() const
{
    return ui->Tapered->isChecked();
}

bool TaskHoleParameters::getDrillForDepth() const
{
    return ui->DrillForDepth->isChecked();
}

Base::Quantity TaskHoleParameters::getTaperedAngle() const
{
    return ui->TaperedAngle->value();
}

bool TaskHoleParameters::getUseCustomThreadClearance() const
{
    return ui->UseCustomThreadClearance->isChecked();
}

double TaskHoleParameters::getCustomThreadClearance() const
{
    return ui->CustomThreadClearance->value().getValue();
}

long TaskHoleParameters::getThreadDepthType() const
{
    return ui->ThreadDepthType->currentIndex();
}

double TaskHoleParameters::getThreadDepth() const
{
    return ui->ThreadDepth->value().getValue();
}
int TaskHoleParameters::getBaseProfileType() const
{
    return PartDesign::Hole::baseProfileOption_idxToBitmask(ui->BaseProfileType->currentIndex());
}
void TaskHoleParameters::apply()
{
    auto hole = getObject<PartDesign::Hole>();

    ui->Diameter->apply();
    ui->HoleCutDiameter->apply();
    ui->HoleCutDepth->apply();
    ui->HoleCutCountersinkAngle->apply();
    ui->Depth->apply();
    ui->DrillPointAngle->apply();
    ui->TaperedAngle->apply();

    if (!hole->Threaded.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "Threaded = " << (getThreaded() ? 1 : 0));
    }
    if (!hole->ModelThread.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ModelThread = " << (getModelThread() ? 1 : 0));
    }
    if (!hole->ThreadDepthType.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadDepthType = " << getThreadDepthType());
    }
    if (!hole->ThreadDepth.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadDepth = " << getThreadDepth());
    }
    if (!hole->UseCustomThreadClearance.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "UseCustomThreadClearance = " << (getUseCustomThreadClearance() ? 1 : 0));
    }
    if (!hole->CustomThreadClearance.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "CustomThreadClearance = " << getCustomThreadClearance());
    }
    if (!hole->ThreadType.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadType = " << getThreadType());
    }
    if (!hole->ThreadSize.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadSize = " << getThreadSize());
    }
    if (!hole->ThreadClass.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadClass = " << getThreadClass());
    }
    if (!hole->ThreadFit.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadFit = " << getThreadFit());
    }
    if (!hole->ThreadDirection.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "ThreadDirection = " << getThreadDirection());
    }
    if (!hole->HoleCutType.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "HoleCutType = " << getHoleCutType());
    }
    if (!hole->HoleCutCustomValues.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "HoleCutCustomValues = " << (getHoleCutCustomValues() ? 1 : 0));
    }
    if (!hole->DepthType.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "DepthType = " << getDepthType());
    }
    if (!hole->DrillPoint.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "DrillPoint = " << getDrillPoint());
    }
    if (!hole->DrillForDepth.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "DrillForDepth = " << (getDrillForDepth() ? 1 : 0));
    }
    if (!hole->Tapered.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "Tapered = " << getTapered());
    }
    if (!hole->BaseProfileType.isReadOnly()) {
        FCMD_OBJ_CMD(hole, "BaseProfileType = " << getBaseProfileType());
    }
}

void TaskHoleParameters::updateHoleCutLimits(PartDesign::Hole* hole)
{
    constexpr double minHoleCutDifference = 0.1;
    // HoleCutDiameter must not be smaller or equal than the Diameter
    ui->HoleCutDiameter->setMinimum(hole->Diameter.getValue() + minHoleCutDifference);
}

void TaskHoleParameters::setupGizmos(ViewProviderHole* vp)
{
    if (!GizmoContainer::isEnabled()) {
        return;
    }

    holeDepthGizmo = new LinearGizmo(ui->Depth);

    gizmoContainer = GizmoContainer::create({holeDepthGizmo}, vp);

    setGizmoPositions();
}

std::vector<Base::Vector3d> getHolePositionFromShape(
    const Part::TopoShape& profileshape,
    const long baseProfileType
)
{
    using BaseProfileTypeOptions = PartDesign::Hole::BaseProfileTypeOptions;

    std::vector<Base::Vector3d> positions;

    // Iterate over edges and filter out non-circle/non-arc types
    if (baseProfileType & BaseProfileTypeOptions::OnCircles
        || baseProfileType & BaseProfileTypeOptions::OnArcs) {
        for (const auto& profileEdge : profileshape.getSubTopoShapes(TopAbs_EDGE)) {
            TopoDS_Edge edge = TopoDS::Edge(profileEdge.getShape());
            BRepAdaptor_Curve adaptor(edge);

            // Circle base?
            if (adaptor.GetType() != GeomAbs_Circle) {
                continue;
            }
            // Filter for circles
            if (!(baseProfileType & BaseProfileTypeOptions::OnCircles) && adaptor.IsClosed()) {
                continue;
            }

            // Filter for arcs
            if (!(baseProfileType & BaseProfileTypeOptions::OnArcs) && !adaptor.IsClosed()) {
                continue;
            }

            gp_Circ circle = adaptor.Circle();
            positions.push_back(Base::convertTo<Base::Vector3d>(circle.Axis().Location()));
        }
    }

    // To avoid breaking older files which where not made with
    // holes on points
    if (baseProfileType & BaseProfileTypeOptions::OnPoints) {
        // Iterate over vertices while avoiding edges so that curve handles are ignored
        for (const auto& profileVertex : profileshape.getSubTopoShapes(TopAbs_VERTEX, TopAbs_EDGE)) {
            TopoDS_Vertex vertex = TopoDS::Vertex(profileVertex.getShape());
            positions.push_back(Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(vertex)));
        }
    }

    return positions;
}

void TaskHoleParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    auto hole = getObject<PartDesign::Hole>();
    if (!hole || hole->isError()) {
        gizmoContainer->visible = false;
        return;
    }
    Part::TopoShape profileShape = hole->getProfileShape(
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
        | Part::ShapeOption::Transform | Part::ShapeOption::DontSimplifyCompound
    );
    Base::Vector3d dir = hole->guessNormalDirection(profileShape);
    dir *= hole->Reversed.getValue() ? -1 : 1;
    std::vector<Base::Vector3d> holePositions
        = getHolePositionFromShape(profileShape, hole->BaseProfileType.getValue());

    if (holePositions.size() == 0) {
        gizmoContainer->visible = false;
        return;
    }
    gizmoContainer->visible = true;

    holeDepthGizmo->Gizmo::setDraggerPlacement(
        holePositions[0] - ui->HoleCutDepth->value().getValue() * dir,
        -dir
    );
    holeDepthGizmo->setVisibility(std::string(hole->DepthType.getValueAsString()) == "Dimension");

    holeDepthGizmo->setDragLength(ui->Depth->rawValue());
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgHoleParameters::TaskDlgHoleParameters(ViewProviderHole* HoleView)
    : TaskDlgSketchBasedParameters(HoleView)
{
    assert(HoleView);
    parameter = new TaskHoleParameters(HoleView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgHoleParameters::~TaskDlgHoleParameters() = default;

#include "moc_TaskHoleParameters.cpp"

TaskHoleParameters::Observer::Observer(TaskHoleParameters* _owner, PartDesign::Hole* _hole)
    : DocumentObserver(_hole->getDocument())
    , owner(_owner)
    , hole(_hole)
{}

void TaskHoleParameters::Observer::slotChangedObject(
    const App::DocumentObject& Obj,
    const App::Property& Prop
)
{
    if (&Obj == hole) {
        Base::Console().log("Parameter %s was updated with a new value\n", Prop.getName());
        if (Obj.getDocument()) {
            owner->changedObject(*Obj.getDocument(), Prop);
        }
    }
}
