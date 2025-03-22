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

#include <gsl/pointers> // for not_null, operator<, operator<=, operator>
#include <gtest/gtest.h>

#include <type_traits> // for declval

#include "deathTestCommon.h"

using namespace gsl;

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

// stand-in for a user-defined ref-counted class
template <typename T>
struct RefCounted
{
    RefCounted(T* p) : p_(p) {}
    operator T*() { return p_; }
    T* p_;
};

namespace
{
// clang-format off
GSL_SUPPRESS(f.4) // NO-FORMAT: attribute
// clang-format on
bool helper(not_null<int*> p) { return *p == 12; }

// clang-format off
GSL_SUPPRESS(f.4) // NO-FORMAT: attribute
// clang-format on
bool helper_const(not_null<const int*> p) { return *p == 12; }

// clang-format off
GSL_SUPPRESS(f.4) // NO-FORMAT: attribute
// clang-format on
bool strict_helper(strict_not_null<int*> p) { return *p == 12; }

// clang-format off
GSL_SUPPRESS(f.4) // NO-FORMAT: attribute
// clang-format on
bool strict_helper_const(strict_not_null<const int*> p) { return *p == 12; }

int* return_pointer() { return nullptr; }
} // namespace

template <typename U, typename = void>
static constexpr bool CtorCompilesFor_A = false;
template <typename U>
static constexpr bool
    CtorCompilesFor_A<U, void_t<decltype(gsl::strict_not_null<void*>{std::declval<U>()})>> = true;

template <typename U, int N, typename = void>
static constexpr bool CtorCompilesFor_B = false;
template <typename U, int N>
static constexpr bool CtorCompilesFor_B<U, N, void_t<decltype(gsl::strict_not_null<U>{N})>> = true;

template <typename U, typename = void>
static constexpr bool DefaultCtorCompilesFor = false;
template <typename U>
static constexpr bool DefaultCtorCompilesFor<U, void_t<decltype(gsl::strict_not_null<U>{})>> = true;

template <typename U, typename = void>
static constexpr bool CtorCompilesFor_C = false;
template <typename U>
static constexpr bool CtorCompilesFor_C<
    U, void_t<decltype(gsl::strict_not_null<U*>{std::declval<std::unique_ptr<U>>()})>> = true;

TEST(strict_notnull_tests, TestStrictNotNullConstructors)
{
    {
        static_assert(CtorCompilesFor_A<void*>, "CtorCompilesFor_A<void*>");
        static_assert(!CtorCompilesFor_A<std::nullptr_t>, "!CtorCompilesFor_A<std::nullptr_t>");
        static_assert(!CtorCompilesFor_B<void*, 0>, "!CtorCompilesFor_B<void*, 0>");
        static_assert(!DefaultCtorCompilesFor<void*>, "!DefaultCtorCompilesFor<void*>");
        static_assert(!CtorCompilesFor_C<int>, "CtorCompilesFor_C<int>");
#ifdef CONFIRM_COMPILATION_ERRORS
        // Forbid non-nullptr assignable types
        strict_not_null<std::vector<int>> f(std::vector<int>{1});
        strict_not_null<int> z(10);
        strict_not_null<std::vector<int>> y({1, 2});
#endif
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. TestNotNullConstructors";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    {
        // from shared pointer
        int i = 12;
        auto rp = RefCounted<int>(&i);
        strict_not_null<int*> p(rp);
        EXPECT_TRUE(p.get() == &i);

        strict_not_null<std::shared_ptr<int>> x(
            std::make_shared<int>(10)); // shared_ptr<int> is nullptr assignable

        int* pi = nullptr;
        EXPECT_DEATH((strict_not_null<decltype(pi)>(pi)), expected);
    }

    {
        // from unique pointer
        strict_not_null<std::unique_ptr<int>> x(
            std::make_unique<int>(10)); // unique_ptr<int> is nullptr assignable

        EXPECT_DEATH((strict_not_null<std::unique_ptr<int>>(std::unique_ptr<int>{})), expected);
    }

    {
        // from pointer to local
        int t = 42;

        strict_not_null<int*> x{&t};
        helper(&t);
        helper_const(&t);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from raw pointer
        // from strict_not_null pointer

        int t = 42;
        int* p = &t;

        strict_not_null<int*> x{p};
        helper(p);
        helper_const(p);
        helper(x);
        helper_const(x);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from raw const pointer
        // from strict_not_null const pointer

        int t = 42;
        const int* cp = &t;

        strict_not_null<const int*> x{cp};
        helper_const(cp);
        helper_const(x);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from strict_not_null const pointer, using auto
        int t = 42;
        const int* cp = &t;

        auto x = strict_not_null<const int*>{cp};

        EXPECT_TRUE(*x == 42);
    }

    {
        // from returned pointer

        EXPECT_DEATH(helper(return_pointer()), expected);
        EXPECT_DEATH(helper_const(return_pointer()), expected);
    }
}

