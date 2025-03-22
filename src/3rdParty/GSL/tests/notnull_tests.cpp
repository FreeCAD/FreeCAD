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

#include <gsl/pointers> // for not_null, operator<, operator<=, operator>

#include <algorithm>   // for addressof
#include <cstdint>     // for uint16_t
#include <memory>      // for shared_ptr, make_shared, operator<, opera...
#include <sstream>     // for operator<<, ostringstream, basic_ostream:...
#include <string>      // for basic_string, operator==, string, operator<<
#include <type_traits> // for declval
#include <typeinfo>    // for type_info
#include <variant>     // for variant, monostate, get

#include "deathTestCommon.h"
using namespace gsl;

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

struct MyBase
{
};
struct MyDerived : public MyBase
{
};
struct Unrelated
{
};

// stand-in for a user-defined ref-counted class
template <typename T>
struct RefCounted
{
    RefCounted(T* p) : p_(p) {}
    operator T*() { return p_; }
    T* p_;
};

// user defined smart pointer with comparison operators returning non bool value
template <typename T>
struct CustomPtr
{
    CustomPtr(T* p) : p_(p) {}
    operator T*() const { return p_; }
    bool operator!=(std::nullptr_t) const { return p_ != nullptr; }
    T* p_ = nullptr;
};

template <typename T, typename U>
std::string operator==(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) == reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                          : "false";
}

template <typename T, typename U>
std::string operator!=(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) != reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                          : "false";
}

template <typename T, typename U>
std::string operator<(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) < reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                         : "false";
}

template <typename T, typename U>
std::string operator>(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) > reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                         : "false";
}

template <typename T, typename U>
std::string operator<=(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) <= reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                          : "false";
}

template <typename T, typename U>
std::string operator>=(CustomPtr<T> const& lhs, CustomPtr<U> const& rhs)
{
    // clang-format off
    GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    return reinterpret_cast<const void*>(lhs.p_) >= reinterpret_cast<const void*>(rhs.p_) ? "true"
                                                                                          : "false";
}

struct NonCopyableNonMovable
{
    NonCopyableNonMovable() = default;
    NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
    NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
};

namespace
{
// clang-format off
GSL_SUPPRESS(f .4) // NO-FORMAT: attribute
// clang-format on
bool helper(not_null<int*> p) { return *p == 12; }
// clang-format off
GSL_SUPPRESS(f .4) // NO-FORMAT: attribute
// clang-format on
bool helper_const(not_null<const int*> p) { return *p == 12; }

int* return_pointer() { return nullptr; }
} // namespace

template <typename U, typename = void>
static constexpr bool CtorCompilesFor_A = false;
template <typename U>
static constexpr bool
    CtorCompilesFor_A<U, void_t<decltype(gsl::not_null<void*>{std::declval<U>()})>> = true;

template <typename U, int N, typename = void>
static constexpr bool CtorCompilesFor_B = false;
template <typename U, int N>
static constexpr bool CtorCompilesFor_B<U, N, void_t<decltype(gsl::not_null<U>{N})>> = true;

template <typename U, typename = void>
static constexpr bool DefaultCtorCompilesFor = false;
template <typename U>
static constexpr bool DefaultCtorCompilesFor<U, void_t<decltype(gsl::not_null<U>{})>> = true;

template <typename U, typename = void>
static constexpr bool CtorCompilesFor_C = false;
template <typename U>
static constexpr bool
    CtorCompilesFor_C<U, void_t<decltype(gsl::not_null<U*>{std::declval<std::unique_ptr<U>>()})>> =
        true;

