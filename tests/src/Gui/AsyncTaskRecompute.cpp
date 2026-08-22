// SPDX-License-Identifier: LGPL-2.1-or-later

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <memory>
#include <thread>

#include <QCoreApplication>
#include <QEvent>
#include <QMetaObject>
#include <QThread>

#include <gtest/gtest.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeatureTest.h>
#include <App/MainThreadSignal.h>
#include <Gui/AsyncTaskRecompute.h>
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

namespace
{

bool testIsMainThread()
{
    auto* app = QCoreApplication::instance();
    return !app || QThread::currentThread() == app->thread();
}

std::mutex testSignalMutex;
std::condition_variable testSignalChanged;
bool testSignalInvocationPending = false;

void testInvokeOnMain(std::function<void()>&& function, bool blocking)
{
    auto* app = QCoreApplication::instance();
    if (!app || testIsMainThread()) {
        function();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(testSignalMutex);
        testSignalInvocationPending = true;
    }
    testSignalChanged.notify_all();

    QMetaObject::invokeMethod(
        app,
        [function = std::move(function)]() mutable { function(); },
        blocking ? Qt::BlockingQueuedConnection : Qt::QueuedConnection
    );
}

void testPumpMainThreadDispatches()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}

class ScopedMainThreadSignalHooks
{
public:
    ScopedMainThreadSignalHooks()
    {
        App::MainThreadSignalConfig::setHooks(
            &testIsMainThread,
            &testInvokeOnMain,
            &testPumpMainThreadDispatches
        );
    }

    ~ScopedMainThreadSignalHooks()
    {
        App::MainThreadSignalConfig::setHooks(nullptr, nullptr);
    }
};

}  // namespace

class AsyncTaskRecomputeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char appName[] = "Gui_tests_run";
            static char* argv[] = {appName, nullptr};
            new QCoreApplication(argc, argv);
        }
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("async_gui_recompute");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        if (!_docName.empty() && App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    bool processUntil(const std::function<bool()>& predicate)
    {
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            QCoreApplication::processEvents();
            std::this_thread::sleep_for(1ms);
        }
        QCoreApplication::processEvents();
        return predicate();
    }

    std::string _docName;
    App::Document* _doc {};
};

TEST_F(AsyncTaskRecomputeTest, CompletionIsDeliveredOnMainThread)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    App::FeatureTestAsyncBlocker::releaseBlocker();

    Gui::AsyncTaskRecompute task;
    bool callbackDone = false;
    bool callbackOnMainThread = false;
    auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
    task.schedule(std::move(request), 0, [&](App::RecomputeResult& result) {
        callbackDone = true;
        callbackOnMainThread = QThread::currentThread() == QCoreApplication::instance()->thread();
        EXPECT_TRUE(result.success);
    });

    EXPECT_TRUE(processUntil([&] { return callbackDone; }));
    EXPECT_TRUE(callbackOnMainThread);
    EXPECT_TRUE(task.hasCurrentSuccessfulPreview());
}

TEST_F(AsyncTaskRecomputeTest, CancellationInvalidatesCurrentPreview)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    Gui::AsyncTaskRecompute task;
    bool callbackDone = false;
    App::RecomputeFailure failure = App::RecomputeFailure::None;
    auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
    task.schedule(std::move(request), 0, [&](App::RecomputeResult& result) {
        callbackDone = true;
        failure = result.failure;
    });

    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));
    task.cancel();
    App::FeatureTestAsyncBlocker::releaseBlocker();

    EXPECT_TRUE(processUntil([&] { return callbackDone; }));
    EXPECT_EQ(failure, App::RecomputeFailure::Canceled);
    EXPECT_FALSE(task.hasCurrentSuccessfulPreview());
}

TEST_F(AsyncTaskRecomputeTest, CloseDocumentPumpsBlockingMainThreadSignal)
{
    ScopedMainThreadSignalHooks hooks;

    {
        std::lock_guard<std::mutex> lock(testSignalMutex);
        testSignalInvocationPending = false;
    }

    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    blocker->touch();
    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocument(*_doc, true));

    {
        std::unique_lock<std::mutex> lock(testSignalMutex);
        ASSERT_TRUE(testSignalChanged.wait_for(lock, 2s, [] { return testSignalInvocationPending; }));
    }

    std::thread releaser([] {
        std::this_thread::sleep_for(25ms);
        App::FeatureTestAsyncBlocker::releaseBlocker();
    });

    EXPECT_TRUE(App::GetApplication().closeDocument(_docName.c_str()));
    releaser.join();
    _doc = nullptr;
}