template <typename U, typename = void>
static constexpr bool StrictHelperCompilesFor = false;
template <typename U>
static constexpr bool
    StrictHelperCompilesFor<U, void_t<decltype(strict_helper(std::declval<U>()))>> = true;


template <typename U, typename = void>
static constexpr bool StrictHelperConstCompilesFor = false;
template <typename U>
static constexpr bool
    StrictHelperConstCompilesFor<U, void_t<decltype(strict_helper_const(std::declval<U>()))>> =
        true;


template <typename U, typename = void>
static constexpr bool HelperCompilesFor = false;
template <typename U>
static constexpr bool HelperCompilesFor<U, void_t<decltype(helper(std::declval<U>()))>> = true;

TEST(strict_notnull_tests, TestStrictNotNull)
{
    {
        // raw ptr <-> strict_not_null
        int x = 42;

#ifdef CONFIRM_COMPILATION_ERRORS
        strict_not_null<int*> snn = &x;
#endif
        static_assert(!StrictHelperCompilesFor<int*>, "!StrictHelperCompilesFor<int*>");
        static_assert(!StrictHelperConstCompilesFor<int*>,
                      "!StrictHelperCompilesFor<int*>");

        const strict_not_null<int*> snn1{&x};

        static_assert(StrictHelperCompilesFor<const strict_not_null<int*>>,
                          "StrictHelperCompilesFor<const strict_not_null<int*>>");
        helper(snn1);
        helper_const(snn1);

        EXPECT_TRUE(*snn1 == 42);
    }

    {
        // raw ptr <-> strict_not_null
        const int x = 42;

#ifdef CONFIRM_COMPILATION_ERRORS
        strict_not_null<int*> snn = &x;
#endif
        static_assert(!StrictHelperCompilesFor<const int*>, "!StrictHelperFor<const int*>");
        static_assert(!StrictHelperConstCompilesFor<const int*>,
                      "!StrictHelperCompilesFor<const int*>");

        const strict_not_null<const int*> snn1{&x};

        static_assert(!HelperCompilesFor<const strict_not_null<const int*>>,
                      "!HelperCompilesFor<const strict_not_null<const int*>>");
        static_assert(StrictHelperConstCompilesFor<const strict_not_null<const int*>>,
                      "StrictHelperCompilesFor<const strict_not_null<const int*>>");
        helper_const(snn1);

        EXPECT_TRUE(*snn1 == 42);
    }

    {
        // strict_not_null -> strict_not_null
        int x = 42;

        strict_not_null<int*> snn1{&x};
        const strict_not_null<int*> snn2{&x};

        strict_helper(snn1);
        strict_helper_const(snn1);
        strict_helper_const(snn2);

        EXPECT_TRUE(snn1 == snn2);
    }

    {
        // strict_not_null -> strict_not_null
        const int x = 42;

        strict_not_null<const int*> snn1{&x};
        const strict_not_null<const int*> snn2{&x};

        static_assert(!StrictHelperCompilesFor<strict_not_null<const int*>>,
                      "!StrictHelperCompilesFor<strict_not_null<const int*>>");
        strict_helper_const(snn1);
        strict_helper_const(snn2);

        EXPECT_TRUE(snn1 == snn2);
    }

    {
        // strict_not_null -> not_null
        int x = 42;

        strict_not_null<int*> snn{&x};

        const not_null<int*> nn1 = snn;
        const not_null<int*> nn2{snn};

        helper(snn);
        helper_const(snn);

        EXPECT_TRUE(snn == nn1);
        EXPECT_TRUE(snn == nn2);
    }

    {
        // strict_not_null -> not_null
        const int x = 42;

        strict_not_null<const int*> snn{&x};

        const not_null<const int*> nn1 = snn;
        const not_null<const int*> nn2{snn};

        static_assert(!HelperCompilesFor<strict_not_null<const int*>>,
                      "!HelperCompilesFor<strict_not_null<const int*>>");
        helper_const(snn);

        EXPECT_TRUE(snn == nn1);
        EXPECT_TRUE(snn == nn2);
    }

    {
        // not_null -> strict_not_null
        int x = 42;

        not_null<int*> nn{&x};

        const strict_not_null<int*> snn1{nn};
        const strict_not_null<int*> snn2{nn};

        strict_helper(nn);
        strict_helper_const(nn);

        EXPECT_TRUE(snn1 == nn);
        EXPECT_TRUE(snn2 == nn);

        std::hash<strict_not_null<int*>> hash_snn;
        std::hash<not_null<int*>> hash_nn;

        EXPECT_TRUE(hash_nn(snn1) == hash_nn(nn));
        EXPECT_TRUE(hash_snn(snn1) == hash_nn(nn));
        EXPECT_TRUE(hash_nn(snn1) == hash_nn(snn2));
        EXPECT_TRUE(hash_snn(snn1) == hash_snn(nn));
    }

    {
        // not_null -> strict_not_null
        const int x = 42;

        not_null<const int*> nn{&x};

        const strict_not_null<const int*> snn1{nn};
        const strict_not_null<const int*> snn2{nn};

        static_assert(!StrictHelperCompilesFor<not_null<const int*>>,
                      "!StrictHelperCompilesFor<not_null<const int*>>");
        strict_helper_const(nn);

        EXPECT_TRUE(snn1 == nn);
        EXPECT_TRUE(snn2 == nn);

        std::hash<strict_not_null<const int*>> hash_snn;
        std::hash<not_null<const int*>> hash_nn;

        EXPECT_TRUE(hash_nn(snn1) == hash_nn(nn));
        EXPECT_TRUE(hash_snn(snn1) == hash_nn(nn));
        EXPECT_TRUE(hash_nn(snn1) == hash_nn(snn2));
        EXPECT_TRUE(hash_snn(snn1) == hash_snn(nn));
    }
}