TEST(notnull_tests, TestNotNullConstructors)
{
    {
        static_assert(CtorCompilesFor_A<void*>, "CtorCompilesFor_A<void*>");
        static_assert(!CtorCompilesFor_A<std::nullptr_t>, "!CtorCompilesFor_A<std::nullptr_t>");
        static_assert(!CtorCompilesFor_B<void*, 0>, "!CtorCompilesFor_B<void*, 0>");
        static_assert(!DefaultCtorCompilesFor<void*>, "!DefaultCtorCompilesFor<void*>");
        static_assert(!CtorCompilesFor_C<int>, "CtorCompilesFor_C<int>");

#ifdef CONFIRM_COMPILATION_ERRORS
        // Forbid non-nullptr assignable types
        not_null<std::vector<int>> f(std::vector<int>{1});
        not_null<int> z(10);
        not_null<std::vector<int>> y({1, 2});
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
        not_null<int*> p(rp);
        EXPECT_TRUE(p.get() == &i);

        not_null<std::shared_ptr<int>> x(
            std::make_shared<int>(10)); // shared_ptr<int> is nullptr assignable

        int* pi = nullptr;
        EXPECT_DEATH((not_null<decltype(pi)>(pi)), expected);
    }

    {
        // from unique pointer
        not_null<std::unique_ptr<int>> x(
            std::make_unique<int>(10)); // unique_ptr<int> is nullptr assignable

        EXPECT_DEATH((not_null<std::unique_ptr<int>>(std::unique_ptr<int>{})), expected);
    }

    {
        // from pointer to local
        int t = 42;

        not_null<int*> x = &t;
        helper(&t);
        helper_const(&t);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from raw pointer
        // from not_null pointer

        int t = 42;
        int* p = &t;

        not_null<int*> x = p;
        helper(p);
        helper_const(p);
        helper(x);
        helper_const(x);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from raw const pointer
        // from not_null const pointer

        int t = 42;
        const int* cp = &t;

        not_null<const int*> x = cp;
        helper_const(cp);
        helper_const(x);

        EXPECT_TRUE(*x == 42);
    }

    {
        // from not_null const pointer, using auto
        int t = 42;
        const int* cp = &t;

        auto x = not_null<const int*>{cp};

        EXPECT_TRUE(*x == 42);
    }

    {
        // from returned pointer

        EXPECT_DEATH(helper(return_pointer()), expected);
        EXPECT_DEATH(helper_const(return_pointer()), expected);
    }
}

template <typename T>
void ostream_helper(T v)
{
    not_null<T*> p(&v);
    {
        std::ostringstream os;
        std::ostringstream ref;
        os << static_cast<void*>(p);
        ref << static_cast<void*>(&v);
        EXPECT_TRUE(os.str() == ref.str());
    }
    {
        std::ostringstream os;
        std::ostringstream ref;
        os << *p;
        ref << v;
        EXPECT_TRUE(os.str() == ref.str());
    }
}

TEST(notnull_tests, TestNotNullostream)
{
    ostream_helper<int>(17);
    ostream_helper<float>(21.5f);
    ostream_helper<double>(3.4566e-7);
    ostream_helper<char>('c');
    ostream_helper<uint16_t>(0x0123u);
    ostream_helper<const char*>("cstring");
    ostream_helper<std::string>("string");
}

template <typename U, typename V, typename = void>
static constexpr bool AssignmentCompilesFor = false;
template <typename U, typename V>
static constexpr bool
    AssignmentCompilesFor<U, V,
                          void_t<decltype(std::declval<gsl::not_null<U*>&>().operator=(
                              std::declval<gsl::not_null<V*>&>()))>> = true;

template <typename U, typename V, typename = void>
static constexpr bool SCastCompilesFor = false;
template <typename U, typename V>
static constexpr bool
    SCastCompilesFor<U, V, void_t<decltype(static_cast<U*>(std::declval<gsl::not_null<V*>&>()))>> =
        true;

template <typename U, typename V, typename = void>
static constexpr bool RCastCompilesFor = false;
template <typename U, typename V>
static constexpr bool RCastCompilesFor<
    U, V, void_t<decltype(reinterpret_cast<U*>(std::declval<gsl::not_null<V*>&>()))>> = true;

