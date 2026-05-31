// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
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


#include <QAction>
#include <QCoreApplication>
#include <QMessageBox>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/AsyncPreviewSession.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Tools.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>

#include "ui_TaskBooleanParameters.h"
#include "TaskBooleanParameters.h"
#include "DeferredDialogRejectUtils.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskBooleanParameters */

namespace
{
void setDocumentObjectVisible(App::DocumentObject* object, bool visible)
{
    auto* viewProvider = dynamic_cast<Gui::ViewProviderDocumentObject*>(
        Gui::Application::Instance->getViewProvider(object)
    );
    if (!viewProvider) {
        return;
    }

    if (visible) {
        viewProvider->show();
    }
    else {
        viewProvider->hide();
    }
}
}  // namespace

TaskBooleanParameters::TaskBooleanParameters(ViewProviderBoolean* BooleanView, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Boolean"), tr("Boolean Parameters"), true, parent)
    , ui(new Ui_TaskBooleanParameters)
    , BooleanView(BooleanView)
{
    selectionMode = none;

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    AsyncPreviewController::Callbacks callbacks;
    callbacks.makeRequest = [this]() {
        auto* object = this->BooleanView ? this->BooleanView->getObject() : nullptr;
        return object ? App::RecomputeRequest::fromDocumentObject(*object) : App::RecomputeRequest {};
    };
    callbacks.runSync = [this]() {
        if (auto* boolean = this->BooleanView ? this->BooleanView->getObject<PartDesign::Boolean>()
                                              : nullptr) {
            boolean->getDocument()->recomputeFeature(boolean);
        }
    };
    callbacks.onAppliedResult = [](bool, bool) {
    };
    asyncPreviewSession = std::make_unique<Gui::AsyncPreviewSession>(std::move(callbacks), this);
    connect(
        asyncPreviewSession->controller(),
        &AsyncPreviewController::recomputeSettled,
        this,
        &TaskBooleanParameters::recomputeSettled
    );
    asyncPreviewSession->bindWidgets(
        {
            ui->previewStatusWidget,
            ui->progressBarPreview,
            ui->labelPreviewStatus,
            ui->buttonCancelPreview,
        },
        [](const char* text) { return TaskBooleanParameters::tr(text); },
        proxy
    );

    // clang-format off
    connect(ui->buttonBodyAdd, &QToolButton::toggled,
            this, &TaskBooleanParameters::onButtonBodyAdd);
    connect(ui->buttonBodyRemove, &QToolButton::toggled,
            this, &TaskBooleanParameters::onButtonBodyRemove);
    // clang-format on

    this->groupLayout()->addWidget(proxy);

    PartDesign::Boolean* pcBoolean = BooleanView->getObject<PartDesign::Boolean>();
    std::vector<App::DocumentObject*> bodies = pcBoolean->Group.getValues();
    for (auto body : bodies) {
        QListWidgetItem* item = new QListWidgetItem(ui->listWidgetBodies);
        item->setText(QString::fromUtf8(body->Label.getValue()));
        item->setData(Qt::UserRole, QString::fromLatin1(body->getNameInDocument()));
    }

    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    action->setShortcut(Gui::QtTools::deleteKeySequence());

    // display shortcut behind the context menu entry
    action->setShortcutVisibleInContextMenu(true);

    ui->listWidgetBodies->addAction(action);
    connect(action, &QAction::triggered, this, &TaskBooleanParameters::onBodyDeleted);
    ui->listWidgetBodies->setContextMenuPolicy(Qt::ActionsContextMenu);

    int index = pcBoolean->Type.getValue();
    ui->comboType->setCurrentIndex(index);

    connect(
        ui->comboType,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskBooleanParameters::onTypeChanged
    );
}

void TaskBooleanParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none) {
        return;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (strcmp(msg.pDocName, BooleanView->getObject()->getDocument()->getName()) != 0) {
            return;
        }

        // get the selected object
        PartDesign::Boolean* pcBoolean = BooleanView->getObject<PartDesign::Boolean>();
        std::string body(msg.pObjectName);
        if (body.empty()) {
            return;
        }
        App::DocumentObject* pcBody = pcBoolean->getDocument()->getObject(body.c_str());
        if (!pcBody) {
            return;
        }

        // if the selected object is not a body then get the body it is part of
        if (!pcBody->isDerivedFrom<PartDesign::Body>()) {
            pcBody = PartDesign::Body::findBodyOf(pcBody);
            if (!pcBody) {
                return;
            }
            body = pcBody->getNameInDocument();
        }

        std::vector<App::DocumentObject*> bodies = pcBoolean->Group.getValues();

        if (selectionMode == bodyAdd) {
            if (std::ranges::find(bodies, pcBody) == bodies.end()) {
                bodies.push_back(pcBody);
                pcBoolean->Group.setValues(std::vector<App::DocumentObject*>());
                pcBoolean->addObjects(bodies);

                QListWidgetItem* item = new QListWidgetItem(ui->listWidgetBodies);
                item->setText(QString::fromUtf8(pcBody->Label.getValue()));
                item->setData(Qt::UserRole, QString::fromLatin1(pcBody->getNameInDocument()));

                requestRecompute(/*waitForCompletion=*/false);
                ui->buttonBodyAdd->setChecked(false);

                // Hide the bodies
                if (bodies.size() == 1) {
                    // Hide base body and added body
                    setDocumentObjectVisible(pcBoolean->BaseFeature.getValue(), false);
                    setDocumentObjectVisible(bodies.front(), false);
                    BooleanView->show();
                }
                else {
                    // Hide newly added body
                    setDocumentObjectVisible(bodies.back(), false);
                }
            }
        }
        else if (selectionMode == bodyRemove) {
            if (const auto b = std::ranges::find(bodies, pcBody); b != bodies.end()) {
                bodies.erase(b);
                pcBoolean->setObjects(bodies);

                const QString internalName = QString::fromStdString(body);
                for (int row = 0; row < ui->listWidgetBodies->count(); row++) {
                    QListWidgetItem* item = ui->listWidgetBodies->item(row);
                    QString name = item->data(Qt::UserRole).toString();
                    if (name == internalName) {
                        ui->listWidgetBodies->takeItem(row);
                        delete item;
                        break;
                    }
                }

                requestRecompute(/*waitForCompletion=*/false);
                ui->buttonBodyRemove->setChecked(false);

                // Make bodies visible again
                setDocumentObjectVisible(pcBody, true);
                if (bodies.empty()) {
                    setDocumentObjectVisible(pcBoolean->BaseFeature.getValue(), true);
                    BooleanView->hide();
                }
            }
        }
    }
}

