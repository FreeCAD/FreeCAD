// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <FCConfig.h>
#include <Mod/Part/App/SignalException.h>

#if defined(__GNUC__) && defined(FC_OS_LINUX)
# include <array>
# include <csignal>
# include <stdexcept>

namespace
{
void testHandler(int, siginfo_t*, void*)
{}

class SignalExceptionTest: public ::testing::Test
{
protected:
    static constexpr auto signals = std::to_array({SIGFPE, SIGSEGV});
    std::array<struct sigaction, signals.size()> original {};
    std::array<struct sigaction, signals.size()> expected {};
    std::array<bool, signals.size()> saved {};

    void SetUp() override
    {
        for (std::size_t i = 0; i < signals.size(); ++i) {
            ASSERT_EQ(sigaction(signals[i], nullptr, &original[i]), 0);
            saved[i] = true;
            struct sigaction action {};
            action.sa_sigaction = testHandler;
            action.sa_flags = SA_SIGINFO | SA_RESTART;
            sigemptyset(&action.sa_mask);
            sigaddset(&action.sa_mask, SIGUSR1);
            ASSERT_EQ(sigaction(signals[i], &action, nullptr), 0);
            ASSERT_EQ(sigaction(signals[i], nullptr, &expected[i]), 0);
        }
    }

    void TearDown() override
    {
        for (std::size_t i = 0; i < signals.size(); ++i) {
            if (saved[i]) {
                EXPECT_EQ(sigaction(signals[i], &original[i], nullptr), 0);
            }
        }
    }

    void checkRestored()
    {
        for (std::size_t i = 0; i < signals.size(); ++i) {
            struct sigaction action {};
            ASSERT_EQ(sigaction(signals[i], nullptr, &action), 0);
            EXPECT_EQ(action.sa_sigaction, expected[i].sa_sigaction);
            EXPECT_EQ(action.sa_flags, expected[i].sa_flags);
            for (int sig = 1; sig < NSIG; ++sig) {
                EXPECT_EQ(sigismember(&action.sa_mask, sig), sigismember(&expected[i].sa_mask, sig));
            }
        }
    }
};

TEST_F(SignalExceptionTest, restoresHandlersFlagsAndMasksAfterNestedAndRepeatedScopes)
{
    for (int iteration = 0; iteration < 2; ++iteration) {
        {
            Part::SignalException outer;
            {
                Part::SignalException inner;
            }
            struct sigaction action {};
            ASSERT_EQ(sigaction(SIGFPE, nullptr, &action), 0);
            EXPECT_NE(action.sa_sigaction, testHandler);
        }
        checkRestored();
    }
}

TEST_F(SignalExceptionTest, restoresHandlersDuringExceptionUnwinding)
{
    EXPECT_THROW(
        {
            Part::SignalException guard;
            throw std::runtime_error("Operation failed");
        },
        std::runtime_error
    );
    checkRestored();
}
}  // namespace
#endif