TEST(notnull_tests, TestNotNullCasting)
{
    MyBase base;
    MyDerived derived;
    Unrelated unrelated;
    not_null<Unrelated*> u{&unrelated};
    (void) u;
    not_null<MyDerived*> p{&derived};
    not_null<MyBase*> q(&base);
    q = p; // allowed with heterogeneous copy ctor
    EXPECT_TRUE(q == p);

    static_assert(AssignmentCompilesFor<MyBase, MyDerived>,
                  "AssignmentCompilesFor<MyBase, MyDerived>");
    static_assert(!AssignmentCompilesFor<MyBase, Unrelated>,
                  "!AssignmentCompilesFor<MyBase, Unrelated>");
    static_assert(!AssignmentCompilesFor<Unrelated, MyDerived>,
                  "!AssignmentCompilesFor<Unrelated, MyDerived>");
    static_assert(!AssignmentCompilesFor<MyDerived, MyBase>,
                  "!AssignmentCompilesFor<MyDerived, MyBase>");

    static_assert(SCastCompilesFor<MyDerived, MyDerived>, "SCastCompilesFor<MyDerived, MyDerived>");
    static_assert(SCastCompilesFor<MyBase, MyDerived>, "SCastCompilesFor<MyBase, MyDerived>");
    static_assert(!SCastCompilesFor<MyDerived, MyBase>, "!SCastCompilesFor<MyDerived, MyBase>");
    static_assert(!SCastCompilesFor<Unrelated, MyDerived>,
                  "!SCastCompilesFor<Unrelated, MyDerived>");
    static_assert(!RCastCompilesFor<MyDerived, MyDerived>,
                  "!SCastCompilesFor<MyDerived, MyDerived>");
    static_assert(!RCastCompilesFor<Unrelated, MyDerived>,
                  "!SCastCompilesFor<Unrelated, MyDerived>");

    not_null<Unrelated*> t(reinterpret_cast<Unrelated*>(p.get()));
    EXPECT_TRUE(reinterpret_cast<void*>(p.get()) == reinterpret_cast<void*>(t.get()));

    (void) static_cast<MyDerived*>(p);
    (void) static_cast<MyBase*>(p);
}

TEST(notnull_tests, TestNotNullAssignment)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. TestNotNullAssignmentd";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    int i = 12;
    not_null<int*> p(&i);
    EXPECT_TRUE(helper(p));

    int* q = nullptr;
    EXPECT_DEATH(p = not_null<int*>(q), expected);
}

TEST(notnull_tests, TestNotNullRawPointerComparison)
{
    int ints[2] = {42, 43};
    int* p1 = &ints[0];
    const int* p2 = &ints[1];

    using NotNull1 = not_null<decltype(p1)>;
    using NotNull2 = not_null<decltype(p2)>;

    EXPECT_TRUE((NotNull1(p1) == NotNull1(p1)) == true);
    EXPECT_TRUE((NotNull1(p1) == NotNull2(p2)) == false);

    EXPECT_TRUE((NotNull1(p1) != NotNull1(p1)) == false);
    EXPECT_TRUE((NotNull1(p1) != NotNull2(p2)) == true);

    EXPECT_TRUE((NotNull1(p1) < NotNull1(p1)) == false);
    EXPECT_TRUE((NotNull1(p1) < NotNull2(p2)) == (p1 < p2));
    EXPECT_TRUE((NotNull2(p2) < NotNull1(p1)) == (p2 < p1));

    EXPECT_TRUE((NotNull1(p1) > NotNull1(p1)) == false);
    EXPECT_TRUE((NotNull1(p1) > NotNull2(p2)) == (p1 > p2));
    EXPECT_TRUE((NotNull2(p2) > NotNull1(p1)) == (p2 > p1));

    EXPECT_TRUE((NotNull1(p1) <= NotNull1(p1)) == true);
    EXPECT_TRUE((NotNull1(p1) <= NotNull2(p2)) == (p1 <= p2));
    EXPECT_TRUE((NotNull2(p2) <= NotNull1(p1)) == (p2 <= p1));
}

