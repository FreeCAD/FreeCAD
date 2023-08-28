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
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Mod/PartDesign/App/FeatureLoft.h>

#include "ui_TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include "TaskSketchBasedParameters.h"

Q_DECLARE_METATYPE(App::PropertyLinkSubList::SubSet)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView, bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_AdditiveLoft", tr("Loft parameters"))
    , ui(new Ui_TaskLoftParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonProfileBase, &QToolButton::toggled,
            this, &TaskLoftParameters::onProfileButton);
    connect(ui->buttonRefAdd, &QToolButton::toggled,
            this, &TaskLoftParameters::onRefButtonAdd);
    connect(ui->buttonRefRemove, &QToolButton::toggled,
            this, &TaskLoftParameters::onRefButtonRemove);
    connect(ui->checkBoxRuled, &QCheckBox::toggled,
            this, &TaskLoftParameters::onRuled);
    connect(ui->checkBoxClosed, &QCheckBox::toggled,
            this, &TaskLoftParameters::onClosed);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskLoftParameters::onUpdateView);

    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetReferences->addAction(remove);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(remove, &QAction::triggered, this, &TaskLoftParameters::onDeleteSection);

    connect(ui->listWidgetReferences->model(), &QAbstractListModel::rowsMoved,
            this, &TaskLoftParameters::indexesMoved);

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    const auto childs = proxy->findChildren<QWidget*>();
    for (QWidget* child : childs)
        child->blockSignals(true);

    //add the profiles
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(LoftView->getObject());
    App::DocumentObject* profile = loft->Profile.getValue();
    if (profile) {
        Gui::Application::Instance->showViewProvider(profile);

        // TODO: if it is a single vertex of a sketch, use that subshape's name
        QString label = make2DLabel(profile, loft->Profile.getSubValues());
        ui->profileBaseEdit->setText(label);
    }

    for (auto &subSet : loft->Sections.getSubListValues()) {
        Gui::Application::Instance->showViewProvider(subSet.first);

        // TODO: if it is a single vertex of a sketch, use that subshape's name
        QString label = make2DLabel(subSet.first, subSet.second);
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(label);
        item->setData(Qt::UserRole, QVariant::fromValue(subSet));
        ui->listWidgetReferences->addItem(item);
    }

    // get options
    ui->checkBoxRuled->setChecked(loft->Ruled.getValue());
    ui->checkBoxClosed->setChecked(loft->Closed.getValue());

    // activate and de-activate dialog elements as appropriate
    for (QWidget* child : childs)
        child->blockSignals(false);

    updateUI();
}

TaskLoftParameters::~TaskLoftParameters() = default;

void TaskLoftParameters::updateUI()
{
    // we must assure the changed loft is kept visible on section changes,
    // see https://forum.freecad.org/viewtopic.php?f=3&t=63252
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    vp->makeTemporaryVisible(!loft->Sections.getValues().empty());
}

void TaskLoftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            App::Document* document = App::GetApplication().getDocument(msg.pDocName);
            App::DocumentObject* object = document ? document->getObject(msg.pObjectName) : nullptr;
            if (object) {
                // TODO: if it is a single vertex of a sketch, use that subshape's name
                QString label = make2DLabel(object, {msg.pSubName});
                if (selectionMode == refProfile) {
                    ui->profileBaseEdit->setText(label);
                }
                else if (selectionMode == refAdd) {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setText(label);
                    item->setData(Qt::UserRole,
                                  QVariant::fromValue(std::make_pair(object, std::vector<std::string>(1, msg.pSubName))));
                    ui->listWidgetReferences->addItem(item);
                }
                else if (selectionMode == refRemove) {
                    removeFromListWidget(ui->listWidgetReferences, label);
                }
            }

            clearButtons();
            recomputeFeature();
        }

        clearButtons();
        exitSelectionMode();
        updateUI();
    }
}

