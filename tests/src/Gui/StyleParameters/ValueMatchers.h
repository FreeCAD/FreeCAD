// SPDX-License-Identifier: LGPL-2.1-or-later
// Test-only helper. Nothing in shipped code uses this.

#pragma once

#include <ostream>
#include <string>
#include <string_view>

#include <gmock/gmock.h>

#include <Gui/StyleParameters/Value.h>

namespace Base
{
inline void PrintTo(const Color& color, std::ostream* os)
{
    *os << Gui::StyleParameters::Value {color}.toString();
}
}  // namespace Base

namespace Gui::StyleParameters
{

inline void PrintTo(const Value& value, std::ostream* os)
{
    *os << value.toString();
}

inline void PrintTo(const Numeric& numeric, std::ostream* os)
{
    *os << Value {numeric}.toString();
}

inline void PrintTo(const Tuple& tuple, std::ostream* os)
{
    *os << Value {tuple}.toString();
}

inline void PrintTo(TupleKind kind, std::ostream* os)
{
    *os << tupleKindName(kind);
}

/**
 * @brief Matchers for the shapes style parameter tests assert over and over.
 *
 * Each matcher checks exactly what it names and nothing else, so a test that only cares about
 * two sides of a box says so instead of being forced to pin all four.
 */
namespace Matchers
{

::testing::Matcher<Base::Color> ColorNear(const Base::Color& expected, double tolerance);
::testing::Matcher<Value> IsNumeric(double value);
::testing::Matcher<Value> IsNumeric(double value, std::string_view unit);
::testing::Matcher<Value> IsColor(const Base::Color& expected);
::testing::Matcher<Value> IsColorNear(const Base::Color& expected, double tolerance);
::testing::Matcher<Tuple> HasField(std::string name, ::testing::Matcher<Value> matcher);
::testing::Matcher<Tuple> HasElement(size_t index, ::testing::Matcher<Value> matcher);

::testing::Matcher<Tuple> HasNumericField(std::string name, double value);
::testing::Matcher<Tuple> HasNumericField(std::string name, double value, std::string_view unit);

::testing::Matcher<Tuple> HasNumericElement(size_t index, double value);
::testing::Matcher<Tuple> HasNumericElement(size_t index, double value, std::string_view unit);

::testing::Matcher<Tuple> HasSides(double top, double right, double bottom, double left);

::testing::Matcher<Tuple> HasCorners(double topLeft, double topRight, double bottomRight, double bottomLeft);

}  // namespace Matchers
}  // namespace Gui::StyleParameters
