// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <utility>

#include <QDialogButtonBox>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPointer>
#include <QProgressDialog>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <App/Document.h>
#include <Base/Exception.h>

#include "AsyncPreviewController.h"
#include "AsyncPreviewWidgetUtils.h"

namespace Gui
{

/// Delay before showing accepted-operation progress, avoiding modal flicker for fast recomputes.
inline constexpr int AsyncRecomputeProgressDialogDelayMs = 150;

/**
 * @brief Result captured from an async recompute progress dialog helper.
 */
struct AsyncRecomputeProgressOutcome
{
    bool success = false;
    bool canceled = false;
    App::RecomputeFailure failure = App::RecomputeFailure::None;
    std::string message;
};

struct AsyncInlineRecomputeProgressTarget
{
    QPointer<QWidget> contentWidget;
    QPointer<QDialogButtonBox> buttonBox;
    QString statusText;
    std::function<void(bool pending, const QString& statusText)> setPending;

    explicit operator bool() const
    {
        return static_cast<bool>(setPending);
    }
};

struct AsyncRecomputeDialogOptions
{
    int showDelayMs = AsyncRecomputeProgressDialogDelayMs;
    bool cancelable = true;
    bool dynamicLabel = true;
    bool forceIndeterminate = false;
    bool showDialog = true;
    AsyncInlineRecomputeProgressTarget inlineProgressTarget;
};

class ScopedAsyncInlineRecomputeProgress
{
public:
    explicit ScopedAsyncInlineRecomputeProgress(AsyncInlineRecomputeProgressTarget target)
        : target(std::move(target))
        , contentWasEnabled(this->target.contentWidget ? this->target.contentWidget->isEnabled() : false)
        , buttonBoxWasEnabled(this->target.buttonBox ? this->target.buttonBox->isEnabled() : false)
    {
        if (!this->target) {
            return;
        }

        active = true;
        this->target.setPending(true, this->target.statusText);
        if (this->target.contentWidget) {
            this->target.contentWidget->setEnabled(false);
        }
        if (this->target.buttonBox) {
            this->target.buttonBox->setEnabled(false);
        }
    }

    ~ScopedAsyncInlineRecomputeProgress()
    {
        if (!active) {
            return;
        }

        if (target.buttonBox) {
            target.buttonBox->setEnabled(buttonBoxWasEnabled);
        }
        if (target.contentWidget) {
            target.contentWidget->setEnabled(contentWasEnabled);
        }
        target.setPending(false, {});
    }

    ScopedAsyncInlineRecomputeProgress(const ScopedAsyncInlineRecomputeProgress&) = delete;
    ScopedAsyncInlineRecomputeProgress& operator=(const ScopedAsyncInlineRecomputeProgress&) = delete;