TEST(notnull_tests, TestNotNullDereferenceOperator)
{
    {
        auto sp1 = std::make_shared<NonCopyableNonMovable>();

        using NotNullSp1 = not_null<decltype(sp1)>;
        EXPECT_TRUE(typeid(*sp1) == typeid(*NotNullSp1(sp1)));
        EXPECT_TRUE(std::addressof(*NotNullSp1(sp1)) == std::addressof(*sp1));
    }

    {
        int ints[1] = {42};
        CustomPtr<int> p1(&ints[0]);

        using NotNull1 = not_null<decltype(p1)>;
        EXPECT_TRUE(typeid(*NotNull1(p1)) == typeid(*p1));
        EXPECT_TRUE(*NotNull1(p1) == 42);
        *NotNull1(p1) = 43;
        EXPECT_TRUE(ints[0] == 43);
    }

    {
        int v = 42;
        gsl::not_null<int*> p(&v);
        EXPECT_TRUE(typeid(*p) == typeid(*(&v)));
        *p = 43;
        EXPECT_TRUE(v == 43);
    }
}

TEST(notnull_tests, TestNotNullSharedPtrComparison)
{
    auto sp1 = std::make_shared<int>(42);
    auto sp2 = std::make_shared<const int>(43);

    using NotNullSp1 = not_null<decltype(sp1)>;
    using NotNullSp2 = not_null<decltype(sp2)>;

    EXPECT_TRUE((NotNullSp1(sp1) == NotNullSp1(sp1)) == true);
    EXPECT_TRUE((NotNullSp1(sp1) == NotNullSp2(sp2)) == false);

    EXPECT_TRUE((NotNullSp1(sp1) != NotNullSp1(sp1)) == false);
    EXPECT_TRUE((NotNullSp1(sp1) != NotNullSp2(sp2)) == true);

    EXPECT_TRUE((NotNullSp1(sp1) < NotNullSp1(sp1)) == false);
    EXPECT_TRUE((NotNullSp1(sp1) < NotNullSp2(sp2)) == (sp1 < sp2));
    EXPECT_TRUE((NotNullSp2(sp2) < NotNullSp1(sp1)) == (sp2 < sp1));

    EXPECT_TRUE((NotNullSp1(sp1) > NotNullSp1(sp1)) == false);
    EXPECT_TRUE((NotNullSp1(sp1) > NotNullSp2(sp2)) == (sp1 > sp2));
    EXPECT_TRUE((NotNullSp2(sp2) > NotNullSp1(sp1)) == (sp2 > sp1));

    EXPECT_TRUE((NotNullSp1(sp1) <= NotNullSp1(sp1)) == true);
    EXPECT_TRUE((NotNullSp1(sp1) <= NotNullSp2(sp2)) == (sp1 <= sp2));
    EXPECT_TRUE((NotNullSp2(sp2) <= NotNullSp1(sp1)) == (sp2 <= sp1));

    EXPECT_TRUE((NotNullSp1(sp1) >= NotNullSp1(sp1)) == true);
    EXPECT_TRUE((NotNullSp1(sp1) >= NotNullSp2(sp2)) == (sp1 >= sp2));
    EXPECT_TRUE((NotNullSp2(sp2) >= NotNullSp1(sp1)) == (sp2 >= sp1));
}