void TaskBooleanParameters::onButtonBodyAdd(bool checked)
{
    if (checked) {
        PartDesign::Boolean* pcBoolean = BooleanView->getObject<PartDesign::Boolean>();
        Gui::Document* doc = BooleanView->getDocument();
        BooleanView->hide();
        if (pcBoolean->Group.getValues().empty() && pcBoolean->BaseFeature.getValue()) {
            doc->setHide(pcBoolean->BaseFeature.getValue()->getNameInDocument());
        }
        selectionMode = bodyAdd;
        Gui::Selection().clearSelection();
    }
    else {
        exitSelectionMode();
    }

    ui->buttonBodyRemove->setDisabled(checked);
}

void TaskBooleanParameters::onButtonBodyRemove(bool checked)
{
    if (checked) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            BooleanView->show();
        }
        selectionMode = bodyRemove;
        Gui::Selection().clearSelection();
    }
    else {
        exitSelectionMode();
    }

    ui->buttonBodyAdd->setDisabled(checked);
}

void TaskBooleanParameters::onTypeChanged(int index)
{
    PartDesign::Boolean* pcBoolean = BooleanView->getObject<PartDesign::Boolean>();

    switch (index) {
        case 0:
            pcBoolean->Type.setValue("Fuse");
            break;
        case 1:
            pcBoolean->Type.setValue("Cut");
            break;
        case 2:
            pcBoolean->Type.setValue("Common");
            break;
        default:
            pcBoolean->Type.setValue("Fuse");
    }

    requestRecompute(/*waitForCompletion=*/false);
}

const std::vector<std::string> TaskBooleanParameters::getBodies() const
{
    std::vector<std::string> result;
    for (int i = 0; i < ui->listWidgetBodies->count(); i++) {
        result.push_back(ui->listWidgetBodies->item(i)->data(Qt::UserRole).toString().toStdString());
    }
    return result;
}