    bool isActive() const
    {
        return active;
    }

private:
    AsyncInlineRecomputeProgressTarget target;
    bool contentWasEnabled = false;
    bool buttonBoxWasEnabled = false;
    bool active = false;
};

inline AsyncInlineRecomputeProgressTarget makeTaskPanelInlineRecomputeProgressTarget(
    QWidget* contentWidget,
    QDialogButtonBox* buttonBox,
    QString statusText,
    QLayout* statusLayout = nullptr
)
{
    auto* layout = statusLayout ? statusLayout : (contentWidget ? contentWidget->layout() : nullptr);
    if (!contentWidget || !layout) {
        return {};
    }

    struct State
    {
        QPointer<QWidget> contentWidget;
        QPointer<QLayout> statusLayout;
        QPointer<QWidget> statusWidget;
        QPointer<AsyncPreviewBusySpinner> spinner;
        QPointer<QLabel> statusLabel;
    };

    auto state = std::make_shared<State>();
    state->contentWidget = contentWidget;
    state->statusLayout = layout;

    AsyncInlineRecomputeProgressTarget target;
    target.contentWidget = contentWidget;
    target.buttonBox = buttonBox;
    target.statusText = std::move(statusText);
    target.setPending = [state](bool pending, const QString& status) {
        if (!state->contentWidget || !state->statusLayout) {
            return;
        }

        if (!pending) {
            if (state->spinner) {
                state->spinner->setAnimated(false);
            }
            delete state->statusWidget.data();
            state->statusWidget = nullptr;
            state->spinner = nullptr;
            state->statusLabel = nullptr;
            return;
        }

        if (!state->statusWidget) {
            auto* statusParent = state->statusLayout->parentWidget()
                ? state->statusLayout->parentWidget()
                : state->contentWidget.data();
            auto* statusWidget = new QWidget(statusParent);
            statusWidget->setObjectName(QStringLiteral("fcAsyncAcceptProgress"));
            statusWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            auto* rowLayout = new QHBoxLayout(statusWidget);
            rowLayout->setContentsMargins(0, 4, 0, 0);
            rowLayout->setSpacing(6);

            auto* spinner = new AsyncPreviewBusySpinner(statusWidget);
            auto* statusLabel = new QLabel(statusWidget);
            statusLabel->setObjectName(QStringLiteral("fcAsyncAcceptProgressLabel"));
            statusLabel->setMinimumWidth(0);
            statusLabel->setWordWrap(false);
            statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            rowLayout->addWidget(spinner);
            rowLayout->addWidget(statusLabel);
            state->statusLayout->addWidget(statusWidget);

            state->statusWidget = statusWidget;
            state->spinner = spinner;
            state->statusLabel = statusLabel;
        }

        if (state->statusLabel) {
            state->statusLabel->setText(status);
            state->statusLabel->setToolTip(status);
        }
        if (state->spinner) {
            state->spinner->setAnimated(true);
        }
        if (state->statusWidget) {
            state->statusWidget->show();
        }
    };

    return target;
}

/**
 * @brief Execute the legacy synchronous recompute path and normalize exceptions.
 */
template<typename RunSyncFn>
AsyncRecomputeProgressOutcome runSyncDocumentRecomputeFallback(App::Document* document, RunSyncFn&& runSync)
{
    AsyncRecomputeProgressOutcome outcome;
    if (!document) {
        outcome.message = "No document available for recompute";
        return outcome;
    }

    try {
        runSync();
        outcome.success = true;
    }
    catch (const Base::Exception& e) {
        outcome.failure = App::RecomputeFailure::Exception;
        outcome.message = e.what();
    }
    catch (const std::exception& e) {
        outcome.failure = App::RecomputeFailure::Exception;
        outcome.message = e.what();
    }
    catch (...) {
        outcome.failure = App::RecomputeFailure::Exception;
        outcome.message = "Document recompute failed";
    }

    return outcome;
}

/**
 * @brief Build the progress label from controller state and fallback text.
 */
inline QString asyncRecomputeProgressLabel(
    const AsyncPreviewController& controller,
    const QString& fallbackLabel
)
{
    QString label = controller.progressText().empty()
        ? fallbackLabel
        : QString::fromUtf8(controller.progressText().c_str());

    if (controller.isCancelRequested()) {
        return QObject::tr("Canceling operation…");
    }
    if (controller.isQueued()) {
        label += QObject::tr(" (latest change queued)");
    }
    return label;
}

/**
 * @brief Refresh a modal recompute progress dialog from controller state.
 */
inline void updateAsyncRecomputeProgressDialog(
    QProgressDialog& dialog,
    const AsyncPreviewController& controller,
    const QString& fallbackLabel,
    const AsyncRecomputeDialogOptions& options
)
{
    dialog.setLabelText(
        options.dynamicLabel ? asyncRecomputeProgressLabel(controller, fallbackLabel) : fallbackLabel
    );
    if (!options.forceIndeterminate && controller.isProgressDeterminate()) {
        dialog.setRange(0, 100);
        dialog.setValue(controller.progressPercent());
    }
    else {
        dialog.setRange(0, 0);
    }
}

/**
 * @brief Run an async recompute behind a delayed modal progress dialog.
 *
 * The supplied controller owns the recompute state. `start()` must initiate the
 * request, usually by calling `AsyncPreviewController::requestRecompute(false)`.
 * The dialog pumps a local Qt event loop until the latest controller generation
 * settles, and its Cancel button delegates to `controller.stopPendingRecompute()`.
 */
template<typename StartFn>
void runAsyncRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    AsyncPreviewController& controller,
    const AsyncRecomputeDialogOptions& options,
    StartFn&& start
)
{
    AsyncRecomputeDialogOptions effectiveOptions = options;
    ScopedAsyncInlineRecomputeProgress inlineProgress(effectiveOptions.inlineProgressTarget);
    if (inlineProgress.isActive()) {
        effectiveOptions.showDialog = false;
    }

    if (!effectiveOptions.showDialog) {
        Q_UNUSED(parent);
        Q_UNUSED(windowTitle);
        Q_UNUSED(fallbackLabel);

        QEventLoop loop;
        QObject loopContext;
        bool settled = false;
        const auto quitIfSettled = [&]() {
            if (!controller.hasOutstandingRecompute()) {
                settled = true;
                loop.quit();
            }
        };
        QObject::connect(&controller, &AsyncPreviewController::stateChanged, &loopContext, quitIfSettled);
        QObject::connect(
            &controller,
            &AsyncPreviewController::recomputeSettled,
            &loopContext,
            quitIfSettled
        );

        start();
        quitIfSettled();
        if (!settled) {
            loop.exec();
        }
        return;
    }

    QProgressDialog dialog(parent);
    dialog.setWindowTitle(windowTitle);
    dialog.setCancelButtonText(QObject::tr("Cancel"));
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setAutoClose(false);
    dialog.setAutoReset(false);
    dialog.setMinimumDuration(0);
    dialog.setRange(0, 0);
    dialog.setLabelText(fallbackLabel);
    if (!effectiveOptions.cancelable) {
        dialog.setCancelButton(nullptr);
    }

    QEventLoop loop;
    QTimer showTimer;
    showTimer.setSingleShot(true);
    if (effectiveOptions.cancelable) {
        QObject::connect(&dialog, &QProgressDialog::canceled, &controller, [&controller]() {
            controller.stopPendingRecompute();
        });
    }
    QObject::connect(
        &controller,
        &AsyncPreviewController::stateChanged,
        &dialog,
        [&dialog, &controller, fallbackLabel, &effectiveOptions, &loop, &showTimer]() {
            updateAsyncRecomputeProgressDialog(dialog, controller, fallbackLabel, effectiveOptions);
            if (!controller.hasOutstandingRecompute()) {
                showTimer.stop();
                dialog.hide();
                loop.quit();
            }
        }
    );
    QObject::connect(
        &controller,
        &AsyncPreviewController::recomputeSettled,
        &dialog,
        [&dialog, &loop, &showTimer]() {
            showTimer.stop();
            dialog.hide();
            loop.quit();
        }
    );
    QObject::connect(
        &showTimer,
        &QTimer::timeout,
        &dialog,
        [&dialog, &controller, fallbackLabel, &effectiveOptions]() {
            if (!controller.hasOutstandingRecompute()) {
                return;
            }
            updateAsyncRecomputeProgressDialog(dialog, controller, fallbackLabel, effectiveOptions);
            dialog.show();
        }
    );

    start();

    if (!controller.hasOutstandingRecompute()) {
        return;
    }

    updateAsyncRecomputeProgressDialog(dialog, controller, fallbackLabel, effectiveOptions);
    if (effectiveOptions.showDialog) {
        showTimer.start(std::max(0, effectiveOptions.showDelayMs));
    }
    loop.exec();
}

