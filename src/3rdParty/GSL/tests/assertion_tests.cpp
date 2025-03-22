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

#include "deathTestCommon.h"
#include <gsl/assert> // for Ensures, Expects
#include <gtest/gtest.h>

using namespace gsl;

namespace
{

int f(int i)
{
    Expects(i > 0 && i < 10);
    return i;
}

int g(int i)
{
    i++;
    Ensures(i > 0 && i < 10);
    return i;
}
} // namespace

TEST(assertion_tests, expects)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. expects";
        std::abort();
    });

    EXPECT_TRUE(f(2) == 2);
    EXPECT_DEATH(f(10), GetExpectedDeathString(terminateHandler));
}

TEST(assertion_tests, ensures)
{
    const auto terminateHandler = std::set_terminate([] {
        std::cerr << "Expected Death. ensures";
        std::abort();
    });

    EXPECT_TRUE(g(2) == 3);
    EXPECT_DEATH(g(9), GetExpectedDeathString(terminateHandler));
}
