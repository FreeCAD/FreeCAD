#include <gtest/gtest.h>

#include <gsl/pointers>

#include <memory>
#include <type_traits>
#include <utility>

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

namespace
{
// Custom pointer type that can be used for gsl::not_null, but for which these cannot be swapped.
struct NotMoveAssignableCustomPtr
{
    NotMoveAssignableCustomPtr() = default;
    NotMoveAssignableCustomPtr(const NotMoveAssignableCustomPtr&) = default;
    NotMoveAssignableCustomPtr& operator=(const NotMoveAssignableCustomPtr&) = default;
    NotMoveAssignableCustomPtr(NotMoveAssignableCustomPtr&&) = default;
    NotMoveAssignableCustomPtr& operator=(NotMoveAssignableCustomPtr&&) = delete;

    bool operator!=(std::nullptr_t) const { return true; }

    int dummy{}; // Without this clang warns, that NotMoveAssignableCustomPtr() is unneeded
};

template <typename U, typename = void>
static constexpr bool SwapCompilesFor = false;
template <typename U>
static constexpr bool
    SwapCompilesFor<U, void_t<decltype(gsl::swap<U>(std::declval<gsl::not_null<U>&>(),
                                                    std::declval<gsl::not_null<U>&>()))>> = true;

TEST(pointers_test, swap)
{
    // taken from gh-1129:
    {
        gsl::not_null<std::unique_ptr<int>> a(std::make_unique<int>(0));
        gsl::not_null<std::unique_ptr<int>> b(std::make_unique<int>(1));

        EXPECT_TRUE(*a == 0);
        EXPECT_TRUE(*b == 1);

        gsl::swap(a, b);

        EXPECT_TRUE(*a == 1);
        EXPECT_TRUE(*b == 0);

        // Make sure our custom ptr can be used with not_null. The shared_pr is to prevent "unused"
        // compiler warnings.
        const auto shared_custom_ptr{std::make_shared<NotMoveAssignableCustomPtr>()};
        gsl::not_null<NotMoveAssignableCustomPtr> c{*shared_custom_ptr};
        EXPECT_TRUE(c.get() != nullptr);
    }

    {
        gsl::strict_not_null<std::unique_ptr<int>> a{std::make_unique<int>(0)};
        gsl::strict_not_null<std::unique_ptr<int>> b{std::make_unique<int>(1)};

        EXPECT_TRUE(*a == 0);
        EXPECT_TRUE(*b == 1);

        gsl::swap(a, b);

        EXPECT_TRUE(*a == 1);
        EXPECT_TRUE(*b == 0);
    }

    {
        gsl::not_null<std::unique_ptr<int>> a{std::make_unique<int>(0)};
        gsl::strict_not_null<std::unique_ptr<int>> b{std::make_unique<int>(1)};

        EXPECT_TRUE(*a == 0);
        EXPECT_TRUE(*b == 1);

        gsl::swap(a, b);

        EXPECT_TRUE(*a == 1);
        EXPECT_TRUE(*b == 0);
    }

    static_assert(!SwapCompilesFor<NotMoveAssignableCustomPtr>,
                  "!SwapCompilesFor<NotMoveAssignableCustomPtr>");
}

TEST(pointers_test, member_types)
{
    static_assert(std::is_same<gsl::not_null<int*>::element_type, int*>::value,
                  "check member type: element_type");
}

} // namespace
