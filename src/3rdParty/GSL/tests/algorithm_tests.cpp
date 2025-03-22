///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include <array>         // for array
#include <cstddef>       // for size_t
#include <gsl/algorithm> // for copy
#include <gsl/span>      // for span
#include <gtest/gtest.h>

#include "deathTestCommon.h"

namespace gsl
{
struct fail_fast;
} // namespace gsl

using namespace gsl;

TEST(algorithm_tests, same_type)
{
    // dynamic source and destination span
    {
        std::array<int, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<int> src_span(src);
        const span<int> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // static source and dynamic destination span
    {
        std::array<int, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<int, 5> src_span(src);
        const span<int> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // dynamic source and static destination span
    {
        std::array<int, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const gsl::span<int> src_span(src);
        const gsl::span<int, 10> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // static source and destination span
    {
        std::array<int, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<int, 5> src_span(src);
        const span<int, 10> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }
}

TEST(algorithm_tests, compatible_type)
{
    // dynamic source and destination span
    {
        std::array<short, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<short> src_span(src);
        const span<int> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // static source and dynamic destination span
    {
        std::array<short, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<short, 5> src_span(src);
        const span<int> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // dynamic source and static destination span
    {
        std::array<short, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<short> src_span(src);
        const span<int, 10> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }

    // static source and destination span
    {
        std::array<short, 5> src{1, 2, 3, 4, 5};
        std::array<int, 10> dst{};

        const span<short, 5> src_span(src);
        const span<int, 10> dst_span(dst);

        copy(src_span, dst_span);
        copy(src_span, dst_span.subspan(src_span.size()));

        for (std::size_t i = 0; i < src.size(); ++i)
        {
            EXPECT_TRUE(dst[i] == src[i]);
            EXPECT_TRUE(dst[i + src.size()] == src[i]);
        }
    }
}

#ifdef CONFIRM_COMPILATION_ERRORS
TEST(algorithm_tests, incompatible_type)
{
    std::array<int, 4> src{1, 2, 3, 4};
    std::array<int*, 12> dst{};

    span<int> src_span_dyn(src);
    span<int, 4> src_span_static(src);
    span<int*> dst_span_dyn(dst);
    span<int*, 4> dst_span_static(gsl::make_span(dst));

    // every line should produce a compilation error
    copy(src_span_dyn, dst_span_dyn);
    copy(src_span_dyn, dst_span_static);
    copy(src_span_static, dst_span_dyn);
    copy(src_span_static, dst_span_static);
}
#endif

TEST(algorithm_tests, small_destination_span)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. small_destination_span";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    std::array<int, 12> src{1, 2, 3, 4};
    std::array<int, 4> dst{};

    const span<int> src_span_dyn(src);
    const span<int, 12> src_span_static(src);
    const span<int> dst_span_dyn(dst);
    const span<int, 4> dst_span_static(dst);

    EXPECT_DEATH(copy(src_span_dyn, dst_span_dyn), expected);
    EXPECT_DEATH(copy(src_span_dyn, dst_span_static), expected);
    EXPECT_DEATH(copy(src_span_static, dst_span_dyn), expected);

#ifdef CONFIRM_COMPILATION_ERRORS
    copy(src_span_static, dst_span_static);
#endif
}
