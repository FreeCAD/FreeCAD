// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef STYLEPARAMETERS_VALUE_H
#define STYLEPARAMETERS_VALUE_H

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <cstdint>

#include <fmt/format.h>

#include <Base/Color.h>
#include <FCGlobal.h>

#include "Diagnostics.h"

namespace Gui::StyleParameters
{

/**
 * @brief Represents a length in a specified unit.
 *
 * This struct is a very simplified representation of lengths that can be used as parameters for
 * styling purposes. The length basically consists of value and unit. Unit is optional, empty unit
 * represents a dimensionless length that can be used as a scalar. This struct does not care about
 * unit conversions as its uses do not require it.
 */
struct GuiExport Numeric
{
    /// Numeric value of the length.
    double value = 0.0;
    /// Unit of the length, empty if the value is dimensionless.
    std::string unit = "";

    /**
     * @name Operators
     *
     * This struct supports basic operations on Length. Each operation requires for operands to be
     * the same unit. Multiplication and division additionally allow one operand to be dimensionless
     * and hence act as a scalar.
     *
     * @code{c++}
     * Numeric a { 10, "px" };
     * Numeric b { 5, "px" };
     *
     * Numeric differentUnit { 3, "rem" }
     * Numeric scalar { 2, "" };
     *
     * // basic operations with the same unit are allowed
     * auto sum = a + b; // 15 px
     * auto difference = a - 5; // 10 px
     *
     * // basic operations with mixed units are NOT allowed
     * auto sumOfIncompatibleUnits = a + differentUnit; // will throw
     * auto productOfIncompatibleUnits = a * differentUnit; // will throw
     *
     * // exception is that for multiplication and division dimensionless units are allowed
     * auto productWithScalar = a * scalar; // 20 px
     * @endcode
     * @{
     */
    Numeric operator+(const Numeric& rhs) const;
    Numeric operator-(const Numeric& rhs) const;
    Numeric operator-() const;