template<typename StartFn>
void runAsyncRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    AsyncPreviewController& controller,
    StartFn&& start
)
{
    runAsyncRecomputeProgressDialog(
        parent,
        windowTitle,
        fallbackLabel,
        controller,
        AsyncRecomputeDialogOptions {},
        std::forward<StartFn>(start)
    );
}

/**
 * @brief Recompute one document object with shared async progress/cancel UI.
 *
 * Falls back to `runSync` when async recompute is disabled or the object is not
 * worker-safe. The returned outcome describes the accepted request result.
 */
template<typename RunSyncFn>
AsyncRecomputeProgressOutcome runAsyncDocumentObjectRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    App::DocumentObject* object,
    bool recursive,
    const AsyncRecomputeDialogOptions& options,
    RunSyncFn&& runSync
)
{
    AsyncRecomputeProgressOutcome outcome;
    if (!object) {
        outcome.message = "No object available for recompute";
        return outcome;
    }

    const App::RecomputeRequest recomputeRequest
        = App::RecomputeRequest::fromDocumentObject(*object, recursive);

    AsyncPreviewController::Callbacks callbacks;
    callbacks.makeRequest = [recomputeRequest]() {
        return recomputeRequest;
    };
    callbacks.runSync = std::forward<RunSyncFn>(runSync);
    callbacks.onCompleted = [&outcome](
                                bool success,
                                bool canceled,
                                App::RecomputeFailure failure,
                                const std::string& message
                            ) {
        outcome.success = success;
        outcome.canceled = canceled;
        outcome.failure = failure;
        outcome.message = message;
    };

    AsyncPreviewController controller(std::move(callbacks), parent);
    runAsyncRecomputeProgressDialog(
        parent,
        windowTitle,
        fallbackLabel,
        controller,
        options,
        [&controller]() { controller.requestRecompute(/*waitForCompletion=*/false); }
    );
    return outcome;
}

