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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QAction>
# include <QListWidget>
# include <QMessageBox>
#endif

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureThickness.h>

#include "ui_TaskThicknessParameters.h"
#include "TaskThicknessParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskThicknessParameters */

TaskThicknessParameters::TaskThicknessParameters(ViewProviderDressUp *DressUpView, QWidget *parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
    , ui(new Ui_TaskThicknessParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    double a = pcThickness->Value.getValue();

    ui->Value->setMinimum(0.0);
    ui->Value->setMaximum(89.99);
    ui->Value->setValue(a);
    ui->Value->selectAll();
    QMetaObject::invokeMethod(ui->Value, "setFocus", Qt::QueuedConnection);

    // Bind input fields to properties
    ui->Value->bind(pcThickness->Value);

    bool r = pcThickness->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    bool i = pcThickness->Intersection.getValue();
    ui->checkIntersection->setChecked(i);

    std::vector<std::string> strings = pcThickness->Base.getSubValues();
    for (const auto & string : strings)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(string));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->Value, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskThicknessParameters::onValueChanged);
    connect(ui->checkReverse, &QCheckBox::toggled,
        this, &TaskThicknessParameters::onReversedChanged);
    connect(ui->checkIntersection, &QCheckBox::toggled,
        this, &TaskThicknessParameters::onIntersectionChanged);
    connect(ui->buttonRefSel, &QToolButton::toggled,
        this, &TaskThicknessParameters::onButtonRefSel);
    connect(ui->modeComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &TaskThicknessParameters::onModeChanged);
    connect(ui->joinComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &TaskThicknessParameters::onJoinTypeChanged);

    // Create context menu
    createDeleteAction(ui->listWidgetReferences);
    connect(deleteAction, &QAction::triggered, this, &TaskThicknessParameters::onRefDeleted);

    connect(ui->listWidgetReferences, &QListWidget::currentItemChanged,
        this, &TaskThicknessParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemClicked,
        this, &TaskThicknessParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemDoubleClicked,
        this, &TaskThicknessParameters::doubleClicked);

    int mode = pcThickness->Mode.getValue();
    ui->modeComboBox->setCurrentIndex(mode);

    int join = pcThickness->Join.getValue();
    ui->joinComboBox->setCurrentIndex(join);

    if (strings.size() == 0)
        setSelectionMode(refSel);
    else
        hideOnError();
}

void TaskThicknessParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // executed when the user selected something in the CAD object
    // adds/deletes the selection accordingly

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelected(msg, ui->listWidgetReferences);
        }
    }
}

void TaskThicknessParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? btnPreviewStr() : btnSelectStr());
}

void TaskThicknessParameters::onRefDeleted()
{
    TaskDressUpParameters::deleteRef(ui->listWidgetReferences);
}

void TaskThicknessParameters::onValueChanged(double angle)
{
    setButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Value.setValue(angle);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

void TaskThicknessParameters::onJoinTypeChanged(int join) {

    setButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Join.setValue(join);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

void TaskThicknessParameters::onModeChanged(int mode) {

    setButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Mode.setValue(mode);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

double TaskThicknessParameters::getValue() const
{
    return ui->Value->value().getValue();
}

void TaskThicknessParameters::onReversedChanged(const bool on) {
    setButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Reversed.setValue(on);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

bool TaskThicknessParameters::getReversed() const
{
    return ui->checkReverse->isChecked();
}

void TaskThicknessParameters::onIntersectionChanged(const bool on) {
    setButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Intersection.setValue(on);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

bool TaskThicknessParameters::getIntersection() const
{
    return ui->checkIntersection->isChecked();
}

int TaskThicknessParameters::getJoinType() const {

    return ui->joinComboBox->currentIndex();
}

int TaskThicknessParameters::getMode() const {

    return ui->modeComboBox->currentIndex();
}

TaskThicknessParameters::~TaskThicknessParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool TaskThicknessParameters::event(QEvent *e)
{
    return TaskDressUpParameters::KeyEvent(e);
}

void TaskThicknessParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskThicknessParameters::apply()
{
    //Alert user if he created an empty feature
    if (ui->listWidgetReferences->count() == 0)
        Base::Console().Warning(tr("Empty thickness created !\n").toStdString().c_str());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgThicknessParameters::TaskDlgThicknessParameters(ViewProviderThickness *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskThicknessParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgThicknessParameters::~TaskDlgThicknessParameters() = default;

//==== calls from the TaskView ===============================================================


//void TaskDlgThicknessParameters::open()
//{
//    // a transaction is already open at creation time of the draft
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = QObject::tr("Edit draft");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
//
//void TaskDlgThicknessParameters::clicked(int)
//{
//
//}

bool TaskDlgThicknessParameters::accept()
{
    auto obj = vp->getObject();
    if (!obj->isError())
        parameter->showObject();

    parameter->apply();

    TaskThicknessParameters* draftparameter = static_cast<TaskThicknessParameters*>(parameter);

    FCMD_OBJ_CMD(obj,"Value = " << draftparameter->getValue());
    FCMD_OBJ_CMD(obj,"Reversed = " << draftparameter->getReversed());
    FCMD_OBJ_CMD(obj,"Mode = " << draftparameter->getMode());
    FCMD_OBJ_CMD(obj,"Intersection = " << draftparameter->getIntersection());
    FCMD_OBJ_CMD(obj,"Join = " << draftparameter->getJoinType());

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskThicknessParameters.cpp"
