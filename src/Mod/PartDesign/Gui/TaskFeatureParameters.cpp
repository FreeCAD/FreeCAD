// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <QApplication>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPointer>

#include <App/DocumentObserver.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "ui_TaskPreviewParameters.h"

#include "TaskFeatureParameters.h"
#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

namespace
{

Gui::AsyncRecomputeDialogOptions acceptedFeatureRecomputeDialogOptions()
{
    Gui::AsyncRecomputeDialogOptions options;
    options.cancelable = false;
    options.dynamicLabel = false;
    options.forceIndeterminate = true;
    return options;
}

Gui::AsyncRecomputeDialogOptions acceptedFeatureRecomputeDialogOptions(bool showDialog)
{
    auto options = acceptedFeatureRecomputeDialogOptions();
    options.showDialog = showDialog;
    return options;
}

Gui::AsyncRecomputeDialogOptions rollbackRecomputeDialogOptions()
{
    Gui::AsyncRecomputeDialogOptions options;
    options.showDelayMs = 450;
    options.cancelable = false;
    options.dynamicLabel = false;
    options.forceIndeterminate = true;
    return options;
}

}  // namespace

/*********************************************************************
 *                      Task Feature Parameters                      *
 *********************************************************************/

TaskPreviewParameters::TaskPreviewParameters(ViewProvider* vp, QWidget* parent)
    : TaskBox(BitmapFactory().pixmap("tree-pre-sel"), tr("Preview"), true, parent)
    , vp(vp)
    , ui(std::make_unique<Ui_TaskPreviewParameters>())
{
    vp->showPreviousFeature(!hGrp->GetBool("ShowFinal", false));
    vp->showPreview(hGrp->GetBool("ShowTransparentPreview", true));

    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);

    ui->showFinalCheckBox->setChecked(vp->isVisible());
    ui->showTransparentPreviewCheckBox->setChecked(vp->isPreviewEnabled());

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(
        ui->showTransparentPreviewCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &TaskPreviewParameters::onShowPreviewChanged
    );
    connect(
        ui->showFinalCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &TaskPreviewParameters::onShowFinalChanged
    );
#else
    connect(
        ui->showTransparentPreviewCheckBox,
        &QCheckBox::stateChanged,
        this,
        &TaskPreviewParameters::onShowPreviewChanged
    );
    connect(
        ui->showFinalCheckBox,
        &QCheckBox::stateChanged,
        this,
        &TaskPreviewParameters::onShowFinalChanged
    );
#endif

    groupLayout()->addWidget(proxy);
}

TaskPreviewParameters::~TaskPreviewParameters() = default;

void TaskPreviewParameters::onShowFinalChanged(bool show)
{
    vp->showPreviousFeature(!show);
}

void TaskPreviewParameters::onShowPreviewChanged(bool show)
{
    vp->showPreview(show);
}

TaskFeatureParameters::TaskFeatureParameters(
    PartDesignGui::ViewProvider* vp,
    QWidget* parent,
    const std::string& pixmapname,
    const QString& parname
)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname.c_str()), parname, true, parent)
    , vp(vp)
    , blockUpdate(false)
{
    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);
}

TaskFeatureParameters::~TaskFeatureParameters()
{
    hideDraggerHints();
}

void TaskFeatureParameters::showDraggerHints()
{
    if (!Gui::GizmoContainer::isEnabled() || !Gui::GizmoContainer::isCoarseSnapEnabled()) {
        return;
    }

    const Gui::InputHint::UserInput key = Gui::GizmoContainer::getFineSnapKey();
    const bool coarseByDefault = Gui::GizmoContainer::isCoarseByDefault();

    QString message;
    if (coarseByDefault) {
        message = tr("%1 fine dragging");
    }
    else {
        message = tr("%1 coarse dragging");
    }

    Gui::getMainWindow()->showHints({{
        .message = message,
        .sequences = {{key}},
    }});
}

void TaskFeatureParameters::hideDraggerHints()
{
    Gui::getMainWindow()->hideHints();
}

void TaskFeatureParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj) {
        this->vp = nullptr;
    }
}

void TaskFeatureParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    recomputeFeature();
}