int TaskBooleanParameters::getType() const
{
    return ui->comboType->currentIndex();
}

void TaskBooleanParameters::onBodyDeleted()
{
    PartDesign::Boolean* pcBoolean = BooleanView->getObject<PartDesign::Boolean>();
    std::vector<App::DocumentObject*> bodies = pcBoolean->Group.getValues();
    int index = ui->listWidgetBodies->currentRow();
    if (index < 0 || static_cast<size_t>(index) >= bodies.size()) {
        return;
    }

    App::DocumentObject* body = bodies[index];
    QString internalName = ui->listWidgetBodies->item(index)->data(Qt::UserRole).toString();
    for (auto it = bodies.begin(); it != bodies.end(); ++it) {
        if (internalName == QLatin1String((*it)->getNameInDocument())) {
            body = *it;
            bodies.erase(it);
            break;
        }
    }

    ui->listWidgetBodies->model()->removeRow(index);
    pcBoolean->setObjects(bodies);
    requestRecompute(/*waitForCompletion=*/false);

    // Make bodies visible again
    setDocumentObjectVisible(body, true);
    if (bodies.empty()) {
        setDocumentObjectVisible(pcBoolean->BaseFeature.getValue(), true);
        BooleanView->hide();
    }
}

TaskBooleanParameters::~TaskBooleanParameters()
{
    stopPendingRecompute();
}

bool TaskBooleanParameters::hasOutstandingRecompute() const
{
    return asyncPreviewSession && asyncPreviewSession->hasOutstandingRecompute();
}

bool TaskBooleanParameters::canReuseAcceptedPreviewResult() const
{
    return asyncPreviewSession && asyncPreviewSession->didLastRecomputeSucceed();
}

void TaskBooleanParameters::setDeferredClosePending(bool pending)
{
    if (asyncPreviewSession) {
        asyncPreviewSession->setDeferredClosePending(pending);
    }
}

Gui::AsyncInlineRecomputeProgressTarget TaskBooleanParameters::makeAcceptedRecomputeProgressTarget(
    QDialogButtonBox* dialogButtonBox,
    const QString& statusText
)
{
    if (!asyncPreviewSession) {
        return {};
    }

    return asyncPreviewSession->makeInlineRecomputeProgressTarget(this, dialogButtonBox, statusText);
}

void TaskBooleanParameters::updateRecomputeUi()
{
    if (!ui || !asyncPreviewSession) {
        return;
    }
    asyncPreviewSession->updateUi();
}

void TaskBooleanParameters::flushPendingRecompute()
{
    if (asyncPreviewSession) {
        asyncPreviewSession->flushPendingRecompute();
    }
}

void TaskBooleanParameters::stopPendingRecompute()
{
    if (asyncPreviewSession) {
        asyncPreviewSession->stopPendingRecompute();
    }
}

void TaskBooleanParameters::requestRecompute(bool waitForCompletion)
{
    if (asyncPreviewSession) {
        asyncPreviewSession->requestRecompute(waitForCompletion);
    }
}

void TaskBooleanParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->comboType->blockSignals(true);
        int index = ui->comboType->currentIndex();
        ui->retranslateUi(proxy);
        ui->comboType->setCurrentIndex(index);
        ui->comboType->blockSignals(false);
        updateRecomputeUi();
    }
}

void TaskBooleanParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        doc->setShow(BooleanView->getObject()->getNameInDocument());
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgBooleanParameters::TaskDlgBooleanParameters(ViewProviderBoolean* BooleanView)
    : TaskDlgFeatureParameters(BooleanView)
    , BooleanView(BooleanView)
{
    assert(BooleanView);
    parameter = new TaskBooleanParameters(BooleanView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgBooleanParameters::~TaskDlgBooleanParameters() = default;

//==== calls from the TaskView ===============================================================


void TaskDlgBooleanParameters::open()
{}

void TaskDlgBooleanParameters::clicked(int)
{}

void TaskDlgBooleanParameters::ensureDeferredRejectConnection()
{
    ensureDeferredDialogRejectConnection(
        deferredReject,
        parameter,
        &TaskBooleanParameters::recomputeSettled,
        this,
        &TaskDlgBooleanParameters::onParameterRecomputeSettled
    );
}

void TaskDlgBooleanParameters::setDeferredRejectPending(bool pending)
{
    setDeferredDialogRejectPending(deferredReject, pending, buttonBox, [this](bool pending) {
        if (parameter) {
            parameter->setDeferredClosePending(pending);
        }
    });
}

bool TaskDlgBooleanParameters::accept()
{
    if (deferredReject.pending) {
        return false;
    }

    ensureDeferredRejectConnection();
    auto obj = BooleanView->getObject();
    App::Document* document = obj ? obj->getDocument() : nullptr;
    if (!obj || !obj->isAttachedToDocument() || !document) {
        return false;
    }
    BooleanView->Visibility.setValue(true);

    try {
        std::vector<std::string> bodies = parameter->getBodies();
        if (bodies.empty()) {
            QMessageBox::warning(parameter, tr("Empty body list"), tr("The body list cannot be empty"));
            return false;
        }
        const bool previewSettled = !parameter->hasOutstandingRecompute()
            && parameter->canReuseAcceptedPreviewResult();
        if (!previewSettled) {
            parameter->stopPendingRecompute();
        }
        else {
            parameter->flushPendingRecompute();
        }
        std::stringstream str;
        str << Gui::Command::getObjectCmd(obj) << ".setObjects( [";
        for (const auto& body : bodies) {
            str << "App.getDocument('" << obj->getDocument()->getName() << "').getObject('" << body
                << "'),";
        }
        str << "])";
        Gui::Command::runCommand(Gui::Command::Doc, str.str().c_str());
        FCMD_OBJ_CMD(obj, "Type = " << parameter->getType());

        if (previewSettled) {
            // The async preview already settled the latest boolean state. Clear the
            // redundant touch from the command serialization above so the final
            // document recompute only updates downstream dependents.
            Gui::cmdAppObject(obj, "purgeTouched()");
            for (auto parent : obj->getInList()) {
                parent->touch();
            }
        }

        auto progressTarget
            = parameter->makeAcceptedRecomputeProgressTarget(buttonBox, tr("Applying changes..."));
        Gui::AsyncRecomputeDialogOptions options;
        options.cancelable = false;
        options.dynamicLabel = false;
        options.forceIndeterminate = true;
        options.inlineProgressTarget = progressTarget;
        if (!runAsyncAcceptDocumentRecompute(document, options)) {
            return false;
        }
        Gui::cmdGuiDocument(document, "resetEdit()");
        document->commitTransaction();
    }
    catch (const Base::Exception& e) {
        document->abortTransaction();
        QMessageBox::warning(
            parameter,
            tr("Boolean: Accept: Input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }
    return true;
}

bool TaskDlgBooleanParameters::performReject()
{
    if (!parameter) {
        return false;
    }

    // Show the bodies again
    PartDesign::Boolean* obj = BooleanView->getObject<PartDesign::Boolean>();
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        if (obj->BaseFeature.getValue()) {
            doc->setShow(obj->BaseFeature.getValue()->getNameInDocument());
            std::vector<App::DocumentObject*> bodies = obj->Group.getValues();
            for (auto body : bodies) {
                doc->setShow(body->getNameInDocument());
            }
        }
        // roll back the done things
        doc->abortCommand();
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    }

    return true;
}

bool TaskDlgBooleanParameters::reject()
{
    if (!parameter) {
        return false;
    }

    ensureDeferredRejectConnection();
    parameter->stopPendingRecompute();
    if (!parameter->hasOutstandingRecompute()) {
        return performReject();
    }

    if (!deferredReject.pending) {
        auto* object = BooleanView ? BooleanView->getObject() : nullptr;
        deferredReject.documentName = object && object->getDocument()
            ? std::string(object->getDocument()->getName())
            : std::string();
        setDeferredRejectPending(true);
    }

    return false;
}

void TaskDlgBooleanParameters::onParameterRecomputeSettled()
{
    finishDeferredDialogReject(
        this,
        deferredReject,
        parameter && !parameter->hasOutstandingRecompute(),
        [this]() { return performReject(); },
        [this](bool pending) { setDeferredRejectPending(pending); }
    );
}

#include "moc_TaskBooleanParameters.cpp"
