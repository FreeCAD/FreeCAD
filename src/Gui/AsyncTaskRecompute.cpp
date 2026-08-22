// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PreCompiled.h"

#include "AsyncTaskRecompute.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>

namespace Gui
{

AsyncTaskRecompute::AsyncTaskRecompute(QObject* parent)
    : QObject(parent)
{
    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &AsyncTaskRecompute::startPending);
}

AsyncTaskRecompute::~AsyncTaskRecompute()
{
    _timer.stop();
    if (_activeCancellation) {
        App::GetApplication().cancelRecomputeRequest(_activeCancellation);
    }
}

void AsyncTaskRecompute::schedule(App::RecomputeRequest request, int delayMs, Completion completion)
{
    ++_editSerial;
    _completedSerial = 0;

    _timer.stop();
    if (_activeCancellation) {
        App::GetApplication().cancelRecomputeRequest(_activeCancellation);
    }

    if (!request.cancellation) {
        request.cancellation = std::make_shared<App::RecomputeCancellationState>();
    }
    _pendingRequest = std::move(request);
    _pendingCompletion = std::move(completion);
    if (!_activeCancellation) {
        _running = false;
    }

    if (delayMs <= 0) {
        startPending();
    }
    else {
        _timer.start(delayMs);
    }
}

void AsyncTaskRecompute::setRunningChanged(RunningChanged callback)
{
    _runningChanged = std::move(callback);
}

bool AsyncTaskRecompute::flushPending()
{
    if (!_pendingRequest) {
        return false;
    }

    _timer.stop();
    startPending();
    return true;
}

void AsyncTaskRecompute::cancel()
{
    _timer.stop();

    if (_pendingRequest && _pendingRequest->cancellation) {
        _pendingRequest->cancellation->cancel();
    }
    _pendingRequest.reset();
    _pendingCompletion = {};

    if (_activeCancellation) {
        App::GetApplication().cancelRecomputeRequest(_activeCancellation);
        return;
    }

    _running = false;
    _completedSerial = 0;
}

bool AsyncTaskRecompute::isPending() const
{
    return _pendingRequest.has_value() && _timer.isActive();
}

bool AsyncTaskRecompute::isRunning() const
{
    return _running;
}

bool AsyncTaskRecompute::isSettling() const
{
    return static_cast<bool>(_activeCancellation);
}

bool AsyncTaskRecompute::hasCurrentSuccessfulPreview() const
{
    return !_pendingRequest && !_activeCancellation && _completedSerial == _editSerial
        && _completedSerial != 0;
}

std::uint64_t AsyncTaskRecompute::editSerial() const
{
    return _editSerial;
}

std::uint64_t AsyncTaskRecompute::completedSerial() const
{
    return _completedSerial;
}

void AsyncTaskRecompute::startPending()
{
    if (!_pendingRequest) {
        return;
    }

    App::RecomputeRequest request = std::move(*_pendingRequest);
    _pendingRequest.reset();
    _activeCompletion = std::move(_pendingCompletion);
    _pendingCompletion = {};
    _activeSerial = _editSerial;
    _activeCancellation = request.cancellation;
    _running = true;
    if (_runningChanged) {
        _runningChanged(true);
    }

    QPointer<AsyncTaskRecompute> guard(this);
    QPointer<QCoreApplication> application(QCoreApplication::instance());
    const auto serial = _activeSerial;
    request.callback =
        [guard, application, serial](App::RecomputeRequest&, App::RecomputeResult& result) {
            if (!application) {
                return;
            }

            auto completed = std::make_shared<App::RecomputeResult>();
            completed->success = result.success;
            completed->failure = result.failure;
            completed->exception = std::move(result.exception);

            QMetaObject::invokeMethod(
                application,
                [guard, serial, completed]() mutable {
                    if (guard) {
                        guard->finish(serial, std::move(completed));
                    }
                },
                Qt::QueuedConnection
            );
        };

    App::GetApplication().queueRecomputeRequest(std::move(request));
}

void AsyncTaskRecompute::finish(std::uint64_t serial, CompletionData result)
{
    if (serial != _activeSerial) {
        return;
    }

    _activeCancellation.reset();
    _running = false;
    if (_runningChanged) {
        _runningChanged(false);
    }
    const bool current = serial == _editSerial && !_pendingRequest;
    if (current && result->success && result->failure == App::RecomputeFailure::None) {
        _completedSerial = serial;
    }
    else {
        _completedSerial = 0;
    }

    Completion completion = std::move(_activeCompletion);
    _activeCompletion = {};
    if (current && completion) {
        completion(*result);
    }
}

}  // namespace Gui
