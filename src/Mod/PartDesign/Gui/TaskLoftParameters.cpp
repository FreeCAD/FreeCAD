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
# include <sstream>
# include <QAction>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView,bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_Additive_Loft",tr("Loft parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLoftParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onProfileButton(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonRemvove(bool)));
    connect(ui->checkBoxRuled, SIGNAL(toggled(bool)),
            this, SLOT(onRuled(bool)));
    connect(ui->checkBoxClosed, SIGNAL(toggled(bool)),
            this, SLOT(onClosed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QKeySequence::Delete);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetReferences->addAction(remove);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(remove, SIGNAL(triggered()), this, SLOT(onDeleteSection()));

    connect(ui->listWidgetReferences->model(),
        SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(indexesMoved()));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    //add the profiles
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(LoftView->getObject());
    App::DocumentObject* profile = loft->Profile.getValue();
    if (profile) {
        Gui::Application::Instance->showViewProvider(profile);

        QString label = QString::fromUtf8(profile->Label.getValue());
        ui->profileBaseEdit->setText(label);
    }

    for (auto obj : loft->Sections.getValues()) {
        Gui::Application::Instance->showViewProvider(obj);

        QString label = QString::fromUtf8(obj->Label.getValue());
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(label);
        item->setData(Qt::UserRole, QByteArray(obj->getNameInDocument()));
        ui->listWidgetReferences->addItem(item);
    }

    // get options
    ui->checkBoxRuled->setChecked(loft->Ruled.getValue());
    ui->checkBoxClosed->setChecked(loft->Closed.getValue());

    if (!loft->Sections.getValues().empty()) {
        LoftView->makeTemporaryVisible(true);
    }

    // activate and de-activate dialog elements as appropriate
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    updateUI(0);
}

TaskLoftParameters::~TaskLoftParameters()
{
    delete ui;
}

void TaskLoftParameters::updateUI(int index)
{
    Q_UNUSED(index);
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
                QString label = QString::fromUtf8(object->Label.getValue());
                if (selectionMode == refProfile) {
                    ui->profileBaseEdit->setText(label);
                }
                else if (selectionMode == refAdd) {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setText(label);
                    item->setData(Qt::UserRole, QByteArray(msg.pObjectName));
                    ui->listWidgetReferences->addItem(item);
                }
                else if (selectionMode == refRemove) {
                    removeFromListWidget(ui->listWidgetReferences, label);
                }
            }

            clearButtons();
            //static_cast<ViewProviderLoft*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        }

        clearButtons();
        exitSelectionMode();
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
            loft->Profile.setValue(obj);
            return true;
        }
        else if (selectionMode == refAdd || selectionMode == refRemove) {
            // now check the sections
            std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
            std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);

            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(obj);
                else
                    return false; // duplicate selection
            }
            else if (selectionMode == refRemove) {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }

            static_cast<PartDesign::Loft*>(vp->getObject())->Sections.setValues(refs);
            return true;
        }
    }

    return false;
}

void TaskLoftParameters::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
            QListWidgetItem* item = widget->takeItem(widget->row(*it));
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
        QByteArray data = item->data(Qt::UserRole).toByteArray();
        delete item;

        // search inside the list of sections
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
        App::DocumentObject* obj = loft->getDocument()->getObject(data.constData());
        std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);
        if (f != refs.end()) {
            refs.erase(f);
            loft->Sections.setValues(refs);

            //static_cast<ViewProviderLoft*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        }
    }
}

void TaskLoftParameters::indexesMoved()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    if (!model)
        return;

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    std::vector<App::DocumentObject*> originals = loft->Sections.getValues();

    QByteArray name;
    int rows = model->rowCount();
    for (int i = 0; i < rows; i++) {
        QModelIndex index = model->index(i, 0);
        name = index.data(Qt::UserRole).toByteArray().constData();
        originals[i] = loft->getDocument()->getObject(name.constData());
    }

    loft->Sections.setValues(originals);
    recomputeFeature();
}

void TaskLoftParameters::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
}

void TaskLoftParameters::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
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
        Gui::Selection().clearSelection();
        selectionMode = refProfile;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}

void TaskLoftParameters::onRefButtonAdd(bool checked) {
    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}

void TaskLoftParameters::onRefButtonRemvove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refRemove;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
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

TaskDlgLoftParameters::~TaskDlgLoftParameters()
{
}


bool TaskDlgLoftParameters::accept()
{
    // TODO Fill this with commands (2015-09-11, Fat-Zer)
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(vp->getObject());

    for(App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        FCMD_OBJ_HIDE(obj);
    }


    return TaskDlgSketchBasedParameters::accept ();
}

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
