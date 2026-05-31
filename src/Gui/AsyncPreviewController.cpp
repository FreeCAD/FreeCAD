// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

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

#include <QCoreApplication>
#include <QEvent>
#include <QMetaObject>
#include <QPointer>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>

#include <App/AsyncRecomputeDebug.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/MainThreadSignal.h>
#include <Gui/ActionFunction.h>
#include <Gui/Inventor/Draggers/Gizmo.h>

#include "AsyncPreviewController.h"

namespace Gui
{
namespace
{
bool recomputeResultWasCanceled(const App::RecomputeResult& result)
{
    return result.failure == App::RecomputeFailure::Canceled;
}

const char* recomputeFailureName(App::RecomputeFailure failure)
{
    switch (failure) {
        case App::RecomputeFailure::None:
            return "none";
        case App::RecomputeFailure::Canceled:
            return "canceled";
        case App::RecomputeFailure::DependencyCycle:
            return "dependency_cycle";
        case App::RecomputeFailure::Exception:
            return "exception";
    }

    return "unknown";
}

bool hasStableDocumentIdentity(const App::RecomputeRequest& request)
{
    return !request.documentName.empty();
}

std::string describeRecomputeRequest(const App::RecomputeRequest& request)
{
    std::ostringstream stream;
    stream << "doc=" << (request.documentName.empty() ? "<none>" : request.documentName);
    stream << " object="
           << (request.documentObjectName.empty() ? "<none>" : request.documentObjectName);
    stream << " recursive=" << request.recursive;
    stream << " force=" << request.force;
    stream << " options=" << request.options;
    return stream.str();
}

void appendPreviewControllerDebugLog(
    const AsyncPreviewController* controller,
    const char* event,
    const std::string& details = {}
)
{
    std::ostringstream stream;
    stream << "[Gui::AsyncPreviewController] " << event;
    stream << " controller=" << controller;
    if (!details.empty()) {
        stream << ' ' << details;
    }
    App::appendAsyncRecomputeDebugLog(stream.str());
}

void queueOnMainThread(std::function<void()>&& fn)
{
    if (App::MainThreadSignalConfig::hasHooks()) {
        App::MainThreadSignalConfig::invoke(std::move(fn), /*blocking=*/false);
        return;
    }

    if (auto* app = QCoreApplication::instance()) {
        QMetaObject::invokeMethod(app, [f = std::move(fn)]() mutable { f(); }, Qt::QueuedConnection);
        return;
    }

    fn();
}

bool canPumpQueuedMainThreadDispatches()
{
    if (App::MainThreadSignalConfig::hasHooks() && App::MainThreadSignalConfig::isMainThread()
        && App::MainThreadSignalConfig::canPumpEvents()) {
        return true;
    }

    auto* app = QCoreApplication::instance();
    return app && QThread::currentThread() == app->thread();
}

void pumpQueuedMainThreadDispatches()
{
    if (App::MainThreadSignalConfig::hasHooks() && App::MainThreadSignalConfig::isMainThread()
        && App::MainThreadSignalConfig::canPumpEvents()) {
        App::MainThreadSignalConfig::pumpEvents();
        return;
    }

    if (auto* app = QCoreApplication::instance(); app && QThread::currentThread() == app->thread()) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    }
}

template<typename Predicate>
void waitWithQueuedMainThreadDispatch(
    std::unique_lock<std::mutex>& lock,
    std::condition_variable& cv,
    Predicate&& predicate
)
{
    while (!predicate()) {
        if (canPumpQueuedMainThreadDispatches()) {
            lock.unlock();
            pumpQueuedMainThreadDispatches();
            lock.lock();
            if (!predicate()) {
                cv.wait_for(lock, std::chrono::milliseconds(1));
            }
            continue;
        }

        cv.wait(lock, std::forward<Predicate>(predicate));
    }
}
}  // namespace

AsyncPreviewController::AsyncPreviewController(Callbacks callbacks, QObject* parent)
    : QObject(parent)
    , callbacks(std::move(callbacks))
{}

AsyncPreviewController::~AsyncPreviewController()
{
    if (recomputeScheduler) {
        recomputeScheduler->stop();
    }
    detachRecomputeProgressObserver();
}

void AsyncPreviewController::setSchedulerInterval(int intervalMs)
{
    schedulerIntervalMs = std::max(0, intervalMs);

    if (!recomputeScheduler) {
        recomputeScheduler = std::make_unique<Gui::DebouncedFunction>(this);
        recomputeScheduler->setFunction([this]() { requestRecompute(/*waitForCompletion=*/false); });
    }

    recomputeScheduler->setInterval(effectiveSchedulerIntervalMs());
}

bool AsyncPreviewController::hasScheduledRecompute() const
{
    return recomputeScheduler && recomputeScheduler->isActive();
}

