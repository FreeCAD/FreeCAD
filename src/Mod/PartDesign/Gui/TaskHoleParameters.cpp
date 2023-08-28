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

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureHole.h>

#include "ui_TaskHoleParameters.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;
using namespace Gui;
namespace sp = std::placeholders;

/* TRANSLATOR PartDesignGui::TaskHoleParameters */

// See Hole::HoleCutType_ISOmetric_Enums
// and Hole::HoleCutType_ISOmetricfine_Enums
#if 0 // needed for Qt's lupdate utility
    qApp->translate("PartDesignGui::TaskHoleParameters", "Counterbore");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Countersink");
    qApp->translate("PartDesignGui::TaskHoleParameters", "Counterdrill");
#endif

TaskHoleParameters::TaskHoleParameters(ViewProviderHole* HoleView, QWidget* parent)
    : TaskSketchBasedParameters(HoleView, parent, "PartDesign_Hole", tr("Hole parameters"))
    , observer(new Observer(this, static_cast<PartDesign::Hole*>(vp->getObject())))
    , isApplying(false)
    , ui(new Ui_TaskHoleParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    ui->ThreadType->addItem(tr("None"), QByteArray("None"));
    ui->ThreadType->addItem(tr("ISO metric regular profile"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("ISO metric fine profile"), QByteArray("ISO"));
    ui->ThreadType->addItem(tr("UTS coarse profile"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS fine profile"), QByteArray("UTS"));
    ui->ThreadType->addItem(tr("UTS extra fine profile"), QByteArray("UTS"));

    // read values from the hole properties
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    ui->Threaded->setChecked(pcHole->Threaded.getValue());
    ui->Threaded->setDisabled(std::string(pcHole->ThreadType.getValueAsString()) == "None");

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
    ui->ThreadFit->setEnabled(!pcHole->Threaded.getValue());
    ui->Diameter->setMinimum(pcHole->Diameter.getMinimum());
    ui->Diameter->setValue(pcHole->Diameter.getValue());
    // Diameter is only enabled if ThreadType is None
    if (pcHole->ThreadType.getValue() != 0L)
        ui->Diameter->setEnabled(false);
    if (pcHole->ThreadDirection.getValue() == 0L)
        ui->directionRightHand->setChecked(true);
    else
        ui->directionLeftHand->setChecked(true);
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
    ui->HoleCutCustomValues->setDisabled(pcHole->HoleCutCustomValues.isReadOnly());
    // HoleCutDiameter must not be smaller or equal than the Diameter
    ui->HoleCutDiameter->setMinimum(pcHole->Diameter.getValue() + 0.1);
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
    if (pcHole->DrillPoint.getValue() == 0L)
        ui->drillPointFlat->setChecked(true);
    else
        ui->drillPointAngled->setChecked(true);
    ui->DrillPointAngle->setMinimum(pcHole->DrillPointAngle.getMinimum());
    ui->DrillPointAngle->setMaximum(pcHole->DrillPointAngle.getMaximum());
    ui->DrillPointAngle->setValue(pcHole->DrillPointAngle.getValue());
    ui->DrillForDepth->setChecked(pcHole->DrillForDepth.getValue());
    // drill point settings are only enabled (sensible) if type is 'Dimension'
    if (std::string(pcHole->DepthType.getValueAsString()) == "Dimension") {
        ui->drillPointFlat->setEnabled(true);
        ui->drillPointAngled->setEnabled(true);
        ui->DrillPointAngle->setEnabled(true);
        ui->DrillForDepth->setEnabled(true);
    }
    else {
        ui->drillPointFlat->setEnabled(false);
        ui->drillPointAngled->setEnabled(false);
        ui->DrillPointAngle->setEnabled(false);
        ui->DrillForDepth->setEnabled(false);
    }
    // drill point is sensible but flat, disable angle and option
    if (!ui->drillPointFlat->isChecked()) {
        ui->DrillPointAngle->setEnabled(true);
        ui->DrillForDepth->setEnabled(true);
    }
    else {
        ui->DrillPointAngle->setEnabled(false);
        ui->DrillForDepth->setEnabled(false);
    }
    ui->Tapered->setChecked(pcHole->Tapered.getValue());
    // Angle is only enabled (sensible) if tapered
    ui->TaperedAngle->setEnabled(pcHole->Tapered.getValue());
    ui->TaperedAngle->setMinimum(pcHole->TaperedAngle.getMinimum());
    ui->TaperedAngle->setMaximum(pcHole->TaperedAngle.getMaximum());
    ui->TaperedAngle->setValue(pcHole->TaperedAngle.getValue());
    ui->Reversed->setChecked(pcHole->Reversed.getValue());

    ui->ModelThread->setChecked(pcHole->ModelThread.getValue());
    ui->UseCustomThreadClearance->setChecked(pcHole->UseCustomThreadClearance.getValue());
    ui->CustomThreadClearance->setValue(pcHole->CustomThreadClearance.getValue());
    ui->ThreadDepthType->setCurrentIndex(pcHole->ThreadDepthType.getValue());
    ui->ThreadDepth->setValue(pcHole->ThreadDepth.getValue());

    // conditional enabling of thread modeling options
    ui->ModelThread->setEnabled(ui->Threaded->isChecked() && ui->ThreadType->currentIndex() != 0);
    ui->UseCustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    ui->CustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked()
        && ui->UseCustomThreadClearance->isChecked());
    ui->UpdateView->setChecked(false);
    ui->UpdateView->setEnabled(ui->ModelThread->isChecked());

    ui->Depth->setEnabled(std::string(pcHole->DepthType.getValueAsString()) == "Dimension");
    ui->ThreadDepthType->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    ui->ThreadDepth->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked()
        && std::string(pcHole->ThreadDepthType.getValueAsString()) == "Dimension");

    connect(ui->Threaded, &QCheckBox::clicked, this, &TaskHoleParameters::threadedChanged);
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
    connect(ui->drillPointFlat, &QRadioButton::clicked,
            this, &TaskHoleParameters::drillPointChanged);
    connect(ui->drillPointAngled, &QRadioButton::clicked,
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
    connect(ui->ModelThread, &QCheckBox::clicked,
            this, &TaskHoleParameters::modelThreadChanged);
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

    vp->show();

    ui->Diameter->bind(pcHole->Diameter);
    ui->HoleCutDiameter->bind(pcHole->HoleCutDiameter);
    ui->HoleCutDepth->bind(pcHole->HoleCutDepth);
    ui->HoleCutCountersinkAngle->bind(pcHole->HoleCutCountersinkAngle);
    ui->Depth->bind(pcHole->Depth);
    ui->DrillPointAngle->bind(pcHole->DrillPointAngle);
    ui->TaperedAngle->bind(pcHole->TaperedAngle);
    ui->ThreadDepth->bind(pcHole->ThreadDepth);
    ui->CustomThreadClearance->bind(pcHole->CustomThreadClearance);

    //NOLINTBEGIN
    connectPropChanged = App::GetApplication().signalChangePropertyEditor.connect(
            std::bind(&TaskHoleParameters::changedObject, this, sp::_1, sp::_2));
    //NOLINTEND

    this->groupLayout()->addWidget(proxy);
}

TaskHoleParameters::~TaskHoleParameters() = default;

void TaskHoleParameters::threadedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    bool isChecked = ui->Threaded->isChecked();
    pcHole->Threaded.setValue(isChecked);

    ui->ModelThread->setEnabled(isChecked);
    ui->ThreadDepthType->setEnabled(isChecked);

    // conditional enabling of thread modeling options
    ui->UseCustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    ui->CustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked() && ui->UseCustomThreadClearance->isChecked());


    // update view not active if modeling threads
    // this will also ensure that the feature is recomputed.
    ui->UpdateView->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    blockUpdate = ui->Threaded->isChecked() && ui->ModelThread->isChecked() && !(ui->UpdateView->isChecked());

    pcHole->Threaded.setValue(ui->Threaded->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::modelThreadChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ModelThread.setValue(ui->ModelThread->isChecked());

    // update view not active if modeling threads
    // this will also ensure that the feature is recomputed.
    ui->UpdateView->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    blockUpdate = ui->Threaded->isChecked() && ui->ModelThread->isChecked() && !(ui->UpdateView->isChecked());

    // conditional enabling of thread modeling options
    ui->UseCustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    ui->CustomThreadClearance->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked() && ui->UseCustomThreadClearance->isChecked());

    ui->ThreadDepthType->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked());
    ui->ThreadDepth->setEnabled(ui->Threaded->isChecked() && ui->ModelThread->isChecked() && std::string(pcHole->ThreadDepthType.getValueAsString()) == "Dimension");

    recomputeFeature();
}

