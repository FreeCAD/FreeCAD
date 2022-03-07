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
# include <QAction>
# include <QListWidget>
# include <QMessageBox>
#endif

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureFillet.h>

#include "ui_TaskFilletParameters.h"
#include "TaskFilletParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskFilletParameters */

TaskFilletParameters::TaskFilletParameters(ViewProviderDressUp *DressUpView, QWidget *parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(new Ui_TaskFilletParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    bool useAllEdges = pcFillet->UseAllEdges.getValue();
    ui->checkBoxUseAllEdges->setChecked(useAllEdges);
    ui->buttonRefAdd->setEnabled(!useAllEdges);
    ui->buttonRefRemove->setEnabled(!useAllEdges);
    ui->listWidgetReferences->setEnabled(!useAllEdges);
    double r = pcFillet->Radius.getValue();

    ui->filletRadius->setUnit(Base::Unit::Length);
    ui->filletRadius->setValue(r);
    ui->filletRadius->setMinimum(0);
    ui->filletRadius->selectNumber();
    ui->filletRadius->bind(pcFillet->Radius);
    QMetaObject::invokeMethod(ui->filletRadius, "setFocus", Qt::QueuedConnection);
    std::vector<std::string> strings = pcFillet->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->filletRadius, SIGNAL(valueChanged(double)),
        this, SLOT(onLengthChanged(double)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefRemove(bool)));
    connect(ui->checkBoxUseAllEdges, SIGNAL(toggled(bool)),
            this, SLOT(onCheckBoxUseAllEdgesToggled(bool)));

    // Create context menu
    createDeleteAction(ui->listWidgetReferences, ui->buttonRefRemove);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(onRefDeleted()));

    createAddAllEdgesAction(ui->listWidgetReferences);
    connect(addAllEdgesAction, &QAction::triggered, this, &TaskFilletParameters::onAddAllEdges);

    connect(ui->listWidgetReferences, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(doubleClicked(QListWidgetItem*)));

    // the dialog can be called on a broken fillet, then hide the fillet
    hideOnError();
}

void TaskFilletParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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

void TaskFilletParameters::onCheckBoxUseAllEdgesToggled(bool checked)
{
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    ui->buttonRefRemove->setEnabled(!checked);
    ui->buttonRefAdd->setEnabled(!checked);
    ui->listWidgetReferences->setEnabled(!checked);
    pcFillet->UseAllEdges.setValue(checked);
    pcFillet->getDocument()->recomputeFeature(pcFillet);
}

void TaskFilletParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskFilletParameters::onRefDeleted(void)
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
    if (selectedList.count() == ui->listWidgetReferences->model()->rowCount()){
        QMessageBox::warning(this, tr("Selection error"), tr("At least one item must be kept."));
        return;
    }

    // get the fillet object
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    App::DocumentObject* base = pcFillet->Base.getValue();
    // get all fillet references
    std::vector<std::string> refs = pcFillet->Base.getSubValues();
    setupTransaction();

    // delete the selection backwards to assure the list index keeps valid for the deletion
    for (int i = selectedList.count()-1; i > -1; i--) {
        // the ref index is the same as the listWidgetReferences index
        // so we can erase using the row number of the element to be deleted
        int rowNumber = ui->listWidgetReferences->row(selectedList.at(i));
        // erase the reference
        refs.erase(refs.begin() + rowNumber);
        // remove from the list
        ui->listWidgetReferences->model()->removeRow(rowNumber);
    }

    // update the object
    pcFillet->Base.setValue(base, refs);
    // recompute the feature
    pcFillet->recomputeFeature();
    // hide the fillet if there was a computation error
    hideOnError();

    // if there is only one item left, it cannot be deleted
    if (ui->listWidgetReferences->count() == 1) {
        deleteAction->setEnabled(false);
        deleteAction->setStatusTip(tr("There must be at least one item"));
        ui->buttonRefRemove->setEnabled(false);
        ui->buttonRefRemove->setToolTip(tr("There must be at least one item"));
    }
}

void TaskFilletParameters::onAddAllEdges(void)
{
    TaskDressUpParameters::addAllEdges(ui->listWidgetReferences);
    ui->buttonRefRemove->setEnabled(true);
}

void TaskFilletParameters::onLengthChanged(double len)
{
    clearButtons(none);
    PartDesign::Fillet* pcFillet = static_cast<PartDesign::Fillet*>(DressUpView->getObject());
    setupTransaction();
    pcFillet->Radius.setValue(len);
    pcFillet->getDocument()->recomputeFeature(pcFillet);
    // hide the fillet if there was a computation error
    hideOnError();   
}

double TaskFilletParameters::getLength(void) const
{
    return ui->filletRadius->value().getValue();
}

TaskFilletParameters::~TaskFilletParameters()
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

bool TaskFilletParameters::event(QEvent *e)
{
    return TaskDressUpParameters::KeyEvent(e);
}

void TaskFilletParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskFilletParameters::apply()
{
    std::string name = getDressUpView()->getObject()->getNameInDocument();

    //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Fillet changed"));
    ui->filletRadius->apply();
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFilletParameters::TaskDlgFilletParameters(ViewProviderFillet *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskFilletParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgFilletParameters::~TaskDlgFilletParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgFilletParameters::open()
//{
//    // a transaction is already open at creation time of the fillet
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = tr("Edit fillet");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}
bool TaskDlgFilletParameters::accept()
{
    auto obj = vp->getObject();
    if (!obj->isError())
        parameter->showObject();

    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskFilletParameters.cpp"