void TaskFeatureParameters::recomputeFeature()
{
    if (!blockUpdate) {
        auto* feature = getObject<PartDesign::Feature>();
        assert(feature);

        feature->recomputeFeature();
        feature->recomputePreview();
    }
}

bool TaskFeatureParameters::hasAcceptedRecomputeProgressUi() const
{
    return false;
}

void TaskFeatureParameters::setAcceptedRecomputePending(bool, const QString&)
{}

Gui::AsyncInlineRecomputeProgressTarget TaskFeatureParameters::makeAcceptedRecomputeProgressTarget(
    QDialogButtonBox* dialogButtonBox,
    const QString& statusText
)
{
    if (!hasAcceptedRecomputeProgressUi()) {
        return {};
    }

    QPointer<TaskFeatureParameters> guard(this);
    Gui::AsyncInlineRecomputeProgressTarget target;
    target.contentWidget = this;
    target.buttonBox = dialogButtonBox;
    target.statusText = statusText;
    target.setPending = [guard](bool pending, const QString& status) {
        if (guard) {
            guard->setAcceptedRecomputePending(pending, status);
        }
    };
    return target;
}

/*********************************************************************
 *                            Task Dialog                            *
 *********************************************************************/
TaskDlgFeatureParameters::TaskDlgFeatureParameters(PartDesignGui::ViewProvider* vp)
    : preview(new TaskPreviewParameters(vp))
    , vp(vp)
{
    assert(vp);
}

TaskDlgFeatureParameters::~TaskDlgFeatureParameters() = default;

TaskDlgFeatureParameters::AcceptPendingRecomputeAction TaskDlgFeatureParameters::acceptPendingRecomputeAction() const
{
    return AcceptPendingRecomputeAction::Flush;
}

TaskDlgFeatureParameters::AcceptRecomputeMode TaskDlgFeatureParameters::acceptRecomputeMode(
    bool,
    AcceptPendingRecomputeAction
) const
{
    return AcceptRecomputeMode::AsyncDocument;
}

bool TaskDlgFeatureParameters::hasDeferredRejectPending() const
{
    return deferredReject.pending;
}

bool TaskDlgFeatureParameters::finishRejectOrDefer(App::DocumentObject* object)
{
    if (!deferredRejectReady || !deferredRejectAction || !deferredRejectSetPending) {
        return false;
    }

    if (deferredRejectReady()) {
        return deferredRejectAction();
    }

    if (!deferredReject.pending) {
        deferredReject.documentName = object && object->getDocument()
            ? std::string(object->getDocument()->getName())
            : std::string();
        deferredRejectSetPending(true);
    }

    return false;
}

void TaskDlgFeatureParameters::onDeferredRejectRecomputeSettled()
{
    if (!deferredRejectReady || !deferredRejectAction || !deferredRejectSetPending) {
        return;
    }

    finishDeferredDialogReject(
        this,
        deferredReject,
        deferredRejectReady(),
        [this]() { return deferredRejectAction ? deferredRejectAction() : false; },
        [this](bool pending) {
            if (deferredRejectSetPending) {
                deferredRejectSetPending(pending);
            }
        }
    );
}

void TaskDlgFeatureParameters::clearDeferredRejectHandlers()
{
    deferredRejectReady = {};
    deferredRejectAction = {};
    deferredRejectSetPending = {};
}

bool PartDesignGui::runAsyncAcceptDocumentRecompute(App::Document* document)
{
    return runAsyncAcceptDocumentRecompute(document, acceptedFeatureRecomputeDialogOptions());
}

bool PartDesignGui::runAsyncAcceptDocumentRecompute(
    App::Document* document,
    const Gui::AsyncRecomputeDialogOptions& options
)
{
    if (!document) {
        throw Base::RuntimeError("Feature document is not available.");
    }

    const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
        Gui::getMainWindow(),
        QApplication::translate("PartDesignGui::TaskDlgFeatureParameters", "Feature parameters"),
        QApplication::translate("PartDesignGui::TaskDlgFeatureParameters", "Applying changes..."),
        document,
        /*force=*/false,
        options,
        [document]() {
            if (document) {
                document->recompute();
            }
        }
    );
    if (!outcome.success) {
        if (outcome.canceled) {
            return false;
        }
        throw Base::RuntimeError(
            outcome.message.empty() ? "Feature recompute failed" : outcome.message
        );
    }

    return true;
}

