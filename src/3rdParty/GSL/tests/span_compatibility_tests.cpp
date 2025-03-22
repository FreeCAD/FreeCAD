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

#include <gsl/byte> // for byte
#include <gsl/span> // for span, span_iterator, operator==, operator!=

#include <array>       // for array
#include <cstddef>     // for ptrdiff_t
#include <iterator>    // for reverse_iterator, operator-, operator==
#include <type_traits> // for integral_constant<>::value, is_default_co...
#include <utility>
#include <vector> // for vector

using namespace std;
using namespace gsl;

// Below are tests that verify the gsl interface support the same things as the std
// Ranges and Concepts support need to be added later.

struct Base
{
};
struct Derived : Base
{
};
static_assert(std::is_convertible<Derived*, Base*>::value, "std::is_convertible<Derived*, Base*>");
static_assert(!std::is_convertible<Derived (*)[], Base (*)[]>::value,
              "!std::is_convertible<Derived(*)[], Base(*)[]>");

// int*(*) [], int const* const(*)[] was identified as an issue in CWG330 and the resolution was
// provided with N4261.
template <class T = int>
void ArrayConvertibilityCheck()
{
#if __cplusplus >= 201703l
    if constexpr (std::is_convertible<T*(*) [], T const* const(*)[]>::value)
    {
        std::array<T*, 3> stl_nullptr{{nullptr, nullptr, nullptr}};
        gsl::span<const T* const> sp_const_nullptr_1{stl_nullptr};
        EXPECT_TRUE(sp_const_nullptr_1.data() == stl_nullptr.data());
        EXPECT_TRUE(sp_const_nullptr_1.size() == 3);

        gsl::span<const T* const> sp_const_nullptr_2{std::as_const(stl_nullptr)};
        EXPECT_TRUE(sp_const_nullptr_2.data() == stl_nullptr.data());
        EXPECT_TRUE(sp_const_nullptr_2.size() == 3);

        static_assert(std::is_same<decltype(gsl::span{stl_nullptr}), gsl::span<T*, 3>>::value,
                      "std::is_same< decltype(span{stl_nullptr}), span<T*, 3>>::value");
        static_assert(
            std::is_same<decltype(gsl::span{std::as_const(stl_nullptr)}), gsl::span<T* const, 3>>::value,
            "std::is_same< decltype(span{std::as_const(stl_nullptr)}), span<T* const, "
            "3>>::value");
    }
#endif
}

