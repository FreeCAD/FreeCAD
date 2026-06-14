// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "AsyncRecomputeProgressDialog.h"
#include "AsyncPreviewController.h"
#include "AsyncPreviewWidgetUtils.h"

namespace Gui
{

/**
 * @brief Convenience owner for task-panel async preview recomputes.
 *
 * `AsyncPreviewSession` combines `AsyncPreviewController` with the common preview
 * status widgets used by task dialogs. It wires the cancel button, delays the
 * progress UI for short operations, and exposes a small forwarding API so task
 * panels do not need to duplicate controller state management.
 *
 * Use this for live preview panels where property changes should schedule or
 * coalesce recomputes. Use the `AsyncRecomputeProgressDialog` helpers instead for
 * accepted command-style operations that should block the user behind a modal
 * progress surface until the recompute settles.
 */
class AsyncPreviewSession
{
public:
    /// Delay before showing preview progress, avoiding layout noise for fast updates.
    static constexpr int DefaultProgressUiDelayMs = 150;

    explicit AsyncPreviewSession(AsyncPreviewController::Callbacks callbacks, QObject* owner = nullptr)
        : owner(owner)
    {
        auto onStateChanged = std::move(callbacks.onStateChanged);
        callbacks.onStateChanged = [this, onStateChanged = std::move(onStateChanged)]() mutable {
            if (onStateChanged) {
                onStateChanged();
            }
            updateUi();
        };
        previewController = std::make_unique<AsyncPreviewController>(std::move(callbacks), owner);
        progressUiDelayTimer = std::make_unique<QTimer>();
        progressUiDelayTimer->setSingleShot(true);
        QObject::connect(
            progressUiDelayTimer.get(),
            &QTimer::timeout,
            progressUiDelayTimer.get(),
            [this]() {
                progressUiVisible = previewController && previewController->isInProgress()
                    && previewController->currentGeneration() == progressUiDelayGeneration;
                updateUi();
            }
        );
    }

    ~AsyncPreviewSession()
    {
        QObject::disconnect(cancelConnection);
    }

    AsyncPreviewSession(const AsyncPreviewSession&) = delete;
    AsyncPreviewSession& operator=(const AsyncPreviewSession&) = delete;

    AsyncPreviewController* controller()
    {
        return previewController.get();
    }

    const AsyncPreviewController* controller() const
    {
        return previewController.get();
    }

    /**
     * @brief Attach the common progress/cancel widgets owned by a task dialog.
     *
     * The widgets remain owned by the caller. `deferredCloseTarget`, when set, is
     * disabled while the session is canceling an in-flight preview before close.
     */
    void bindWidgets(
        const AsyncPreviewWidgetSet& widgets,
        std::function<QString(const char*)> translate,
        QWidget* deferredCloseTarget = nullptr
    )
    {
        previewWidgets = widgets;
        translateCallback = std::move(translate);
        this->deferredCloseTarget = deferredCloseTarget;

        initializeAsyncPreviewWidgets(previewWidgets);

        QObject::disconnect(cancelConnection);
        if (previewWidgets.cancelButton && owner) {
            cancelConnection = QObject::connect(
                previewWidgets.cancelButton,
                &QPushButton::clicked,
                owner,
                [this]() { stopPendingRecompute(); }
            );
        }

        if (this->deferredCloseTarget) {
            this->deferredCloseTarget->setEnabled(!deferredClosePending);
        }
        updateUi();
    }

    void setSchedulerInterval(int intervalMs)
    {
        if (previewController) {
            previewController->setSchedulerInterval(intervalMs);
        }
    }

    /// Override the delayed progress UI threshold. Use 0 to show progress immediately.
    void setProgressUiDelay(int delayMs)
    {
        progressUiDelayMs = std::max(0, delayMs);
        if (progressUiDelayMs == 0 && previewController && previewController->isInProgress()) {
            progressUiVisible = true;
        }
        updateUi();
    }

    void scheduleRecompute()
    {
        if (previewController) {
            previewController->scheduleRecompute();
        }
    }

    void stopScheduledRecompute()
    {
        if (previewController) {
            previewController->stopScheduledRecompute();
        }
    }

    bool triggerScheduledRecomputeNow()
    {
        return previewController && previewController->triggerScheduledRecomputeNow();
    }

    void requestRecompute(bool waitForCompletion)
    {
        if (previewController) {
            previewController->requestRecompute(waitForCompletion);
        }
    }

    void flushPendingRecompute()
    {
        if (previewController) {
            previewController->flushPendingRecompute();
        }
    }

