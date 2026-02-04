// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <mutex>
#include <vector>

#include "Base/Console.h"

namespace
{

struct CapturedLog
{
    std::string notifier;
    std::string msg;
    Base::LogStyle level;
    Base::IntendedRecipient recipient;
    Base::ContentType content;
};

class CapturingLogger final: public Base::ILogger
{
public:
    explicit CapturingLogger(std::vector<CapturedLog>& out, std::mutex& outMutex)
        : output(out)
        , mutex(outMutex)
    {}

    void sendLog(
        const std::string& notifiername,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient recipient,
        Base::ContentType content
    ) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        output.push_back({notifiername, msg, level, recipient, content});
    }

    const char* name() override
    {
        return "CapturingLogger";
    }

private:
    std::vector<CapturedLog>& output;
    std::mutex& mutex;
};

class ScopedObserver
{
public:
    explicit ScopedObserver(Base::ILogger& logger)
        : logger(logger)
    {
        Base::Console().attachObserver(&logger);
    }

    ~ScopedObserver()
    {
        Base::Console().detachObserver(&logger);
    }

    ScopedObserver(const ScopedObserver&) = delete;
    ScopedObserver(ScopedObserver&&) = delete;
    ScopedObserver& operator=(const ScopedObserver&) = delete;
    ScopedObserver& operator=(ScopedObserver&&) = delete;

private:
    Base::ILogger& logger;
};

}  // namespace

TEST(Console, DirectModeDeliversToObserver)
{
    std::mutex mutex;
    std::vector<CapturedLog> logs;
    CapturingLogger logger(logs, mutex);
    ScopedObserver scoped(logger);

    Base::Console().setConnectionMode(Base::ConsoleSingleton::Direct);

    Base::Console().message("Hello %s", "world");

    ASSERT_EQ(1U, logs.size());
    EXPECT_EQ(Base::LogStyle::Message, logs[0].level);
    EXPECT_EQ(std::string("Hello world"), logs[0].msg);
}

TEST(Console, QueuedModeFallsBackToDirectWithoutHandler)
{
    std::mutex mutex;
    std::vector<CapturedLog> logs;
    CapturingLogger logger(logs, mutex);
    ScopedObserver scoped(logger);

    Base::Console().setPostEventHandler({});
    Base::Console().setConnectionMode(Base::ConsoleSingleton::Queued);

    Base::Console().warning("Warn %d", 7);

    Base::Console().setConnectionMode(Base::ConsoleSingleton::Direct);

    ASSERT_EQ(1U, logs.size());
    EXPECT_EQ(Base::LogStyle::Warning, logs[0].level);
    EXPECT_EQ(std::string("Warn 7"), logs[0].msg);
}

TEST(Console, QueuedModeCallsPostEventHandlerWhenSet)
{
    int called = 0;
    Base::Console().setPostEventHandler([&called](
                                            Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type,
                                            Base::IntendedRecipient recipient,
                                            Base::ContentType content,
                                            const std::string& notifier,
                                            const std::string& msg
                                        ) {
        EXPECT_EQ(Base::ConsoleSingleton::MsgType_Log, type);
        EXPECT_EQ(Base::IntendedRecipient::All, recipient);
        EXPECT_EQ(Base::ContentType::Untranslated, content);
        EXPECT_EQ(std::string("N"), notifier);
        EXPECT_EQ(std::string("X"), msg);
        called++;
    });

    Base::Console().setConnectionMode(Base::ConsoleSingleton::Queued);
    Base::Console().log(std::string("N"), "%s", "X");
    Base::Console().setConnectionMode(Base::ConsoleSingleton::Direct);
    Base::Console().setPostEventHandler({});

    EXPECT_EQ(1, called);
}

TEST(Console, RefreshCallsHandlerWhenEnabled)
{
    int called = 0;
    Base::Console().setRefreshHandler([&called]() { called++; });

    Base::Console().enableRefresh(true);
    Base::Console().refresh();
    EXPECT_EQ(1, called);

    Base::Console().enableRefresh(false);
    Base::Console().refresh();
    EXPECT_EQ(1, called);

    Base::Console().setRefreshHandler({});
    Base::Console().enableRefresh(true);
}
