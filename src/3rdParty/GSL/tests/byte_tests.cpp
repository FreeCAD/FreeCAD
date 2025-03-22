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

#define GSL_USE_STD_BYTE 0
#include <gsl/byte> // for to_byte, to_integer, byte, operator&, ope...

#include <type_traits>
#include <utility>

using namespace std;
using namespace gsl;

namespace
{
int modify_both(gsl::byte& b, int& i)
{
    i = 10;
    b = to_byte<5>();
    return i;
}

TEST(byte_tests, construction)
{
    {
        const gsl::byte b = static_cast<gsl::byte>(4);
        EXPECT_TRUE(static_cast<unsigned char>(b) == 4);
    }

    {
        const gsl::byte b = gsl::byte(12);
        EXPECT_TRUE(static_cast<unsigned char>(b) == 12);
    }

    {
        const gsl::byte b = to_byte<12>();
        EXPECT_TRUE(static_cast<unsigned char>(b) == 12);
    }
    {
        const unsigned char uc = 12;
        const gsl::byte b = to_byte(uc);
        EXPECT_TRUE(static_cast<unsigned char>(b) == 12);
    }

#if defined(__cplusplus) && (__cplusplus >= 201703L)
    {
        const gsl::byte b{14};
        EXPECT_TRUE(static_cast<unsigned char>(b) == 14);
    }
#endif

#ifdef CONFIRM_COMPILATION_ERRORS
    to_byte(char{});
    to_byte(3);
    to_byte(3u);
    to_byte<-1>();
    to_byte<256u>();
#endif
}

TEST(byte_tests, bitwise_operations)
{
    const gsl::byte b = to_byte<0xFF>();

    gsl::byte a = to_byte<0x00>();
    EXPECT_TRUE((b | a) == to_byte<0xFF>());
    EXPECT_TRUE(a == to_byte<0x00>());

    a |= b;
    EXPECT_TRUE(a == to_byte<0xFF>());

    a = to_byte<0x01>();
    EXPECT_TRUE((b & a) == to_byte<0x01>());

    a &= b;
    EXPECT_TRUE(a == to_byte<0x01>());

    EXPECT_TRUE((b ^ a) == to_byte<0xFE>());

    EXPECT_TRUE(a == to_byte<0x01>());
    a ^= b;
    EXPECT_TRUE(a == to_byte<0xFE>());

    a = to_byte<0x01>();
    EXPECT_TRUE(~a == to_byte<0xFE>());

    a = to_byte<0xFF>();
    EXPECT_TRUE((a << 4) == to_byte<0xF0>());
    EXPECT_TRUE((a >> 4) == to_byte<0x0F>());

    a <<= 4;
    EXPECT_TRUE(a == to_byte<0xF0>());
    a >>= 4;
    EXPECT_TRUE(a == to_byte<0x0F>());
}

TEST(byte_tests, to_integer)
{
    const gsl::byte b = to_byte<0x12>();

    EXPECT_TRUE(0x12 == gsl::to_integer<char>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<short>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<long>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<long long>(b));

    EXPECT_TRUE(0x12 == gsl::to_integer<unsigned char>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<unsigned short>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<unsigned long>(b));
    EXPECT_TRUE(0x12 == gsl::to_integer<unsigned long long>(b));

    //      EXPECT_TRUE(0x12 == gsl::to_integer<float>(b));   // expect compile-time error
    //      EXPECT_TRUE(0x12 == gsl::to_integer<double>(b));  // expect compile-time error
}

TEST(byte_tests, aliasing)
{
    int i{0};
    const int res = modify_both(reinterpret_cast<gsl::byte&>(i), i);
    EXPECT_TRUE(res == i);
}

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

template <typename U, typename = void>
static constexpr bool LShiftCompilesFor = false;
template <typename U>
static constexpr bool LShiftCompilesFor<
    U, void_t<decltype(gsl::operator<< <float>(declval<gsl::byte>(), declval<U>()))>> = true;
static_assert(!LShiftCompilesFor<float>, "!LShiftCompilesFor<float>");

template <typename U, typename = void>
static constexpr bool RShiftCompilesFor = false;
template <typename U>
static constexpr bool RShiftCompilesFor<
    U, void_t<decltype(gsl::operator>> <U>(declval<gsl::byte>(), declval<U>()))>> = true;
static_assert(!RShiftCompilesFor<float>, "!RShiftCompilesFor<float>");

template <typename U, typename = void>
static constexpr bool LShiftAssignCompilesFor = false;
template <typename U>
static constexpr bool LShiftAssignCompilesFor<
    U, void_t<decltype(gsl::operator<<= <U>(declval<gsl::byte&>(), declval<U>()))>> = true;
static_assert(!LShiftAssignCompilesFor<float>, "!LShiftAssignCompilesFor<float>");

template <typename U, typename = void>
static constexpr bool RShiftAssignCompilesFor = false;
template <typename U>
static constexpr bool RShiftAssignCompilesFor<
    U, void_t<decltype(gsl::operator>>= <U>(declval<gsl::byte&>(), declval<U>()))>> = true;
static_assert(!RShiftAssignCompilesFor<float>, "!RShiftAssignCompilesFor<float>");

template <typename U, typename = void>
static constexpr bool ToIntegerCompilesFor = false;
template <typename U>
static constexpr bool
    ToIntegerCompilesFor<U, void_t<decltype(gsl::to_integer<U>(gsl::byte{}))>> = true;
static_assert(!ToIntegerCompilesFor<float>, "!ToIntegerCompilesFor<float>");

} // namespace