    void stopPendingRecompute()
    {
        if (previewController) {
            previewController->stopPendingRecompute();
        }
    }

    bool hasOutstandingRecompute() const
    {
        return previewController && previewController->hasOutstandingRecompute();
    }

    bool isDeferredClosePending() const
    {
        return deferredClosePending;
    }

    bool didLastRecomputeSucceed() const
    {
        return previewController && previewController->didLastRecomputeSucceed();
    }

    AsyncInlineRecomputeProgressTarget makeInlineRecomputeProgressTarget(
        QWidget* contentWidget,
        QDialogButtonBox* buttonBox,
        QString statusText
    )
    {
        if (!previewController || !owner || !contentWidget) {
            return {};
        }

        auto* session = this;
        QPointer<QObject> ownerGuard(owner);
        QPointer<QWidget> contentGuard(contentWidget);
        AsyncInlineRecomputeProgressTarget target;
        target.contentWidget = contentWidget;
        target.buttonBox = buttonBox;
        target.statusText = std::move(statusText);
        target.setPending = [session, ownerGuard, contentGuard](bool pending, const QString& status) {
            if (ownerGuard && contentGuard) {
                session->setForcedBusy(pending, status);
            }
        };
        return target;
    }

    /**
     * @brief Mark that the task is waiting for preview cancellation before close.
     *
     * This forces the progress UI visible and changes the status text so the user
     * sees that close has been deferred until the active recompute observes
     * cancellation.
     */
    void setDeferredClosePending(bool pending)
    {
        if (deferredClosePending == pending) {
            return;
        }

        deferredClosePending = pending;
        if (deferredCloseTarget) {
            deferredCloseTarget->setEnabled(!pending);
        }
        updateUi();
    }

    void setForcedBusy(bool busy, QString statusText = {})
    {
        if (forcedBusy == busy && forcedBusyStatusText == statusText) {
            return;
        }

        forcedBusy = busy;
        forcedBusyStatusText = std::move(statusText);
        updateUi();
    }

    void updateUi()
    {
        updateProgressUiVisibility();
        updateAsyncPreviewWidgets(
            previewController.get(),
            deferredClosePending,
            forcedBusy,
            forcedBusyStatusText,
            progressUiVisible,
            previewWidgets,
            translateCallback ? translateCallback : defaultTranslate
        );
    }

private:
    void updateProgressUiVisibility()
    {
        if (forcedBusy) {
            progressUiVisible = true;
            progressUiDelayGeneration = 0;
            if (progressUiDelayTimer) {
                progressUiDelayTimer->stop();
            }
            return;
        }

        const bool inProgress = previewController && previewController->isInProgress();
        const bool forceVisible = inProgress
            && (deferredClosePending || previewController->isCancelRequested()
                || progressUiDelayMs == 0);

        if (!inProgress) {
            progressUiVisible = false;
            progressUiDelayGeneration = 0;
            if (progressUiDelayTimer) {
                progressUiDelayTimer->stop();
            }
            return;
        }

        if (forceVisible) {
            progressUiVisible = true;
            progressUiDelayGeneration = 0;
            if (progressUiDelayTimer) {
                progressUiDelayTimer->stop();
            }
            return;
        }

        if (progressUiVisible) {
            return;
        }

        // Delay short-lived preview progress UI to avoid distracting layout churn
        // for recomputes that settle almost immediately.
        const auto currentGeneration = previewController ? previewController->currentGeneration() : 0;
        if (progressUiDelayTimer
            && (!progressUiDelayTimer->isActive() || progressUiDelayGeneration != currentGeneration)) {
            progressUiDelayGeneration = currentGeneration;
            progressUiDelayTimer->start(progressUiDelayMs);
        }
    }

    static QString defaultTranslate(const char* text)
    {
        return QString::fromUtf8(text);
    }

private:
    QObject* owner = nullptr;
    std::unique_ptr<AsyncPreviewController> previewController;
    AsyncPreviewWidgetSet previewWidgets {};
    std::function<QString(const char*)> translateCallback;
    QWidget* deferredCloseTarget = nullptr;
    bool deferredClosePending = false;
    bool forcedBusy = false;
    QString forcedBusyStatusText;
    std::unique_ptr<QTimer> progressUiDelayTimer;
    std::uint64_t progressUiDelayGeneration = 0;
    int progressUiDelayMs = DefaultProgressUiDelayMs;
    bool progressUiVisible = false;
    QMetaObject::Connection cancelConnection;
};

}  // namespace Gui