bool AsyncPreviewController::triggerScheduledRecomputeNow()
{
    if (!hasScheduledRecompute()) {
        return false;
    }

    recomputeScheduler->triggerNow();
    return true;
}

void AsyncPreviewController::scheduleRecompute()
{
    if (recomputeScheduler) {
        recomputeScheduler->setInterval(effectiveSchedulerIntervalMs());
        recomputeScheduler->start();
        return;
    }

    requestRecompute(/*waitForCompletion=*/false);
}

int AsyncPreviewController::effectiveSchedulerIntervalMs() const
{
    if (schedulerIntervalMs <= 0) {
        return schedulerIntervalMs;
    }

    if (Gui::Gizmo::isAnyDragActive()) {
        return std::min(schedulerIntervalMs, Gui::Gizmo::activeDragPreviewDebounceMs());
    }

    return schedulerIntervalMs;
}

void AsyncPreviewController::stopScheduledRecompute()
{
    if (recomputeScheduler) {
        recomputeScheduler->stop();
    }
}

void AsyncPreviewController::flushPendingRecompute()
{
    if (!hasScheduledRecompute() && !hasOutstandingRecompute() && lastRecomputeSucceeded) {
        return;
    }

    if (hasScheduledRecompute()) {
        stopScheduledRecompute();
        requestRecompute(/*waitForCompletion=*/true);
        return;
    }

    if (hasOutstandingRecompute()) {
        waitForOutstandingRecomputeToSettle();
        return;
    }

    requestRecompute(/*waitForCompletion=*/true);
}

void AsyncPreviewController::stopPendingRecompute()
{
    stopScheduledRecompute();

    if (!recomputeProgress || !callbacks.makeRequest) {
        return;
    }

    App::RecomputeRequest request = callbacks.makeRequest();
    if (!hasStableDocumentIdentity(request)) {
        return;
    }

    appendPreviewControllerDebugLog(this, "stop_pending_begin", describeRecomputeRequest(request));

    const App::RecomputeCancellationResult cancellation
        = App::GetApplication().cancelRecomputeRequest(request);

    {
        std::ostringstream details;
        details << describeRecomputeRequest(request);
        details << " canceled_in_progress=" << cancellation.canceledInProgress;
        details << " removed_queued=" << cancellation.removedQueued;
        appendPreviewControllerDebugLog(this, "stop_pending_result", details.str());
    }

    if (cancellation.canceledInProgress) {
        recomputeGeneration = appliedRecomputeGeneration + 1;
        recomputeQueued = false;
        recomputeCancelRequested = true;
        notifyStateChanged();
        return;
    }

    if (cancellation.removedQueued) {
        recomputeGeneration = appliedRecomputeGeneration;
        recomputeInProgress = false;
        recomputeQueued = false;
        recomputeCancelRequested = false;
        detachRecomputeProgressObserver();
        recomputeProgress.reset();
        resetRecomputeProgressDisplayState();
        notifyStateChanged();
    }
}