TEST(span_compatibility_tests, assertion_tests)
{
    int arr[3]{10, 20, 30};
    std::array<int, 3> stl{{100, 200, 300}};

    ArrayConvertibilityCheck();

    {
        gsl::span<int> sp_dyn;
        EXPECT_TRUE(sp_dyn.data() == nullptr);
        EXPECT_TRUE(sp_dyn.size() == 0);
        EXPECT_TRUE(sp_dyn.empty());
    }
    {
        gsl::span<int, 0> sp_zero;
        EXPECT_TRUE(sp_zero.data() == nullptr);
        EXPECT_TRUE(sp_zero.size() == 0);
        EXPECT_TRUE(sp_zero.empty());

        gsl::span<int> sp_dyn_a(arr, 3);
        gsl::span<int> sp_dyn_b(begin(arr), 3);
        EXPECT_TRUE(sp_dyn_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_dyn_b.data() == std::begin(arr));
        EXPECT_TRUE(sp_dyn_a.size() == 3);
        EXPECT_TRUE(sp_dyn_b.size() == 3);

        gsl::span<int, 3> sp_three_a(arr, 3);
        gsl::span<int, 3> sp_three_b(begin(arr), 3);
        EXPECT_TRUE(sp_three_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_three_b.data() == std::begin(arr));
        EXPECT_TRUE(sp_three_a.size() == 3);
        EXPECT_TRUE(sp_three_b.size() == 3);

        gsl::span<const int> sp_const_a(arr, 3);
        gsl::span<const int> sp_const_b(begin(arr), 3);
        EXPECT_TRUE(sp_const_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_b.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_a.size() == 3);
        EXPECT_TRUE(sp_const_b.size() == 3);

#if __cplusplus >= 201703l
        gsl::span<const int> sp_const_c(std::as_const(arr), 3);
        EXPECT_TRUE(sp_const_c.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_c.size() == 3);
#endif // __cplusplus >= 201703l

        gsl::span<const int> sp_const_d(cbegin(arr), 3);
        EXPECT_TRUE(sp_const_d.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_d.size() == 3);
    }
    {
        gsl::span<int> sp_dyn_a(begin(arr), std::end(arr));
        EXPECT_TRUE(sp_dyn_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_dyn_a.size() == 3);

        gsl::span<int, 3> sp_three_a(begin(arr), std::end(arr));
        EXPECT_TRUE(sp_three_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_three_a.size() == 3);

        gsl::span<const int> sp_const_a(begin(arr), std::end(arr));
        gsl::span<const int> sp_const_b(begin(arr), std::cend(arr));
        gsl::span<const int> sp_const_c(cbegin(arr), std::end(arr));
        gsl::span<const int> sp_const_d(cbegin(arr), std::cend(arr));
        EXPECT_TRUE(sp_const_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_b.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_c.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_d.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_a.size() == 3);
        EXPECT_TRUE(sp_const_b.size() == 3);
        EXPECT_TRUE(sp_const_c.size() == 3);
        EXPECT_TRUE(sp_const_d.size() == 3);
    }
    {
        gsl::span<int> sp_dyn_a(arr);
        gsl::span<int> sp_dyn_b(stl);
        gsl::span<int> sp_dyn_c{stl};
        gsl::span<const int> sp_dyn_d{stl};
        EXPECT_TRUE(sp_dyn_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_dyn_b.data() == stl.data());
        EXPECT_TRUE(sp_dyn_a.size() == 3);
        EXPECT_TRUE(sp_dyn_b.size() == 3);

        gsl::span<int, 3> sp_three_a(arr);
        gsl::span<int, 3> sp_three_b(stl);
        EXPECT_TRUE(sp_three_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_three_b.data() == stl.data());
        EXPECT_TRUE(sp_three_a.size() == 3);
        EXPECT_TRUE(sp_three_b.size() == 3);

        gsl::span<const int> sp_const_w(arr);
        gsl::span<const int> sp_const_y(stl);
        EXPECT_TRUE(sp_const_w.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_y.data() == stl.data());
        EXPECT_TRUE(sp_const_w.size() == 3);
        EXPECT_TRUE(sp_const_y.size() == 3);

#if __cplusplus >= 201703l
        gsl::span<const int> sp_const_x(std::as_const(arr));
        EXPECT_TRUE(sp_const_x.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_x.size() == 3);

        gsl::span<const int> sp_const_z(std::as_const(stl));
        EXPECT_TRUE(sp_const_z.data() == stl.data());
        EXPECT_TRUE(sp_const_z.size() == 3);
#endif // __cplusplus >= 201703l
    }
    {
        const gsl::span<int> orig_dyn(arr);
        const gsl::span<int, 3> orig_three(arr);
        const gsl::span<const int> orig_const_dyn(arr);
        const gsl::span<const int, 3> orig_const_three(arr);

        gsl::span<int> sp_a(orig_dyn);
        gsl::span<int> sp_b(orig_three);

        gsl::span<int, 3> sp_c(orig_three);

        gsl::span<const int> sp_d(orig_dyn);
        gsl::span<const int> sp_e(orig_three);
        gsl::span<const int> sp_f(orig_const_dyn);
        gsl::span<const int> sp_g(orig_const_three);

        gsl::span<const int, 3> sp_h(orig_three);
        gsl::span<const int, 3> sp_i(orig_const_three);

        EXPECT_TRUE(sp_a.data() == std::begin(arr));
        EXPECT_TRUE(sp_b.data() == std::begin(arr));
        EXPECT_TRUE(sp_c.data() == std::begin(arr));
        EXPECT_TRUE(sp_d.data() == std::begin(arr));
        EXPECT_TRUE(sp_e.data() == std::begin(arr));
        EXPECT_TRUE(sp_f.data() == std::begin(arr));
        EXPECT_TRUE(sp_g.data() == std::begin(arr));
        EXPECT_TRUE(sp_h.data() == std::begin(arr));
        EXPECT_TRUE(sp_i.data() == std::begin(arr));
        EXPECT_TRUE(sp_a.size() == 3);
        EXPECT_TRUE(sp_b.size() == 3);
        EXPECT_TRUE(sp_c.size() == 3);
        EXPECT_TRUE(sp_d.size() == 3);
        EXPECT_TRUE(sp_e.size() == 3);
        EXPECT_TRUE(sp_f.size() == 3);
        EXPECT_TRUE(sp_g.size() == 3);
        EXPECT_TRUE(sp_h.size() == 3);
        EXPECT_TRUE(sp_i.size() == 3);
    }
    {
        gsl::span<int> sp_dyn(arr);
        gsl::span<int, 3> sp_three(arr);
        gsl::span<const int> sp_const_dyn(arr);
        gsl::span<const int, 3> sp_const_three(arr);

        EXPECT_TRUE(sp_dyn.data() == std::begin(arr));
        EXPECT_TRUE(sp_three.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_dyn.data() == std::begin(arr));
        EXPECT_TRUE(sp_const_three.data() == std::begin(arr));
        EXPECT_TRUE(sp_dyn.size() == 3);
        EXPECT_TRUE(sp_three.size() == 3);
        EXPECT_TRUE(sp_const_dyn.size() == 3);
        EXPECT_TRUE(sp_const_three.size() == 3);

        int other[4]{12, 34, 56, 78};

        sp_dyn = gsl::span<int>{other};
        sp_three = gsl::span<int, 3>{stl};
        sp_const_dyn = gsl::span<const int>{other};
        sp_const_three = gsl::span<const int, 3>{stl};

        EXPECT_TRUE(sp_dyn.data() == std::begin(other));
        EXPECT_TRUE(sp_three.data() == stl.data());
        EXPECT_TRUE(sp_const_dyn.data() == std::begin(other));
        EXPECT_TRUE(sp_const_three.data() == stl.data());
        EXPECT_TRUE(sp_dyn.size() == 4);
        EXPECT_TRUE(sp_three.size() == 3);
        EXPECT_TRUE(sp_const_dyn.size() == 4);
        EXPECT_TRUE(sp_const_three.size() == 3);
    }
    {
        gsl::span<int>::iterator it_dyn{};

        {
            gsl::span<int> sp_dyn(arr);
            it_dyn = sp_dyn.begin();
        }

        EXPECT_TRUE(*it_dyn == arr[0]);
        EXPECT_TRUE(it_dyn[2] == arr[2]);

        gsl::span<int, 3>::iterator it_three{};

        {
            gsl::span<int, 3> sp_three(stl);
            it_three = sp_three.begin();
        }

        EXPECT_TRUE(*it_three == stl[0]);
        EXPECT_TRUE(it_three[2] == stl[2]);
    }

    {
        int sequence[9]{10, 20, 30, 40, 50, 60, 70, 80, 90};

        const gsl::span<int> sp_dyn(sequence);
        const gsl::span<int, 9> sp_nine(sequence);

        auto first_3 = sp_dyn.first<3>();
        auto first_4 = sp_nine.first<4>();
        auto first_5 = sp_dyn.first(5);
        auto first_6 = sp_nine.first(6);
        static_assert(noexcept(sp_dyn.first<3>()), "noexcept(sp_dyn.first<3>())");   // strengthened
        static_assert(noexcept(sp_nine.first<4>()), "noexcept(sp_nine.first<4>())"); // strengthened
        static_assert(noexcept(sp_dyn.first(5)), "noexcept(sp_dyn.first(5))");       // strengthened
        static_assert(noexcept(sp_nine.first(6)), "noexcept(sp_nine.first(6))");     // strengthened
        static_assert(is_same<decltype(first_3), gsl::span<int, 3>>::value,
                      "is_same<decltype(first_3), gsl::span<int, 3>>::value");
        static_assert(is_same<decltype(first_4), gsl::span<int, 4>>::value,
                      "is_same<decltype(first_4), gsl::span<int, 4>>::value");
        static_assert(is_same<decltype(first_5), gsl::span<int>>::value,
                      "is_same<decltype(first_5), gsl::span<int>>::value");
        static_assert(is_same<decltype(first_6), gsl::span<int>>::value,
                      "is_same<decltype(first_6), gsl::span<int>>::value");
        EXPECT_TRUE(first_3.data() == std::begin(sequence));
        EXPECT_TRUE(first_4.data() == std::begin(sequence));
        EXPECT_TRUE(first_5.data() == std::begin(sequence));
        EXPECT_TRUE(first_6.data() == std::begin(sequence));
        EXPECT_TRUE(first_3.size() == 3);
        EXPECT_TRUE(first_4.size() == 4);
        EXPECT_TRUE(first_5.size() == 5);
        EXPECT_TRUE(first_6.size() == 6);

        auto last_3 = sp_dyn.last<3>();
        auto last_4 = sp_nine.last<4>();
        auto last_5 = sp_dyn.last(5);
        auto last_6 = sp_nine.last(6);
        static_assert(noexcept(sp_dyn.last<3>()), "noexcept(sp_dyn.last<3>())");   // strengthened
        static_assert(noexcept(sp_nine.last<4>()), "noexcept(sp_nine.last<4>())"); // strengthened
        static_assert(noexcept(sp_dyn.last(5)), "noexcept(sp_dyn.last(5))");       // strengthened
        static_assert(noexcept(sp_nine.last(6)), "noexcept(sp_nine.last(6))");     // strengthened
        static_assert(is_same<decltype(last_3), gsl::span<int, 3>>::value,
                      "is_same<decltype(last_3), gsl::span<int, 3>>::value");
        static_assert(is_same<decltype(last_4), gsl::span<int, 4>>::value,
                      "is_same<decltype(last_4), gsl::span<int, 4>>::value");
        static_assert(is_same<decltype(last_5), gsl::span<int>>::value,
                      "is_same<decltype(last_5), gsl::span<int>>::value");
        static_assert(is_same<decltype(last_6), gsl::span<int>>::value,
                      "is_same<decltype(last_6), gsl::span<int>>::value");
        EXPECT_TRUE(last_3.data() == std::begin(sequence) + 6);
        EXPECT_TRUE(last_4.data() == std::begin(sequence) + 5);
        EXPECT_TRUE(last_5.data() == std::begin(sequence) + 4);
        EXPECT_TRUE(last_6.data() == std::begin(sequence) + 3);
        EXPECT_TRUE(last_3.size() == 3);
        EXPECT_TRUE(last_4.size() == 4);
        EXPECT_TRUE(last_5.size() == 5);
        EXPECT_TRUE(last_6.size() == 6);

        auto offset_3 = sp_dyn.subspan<3>();
        auto offset_4 = sp_nine.subspan<4>();
        auto offset_5 = sp_dyn.subspan(5);
        auto offset_6 = sp_nine.subspan(6);
        static_assert(noexcept(sp_dyn.subspan<3>()),
                      "noexcept(sp_dyn.subspan<3>())"); // strengthened
        static_assert(noexcept(sp_nine.subspan<4>()),
                      "noexcept(sp_nine.subspan<4>())");                             // strengthened
        static_assert(noexcept(sp_dyn.subspan(5)), "noexcept(sp_dyn.subspan(5))");   // strengthened
        static_assert(noexcept(sp_nine.subspan(6)), "noexcept(sp_nine.subspan(6))"); // strengthened
        static_assert(is_same<decltype(offset_3), gsl::span<int>>::value,
                      "is_same<decltype(offset_3), gsl::span<int>>::value");
        static_assert(is_same<decltype(offset_4), gsl::span<int, 5>>::value,
                      "is_same<decltype(offset_4), gsl::span<int, 5>>::value");
        static_assert(is_same<decltype(offset_5), gsl::span<int>>::value,
                      "is_same<decltype(offset_5), gsl::span<int>>::value");
        static_assert(is_same<decltype(offset_6), gsl::span<int>>::value,
                      "is_same<decltype(offset_6), gsl::span<int>>::value");
        EXPECT_TRUE(offset_3.data() == std::begin(sequence) + 3);
        EXPECT_TRUE(offset_4.data() == std::begin(sequence) + 4);
        EXPECT_TRUE(offset_5.data() == std::begin(sequence) + 5);
        EXPECT_TRUE(offset_6.data() == std::begin(sequence) + 6);
        EXPECT_TRUE(offset_3.size() == 6);
        EXPECT_TRUE(offset_4.size() == 5);
        EXPECT_TRUE(offset_5.size() == 4);
        EXPECT_TRUE(offset_6.size() == 3);

        auto subspan_3 = sp_dyn.subspan<3, 2>();
        auto subspan_4 = sp_nine.subspan<4, 2>();
        auto subspan_5 = sp_dyn.subspan(5, 2);
        auto subspan_6 = sp_nine.subspan(6, 2);
        static_assert(noexcept(sp_dyn.subspan<3, 2>()),
                      "noexcept(sp_dyn.subspan<3, 2>())"); // strengthened
        static_assert(noexcept(sp_nine.subspan<4, 2>()),
                      "noexcept(sp_nine.subspan<4, 2>())"); // strengthened
        static_assert(noexcept(sp_dyn.subspan(5, 2)),
                      "noexcept(sp_dyn.subspan(5, 2))"); // strengthened
        static_assert(noexcept(sp_nine.subspan(6, 2)),
                      "noexcept(sp_nine.subspan(6, 2))"); // strengthened
        static_assert(is_same<decltype(subspan_3), gsl::span<int, 2>>::value,
                      "is_same<decltype(subspan_3), gsl::span<int, 2>>::value");
        static_assert(is_same<decltype(subspan_4), gsl::span<int, 2>>::value,
                      "is_same<decltype(subspan_4), gsl::span<int, 2>>::value");
        static_assert(is_same<decltype(subspan_5), gsl::span<int>>::value,
                      "is_same<decltype(subspan_5), gsl::span<int>>::value");
        static_assert(is_same<decltype(subspan_6), gsl::span<int>>::value,
                      "is_same<decltype(subspan_6), gsl::span<int>>::value");
        EXPECT_TRUE(subspan_3.data() == std::begin(sequence) + 3);
        EXPECT_TRUE(subspan_4.data() == std::begin(sequence) + 4);
        EXPECT_TRUE(subspan_5.data() == std::begin(sequence) + 5);
        EXPECT_TRUE(subspan_6.data() == std::begin(sequence) + 6);
        EXPECT_TRUE(subspan_3.size() == 2);
        EXPECT_TRUE(subspan_4.size() == 2);
        EXPECT_TRUE(subspan_5.size() == 2);
        EXPECT_TRUE(subspan_6.size() == 2);

        static_assert(noexcept(sp_dyn.size()), "noexcept(sp_dyn.size())");
        static_assert(noexcept(sp_dyn.size_bytes()), "noexcept(sp_dyn.size_bytes())");
        static_assert(noexcept(sp_dyn.empty()), "noexcept(sp_dyn.empty())");
        static_assert(noexcept(sp_dyn[0]), "noexcept(sp_dyn[0])");           // strengthened
        static_assert(noexcept(sp_dyn.front()), "noexcept(sp_dyn.front())"); // strengthened
        static_assert(noexcept(sp_dyn.back()), "noexcept(sp_dyn.back())");   // strengthened
        static_assert(noexcept(sp_dyn.data()), "noexcept(sp_dyn.data())");
        static_assert(noexcept(sp_dyn.begin()), "noexcept(sp_dyn.begin())");
        static_assert(noexcept(sp_dyn.end()), "noexcept(sp_dyn.end())");
        static_assert(noexcept(sp_dyn.rbegin()), "noexcept(sp_dyn.rbegin())");
        static_assert(noexcept(sp_dyn.rend()), "noexcept(sp_dyn.rend())");

        static_assert(noexcept(sp_nine.size()), "noexcept(sp_nine.size())");
        static_assert(noexcept(sp_nine.size_bytes()), "noexcept(sp_nine.size_bytes())");
        static_assert(noexcept(sp_nine.empty()), "noexcept(sp_nine.empty())");
        static_assert(noexcept(sp_nine[0]), "noexcept(sp_nine[0])");           // strengthened
        static_assert(noexcept(sp_nine.front()), "noexcept(sp_nine.front())"); // strengthened
        static_assert(noexcept(sp_nine.back()), "noexcept(sp_nine.back())");   // strengthened
        static_assert(noexcept(sp_nine.data()), "noexcept(sp_nine.data())");
        static_assert(noexcept(sp_nine.begin()), "noexcept(sp_nine.begin())");
        static_assert(noexcept(sp_nine.end()), "noexcept(sp_nine.end())");
        static_assert(noexcept(sp_nine.rbegin()), "noexcept(sp_nine.rbegin())");
        static_assert(noexcept(sp_nine.rend()), "noexcept(sp_nine.rend())");

        EXPECT_TRUE(sp_dyn.size() == 9);
        EXPECT_TRUE(sp_nine.size() == 9);

        EXPECT_TRUE(sp_dyn.size_bytes() == 9 * sizeof(int));
        EXPECT_TRUE(sp_nine.size_bytes() == 9 * sizeof(int));

        EXPECT_TRUE(!sp_dyn.empty());
        EXPECT_TRUE(!sp_nine.empty());

        EXPECT_TRUE(sp_dyn[0] == 10);
        EXPECT_TRUE(sp_nine[0] == 10);
        EXPECT_TRUE(sp_dyn[8] == 90);
        EXPECT_TRUE(sp_nine[8] == 90);

        EXPECT_TRUE(sp_dyn.front() == 10);
        EXPECT_TRUE(sp_nine.front() == 10);

        EXPECT_TRUE(sp_dyn.back() == 90);
        EXPECT_TRUE(sp_nine.back() == 90);

        EXPECT_TRUE(&sp_dyn.front() == std::begin(sequence));
        EXPECT_TRUE(&sp_nine.front() == std::begin(sequence));
        EXPECT_TRUE(&sp_dyn[4] == std::begin(sequence) + 4);
        EXPECT_TRUE(&sp_nine[4] == std::begin(sequence) + 4);
        EXPECT_TRUE(&sp_dyn.back() == std::begin(sequence) + 8);
        EXPECT_TRUE(&sp_nine.back() == std::begin(sequence) + 8);

        EXPECT_TRUE(sp_dyn.data() == std::begin(sequence));
        EXPECT_TRUE(sp_nine.data() == std::begin(sequence));

        EXPECT_TRUE(*sp_dyn.begin() == 10);
        EXPECT_TRUE(*sp_nine.begin() == 10);

        EXPECT_TRUE(sp_dyn.end()[-2] == 80);
        EXPECT_TRUE(sp_nine.end()[-2] == 80);

        EXPECT_TRUE(*sp_dyn.rbegin() == 90);
        EXPECT_TRUE(*sp_nine.rbegin() == 90);

        EXPECT_TRUE(sp_dyn.rend()[-2] == 20);
        EXPECT_TRUE(sp_nine.rend()[-2] == 20);

        static_assert(is_same<decltype(sp_dyn.begin()), gsl::span<int>::iterator>::value,
                      "is_same<decltype(sp_dyn.begin()), gsl::span<int>::iterator>::value");
        static_assert(is_same<decltype(sp_nine.begin()), gsl::span<int, 9>::iterator>::value,
                      "is_same<decltype(sp_nine.begin()), gsl::span<int, 9>::iterator>::value");
        static_assert(is_same<decltype(sp_dyn.end()), gsl::span<int>::iterator>::value,
                      "is_same<decltype(sp_dyn.end()), gsl::span<int>::iterator>::value");
        static_assert(is_same<decltype(sp_nine.end()), gsl::span<int, 9>::iterator>::value,
                      "is_same<decltype(sp_nine.end()), gsl::span<int, 9>::iterator>::value");
        static_assert(
            is_same<decltype(sp_dyn.rbegin()), gsl::span<int>::reverse_iterator>::value,
            "is_same<decltype(sp_dyn.rbegin()), gsl::span<int>::reverse_iterator>::value");
        static_assert(
            is_same<decltype(sp_nine.rbegin()), gsl::span<int, 9>::reverse_iterator>::value,
            "is_same<decltype(sp_nine.rbegin()), gsl::span<int, 9>::reverse_iterator>::value");
        static_assert(is_same<decltype(sp_dyn.rend()), gsl::span<int>::reverse_iterator>::value,
                      "is_same<decltype(sp_dyn.rend()), gsl::span<int>::reverse_iterator>::value");
        static_assert(
            is_same<decltype(sp_nine.rend()), gsl::span<int, 9>::reverse_iterator>::value,
            "is_same<decltype(sp_nine.rend()), gsl::span<int, 9>::reverse_iterator>::value");
    }
    {
        int sequence[9]{10, 20, 30, 40, 50, 60, 70, 80, 90};

        constexpr size_t SizeBytes = sizeof(sequence);

        const gsl::span<int> sp_dyn(sequence);
        const gsl::span<int, 9> sp_nine(sequence);
        const gsl::span<const int> sp_const_dyn(sequence);
        const gsl::span<const int, 9> sp_const_nine(sequence);

        static_assert(noexcept(as_bytes(sp_dyn)), "noexcept(as_bytes(sp_dyn))");
        static_assert(noexcept(as_bytes(sp_nine)), "noexcept(as_bytes(sp_nine))");
        static_assert(noexcept(as_bytes(sp_const_dyn)), "noexcept(as_bytes(sp_const_dyn))");
        static_assert(noexcept(as_bytes(sp_const_nine)), "noexcept(as_bytes(sp_const_nine))");
        static_assert(noexcept(as_writable_bytes(sp_dyn)), "noexcept(as_writable_bytes(sp_dyn))");
        static_assert(noexcept(as_writable_bytes(sp_nine)), "noexcept(as_writable_bytes(sp_nine))");

        auto sp_1 = as_bytes(sp_dyn);
        auto sp_2 = as_bytes(sp_nine);
        auto sp_3 = as_bytes(sp_const_dyn);
        auto sp_4 = as_bytes(sp_const_nine);
        auto sp_5 = as_writable_bytes(sp_dyn);
        auto sp_6 = as_writable_bytes(sp_nine);

        static_assert(is_same<decltype(sp_1), gsl::span<const byte>>::value,
                      "is_same<decltype(sp_1), gsl::span<const byte>>::value");
        static_assert(is_same<decltype(sp_2), gsl::span<const byte, SizeBytes>>::value,
                      "is_same<decltype(sp_2), gsl::span<const byte, SizeBytes>>::value");
        static_assert(is_same<decltype(sp_3), gsl::span<const byte>>::value,
                      "is_same<decltype(sp_3), gsl::span<const byte>>::value");
        static_assert(is_same<decltype(sp_4), gsl::span<const byte, SizeBytes>>::value,
                      "is_same<decltype(sp_4), gsl::span<const byte, SizeBytes>>::value");
        static_assert(is_same<decltype(sp_5), gsl::span<byte>>::value,
                      "is_same<decltype(sp_5), gsl::span<byte>>::value");
        static_assert(is_same<decltype(sp_6), gsl::span<byte, SizeBytes>>::value,
                      "is_same<decltype(sp_6), gsl::span<byte, SizeBytes>>::value");

        EXPECT_TRUE(sp_1.data() == reinterpret_cast<const byte*>(begin(sequence)));
        EXPECT_TRUE(sp_2.data() == reinterpret_cast<const byte*>(begin(sequence)));
        EXPECT_TRUE(sp_3.data() == reinterpret_cast<const byte*>(begin(sequence)));
        EXPECT_TRUE(sp_4.data() == reinterpret_cast<const byte*>(begin(sequence)));
        EXPECT_TRUE(sp_5.data() == reinterpret_cast<byte*>(begin(sequence)));
        EXPECT_TRUE(sp_6.data() == reinterpret_cast<byte*>(begin(sequence)));

        EXPECT_TRUE(sp_1.size() == SizeBytes);
        EXPECT_TRUE(sp_2.size() == SizeBytes);
        EXPECT_TRUE(sp_3.size() == SizeBytes);
        EXPECT_TRUE(sp_4.size() == SizeBytes);
        EXPECT_TRUE(sp_5.size() == SizeBytes);
        EXPECT_TRUE(sp_6.size() == SizeBytes);
    }
}