    Numeric operator/(const Numeric& rhs) const;
    Numeric operator*(const Numeric& rhs) const;
    /// @}

private:
    void ensureEqualUnits(const Numeric& rhs) const;
};

// Forward declaration: Tuple::Element uses shared_ptr<const Value> to break the
// circular dependency (Value contains Tuple, Tuple elements contain Value).
struct Value;

/**
 * @brief Identifies the semantic kind of a tuple.
 *
 * Generic tuples have no special meaning. A typed kind carries structural identity, so
 * consuming code can tell box-model insets from corner radii or a gradient rather than
 * guessing from shape alone. Each kind is produced by its own construction function and
 * accepted only after its expected element names and types have been validated.
 */
enum class TupleKind : std::uint8_t
{
    Generic,
    Padding,
    Margins,
    BorderThickness,
    BorderColors,
    Corners,
    LinearGradient,
    RadialGradient,
};

constexpr const char* tupleKindName(TupleKind kind)
{
    switch (kind) {
        case TupleKind::Generic:
            return "Generic";
        case TupleKind::Padding:
            return "Padding";
        case TupleKind::Margins:
            return "Margins";
        case TupleKind::BorderThickness:
            return "BorderThickness";
        case TupleKind::BorderColors:
            return "BorderColors";
        case TupleKind::Corners:
            return "Corners";
        case TupleKind::LinearGradient:
            return "LinearGradient";
        case TupleKind::RadialGradient:
            return "RadialGradient";
    }
    return "<unknown>";
}

/// True for the tuple kinds that share the four-sided box shape and may coerce between each other.
constexpr bool isEdgeKind(TupleKind kind)
{
    switch (kind) {
        case TupleKind::Padding:
        case TupleKind::Margins:
        case TupleKind::BorderThickness:
        case TupleKind::BorderColors:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Represents a tuple of named or unnamed values.
 *
 * Tuples group related values into a single parameter using `(key: val1, val2)` syntax.
 * Elements can optionally have names for named access.
 */
struct GuiExport Tuple
{
    struct Element
    {
        std::optional<std::string> name;
        std::shared_ptr<const Value> value;

        /// Creates a named element, wrapping val in a shared_ptr.
        static Element named(std::string name, Value val);

        /// Creates an unnamed element, wrapping val in a shared_ptr.
        static Element unnamed(Value val);
    };

    Tuple() = default;
    Tuple(std::initializer_list<Element> elements);
    Tuple(std::initializer_list<Element> elements, TupleKind kind);

    TupleKind kind = TupleKind::Generic;
    std::vector<Element> elements;

    /**
     * @brief Returns a pointer to the value at the given index, or nullptr when out of range.
     */
    const Value* tryAt(size_t index) const;

    /**
     * @brief Returns the value at the given index, or an empty value when out of range.
     */
    const Value& at(size_t index) const;

    /**
     * @brief Finds an element by name.
     * @return Pointer to the value if found, nullptr otherwise.
     */
    const Value* find(const std::string& name) const;

    /**
     * @brief Returns the number of elements in the tuple.
     */
    size_t size() const;

    /**
     * @brief Returns a pointer to the named element of type T, or nullptr when absent or of
     *        another type.
     */
    template<typename T>
    const T* tryGet(const std::string& name) const;

    /**
     * @brief Returns the named element, or the empty value of type T when it cannot be produced.
     */
    template<typename T>
    const T& get(const std::string& name) const;

    /**
     * @brief Returns the named element, or the given fallback when it cannot be produced.
     */
    template<typename T>
    T get(const std::string& name, const T& fallback) const;

    /**
     * @brief Returns the named element of type T, reporting when it is absent or of another type.
     */
    template<typename T>
    const T* tryGetOrReport(const std::string& name) const;
};

/// Convenience alias for Tuple::Element, used pervasively by tuple-shaped wrappers.
using Element = Tuple::Element;

template<typename T>
constexpr const char* valueTypeName()
{
    if constexpr (std::is_same_v<T, Numeric>) {
        return "numeric";
    }
    else if constexpr (std::is_same_v<T, Base::Color>) {
        return "color";
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return "string";
    }
    else if constexpr (std::is_same_v<T, Tuple>) {
        return "tuple";
    }

    return "<unknown>";
}

/**
 * @brief The empty value substituted when a request of type T cannot be satisfied.
 */
template<typename T>
const T& styleDefault();

/**
 * @brief This struct represents any valid value that can be used as the parameter value.
 *
 * The value can be one of four basic types:
 *  - Numbers / Lengths (so any length with optional unit) (Numeric)
 *  - Colors (Base::Color)
 *  - Any other generic expression (std::string)
 *  - Tuples of values (Tuple)
 *
 * As a rule, operations can be only performed over values of the same type.
 */
struct GuiExport Value: std::variant<Numeric, Base::Color, std::string, Tuple>
{
    using std::variant<Numeric, Base::Color, std::string, Tuple>::variant;

    /**
     * Converts the object into its string representation.
     *
     * @return A human-readable string representation of the object (debug/display format).
     */
    std::string toString() const;

    /**
     * @brief Checks whether this value holds the given type.
     */
    template<typename T>
    bool holds() const
    {
        return std::holds_alternative<T>(*this);
    }

    /**
     * @brief Returns a pointer to the held value, or nullptr if it holds another type.
     */
    template<typename T>
    const T* tryGet() const
    {
        return std::get_if<T>(this);
    }

    /**
     * @brief Returns the held value, or the empty value of type T when it holds another type.
     */
    template<typename T>
    const T& get() const
    {
        if (const T* value = tryGet<T>()) {
            return *value;
        }

        Diagnostics::report("Expected {} value, got {}", valueTypeName<T>(), toString());
        return styleDefault<T>();
    }

    /**
     * @brief Returns the held value, or the given fallback when it holds another type.
     */
    template<typename T>
    T get(const T& fallback) const
    {
        if (const T* value = tryGet<T>()) {
            return *value;
        }

        Diagnostics::report("Expected {} value, got {}", valueTypeName<T>(), toString());
        return fallback;
    }

    /**
     * @name Arithmetic operators
     *
     * Element-wise operations for tuples of equal size, scalar broadcast for Tuple × Numeric.
     * Delegates to Numeric operators for Numeric × Numeric.
     * @{
     */
    Value operator+(const Value& rhs) const;
    Value operator-(const Value& rhs) const;
    Value operator*(const Value& rhs) const;
    Value operator/(const Value& rhs) const;
    Value operator-() const;
    /// @}
};

template<>
inline const Numeric& styleDefault<Numeric>()
{
    static const Numeric value {.value = 0.0, .unit = ""};
    return value;
}

template<>
inline const Base::Color& styleDefault<Base::Color>()
{
    static const Base::Color value(0.0F, 0.0F, 0.0F, 0.0F);
    return value;
}

template<>
inline const std::string& styleDefault<std::string>()
{
    static const std::string value;
    return value;
}

template<>
inline const Tuple& styleDefault<Tuple>()
{
    static const Tuple value;
    return value;
}

template<typename T>
const T* Tuple::tryGetOrReport(const std::string& name) const
{
    if (const T* value = tryGet<T>(name)) {
        return value;
    }

    if (const Value* found = find(name)) {
        Diagnostics::report("Argument '{}' must be {}, got {}", name, valueTypeName<T>(), *found);
    }
    else {
        Diagnostics::report("Missing argument '{}'", name);
    }

    return nullptr;
}

template<typename T>
const T* Tuple::tryGet(const std::string& name) const
{
    const Value* value = find(name);
    return value ? value->tryGet<T>() : nullptr;
}

template<typename T>
const T& Tuple::get(const std::string& name) const
{
    if (const T* value = tryGetOrReport<T>(name)) {
        return *value;
    }

    return styleDefault<T>();
}

template<typename T>
T Tuple::get(const std::string& name, const T& fallback) const
{
    if (const T* value = tryGetOrReport<T>(name)) {
        return *value;
    }

    return fallback;
}

/**
 * @brief Wraps a Value in a 1-element unnamed generic Tuple if it is not already a Tuple.
 *
 * This allows scalar values (e.g. a bare Numeric) to be passed into constructors that
 * expect a Tuple and use CSS-like expansion (1 element → all sides equal).
 */
inline Tuple asTuple(const Value& value)
{
    if (value.holds<Tuple>()) {
        return value.get<Tuple>();
    }
    return Tuple({Tuple::Element::unnamed(value)});
}

/**
 * @brief Defines a single parameter in a function signature.
 *
 * Used with ArgumentParser to declare positional/named parameters with optional defaults.
 */
struct ParamDef
{
    std::string name;
    std::optional<Value> defaultValue = {};
};

/**
 * @brief Resolves a Tuple of mixed positional/named arguments against a declared signature.
 *
 * Named arguments are matched by name, unnamed arguments fill remaining slots in declaration
 * order, and defaults fill any still-empty slots. The result is a Tuple with all elements named
 * per the signature.
 *
 * Example:
 * @code{.cpp}
 * auto resolved = ArgumentParser{{"color"}, {"amount", Numeric{20, ""}}}.resolve(args);
 * auto& color  = resolved.get<Base::Color>("color");
 * auto& amount = resolved.get<Numeric>("amount");
 * @endcode
 */
class GuiExport ArgumentParser
{
public:
    ArgumentParser(std::initializer_list<ParamDef> params);

    Tuple resolve(const Tuple& args) const;

private:
    std::vector<ParamDef> params_;
};

/**
 * @brief Satisfied by domain wrapper types that validate a Value themselves.
 */
template<typename T>
concept HasTryFrom = requires(const Value& value) {
    { T::tryFrom(value) } -> std::same_as<std::optional<T>>;
};

/**
 * @brief Extracts a typed value from an optional Value.
 *
 * Domain wrapper types (Insets, Padding, Corners, LinearGradient, …) validate through their
 * own tryFrom. Variant member types (Numeric, Base::Color, std::string, Tuple) are returned
 * only when the value holds exactly that type. Either way, a value of the wrong shape yields
 * nothing, so callers keep their own default.
 */
template<HasTryFrom T>
std::optional<T> valueAs(const std::optional<Value>& value)
{
    if (!value) {
        return std::nullopt;
    }
    return T::tryFrom(*value);
}

template<typename T>
    requires(!HasTryFrom<T>)
std::optional<T> valueAs(const std::optional<Value>& value)
{
    if (!value) {
        return std::nullopt;
    }
    const T* held = value->tryGet<T>();
    return held ? std::optional<T>(*held) : std::nullopt;
}

}  // namespace Gui::StyleParameters

template<>
struct fmt::formatter<Gui::StyleParameters::Value>: fmt::formatter<std::string>
{
    auto format(const Gui::StyleParameters::Value& value, fmt::format_context& ctx) const
    {
        return fmt::formatter<std::string>::format(value.toString(), ctx);
    }
};

#endif  // STYLEPARAMETERS_VALUE_H
