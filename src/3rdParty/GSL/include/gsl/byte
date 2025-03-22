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

#ifndef GSL_BYTE_H
#define GSL_BYTE_H

#include "./util" // for GSL_DEPRECATED

#include <type_traits>

#ifdef _MSC_VER

#pragma warning(push)

// Turn MSVC /analyze rules that generate too much noise. TODO: fix in the tool.
#pragma warning(disable : 26493) // don't use c-style casts // TODO: MSVC suppression in templates
                                 // does not always work

#ifndef GSL_USE_STD_BYTE
// this tests if we are under MSVC and the standard lib has std::byte and it is enabled
#if defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603

#define GSL_USE_STD_BYTE 1

#else // defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603

#define GSL_USE_STD_BYTE 0

#endif // defined(__cpp_lib_byte) && __cpp_lib_byte >= 201603
#endif // GSL_USE_STD_BYTE

#else // _MSC_VER

#ifndef GSL_USE_STD_BYTE
#include <cstddef> /* __cpp_lib_byte */
// this tests if we are under GCC or Clang with enough -std=c++1z power to get us std::byte
// also check if libc++ version is sufficient (> 5.0) or libstdc++ actually contains std::byte
#if defined(__cplusplus) && (__cplusplus >= 201703L) &&                                            \
    (defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603) ||                                      \
     defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 5000))

#define GSL_USE_STD_BYTE 1

#else // defined(__cplusplus) && (__cplusplus >= 201703L) &&
      //   (defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603)  ||
      //    defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 5000))

#define GSL_USE_STD_BYTE 0

#endif // defined(__cplusplus) && (__cplusplus >= 201703L) &&
       //   (defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603)  ||
       //    defined(_LIBCPP_VERSION) && (_LIBCPP_VERSION >= 5000))
#endif // GSL_USE_STD_BYTE

#endif // _MSC_VER

// Use __may_alias__ attribute on gcc and clang
#if defined __clang__ || (defined(__GNUC__) && __GNUC__ > 5)
#define byte_may_alias __attribute__((__may_alias__))
#else // defined __clang__ || defined __GNUC__
#define byte_may_alias
#endif // defined __clang__ || defined __GNUC__

#if GSL_USE_STD_BYTE
#include <cstddef>
#endif

namespace gsl
{
#if GSL_USE_STD_BYTE

namespace impl {
// impl::byte is used by gsl::as_bytes so our own code does not trigger a deprecation warning as would be the case when we used gsl::byte.
// Users of GSL should only use gsl::byte, not gsl::impl::byte.
using byte = std::byte;
}

using byte GSL_DEPRECATED("Use std::byte instead.") = std::byte;

using std::to_integer;

#else // GSL_USE_STD_BYTE

// This is a simple definition for now that allows
// use of byte within span<> to be standards-compliant
enum class byte_may_alias byte : unsigned char
{
};

namespace impl {
// impl::byte is used by gsl::as_bytes so our own code does not trigger a deprecation warning as would be the case when we used gsl::byte.
// Users of GSL should only use gsl::byte, not gsl::impl::byte.
using byte = gsl::byte;
}

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte& operator<<=(byte& b, IntegerType shift) noexcept
{
    return b = byte(static_cast<unsigned char>(b) << shift);
}

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte operator<<(byte b, IntegerType shift) noexcept
{
    return byte(static_cast<unsigned char>(b) << shift);
}

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte& operator>>=(byte& b, IntegerType shift) noexcept
{
    return b = byte(static_cast<unsigned char>(b) >> shift);
}

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr byte operator>>(byte b, IntegerType shift) noexcept
{
    return byte(static_cast<unsigned char>(b) >> shift);
}

constexpr byte& operator|=(byte& l, byte r) noexcept
{
    return l = byte(static_cast<unsigned char>(l) | static_cast<unsigned char>(r));
}

constexpr byte operator|(byte l, byte r) noexcept
{
    return byte(static_cast<unsigned char>(l) | static_cast<unsigned char>(r));
}

constexpr byte& operator&=(byte& l, byte r) noexcept
{
    return l = byte(static_cast<unsigned char>(l) & static_cast<unsigned char>(r));
}

constexpr byte operator&(byte l, byte r) noexcept
{
    return byte(static_cast<unsigned char>(l) & static_cast<unsigned char>(r));
}

constexpr byte& operator^=(byte& l, byte r) noexcept
{
    return l = byte(static_cast<unsigned char>(l) ^ static_cast<unsigned char>(r));
}

constexpr byte operator^(byte l, byte r) noexcept
{
    return byte(static_cast<unsigned char>(l) ^ static_cast<unsigned char>(r));
}

constexpr byte operator~(byte b) noexcept { return byte(~static_cast<unsigned char>(b)); }

template <class IntegerType, std::enable_if_t<std::is_integral<IntegerType>::value, bool> = true>
constexpr IntegerType to_integer(byte b) noexcept
{
    return static_cast<IntegerType>(b);
}

#endif // GSL_USE_STD_BYTE


template <typename T>
// NOTE: need suppression since c++14 does not allow "return {t}"
// GSL_SUPPRESS(type.4) // NO-FORMAT: attribute // TODO: suppression does not work
constexpr gsl::impl::byte to_byte(T t) noexcept
{
    static_assert(std::is_same<T, unsigned char>::value,
                  "gsl::to_byte(t) must be provided an unsigned char, otherwise data loss may occur. "
                  "If you are calling to_byte with an integer constant use: gsl::to_byte<t>() version.");
    return gsl::impl::byte(t);
}

template <int I>
constexpr gsl::impl::byte to_byte() noexcept
{
    static_assert(I >= 0 && I <= 255,
                  "gsl::byte only has 8 bits of storage, values must be in range 0-255");
    return static_cast<gsl::impl::byte>(I);
}

} // namespace gsl

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // GSL_BYTE_H