// assertions for span's definition
static_assert(std::is_same<decltype(gsl::dynamic_extent), const std::size_t>::value,
              "gsl::dynamic_extent must be represented as std::size_t");
static_assert(gsl::dynamic_extent == static_cast<std::size_t>(-1),
              "gsl::dynamic_extent must be defined as the max value of std::size_t");

static_assert(std::is_same<decltype(gsl::span<int>::extent), const std::size_t>::value,
              "Ensure that the type of  gsl::span::extent is std::size_t");
static_assert(gsl::span<int>::extent == gsl::dynamic_extent,
              "gsl::span<int>::extent should be equivalent to gsl::dynamic_extent");

static_assert(std::is_same<decltype(gsl::span<int, 3>::extent), const std::size_t>::value,
              "Ensure that the type of gsl::span::extent is std::size_t");
static_assert(gsl::span<int, 3>::extent == 3, "Ensure that span<int, 3>::extent is equal to 3");

static_assert(std::is_same<gsl::span<int>::element_type, int>::value,
              "span<int>::element_type should be int");
static_assert(std::is_same<gsl::span<int>::value_type, int>::value,
              "span<int>::value_type should be int");
static_assert(std::is_same<gsl::span<int>::size_type, std::size_t>::value,
              "span<int>::size_type should be std::size_t");
