// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/Application.h>

#include <QObject>
#include <QTimer>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace Gui
{

/**
 * @brief Small GUI-side coordinator for one asynchronous recompute preview.
 *
 * Recompute itself remains an App concern. This class only handles request
 * replacement/cancellation, debounce, edit serials, and delivery of the
 * completion callback on the Qt thread.
 */
class GuiExport AsyncTaskRecompute: public QObject
{
public:
    using Completion = std::function<void(App::RecomputeResult&)>;
    using RunningChanged = std::function<void(bool)>;

    explicit AsyncTaskRecompute(QObject* parent = nullptr);
    ~AsyncTaskRecompute() override;

    AsyncTaskRecompute(const AsyncTaskRecompute&) = delete;
    AsyncTaskRecompute& operator=(const AsyncTaskRecompute&) = delete;

    /// Queue a request after delayMs. A new schedule supersedes pending work.
    void schedule(App::RecomputeRequest request, int delayMs, Completion completion);

    /// Called on the GUI thread when worker ownership starts or ends.
    void setRunningChanged(RunningChanged callback);

    /// Start a pending request immediately, preserving its edit serial.
    bool flushPending();

    /// Cooperatively cancel the active request and discard pending work.
    void cancel();

    bool isPending() const;
    bool isRunning() const;
    bool isSettling() const;

    /// True only when the latest edit has a successfully completed request.
    bool hasCurrentSuccessfulPreview() const;
    std::uint64_t editSerial() const;
    std::uint64_t completedSerial() const;

private:
    using CompletionData = std::shared_ptr<App::RecomputeResult>;

    void startPending();
    void finish(std::uint64_t serial, CompletionData result);

    QTimer _timer;
    std::optional<App::RecomputeRequest> _pendingRequest;
    Completion _pendingCompletion;
    Completion _activeCompletion;
    RunningChanged _runningChanged;
    App::RecomputeCancellationHandle _activeCancellation;
    std::uint64_t _editSerial {0};
    std::uint64_t _activeSerial {0};
    std::uint64_t _completedSerial {0};
    bool _running {false};
};

}  // namespace Gui
