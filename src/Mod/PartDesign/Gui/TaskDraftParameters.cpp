/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QKeyEvent>
# include <QListWidget>
# include <QMessageBox>
#endif

#include <Base/Interpreter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureDraft.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

#include "ui_TaskDraftParameters.h"
#include "TaskDraftParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskDraftParameters */

TaskDraftParameters::TaskDraftParameters(ViewProviderDressUp *DressUpView, QWidget *parent)
    : TaskDressUpParameters(DressUpView, false, true, parent)
    , ui(new Ui_TaskDraftParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);

    this->groupLayout()->addWidget(proxy);

    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    double a = pcDraft->Angle.getValue();

    ui->draftAngle->setMinimum(pcDraft->Angle.getMinimum());
    ui->draftAngle->setMaximum(pcDraft->Angle.getMaximum());
    ui->draftAngle->setValue(a);
    ui->draftAngle->selectAll();
    QMetaObject::invokeMethod(ui->draftAngle, "setFocus", Qt::QueuedConnection);

    // Bind input fields to properties
    ui->draftAngle->bind(pcDraft->Angle);

    bool r = pcDraft->Reversed.getValue();
    ui->checkReverse->setChecked(r);

    std::vector<std::string> strings = pcDraft->Base.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); ++i)
    {
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
    }

    QMetaObject::connectSlotsByName(this);

    connect(ui->draftAngle, SIGNAL(valueChanged(double)),
        this, SLOT(onAngleChanged(double)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
        this, SLOT(onReversedChanged(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
        this, SLOT(onButtonRefRemove(bool)));
    connect(ui->buttonPlane, SIGNAL(toggled(bool)),
        this, SLOT(onButtonPlane(bool)));
    connect(ui->buttonLine, SIGNAL(toggled(bool)),
        this, SLOT(onButtonLine(bool)));

    // Create context menu
    createDeleteAction(ui->listWidgetReferences, ui->buttonRefRemove);
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(onRefDeleted()));

    connect(ui->listWidgetReferences, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemClicked(QListWidgetItem*)),
        this, SLOT(setSelection(QListWidgetItem*)));
    connect(ui->listWidgetReferences, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(doubleClicked(QListWidgetItem*)));

    App::DocumentObject* ref = pcDraft->NeutralPlane.getValue();
    strings = pcDraft->NeutralPlane.getSubValues();
    ui->linePlane->setText(getRefStr(ref, strings));

    ref = pcDraft->PullDirection.getValue();
    strings = pcDraft->PullDirection.getSubValues();
    ui->lineLine->setText(getRefStr(ref, strings));

    // the dialog can be called on a broken draft, then hide the draft
    hideOnError();
}

void TaskDraftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
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
        } else if (selectionMode == plane) {
            PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
            std::vector<std::string> planes;
            App::DocumentObject* selObj;
            getReferencedSelection(pcDraft, msg, selObj, planes);
            if(!selObj)
                return;
            setupTransaction();
            pcDraft->NeutralPlane.setValue(selObj, planes);
            ui->linePlane->setText(getRefStr(selObj, planes));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            // highlight existing references for possible further selections
            DressUpView->highlightReferences(true);
            // hide the draft if there was a computation error
            hideOnError();
        } else if (selectionMode == line) {
            PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
            std::vector<std::string> edges;
            App::DocumentObject* selObj;
            getReferencedSelection(pcDraft, msg, selObj, edges);
            if(!selObj)
                return;
            setupTransaction();
            pcDraft->PullDirection.setValue(selObj, edges);
            ui->lineLine->setText(getRefStr(selObj, edges));

            pcDraft->getDocument()->recomputeFeature(pcDraft);
            // highlight existing references for possible further selections
            DressUpView->highlightReferences(true);
            // hide the draft if there was a computation error
            hideOnError();
        }
    }
}

void TaskDraftParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refAdd) ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove) ui->buttonRefRemove->setChecked(false);
    if (notThis != line) ui->buttonLine->setChecked(false);
    if (notThis != plane) ui->buttonPlane->setChecked(false);
    DressUpView->highlightReferences(false);
}

void TaskDraftParameters::onButtonPlane(bool checked)
{
    if (checked) {
        clearButtons(plane);
        hideObject();
        selectionMode = plane;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), AllowSelection::EDGE |
                                                                                  AllowSelection::FACE |
                                                                                  AllowSelection::PLANAR));
    }
}