static_assert(std::is_same<gsl::span<int>::difference_type, ptrdiff_t>::value,
              "span<int>::difference_type should be std::ptrdiff_t");
static_assert(std::is_same<gsl::span<int>::pointer, int*>::value,
              "span<int>::pointer should be int*");
static_assert(std::is_same<gsl::span<int>::const_pointer, const int*>::value,
              "span<int>::const_pointer should be const int*");
static_assert(std::is_same<gsl::span<int>::reference, int&>::value,
              "span<int>::reference should be int&");
static_assert(std::is_same<gsl::span<int>::const_reference, const int&>::value,
              "span<int>::const_reference should be const int&");

static_assert(std::is_same<gsl::span<int, 3>::element_type, int>::value,
              "span<int, 3>::element_type should be int");
static_assert(std::is_same<gsl::span<int, 3>::value_type, int>::value,
              "span<int, 3>::value_type should be int");
static_assert(std::is_same<gsl::span<int, 3>::size_type, std::size_t>::value,
              "span<int, 3>::size_type should be std::size_t");
static_assert(std::is_same<gsl::span<int, 3>::difference_type, ptrdiff_t>::value,
              "span<int, 3>::difference_type should be std::ptrdiff_t");
static_assert(std::is_same<gsl::span<int, 3>::pointer, int*>::value,
              "span<int, 3>::pointer should be int*");
