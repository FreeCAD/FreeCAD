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

#include <gtest/gtest.h>

#include <gsl/util> // for at

#include <array>            // for array
#include <cstddef>          // for size_t
#include <exception>        // for terminate
#include <initializer_list> // for initializer_list
#include <vector>           // for vector
#if defined(__cplusplus) && __cplusplus >= 202002L
#include <span>
#endif // __cplusplus >= 202002L

#include "deathTestCommon.h"

TEST(at_tests, static_array)
{
    int a[4] = {1, 2, 3, 4};
    const int(&c_a)[4] = a;

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(&gsl::at(a, i) == &a[i]);
        EXPECT_TRUE(&gsl::at(c_a, i) == &a[i]);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. static_array";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(a, -1), expected);
    EXPECT_DEATH(gsl::at(a, 4), expected);
    EXPECT_DEATH(gsl::at(c_a, -1), expected);
    EXPECT_DEATH(gsl::at(c_a, 4), expected);
}

TEST(at_tests, std_array)
{
    std::array<int, 4> a = {1, 2, 3, 4};
    const std::array<int, 4>& c_a = a;

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(&gsl::at(a, i) == &a[static_cast<std::size_t>(i)]);
        EXPECT_TRUE(&gsl::at(c_a, i) == &a[static_cast<std::size_t>(i)]);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. std_array";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(a, -1), expected);
    EXPECT_DEATH(gsl::at(a, 4), expected);
    EXPECT_DEATH(gsl::at(c_a, -1), expected);
    EXPECT_DEATH(gsl::at(c_a, 4), expected);
}

TEST(at_tests, std_vector)
{
    std::vector<int> a = {1, 2, 3, 4};
    const std::vector<int>& c_a = a;

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(&gsl::at(a, i) == &a[static_cast<std::size_t>(i)]);
        EXPECT_TRUE(&gsl::at(c_a, i) == &a[static_cast<std::size_t>(i)]);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. std_vector";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(a, -1), expected);
    EXPECT_DEATH(gsl::at(a, 4), expected);
    EXPECT_DEATH(gsl::at(c_a, -1), expected);
    EXPECT_DEATH(gsl::at(c_a, 4), expected);
}

TEST(at_tests, InitializerList)
{
    const std::initializer_list<int> a = {1, 2, 3, 4};

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(gsl::at(a, i) == i + 1);
        EXPECT_TRUE(gsl::at({1, 2, 3, 4}, i) == i + 1);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. InitializerList";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(a, -1), expected);
    EXPECT_DEATH(gsl::at(a, 4), expected);
    EXPECT_DEATH(gsl::at({1, 2, 3, 4}, -1), expected);
    EXPECT_DEATH(gsl::at({1, 2, 3, 4}, 4), expected);
}

#if defined(FORCE_STD_SPAN_TESTS) || defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
TEST(at_tests, std_span)
{
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::span sp{vec};

    std::vector<int> cvec{1, 2, 3, 4, 5};
    std::span csp{cvec};

    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vec.size()); ++i)
    {
        EXPECT_TRUE(&gsl::at(sp, i) == &vec[gsl::narrow_cast<size_t>(i)]);
        EXPECT_TRUE(&gsl::at(csp, i) == &cvec[gsl::narrow_cast<size_t>(i)]);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. std_span";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(sp, -1), expected);
    EXPECT_DEATH(gsl::at(sp, gsl::narrow_cast<gsl::index>(sp.size())), expected);
    EXPECT_DEATH(gsl::at(csp, -1), expected);
    EXPECT_DEATH(gsl::at(csp, gsl::narrow_cast<gsl::index>(sp.size())), expected);
}
#endif // defined(FORCE_STD_SPAN_TESTS) || defined(__cpp_lib_span) && __cpp_lib_span >= 202002L

#if !defined(_MSC_VER) || defined(__clang__) || _MSC_VER >= 1910
static constexpr bool test_constexpr()
{
    int a1[4] = {1, 2, 3, 4};
    const int(&c_a1)[4] = a1;
    std::array<int, 4> a2 = {1, 2, 3, 4};
    const std::array<int, 4>& c_a2 = a2;

    for (int i = 0; i < 4; ++i)
    {
        if (&gsl::at(a1, i) != &a1[i]) return false;
        if (&gsl::at(c_a1, i) != &a1[i]) return false;
        // requires C++17:
        // if (&gsl::at(a2, i) != &a2[static_cast<std::size_t>(i)]) return false;
        if (&gsl::at(c_a2, i) != &c_a2[static_cast<std::size_t>(i)]) return false;
        if (gsl::at({1, 2, 3, 4}, i) != i + 1) return false;
    }

    return true;
}

static_assert(test_constexpr(), "FAIL");
#endif
