// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2026 The FreeCAD project association AISBL              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <gtest/gtest.h>

#include <FCConfig.h>
#include <Mod/Part/App/SignalException.h>
#include <Standard_Failure.hxx>

#include <cmath>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>

class SignalExceptionTest: public ::testing::Test
{
};

TEST_F(SignalExceptionTest, normalExecution)
{
    int result = 0;
    Part::SignalException::guard([&] { result = 42; });
    EXPECT_EQ(result, 42);
}

TEST_F(SignalExceptionTest, cppExceptionPassesThrough)
{
    EXPECT_THROW(
        Part::SignalException::guard([&] { throw std::runtime_error("test"); }),
        std::runtime_error
    );
}

TEST_F(SignalExceptionTest, nestedGuardsWork)
{
    int depth = 0;
    Part::SignalException::guard([&] {
        depth = 1;
        Part::SignalException::guard([&] { depth = 2; });
        EXPECT_EQ(depth, 2);
    });
    EXPECT_EQ(depth, 2);
}

#if defined(__GNUC__) && (defined(FC_OS_LINUX) || defined(FC_OS_MACOSX))

TEST_F(SignalExceptionTest, exceptionContainsStackTrace)
{
    try {
        Part::SignalException::guard([&] { raise(SIGSEGV); });
        FAIL() << "Expected Standard_Failure";
    }
    catch (const Standard_Failure& e) {
        std::string trace = e.GetStackString();
        std::cerr << "Stack trace from exception:\n" << trace << std::endl;
        EXPECT_FALSE(trace.empty()) << "Exception should contain a stack trace";
        // Check for the demangled function name - this verifies both
        // that the stack trace captures the right frames and that
        // demangling works.
        EXPECT_NE(trace.find("Part::SignalException::guard"), std::string::npos)
            << "Stack trace should contain demangled Part::SignalException::guard";
    }
}

TEST_F(SignalExceptionTest, catchesSIGSEGV)
{
    EXPECT_THROW(
        Part::SignalException::guard([&] {
            // Raise SIGSEGV explicitly rather than dereferencing null,
            // which would be undefined behavior even in a test.
            raise(SIGSEGV);
        }),
        Standard_Failure
    );
}

TEST_F(SignalExceptionTest, catchesSIGFPE)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGFPE); }), Standard_Failure);
}

TEST_F(SignalExceptionTest, catchesSIGHUP)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGHUP); }), Standard_Failure);
}

// SIGINT is intentionally not tested - OCC's handler sets a cancellation
// flag rather than converting it to an exception via longjmp.

TEST_F(SignalExceptionTest, catchesSIGQUIT)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGQUIT); }), Standard_Failure);
}

# ifdef SIGSYS
TEST_F(SignalExceptionTest, catchesSIGSYS)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGSYS); }), Standard_Failure);
}
# endif

TEST_F(SignalExceptionTest, catchesSIGBUS)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGBUS); }), Standard_Failure);
}

TEST_F(SignalExceptionTest, catchesSIGILL)
{
    EXPECT_THROW(Part::SignalException::guard([&] { raise(SIGILL); }), Standard_Failure);
}

TEST_F(SignalExceptionTest, fpeNotArmedOutsideGuard)
{
    // Run a guard to install/uninstall handlers
    Part::SignalException::guard([&] {});

    // After guard exits, FPE should NOT be armed.
    // A floating-point division by zero should produce inf, not SIGFPE.
    volatile double zero = 0.0;
    volatile double result = 1.0 / zero;
    EXPECT_TRUE(std::isinf(result));
}

TEST_F(SignalExceptionTest, signalHandlersRestoredAfterGuard)
{
    // Capture SIGSEGV handler before guard
    struct sigaction before = {};
    sigaction(SIGSEGV, nullptr, &before);

    Part::SignalException::guard([&] {});

    // Capture SIGSEGV handler after guard
    struct sigaction after = {};
    sigaction(SIGSEGV, nullptr, &after);

    // Handler should be restored to what it was before
    EXPECT_EQ(before.sa_handler, after.sa_handler);
    EXPECT_EQ(before.sa_sigaction, after.sa_sigaction);
}

TEST_F(SignalExceptionTest, signalHandlersRestoredAfterException)
{
    struct sigaction before = {};
    sigaction(SIGSEGV, nullptr, &before);

    try {
        Part::SignalException::guard([&] { raise(SIGSEGV); });
    }
    catch (const Standard_Failure&) {
        // expected
    }

    struct sigaction after = {};
    sigaction(SIGSEGV, nullptr, &after);

    EXPECT_EQ(before.sa_handler, after.sa_handler);
    EXPECT_EQ(before.sa_sigaction, after.sa_sigaction);
}