void TaskDraftParameters::onButtonLine(bool checked)
{
    if (checked) {
        clearButtons(line);
        hideObject();
        selectionMode = line;
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ReferenceSelection(this->getBase(), AllowSelection::EDGE |
                                                                                  AllowSelection::PLANAR));
    }
}

void TaskDraftParameters::onRefDeleted(void)
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

    // get the draft object
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    App::DocumentObject* base = pcDraft->Base.getValue();
    // get all draft references
    std::vector<std::string> refs = pcDraft->Base.getSubValues();
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
    pcDraft->Base.setValue(base, refs);
    // recompute the feature
    pcDraft->recomputeFeature();
    // hide the draft if there was a computation error
    hideOnError();

    // if there is only one item left, it cannot be deleted
    if (ui->listWidgetReferences->count() == 1) {
        deleteAction->setEnabled(false);
        deleteAction->setStatusTip(tr("There must be at least one item"));
        ui->buttonRefRemove->setEnabled(false);
        ui->buttonRefRemove->setToolTip(tr("There must be at least one item"));
    }
}

void TaskDraftParameters::getPlane(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->linePlane->text().split(QChar::fromLatin1(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::getLine(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    sub = std::vector<std::string>(1,"");
    QStringList parts = ui->lineLine->text().split(QChar::fromLatin1(':'));
    obj = DressUpView->getObject()->getDocument()->getObject(parts[0].toStdString().c_str());
    if (parts.size() > 1)
        sub[0] = parts[1].toStdString();
}

void TaskDraftParameters::onAngleChanged(double angle)
{
    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    setupTransaction();
    pcDraft->Angle.setValue(angle);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
    // hide the draft if there was a computation error
    hideOnError();
}

double TaskDraftParameters::getAngle(void) const
{
    return ui->draftAngle->value().getValue();
}

void TaskDraftParameters::onReversedChanged(const bool on) {
    clearButtons(none);
    PartDesign::Draft* pcDraft = static_cast<PartDesign::Draft*>(DressUpView->getObject());
    setupTransaction();
    pcDraft->Reversed.setValue(on);
    pcDraft->getDocument()->recomputeFeature(pcDraft);
    // hide the draft if there was a computation error
    hideOnError();
}

bool TaskDraftParameters::getReversed(void) const
{
    return ui->checkReverse->isChecked();
}

TaskDraftParameters::~TaskDraftParameters()
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

bool TaskDraftParameters::event(QEvent *e)
{
    return TaskDressUpParameters::KeyEvent(e);
}

void TaskDraftParameters::changeEvent(QEvent *e)
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

TaskDlgDraftParameters::TaskDlgDraftParameters(ViewProviderDraft *DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter  = new TaskDraftParameters(DressUpView);

    Content.push_back(parameter);
}

TaskDlgDraftParameters::~TaskDlgDraftParameters()
{

}

//==== calls from the TaskView ===============================================================


//void TaskDlgDraftParameters::open()
//{
//    // a transaction is already open at creation time of the draft
//    if (!Gui::Command::hasPendingCommand()) {
//        QString msg = QObject::tr("Edit draft");
//        Gui::Command::openCommand((const char*)msg.toUtf8());
//    }
//}

bool TaskDlgDraftParameters::accept()
{
    auto tobj = vp->getObject();
    if (!tobj->isError())
        parameter->showObject();

    std::vector<std::string> strings;
    App::DocumentObject* obj;
    TaskDraftParameters* draftparameter = static_cast<TaskDraftParameters*>(parameter);

    draftparameter->getPlane(obj, strings);
    std::string neutralPlane = buildLinkSingleSubPythonStr(obj, strings);

    draftparameter->getLine(obj, strings);
    std::string pullDirection = buildLinkSingleSubPythonStr(obj, strings);

    // Force the user to select a neutral plane
    // if (neutralPlane.empty() || neutralPlane == "None") {
    //     QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Missing neutral plane"),
    //         QObject::tr("Please select a plane or an edge plus a pull direction"));
    //     return false;
    // }

    FCMD_OBJ_CMD(tobj,"Angle = " << draftparameter->getAngle());
    FCMD_OBJ_CMD(tobj,"Reversed = " << draftparameter->getReversed());
    if(neutralPlane.empty())
        neutralPlane = "None";
    FCMD_OBJ_CMD(tobj,"NeutralPlane = " << neutralPlane);
    if(pullDirection.empty())
        pullDirection = "None";
    FCMD_OBJ_CMD(tobj,"PullDirection = " << pullDirection);

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskDraftParameters.cpp"