template<typename RunSyncFn>
AsyncRecomputeProgressOutcome runAsyncDocumentObjectRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    App::DocumentObject* object,
    bool recursive,
    RunSyncFn&& runSync
)
{
    return runAsyncDocumentObjectRecomputeProgressDialog(
        parent,
        windowTitle,
        fallbackLabel,
        object,
        recursive,
        AsyncRecomputeDialogOptions {},
        std::forward<RunSyncFn>(runSync)
    );
}

/**
 * @brief Recompute a whole document with shared async progress/cancel UI.
 *
 * The queued request captures the document's stable internal name rather than a
 * document object pointer, so document-wide recomputes do not need an anchor
 * object for controller/cancel tracking.
 */
template<typename RunSyncFn>
AsyncRecomputeProgressOutcome runAsyncDocumentRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    App::Document* document,
    bool force,
    const AsyncRecomputeDialogOptions& options,
    RunSyncFn&& runSync
)
{
    if (!document) {
        AsyncRecomputeProgressOutcome outcome;
        outcome.message = "No document available for recompute";
        return outcome;
    }

    const App::RecomputeRequest recomputeRequest
        = App::RecomputeRequest::fromDocument(*document, force);

    AsyncRecomputeProgressOutcome outcome;
    AsyncPreviewController::Callbacks callbacks;
    callbacks.makeRequest = [recomputeRequest]() {
        return recomputeRequest;
    };
    callbacks.runSync = std::forward<RunSyncFn>(runSync);
    callbacks.onCompleted = [&outcome](
                                bool success,
                                bool canceled,
                                App::RecomputeFailure failure,
                                const std::string& message
                            ) {
        outcome.success = success;
        outcome.canceled = canceled;
        outcome.failure = failure;
        outcome.message = message;
    };

    AsyncPreviewController controller(std::move(callbacks), parent);
    runAsyncRecomputeProgressDialog(
        parent,
        windowTitle,
        fallbackLabel,
        controller,
        options,
        [&controller]() { controller.requestRecompute(/*waitForCompletion=*/false); }
    );
    return outcome;
}

template<typename RunSyncFn>
AsyncRecomputeProgressOutcome runAsyncDocumentRecomputeProgressDialog(
    QWidget* parent,
    const QString& windowTitle,
    const QString& fallbackLabel,
    App::Document* document,
    bool force,
    RunSyncFn&& runSync
)
{
    return runAsyncDocumentRecomputeProgressDialog(
        parent,
        windowTitle,
        fallbackLabel,
        document,
        force,
        AsyncRecomputeDialogOptions {},
        std::forward<RunSyncFn>(runSync)
    );
}

}  // namespace Gui