void TaskHoleParameters::updateViewChanged(bool isChecked)
{
    blockUpdate = !isChecked;
    recomputeFeature();
}

void TaskHoleParameters::threadDepthTypeChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadDepthType.setValue(index);
    ui->ThreadDepth->setEnabled(index == 1);
    ui->ThreadDepth->setValue(pcHole->ThreadDepth.getValue());
    recomputeFeature();
}

void TaskHoleParameters::threadDepthChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadDepth.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::useCustomThreadClearanceChanged()
{
    bool isChecked = ui->UseCustomThreadClearance->isChecked();
    ui->CustomThreadClearance->setEnabled(isChecked);
    ui->ThreadClass->setDisabled(isChecked);

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->UseCustomThreadClearance.setValue(isChecked);
    recomputeFeature();
}

void TaskHoleParameters::customThreadClearanceChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->CustomThreadClearance.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadPitchChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadPitch.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::holeCutTypeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    // the HoleCutDepth is something different for countersinks and counterbores
    // therefore reset it, it will be reset to sensible values by setting the new HoleCutType
    pcHole->HoleCutDepth.setValue(0.0);
    pcHole->HoleCutType.setValue(index);

    // when holeCutType was changed, reset HoleCutCustomValues to false because it should
    // be a purpose decision to overwrite the normed values
    // we will handle the case that there is no normed value later in this routine
    ui->HoleCutCustomValues->setChecked(false);
    pcHole->HoleCutCustomValues.setValue(false);

    // recompute to get the info about the HoleCutType properties
    recomputeFeature();

    // apply the result to the widgets
    ui->HoleCutCustomValues->setDisabled(pcHole->HoleCutCustomValues.isReadOnly());
    ui->HoleCutCustomValues->setChecked(pcHole->HoleCutCustomValues.getValue());

    // HoleCutCustomValues is only enabled for screw definitions
    // we must do this after recomputeFeature() because this gives us the info if
    // the type is a countersink and thus if HoleCutCountersinkAngle can be enabled
    std::string HoleCutTypeString = pcHole->HoleCutType.getValueAsString();
    if (HoleCutTypeString == "None" || HoleCutTypeString == "Counterbore"
        || HoleCutTypeString == "Countersink" || HoleCutTypeString == "Counterdrill") {
        ui->HoleCutCustomValues->setEnabled(false);
        if (HoleCutTypeString == "None") {
            ui->HoleCutDiameter->setEnabled(false);
            ui->HoleCutDepth->setEnabled(false);
            ui->HoleCutCountersinkAngle->setEnabled(false);
        }
        if (HoleCutTypeString == "Counterbore")
            ui->HoleCutCountersinkAngle->setEnabled(false);
        if (HoleCutTypeString == "Countersink")
            ui->HoleCutCountersinkAngle->setEnabled(true);
    }
    else { // screw definition
        // we can have the case that we have no normed values
        // in this case HoleCutCustomValues is read-only AND true
        if (ui->HoleCutCustomValues->isChecked()) {
            ui->HoleCutDiameter->setEnabled(true);
            ui->HoleCutDepth->setEnabled(true);
            if (!pcHole->HoleCutCountersinkAngle.isReadOnly())
                ui->HoleCutCountersinkAngle->setEnabled(true);
        }
        else {
            ui->HoleCutCustomValues->setEnabled(true);
            ui->HoleCutDiameter->setEnabled(false);
            ui->HoleCutDepth->setEnabled(false);
            ui->HoleCutCountersinkAngle->setEnabled(false);
        }
    }
}

