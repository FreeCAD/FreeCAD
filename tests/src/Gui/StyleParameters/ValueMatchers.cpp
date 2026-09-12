// SPDX-License-Identifier: LGPL-2.1-or-later
// Test-only helper. Nothing in shipped code uses this.

#include "ValueMatchers.h"

#include <fmt/format.h>

using ::testing::AllOf;
using ::testing::DoubleEq;
using ::testing::Eq;
using ::testing::Field;
using ::testing::FloatNear;
using ::testing::Matcher;
using ::testing::Pointee;
using ::testing::ResultOf;

namespace Gui::StyleParameters::Matchers
{

namespace
{

/// Lifts a matcher on T onto a Value, failing when the Value holds another type.
template<typename T>
Matcher<Value> holding(Matcher<T> matcher)
{
    return ResultOf(
        valueTypeName<T>(),
        [](const Value& value) { return value.tryGet<T>(); },
        Pointee(std::move(matcher))
    );
}

}  // namespace

Matcher<Base::Color> ColorNear(const Base::Color& expected, double tolerance)
{
    const auto margin = static_cast<float>(tolerance);

    return AllOf(
        Field("r", &Base::Color::r, FloatNear(expected.r, margin)),
        Field("g", &Base::Color::g, FloatNear(expected.g, margin)),
        Field("b", &Base::Color::b, FloatNear(expected.b, margin))
    );
}

Matcher<Value> IsNumeric(double value)
{
    return holding<Numeric>(Field("value", &Numeric::value, DoubleEq(value)));
}

Matcher<Value> IsNumeric(double value, std::string_view unit)
{
    return holding<Numeric>(AllOf(
        Field("value", &Numeric::value, DoubleEq(value)),
        Field("unit", &Numeric::unit, Eq(std::string {unit}))
    ));
}

Matcher<Value> IsColor(const Base::Color& expected)
{
    return holding<Base::Color>(Eq(expected));
}

Matcher<Value> IsColorNear(const Base::Color& expected, double tolerance)
{
    return holding<Base::Color>(ColorNear(expected, tolerance));
}

Matcher<Tuple> HasField(std::string name, Matcher<Value> matcher)
{
    return ResultOf(
        fmt::format("field '{}'", name),
        [name = std::move(name)](const Tuple& tuple) { return tuple.find(name); },
        Pointee(matcher)
    );
}

Matcher<Tuple> HasElement(size_t index, Matcher<Value> matcher)
{
    return ResultOf(
        fmt::format("element {}", index),
        [index](const Tuple& tuple) { return tuple.tryAt(index); },
        Pointee(matcher)
    );
}

Matcher<Tuple> HasNumericField(std::string name, double value)
{
    return HasField(std::move(name), IsNumeric(value));
}

Matcher<Tuple> HasNumericField(std::string name, double value, std::string_view unit)
{
    return HasField(std::move(name), IsNumeric(value, unit));
}

Matcher<Tuple> HasNumericElement(size_t index, double value)
{
    return HasElement(index, IsNumeric(value));
}

Matcher<Tuple> HasNumericElement(size_t index, double value, std::string_view unit)
{
    return HasElement(index, IsNumeric(value, unit));
}

Matcher<Tuple> HasSides(double top, double right, double bottom, double left)
{
    return AllOf(
        HasNumericField("top", top),
        HasNumericField("right", right),
        HasNumericField("bottom", bottom),
        HasNumericField("left", left)
    );
}

Matcher<Tuple> HasCorners(double topLeft, double topRight, double bottomRight, double bottomLeft)
{
    return AllOf(
        HasNumericField("top_left", topLeft),
        HasNumericField("top_right", topRight),
        HasNumericField("bottom_right", bottomRight),
        HasNumericField("bottom_left", bottomLeft)
    );
}

}  // namespace Gui::StyleParameters::Matchers
