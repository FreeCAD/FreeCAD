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

#include <gsl/span> // for span and span_ext
#include <gsl/util> // for narrow_cast, at

#include <array>     // for array
#include <exception> // for terminate
#include <iostream>  // for cerr
#include <vector>    // for vector

using namespace std;
using namespace gsl;

#include "deathTestCommon.h"

TEST(span_ext_test, make_span_from_pointer_length_constructor)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. from_pointer_length_constructor";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    int arr[4] = {1, 2, 3, 4};

    {
        auto s = make_span(&arr[0], 2);
        EXPECT_TRUE(s.size() == 2);
        EXPECT_TRUE(s.data() == &arr[0]);
        EXPECT_TRUE(s[0] == 1);
        EXPECT_TRUE(s[1] == 2);
    }

    {
        int* p = nullptr;
        auto s = make_span(p, narrow_cast<gsl::span<int>::size_type>(0));
        EXPECT_TRUE(s.size() == 0);
        EXPECT_TRUE(s.data() == nullptr);
    }

    {
        int* p = nullptr;
        auto workaround_macro = [=]() { make_span(p, 2); };
        EXPECT_DEATH(workaround_macro(), expected);
    }
}

TEST(span_ext_test, make_span_from_pointer_pointer_construction)
{
    int arr[4] = {1, 2, 3, 4};

    {
        auto s = make_span(&arr[0], &arr[2]);
        EXPECT_TRUE(s.size() == 2);
        EXPECT_TRUE(s.data() == &arr[0]);
        EXPECT_TRUE(s[0] == 1);
        EXPECT_TRUE(s[1] == 2);
    }

    {
        auto s = make_span(&arr[0], &arr[0]);
        EXPECT_TRUE(s.size() == 0);
        EXPECT_TRUE(s.data() == &arr[0]);
    }

    {
        int* p = nullptr;
        auto s = make_span(p, p);
        EXPECT_TRUE(s.size() == 0);
        EXPECT_TRUE(s.data() == nullptr);
    }
}

TEST(span_ext_test, make_span_from_array_constructor)
{
    int arr[5] = {1, 2, 3, 4, 5};
    int arr2d[2][3] = {1, 2, 3, 4, 5, 6};
    int arr3d[2][3][2] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    {
        const auto s = make_span(arr);
        EXPECT_TRUE(s.size() == 5);
        EXPECT_TRUE(s.data() == std::addressof(arr[0]));
    }

    {
        const auto s = make_span(std::addressof(arr2d[0]), 1);
        EXPECT_TRUE(s.size() == 1);
        EXPECT_TRUE(s.data() == std::addressof(arr2d[0]));
    }

    {
        const auto s = make_span(std::addressof(arr3d[0]), 1);
        EXPECT_TRUE(s.size() == 1);
        EXPECT_TRUE(s.data() == std::addressof(arr3d[0]));
    }
}

TEST(span_ext_test, make_span_from_dynamic_array_constructor)
{
    double(*arr)[3][4] = new double[100][3][4];

    {
        auto s = make_span(&arr[0][0][0], 10);
        EXPECT_TRUE(s.size() == 10);
        EXPECT_TRUE(s.data() == &arr[0][0][0]);
    }

    delete[] arr;
}

TEST(span_ext_test, make_span_from_std_array_constructor)
{
    std::array<int, 4> arr = {1, 2, 3, 4};

    {
        auto s = make_span(arr);
        EXPECT_TRUE(s.size() == arr.size());
        EXPECT_TRUE(s.data() == arr.data());
    }

    // This test checks for the bug found in gcc 6.1, 6.2, 6.3, 6.4, 6.5 7.1, 7.2, 7.3 - issue #590
    {
        gsl::span<int> s1 = make_span(arr);

        static gsl::span<int> s2;
        s2 = s1;

#if defined(__GNUC__) && __GNUC__ == 6 && (__GNUC_MINOR__ == 4 || __GNUC_MINOR__ == 5) &&          \
    __GNUC_PATCHLEVEL__ == 0 && defined(__OPTIMIZE__)
        // Known to be broken in gcc 6.4 and 6.5 with optimizations
        // Issue in gcc: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83116
        EXPECT_TRUE(s1.size() == 4);
        EXPECT_TRUE(s2.size() == 0);
#else
        EXPECT_TRUE(s1.size() == s2.size());
#endif
    }
}