void TaskHoleParameters::holeCutCustomValuesChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutCustomValues.setValue(ui->HoleCutCustomValues->isChecked());

    if (ui->HoleCutCustomValues->isChecked()) {
        ui->HoleCutDiameter->setEnabled(true);
        ui->HoleCutDepth->setEnabled(true);
        if (!pcHole->HoleCutCountersinkAngle.isReadOnly())
            ui->HoleCutCountersinkAngle->setEnabled(true);
    }
    else {
        ui->HoleCutDiameter->setEnabled(false);
        ui->HoleCutDepth->setEnabled(false);
        ui->HoleCutCountersinkAngle->setEnabled(false);
    }

    recomputeFeature();
}

void TaskHoleParameters::holeCutDiameterChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutDiameter.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::holeCutDepthChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    std::string HoleCutTypeString = pcHole->HoleCutType.getValueAsString();

    if (ui->HoleCutCountersinkAngle->isEnabled() && HoleCutTypeString != "Counterdrill") {
        // we have a countersink and recalculate the HoleCutDiameter

        // store current depth
        double DepthDifference = value - pcHole->HoleCutDepth.getValue();
        // new diameter is the old one + 2 * tan(angle / 2) * DepthDifference
        double newDiameter = pcHole->HoleCutDiameter.getValue()
            + 2 * tan(Base::toRadians(pcHole->HoleCutCountersinkAngle.getValue() / 2)) * DepthDifference;
        // only apply if the result is not smaller than the hole diameter
        if (newDiameter > pcHole->Diameter.getValue()) {
            pcHole->HoleCutDiameter.setValue(newDiameter);
            pcHole->HoleCutDepth.setValue(value);
        }
    }
    else {
        pcHole->HoleCutDepth.setValue(value);
    }

    recomputeFeature();
}

