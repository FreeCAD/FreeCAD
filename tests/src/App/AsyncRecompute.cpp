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
#include <future>
#include <thread>

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

TEST_F(AsyncRecomputeTest, QueuedDuplicateRequestsAreCoalesced)
{
    auto* blocker = dynamic_cast<App::FeatureTestAsyncBlocker*>(
        _doc->addObject("App::FeatureTestAsyncBlocker", "BlockingFeature")
    );
    auto* safeObject = dynamic_cast<App::FeatureTest*>(
        _doc->addObject("App::FeatureTest", "SafeFeature")
    );
    ASSERT_NE(blocker, nullptr);
    ASSERT_NE(safeObject, nullptr);

    App::FeatureTestAsyncBlocker::resetBlocker();
    BOOST_SCOPE_EXIT_ALL(&)
    {
        App::FeatureTestAsyncBlocker::releaseBlocker();
    };

    blocker->touch();
    App::GetApplication().queueRecomputeRequest(App::RecomputeRequest::fromDocumentObject(*blocker));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    int callbackCount = 0;
    const auto onQueuedCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++callbackCount;
        callbackChanged.notify_all();
    };

    safeObject->touch();
    App::RecomputeRequest firstRequest = App::RecomputeRequest::fromDocumentObject(*safeObject);
    firstRequest.callback = onQueuedCallback;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));

    safeObject->touch();
    App::RecomputeRequest secondRequest = App::RecomputeRequest::fromDocumentObject(*safeObject);
    secondRequest.callback = onQueuedCallback;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return callbackCount == 2; }));
    EXPECT_EQ(safeObject->ExecCount.getValue(), 1);
}

TEST_F(AsyncRecomputeTest, InFlightDuplicateRequestsScheduleSingleRerun)
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

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    int totalCallbacks = 0;
    int firstRunCallbacks = 0;
    int rerunCallbacks = 0;

    const auto onFirstRunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++firstRunCallbacks;
        callbackChanged.notify_all();
    };
    const auto onRerunCallback = [&](App::RecomputeRequest&, App::RecomputeResult&) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        ++totalCallbacks;
        ++rerunCallbacks;
        callbackChanged.notify_all();
    };

    blocker->touch();
    App::RecomputeRequest firstRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    firstRequest.callback = onFirstRunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(firstRequest));
    ASSERT_TRUE(App::FeatureTestAsyncBlocker::waitUntilStarted(2s));

    blocker->touch();
    App::RecomputeRequest secondRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    secondRequest.callback = onRerunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(secondRequest));

    blocker->touch();
    App::RecomputeRequest thirdRequest = App::RecomputeRequest::fromDocumentObject(*blocker);
    thirdRequest.callback = onRerunCallback;
    App::GetApplication().queueRecomputeRequest(std::move(thirdRequest));

    App::FeatureTestAsyncBlocker::releaseBlocker();

    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    ASSERT_TRUE(callbackChanged.wait_for(callbackLock, 2s, [&] { return totalCallbacks == 3; }));
    EXPECT_EQ(firstRunCallbacks, 1);
    EXPECT_EQ(rerunCallbacks, 2);
    EXPECT_TRUE(App::FeatureTestAsyncBlocker::waitUntilExecutionCount(2, 2s));
    EXPECT_EQ(App::FeatureTestAsyncBlocker::getExecutionCount(), 2);
}
