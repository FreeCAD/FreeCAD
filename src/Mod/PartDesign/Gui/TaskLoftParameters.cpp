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
#include <QComboBox>
#include <QSpinBox>


#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Mod/PartDesign/App/FeatureLoft.h>

#include "ui_TaskLoftParameters.h"
#include "ui_TaskLoftAdvancedParameters.h"
#include "TaskLoftParameters.h"
#include "TaskSketchBasedParameters.h"

Q_DECLARE_METATYPE(App::PropertyLinkSubList::SubSet)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

namespace
{
bool isSubtractiveLoft(ViewProviderLoft* view)
{
    auto* loft = view->getObject<PartDesign::Loft>();
    return loft->getAddSubType() == PartDesign::FeatureAddSub::Subtractive;
}

std::string loftTaskIconName(ViewProviderLoft* view)
{
    return isSubtractiveLoft(view) ? "PartDesign_SubtractiveLoft" : "PartDesign_AdditiveLoft";
}

QString loftTaskTitle(ViewProviderLoft* view)
{
    return isSubtractiveLoft(view) ? TaskLoftParameters::tr("Subtractive Loft Parameters")
                                   : TaskLoftParameters::tr("Additive Loft Parameters");
}

QString loftAdvancedTaskTitle(ViewProviderLoft* view)
{
    return isSubtractiveLoft(view)
        ? TaskLoftAdvancedParameters::tr("Subtractive Loft Advanced Parameters")
        : TaskLoftAdvancedParameters::tr("Additive Loft Advanced Parameters");
}
}  // namespace

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft* LoftView, bool /*newObj*/, QWidget* parent)
    : TaskSketchBasedParameters(LoftView, parent, loftTaskIconName(LoftView), loftTaskTitle(LoftView))
    , ui(new Ui_TaskLoftParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // clang-format off
    connect(ui->buttonProfileBase, &QToolButton::toggled,
            this, &TaskLoftParameters::onProfileButton);
    connect(ui->buttonRefAdd, &QToolButton::toggled,
            this, &TaskLoftParameters::onRefButtonAdd);
    connect(ui->buttonRefRemove, &QToolButton::toggled,
            this, &TaskLoftParameters::onRefButtonRemove);
    connect(ui->comboBoxMode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskLoftParameters::onModeChanged);
    connect(ui->checkBoxClosed, &QCheckBox::toggled,
            this, &TaskLoftParameters::onClosed);
    connect(ui->checkBoxAdaptive, &QCheckBox::toggled,
            this, &TaskLoftParameters::onAdaptive);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskLoftParameters::onUpdateView);
    connect(ui->checkBoxUpdateView, &QCheckBox::toggled,
            this, &TaskLoftParameters::updateViewChanged);
    // clang-format on

    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(Gui::QtTools::deleteKeySequence());

    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
    ui->listWidgetReferences->addAction(remove);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(remove, &QAction::triggered, this, &TaskLoftParameters::onDeleteSection);

    connect(
        ui->listWidgetReferences->model(),
        &QAbstractListModel::rowsMoved,
        this,
        &TaskLoftParameters::indexesMoved
    );

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    const auto childs = proxy->findChildren<QWidget*>();
    for (QWidget* child : childs) {
        child->blockSignals(true);
    }

    // add the profiles
    PartDesign::Loft* loft = LoftView->getObject<PartDesign::Loft>();
    App::DocumentObject* profile = loft->Profile.getValue();
    if (profile) {
        Gui::Application::Instance->showViewProvider(profile);

        // TODO: if it is a single vertex of a sketch, use that subshape's name
        QString label = make2DLabel(profile, loft->Profile.getSubValues());
        ui->profileBaseEdit->setText(label);
    }

    for (auto& subSet : loft->Sections.getSubListValues()) {
        Gui::Application::Instance->showViewProvider(subSet.first);

        // TODO: if it is a single vertex of a sketch, use that subshape's name
        QString label = make2DLabel(subSet.first, subSet.second);
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(label);
        item->setData(Qt::UserRole, QVariant::fromValue(subSet));
        ui->listWidgetReferences->addItem(item);
    }

    // get options
    const int mode = loft->LoftType.getValue();
    ui->comboBoxMode->setCurrentIndex(mode);
    ui->checkBoxClosed->setChecked(loft->Closed.getValue());
    ui->checkBoxAdaptive->setChecked(loft->Adaptive.getValue());

    // activate and de-activate dialog elements as appropriate
    for (QWidget* child : childs) {
        child->blockSignals(false);
    }

    updateUI();
}

TaskLoftParameters::~TaskLoftParameters() = default;

