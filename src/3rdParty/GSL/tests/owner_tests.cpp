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

#include <gsl/pointers> // for owner
#include <type_traits>  // for declval

using namespace gsl;

GSL_SUPPRESS(f .23) // NO-FORMAT: attribute
void f(int* i) { *i += 1; }

TEST(owner_tests, basic_test)
{
    owner<int*> p = new int(120);
    EXPECT_TRUE(*p == 120);
    f(p);
    EXPECT_TRUE(*p == 121);
    delete p;
}

#if __cplusplus >= 201703l
using std::void_t;
#else  // __cplusplus >= 201703l
template <class...>
using void_t = void;
#endif // __cplusplus < 201703l

template <typename U, typename = void>
static constexpr bool OwnerCompilesFor = false;
template <typename U>
static constexpr bool OwnerCompilesFor<U, void_t<decltype(gsl::owner<U>{})>> =
        true;
static_assert(OwnerCompilesFor<int*>, "OwnerCompilesFor<int*>");
static_assert(!OwnerCompilesFor<int>, "!OwnerCompilesFor<int>");
static_assert(!OwnerCompilesFor<std::shared_ptr<int>>, "!OwnerCompilesFor<std::shared_ptr<int>>");