void TaskHoleParameters::holeCutCountersinkAngleChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->HoleCutCountersinkAngle.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::depthChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DepthType.setValue(index);

    // disable drill point widgets if not 'Dimension'
    if (std::string(pcHole->DepthType.getValueAsString()) == "Dimension") {
        ui->drillPointFlat->setEnabled(true);
        ui->drillPointAngled->setEnabled(true);
        ui->DrillPointAngle->setEnabled(true);
        ui->DrillForDepth->setEnabled(true);
    }
    else { // through all
        ui->drillPointFlat->setEnabled(false);
        ui->drillPointAngled->setEnabled(false);
        ui->DrillPointAngle->setEnabled(false);
        ui->DrillForDepth->setEnabled(false);
    }
    recomputeFeature();
    // enabling must be handled after recompute
    ui->ThreadDepth->setEnabled(std::string(pcHole->ThreadDepthType.getValueAsString()) == "Dimension");
}

void TaskHoleParameters::depthValueChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Depth.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::drillPointChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    if (sender() == ui->drillPointFlat) {
        pcHole->DrillPoint.setValue((long)0);
        ui->DrillForDepth->setEnabled(false);
    }
    else if (sender() == ui->drillPointAngled) {
        pcHole->DrillPoint.setValue((long)1);
        ui->DrillForDepth->setEnabled(true);
    }
    else {
        assert(0);
    }
    recomputeFeature();
}

void TaskHoleParameters::drillPointAngledValueChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DrillPointAngle.setValue((double)value);
    recomputeFeature();
}

void TaskHoleParameters::drillForDepthChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->DrillForDepth.setValue(ui->DrillForDepth->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::taperedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Tapered.setValue(ui->Tapered->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::reversedChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Reversed.setValue(ui->Reversed->isChecked());
    recomputeFeature();
}

void TaskHoleParameters::taperedAngleChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->TaperedAngle.setValue(value);
    recomputeFeature();
}

void TaskHoleParameters::threadTypeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

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
    pcHole->ThreadType.setValue(index);

    // Threaded checkbox is meaningless if no thread profile is selected.
    ui->Threaded->setDisabled(std::string(pcHole->ThreadType.getValueAsString()) == "None");

    // size and clearance
    if (TypeClass == QByteArray("ISO")) {
        // the size for ISO type has either the form "M3x0.35" or just "M3"
        // so we need to check if the size contains a 'x'. If yes, check if the string
        // up to the 'x' is exists in the new list
        if (ThreadSizeString.indexOf(QString::fromLatin1("x")) > -1) {
            // we have an ISO fine size
            // cut of the part behind the 'x'
            ThreadSizeString = ThreadSizeString.left(ThreadSizeString.indexOf(QString::fromLatin1("x")));
        }
        // search if the string exists in the combobox
        int threadSizeIndex = ui->ThreadSize->findText(ThreadSizeString, Qt::MatchContains);
        if (threadSizeIndex > -1) {
            // we can set it
            ui->ThreadSize->setCurrentIndex(threadSizeIndex);
        }
        // the names of the clearance types are different in ISO and UTS
        ui->ThreadFit->setItemText(0, QCoreApplication::translate("TaskHoleParameters", "Standard", nullptr));
        ui->ThreadFit->setItemText(1, QCoreApplication::translate("TaskHoleParameters", "Close", nullptr));
        ui->ThreadFit->setItemText(2, QCoreApplication::translate("TaskHoleParameters", "Wide", nullptr));
    }
    else if (TypeClass == QByteArray("UTS")) {
        // for all UTS types the size entries are the same
        int threadSizeIndex = ui->ThreadSize->findText(ThreadSizeString, Qt::MatchContains);
        if (threadSizeIndex > -1) {
            ui->ThreadSize->setCurrentIndex(threadSizeIndex);
        }
        // the names of the clearance types are different in ISO and UTS
        ui->ThreadFit->setItemText(0, QCoreApplication::translate("TaskHoleParameters", "Normal", nullptr));
        ui->ThreadFit->setItemText(1, QCoreApplication::translate("TaskHoleParameters", "Close", nullptr));
        ui->ThreadFit->setItemText(2, QCoreApplication::translate("TaskHoleParameters", "Loose", nullptr));
    }

    // Class and cut type
    // the class and cut types are the same for both TypeClass so we don't need to distinguish between ISO and UTS
    int threadClassIndex = ui->ThreadClass->findText(ThreadClassString, Qt::MatchContains);
    if (threadClassIndex > -1)
        ui->ThreadClass->setCurrentIndex(threadClassIndex);
    int holeCutIndex = ui->HoleCutType->findText(CutTypeString, Qt::MatchContains);
    if (holeCutIndex > -1)
        ui->HoleCutType->setCurrentIndex(holeCutIndex);

    // we must set the read-only state according to the new HoleCutType
    holeCutTypeChanged(ui->HoleCutType->currentIndex());

    recomputeFeature();
}

