// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <QAction>
#include <QStandardItemModel>

#include <BRepOffset_Mode.hxx>

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureThickness.h>
#include <Mod/Part/App/GizmoHelper.h>

#include "ui_TaskThicknessParameters.h"
#include "TaskThicknessParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskThicknessParameters */

using PartDesign::Thickness;

TaskThicknessParameters::TaskThicknessParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
    , ui(new Ui_TaskThicknessParameters)
{
    addContainerWidget();
    initControls();

    setupGizmos(DressUpView);
}

void TaskThicknessParameters::addContainerWidget()
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    // Keep the mode indices aligned with BRepOffset_Mode while hiding Pipe.
    this->groupLayout()->addWidget(proxy);
}

void TaskThicknessParameters::initControls()
{
    auto thickness = getObject<Thickness>();
    double a = thickness->Value.getValue();

    ui->Value->setMinimum(0.0);
    ui->Value->setValue(a);
    ui->Value->selectAll();
    QMetaObject::invokeMethod(ui->Value, "setFocus", Qt::QueuedConnection);

    // Bind input fields to properties
    ui->Value->bind(thickness->Value);

    bool i = thickness->Intersection.getValue();
    ui->checkIntersection->setChecked(i);

    std::vector<std::string> strings = thickness->Base.getSubValues();
    for (const auto& string : strings) {
        ui->listWidgetReferences->addItem(QString::fromStdString(string));
    }

    setupConnections();

    int join = static_cast<int>(thickness->Join.getValue());
    ui->joinComboBox->setCurrentIndex(join);

    int selectionMode = static_cast<int>(thickness->Selection.getValue());
    ui->selectionMode->setCurrentIndex(selectionMode);

    const bool enableSelection = selectionMode
        != static_cast<int>(Thickness::SelectionMode::AllSolids);
    ui->listWidgetReferences->setEnabled(enableSelection);
    ui->buttonRefSel->setEnabled(enableSelection);

    const double centering = thickness->Centering.getValue();
    ui->centering->setValue(centering * 100);
    ui->centeringValue->setMinimum(-1);
    ui->centeringValue->setMaximum(1);
    ui->centeringValue->setValue(centering);

    if (strings.empty()) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }
}

void TaskThicknessParameters::setupConnections()
{
    // clang-format off
    QMetaObject::connectSlotsByName(this);

    connect(ui->Value, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &TaskThicknessParameters::onValueChanged);
    connect(ui->checkIntersection, &QCheckBox::toggled,
            this, &TaskThicknessParameters::onIntersectionChanged);
    connect(ui->buttonRefSel, &QToolButton::toggled,
            this, &TaskThicknessParameters::onButtonRefSel);
    connect(ui->joinComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
    this, &TaskThicknessParameters::onJoinTypeChanged);
    connect(ui->selectionMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskThicknessParameters::onSelectionModeChanged);
    connect(ui->centering, qOverload<int>(&QSlider::valueChanged), this, &TaskThicknessParameters::onCenteringChanged);
    connect(ui->centeringValue, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &TaskThicknessParameters::onCenteringValueChanged);
    connect(ui->centerButton, &QPushButton::clicked, this, &TaskThicknessParameters::onSetRectoVerso);
    connect(ui->insideButton, &QPushButton::clicked, this, &TaskThicknessParameters::onSetInside);
    connect(ui->outsideButton, &QPushButton::clicked, this, &TaskThicknessParameters::onSetOutside);

    // Create context menu
    createDeleteAction(ui->listWidgetReferences);
    connect(deleteAction, &QAction::triggered, this, &TaskThicknessParameters::onRefDeleted);

    connect(ui->listWidgetReferences, &QListWidget::currentItemChanged,
            this, &TaskThicknessParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemClicked,
            this, &TaskThicknessParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemDoubleClicked,
            this, &TaskThicknessParameters::doubleClicked);
    // clang-format on
}

void TaskThicknessParameters::onSelectionModeChanged(int selectionMode)
{
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Selection.setValue(selectionMode);
        onAfterChange(thickness);
    }

    const bool enableSelection = selectionMode
        != static_cast<int>(Thickness::SelectionMode::AllSolids);
    ui->listWidgetReferences->setEnabled(enableSelection);
    ui->buttonRefSel->setEnabled(enableSelection);
}

void TaskThicknessParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelected(msg, ui->listWidgetReferences);
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        // TODO: the gizmo position should be only recalculated when the feature associated
        // with the gizmo is removed from the list
        setGizmoPositions();
    }
}

void TaskThicknessParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? stopSelectionLabel() : startSelectionLabel());
}

void TaskThicknessParameters::onRefDeleted()
{
    TaskDressUpParameters::deleteRef(ui->listWidgetReferences);
}