TEST(span_ext_test, make_span_from_const_std_array_constructor)
{
    const std::array<int, 4> arr = {1, 2, 3, 4};

    {
        auto s = make_span(arr);
        EXPECT_TRUE(s.size() == arr.size());
        EXPECT_TRUE(s.data() == arr.data());
    }
}

TEST(span_ext_test, make_span_from_std_array_const_constructor)
{
    std::array<const int, 4> arr = {1, 2, 3, 4};

    {
        auto s = make_span(arr);
        EXPECT_TRUE(s.size() == arr.size());
        EXPECT_TRUE(s.data() == arr.data());
    }
}

TEST(span_ext_test, make_span_from_container_constructor)
{
    std::vector<int> v = {1, 2, 3};
    const std::vector<int> cv = v;

    {
        auto s = make_span(v);
        EXPECT_TRUE(s.size() == v.size());
        EXPECT_TRUE(s.data() == v.data());

        auto cs = make_span(cv);
        EXPECT_TRUE(cs.size() == cv.size());
        EXPECT_TRUE(cs.data() == cv.data());
    }
}

TEST(span_test, interop_with_gsl_at)
{
    std::vector<int> vec{1, 2, 3, 4, 5};
    gsl::span<int> sp{vec};

    std::vector<int> cvec{1, 2, 3, 4, 5};
    gsl::span<int> csp{cvec};

    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vec.size()); ++i)
    {
        EXPECT_TRUE(&gsl::at(sp, i) == &vec[gsl::narrow_cast<size_t>(i)]);
        EXPECT_TRUE(&gsl::at(csp, i) == &cvec[gsl::narrow_cast<size_t>(i)]);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. interop_with_gsl_at";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    EXPECT_DEATH(gsl::at(sp, -1), expected);
    EXPECT_DEATH(gsl::at(sp, gsl::narrow_cast<gsl::index>(sp.size())), expected);
    EXPECT_DEATH(gsl::at(csp, -1), expected);
    EXPECT_DEATH(gsl::at(csp, gsl::narrow_cast<gsl::index>(sp.size())), expected);
}

TEST(span_ext_test, iterator_free_functions)
{
    int a[] = {1, 2, 3, 4};
    gsl::span<int> s{a};

    EXPECT_TRUE((std::is_same<decltype(s.begin()), decltype(begin(s))>::value));
    EXPECT_TRUE((std::is_same<decltype(s.end()), decltype(end(s))>::value));

    EXPECT_TRUE((std::is_same<decltype(std::cbegin(s)), decltype(cbegin(s))>::value));
    EXPECT_TRUE((std::is_same<decltype(std::cend(s)), decltype(cend(s))>::value));

    EXPECT_TRUE((std::is_same<decltype(s.rbegin()), decltype(rbegin(s))>::value));
    EXPECT_TRUE((std::is_same<decltype(s.rend()), decltype(rend(s))>::value));

    EXPECT_TRUE((std::is_same<decltype(std::crbegin(s)), decltype(crbegin(s))>::value));
    EXPECT_TRUE((std::is_same<decltype(std::crend(s)), decltype(crend(s))>::value));

    EXPECT_TRUE(s.begin() == begin(s));
    EXPECT_TRUE(s.end() == end(s));

    EXPECT_TRUE(s.rbegin() == rbegin(s));
    EXPECT_TRUE(s.rend() == rend(s));

    EXPECT_TRUE(s.begin() == cbegin(s));
    EXPECT_TRUE(s.end() == cend(s));

    EXPECT_TRUE(s.rbegin() == crbegin(s));
    EXPECT_TRUE(s.rend() == crend(s));
}

TEST(span_ext_test, ssize_free_function)
{
    int a[] = {1, 2, 3, 4};
    gsl::span<int> s{a};

    EXPECT_FALSE((std::is_same<decltype(s.size()), decltype(ssize(s))>::value));
    EXPECT_TRUE(s.size() == static_cast<std::size_t>(ssize(s)));
}