static_assert(std::is_same<gsl::span<int, 3>::const_pointer, const int*>::value,
              "span<int, 3>::const_pointer should be const int*");
static_assert(std::is_same<gsl::span<int, 3>::reference, int&>::value,
              "span<int, 3>::reference should be int&");
static_assert(std::is_same<gsl::span<int, 3>::const_reference, const int&>::value,
              "span<int, 3>::const_reference should be const int&");

static_assert(std::is_same<gsl::span<const int>::element_type, const int>::value,
              "span<const int>::element_type should be const int");
static_assert(std::is_same<gsl::span<const int>::value_type, int>::value,
              "span<const int>::value_type should be int");
static_assert(std::is_same<gsl::span<const int>::size_type, std::size_t>::value,
              "span<const int>::size_type should be size_t");
static_assert(std::is_same<gsl::span<const int>::difference_type, ptrdiff_t>::value,
              "span<const int>::difference_type should be ptrdiff_t");
static_assert(std::is_same<gsl::span<const int>::pointer, const int*>::value,
              "span<const int>::pointer should be const int*");
static_assert(std::is_same<gsl::span<const int>::const_pointer, const int*>::value,
              "span<const int>::const_pointer should be const int*");
static_assert(std::is_same<gsl::span<const int>::reference, const int&>::value,
              "span<const int>::reference should be const int&");
static_assert(std::is_same<gsl::span<const int>::const_reference, const int&>::value,
              "span<const int>::const_reference should be const int&");

static_assert(std::is_same<gsl::span<const int, 3>::element_type, const int>::value,
              "span<const int, 3>::element_type should be const int");
static_assert(std::is_same<gsl::span<const int, 3>::value_type, int>::value,
              "span<const int, 3>::value_type should be int");
static_assert(std::is_same<gsl::span<const int, 3>::size_type, std::size_t>::value,
              "span<const int, 3>::size_type should be size_t");
static_assert(std::is_same<gsl::span<const int, 3>::difference_type, ptrdiff_t>::value,
              "span<const int, 3>::difference_type should be ptrdiff_t");
static_assert(std::is_same<gsl::span<const int, 3>::pointer, const int*>::value,
              "span<const int, 3>::pointer should be const int*");
static_assert(std::is_same<gsl::span<const int, 3>::const_pointer, const int*>::value,
              "span<const int, 3>::const_pointer should be const int*");
static_assert(std::is_same<gsl::span<const int, 3>::reference, const int&>::value,
              "span<const int, 3>::reference should be const int&");
static_assert(std::is_same<gsl::span<const int, 3>::const_reference, const int&>::value,
              "span<const int, 3>::const_reference should be const int&");

// assertions for span_iterator
static_assert(std::is_same<std::iterator_traits<gsl::span<int>::iterator>::pointer, int*>::value,
              "span<int>::iterator's pointer should be int*");
static_assert(
    std::is_same<gsl::span<int>::reverse_iterator,
                 std::reverse_iterator<gsl::span<int>::iterator>>::value,
    "span<int>::reverse_iterator should equal std::reverse_iterator<span<int>::iterator>");

static_assert(std::is_same<std::iterator_traits<gsl::span<int, 3>::iterator>::pointer, int*>::value,
              "span<int, 3>::iterator's pointer should be int*");
static_assert(
    std::is_same<gsl::span<int, 3>::reverse_iterator,
                 std::reverse_iterator<gsl::span<int, 3>::iterator>>::value,
    "span<int, 3>::reverse_iterator should equal std::reverse_iterator<span<int, 3>::iterator>");

static_assert(
    std::is_same<std::iterator_traits<gsl::span<const int>::iterator>::pointer, const int*>::value,
    "span<const int>::iterator's pointer should be int*");
static_assert(std::is_same<gsl::span<const int>::reverse_iterator,
                           std::reverse_iterator<gsl::span<const int>::iterator>>::value,
              "span<const int>::reverse_iterator should equal std::reverse_iterator<span<const "
              "int>::iterator>");

static_assert(std::is_same<std::iterator_traits<gsl::span<const int, 3>::iterator>::pointer,
                           const int*>::value,
              "span<const int, 3>::iterator's pointer should be int*");
static_assert(std::is_same<gsl::span<const int, 3>::reverse_iterator,
                           std::reverse_iterator<gsl::span<const int, 3>::iterator>>::value,
              "span<const int, 3>::reverse_iterator should equal std::reverse_iterator<span<const "
              "int, 3>::iterator>");

// copyability assertions
static_assert(std::is_trivially_copyable<gsl::span<int>>::value,
              "span<int> should be trivially copyable");