TEST_F(SignalExceptionTest, nestedGuardRestoresCorrectly)
{
    struct sigaction before = {};
    sigaction(SIGSEGV, nullptr, &before);

    Part::SignalException::guard([&] {
        // Inner guard should save/restore independently
        Part::SignalException::guard([&] {});
    });

    struct sigaction after = {};
    sigaction(SIGSEGV, nullptr, &after);

    EXPECT_EQ(before.sa_handler, after.sa_handler);
    EXPECT_EQ(before.sa_sigaction, after.sa_sigaction);
}

// Conductor: the main thread steps through phases, workers wait at each phase.
struct Conductor
{
    std::mutex mtx;
    std::condition_variable cv;
    int phase = 0;

    void waitFor(int target)
    {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&] { return phase >= target; });
    }

    void advance()
    {
        {
            std::lock_guard lock(mtx);
            ++phase;
        }
        cv.notify_all();
    }
};

// Thread 1 enters first and exits first (while thread 2 is still inside).
// This is the bug case: the first entrant's exit must not uninstall handlers.
TEST_F(SignalExceptionTest, threadSafeFirstInFirstOut)
{
    Conductor c;
    bool thread2Caught = false;

    // Thread 1: enter guard, wait, exit guard while thread 2 is still inside
    std::thread t1([&] {
        Part::SignalException::guard([&] {
            c.advance();   // phase 1: thread 1 inside guard
            c.waitFor(3);  // wait for thread 2 to also be inside
        });
        c.advance();       // phase 4: thread 1 has exited guard
    });

    // Thread 2: enter guard, signal thread 1 to exit, then raise SIGSEGV
    std::thread t2([&] {
        try {
            Part::SignalException::guard([&] {
                c.waitFor(1);  // wait for thread 1 to be inside guard
                c.advance();   // phase 2: thread 2 inside guard
                // Not used but keeps phase sequence logical
                c.advance();   // phase 3: tell thread 1 it can exit
                c.waitFor(4);  // wait for thread 1 to have exited
                raise(SIGSEGV);
            });
        }
        catch (const Standard_Failure&) {
            thread2Caught = true;
        }
    });

    t1.join();
    t2.join();
    EXPECT_TRUE(thread2Caught) << "Thread 2 should catch SIGSEGV after thread 1 exits guard";
}

// Thread 1 enters first but exits last (thread 2 exits first).
// This is the natural stack-like ordering.
TEST_F(SignalExceptionTest, threadSafeFirstInLastOut)
{
    Conductor c;
    bool thread1Caught = false;

    // Thread 1: enter guard, wait for thread 2 to exit, then raise SIGSEGV
    std::thread t1([&] {
        try {
            Part::SignalException::guard([&] {
                c.advance();   // phase 1: thread 1 inside guard
                c.waitFor(3);  // wait for thread 2 to have exited
                raise(SIGSEGV);
            });
        }
        catch (const Standard_Failure&) {
            thread1Caught = true;
        }
    });

    // Thread 2: enter guard, wait, exit guard while thread 1 is still inside
    std::thread t2([&] {
        Part::SignalException::guard([&] {
            c.waitFor(1);  // wait for thread 1 to be inside guard
            c.advance();   // phase 2: thread 2 inside guard
        });
        c.advance();       // phase 3: thread 2 has exited guard
    });

    t1.join();
    t2.join();
    EXPECT_TRUE(thread1Caught) << "Thread 1 should catch SIGSEGV after thread 2 exits guard";
}

// Verify signal handlers are restored after all threads exit guard.
TEST_F(SignalExceptionTest, handlersRestoredAfterConcurrentGuards)
{
    struct sigaction before = {};
    sigaction(SIGSEGV, nullptr, &before);

    Conductor c;

    std::thread t1([&] {
        Part::SignalException::guard([&] {
            c.advance();   // phase 1: inside
            c.waitFor(2);  // wait for t2 to be inside
        });
        c.advance();       // phase 3: t1 exited
    });

    std::thread t2([&] {
        Part::SignalException::guard([&] {
            c.waitFor(1);  // wait for t1 to be inside
            c.advance();   // phase 2: inside
            c.waitFor(3);  // wait for t1 to exit
        });
    });

    t1.join();
    t2.join();

    struct sigaction after = {};
    sigaction(SIGSEGV, nullptr, &after);

    EXPECT_EQ(before.sa_handler, after.sa_handler);
    EXPECT_EQ(before.sa_sigaction, after.sa_sigaction);
}

#endif
