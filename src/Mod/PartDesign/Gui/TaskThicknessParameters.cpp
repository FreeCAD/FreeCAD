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
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->Value, SIGNAL(valueChanged(double)),
        this, SLOT(onValueChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
        this, SLOT(onReversedChanged(bool)));
    connect(ui->checkIntersection, SIGNAL(toggled(bool)),
        this, SLOT(onIntersectionChanged(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefRemove(bool)));
    connect(ui->modeComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onModeChanged(int)));
    connect(ui->joinComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onJoinTypeChanged(int)));

    // Create context menu
    createDeleteAction(ui->listWidgetReferences, ui->buttonRefRemove);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(onRefDeleted()));

    connect(ui->listWidgetReferences, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(doubleClicked(QListWidgetItem*)));

    int mode = pcThickness->Mode.getValue();
    ui->modeComboBox->setCurrentIndex(mode);

    int join = pcThickness->Join.getValue();
    ui->joinComboBox->setCurrentIndex(join);

    // the dialog can be called on a broken thickness, then hide the thickness
    hideOnError();
}

void TaskThicknessParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // executed when the user selected something in the CAD object
    // adds/deletes the selection accordingly

    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                // it might be the second one so we can enable the context menu
                if (ui->listWidgetReferences->count() > 1) {
                    deleteAction->setEnabled(true);
                    deleteAction->setStatusTip(QString());
                    ui->buttonRefRemove->setEnabled(true);
                    ui->buttonRefRemove->setToolTip(tr("Click button to enter selection mode,\nclick again to end selection"));
                }
            }
            else {
                removeItemFromListWidget(ui->listWidgetReferences, msg.pSubName);
                // remove its selection too
                Gui::Selection().clearSelection();
                // if there is only one item left, it cannot be deleted
                if (ui->listWidgetReferences->count() == 1) {
                    deleteAction->setEnabled(false);
                    deleteAction->setStatusTip(tr("There must be at least one item"));
                    ui->buttonRefRemove->setEnabled(false);
                    ui->buttonRefRemove->setToolTip(tr("There must be at least one item"));
                    // we must also end the selection mode
                    exitSelectionMode();
                    clearButtons(none);
                }
            }
            // highlight existing references for possible further selections
            DressUpView->highlightReferences(true);
        } 
    }
}

void TaskThicknessParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskThicknessParameters::onRefDeleted(void)
{
    // assure we we are not in selection mode
    exitSelectionMode();
    clearButtons(none);
    // delete any selections since the reference(s) might be highlighted
    Gui::Selection().clearSelection();
    DressUpView->highlightReferences(false);

    // get the list of items to be deleted
    QList<QListWidgetItem*> selectedList = ui->listWidgetReferences->selectedItems();

    // if all items are selected, we must stop because one must be kept to avoid that the feature gets broken
    if (selectedList.count() == ui->listWidgetReferences->model()->rowCount()) {
        QMessageBox::warning(this, tr("Selection error"), tr("At least one item must be kept."));
        return;
    }

    // get the thickness object
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    App::DocumentObject* base = pcThickness->Base.getValue();
    // get all thickness references
    std::vector<std::string> refs = pcThickness->Base.getSubValues();
    setupTransaction();

    // delete the selection backwards to assure the list index keeps valid for the deletion
    for (int i = selectedList.count() - 1; i > -1; i--) {
        // the ref index is the same as the listWidgetReferences index
        // so we can erase using the row number of the element to be deleted
        int rowNumber = ui->listWidgetReferences->row(selectedList.at(i));
        // erase the reference
        refs.erase(refs.begin() + rowNumber);
        // remove from the list
        ui->listWidgetReferences->model()->removeRow(rowNumber);
    }

    // update the object
    pcThickness->Base.setValue(base, refs);
    // recompute the feature
    pcThickness->recomputeFeature();
    // hide the thickness if there was a computation error
    hideOnError();

    // if there is only one item left, it cannot be deleted
    if (ui->listWidgetReferences->count() == 1) {
        deleteAction->setEnabled(false);
        deleteAction->setStatusTip(tr("There must be at least one item"));
        ui->buttonRefRemove->setEnabled(false);
        ui->buttonRefRemove->setToolTip(tr("There must be at least one item"));
    }
}

void TaskThicknessParameters::onValueChanged(double angle)
{
    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Value.setValue(angle);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

void TaskThicknessParameters::onJoinTypeChanged(int join) {

    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Join.setValue(join);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

void TaskThicknessParameters::onModeChanged(int mode) {

    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Mode.setValue(mode);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

double TaskThicknessParameters::getValue(void) const
{
    return ui->Value->value().getValue();
}

void TaskThicknessParameters::onReversedChanged(const bool on) {
    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    setupTransaction();
    pcThickness->Reversed.setValue(on);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

bool TaskThicknessParameters::getReversed(void) const
{
    return ui->checkReverse->isChecked();
}

void TaskThicknessParameters::onIntersectionChanged(const bool on) {
    clearButtons(none);
    PartDesign::Thickness* pcThickness = static_cast<PartDesign::Thickness*>(DressUpView->getObject());
    pcThickness->Intersection.setValue(on);
    pcThickness->getDocument()->recomputeFeature(pcThickness);
    // hide the thickness if there was a computation error
    hideOnError();
}

bool TaskThicknessParameters::getIntersection(void) const
{
    return ui->checkIntersection->isChecked();
}

int TaskThicknessParameters::getJoinType(void) const {
    
    return ui->joinComboBox->currentIndex();
}

int TaskThicknessParameters::getMode(void) const {

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

TaskDlgThicknessParameters::~TaskDlgThicknessParameters()
{

}

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

    TaskThicknessParameters* draftparameter = static_cast<TaskThicknessParameters*>(parameter);

    FCMD_OBJ_CMD(obj,"Value = " << draftparameter->getValue());
    FCMD_OBJ_CMD(obj,"Reversed = " << draftparameter->getReversed());
    FCMD_OBJ_CMD(obj,"Mode = " << draftparameter->getMode());
    FCMD_OBJ_CMD(obj,"Intersection = " << draftparameter->getIntersection());
    FCMD_OBJ_CMD(obj,"Join = " << draftparameter->getJoinType());

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskThicknessParameters.cpp"
