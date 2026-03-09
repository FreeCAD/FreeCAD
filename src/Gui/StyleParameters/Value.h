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

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <Base/Color.h>
#include <Base/Exception.h>
#include <FCGlobal.h>

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
    double value;
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
    };

    std::vector<Element> elements;

    /**
     * @brief Returns the value at the given index (bounds-checked).
     * @throws Base::RuntimeError if index is out of range.
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
     * @brief Gets a named element with type checking and user-friendly error messages.
     *
     * @throws Base::ExpressionError if the element is not found or has the wrong type.
     */
    template<typename T>
    const T& get(const std::string& name) const;
};

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
     * @return A string representation of the object that can later be used in QSS.
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
     * @brief Gets the value of the given type.
     *
     * Throws std::bad_variant_access if the value does not hold the requested type.
     */
    template<typename T>
    const T& get() const
    {
        return std::get<T>(*this);
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

template<typename T>
const T& Tuple::get(const std::string& name) const
{
    const Value* value = find(name);

    if (!value) {
        THROWM(Base::ExpressionError, fmt::format("Missing argument '{}'", name));
    }

    if (!value->holds<T>()) {
        THROWM(
            Base::ExpressionError,
            fmt::format("Argument '{}' must be {}, got {}", name, valueTypeName<T>(), value->toString())
        );
    }

    return value->get<T>();
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

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_VALUE_H
