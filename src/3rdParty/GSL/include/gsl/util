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

#ifndef GSL_UTIL_H
#define GSL_UTIL_H

#include "./assert" // for Expects

#include <array>
#include <cstddef>          // for ptrdiff_t, size_t
#include <limits>           // for numeric_limits
#include <initializer_list> // for initializer_list
#include <type_traits>      // for is_signed, integral_constant
#include <utility>          // for exchange, forward

#if defined(__has_include) && __has_include(<version>)
#include <version>
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#include <span>
#endif // __cpp_lib_span >= 202002L
#endif //__has_include(<version>)

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant

#endif // _MSC_VER

// Turn off clang unsafe buffer warnings as all accessed are guarded by runtime checks
#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif // __has_warning("-Wunsafe-buffer-usage")
#endif // defined(__clang__)

#if defined(__cplusplus) && (__cplusplus >= 201703L)
#define GSL_NODISCARD [[nodiscard]]
#else
#define GSL_NODISCARD
#endif // defined(__cplusplus) && (__cplusplus >= 201703L)

#if defined(__cpp_inline_variables)
#define GSL_INLINE inline
#else
#define GSL_INLINE
#endif

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(deprecated)
#define GSL_DEPRECATED(msg) [[deprecated(msg)]]
#endif // __has_cpp_attribute(deprecated)
#endif // defined(__has_cpp_attribute)

#if !defined(GSL_DEPRECATED)
#if defined(__cplusplus)
#if __cplusplus >= 201309L
#define GSL_DEPRECATED(msg) [[deprecated(msg)]]
#endif // __cplusplus >= 201309L
#endif // defined(__cplusplus)
#endif // !defined(GSL_DEPRECATED)

#if !defined(GSL_DEPRECATED)
#if defined(_MSC_VER)
#define GSL_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__)
#define GSL_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif // defined(_MSC_VER)
#endif // !defined(GSL_DEPRECATED)

#if !defined(GSL_DEPRECATED)
#define GSL_DEPRECATED(msg)
#endif // !defined(GSL_DEPRECATED)

namespace gsl
{
//
// GSL.util: utilities
//

// index type for all container indexes/subscripts/sizes
using index = std::ptrdiff_t;

// final_action allows you to ensure something gets run at the end of a scope
template <class F>
class final_action
{
public:
    explicit final_action(const F& ff) noexcept : f{ff} { }
    explicit final_action(F&& ff) noexcept : f{std::move(ff)} { }

    ~final_action() noexcept { if (invoke) f(); }

    final_action(final_action&& other) noexcept
        : f(std::move(other.f)), invoke(std::exchange(other.invoke, false))
    { }

    final_action(const final_action&)   = delete;
    void operator=(const final_action&) = delete;
    void operator=(final_action&&)      = delete;

private:
    F f;
    bool invoke = true;
};

// finally() - convenience function to generate a final_action
template <class F>
GSL_NODISCARD auto finally(F&& f) noexcept
{
    return final_action<std::decay_t<F>>{std::forward<F>(f)};
}

// narrow_cast(): a searchable way to do narrowing casts of values
template <class T, class U>
// clang-format off
GSL_SUPPRESS(type.1) // NO-FORMAT: attribute
    // clang-format on
    constexpr T narrow_cast(U&& u) noexcept
{
    return static_cast<T>(std::forward<U>(u));
}

//
// at() - Bounds-checked way of accessing builtin arrays, std::array, std::vector
//
template <class T, std::size_t N>
// clang-format off
GSL_SUPPRESS(bounds.4) // NO-FORMAT: attribute
GSL_SUPPRESS(bounds.2) // NO-FORMAT: attribute
    // clang-format on
    constexpr T& at(T (&arr)[N], const index i)
{
    static_assert(N <= static_cast<std::size_t>((std::numeric_limits<std::ptrdiff_t>::max)()), "We only support arrays up to PTRDIFF_MAX bytes.");
    Expects(i >= 0 && i < narrow_cast<index>(N));
    return arr[narrow_cast<std::size_t>(i)];
}

template <class Cont>
// clang-format off
GSL_SUPPRESS(bounds.4) // NO-FORMAT: attribute
GSL_SUPPRESS(bounds.2) // NO-FORMAT: attribute
    // clang-format on
    constexpr auto at(Cont& cont, const index i) -> decltype(cont[cont.size()])
{
    Expects(i >= 0 && i < narrow_cast<index>(cont.size()));
    using size_type = decltype(cont.size());
    return cont[narrow_cast<size_type>(i)];
}

template <class T>
// clang-format off
GSL_SUPPRESS(bounds.1) // NO-FORMAT: attribute
    // clang-format on
    constexpr T at(const std::initializer_list<T> cont, const index i)
{
    Expects(i >= 0 && i < narrow_cast<index>(cont.size()));
    return *(cont.begin() + i);
}

template <class T, std::enable_if_t<std::is_move_assignable<T>::value && std::is_move_constructible<T>::value>>
void swap(T& a, T& b) { std::swap(a, b); }

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
template <class T, std::size_t extent = std::dynamic_extent>
constexpr auto at(std::span<T, extent> sp, const index i) -> decltype(sp[sp.size()])
{
    Expects(i >= 0 && i < narrow_cast<index>(sp.size()));
    return sp[gsl::narrow_cast<std::size_t>(i)];
}
#endif // __cpp_lib_span >= 202002L
} // namespace gsl

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif // _MSC_VER

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic pop
#endif // __has_warning("-Wunsafe-buffer-usage")
#endif // defined(__clang__)

#endif // GSL_UTIL_H
