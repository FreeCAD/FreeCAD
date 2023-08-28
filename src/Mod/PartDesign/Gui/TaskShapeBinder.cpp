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
# include <boost/core/ignore_unused.hpp>
# include <QAction>
# include <QMessageBox>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Widgets.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ui_TaskShapeBinder.h"
#include "TaskShapeBinder.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskShapeBinder */


//**************************************************************************
//**************************************************************************
// TaskShapeBinder
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskShapeBinder::TaskShapeBinder(ViewProviderShapeBinder* view, bool newObj, QWidget* parent)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("PartDesign_ShapeBinder"),
        tr("Datum shape parameters"), true, parent)
    , SelectionObserver(view)
    , ui(new Ui_TaskShapeBinder)
    , vp(view)
{
    boost::ignore_unused(newObj);

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    setupButtonGroup();
    setupContextMenu();
    this->groupLayout()->addWidget(proxy);

    updateUI();
}

TaskShapeBinder::~TaskShapeBinder() = default;

void TaskShapeBinder::updateUI()
{
    Gui::Document* doc = vp->getDocument();

    //add initial values
    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;

    PartDesign::ShapeBinder::getFilteredReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, subs);

    if (obj) {
        ui->baseEdit->setText(QString::fromStdString(obj->Label.getStrValue()));
    }

    // Allow to clear the Support
    ui->baseEdit->setClearButtonEnabled(true);
    connect(ui->baseEdit, &QLineEdit::textChanged,
            this, &TaskShapeBinder::supportChanged);

    for (const auto& sub : subs)
        ui->listWidgetReferences->addItem(QString::fromStdString(sub));

    if (obj) {
        auto* svp = doc->getViewProvider(obj);
        if (svp) {
            supportShow = svp->isShow();
            svp->setVisible(true);
        }
    }
}

void TaskShapeBinder::setupButtonGroup()
{
    buttonGroup = new ButtonGroup(this);
    buttonGroup->setExclusive(true);

    buttonGroup->addButton(ui->buttonRefAdd,
                           TaskShapeBinder::refAdd);
    buttonGroup->addButton(ui->buttonRefRemove,
                           TaskShapeBinder::refRemove);
    buttonGroup->addButton(ui->buttonBase,
                           TaskShapeBinder::refObjAdd);
    connect(buttonGroup, qOverload<QAbstractButton *, bool>(&QButtonGroup::buttonToggled),
            this, &TaskShapeBinder::onButtonToggled);
}

void TaskShapeBinder::setupContextMenu()
{
    // Create context menu
    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QKeySequence::Delete);
    remove->setShortcutContext(Qt::WidgetShortcut);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // display shortcut behind the context menu entry
    remove->setShortcutVisibleInContextMenu(true);
#endif
    ui->listWidgetReferences->addAction(remove);
    connect(remove, &QAction::triggered, this, &TaskShapeBinder::deleteItem);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void TaskShapeBinder::supportChanged(const QString& text)
{
    if (!vp.expired() && text.isEmpty()) {
        PartDesign::ShapeBinder* binder = static_cast<PartDesign::ShapeBinder*>(vp->getObject());
        binder->Support.setValue(nullptr, nullptr);
        vp->highlightReferences(false);
        vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
        ui->listWidgetReferences->clear();
    }
}

void TaskShapeBinder::onButtonToggled(QAbstractButton *button, bool checked)
{
    int id = buttonGroup->id(button);

    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = static_cast<TaskShapeBinder::selectionModes>(id);
    }
    else {
        Gui::Selection().clearSelection();
        if (selectionMode == static_cast<TaskShapeBinder::selectionModes>(id))
            selectionMode = TaskShapeBinder::none;
    }

    switch (id) {
    case TaskShapeBinder::refAdd:
    case TaskShapeBinder::refRemove:
        if (!vp.expired())
            vp->highlightReferences(true);
        break;
    case TaskShapeBinder::refObjAdd:
        break;
    default:
        break;
    }
}

void TaskShapeBinder::changeEvent(QEvent*)
{
}

void TaskShapeBinder::deleteItem()
{
    if (vp.expired())
        return;

    // Delete the selected spine
    int row = ui->listWidgetReferences->currentRow();
    QListWidgetItem* item = ui->listWidgetReferences->takeItem(row);
    if (item) {
        QByteArray data = item->text().toLatin1();
        delete item;

        // search inside the list of sub-elements
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> subs;

        PartDesign::ShapeBinder* binder = static_cast<PartDesign::ShapeBinder*>(vp->getObject());
        PartDesign::ShapeBinder::getFilteredReferences(&binder->Support, obj, subs);

        std::string subname = data.constData();
        std::vector<std::string>::iterator it = std::find(subs.begin(), subs.end(), subname);

        // if something was found, delete it and update the support
        if (it != subs.end()) {
            subs.erase(it);
            binder->Support.setValue(obj, subs);

            vp->highlightReferences(false);
            vp->getObject()->getDocument()->recomputeFeature(vp->getObject());

            clearButtons();
        }
    }
}