Thickness* TaskThicknessParameters::onBeforeChange()
{
    setButtons(none);
    setupTransaction();
    return getObject<Thickness>();
}

void TaskThicknessParameters::onAfterChange(Thickness* obj)
{
    obj->recomputeFeature();
    // hide the thickness if there was a computation error
    hideOnError();
}

void TaskThicknessParameters::onValueChanged(double size)
{
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Value.setValue(size);
        onAfterChange(thickness);
    }
}

double TaskThicknessParameters::getCentering() const
{
    return ui->centeringValue->value().getValue();
}

void TaskThicknessParameters::onCenteringValueChanged(double size)
{
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Centering.setValue(size);
        onAfterChange(thickness);
    }
    ui->centering->setValue(size * 100);
}

void TaskThicknessParameters::onCenteringChanged(int size)
{
    const double val = size / 100.0;
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Centering.setValue(val);
        onAfterChange(thickness);
    }
    ui->centeringValue->setValue(val);
}

void TaskThicknessParameters::onSetInside()
{
    onCenteringValueChanged(-1);
}

void TaskThicknessParameters::onSetRectoVerso()
{
    onCenteringValueChanged(0);
}

void TaskThicknessParameters::onSetOutside()
{
    onCenteringValueChanged(1);
}

void TaskThicknessParameters::onJoinTypeChanged(int join)
{
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Join.setValue(join);
        onAfterChange(thickness);
    }
}

double TaskThicknessParameters::getValue() const
{
    return ui->Value->value().getValue();
}

int TaskThicknessParameters::getSelectionMode() const
{
    return ui->selectionMode->currentIndex();
}

void TaskThicknessParameters::onIntersectionChanged(bool on)
{
    if (Thickness* thickness = onBeforeChange()) {
        thickness->Intersection.setValue(on);
        onAfterChange(thickness);
    }
}

bool TaskThicknessParameters::getIntersection() const
{
    return ui->checkIntersection->isChecked();
}

int TaskThicknessParameters::getJoinType() const
{

    return ui->joinComboBox->currentIndex();
}

TaskThicknessParameters::~TaskThicknessParameters()
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

void TaskThicknessParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskThicknessParameters::apply()
{
    // Alert user if he created an empty feature
    if (ui->listWidgetReferences->count() == 0
        && ui->selectionMode->currentIndex()
            != static_cast<int>(Thickness::SelectionMode::AllSolids)) {
        Base::Console().warning("{}", tr("Empty thickness created!\n").toStdString());
    }
}

void TaskThicknessParameters::setupGizmos(ViewProviderDressUp* vp)
{
    if (!GizmoContainer::isEnabled()) {
        return;
    }

    linearGizmo = new Gui::LinearGizmo(ui->Value);

    gizmoContainer = GizmoContainer::create({linearGizmo}, vp);

    setGizmoPositions();
    showDraggerHints();
}

void TaskThicknessParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    auto thickness = getObject<Thickness>();
    if (!thickness) {
        gizmoContainer->visible = false;
        return;
    }
    if (thickness->Mode.getValue() == BRepOffset_RectoVerso) {
        gizmoContainer->visible = false;
        return;
    }
    auto baseShape = thickness->getBaseTopoShape();
    auto shapes = thickness->getContinuousEdges(baseShape);
    auto faces = thickness->getFaces(baseShape);

    if (shapes.size() == 0 || faces.size() == 0) {
        gizmoContainer->visible = false;
        return;
    }
    gizmoContainer->visible = true;

    Part::TopoShape edge = shapes[0];
    DraggerPlacementProps props = getDraggerPlacementFromEdgeAndFace(edge, faces[0]);
    props.dir *= thickness->Reversed.getValue() ? 1 : -1;

    linearGizmo->Gizmo::setDraggerPlacement(props.position, props.dir);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgThicknessParameters::TaskDlgThicknessParameters(ViewProviderThickness* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskThicknessParameters(DressUpView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgThicknessParameters::~TaskDlgThicknessParameters() = default;

bool TaskDlgThicknessParameters::accept()
{
    auto obj = getObject();
    if (!obj->isError()) {
        getViewObject()->showPreviousFeature(false);
    }

    parameter->apply();

    auto draftparameter = dynamic_cast<TaskThicknessParameters*>(parameter);

    FCMD_OBJ_CMD(obj, "Value = " << draftparameter->getValue());
    FCMD_OBJ_CMD(obj, "Intersection = " << draftparameter->getIntersection());
    FCMD_OBJ_CMD(obj, "Join = " << draftparameter->getJoinType());
    FCMD_OBJ_CMD(obj, "Selection = " << draftparameter->getSelectionMode());
    FCMD_OBJ_CMD(obj, "Centering = " << draftparameter->getCentering());

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskThicknessParameters.cpp"