void TaskHoleParameters::threadSizeChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadSize.setValue(index);
    recomputeFeature();

    // apply the recompute result to the widgets
    ui->HoleCutCustomValues->setDisabled(pcHole->HoleCutCustomValues.isReadOnly());
    ui->HoleCutCustomValues->setChecked(pcHole->HoleCutCustomValues.getValue());
}

void TaskHoleParameters::threadClassChanged(int index)
{
    if (index < 0)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadClass.setValue(index);
    recomputeFeature();
}

void TaskHoleParameters::threadDiameterChanged(double value)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->Diameter.setValue(value);

    // HoleCutDiameter must not be smaller or equal than the Diameter
    ui->HoleCutDiameter->setMinimum(value + 0.1);

    recomputeFeature();
}

void TaskHoleParameters::threadFitChanged(int index)
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    pcHole->ThreadFit.setValue(index);
    recomputeFeature();
}

void TaskHoleParameters::threadDirectionChanged()
{
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    if (sender() == ui->directionRightHand)
        pcHole->ThreadDirection.setValue((long)0);
    else
        pcHole->ThreadDirection.setValue((long)1);
    recomputeFeature();
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
    // happens when aborting the command
    if (!vp)
        return;

    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());
    bool ro = Prop.isReadOnly();

    Base::Console().Log("Parameter %s was updated\n", Prop.getName());

    if (&Prop == &pcHole->Threaded) {
        ui->Threaded->setEnabled(true);
        if (ui->Threaded->isChecked() ^ pcHole->Threaded.getValue()) {
            ui->Threaded->blockSignals(true);
            ui->Threaded->setChecked(pcHole->Threaded.getValue());
            ui->Threaded->blockSignals(false);
        }
        ui->Threaded->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadType) {
        ui->ThreadType->setEnabled(true);

        ui->ThreadSize->blockSignals(true);
        ui->ThreadSize->clear();
        std::vector<std::string> cursor = pcHole->ThreadSize.getEnumVector();
        for (const auto& it : cursor) {
            ui->ThreadSize->addItem(QString::fromStdString(it));
        }
        ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());
        ui->ThreadSize->blockSignals(false);

        // Thread type also updates HoleCutType and ThreadClass
        ui->HoleCutType->blockSignals(true);
        ui->HoleCutType->clear();
        cursor = pcHole->HoleCutType.getEnumVector();
        for (const auto& it: cursor) {
            ui->HoleCutType->addItem(QString::fromStdString(it));
        }
        ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
        ui->HoleCutType->blockSignals(false);

        ui->ThreadClass->blockSignals(true);
        ui->ThreadClass->clear();
        cursor = pcHole->ThreadClass.getEnumVector();
        for (const auto& it : cursor) {
            ui->ThreadClass->addItem(QString::fromStdString(it));
        }
        ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
        ui->ThreadClass->blockSignals(false);

        if (ui->ThreadType->currentIndex() != pcHole->ThreadType.getValue()) {
            ui->ThreadType->blockSignals(true);
            ui->ThreadType->setCurrentIndex(pcHole->ThreadType.getValue());
            ui->ThreadType->blockSignals(false);
        }
        ui->ThreadType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadSize) {
        ui->ThreadSize->setEnabled(true);
        if (ui->ThreadSize->currentIndex() != pcHole->ThreadSize.getValue()) {
            ui->ThreadSize->blockSignals(true);
            ui->ThreadSize->setCurrentIndex(pcHole->ThreadSize.getValue());
            ui->ThreadSize->blockSignals(false);
        }
        ui->ThreadSize->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadClass) {
        ui->ThreadClass->setEnabled(true);
        if (ui->ThreadClass->currentIndex() != pcHole->ThreadClass.getValue()) {
            ui->ThreadClass->blockSignals(true);
            ui->ThreadClass->setCurrentIndex(pcHole->ThreadClass.getValue());
            ui->ThreadClass->blockSignals(false);
        }
        ui->ThreadClass->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadFit) {
        ui->ThreadFit->setEnabled(true);
        if (ui->ThreadFit->currentIndex() != pcHole->ThreadFit.getValue()) {
            ui->ThreadFit->blockSignals(true);
            ui->ThreadFit->setCurrentIndex(pcHole->ThreadFit.getValue());
            ui->ThreadFit->blockSignals(false);
        }
        ui->ThreadFit->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Diameter) {
        ui->Diameter->setEnabled(true);
        if (ui->Diameter->value().getValue() != pcHole->Diameter.getValue()) {
            ui->Diameter->blockSignals(true);
            ui->Diameter->setValue(pcHole->Diameter.getValue());
            ui->Diameter->blockSignals(false);
        }
        ui->Diameter->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadDirection) {
        ui->directionRightHand->setEnabled(true);
        ui->directionLeftHand->setEnabled(true);
        std::string direction(pcHole->ThreadDirection.getValueAsString());
        if (direction == "Right" && !ui->directionRightHand->isChecked()) {
            ui->directionRightHand->blockSignals(true);
            ui->directionRightHand->setChecked(true);
            ui->directionRightHand->blockSignals(false);
        }
        if (direction == "Left" && !ui->directionLeftHand->isChecked()) {
            ui->directionLeftHand->blockSignals(true);
            ui->directionLeftHand->setChecked(true);
            ui->directionLeftHand->blockSignals(false);
        }
        ui->directionRightHand->setDisabled(ro);
        ui->directionLeftHand->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutType) {
        ui->HoleCutType->setEnabled(true);
        if (ui->HoleCutType->currentIndex() != pcHole->HoleCutType.getValue()) {
            ui->HoleCutType->blockSignals(true);
            ui->HoleCutType->setCurrentIndex(pcHole->HoleCutType.getValue());
            ui->HoleCutType->blockSignals(false);
        }
        ui->HoleCutType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutDiameter) {
        ui->HoleCutDiameter->setEnabled(true);
        if (ui->HoleCutDiameter->value().getValue() != pcHole->HoleCutDiameter.getValue()) {
            ui->HoleCutDiameter->blockSignals(true);
            ui->HoleCutDiameter->setValue(pcHole->HoleCutDiameter.getValue());
            ui->HoleCutDiameter->blockSignals(false);
        }
        ui->HoleCutDiameter->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutDepth) {
        ui->HoleCutDepth->setEnabled(true);
        if (ui->HoleCutDepth->value().getValue() != pcHole->HoleCutDepth.getValue()) {
            ui->HoleCutDepth->blockSignals(true);
            ui->HoleCutDepth->setValue(pcHole->HoleCutDepth.getValue());
            ui->HoleCutDepth->blockSignals(false);
        }
        ui->HoleCutDepth->setDisabled(ro);
    }
    else if (&Prop == &pcHole->HoleCutCountersinkAngle) {
        ui->HoleCutCountersinkAngle->setEnabled(true);
        if (ui->HoleCutCountersinkAngle->value().getValue() != pcHole->HoleCutCountersinkAngle.getValue()) {
            ui->HoleCutCountersinkAngle->blockSignals(true);
            ui->HoleCutCountersinkAngle->setValue(pcHole->HoleCutCountersinkAngle.getValue());
            ui->HoleCutCountersinkAngle->blockSignals(false);
        }
        ui->HoleCutCountersinkAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DepthType) {
        ui->DepthType->setEnabled(true);
        if (ui->DepthType->currentIndex() != pcHole->DepthType.getValue()) {
            ui->DepthType->blockSignals(true);
            ui->DepthType->setCurrentIndex(pcHole->DepthType.getValue());
            ui->DepthType->blockSignals(false);
        }
        ui->DepthType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Depth) {
        ui->Depth->setEnabled(true);
        if (ui->Depth->value().getValue() != pcHole->Depth.getValue()) {
            ui->Depth->blockSignals(true);
            ui->Depth->setValue(pcHole->Depth.getValue());
            ui->Depth->blockSignals(false);
        }
        ui->Depth->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DrillPoint) {
        ui->drillPointFlat->setEnabled(true);
        ui->drillPointAngled->setEnabled(true);
        std::string drillPoint(pcHole->DrillPoint.getValueAsString());
        if (drillPoint == "Flat" && !ui->drillPointFlat->isChecked()) {
            ui->drillPointFlat->blockSignals(true);
            ui->drillPointFlat->setChecked(true);
            ui->drillPointFlat->blockSignals(false);
        }
        if (drillPoint == "Angled" && !ui->drillPointAngled->isChecked()) {
            ui->drillPointAngled->blockSignals(true);
            ui->drillPointAngled->setChecked(true);
            ui->drillPointAngled->blockSignals(false);
        }
        ui->drillPointFlat->setDisabled(ro);
        ui->drillPointAngled->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DrillPointAngle) {
        ui->DrillPointAngle->setEnabled(true);
        if (ui->DrillPointAngle->value().getValue() != pcHole->DrillPointAngle.getValue()) {
            ui->DrillPointAngle->blockSignals(true);
            ui->DrillPointAngle->setValue(pcHole->DrillPointAngle.getValue());
            ui->DrillPointAngle->blockSignals(false);
        }
        ui->DrillPointAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->DrillForDepth) {
        ui->DrillForDepth->setEnabled(true);
        if (ui->DrillForDepth->isChecked() ^ pcHole->DrillForDepth.getValue()) {
            ui->DrillForDepth->blockSignals(true);
            ui->DrillForDepth->setChecked(pcHole->DrillForDepth.getValue());
            ui->DrillForDepth->blockSignals(false);
        }
        ui->DrillForDepth->setDisabled(ro);
    }
    else if (&Prop == &pcHole->Tapered) {
        ui->Tapered->setEnabled(true);
        if (ui->Tapered->isChecked() ^ pcHole->Tapered.getValue()) {
            ui->Tapered->blockSignals(true);
            ui->Tapered->setChecked(pcHole->Tapered.getValue());
            ui->Tapered->blockSignals(false);
        }
        ui->Tapered->setDisabled(ro);
    }
    else if (&Prop == &pcHole->TaperedAngle) {
        ui->TaperedAngle->setEnabled(true);
        if (ui->TaperedAngle->value().getValue() != pcHole->TaperedAngle.getValue()) {
            ui->TaperedAngle->blockSignals(true);
            ui->TaperedAngle->setValue(pcHole->TaperedAngle.getValue());
            ui->TaperedAngle->blockSignals(false);
        }
        ui->TaperedAngle->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ModelThread) {
        ui->ModelThread->setEnabled(true);
        if (ui->ModelThread->isChecked() ^ pcHole->ModelThread.getValue()) {
            ui->ModelThread->blockSignals(true);
            ui->ModelThread->setChecked(pcHole->ModelThread.getValue());
            ui->ModelThread->blockSignals(false);
        }
        ui->ModelThread->setDisabled(ro);
    }
    else if (&Prop == &pcHole->UseCustomThreadClearance) {
        ui->UseCustomThreadClearance->setEnabled(true);
        if (ui->UseCustomThreadClearance->isChecked() ^ pcHole->UseCustomThreadClearance.getValue()) {
            ui->UseCustomThreadClearance->blockSignals(true);
            ui->UseCustomThreadClearance->setChecked(pcHole->UseCustomThreadClearance.getValue());
            ui->UseCustomThreadClearance->blockSignals(false);
        }
        ui->UseCustomThreadClearance->setDisabled(ro);
    }
    else if (&Prop == &pcHole->CustomThreadClearance) {
        ui->CustomThreadClearance->setEnabled(true);
        if (ui->CustomThreadClearance->value().getValue() != pcHole->CustomThreadClearance.getValue()) {
            ui->CustomThreadClearance->blockSignals(true);
            ui->CustomThreadClearance->setValue(pcHole->CustomThreadClearance.getValue());
            ui->CustomThreadClearance->blockSignals(false);
        }
        ui->CustomThreadClearance->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadDepthType) {
        ui->ThreadDepthType->setEnabled(true);
        if (ui->ThreadDepthType->currentIndex() != pcHole->ThreadDepthType.getValue()) {
            ui->ThreadDepthType->blockSignals(true);
            ui->ThreadDepthType->setCurrentIndex(pcHole->ThreadDepthType.getValue());
            ui->ThreadDepthType->blockSignals(false);
        }
        ui->ThreadDepthType->setDisabled(ro);
    }
    else if (&Prop == &pcHole->ThreadDepth) {
        ui->ThreadDepth->setEnabled(true);
        if (ui->ThreadDepth->value().getValue() != pcHole->ThreadDepth.getValue()) {
            ui->ThreadDepth->blockSignals(true);
            ui->ThreadDepth->setValue(pcHole->ThreadDepth.getValue());
            ui->ThreadDepth->blockSignals(false);
        }
        ui->ThreadDepth->setDisabled(ro);
    }
}

void TaskHoleParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Q_UNUSED(msg)
}

bool TaskHoleParameters::getThreaded() const
{
    return ui->Threaded->isChecked();
}

long TaskHoleParameters::getThreadType() const
{
    return ui->ThreadType->currentIndex();
}

long TaskHoleParameters::getThreadSize() const
{
    if (ui->ThreadSize->currentIndex() == -1)
        return 0;
    else
        return ui->ThreadSize->currentIndex();
}

long TaskHoleParameters::getThreadClass() const
{
    if (ui->ThreadSize->currentIndex() == -1)
        return 0;
    else
        return ui->ThreadClass->currentIndex();
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
    if (ui->directionRightHand->isChecked())
        return 0;
    else
        return 1;
}

long TaskHoleParameters::getHoleCutType() const
{
    if (ui->HoleCutType->currentIndex() == -1)
        return 0;
    else
        return ui->HoleCutType->currentIndex();
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
    if (ui->drillPointFlat->isChecked())
        return 0;
    if (ui->drillPointAngled->isChecked())
        return 1;
    assert(0);
    return -1; // to avoid a compiler warning
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

double  TaskHoleParameters::getCustomThreadClearance() const
{
    return ui->CustomThreadClearance->value().getValue();
}

bool TaskHoleParameters::getModelThread() const
{
    return ui->ModelThread->isChecked();
}

long TaskHoleParameters::getThreadDepthType() const
{
    return ui->ThreadDepthType->currentIndex();
}

double TaskHoleParameters::getThreadDepth() const
{
    return ui->ThreadDepth->value().getValue();
}

void TaskHoleParameters::apply()
{
    auto obj = vp->getObject();
    PartDesign::Hole* pcHole = static_cast<PartDesign::Hole*>(vp->getObject());

    isApplying = true;

    ui->Diameter->apply();
    ui->HoleCutDiameter->apply();
    ui->HoleCutDepth->apply();
    ui->HoleCutCountersinkAngle->apply();
    ui->Depth->apply();
    ui->DrillPointAngle->apply();
    ui->TaperedAngle->apply();

    if (!pcHole->Threaded.isReadOnly())
        FCMD_OBJ_CMD(obj, "Threaded = " << (getThreaded() ? 1 : 0));
    if (!pcHole->ModelThread.isReadOnly())
        FCMD_OBJ_CMD(obj, "ModelThread = " << (getModelThread() ? 1 : 0));
    if (!pcHole->ThreadDepthType.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadDepthType = " << getThreadDepthType());
    if (!pcHole->ThreadDepth.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadDepth = " << getThreadDepth());
    if (!pcHole->UseCustomThreadClearance.isReadOnly())
        FCMD_OBJ_CMD(obj, "UseCustomThreadClearance = " << (getUseCustomThreadClearance() ? 1 : 0));
    if (!pcHole->CustomThreadClearance.isReadOnly())
        FCMD_OBJ_CMD(obj, "CustomThreadClearance = " << getCustomThreadClearance());
    if (!pcHole->ThreadType.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadType = " << getThreadType());
    if (!pcHole->ThreadSize.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadSize = " << getThreadSize());
    if (!pcHole->ThreadClass.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadClass = " << getThreadClass());
    if (!pcHole->ThreadFit.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadFit = " << getThreadFit());
    if (!pcHole->ThreadDirection.isReadOnly())
        FCMD_OBJ_CMD(obj, "ThreadDirection = " << getThreadDirection());
    if (!pcHole->HoleCutType.isReadOnly())
        FCMD_OBJ_CMD(obj, "HoleCutType = " << getHoleCutType());
    if (!pcHole->HoleCutCustomValues.isReadOnly())
        FCMD_OBJ_CMD(obj, "HoleCutCustomValues = " << (getHoleCutCustomValues() ? 1 : 0));
    if (!pcHole->DepthType.isReadOnly())
        FCMD_OBJ_CMD(obj, "DepthType = " << getDepthType());
    if (!pcHole->DrillPoint.isReadOnly())
        FCMD_OBJ_CMD(obj, "DrillPoint = " << getDrillPoint());
    if (!pcHole->DrillForDepth.isReadOnly())
        FCMD_OBJ_CMD(obj, "DrillForDepth = " << (getDrillForDepth() ? 1 : 0));
    if (!pcHole->Tapered.isReadOnly())
        FCMD_OBJ_CMD(obj, "Tapered = " << getTapered());

    isApplying = false;
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgHoleParameters::TaskDlgHoleParameters(ViewProviderHole* HoleView)
    : TaskDlgSketchBasedParameters(HoleView)
{
    assert(HoleView);
    parameter = new TaskHoleParameters(static_cast<ViewProviderHole*>(vp));

    Content.push_back(parameter);
}

TaskDlgHoleParameters::~TaskDlgHoleParameters() = default;

#include "moc_TaskHoleParameters.cpp"

TaskHoleParameters::Observer::Observer(TaskHoleParameters* _owner, PartDesign::Hole* _hole)
    : DocumentObserver(_hole->getDocument())
    , owner(_owner)
    , hole(_hole)
{
}

void TaskHoleParameters::Observer::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    if (&Obj == hole) {
        Base::Console().Log("Parameter %s was updated with a new value\n", Prop.getName());
        if (Obj.getDocument())
            owner->changedObject(*Obj.getDocument(), Prop);
    }
}
