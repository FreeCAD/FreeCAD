// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QSignalSpy>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTest>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeatureTest.h>
#include <App/MainThreadSignal.h>
#include <Base/Parameter.h>

#include <Gui/AsyncRecomputeProgressDialog.h>
#include <Gui/AsyncPreviewController.h>
#include <Gui/AsyncPreviewSession.h>
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

namespace
{
constexpr auto AsyncPrefsPath = "User parameter:BaseApp/Preferences/Document";

struct CallbackState
{
    int completedCount = 0;
    bool success = true;
    bool canceled = false;
    App::RecomputeFailure failure = App::RecomputeFailure::None;
    std::string message;
    int appliedCount = 0;
    bool appliedSuccess = true;
    bool appliedCanceled = false;
    int stateChangedCount = 0;
};

struct FakeMainThreadInvokeItem
{
    std::function<void()> fn;
    std::mutex mutex;
    bool done = false;
    std::condition_variable changed;
};

struct FakeMainThreadHookState
{
    std::mutex mutex;
    std::deque<std::shared_ptr<FakeMainThreadInvokeItem>> queued;
    std::thread::id mainThreadId;
    int pumpCount = 0;
};

FakeMainThreadHookState& getFakeMainThreadHookState()
{
    static FakeMainThreadHookState state;
    return state;
}

void resetFakeMainThreadHooks()
{
    auto& state = getFakeMainThreadHookState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.queued.clear();
    state.mainThreadId = std::this_thread::get_id();
    state.pumpCount = 0;
}

bool fakeIsMainThread()
{
    return std::this_thread::get_id() == getFakeMainThreadHookState().mainThreadId;
}

void fakeInvokeOnMainThread(std::function<void()>&& fn, bool blocking)
{
    if (fakeIsMainThread()) {
        fn();
        return;
    }

    auto item = std::make_shared<FakeMainThreadInvokeItem>();
    item->fn = std::move(fn);

    {
        std::lock_guard<std::mutex> lock(getFakeMainThreadHookState().mutex);
        getFakeMainThreadHookState().queued.push_back(item);
    }

    if (!blocking) {
        return;
    }

    std::unique_lock<std::mutex> lock(item->mutex);
    item->changed.wait(lock, [&item]() { return item->done; });
}

void fakePumpMainThreadEvents()
{
    std::deque<std::shared_ptr<FakeMainThreadInvokeItem>> queued;
    {
        auto& state = getFakeMainThreadHookState();
        std::lock_guard<std::mutex> lock(state.mutex);
        ++state.pumpCount;
        queued.swap(state.queued);
    }

    for (const auto& item : queued) {
        item->fn();
        {
            std::lock_guard<std::mutex> lock(item->mutex);
            item->done = true;
        }
        item->changed.notify_all();
    }
}
}  // namespace