void AsyncPreviewController::requestRecompute(bool waitForCompletion)
{
    if (!callbacks.makeRequest || !callbacks.runSync) {
        return;
    }

    App::RecomputeRequest request = callbacks.makeRequest();
    if (!hasStableDocumentIdentity(request)) {
        return;
    }

    const bool asyncEnabled = App::GetApplication().isAsyncRecomputeEnabled();
    const bool canRunOnWorker = asyncEnabled
        && App::GetApplication().canRecomputeRequestOnWorker(request);
    {
        std::ostringstream details;
        details << describeRecomputeRequest(request);
        details << " wait=" << waitForCompletion;
        details << " async_enabled=" << asyncEnabled;
        details << " worker=" << canRunOnWorker;
        details << " generation=" << (recomputeGeneration + 1);
        details << " had_outstanding=" << (recomputeGeneration != appliedRecomputeGeneration);
        appendPreviewControllerDebugLog(this, "request_begin", details.str());
    }

    if (!canRunOnWorker) {
        App::ScopedRecomputeOptions optionsScope(request.options);
        callbacks.runSync();

        const auto generation = ++recomputeGeneration;
        appliedRecomputeGeneration = generation;
        lastRecomputeSucceeded = true;
        recomputeInProgress = false;
        recomputeQueued = false;
        recomputeCancelRequested = false;
        detachRecomputeProgressObserver();
        recomputeProgress.reset();
        resetRecomputeProgressDisplayState();

        if (callbacks.onCompleted) {
            callbacks.onCompleted(
                /*success=*/true,
                /*canceled=*/false,
                App::RecomputeFailure::None,
                {}
            );
        }
        if (callbacks.onAppliedResult) {
            callbacks.onAppliedResult(/*success=*/true, /*canceled=*/false);
        }
        appendPreviewControllerDebugLog(this, "request_sync_done", describeRecomputeRequest(request));
        notifyStateChanged();
        Q_EMIT recomputeSettled();
        return;
    }

    const bool hadOutstanding = recomputeGeneration != appliedRecomputeGeneration;
    const auto generation = ++recomputeGeneration;
    if (!recomputeProgress || !hadOutstanding) {
        detachRecomputeProgressObserver();
        recomputeProgress = std::make_shared<App::RecomputeProgressHandle>();
        resetRecomputeProgressDisplayState();
    }

    attachRecomputeProgressObserver(generation);
    request.progress = recomputeProgress;

    if (!waitForCompletion) {
        recomputeInProgress = true;
        recomputeQueued = hadOutstanding;
        recomputeCancelRequested = false;
        notifyStateChanged();

        QPointer<AsyncPreviewController> guard(this);
        request.callback = [guard, generation](App::RecomputeRequest&, App::RecomputeResult& result) {
            const bool success = result.success;
            const bool canceled = recomputeResultWasCanceled(result);
            const auto failure = result.failure;
            const std::string message = result.exception ? result.exception->what() : std::string();

            {
                std::ostringstream details;
                details << "generation=" << generation;
                details << " success=" << success;
                details << " canceled=" << canceled;
                details << " failure=" << recomputeFailureName(failure);
                if (!message.empty()) {
                    details << " message=" << message;
                }
                appendPreviewControllerDebugLog(guard, "request_callback", details.str());
            }

            queueOnMainThread([guard, generation, success, canceled, failure, message]() {
                if (guard) {
                    guard->finishRecompute(generation, success, canceled, failure, message);
                }
            });
        };

        App::GetApplication().queueRecomputeRequest(std::move(request));
        return;
    }

    struct WaitState
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        bool success = true;
        bool canceled = false;
        App::RecomputeFailure failure = App::RecomputeFailure::None;
        std::string message;
    };

    auto waitState = std::make_shared<WaitState>();
    request.callback = [waitState](App::RecomputeRequest&, App::RecomputeResult& result) {
        {
            std::lock_guard<std::mutex> lock(waitState->mutex);
            waitState->done = true;
            waitState->success = result.success;
            waitState->canceled = recomputeResultWasCanceled(result);
            waitState->failure = result.failure;
            if (result.exception) {
                waitState->message = result.exception->what();
            }
        }
        waitState->cv.notify_one();
    };

    App::GetApplication().queueRecomputeRequest(std::move(request));

    std::unique_lock<std::mutex> lock(waitState->mutex);
    waitWithQueuedMainThreadDispatch(lock, waitState->cv, [&waitState]() { return waitState->done; });
    lock.unlock();

    finishRecompute(
        generation,
        waitState->success,
        waitState->canceled,
        waitState->failure,
        std::move(waitState->message)
    );
}

bool AsyncPreviewController::hasOutstandingRecompute() const
{
    return recomputeGeneration != appliedRecomputeGeneration;
}

bool AsyncPreviewController::isInProgress() const
{
    return recomputeInProgress;
}

bool AsyncPreviewController::isQueued() const
{
    return recomputeQueued;
}

bool AsyncPreviewController::isCancelRequested() const
{
    return recomputeCancelRequested;
}

bool AsyncPreviewController::didLastRecomputeSucceed() const
{
    return lastRecomputeSucceeded;
}

std::uint64_t AsyncPreviewController::currentGeneration() const
{
    return recomputeGeneration;
}

bool AsyncPreviewController::isProgressDeterminate() const
{
    return recomputeProgressDeterminate;
}

int AsyncPreviewController::progressPercent() const
{
    return recomputeProgressPercent;
}

const std::string& AsyncPreviewController::progressText() const
{
    return recomputeProgressText;
}

void AsyncPreviewController::finishRecompute(
    std::uint64_t generation,
    bool success,
    bool canceled,
    App::RecomputeFailure failure,
    std::string message
)
{
    if (generation != recomputeGeneration) {
        recomputeQueued = generation + 1 < recomputeGeneration;
        recomputeInProgress = true;
        {
            std::ostringstream details;
            details << "generation=" << generation;
            details << " current_generation=" << recomputeGeneration;
            details << " success=" << success;
            details << " canceled=" << canceled;
            details << " failure=" << recomputeFailureName(failure);
            appendPreviewControllerDebugLog(this, "finish_superseded", details.str());
        }
        notifyStateChanged();
        return;
    }

    appliedRecomputeGeneration = generation;
    lastRecomputeSucceeded = success;
    recomputeInProgress = false;
    recomputeQueued = false;
    recomputeCancelRequested = false;
    detachRecomputeProgressObserver();
    recomputeProgress.reset();
    resetRecomputeProgressDisplayState();

    if (callbacks.onCompleted) {
        callbacks.onCompleted(success, canceled, failure, message);
    }
    if (callbacks.onAppliedResult) {
        callbacks.onAppliedResult(success, canceled);
    }
    {
        std::ostringstream details;
        details << "generation=" << generation;
        details << " success=" << success;
        details << " canceled=" << canceled;
        details << " failure=" << recomputeFailureName(failure);
        if (!message.empty()) {
            details << " message=" << message;
        }
        appendPreviewControllerDebugLog(this, "finish_done", details.str());
    }
    notifyStateChanged();
    Q_EMIT recomputeSettled();
}