static_assert(std::is_trivially_copyable<gsl::span<int>::iterator>::value,
              "span<int>::iterator should be trivially copyable");

static_assert(std::is_trivially_copyable<gsl::span<int, 3>>::value,
              "span<int, 3> should be trivially copyable");
static_assert(std::is_trivially_copyable<gsl::span<int, 3>::iterator>::value,
              "span<int, 3>::iterator should be trivially copyable");

static_assert(std::is_trivially_copyable<gsl::span<const int>>::value,
              "span<const int> should be trivially copyable");
static_assert(std::is_trivially_copyable<gsl::span<const int>::iterator>::value,
              "span<const int>::iterator should be trivially copyable");

static_assert(std::is_trivially_copyable<gsl::span<const int, 3>>::value,
              "span<const int, 3> should be trivially copyable");
static_assert(std::is_trivially_copyable<gsl::span<const int, 3>::iterator>::value,
              "span<const int, 3>::iterator should be trivially copyable");

// nothrow constructible assertions
static_assert(std::is_nothrow_constructible<gsl::span<int>, int*, std::size_t>::value,
              "std::is_nothrow_constructible<gsl::span<int>, int*, std::size_t>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, int*, std::uint16_t>::value,
              "std::is_nothrow_constructible<gsl::span<int>, int*, std::uint16_t>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, int*, int*>::value,
              "std::is_nothrow_constructible<gsl::span<int>, int*, int*>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, int (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<int>, int(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int>&>::value,
              "std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int>&>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int, 500>&>::value,
              "std::is_nothrow_constructible<gsl::span<int>, const gsl::span<int, 500>&>");
