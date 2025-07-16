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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <limits>
#endif

#include <App/DocumentObject.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Command.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureScaled.h>

#include "ui_TaskScaledParameters.h"
#include "TaskScaledParameters.h"
#include "TaskMultiTransformParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskScaledParameters */

TaskScaledParameters::TaskScaledParameters(ViewProviderTransformed* TransformedView,
                                           QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskScaledParameters)
{
    setupUI();
}

TaskScaledParameters::TaskScaledParameters(TaskMultiTransformParameters* parentTask,
                                           QWidget* parameterWidget)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui_TaskScaledParameters)
{
    setupParameterUI(parameterWidget);
}

void TaskScaledParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);
    QMetaObject::connectSlotsByName(this);

    connect(ui->spinFactor,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this,
            &TaskScaledParameters::onFactor);
    connect(ui->spinOccurrences,
            &Gui::UIntSpinBox::unsignedChanged,
            this,
            &TaskScaledParameters::onOccurrences);

    // Get the feature data
    auto pcScaled = getObject<PartDesign::Scaled>();

    ui->spinFactor->bind(pcScaled->Factor);
    ui->spinOccurrences->setMaximum(std::numeric_limits<int>::max());
    ui->spinOccurrences->bind(pcScaled->Occurrences);
    ui->spinFactor->setEnabled(true);
    ui->spinOccurrences->setEnabled(true);
    // ui->spinFactor->setDecimals(Base::UnitsApi::getDecimals());

    updateUI();
}

void TaskScaledParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

void TaskScaledParameters::updateUI()
{
    if (blockUpdate) {
        return;
    }
    blockUpdate = true;

    auto pcScaled = getObject<PartDesign::Scaled>();

    double factor = pcScaled->Factor.getValue();
    unsigned occurrences = pcScaled->Occurrences.getValue();

    ui->spinFactor->setValue(factor);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskScaledParameters::onFactor(const double factor)
{
    if (blockUpdate) {
        return;
    }
    auto pcScaled = getObject<PartDesign::Scaled>();
    pcScaled->Factor.setValue(factor);
    recomputeFeature();
}

void TaskScaledParameters::onOccurrences(const uint number)
{
    if (blockUpdate) {
        return;
    }
    auto pcScaled = getObject<PartDesign::Scaled>();
    pcScaled->Occurrences.setValue(number);
    recomputeFeature();
}

void TaskScaledParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgScaledParameters::accept() but without doCommand
        auto pcScaled = getObject<PartDesign::Scaled>();
        pcScaled->Factor.setValue(getFactor());
        pcScaled->Occurrences.setValue(getOccurrences());
        recomputeFeature();
    }
}

double TaskScaledParameters::getFactor() const
{
    return ui->spinFactor->value().getValue();
}

unsigned TaskScaledParameters::getOccurrences() const
{
    return ui->spinOccurrences->value();
}

void TaskScaledParameters::apply()
{
    FCMD_OBJ_CMD(getObject(), "Factor = " << getFactor());
    ui->spinOccurrences->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgScaledParameters::TaskDlgScaledParameters(ViewProviderScaled* ScaledView)
    : TaskDlgTransformedParameters(ScaledView)
{
    parameter = new TaskScaledParameters(ScaledView);

    Content.push_back(parameter);
}

#include "moc_TaskScaledParameters.cpp"