TaskLoftAdvancedParameters::TaskLoftAdvancedParameters(
    ViewProviderLoft* LoftView,
    bool /*newObj*/,
    QWidget* parent
)
    : TaskSketchBasedParameters(
          LoftView,
          parent,
          loftTaskIconName(LoftView),
          loftAdvancedTaskTitle(LoftView)
      )
    , ui(new Ui_TaskLoftAdvancedParameters)
{
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // clang-format off
    connect(ui->spinBoxMaxDegree, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskLoftAdvancedParameters::onMaxDegreeChanged);
    connect(ui->comboBoxParametrization, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskLoftAdvancedParameters::onParametrizationChanged);
    connect(ui->comboBoxContinuity, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskLoftAdvancedParameters::onContinuityChanged);
    connect(ui->checkBoxCompatibility, &QCheckBox::toggled,
            this, &TaskLoftAdvancedParameters::onCheckCompatibility);
    // clang-format on

    groupLayout()->addWidget(proxy);

    const auto children = proxy->findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->blockSignals(true);
    }

    auto* loft = LoftView->getObject<PartDesign::Loft>();
    const auto* degreeConstraints = loft->MaxDegree.getConstraints();
    ui->spinBoxMaxDegree->setRange(degreeConstraints->LowerBound, degreeConstraints->UpperBound);
    ui->spinBoxMaxDegree->setSingleStep(degreeConstraints->StepSize);
    ui->spinBoxMaxDegree->setValue(loft->MaxDegree.getValue());
    ui->comboBoxParametrization->setCurrentIndex(loft->Parametrization.getValue());
    ui->comboBoxContinuity->setCurrentIndex(loft->Continuity.getValue());
    ui->checkBoxCompatibility->setChecked(loft->CheckCompatibility.getValue());
    updateAlgorithmOptions(loft->LoftType.getValue());

    for (QWidget* child : children) {
        child->blockSignals(false);
    }

    // if (!isSubtractiveLoft(LoftView)) {
    hideGroupBox();
    // }
}

TaskLoftAdvancedParameters::~TaskLoftAdvancedParameters() = default;

void TaskLoftAdvancedParameters::onSelectionChanged(const Gui::SelectionChanges&)
{}

void TaskLoftParameters::updateUI()
{
    // we must assure the changed loft is kept visible on section changes,
    // see https://forum.freecad.org/viewtopic.php?f=3&t=63252
    auto loft = getObject<PartDesign::Loft>();
    if (loft) {
        auto view = getViewObject();
        view->makeTemporaryVisible(!loft->Sections.getValues().empty());
    }
}

void TaskLoftAdvancedParameters::updateAlgorithmOptions(int mode)
{
    // Ruled lofts bypass approximation; only the standard solver accepts parametrization.
    const bool usesApproximation = mode != 1;
    const bool usesParametrization = mode == 0;
    ui->spinBoxMaxDegree->setEnabled(usesApproximation);
    ui->labelMaxDegree->setEnabled(usesApproximation);
    ui->comboBoxContinuity->setEnabled(usesApproximation);
    ui->labelContinuity->setEnabled(usesApproximation);
    ui->comboBoxParametrization->setEnabled(usesParametrization);
    ui->labelParametrization->setEnabled(usesParametrization);
    ui->checkBoxCompatibility->setEnabled(true);
}

void TaskLoftAdvancedParameters::setUpdateView(bool enabled)
{
    onUpdateView(enabled);
}

void TaskLoftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none) {
        return;
    }

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
                    item->setData(
                        Qt::UserRole,
                        QVariant::fromValue(
                            std::make_pair(object, std::vector<std::string>(1, msg.pSubName))
                        )
                    );
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

bool TaskLoftParameters::referenceSelected(const Gui::SelectionChanges& msg) const
{

    if (msg.Type == Gui::SelectionChanges::AddSelection && selectionMode != none) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0) {
            return false;
        }

        // not allowed to reference ourself
        const char* fname = getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0) {
            return false;
        }

        // every selection needs to be a profile in itself, hence currently only full objects are
        // supported, not individual edges of a part

        // change the references
        auto loft = getObject<PartDesign::Loft>();
        App::Document* doc = loft->getDocument();
        App::DocumentObject* obj = doc->getObject(msg.pObjectName);

        if (selectionMode == refProfile) {
            auto view = getViewObject<ViewProviderLoft>();
            view->highlightReferences(ViewProviderLoft::Profile, false);
            loft->Profile.setValue(obj, {msg.pSubName});
            return true;
        }

        if (selectionMode == refAdd || selectionMode == refRemove) {
            // now check the sections
            std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
            const auto f = std::ranges::find(refs, obj);

            if (selectionMode == refAdd) {
                if (f != refs.end()) {
                    return false;  // duplicate selection
                }

                loft->Sections.addValue(obj, {msg.pSubName});
            }
            else if (selectionMode == refRemove) {
                if (f == refs.end()) {
                    return false;
                }

                loft->Sections.removeValue(obj);
            }

            return true;
        }
    }

    return false;
}