static_assert(std::is_nothrow_constructible<gsl::span<int>, std::array<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<int>, std::array<int, 3>&>");

static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, int*, std::size_t>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, int*, std::size_t>");
static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, int*, std::uint16_t>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, int*, std::uint16_t>");
static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, int*, int*>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, int*, int*>");
static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, int (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, int(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, const gsl::span<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, const gsl::span<int, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<int, 3>, std::array<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<int, 3>, std::array<int, 3>&>");

static_assert(std::is_nothrow_constructible<gsl::span<const int>, int*, std::size_t>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, int*, std::size_t>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, int*, int*>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, int*, int*>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, int*, const int*>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, int*, const int*>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, int (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, int(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const int*, int*>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const int*, int*>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const int*, const int*>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const int*, const int*>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const int*, std::size_t>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const int*, std::size_t>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const int (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const int(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int>&>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int>&>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int, 500>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<int, 500>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int, 500>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int>, const gsl::span<const int, 500>&>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, std::array<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, std::array<int, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<const int>, const std::array<int, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<const int>, const std::array<int, 3>&>");

static_assert(
    std::is_nothrow_constructible<gsl::span<const int, 3>, const gsl::span<int, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int, 3>, const gsl::span<int, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const int, 3>, const gsl::span<const int, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const int, 3>, const gsl::span<const int, 3>&>");

static_assert(std::is_nothrow_constructible<gsl::span<Base>, Base (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<Base>, Base(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<Base>, const gsl::span<Base>&>::value,
              "std::is_nothrow_constructible<gsl::span<Base>, const gsl::span<Base>&>");
static_assert(std::is_nothrow_constructible<gsl::span<Base>, const gsl::span<Base, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<Base>, const gsl::span<Base, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<Base>, std::array<Base, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<Base>, std::array<Base, 3>&>");

static_assert(std::is_nothrow_constructible<gsl::span<Base, 3>, Base (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<Base, 3>, Base(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<Base, 3>, const gsl::span<Base, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<Base, 3>, const gsl::span<Base, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<Base, 3>, std::array<Base, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<Base, 3>, std::array<Base, 3>&>");

static_assert(std::is_nothrow_constructible<gsl::span<const Base>, Base (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<const Base>, Base(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<const Base>, const Base (&)[3]>::value,
              "std::is_nothrow_constructible<gsl::span<const Base>, const Base(&)[3]>");
static_assert(std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<Base>&>::value,
              "std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<Base>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<Base, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<Base, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<const Base>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<const Base>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<const Base, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base>, const gsl::span<const Base, 3>&>");
static_assert(std::is_nothrow_constructible<gsl::span<const Base>, std::array<Base, 3>&>::value,
              "std::is_nothrow_constructible<gsl::span<const Base>, std::array<Base, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const Base>, const std::array<Base, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base>, const std::array<Base, 3>&>");

static_assert(
    std::is_nothrow_constructible<gsl::span<const Base, 3>, const gsl::span<Base, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base, 3>, const gsl::span<Base, 3>&>");
static_assert(
    std::is_nothrow_constructible<gsl::span<const Base, 3>, const gsl::span<const Base, 3>&>::value,
    "std::is_nothrow_constructible<gsl::span<const Base, 3>, const gsl::span<const Base, 3>&>");

// non-constructible assertions
static_assert(!std::is_constructible<gsl::span<int>, const int*, int*>::value,
              "!std::is_constructible<gsl::span<int>, const int*, int*>");
static_assert(!std::is_constructible<gsl::span<int>, const int*, const int*>::value,
              "!std::is_constructible<gsl::span<int>, const int*, const int*>");
static_assert(!std::is_constructible<gsl::span<int>, const int*, double*>::value,
              "!std::is_constructible<gsl::span<int>, const int*, double*>");
static_assert(!std::is_constructible<gsl::span<int>, const int*, std::size_t>::value,
              "!std::is_constructible<gsl::span<int>, const int*, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int>, const int (&)[3]>::value,
              "!std::is_constructible<gsl::span<int>, const int(&)[3]>");
static_assert(!std::is_constructible<gsl::span<int>, double*, int*>::value,
              "!std::is_constructible<gsl::span<int>, double*, int*>");
static_assert(!std::is_constructible<gsl::span<int>, double*, const int*>::value,
              "!std::is_constructible<gsl::span<int>, double*, const int*>");
static_assert(!std::is_constructible<gsl::span<int>, double*, double*>::value,
              "!std::is_constructible<gsl::span<int>, double*, double*>");
static_assert(!std::is_constructible<gsl::span<int>, double*, std::size_t>::value,
              "!std::is_constructible<gsl::span<int>, double*, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int>, double (&)[3]>::value,
              "!std::is_constructible<gsl::span<int>, double(&)[3]>");
static_assert(!std::is_constructible<gsl::span<int>, int*, double*>::value,
              "!std::is_constructible<gsl::span<int>, int*, double*>");
static_assert(!std::is_constructible<gsl::span<int>, std::size_t, int*>::value,
              "!std::is_constructible<gsl::span<int>, std::size_t, int*>");
static_assert(!std::is_constructible<gsl::span<int>, std::size_t, std::size_t>::value,
              "!std::is_constructible<gsl::span<int>, std::size_t, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int>, const gsl::span<const int>&>::value,
              "!std::is_constructible<gsl::span<int>, const gsl::span<const int>&>");
static_assert(!std::is_constructible<gsl::span<int>, const gsl::span<const int, 3>&>::value,
              "!std::is_constructible<gsl::span<int>, const gsl::span<const int, 3>&>");
static_assert(!std::is_constructible<gsl::span<int>, const gsl::span<const int, 500>&>::value,
              "!std::is_constructible<gsl::span<int>, const gsl::span<const int, 500>&>");
static_assert(!std::is_constructible<gsl::span<int>, const gsl::span<double, 3>&>::value,
              "!std::is_constructible<gsl::span<int>, const gsl::span<double, 3>&>");
static_assert(!std::is_constructible<gsl::span<int>, std::array<double, 3>&>::value,
              "!std::is_constructible<gsl::span<int>, std::array<double, 3>&>");
static_assert(!std::is_constructible<gsl::span<int>, const std::array<int, 3>&>::value,
              "!std::is_constructible<gsl::span<int>, const std::array<int, 3>&>");

static_assert(!std::is_constructible<gsl::span<int, 3>, int*, double*>::value,
              "!std::is_constructible<gsl::span<int, 3>, int*, double*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, int (&)[500]>::value,
              "!std::is_constructible<gsl::span<int, 3>, int(&)[500]>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const int*, int*>::value,
              "!std::is_constructible<gsl::span<int, 3>, const int*, int*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const int*, const int*>::value,
              "!std::is_constructible<gsl::span<int, 3>, const int*, const int*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const int*, std::size_t>::value,
              "!std::is_constructible<gsl::span<int, 3>, const int*, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const int*, double*>::value,
              "!std::is_constructible<gsl::span<int, 3>, const int*, double*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const int (&)[3]>::value,
              "!std::is_constructible<gsl::span<int, 3>, const int(&)[3]>");
static_assert(!std::is_constructible<gsl::span<int, 3>, double*, std::size_t>::value,
              "!std::is_constructible<gsl::span<int, 3>, double*, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int, 3>, double*, int*>::value,
              "!std::is_constructible<gsl::span<int, 3>, double*, int*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, double*, const int*>::value,
              "!std::is_constructible<gsl::span<int, 3>, double*, const int*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, double*, double*>::value,
              "!std::is_constructible<gsl::span<int, 3>, double*, double*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, double (&)[3]>::value,
              "!std::is_constructible<gsl::span<int, 3>, double(&)[3]>");

static_assert(!std::is_constructible<gsl::span<int, 3>, std::size_t, int*>::value,
              "!std::is_constructible<gsl::span<int, 3>, std::size_t, int*>");
static_assert(!std::is_constructible<gsl::span<int, 3>, std::size_t, std::size_t>::value,
              "!std::is_constructible<gsl::span<int, 3>, std::size_t, std::size_t>");
static_assert(!std::is_constructible<gsl::span<int, 3>, std::array<double, 3>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, std::array<double, 3>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, std::array<int, 500>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, std::array<int, 500>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const std::array<int, 3>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const std::array<int, 3>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const gsl::span<int, 500>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const gsl::span<int, 500>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int, 3>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int, 3>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int, 500>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const gsl::span<const int, 500>&>");
static_assert(!std::is_constructible<gsl::span<int, 3>, const gsl::span<double, 3>&>::value,
              "!std::is_constructible<gsl::span<int, 3>, const gsl::span<double, 3>&>");

static_assert(!std::is_constructible<gsl::span<const int>, double (&)[3]>::value,
              "!std::is_constructible<gsl::span<const int>, double(&)[3]>");
static_assert(!std::is_constructible<gsl::span<const int>, std::array<double, 3>&>::value,
              "!std::is_constructible<gsl::span<const int>, std::array<double, 3>&>");
static_assert(!std::is_constructible<gsl::span<const int>, const gsl::span<double, 3>&>::value,
              "!std::is_constructible<gsl::span<const int>, const gsl::span<double, 3>&>");

static_assert(!std::is_constructible<gsl::span<const int, 3>, const gsl::span<int, 500>&>::value,
              "!std::is_constructible<gsl::span<const int, 3>, const gsl::span<int, 500>&>");
static_assert(
    !std::is_constructible<gsl::span<const int, 3>, const gsl::span<const int, 500>&>::value,
    "!std::is_constructible<gsl::span<const int, 3>, const gsl::span<const int, 500>&>");
static_assert(!std::is_constructible<gsl::span<const int, 3>, const gsl::span<double, 3>&>::value,
              "!std::is_constructible<gsl::span<const int, 3>, const gsl::span<double, 3>&>");

static_assert(!std::is_constructible<gsl::span<Base>, Derived (&)[3]>::value,
              "!std::is_constructible<gsl::span<Base>, Derived(&)[3]>");
static_assert(!std::is_constructible<gsl::span<Base>, std::array<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<Base>, std::array<Derived, 3>&>");
static_assert(!std::is_constructible<gsl::span<Base>, std::vector<Derived>&>::value,
              "!std::is_constructible<gsl::span<Base>, std::vector<Derived>&>");
static_assert(!std::is_constructible<gsl::span<Base>, const gsl::span<Derived>&>::value,
              "!std::is_constructible<gsl::span<Base>, const gsl::span<Derived>&>");
static_assert(!std::is_constructible<gsl::span<Base>, const gsl::span<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<Base>, const gsl::span<Derived, 3>&>");

static_assert(!std::is_constructible<gsl::span<Base, 3>, const gsl::span<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<Base, 3>, const gsl::span<Derived, 3>&>");
static_assert(!std::is_constructible<gsl::span<Base, 3>, Derived (&)[3]>::value,
              "!std::is_constructible<gsl::span<Base, 3>, Derived(&)[3]>");
static_assert(!std::is_constructible<gsl::span<Base, 3>, std::array<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<Base, 3>, std::array<Derived, 3>&>");

static_assert(!std::is_constructible<gsl::span<const Base>, Derived (&)[3]>::value,
              "!std::is_constructible<gsl::span<const Base>, Derived(&)[3]>");
static_assert(!std::is_constructible<gsl::span<const Base>, const Derived (&)[3]>::value,
              "!std::is_constructible<gsl::span<const Base>, const Derived(&)[3]>");
static_assert(!std::is_constructible<gsl::span<const Base>, std::array<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<const Base>, std::array<Derived, 3>&>");
static_assert(!std::is_constructible<gsl::span<const Base>, const std::array<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<const Base>, const std::array<Derived, 3>&>");
static_assert(!std::is_constructible<gsl::span<const Base>, const gsl::span<Derived>&>::value,
              "!std::is_constructible<gsl::span<const Base>, const gsl::span<Derived>&>");
static_assert(!std::is_constructible<gsl::span<const Base>, const gsl::span<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<const Base>, const gsl::span<Derived, 3>&>");
static_assert(!std::is_constructible<gsl::span<const Base>, const gsl::span<const Derived>&>::value,
              "!std::is_constructible<gsl::span<const Base>, const gsl::span<const Derived>&>");
static_assert(
    !std::is_constructible<gsl::span<const Base>, const gsl::span<const Derived, 3>&>::value,
    "!std::is_constructible<gsl::span<const Base>, const gsl::span<const Derived, 3>&>");

static_assert(!std::is_constructible<gsl::span<const Base, 3>, const gsl::span<Derived, 3>&>::value,
              "!std::is_constructible<gsl::span<const Base, 3>, const gsl::span<Derived, 3>&>");
static_assert(
    !std::is_constructible<gsl::span<const Base, 3>, const gsl::span<const Derived, 3>&>::value,
    "!std::is_constructible<gsl::span<const Base, 3>, const gsl::span<const Derived, 3>&>");

static_assert(!std::is_constructible<gsl::span<const Derived>, std::array<Base, 3>&>::value,
              "!std::is_constructible<gsl::span<const Derived>, std::array<Base, 3>&>");
static_assert(!std::is_constructible<gsl::span<const Derived>, const std::array<Base, 3>&>::value,
              "!std::is_constructible<gsl::span<const Derived>, const std::array<Base, 3>&>");

// Explicit construction enabled in P1976R2
static_assert(std::is_constructible<gsl::span<int, 3>, const gsl::span<int>&>::value,
              "std::is_constructible<gsl::span<int, 3>, const gsl::span<int>&>");
static_assert(std::is_constructible<gsl::span<const int, 3>, const gsl::span<int>&>::value,
              "std::is_constructible<gsl::span<const int, 3>, const gsl::span<int>&>");
static_assert(std::is_constructible<gsl::span<const int, 3>, const gsl::span<const int>&>::value,
              "std::is_constructible<gsl::span<const int, 3>, const gsl::span<const int>&>");

// no throw copy constructor
static_assert(std::is_nothrow_copy_constructible<gsl::span<int>>::value,
              "std::is_nothrow_copy_constructible<gsl::span<int>>");
static_assert(std::is_nothrow_copy_constructible<gsl::span<int, 3>>::value,
              "std::is_nothrow_copy_constructible<gsl::span<int, 3>>");
static_assert(std::is_nothrow_copy_constructible<gsl::span<const int>>::value,
              "std::is_nothrow_copy_constructible<gsl::span<const int>>");
static_assert(std::is_nothrow_copy_constructible<gsl::span<const int, 3>>::value,
              "std::is_nothrow_copy_constructible<gsl::span<const int, 3>>");

// no throw copy assignment
static_assert(std::is_nothrow_copy_assignable<gsl::span<int>>::value,
              "std::is_nothrow_copy_assignable<gsl::span<int>>");
static_assert(std::is_nothrow_copy_assignable<gsl::span<int, 3>>::value,
              "std::is_nothrow_copy_assignable<gsl::span<int, 3>>");
static_assert(std::is_nothrow_copy_assignable<gsl::span<const int>>::value,
              "std::is_nothrow_copy_assignable<gsl::span<const int>>");
static_assert(std::is_nothrow_copy_assignable<gsl::span<const int, 3>>::value,
              "std::is_nothrow_copy_assignable<gsl::span<const int, 3>>");

// no throw destruction
static_assert(std::is_nothrow_destructible<gsl::span<int>>::value,
              "std::is_nothrow_destructible<gsl::span<int>>");
static_assert(std::is_nothrow_destructible<gsl::span<int, 3>>::value,
              "std::is_nothrow_destructible<gsl::span<int, 3>>");
static_assert(std::is_nothrow_destructible<gsl::span<const int>>::value,
              "std::is_nothrow_destructible<gsl::span<const int>>");

// conversions
static_assert(std::is_convertible<int (&)[3], gsl::span<int>>::value,
              "std::is_convertible<int(&)[3], gsl::span<int>>");
static_assert(std::is_convertible<int (&)[3], gsl::span<int, 3>>::value,
              "std::is_convertible<int(&)[3], gsl::span<int, 3>>");
static_assert(std::is_convertible<int (&)[3], gsl::span<const int>>::value,
              "std::is_convertible<int(&)[3], gsl::span<const int>>");

static_assert(std::is_convertible<const int (&)[3], gsl::span<const int>>::value,
              "std::is_convertible<const int(&)[3], gsl::span<const int>>");

static_assert(std::is_convertible<const gsl::span<int>&, gsl::span<int>>::value,
              "std::is_convertible<const gsl::span<int>&, gsl::span<int>>");
static_assert(std::is_convertible<const gsl::span<int>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<int>&, gsl::span<const int>>");

static_assert(std::is_convertible<const gsl::span<int, 3>&, gsl::span<int>>::value,
              "std::is_convertible<const gsl::span<int, 3>&, gsl::span<int>>");
static_assert(std::is_convertible<const gsl::span<int, 3>&, gsl::span<int, 3>>::value,
              "std::is_convertible<const gsl::span<int, 3>&, gsl::span<int, 3>>");
static_assert(std::is_convertible<const gsl::span<int, 3>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<int, 3>&, gsl::span<const int>>");
static_assert(std::is_convertible<const gsl::span<int, 3>&, gsl::span<const int, 3>>::value,
              "std::is_convertible<const gsl::span<int, 3>&, gsl::span<const int, 3>>");
static_assert(std::is_convertible<const gsl::span<int, 500>&, gsl::span<int>>::value,
              "std::is_convertible<const gsl::span<int, 500>&, gsl::span<int>>");
static_assert(std::is_convertible<const gsl::span<int, 500>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<int, 500>&, gsl::span<const int>>");

static_assert(std::is_convertible<const gsl::span<const int>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<const int>&, gsl::span<const int>>");

static_assert(std::is_convertible<const gsl::span<const int, 3>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<const int, 3>&, gsl::span<const int>>");
static_assert(std::is_convertible<const gsl::span<const int, 3>&, gsl::span<const int, 3>>::value,
              "std::is_convertible<const gsl::span<const int, 3>&, gsl::span<const int, 3>>");
static_assert(std::is_convertible<const gsl::span<const int, 500>&, gsl::span<const int>>::value,
              "std::is_convertible<const gsl::span<const int, 500>&, gsl::span<const int>>");

static_assert(std::is_convertible<std::array<int, 3>&, gsl::span<int>>::value,
              "std::is_convertible<std::array<int, 3>&, gsl::span<int>>");
static_assert(std::is_convertible<std::array<int, 3>&, gsl::span<int, 3>>::value,
              "std::is_convertible<std::array<int, 3>&, gsl::span<int, 3>>");
static_assert(std::is_convertible<std::array<int, 3>&, gsl::span<const int>>::value,
              "std::is_convertible<std::array<int, 3>&, gsl::span<const int>>");

static_assert(std::is_convertible<const std::array<int, 3>&, gsl::span<const int>>::value,
              "std::is_convertible<const std::array<int, 3>&, gsl::span<const int>>");

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

template <typename U, typename = void>
static constexpr bool AsWritableBytesCompilesFor = false;

template <typename U>
static constexpr bool
    AsWritableBytesCompilesFor<U, ::void_t<decltype(as_writable_bytes(declval<U>()))>> = true;

static_assert(AsWritableBytesCompilesFor<gsl::span<int>>,
              "AsWritableBytesCompilesFor<gsl::span<int>>");
static_assert(AsWritableBytesCompilesFor<gsl::span<int, 9>>,
              "AsWritableBytesCompilesFor<gsl::span<int, 9>>");
static_assert(!AsWritableBytesCompilesFor<gsl::span<const int>>,
              "!AsWritableBytesCompilesFor<gsl::span<const int>>");
static_assert(!AsWritableBytesCompilesFor<gsl::span<const int, 9>>,
              "!AsWritableBytesCompilesFor<gsl::span<const int, 9>>");