bool TaskDlgFeatureParameters::applyAcceptedFeatureParameters(
    AcceptPendingRecomputeAction pendingRecomputeAction,
    bool& isUpdateBlocked
)
{
    isUpdateBlocked = false;

    // Iterate over parameter dialogs and apply all parameters from them.
    for (QWidget* wgt : Content) {
        TaskFeatureParameters* param = qobject_cast<TaskFeatureParameters*>(wgt);
        if (!param) {
            continue;
        }

        switch (pendingRecomputeAction) {
            case AcceptPendingRecomputeAction::Flush:
                param->flushPendingRecompute();
                break;
            case AcceptPendingRecomputeAction::Stop:
                param->stopPendingRecompute();
                break;
        }

        param->saveHistory();
        param->apply();
        isUpdateBlocked |= param->isUpdateBlocked();
    }

    return true;
}

void TaskDlgFeatureParameters::prepareAcceptedFeatureForDocumentRecompute(
    App::DocumentObject* feature,
    bool isUpdateBlocked,
    AcceptPendingRecomputeAction pendingRecomputeAction
)
{
    if (isUpdateBlocked || pendingRecomputeAction == AcceptPendingRecomputeAction::Stop) {
        return;
    }

    // object was already computed, nothing more to do with it...
    Gui::cmdAppDocument(feature, "purgeTouched()");

    if (!feature->isValid()) {
        throw Base::RuntimeError(getObject()->getStatusString());
    }

    // ...but touch parents to signal the change...
    for (auto obj : feature->getInList()) {
        obj->touch();
    }
}

bool TaskDlgFeatureParameters::runAcceptedFeatureRecompute(
    App::Document* document,
    AcceptRecomputeMode mode,
    bool hasInlineProgress
)
{
    switch (mode) {
        case AcceptRecomputeMode::AsyncDocument:
            return runAsyncAcceptDocumentRecompute(
                document,
                acceptedFeatureRecomputeDialogOptions(!hasInlineProgress)
            );
        case AcceptRecomputeMode::CommandDocument:
            Gui::cmdAppDocument(document, "recompute()");
            return true;
    }

    return true;
}

void TaskDlgFeatureParameters::finalizeAcceptedFeature(App::DocumentObject* feature)
{
    if (!feature->isValid()) {
        throw Base::RuntimeError(getObject()->getStatusString());
    }

    App::DocumentObject* previous = static_cast<PartDesign::Feature*>(feature)->getBaseObject(
        /* silent = */ true
    );
    Gui::cmdAppObjectHide(previous);

    // detach the task panel from the selection to avoid to invoke
    // eventually onAddSelection when the selection changes
    std::vector<QWidget*> subwidgets = getDialogContent();
    for (auto it : subwidgets) {
        TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
        if (param) {
            param->detachSelection();
        }
    }

    Gui::cmdGuiDocument(feature, "resetEdit()");
    feature->getDocument()->commitTransaction();
}

bool TaskDlgFeatureParameters::reportAcceptException(const Base::Exception& e) const
{
    QString errorText = QString::fromUtf8(e.what());
    QString statusText = QString::fromUtf8(getObject()->getStatusString());

    // generic, fallback error message
    if (errorText == QStringLiteral("Error") || errorText.isEmpty()) {
        if (!statusText.isEmpty() && statusText != QStringLiteral("Error")) {
            errorText = statusText;
        }
        else {
            errorText = tr(
                "The feature could not be created with the given parameters.\n"
                "The geometry may be invalid or the parameters may be incompatible.\n"
                "Adjust the parameters and try again."
            );
        }
    }
    Base::Console().error("%s\n", errorText.toUtf8().constData());
    return false;
}