TEST(pointers_test, member_types)
{
    // make sure `element_type` is inherited from `gsl::not_null`
    static_assert(std::is_same<gsl::strict_not_null<int*>::element_type, int*>::value,
                  "check member type: element_type");
}

#if defined(__cplusplus) && (__cplusplus >= 201703L)

TEST(strict_notnull_tests, TestStrictNotNullConstructorTypeDeduction)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. TestStrictNotNullConstructorTypeDeduction";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    {
        int i = 42;

        strict_not_null x{&i};
        helper(strict_not_null{&i});
        helper_const(strict_not_null{&i});

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;

        strict_not_null x{&i};
        static_assert(!HelperCompilesFor<strict_not_null<const int*>>,
                      "!HelperCompilesFor<strict_not_null<const int*>>");
        helper_const(strict_not_null{&i});

        EXPECT_TRUE(*x == 42);
    }

    {
        int i = 42;
        int* p = &i;

        strict_not_null x{p};
        helper(strict_not_null{p});
        helper_const(strict_not_null{p});

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;
        const int* p = &i;

        strict_not_null x{p};
        static_assert(!HelperCompilesFor<strict_not_null<const int*>>,
                      "!HelperCompilesFor<strict_not_null<const int*>>");
        helper_const(strict_not_null{p});

        EXPECT_TRUE(*x == 42);
    }

    {
        auto workaround_macro = []() {
            int* p1 = nullptr;
            const strict_not_null x{p1};
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        auto workaround_macro = []() {
            const int* p1 = nullptr;
            const strict_not_null x{p1};
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        int* p = nullptr;

        EXPECT_DEATH(helper(strict_not_null{p}), expected);
        EXPECT_DEATH(helper_const(strict_not_null{p}), expected);
    }

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        strict_not_null x{nullptr};
        helper(strict_not_null{nullptr});
        helper_const(strict_not_null{nullptr});
    }
#endif
}
#endif // #if defined(__cplusplus) && (__cplusplus >= 201703L)