TEST(notnull_tests, TestNotNullCustomPtrComparison)
{
    int ints[2] = {42, 43};
    CustomPtr<int> p1(&ints[0]);
    CustomPtr<const int> p2(&ints[1]);

    using NotNull1 = not_null<decltype(p1)>;
    using NotNull2 = not_null<decltype(p2)>;

    EXPECT_TRUE((NotNull1(p1) == NotNull1(p1)) == "true");
    EXPECT_TRUE((NotNull1(p1) == NotNull2(p2)) == "false");

    EXPECT_TRUE((NotNull1(p1) != NotNull1(p1)) == "false");
    EXPECT_TRUE((NotNull1(p1) != NotNull2(p2)) == "true");

    EXPECT_TRUE((NotNull1(p1) < NotNull1(p1)) == "false");
    EXPECT_TRUE((NotNull1(p1) < NotNull2(p2)) == (p1 < p2));
    EXPECT_TRUE((NotNull2(p2) < NotNull1(p1)) == (p2 < p1));

    EXPECT_TRUE((NotNull1(p1) > NotNull1(p1)) == "false");
    EXPECT_TRUE((NotNull1(p1) > NotNull2(p2)) == (p1 > p2));
    EXPECT_TRUE((NotNull2(p2) > NotNull1(p1)) == (p2 > p1));

    EXPECT_TRUE((NotNull1(p1) <= NotNull1(p1)) == "true");
    EXPECT_TRUE((NotNull1(p1) <= NotNull2(p2)) == (p1 <= p2));
    EXPECT_TRUE((NotNull2(p2) <= NotNull1(p1)) == (p2 <= p1));

    EXPECT_TRUE((NotNull1(p1) >= NotNull1(p1)) == "true");
    EXPECT_TRUE((NotNull1(p1) >= NotNull2(p2)) == (p1 >= p2));
    EXPECT_TRUE((NotNull2(p2) >= NotNull1(p1)) == (p2 >= p1));
}

#if defined(__cplusplus) && (__cplusplus >= 201703L)

template <typename U, typename = void>
static constexpr bool TypeDeductionCtorCompilesFor = false;
template <typename U>
static constexpr bool
    TypeDeductionCtorCompilesFor<U, void_t<decltype(not_null{std::declval<U>()})>> = true;

template <typename U, typename = void>
static constexpr bool TypeDeductionHelperCompilesFor = false;
template <typename U>
static constexpr bool
    TypeDeductionHelperCompilesFor<U, void_t<decltype(helper(not_null{std::declval<U>()}))>> = true;

