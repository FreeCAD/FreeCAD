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

#ifndef _PreComp_
#include <QAction>
#include <QFontMetrics>
#include <QListWidget>
#include <QMessageBox>
#endif

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>

#include "ui_TaskChamferParameters.h"
#include "TaskChamferParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskChamferParameters */

TaskChamferParameters::TaskChamferParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(new Ui_TaskChamferParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Chamfer* pcChamfer = DressUpView->getObject<PartDesign::Chamfer>();

    setUpUI(pcChamfer);

    bool useAllEdges = pcChamfer->UseAllEdges.getValue();
    ui->checkBoxUseAllEdges->setChecked(useAllEdges);
    ui->buttonRefSel->setEnabled(!useAllEdges);
    ui->listWidgetReferences->setEnabled(!useAllEdges);
    QMetaObject::invokeMethod(ui->chamferSize, "setFocus", Qt::QueuedConnection);

    std::vector<std::string> strings = pcChamfer->Base.getSubValues();
    for (const auto& string : strings) {
        ui->listWidgetReferences->addItem(QString::fromStdString(string));
    }

    QMetaObject::connectSlotsByName(this);

    // clang-format off
    connect(ui->chamferType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskChamferParameters::onTypeChanged);
    connect(ui->chamferSize, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskChamferParameters::onSizeChanged);
    connect(ui->chamferSize2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskChamferParameters::onSize2Changed);
    connect(ui->chamferAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskChamferParameters::onAngleChanged);
    connect(ui->flipDirection, &QCheckBox::toggled,
            this, &TaskChamferParameters::onFlipDirection);
    connect(ui->buttonRefSel, &QToolButton::toggled,
            this, &TaskChamferParameters::onButtonRefSel);
    connect(ui->checkBoxUseAllEdges, &QCheckBox::toggled,
            this, &TaskChamferParameters::onCheckBoxUseAllEdgesToggled);

    // Create context menu
    createDeleteAction(ui->listWidgetReferences);
    connect(deleteAction, &QAction::triggered,
            this, &TaskChamferParameters::onRefDeleted);

    createAddAllEdgesAction(ui->listWidgetReferences);
    connect(addAllEdgesAction, &QAction::triggered,
            this, &TaskChamferParameters::onAddAllEdges);

    connect(ui->listWidgetReferences, &QListWidget::currentItemChanged,
            this, &TaskChamferParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemClicked,
            this, &TaskChamferParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemDoubleClicked,
            this, &TaskChamferParameters::doubleClicked);
    // clang-format on

    if (strings.size() == 0) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }
}

void TaskChamferParameters::setUpUI(PartDesign::Chamfer* pcChamfer)
{
    const int index = pcChamfer->ChamferType.getValue();
    ui->chamferType->setCurrentIndex(index);

    ui->flipDirection->setEnabled(index != 0);  // Enable if type is not "Equal distance"
    ui->flipDirection->setChecked(pcChamfer->FlipDirection.getValue());

    ui->chamferSize->setUnit(Base::Unit::Length);
    ui->chamferSize->setMinimum(0);
    ui->chamferSize->setValue(pcChamfer->Size.getValue());
    ui->chamferSize->bind(pcChamfer->Size);
    ui->chamferSize->selectNumber();

    ui->chamferSize2->setUnit(Base::Unit::Length);
    ui->chamferSize2->setMinimum(0);
    ui->chamferSize2->setValue(pcChamfer->Size2.getValue());
    ui->chamferSize2->bind(pcChamfer->Size2);

    ui->chamferAngle->setUnit(Base::Unit::Angle);
    ui->chamferAngle->setMinimum(pcChamfer->Angle.getMinimum());
    ui->chamferAngle->setMaximum(pcChamfer->Angle.getMaximum());
    ui->chamferAngle->setValue(pcChamfer->Angle.getValue());
    ui->chamferAngle->bind(pcChamfer->Angle);

    ui->stackedWidget->setFixedHeight(ui->chamferSize2->sizeHint().height());

    QFontMetrics fm(ui->typeLabel->font());
    int minWidth = Gui::QtTools::horizontalAdvance(fm, ui->typeLabel->text());
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->sizeLabel->text()));
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->size2Label->text()));
    minWidth = std::max<int>(minWidth, Gui::QtTools::horizontalAdvance(fm, ui->angleLabel->text()));
    minWidth = minWidth + 5;  // spacing
    ui->typeLabel->setMinimumWidth(minWidth);
    ui->sizeLabel->setMinimumWidth(minWidth);
    ui->size2Label->setMinimumWidth(minWidth);
    ui->angleLabel->setMinimumWidth(minWidth);
}

void TaskChamferParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // executed when the user selected something in the CAD object
    // adds/deletes the selection accordingly

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelected(msg, ui->listWidgetReferences);
        }
    }
}

void TaskChamferParameters::onCheckBoxUseAllEdgesToggled(bool checked)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        if (checked) {
            setSelectionMode(none);
        }

        ui->buttonRefSel->setEnabled(!checked);
        ui->listWidgetReferences->setEnabled(!checked);
        chamfer->UseAllEdges.setValue(checked);
        chamfer->recomputeFeature();
    }
}

void TaskChamferParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? btnPreviewStr() : btnSelectStr());
}

void TaskChamferParameters::onRefDeleted()
{
    TaskDressUpParameters::deleteRef(ui->listWidgetReferences);
}

void TaskChamferParameters::onAddAllEdges()
{
    TaskDressUpParameters::addAllEdges(ui->listWidgetReferences);
}

void TaskChamferParameters::onTypeChanged(int index)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        setSelectionMode(none);
        chamfer->ChamferType.setValue(index);
        ui->stackedWidget->setCurrentIndex(index);
        ui->flipDirection->setEnabled(index != 0);  // Enable if type is not "Equal distance"
        chamfer->recomputeFeature();
        // hide the chamfer if there was a computation error
        hideOnError();
    }
}

void TaskChamferParameters::onSizeChanged(double len)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        setSelectionMode(none);
        setupTransaction();
        chamfer->Size.setValue(len);
        chamfer->recomputeFeature();
        // hide the chamfer if there was a computation error
        hideOnError();
    }
}

void TaskChamferParameters::onSize2Changed(double len)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        setSelectionMode(none);
        setupTransaction();
        chamfer->Size2.setValue(len);
        chamfer->recomputeFeature();
        // hide the chamfer if there was a computation error
        hideOnError();
    }
}

void TaskChamferParameters::onAngleChanged(double angle)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        setSelectionMode(none);
        setupTransaction();
        chamfer->Angle.setValue(angle);
        chamfer->recomputeFeature();
        // hide the chamfer if there was a computation error
        hideOnError();
    }
}

void TaskChamferParameters::onFlipDirection(bool flip)
{
    if (auto chamfer = getObject<PartDesign::Chamfer>()) {
        setSelectionMode(none);
        setupTransaction();
        chamfer->FlipDirection.setValue(flip);
        chamfer->recomputeFeature();
        // hide the chamfer if there was a computation error
        hideOnError();
    }
}

int TaskChamferParameters::getType() const
{
    return ui->chamferType->currentIndex();
}

double TaskChamferParameters::getSize() const
{
    return ui->chamferSize->value().getValue();
}

double TaskChamferParameters::getSize2() const
{
    return ui->chamferSize2->value().getValue();
}

double TaskChamferParameters::getAngle() const
{
    return ui->chamferAngle->value().getValue();
}

bool TaskChamferParameters::getFlipDirection() const
{
    return ui->flipDirection->isChecked();
}

TaskChamferParameters::~TaskChamferParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }
}

void TaskChamferParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskChamferParameters::apply()
{
    auto chamfer = getObject<PartDesign::Chamfer>();

    const int chamfertype = chamfer->ChamferType.getValue();

    switch (chamfertype) {

        case 0:  // "Equal distance"
            ui->chamferSize->apply();
            break;
        case 1:  // "Two distances"
            ui->chamferSize->apply();
            ui->chamferSize2->apply();
            break;
        case 2:  // "Distance and Angle"
            ui->chamferSize->apply();
            ui->chamferAngle->apply();
            break;
    }

    // Alert user if he created an empty feature
    if (ui->listWidgetReferences->count() == 0) {
        Base::Console().warning(tr("Empty chamfer created !\n").toStdString().c_str());
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgChamferParameters::TaskDlgChamferParameters(ViewProviderChamfer* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskChamferParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgChamferParameters::~TaskDlgChamferParameters() = default;

//==== calls from the TaskView ===============================================================

bool TaskDlgChamferParameters::accept()
{
    auto obj = getObject();
    if (!obj->isError()) {
        parameter->showObject();
    }

    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskChamferParameters.cpp"