void TaskLoftParameters::removeFromListWidget(QListWidget* widget, QString name)
{

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
        QByteArray data(
            item->data(Qt::UserRole).value<App::PropertyLinkSubList::SubSet>().first->getNameInDocument()
        );
        delete item;

        // search inside the list of sections
        if (const auto loft = getObject<PartDesign::Loft>()) {
            std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
            App::DocumentObject* obj = loft->getDocument()->getObject(data.constData());
            if (const auto f = std::ranges::find(refs, obj); f != refs.end()) {
                loft->Sections.removeValue(obj);

                recomputeFeature();
                updateUI();
            }
        }
    }
}

void TaskLoftParameters::indexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model) {
        return;
    }

    if (auto loft = getObject<PartDesign::Loft>()) {
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
}

void TaskLoftParameters::clearButtons(const selectionModes notThis)
{
    if (notThis != refProfile) {
        ui->buttonProfileBase->setChecked(false);
    }
    if (notThis != refAdd) {
        ui->buttonRefAdd->setChecked(false);
    }
    if (notThis != refRemove) {
        ui->buttonRefRemove->setChecked(false);
    }
}

void TaskLoftParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Selection().clearSelection();
    this->blockSelection(true);
}

void TaskLoftParameters::changeEvent(QEvent* /*e*/)
{}

void TaskLoftParameters::onClosed(bool val)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->Closed.setValue(val);
        recomputeFeature();
    }
}

void TaskLoftParameters::onAdaptive(bool enabled)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->Adaptive.setValue(enabled);
        recomputeFeature();
    }
}

void TaskLoftParameters::onModeChanged(int index)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->LoftType.setValue(index);
        Q_EMIT algorithmChanged(index);
        recomputeFeature();
    }
}

void TaskLoftAdvancedParameters::onMaxDegreeChanged(int value)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->MaxDegree.setValue(value);
        recomputeFeature();
    }
}

void TaskLoftAdvancedParameters::onParametrizationChanged(int index)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->Parametrization.setValue(index);
        recomputeFeature();
    }
}

void TaskLoftAdvancedParameters::onContinuityChanged(int index)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->Continuity.setValue(index);
        recomputeFeature();
    }
}

void TaskLoftAdvancedParameters::onCheckCompatibility(bool enabled)
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        loft->CheckCompatibility.setValue(enabled);
        recomputeFeature();
    }
}

void TaskLoftParameters::setSelectionMode(selectionModes mode, bool checked)
{
    if (checked) {
        clearButtons(mode);
        Gui::Selection().clearSelection();
        selectionMode = mode;
        this->blockSelection(false);
    }
    else {
        Gui::Selection().clearSelection();
        selectionMode = none;
    }

    auto view = getViewObject<ViewProviderLoft>();
    view->highlightReferences(ViewProviderLoft::Both, checked);
}

void TaskLoftParameters::onProfileButton(bool checked)
{
    setSelectionMode(refProfile, checked);
}

void TaskLoftParameters::onRefButtonAdd(bool checked)
{
    setSelectionMode(refAdd, checked);
}

void TaskLoftParameters::onRefButtonRemove(bool checked)
{
    setSelectionMode(refRemove, checked);
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft* LoftView, bool newObj)
    : TaskDlgSketchBasedParameters(LoftView)
{
    assert(LoftView);
    parameter = new TaskLoftParameters(LoftView, newObj);
    advanced = new TaskLoftAdvancedParameters(LoftView, newObj);

    connect(
        parameter,
        &TaskLoftParameters::algorithmChanged,
        advanced,
        &TaskLoftAdvancedParameters::updateAlgorithmOptions
    );
    connect(
        parameter,
        &TaskLoftParameters::updateViewChanged,
        advanced,
        &TaskLoftAdvancedParameters::setUpdateView
    );

    Content.push_back(parameter);
    Content.push_back(advanced);
    Content.push_back(preview);
}

TaskDlgLoftParameters::~TaskDlgLoftParameters() = default;

bool TaskDlgLoftParameters::accept()
{
    if (auto loft = getObject<PartDesign::Loft>()) {
        getViewObject<ViewProviderLoft>()->highlightReferences(ViewProviderLoft::Both, false);

        // First verify that the loft can be built and then hide the sections as otherwise
        // they will remain hidden if the loft's recompute fails
        if (TaskDlgSketchBasedParameters::accept()) {
            for (App::DocumentObject* obj : loft->Sections.getValues()) {
                Gui::cmdAppObjectHide(obj);
            }

            return true;
        }
    }

    return false;
}

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