TEST(notnull_tests, TestNotNullConstructorTypeDeduction)
{
    {
        int i = 42;

        not_null x{&i};
        helper(not_null{&i});
        helper_const(not_null{&i});

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;

        not_null x{&i};
        static_assert(TypeDeductionHelperCompilesFor<int*>, "TypeDeductionHelperCompilesFor<int*>");
        static_assert(!TypeDeductionHelperCompilesFor<const int*>,
                      "!TypeDeductionHelperCompilesFor<const int*>");
        helper_const(not_null{&i});

        EXPECT_TRUE(*x == 42);
    }

    {
        int i = 42;
        int* p = &i;

        not_null x{p};
        helper(not_null{p});
        helper_const(not_null{p});

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;
        const int* p = &i;

        not_null x{p};
        helper_const(not_null{p});

        EXPECT_TRUE(*x == 42);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. TestNotNullConstructorTypeDeduction";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    {
        auto workaround_macro = []() {
            int* p1 = nullptr;
            const not_null x{p1};
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        auto workaround_macro = []() {
            const int* p1 = nullptr;
            const not_null x{p1};
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        int* p = nullptr;

        EXPECT_DEATH(helper(not_null{p}), expected);
        EXPECT_DEATH(helper_const(not_null{p}), expected);
    }

    static_assert(TypeDeductionCtorCompilesFor<void*>, "TypeDeductionCtorCompilesFor<void*>");
#if defined(_MSC_VER) && !defined(__clang__)
    // Fails on gcc, clang, xcode, VS clang with
    // "error : no type named 'type' in 'std::enable_if<false>'; 'enable_if' cannot be used to
    // disable this declaration"
    static_assert(!TypeDeductionCtorCompilesFor<std::nullptr_t>,
                  "!TypeDeductionCtorCompilesFor<std::nullptr_t>");
    static_assert(!TypeDeductionHelperCompilesFor<std::nullptr_t>,
                  "!TypeDeductionHelperCompilesFor<std::nullptr_t>");
#endif
}

TEST(notnull_tests, TestVariantEmplace)
{
    int i = 0;
    std::variant<std::monostate, not_null<int*>> v;
    v.emplace<not_null<int*>>(&i);

    EXPECT_FALSE(v.valueless_by_exception());
    EXPECT_TRUE(v.index() == 1);
    EXPECT_TRUE(std::get<not_null<int*>>(v) == &i);
}
#endif // #if defined(__cplusplus) && (__cplusplus >= 201703L)

template <typename U, typename = void>
static constexpr bool HelperCompilesFor = false;
template <typename U>
static constexpr bool HelperCompilesFor<U, void_t<decltype(helper(std::declval<U>()))>> = true;

TEST(notnull_tests, TestMakeNotNull)
{
    {
        int i = 42;

        const auto x = make_not_null(&i);
        helper(make_not_null(&i));
        helper_const(make_not_null(&i));

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;

        const auto x = make_not_null(&i);
        static_assert(HelperCompilesFor<gsl::not_null<int*>>,
                      "HelperCompilesFor<gsl::not_null<int*>>");
        helper_const(make_not_null(&i));

        EXPECT_TRUE(*x == 42);
    }

    {
        int i = 42;
        int* p = &i;

        const auto x = make_not_null(p);
        helper(make_not_null(p));
        helper_const(make_not_null(p));

        EXPECT_TRUE(*x == 42);
    }

    {
        const int i = 42;
        const int* p = &i;

        const auto x = make_not_null(p);
        static_assert(!HelperCompilesFor<gsl::not_null<const int*>>,
                      "!HelperCompilesFor<gsl::not_null<const int*>>");
        helper_const(make_not_null(p));

        EXPECT_TRUE(*x == 42);
    }

    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. TestMakeNotNull";
        std::abort();
    });
    const auto expected = GetExpectedDeathString(terminateHandler);

    {
        const auto workaround_macro = []() {
            int* p1 = nullptr;
            const auto x = make_not_null(p1);
            EXPECT_TRUE(*x == 42);
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        const auto workaround_macro = []() {
            const int* p1 = nullptr;
            const auto x = make_not_null(p1);
            EXPECT_TRUE(*x == 42);
        };
        EXPECT_DEATH(workaround_macro(), expected);
    }

    {
        int* p = nullptr;

        EXPECT_DEATH(helper(make_not_null(p)), expected);
        EXPECT_DEATH(helper_const(make_not_null(p)), expected);
    }

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        EXPECT_DEATH(make_not_null(nullptr), expected);
        EXPECT_DEATH(helper(make_not_null(nullptr)), expected);
        EXPECT_DEATH(helper_const(make_not_null(nullptr)), expected);
    }
#endif
}

TEST(notnull_tests, TestStdHash)
{
    {
        int x = 42;
        int y = 99;
        not_null<int*> nn{&x};
        const not_null<int*> cnn{&x};

        std::hash<not_null<int*>> hash_nn;
        std::hash<int*> hash_intptr;

        EXPECT_TRUE(hash_nn(nn) == hash_intptr(&x));
        EXPECT_FALSE(hash_nn(nn) == hash_intptr(&y));
        EXPECT_FALSE(hash_nn(nn) == hash_intptr(nullptr));
    }

    {
        const int x = 42;
        const int y = 99;
        not_null<const int*> nn{&x};
        const not_null<const int*> cnn{&x};

        std::hash<not_null<const int*>> hash_nn;
        std::hash<const int*> hash_intptr;

        EXPECT_TRUE(hash_nn(nn) == hash_intptr(&x));
        EXPECT_FALSE(hash_nn(nn) == hash_intptr(&y));
        EXPECT_FALSE(hash_nn(nn) == hash_intptr(nullptr));
    }
}
