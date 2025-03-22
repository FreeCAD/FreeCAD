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

#include <algorithm>   // for move
#include <complex>
#include <cstddef>     // for std::ptrdiff_t
#include <cstdint>     // for uint32_t, int32_t
#include <functional>  // for reference_wrapper, _Bind_helper<>::type
#include <gsl/narrow>  // for narrow, narrowing_error
#include <gsl/util>    // finally, narrow_cast
#include <limits>      // for numeric_limits
#include <type_traits> // for is_same

using namespace gsl;

namespace
{
void f(int& i) { i += 1; }
static int j = 0;
void g() { j += 1; }
} // namespace

TEST(utils_tests, sanity_check_for_gsl_index_typedef)
{
    static_assert(std::is_same<gsl::index, std::ptrdiff_t>::value,
                  "gsl::index represents wrong arithmetic type");
}

TEST(utils_tests, finally_lambda)
{
    int i = 0;
    {
        auto _ = finally([&]() { f(i); });
        EXPECT_TRUE(i == 0);
    }
    EXPECT_TRUE(i == 1);
}

TEST(utils_tests, finally_lambda_move)
{
    int i = 0;
    {
        auto _1 = finally([&]() { f(i); });
        {
            auto _2 = std::move(_1);
            EXPECT_TRUE(i == 0);
        }
        EXPECT_TRUE(i == 1);
        {
            auto _2 = std::move(_1);
            EXPECT_TRUE(i == 1);
        }
        EXPECT_TRUE(i == 1);
    }
    EXPECT_TRUE(i == 1);
}

TEST(utils_tests, finally_const_lvalue_lambda)
{
    int i = 0;
    {
        const auto const_lvalue_lambda = [&]() { f(i); };
        auto _ = finally(const_lvalue_lambda);
        EXPECT_TRUE(i == 0);
    }
    EXPECT_TRUE(i == 1);
}

TEST(utils_tests, finally_mutable_lvalue_lambda)
{
    int i = 0;
    {
        auto mutable_lvalue_lambda = [&]() { f(i); };
        auto _ = finally(mutable_lvalue_lambda);
        EXPECT_TRUE(i == 0);
    }
    EXPECT_TRUE(i == 1);
}

TEST(utils_tests, finally_function_with_bind)
{
    int i = 0;
    {
        auto _ = finally([&i] { return f(i); });
        EXPECT_TRUE(i == 0);
    }
    EXPECT_TRUE(i == 1);
}

TEST(utils_tests, finally_function_ptr)
{
    j = 0;
    {
        auto _ = finally(&g);
        EXPECT_TRUE(j == 0);
    }
    EXPECT_TRUE(j == 1);
}

TEST(utils_tests, finally_function)
{
    j = 0;
    {
        auto _ = finally(g);
        EXPECT_TRUE(j == 0);
    }
    EXPECT_TRUE(j == 1);
}

TEST(utils_tests, narrow_cast)
{
    int n = 120;
    char c = narrow_cast<char>(n);
    EXPECT_TRUE(c == 120);

    n = 300;
    unsigned char uc = narrow_cast<unsigned char>(n);
    EXPECT_TRUE(uc == 44);
}

#ifndef GSL_KERNEL_MODE
TEST(utils_tests, narrow)
{
    int n = 120;
    const char c = narrow<char>(n);
    EXPECT_TRUE(c == 120);

    n = 300;
    EXPECT_THROW(narrow<char>(n), narrowing_error);

    const auto int32_max = std::numeric_limits<int32_t>::max();
    const auto int32_min = std::numeric_limits<int32_t>::min();

    EXPECT_TRUE(narrow<uint32_t>(int32_t(0)) == 0);
    EXPECT_TRUE(narrow<uint32_t>(int32_t(1)) == 1);
    EXPECT_TRUE(narrow<uint32_t>(int32_max) == static_cast<uint32_t>(int32_max));

    EXPECT_THROW(narrow<uint32_t>(int32_t(-1)), narrowing_error);
    EXPECT_THROW(narrow<uint32_t>(int32_min), narrowing_error);

    n = -42;
    EXPECT_THROW(narrow<unsigned>(n), narrowing_error);

    EXPECT_TRUE(
        narrow<std::complex<float>>(std::complex<double>(4, 2)) == std::complex<float>(4, 2));
    EXPECT_THROW(narrow<std::complex<float>>(std::complex<double>(4.2)), narrowing_error);

    EXPECT_TRUE(narrow<int>(float(1)) == 1);
}
#endif // GSL_KERNEL_MODE
