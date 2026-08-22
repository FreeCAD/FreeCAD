// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/scope_exit.hpp>
#include <gtest/gtest.h>

#include "App/Application.h"
#include "App/Document.h"
#include "App/FeatureTest.h"
#include <src/App/InitApplication.h>

using namespace std::chrono_literals;

class AsyncRecomputeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("async_recompute");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        if (!_docName.empty() && App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    std::string _docName;
    App::Document* _doc {};
};

TEST_F(AsyncRecomputeTest, CloseDocumentWaitsForInFlightAsyncRecompute)
{
    auto* object = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(object, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    object->touch();

    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*object));

    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto closeFuture = std::async(std::launch::async, [this] {
        return App::GetApplication().closeDocument(_docName.c_str());
    });

    EXPECT_EQ(closeFuture.wait_for(50ms), std::future_status::timeout);

    App::FeatureTestAsyncBlocker::releaseBlocker();

    ASSERT_EQ(closeFuture.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(closeFuture.get());

    _doc = nullptr;
}

TEST_F(AsyncRecomputeTest, WorkerSafetyIsCheckedFromRequest)
{
    auto* safeObject = dynamic_cast<App::FeatureTest*>(
        _doc->addObject("App::FeatureTest", "SafeFeature")
    );
    auto* unsafeObject = dynamic_cast<App::FeatureTestAttribute*>(
        _doc->addObject("App::FeatureTestAttribute", "UnsafeFeature")
    );

    ASSERT_NE(safeObject, nullptr);
    ASSERT_NE(unsafeObject, nullptr);

    EXPECT_TRUE(
        App::GetApplication().canRecomputeRequestOnWorker(
            App::RecomputeRequest::fromDocumentObject(*safeObject)
        )
    );
    EXPECT_FALSE(
        App::GetApplication().canRecomputeRequestOnWorker(
            App::RecomputeRequest::fromDocumentObject(*unsafeObject)
        )
    );
    EXPECT_FALSE(
        App::GetApplication().canRecomputeRequestOnWorker(App::RecomputeRequest::fromDocument(*_doc))
    );
}

namespace
{

struct CallbackResults
{
    void add(App::RecomputeFailure failure)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            failures.push_back(failure);
        }
        changed.notify_all();
    }

    bool waitForCount(std::size_t count)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return changed.wait_for(lock, 2s, [&] { return failures.size() == count; });
    }

    std::mutex mutex;
    std::condition_variable changed;
    std::vector<App::RecomputeFailure> failures;
};

}  // namespace

TEST_F(AsyncRecomputeTest, CloseDocumentCancelsActiveRequest)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    CallbackResults callbacks;
    auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
    request.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto closeFuture = std::async(std::launch::async, [this] {
        return App::GetApplication().closeDocument(_docName.c_str());
    });
    EXPECT_EQ(closeFuture.wait_for(50ms), std::future_status::timeout);

    App::FeatureTestAsyncBlocker::releaseBlocker();
    ASSERT_EQ(closeFuture.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(closeFuture.get());
    ASSERT_TRUE(callbacks.waitForCount(1));

    std::lock_guard<std::mutex> lock(callbacks.mutex);
    ASSERT_EQ(callbacks.failures.size(), 1U);
    EXPECT_EQ(callbacks.failures.front(), App::RecomputeFailure::Canceled);
    _doc = nullptr;
}

TEST_F(AsyncRecomputeTest, CancelQueuedRequestBeforeItStarts)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    CallbackResults callbacks;
    auto firstRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    firstRequest.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    App::GetApplication().queueRecomputeRequest(firstRequest);
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto queuedRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    queuedRequest.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    const auto queuedCancellation = queuedRequest.cancellation;
    App::GetApplication().queueRecomputeRequest(std::move(queuedRequest));

    EXPECT_TRUE(App::GetApplication().cancelRecomputeRequest(queuedCancellation));
    App::FeatureTestAsyncBlocker::releaseBlocker();

    ASSERT_TRUE(callbacks.waitForCount(2));
    std::lock_guard<std::mutex> lock(callbacks.mutex);
    ASSERT_EQ(callbacks.failures.size(), 2U);
    EXPECT_EQ(callbacks.failures[0], App::RecomputeFailure::None);
    EXPECT_EQ(callbacks.failures[1], App::RecomputeFailure::Canceled);
}

TEST_F(AsyncRecomputeTest, CancelRunningRequestReportsCanceled)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    CallbackResults callbacks;
    auto request = App::RecomputeRequest::fromDocumentObject(*blocker);
    request.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    const auto cancellation = request.cancellation;
    App::GetApplication().queueRecomputeRequest(std::move(request));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    EXPECT_TRUE(App::GetApplication().cancelRecomputeRequest(cancellation));
    App::FeatureTestAsyncBlocker::releaseBlocker();

    ASSERT_TRUE(callbacks.waitForCount(1));
    std::lock_guard<std::mutex> lock(callbacks.mutex);
    ASSERT_EQ(callbacks.failures.size(), 1U);
    EXPECT_EQ(callbacks.failures.front(), App::RecomputeFailure::Canceled);
}

TEST_F(AsyncRecomputeTest, SameObjectRequestsUseIndependentCancellationTokens)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    ASSERT_NE(blocker, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    CallbackResults callbacks;
    auto firstRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    firstRequest.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    const auto firstCancellation = firstRequest.cancellation;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    auto secondRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    secondRequest.callback = [&callbacks](App::RecomputeRequest&, App::RecomputeResult& result) {
        callbacks.add(result.failure);
    };
    const auto secondCancellation = secondRequest.cancellation;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    EXPECT_TRUE(App::GetApplication().cancelRecomputeRequest(firstCancellation));
    EXPECT_FALSE(
        App::GetApplication().cancelRecomputeRequest(
            std::make_shared<App::RecomputeCancellationState>()
        )
    );
    EXPECT_TRUE(secondCancellation);
    EXPECT_FALSE(secondCancellation->isCanceled());

    App::FeatureTestAsyncBlocker::releaseBlocker();

    ASSERT_TRUE(callbacks.waitForCount(2));
    std::lock_guard<std::mutex> lock(callbacks.mutex);
    ASSERT_EQ(callbacks.failures.size(), 2U);
    EXPECT_EQ(callbacks.failures[0], App::RecomputeFailure::Canceled);
    EXPECT_EQ(callbacks.failures[1], App::RecomputeFailure::None);
}