void TaskShapeBinder::removeFromListWidget(QListWidget* widget, QString itemstr)
{
    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (auto item : items) {
            QListWidgetItem* it = widget->takeItem(widget->row(item));
            delete it;
        }
    }
}

void TaskShapeBinder::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    auto setObjectLabel = [=](const Gui::SelectionChanges& msg) {
        App::DocumentObject* obj = msg.Object.getObject();
        if (obj) {
            ui->baseEdit->setText(QString::fromStdString(obj->Label.getStrValue()));
        }
    };

    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromUtf8(msg.pSubName);
                if (!sub.isEmpty())
                    ui->listWidgetReferences->addItem(sub);

                setObjectLabel(msg);
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromUtf8(msg.pSubName);
                if (!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, sub);
            }
            else if (selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                setObjectLabel(msg);
            }

            clearButtons();

            if (!vp.expired()) {
                vp->highlightReferences(false);
                vp->getObject()->getDocument()->recomputeFeature(vp->getObject());
            }
        }

        clearButtons();
        exitSelectionMode();
    }
}

bool TaskShapeBinder::referenceSelected(const SelectionChanges& msg) const
{
    if (vp.expired())
        return false;

    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
        (selectionMode == refAdd) || (selectionMode == refRemove) || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;

        //change the references
        std::string subName(msg.pSubName);

        Part::Feature* selectedObj = nullptr;
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> refs;

        PartDesign::ShapeBinder::getFilteredReferences(&static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support, obj, refs);

        // get selected object
        auto docObj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
        if (docObj && docObj->isDerivedFrom(Part::Feature::getClassTypeId())) {
            selectedObj = static_cast<Part::Feature*>(docObj);
        }

        // ensure we have a valid object
        if (!selectedObj) {
            return false;
        }
        if (!obj) {
            // Support has not been set before
            obj = selectedObj;
        }

        if (selectionMode != refObjAdd) {
            // ensure the new selected subref belongs to the same object
            if (strcmp(msg.pObjectName, obj->getNameInDocument()) != 0)
                return false;

            std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(subName);
                else
                    return false; // duplicate selection
            }
            else {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }
        }
        else {
            // change object
            refs.clear();
            obj = selectedObj;
        }

        static_cast<PartDesign::ShapeBinder*>(vp->getObject())->Support.setValue(obj, refs);

        return true;
    }

    return false;
}

void TaskShapeBinder::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonBase->setChecked(false);
}

void TaskShapeBinder::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

void TaskShapeBinder::accept()
{
    if (vp.expired())
        return;

    std::string label = ui->baseEdit->text().toStdString();
    PartDesign::ShapeBinder* binder = static_cast<PartDesign::ShapeBinder*>(vp->getObject());
    if (!binder->Support.getValue() && !label.empty()) {
        auto mode = selectionMode;
        selectionMode = refObjAdd;
        SelectionChanges msg(SelectionChanges::AddSelection, binder->getDocument()->getName(), label.c_str());
        referenceSelected(msg);
        selectionMode = mode;
    }
}

//**************************************************************************
//**************************************************************************
// TaskDlgShapeBinder
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgShapeBinder::TaskDlgShapeBinder(ViewProviderShapeBinder* view, bool newObj)
    : Gui::TaskView::TaskDialog()
    , vp(view)
{
    assert(view);
    parameter = new TaskShapeBinder(view, newObj);

    Content.push_back(parameter);
}

TaskDlgShapeBinder::~TaskDlgShapeBinder() = default;

bool TaskDlgShapeBinder::accept()
{
    try {
        if (!vp.expired()) {
            parameter->accept();

            Gui::cmdAppDocument(vp->getObject(), "recompute()");
            if (!vp->getObject()->isValid())
                throw Base::RuntimeError(vp->getObject()->getStatusString());
            Gui::cmdGuiDocument(vp->getObject(), "resetEdit()");
            Gui::Command::commitCommand();
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QApplication::translate("Exception", e.what()));
        return false;
    }

    return true;
}

bool TaskDlgShapeBinder::reject()
{
    if (!vp.expired()) {
        App::Document* doc = vp->getObject()->getDocument();
        // roll back the done things (deletes 'vp')
        Gui::Command::abortCommand();
        Gui::cmdGuiDocument(doc, "resetEdit()");
        Gui::cmdAppDocument(doc, "recompute()");
    }
    return true;
}

#include "moc_TaskShapeBinder.cpp"