void AsyncPreviewController::waitForOutstandingRecomputeToSettle()
{
    if (!hasOutstandingRecompute()) {
        return;
    }

    struct WaitState
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
    };

    auto waitState = std::make_shared<WaitState>();
    QMetaObject::Connection connection
        = QObject::connect(this, &AsyncPreviewController::recomputeSettled, this, [waitState]() {
              {
                  std::lock_guard<std::mutex> lock(waitState->mutex);
                  waitState->done = true;
              }
              waitState->cv.notify_one();
          });

    std::unique_lock<std::mutex> lock(waitState->mutex);
    waitWithQueuedMainThreadDispatch(lock, waitState->cv, [this, &waitState]() {
        return waitState->done || !hasOutstandingRecompute();
    });
    lock.unlock();

    QObject::disconnect(connection);
}

void AsyncPreviewController::attachRecomputeProgressObserver(std::uint64_t generation)
{
    if (!recomputeProgress) {
        resetRecomputeProgressDisplayState();
        return;
    }

    auto* progressHandle = recomputeProgress.get();
    QPointer<AsyncPreviewController> guard(this);
    recomputeProgress->setDisplayObserver(
        [guard, progressHandle, generation](const App::RecomputeProgressDisplayState& state) {
            queueOnMainThread([guard, progressHandle, generation, state]() {
                if (!guard || guard->recomputeProgress.get() != progressHandle
                    || guard->recomputeGeneration != generation) {
                    return;
                }

                guard->setRecomputeProgressDisplayState(generation, state);
            });
        }
    );
    setRecomputeProgressDisplayState(generation, recomputeProgress->displayState());
}

void AsyncPreviewController::detachRecomputeProgressObserver()
{
    if (recomputeProgress) {
        recomputeProgress->setDisplayObserver({});
    }
}

void AsyncPreviewController::resetRecomputeProgressDisplayState()
{
    recomputeProgressGeneration = 0;
    recomputeProgressText.clear();
    recomputeProgressPercent = 0;
    recomputeProgressDeterminate = false;
    appendPreviewControllerDebugLog(this, "progress_reset");
}

void AsyncPreviewController::setRecomputeProgressDisplayState(
    std::uint64_t generation,
    const App::RecomputeProgressDisplayState& state
)
{
    const bool textChanged = recomputeProgressText != state.text;
    const bool percentChanged = recomputeProgressPercent != state.percent;
    const bool determinateChanged = recomputeProgressDeterminate != state.determinate;
    const bool generationChanged = recomputeProgressGeneration != generation;
    if (generationChanged) {
        recomputeProgressGeneration = generation;
        recomputeProgressPercent = 0;
        recomputeProgressDeterminate = false;
    }

    const bool clampedRegression = state.determinate && !generationChanged
        && state.percent < recomputeProgressPercent;
    recomputeProgressDeterminate = state.determinate;
    if (state.determinate) {
        recomputeProgressPercent = std::max(recomputeProgressPercent, state.percent);
    }
    else {
        recomputeProgressPercent = state.percent;
    }
    if (!clampedRegression) {
        recomputeProgressText = state.text;
    }

    if (generationChanged || clampedRegression || textChanged || percentChanged
        || determinateChanged) {
        std::ostringstream details;
        details << "generation=" << generation;
        details << " state_text=" << (state.text.empty() ? "<empty>" : state.text);
        details << " state_percent=" << state.percent;
        details << " state_determinate=" << state.determinate;
        details << " display_text="
                << (recomputeProgressText.empty() ? "<empty>" : recomputeProgressText);
        details << " display_percent=" << recomputeProgressPercent;
        details << " display_determinate=" << recomputeProgressDeterminate;
        details << " generation_changed=" << generationChanged;
        details << " clamped_regression=" << clampedRegression;
        appendPreviewControllerDebugLog(this, "progress_update", details.str());
    }

    notifyStateChanged();
}

void AsyncPreviewController::notifyStateChanged()
{
    if (callbacks.onStateChanged) {
        callbacks.onStateChanged();
    }
    Q_EMIT stateChanged();
}

}  // namespace Gui

#include "moc_AsyncPreviewController.cpp"
