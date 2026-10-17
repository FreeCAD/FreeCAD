// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include <Base/Parallel.h>

class ParallelTest: public ::testing::Test
{
protected:
    // void SetUp() override {};
    // void TearDown() override {};
};

TEST_F(ParallelTest, maxParallelThreadsIsAtLeastOne)
{
    // Act
    unsigned int threads = Base::maxParallelThreads();

    // Assert
    EXPECT_GE(threads, 1U);
}

TEST_F(ParallelTest, maxParallelThreadsHonoursLimit)
{
    // Act
    unsigned int threads = Base::maxParallelThreads(1);

    // Assert
    EXPECT_EQ(threads, 1U);
}

TEST_F(ParallelTest, visitsEveryIndexExactlyOnce)
{
    // Arrange
    constexpr int count = 1000;
    std::vector<int> visits(count, 0);

    // Act
    Base::parallelFor(0, count, [&visits](int index) { visits[index] += 1; });

    // Assert
    EXPECT_EQ(std::accumulate(visits.begin(), visits.end(), 0), count);
    EXPECT_EQ(*std::min_element(visits.begin(), visits.end()), 1);
    EXPECT_EQ(*std::max_element(visits.begin(), visits.end()), 1);
}

TEST_F(ParallelTest, visitsEveryIndexExactlyOnceWithLargeGrainSize)
{
    // Arrange
    constexpr int count = 37;
    std::vector<int> visits(count, 0);

    // Act
    Base::parallelFor(
        0,
        count,
        [&visits](int index) { visits[index] += 1; },
        /*grainSize*/ 64
    );

    // Assert
    EXPECT_EQ(std::accumulate(visits.begin(), visits.end(), 0), count);
    EXPECT_EQ(*std::max_element(visits.begin(), visits.end()), 1);
}

TEST_F(ParallelTest, startsAtBeginIndex)
{
    // Arrange
    std::vector<int> seen;
    std::mutex mutex;

    // Act
    Base::parallelFor(10, 20, [&](int index) {
        const std::lock_guard<std::mutex> guard(mutex);
        seen.push_back(index);
    });

    // Assert
    std::sort(seen.begin(), seen.end());
    ASSERT_EQ(seen.size(), 10U);
    EXPECT_EQ(seen.front(), 10);
    EXPECT_EQ(seen.back(), 19);
}

TEST_F(ParallelTest, emptyRangeDoesNothing)
{
    // Arrange
    std::atomic<int> calls {0};

    // Act
    Base::parallelFor(5, 5, [&calls](int) { calls += 1; });
    Base::parallelFor(5, 1, [&calls](int) { calls += 1; });

    // Assert
    EXPECT_EQ(calls.load(), 0);
}

TEST_F(ParallelTest, singleThreadRunsOnCallingThread)
{
    // Arrange
    const std::thread::id caller = std::this_thread::get_id();
    std::atomic<bool> foreign {false};

    // Act
    Base::parallelFor(
        0,
        100,
        [&](int) {
            if (std::this_thread::get_id() != caller) {
                foreign = true;
            }
        },
        /*grainSize*/ 1,
        /*threadLimit*/ 1
    );

    // Assert
    EXPECT_FALSE(foreign.load());
}

TEST_F(ParallelTest, rethrowsExceptionFromWorker)
{
    // Act & Assert
    EXPECT_THROW(
        Base::parallelFor(
            0,
            1000,
            [](int index) {
                if (index == 0) {
                    throw std::runtime_error("boom");
                }
            }
        ),
        std::runtime_error
    );
}