bool TaskLoftParameters::referenceSelected(const Gui::SelectionChanges& msg) const {

    if (msg.Type == Gui::SelectionChanges::AddSelection && selectionMode != none) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;

        //every selection needs to be a profile in itself, hence currently only full objects are
        //supported, not individual edges of a part

        //change the references
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        App::DocumentObject* obj = loft->getDocument()->getObject(msg.pObjectName);

        if (selectionMode == refProfile) {
            static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Profile, false);
            loft->Profile.setValue(obj, {msg.pSubName});
            return true;
        }
        else if (selectionMode == refAdd || selectionMode == refRemove) {
            // now check the sections
            std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
            std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);

            if (selectionMode == refAdd) {
                if (f == refs.end())
                    loft->Sections.addValue(obj, {msg.pSubName});
                else
                    return false; // duplicate selection
            }
            else if (selectionMode == refRemove) {
                if (f != refs.end())
                    // Removing just the object this way instead of `refs.erase` and
                    // `setValues(ref)` cleanly ensures subnames are preserved.
                    loft->Sections.removeValue(obj);
                else
                    return false;
            }

            return true;
        }
    }

    return false;
}

void TaskLoftParameters::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (auto it : items) {
            QListWidgetItem* item = widget->takeItem(widget->row(it));
            delete item;
        }
    }
}

void TaskLoftParameters::onDeleteSection()
{
    // Delete the selected profile
    int row = ui->listWidgetReferences->currentRow();
    QListWidgetItem* item = ui->listWidgetReferences->takeItem(row);
    if (item) {
        QByteArray data(item->data(Qt::UserRole).value<App::PropertyLinkSubList::SubSet>().first->getNameInDocument());
        delete item;

        // search inside the list of sections
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
        App::DocumentObject* obj = loft->getDocument()->getObject(data.constData());
        std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);
        if (f != refs.end()) {
            // Removing just the object this way instead of `refs.erase` and
            // `setValues(ref)` cleanly ensures subnames are preserved.
            loft->Sections.removeValue(obj);

            recomputeFeature();
            updateUI();
        }
    }
}

void TaskLoftParameters::indexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model)
        return;

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    auto originals = loft->Sections.getSubListValues();

    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        originals[i] = index.data(Qt::UserRole).value<App::PropertyLinkSubList::SubSet>();
    }

    loft->Sections.setSubListValues(originals);
    recomputeFeature();
    updateUI();
}

void TaskLoftParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refProfile)
        ui->buttonProfileBase->setChecked(false);
    if (notThis != refAdd)
        ui->buttonRefAdd->setChecked(false);
    if (notThis != refRemove)
        ui->buttonRefRemove->setChecked(false);
}

void TaskLoftParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().clearSelection();
    this->blockSelection(true);
}

void TaskLoftParameters::changeEvent(QEvent * /*e*/)
{
}

void TaskLoftParameters::onClosed(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Closed.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onRuled(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Ruled.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onProfileButton(bool checked)
{
    if (checked) {
        clearButtons(refProfile);
        Gui::Selection().clearSelection();
        selectionMode = refProfile;
        this->blockSelection(false);
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, true);
    }
    else {
        Gui::Selection().clearSelection();
        selectionMode = none;
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, false);
    }
}

void TaskLoftParameters::onRefButtonAdd(bool checked)
{
    if (checked) {
        clearButtons(refAdd);
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        this->blockSelection(false);
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, true);
    }
    else {
        Gui::Selection().clearSelection();
        selectionMode = none;
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, false);
    }
}

void TaskLoftParameters::onRefButtonRemove(bool checked)
{
    if (checked) {
        clearButtons(refRemove);
        Gui::Selection().clearSelection();
        selectionMode = refRemove;
        this->blockSelection(false);
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, true);
    }
    else {
        Gui::Selection().clearSelection();
        selectionMode = none;
        static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, false);
    }
}



//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj)
   : TaskDlgSketchBasedParameters(LoftView)
{
    assert(LoftView);
    parameter  = new TaskLoftParameters(LoftView,newObj);

    Content.push_back(parameter);
}

TaskDlgLoftParameters::~TaskDlgLoftParameters() = default;

bool TaskDlgLoftParameters::accept()
{
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(vp->getObject());
    static_cast<ViewProviderLoft*>(vp)->highlightReferences(ViewProviderLoft::Both, false);

    // First verify that the loft can be built and then hide the sections as otherwise
    // they will remain hidden if the loft's recompute fails
    if (TaskDlgSketchBasedParameters::accept()) {
        for (App::DocumentObject* obj : pcLoft->Sections.getValues()) {
            Gui::cmdAppObjectHide(obj);
        }

        return true;
    }

    return false;
}

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