bool TaskDlgFeatureParameters::accept()
{
    App::DocumentObject* feature = getObject();
    bool isUpdateBlocked = false;
    auto pendingRecomputeAction = acceptPendingRecomputeAction();
    if (pendingRecomputeAction == AcceptPendingRecomputeAction::Flush) {
        for (QWidget* widget : Content) {
            auto* param = qobject_cast<TaskFeatureParameters*>(widget);
            if (!param) {
                continue;
            }

            if (!param->canReuseAcceptedPreviewResult()) {
                pendingRecomputeAction = AcceptPendingRecomputeAction::Stop;
                break;
            }
        }
    }
    try {
        applyAcceptedFeatureParameters(pendingRecomputeAction, isUpdateBlocked);
        // Make sure the feature is what we are expecting
        // Should be fine but you never know...
        if (!feature->isDerivedFrom<PartDesign::Feature>()) {
            throw Base::TypeError("Bad object processed in the feature dialog.");
        }

        App::Document* document = feature->getDocument();
        if (!document) {
            throw Base::RuntimeError("Feature document is not available.");
        }

        prepareAcceptedFeatureForDocumentRecompute(feature, isUpdateBlocked, pendingRecomputeAction);
        const auto recomputeMode = acceptRecomputeMode(isUpdateBlocked, pendingRecomputeAction);
        Gui::AsyncInlineRecomputeProgressTarget progressTarget;
        if (recomputeMode == AcceptRecomputeMode::AsyncDocument) {
            for (QWidget* widget : Content) {
                auto* param = qobject_cast<TaskFeatureParameters*>(widget);
                if (!param) {
                    continue;
                }

                progressTarget = param->makeAcceptedRecomputeProgressTarget(
                    buttonBox,
                    tr("Applying changes...")
                );
                if (progressTarget) {
                    break;
                }
            }
        }

        const Gui::ScopedAsyncInlineRecomputeProgress inlineProgress(std::move(progressTarget));
        if (!runAcceptedFeatureRecompute(document, recomputeMode, inlineProgress.isActive())) {
            return false;
        }

        finalizeAcceptedFeature(feature);
    }
    catch (const Base::Exception& e) {
        return reportAcceptException(e);
    }
    return true;
}

bool TaskDlgFeatureParameters::reject()
{
    auto feature = getObject<PartDesign::Feature>();
    App::DocumentObjectWeakPtrT weakptr(feature);
    App::Document* document = feature->getDocument();

    PartDesign::Body* body = PartDesign::Body::findBodyOf(feature);

    // Find out previous feature we won't be able to do it after abort
    // (at least in the body case)
    App::DocumentObject* previous = feature->getBaseObject(/* silent = */ true);

    // detach the task panel from the selection to avoid to invoke
    // eventually onAddSelection when the selection changes
    std::vector<QWidget*> subwidgets = getDialogContent();
    for (auto it : subwidgets) {
        TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
        if (param) {
            param->detachSelection();
        }

        TaskFeatureParameters* featureParam = qobject_cast<TaskFeatureParameters*>(it);
        if (featureParam) {
            featureParam->stopPendingRecompute();
        }
    }

    // roll back the done things which may delete the feature
    document->abortTransaction();

    // if abort command deleted the object make the previous feature visible again
    if (weakptr.expired()) {
        // Make the tip or the previous feature visible again with preference to the previous one
        // TODO: ViewProvider::onDelete has the same code. May be this one is excess?
        if (previous && Gui::Application::Instance->getViewProvider(previous)) {
            Gui::Application::Instance->getViewProvider(previous)->show();
        }
        else if (body) {
            App::DocumentObject* tip = body->Tip.getValue();
            if (tip && Gui::Application::Instance->getViewProvider(tip)) {
                Gui::Application::Instance->getViewProvider(tip)->show();
            }
        }
    }

    if (document->mustExecute()) {
        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            Gui::getMainWindow(),
            tr("Feature parameters"),
            tr("Discarding changes..."),
            document,
            /*force=*/false,
            rollbackRecomputeDialogOptions(),
            [document]() {
                if (document) {
                    document->recompute();
                }
            }
        );
        if (!outcome.success && !outcome.canceled) {
            Base::Console().error(
                "%s\n",
                outcome.message.empty() ? "Feature rollback recompute failed"
                                        : outcome.message.c_str()
            );
        }
    }
    Gui::cmdGuiDocument(document, "resetEdit()");

    return true;
}

#include "moc_TaskFeatureParameters.cpp"
