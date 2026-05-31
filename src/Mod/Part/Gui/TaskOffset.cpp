// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <limits>

#include <QDialogButtonBox>
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
#include <Gui/DeferredDialogRejectUtils.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/FeatureOffset.h>

#include "TaskOffset.h"
#include "ui_TaskOffset.h"


using namespace PartGui;

class OffsetWidget::Private
{
public:
    static constexpr int AsyncPreviewDebounceMs = 150;

    Ui_TaskOffset ui {};
    Part::Offset* offset {nullptr};
    std::unique_ptr<Gui::AsyncPreviewSession> asyncPreviewSession;
};

/* TRANSLATOR PartGui::OffsetWidget */

OffsetWidget::OffsetWidget(Part::Offset* offset, QWidget* parent)
    : d(new Private())
{
    Q_UNUSED(parent);
    Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base");
    Gui::Command::runCommand(Gui::Command::App, "import Part");

    d->offset = offset;
    d->ui.setupUi(this);
    setupConnections();

    d->ui.spinOffset->setUnit(Base::Unit::Length);
    d->ui.spinOffset->setRange(-std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    d->ui.spinOffset->setSingleStep(0.1);
    d->ui.facesButton->hide();

    bool is_2d = d->offset->isDerivedFrom<Part::Offset2D>();
    d->ui.selfIntersection->setVisible(!is_2d);
    if (is_2d) {
        d->ui.modeType->removeItem(2);  // remove Recto-Verso mode, not supported by 2d offset
    }

    // block signals to fill values read out from feature...
    bool block = true;
    d->ui.fillOffset->blockSignals(block);
    d->ui.intersection->blockSignals(block);
    d->ui.selfIntersection->blockSignals(block);
    d->ui.modeType->blockSignals(block);
    d->ui.joinType->blockSignals(block);
    d->ui.spinOffset->blockSignals(block);

    // read values from feature
    d->ui.spinOffset->setValue(d->offset->Value.getValue());
    d->ui.fillOffset->setChecked(offset->Fill.getValue());
    d->ui.intersection->setChecked(offset->Intersection.getValue());
    d->ui.selfIntersection->setChecked(offset->SelfIntersection.getValue());
    long mode = offset->Mode.getValue();
    if (mode >= 0 && mode < d->ui.modeType->count()) {
        d->ui.modeType->setCurrentIndex(mode);
    }
    long join = offset->Join.getValue();
    if (join >= 0 && join < d->ui.joinType->count()) {
        d->ui.joinType->setCurrentIndex(join);
    }

    // unblock signals
    block = false;
    d->ui.fillOffset->blockSignals(block);
    d->ui.intersection->blockSignals(block);
    d->ui.selfIntersection->blockSignals(block);
    d->ui.modeType->blockSignals(block);
    d->ui.joinType->blockSignals(block);
    d->ui.spinOffset->blockSignals(block);

    d->ui.spinOffset->bind(d->offset->Value);

    Gui::AsyncPreviewController::Callbacks callbacks;
    callbacks.makeRequest = [this]() {
        auto* object = d->offset;
        return object ? App::RecomputeRequest::fromDocumentObject(*object) : App::RecomputeRequest {};
    };
    callbacks.runSync = [this]() {
        if (d->offset && d->offset->getDocument()) {
            d->offset->getDocument()->recomputeFeature(d->offset);
        }
    };
    d->asyncPreviewSession = std::make_unique<Gui::AsyncPreviewSession>(std::move(callbacks), this);
    d->asyncPreviewSession->setSchedulerInterval(
        App::GetApplication().isAsyncRecomputeEnabled() ? Private::AsyncPreviewDebounceMs : 0
    );
    connect(
        d->asyncPreviewSession->controller(),
        &Gui::AsyncPreviewController::recomputeSettled,
        this,
        &OffsetWidget::recomputeSettled
    );
    d->asyncPreviewSession->bindWidgets(
        {
            d->ui.previewStatusWidget,
            d->ui.progressBarPreview,
            d->ui.labelPreviewStatus,
            d->ui.buttonCancelPreview,
        },
        [](const char* text) { return OffsetWidget::tr(text); }
    );
    updateRecomputeUi();
}

OffsetWidget::~OffsetWidget()
{
    delete d;
}

void OffsetWidget::setupConnections()
{
    // clang-format off
    connect(d->ui.spinOffset, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &OffsetWidget::onSpinOffsetValueChanged);
    connect(d->ui.modeType, qOverload<int>(&QComboBox::activated),
            this, &OffsetWidget::onModeTypeActivated);
    connect(d->ui.joinType, qOverload<int>(&QComboBox::activated),
            this, &OffsetWidget::onJoinTypeActivated);
    connect(d->ui.intersection, &QCheckBox::toggled,
            this, &OffsetWidget::onIntersectionToggled);
    connect(d->ui.selfIntersection, &QCheckBox::toggled,
            this, &OffsetWidget::onSelfIntersectionToggled);
    connect(d->ui.fillOffset, &QCheckBox::toggled,
            this, &OffsetWidget::onFillOffsetToggled);
    connect(d->ui.updateView, &QCheckBox::toggled,
            this, &OffsetWidget::onUpdateViewToggled);
    // clang-format on
}

Part::Offset* OffsetWidget::getObject() const
{
    return d->offset;
}

void OffsetWidget::schedulePreviewRecompute()
{
    if (d->asyncPreviewSession && d->ui.updateView->isChecked()) {
        d->asyncPreviewSession->scheduleRecompute();
    }
}

void OffsetWidget::flushPendingRecompute()
{
    if (d->asyncPreviewSession) {
        d->asyncPreviewSession->flushPendingRecompute();
    }
}

void OffsetWidget::stopPendingRecompute()
{
    if (d->asyncPreviewSession) {
        d->asyncPreviewSession->stopPendingRecompute();
    }
}

bool OffsetWidget::hasOutstandingRecompute() const
{
    return d->asyncPreviewSession && d->asyncPreviewSession->hasOutstandingRecompute();
}

void OffsetWidget::setDeferredClosePending(bool pending)
{
    if (d->asyncPreviewSession) {
        d->asyncPreviewSession->setDeferredClosePending(pending);
    }
}

void OffsetWidget::requestPreviewRecompute(bool waitForCompletion)
{
    if (d->asyncPreviewSession) {
        d->asyncPreviewSession->requestRecompute(waitForCompletion);
    }
}

void OffsetWidget::updateRecomputeUi()
{
    if (d->asyncPreviewSession) {
        d->asyncPreviewSession->updateUi();
    }
}

void OffsetWidget::onSpinOffsetValueChanged(double val)
{
    d->offset->Value.setValue(val);
    schedulePreviewRecompute();
}

void OffsetWidget::onModeTypeActivated(int val)
{
    d->offset->Mode.setValue(val);
    schedulePreviewRecompute();
}

void OffsetWidget::onJoinTypeActivated(int val)
{
    d->offset->Join.setValue((long)val);
    schedulePreviewRecompute();
}

void OffsetWidget::onIntersectionToggled(bool on)
{
    d->offset->Intersection.setValue(on);
    schedulePreviewRecompute();
}

void OffsetWidget::onSelfIntersectionToggled(bool on)
{
    d->offset->SelfIntersection.setValue(on);
    schedulePreviewRecompute();
}

void OffsetWidget::onFillOffsetToggled(bool on)
{
    d->offset->Fill.setValue(on);
    schedulePreviewRecompute();
}

void OffsetWidget::onUpdateViewToggled(bool on)
{
    if (on) {
        if (!d->asyncPreviewSession) {
            return;
        }

        if (!d->asyncPreviewSession->triggerScheduledRecomputeNow()) {
            requestPreviewRecompute(/*waitForCompletion=*/false);
        }
    }
    else {
        stopPendingRecompute();
    }
}

bool OffsetWidget::accept(QDialogButtonBox* dialogButtonBox)
{
    const bool previewUpToDate = d->ui.updateView->isChecked();
    flushPendingRecompute();
    App::Document* document = d->offset->getDocument();
    if (!document) {
        return false;
    }

    try {
        double offsetValue = d->ui.spinOffset->value().getValue();
        Gui::cmdAppObjectArgs(d->offset, "Value = %f", offsetValue);
        d->ui.spinOffset->apply();
        Gui::cmdAppObjectArgs(d->offset, "Mode = %d", d->ui.modeType->currentIndex());
        Gui::cmdAppObjectArgs(d->offset, "Join = %d", d->ui.joinType->currentIndex());
        Gui::cmdAppObjectArgs(
            d->offset,
            "Intersection = %s",
            d->ui.intersection->isChecked() ? "True" : "False"
        );
        Gui::cmdAppObjectArgs(
            d->offset,
            "SelfIntersection = %s",
            d->ui.selfIntersection->isChecked() ? "True" : "False"
        );
        Gui::cmdAppObjectArgs(d->offset, "Fill = %s", d->ui.fillOffset->isChecked() ? "True" : "False");

        if (previewUpToDate) {
            Gui::cmdAppObject(d->offset, "purgeTouched()");
            for (auto parent : d->offset->getInList()) {
                parent->touch();
            }
        }

        const QString recomputeStatus = tr("Computing offset...");
        const auto progressTarget = d->asyncPreviewSession
            ? d->asyncPreviewSession
                  ->makeInlineRecomputeProgressTarget(this, dialogButtonBox, recomputeStatus)
            : Gui::AsyncInlineRecomputeProgressTarget {};
        const Gui::ScopedAsyncInlineRecomputeProgress inlineProgress(progressTarget);
        Gui::AsyncRecomputeDialogOptions recomputeOptions;
        recomputeOptions.showDialog = !inlineProgress.isActive();
        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            this,
            tr("Offset"),
            recomputeStatus,
            document,
            /*force=*/false,
            recomputeOptions,
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
                outcome.message.empty() ? "Offset recompute failed" : outcome.message
            );
        }
        if (!d->offset->isValid()) {
            throw Base::CADKernelError(d->offset->getStatusString());
        }

        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        document->commitTransaction();  // ViewProviderDocumentObject::startDefaultEditMode()
    }
    catch (const Base::Exception& e) {
        document->abortTransaction();  // ViewProviderDocumentObject::startDefaultEditMode()
        QMessageBox::warning(
            this,
            tr("Input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    return true;
}

bool OffsetWidget::reject()
{
    stopPendingRecompute();

    // get the support and Sketch
    App::DocumentObject* source = d->offset->Source.getValue();
    if (source) {
        Gui::Application::Instance->getViewProvider(source)->show();
    }

    // roll back the done things
    d->offset->getDocument()->abortTransaction();  // ViewProviderDocumentObject::startDefaultEditMode()
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();

    return true;
}

void OffsetWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        updateRecomputeUi();
    }
}