#ifndef GSL_KERNEL_MODE
TEST(span_ext_test, comparison_operators)
{
    {
        gsl::span<int> s1;
        gsl::span<int> s2;
        EXPECT_TRUE(s1 == s2);
        EXPECT_FALSE(s1 != s2);
        EXPECT_FALSE(s1 < s2);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s1 >= s2);
        EXPECT_TRUE(s2 == s1);
        EXPECT_FALSE(s2 != s1);
        EXPECT_FALSE(s2 != s1);
        EXPECT_TRUE(s2 <= s1);
        EXPECT_FALSE(s2 > s1);
        EXPECT_TRUE(s2 >= s1);
    }

    {
        int arr[] = {2, 1};
        gsl::span<int> s1 = arr;
        gsl::span<int> s2 = arr;

        EXPECT_TRUE(s1 == s2);
        EXPECT_FALSE(s1 != s2);
        EXPECT_FALSE(s1 < s2);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s1 >= s2);
        EXPECT_TRUE(s2 == s1);
        EXPECT_FALSE(s2 != s1);
        EXPECT_FALSE(s2 < s1);
        EXPECT_TRUE(s2 <= s1);
        EXPECT_FALSE(s2 > s1);
        EXPECT_TRUE(s2 >= s1);
    }

    {
        int arr[] = {2, 1}; // bigger

        gsl::span<int> s1;
        gsl::span<int> s2 = arr;

        EXPECT_TRUE(s1 != s2);
        EXPECT_TRUE(s2 != s1);
        EXPECT_FALSE(s1 == s2);
        EXPECT_FALSE(s2 == s1);
        EXPECT_TRUE(s1 < s2);
        EXPECT_FALSE(s2 < s1);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s2 <= s1);
        EXPECT_TRUE(s2 > s1);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s2 >= s1);
        EXPECT_FALSE(s1 >= s2);
    }

    {
        int arr1[] = {1, 2};
        int arr2[] = {1, 2};
        gsl::span<int> s1 = arr1;
        gsl::span<int> s2 = arr2;

        EXPECT_TRUE(s1 == s2);
        EXPECT_FALSE(s1 != s2);
        EXPECT_FALSE(s1 < s2);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s1 >= s2);
        EXPECT_TRUE(s2 == s1);
        EXPECT_FALSE(s2 != s1);
        EXPECT_FALSE(s2 < s1);
        EXPECT_TRUE(s2 <= s1);
        EXPECT_FALSE(s2 > s1);
        EXPECT_TRUE(s2 >= s1);
    }

    {
        int arr[] = {1, 2, 3};

        gsl::span<int> s1 = {&arr[0], 2}; // shorter
        gsl::span<int> s2 = arr;          // longer

        EXPECT_TRUE(s1 != s2);
        EXPECT_TRUE(s2 != s1);
        EXPECT_FALSE(s1 == s2);
        EXPECT_FALSE(s2 == s1);
        EXPECT_TRUE(s1 < s2);
        EXPECT_FALSE(s2 < s1);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s2 <= s1);
        EXPECT_TRUE(s2 > s1);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s2 >= s1);
        EXPECT_FALSE(s1 >= s2);
    }

    {
        int arr1[] = {1, 2}; // smaller
        int arr2[] = {2, 1}; // bigger

        gsl::span<int> s1 = arr1;
        gsl::span<int> s2 = arr2;

        EXPECT_TRUE(s1 != s2);
        EXPECT_TRUE(s2 != s1);
        EXPECT_FALSE(s1 == s2);
        EXPECT_FALSE(s2 == s1);
        EXPECT_TRUE(s1 < s2);
        EXPECT_FALSE(s2 < s1);
        EXPECT_TRUE(s1 <= s2);
        EXPECT_FALSE(s2 <= s1);
        EXPECT_TRUE(s2 > s1);
        EXPECT_FALSE(s1 > s2);
        EXPECT_TRUE(s2 >= s1);
        EXPECT_FALSE(s1 >= s2);
    }
}
#endif // GSL_KERNEL_MODE