class testAsyncPreviewController: public QObject
{
    Q_OBJECT

public:
    testAsyncPreviewController()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void init()  // NOLINT
    {
        params = App::GetApplication().GetParameterGroupByPath(AsyncPrefsPath);
        oldAsyncEnabled = params->GetBool("EnableAsyncRecompute", true);
        params->SetBool("EnableAsyncRecompute", true);

        callbackState = {};
        App::FeatureTestAsyncBlocker::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("async_preview_controller");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        QVERIFY(doc != nullptr);

        blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
            doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
        );
        QVERIFY(blocker != nullptr);

        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [this]() {
            auto* object = static_cast<App::DocumentObject*>(blocker);
            return object ? App::RecomputeRequest::fromDocumentObject(*object)
                          : App::RecomputeRequest {};
        };
        callbacks.runSync = [this]() {
            if (blocker) {
                blocker->recomputeFeature();
            }
        };
        callbacks.onCompleted = [this](
                                    bool success,
                                    bool canceled,
                                    App::RecomputeFailure failure,
                                    const std::string& message
                                ) {
            ++callbackState.completedCount;
            callbackState.success = success;
            callbackState.canceled = canceled;
            callbackState.failure = failure;
            callbackState.message = message;
        };
        callbacks.onAppliedResult = [this](bool success, bool canceled) {
            ++callbackState.appliedCount;
            callbackState.appliedSuccess = success;
            callbackState.appliedCanceled = canceled;
        };
        callbacks.onStateChanged = [this]() {
            ++callbackState.stateChangedCount;
        };
        controller = std::make_unique<Gui::AsyncPreviewController>(std::move(callbacks));
    }

    void cleanup()  // NOLINT
    {
        App::MainThreadSignalConfig::setHooks(nullptr, nullptr, nullptr);

        if (controller) {
            controller->stopPendingRecompute();
            App::FeatureTestAsyncBlocker::releaseBlocker();
            QTRY_VERIFY_WITH_TIMEOUT(!controller->hasOutstandingRecompute(), 2000);
            controller.reset();
        }
        else {
            App::FeatureTestAsyncBlocker::releaseBlocker();
        }

        blocker = nullptr;

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        doc = nullptr;
        docName.clear();

        if (params) {
            params->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            params = nullptr;
        }
    }

    void stopPendingRecomputeCancelsScheduledBurstBeforeQueueing()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->setSchedulerInterval(200);
        controller->scheduleRecompute();
        controller->scheduleRecompute();
        QVERIFY(controller->hasScheduledRecompute());

        controller->stopPendingRecompute();

        QVERIFY(!controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());
        QCOMPARE(callbackState.completedCount, 0);
        QCOMPARE(callbackState.appliedCount, 0);
        QCOMPARE(settledSpy.count(), 0);

        QTest::qWait(300);
        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 0);
    }

    void scheduledRecomputeBurstCoalescesIntoSingleRun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->setSchedulerInterval(20);
        controller->scheduleRecompute();
        controller->scheduleRecompute();
        controller->scheduleRecompute();

        QVERIFY(controller->hasScheduledRecompute());
        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 1, 2000);
        QTRY_VERIFY_WITH_TIMEOUT(controller->isInProgress(), 2000);
        QVERIFY(!controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(callbackState.stateChangedCount > 0);
    }

    void duplicateInFlightRequestSettlesOnlyAfterFinalRun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);

        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        QVERIFY(controller->isInProgress());
        QVERIFY(!controller->isQueued());

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);

        QVERIFY(controller->isInProgress());
        QVERIFY(controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 2, 2000);
        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(callbackState.stateChangedCount > 0);
        QVERIFY(!controller->hasOutstandingRecompute());
    }

    void stopPendingRecomputeCancelsInFlightAndClearsQueuedRerun()  // NOLINT
    {
        QSignalSpy settledSpy(controller.get(), &Gui::AsyncPreviewController::recomputeSettled);

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(controller->isQueued());

        controller->stopPendingRecompute();

        QVERIFY(controller->isCancelRequested());
        QVERIFY(controller->isInProgress());
        QVERIFY(!controller->isQueued());

        App::FeatureTestAsyncBlocker::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(App::FeatureTestAsyncBlocker::getExecutionCount(), 1, 2000);
        QTRY_COMPARE_WITH_TIMEOUT(settledSpy.count(), 1, 2000);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(!callbackState.success);
        QVERIFY(callbackState.canceled);
        QVERIFY(callbackState.failure == App::RecomputeFailure::Canceled);
        QCOMPARE(QString::fromStdString(callbackState.message), QString("User aborted"));
        QVERIFY(!callbackState.appliedSuccess);
        QVERIFY(callbackState.appliedCanceled);
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
        QVERIFY(!controller->isQueued());
        QVERIFY(!controller->isCancelRequested());
    }

    void flushPendingRecomputeRunsScheduledRecomputeImmediately()  // NOLINT
    {
        blocker->touch();
        controller->setSchedulerInterval(200);
        controller->scheduleRecompute();
        QVERIFY(controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(50ms);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->flushPendingRecompute();
        releaser.join();

        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasScheduledRecompute());
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
    }

    void flushPendingRecomputeWaitsForQueuedRunWithoutExtraRerun()  // NOLINT
    {
        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

        blocker->touch();
        controller->requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(controller->isQueued());

        std::thread releaser([]() {
            std::this_thread::sleep_for(50ms);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->flushPendingRecompute();
        releaser.join();

        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 2);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasOutstandingRecompute());
        QVERIFY(!controller->isInProgress());
        QVERIFY(!controller->isQueued());
    }

    void delayedPreviewUiDoesNotReuseTimerAcrossGenerations()  // NOLINT
    {
        QWidget statusWidget;
        QHBoxLayout layout(&statusWidget);
        QProgressBar progressBar(&statusWidget);
        QLabel statusLabel(&statusWidget);
        QPushButton cancelButton(&statusWidget);
        layout.addWidget(&progressBar);
        layout.addWidget(&statusLabel);
        layout.addWidget(&cancelButton);

        Gui::AsyncPreviewController::Callbacks callbacks;
        callbacks.makeRequest = [this]() {
            auto* object = static_cast<App::DocumentObject*>(blocker);
            return object ? App::RecomputeRequest::fromDocumentObject(*object)
                          : App::RecomputeRequest {};
        };
        callbacks.runSync = [this]() {
            if (blocker) {
                blocker->recomputeFeature();
            }
        };
        Gui::AsyncPreviewSession session(std::move(callbacks));
        session.bindWidgets(
            {&statusWidget, &progressBar, &statusLabel, &cancelButton},
            [](const char* text) { return QString::fromUtf8(text); }
        );
        session.setProgressUiDelay(300);

        blocker->touch();
        session.requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
        QVERIFY(session.controller()->isInProgress());
        QVERIFY(statusLabel.isHidden());

        QTest::qWait(120);

        blocker->touch();
        session.requestRecompute(/*waitForCompletion=*/false);
        QVERIFY(session.controller()->isQueued());

        QTest::qWait(220);

        QVERIFY(session.controller()->isInProgress());
        QVERIFY2(
            statusLabel.isHidden(),
            "an older delay timer must not reveal progress UI for a newer preview generation"
        );

        App::FeatureTestAsyncBlocker::releaseBlocker();
        QTRY_VERIFY_WITH_TIMEOUT(!session.hasOutstandingRecompute(), 2000);
        QVERIFY(statusLabel.isHidden());
    }

    void waitForCompletionPumpsMainThreadHooksWhileBlocked()  // NOLINT
    {
        resetFakeMainThreadHooks();
        App::MainThreadSignalConfig::setHooks(
            &fakeIsMainThread,
            &fakeInvokeOnMainThread,
            &fakePumpMainThreadEvents
        );

        blocker->touch();
        bool helperObservedStart = false;
        bool helperInvokeReturned = false;
        bool queuedMainThreadCallRan = false;

        std::thread helper([&]() {
            helperObservedStart = App::FeatureTestAsyncBlocker::waitUntilStarted(2s);
            if (!helperObservedStart) {
                return;
            }

            App::MainThreadSignalConfig::invoke(
                [&queuedMainThreadCallRan]() { queuedMainThreadCallRan = true; },
                /*blocking=*/true
            );
            helperInvokeReturned = true;
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        controller->requestRecompute(/*waitForCompletion=*/true);
        helper.join();

        QVERIFY(helperObservedStart);
        QVERIFY(queuedMainThreadCallRan);
        QVERIFY(helperInvokeReturned);
        QVERIFY(getFakeMainThreadHookState().pumpCount > 0);
        QCOMPARE(callbackState.completedCount, 1);
        QCOMPARE(callbackState.appliedCount, 1);
        QVERIFY(callbackState.success);
        QVERIFY(!callbackState.canceled);
        QVERIFY(!controller->hasOutstandingRecompute());
    }

    void documentRecomputeProgressDialogUsesStableDocumentRequestWithoutObjectAnchor()  // NOLINT
    {
        auto* removed = doc->addObject("App::FeatureTest", "RemovedFeature");
        QVERIFY(removed != nullptr);
        const std::string removedName = removed->getNameInDocument();
        doc->removeObject(removedName.c_str());
        QVERIFY(doc->getObject(removedName.c_str()) == nullptr);

        blocker->touch();
        bool releaserObservedStart = false;
        std::thread releaser([&]() {
            releaserObservedStart = App::FeatureTestAsyncBlocker::waitUntilStarted(2s);
            App::FeatureTestAsyncBlocker::releaseBlocker();
        });

        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            nullptr,
            QStringLiteral("Document recompute"),
            QStringLiteral("Computing document..."),
            doc,
            /*force=*/false,
            [this]() {
                if (doc) {
                    doc->recompute();
                }
            }
        );
        releaser.join();

        QVERIFY(releaserObservedStart);
        QVERIFY(outcome.success);
        QVERIFY(!outcome.canceled);
        QCOMPARE(outcome.failure, App::RecomputeFailure::None);
        QCOMPARE(App::FeatureTestAsyncBlocker::getExecutionCount(), 1);
    }

    void documentRecomputeProgressDialogDoesNotNeedObjectAnchor()  // NOLINT
    {
        const std::string emptyDocName = App::GetApplication().getUniqueDocumentName(
            "async_progress_empty"
        );
        App::Document* emptyDoc = App::GetApplication().newDocument(emptyDocName.c_str(), "testUser");
        QVERIFY(emptyDoc != nullptr);

        int recomputeCount = 0;
        const auto outcome = Gui::runAsyncDocumentRecomputeProgressDialog(
            nullptr,
            QStringLiteral("Document recompute"),
            QStringLiteral("Computing document..."),
            emptyDoc,
            /*force=*/false,
            [&recomputeCount]() { ++recomputeCount; }
        );

        App::GetApplication().closeDocument(emptyDocName.c_str());

        QVERIFY(outcome.success);
        QVERIFY(!outcome.canceled);
        QCOMPARE(outcome.failure, App::RecomputeFailure::None);
        QCOMPARE(recomputeCount, 0);
    }

private:
    Base::Reference<ParameterGrp> params;
    bool oldAsyncEnabled = true;
    CallbackState callbackState;
    std::string docName;
    App::Document* doc = nullptr;
    App::FeatureTestAsyncBlocker* blocker = nullptr;
    std::unique_ptr<Gui::AsyncPreviewController> controller;
};

QTEST_MAIN(testAsyncPreviewController)

#include "AsyncPreviewControllerTest.moc"