/* TRANSLATOR PartGui::TaskOffset */

TaskOffset::TaskOffset(Part::Offset* offset)
{
    widget = new OffsetWidget(offset);
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Offset"), widget);
    connect(widget, &OffsetWidget::recomputeSettled, this, &TaskOffset::onRecomputeSettled);
}

TaskOffset::~TaskOffset() = default;

Part::Offset* TaskOffset::getObject() const
{
    return widget->getObject();
}

void TaskOffset::open()
{}

void TaskOffset::clicked(int)
{}

bool TaskOffset::accept()
{
    if (!widget || deferredReject.pending) {
        return false;
    }

    return widget->accept(buttonBox);
}

bool TaskOffset::reject()
{
    if (!widget) {
        return false;
    }

    ensureDeferredRejectConnection();
    widget->stopPendingRecompute();
    if (!widget->hasOutstandingRecompute()) {
        return rejectNow();
    }

    if (!deferredReject.pending) {
        auto* object = getObject();
        deferredReject.documentName = object && object->getDocument()
            ? std::string(object->getDocument()->getName())
            : std::string();
        setDeferredRejectPending(true);
    }

    return false;
}

void TaskOffset::ensureDeferredRejectConnection()
{
    Gui::ensureDeferredDialogRejectConnection(
        deferredReject,
        widget,
        &OffsetWidget::recomputeSettled,
        this,
        &TaskOffset::onRecomputeSettled
    );
}

void TaskOffset::setDeferredRejectPending(bool pending)
{
    Gui::setDeferredDialogRejectPending(deferredReject, pending, buttonBox, [this](bool pending) {
        if (widget) {
            widget->setDeferredClosePending(pending);
        }
    });
}

bool TaskOffset::rejectNow()
{
    return widget && widget->reject();
}

void TaskOffset::onRecomputeSettled()
{
    Gui::finishDeferredDialogReject(
        this,
        deferredReject,
        widget && !widget->hasOutstandingRecompute(),
        [this]() { return rejectNow(); },
        [this](bool pending) { setDeferredRejectPending(pending); }
    );
}

#include "moc_TaskOffset.cpp"
