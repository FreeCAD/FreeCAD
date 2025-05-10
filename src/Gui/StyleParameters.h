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

#ifndef STYLEPARAMETERS_H
#define STYLEPARAMETERS_H

#include <QColor>
#include <QModelIndex>
#include <ranges>

#include <App/Application.h>
#include <Base/Parameter.h>

namespace Gui::StyleParameters
{

/**
 * @struct Length
 *
 * @brief Represents a length in a specified unit.
 *
 * This struct is a very simplified representation of lengths that can be used as parameters for
 * styling purposes. The length basically consists of value and unit. Unit is optional, empty unit
 * represents a dimensionless length that can be used as a scalar. This struct does not care about
 * unit conversions as its uses do not require it.
 */
struct Length
{
    /// Numeric value of the length.
    double value;
    /// Unit of the length, empty if the value is dimensionless.
    std::string unit;

    /**
     * @name Operators
     *
     * This struct supports basic operations on Length. Each operation requires for operands to be
     * the same unit. Multiplication and division additionally allow one operand to be dimensionless
     * and hence act as a scalar.
     *
     * @code{c++}
     * Length a { 10, "px" };
     * Length b { 5, "px" };
     *
     * Length differentUnit { 3, "rem" }
     * Length scalar { 2, "" };
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
    Length operator+(const Length& rhs) const;
    Length operator-(const Length& rhs) const;
    Length operator-() const;

    Length operator/(const Length& rhs) const;
    Length operator*(const Length& rhs) const;
    /// @}

private:
    void ensureEqualUnits(const Length& rhs) const;
};

/**
 * @struct Value
 *
 * This struct represents any valid value that can be used as the parameter value.
 *
 * The value can be one of three basic types:
 *  - Numbers / Lengths (so any length with optional unit) (Length)
 *  - Colors (QColor)
 *  - Any other generic expression. (std::string)
 *
 * As a rule, operations can be only performed over values of the same type.
 */
struct Value : std::variant<Length, QColor, std::string>
{
    using std::variant<Length, QColor, std::string>::variant;

    /**
     * Converts the object into its string representation.
     *
     * @return A string representation of the object that can later be used in QSS.
     */
    std::string toString() const;
};

enum class ParameterSource : std::uint8_t
{
    Predefined,
    Theme,
    User,
};

/**
 * @struct Parameter
 *
 * @brief Represents a named, dynamic expression-based parameter.
 *
 * The Parameter structure is used to define reusable named variables in styling or layout systems.
 * Each parameter consists of a `name` and a `value` string, where the value is a CSS-like expression
 * that supports numbers, units, arithmetic, colors, functions, and parameter references.
 *
 * ### Naming Convention
 * - Parameter names must be unique and follow **CamelCase**.
 * - To reference another parameter in expressions, prefix it with `@` (e.g., `@PrimaryColor`).
 */
struct Parameter
{
    std::string name;
    std::string value;
    ParameterSource source = ParameterSource::User;
};

/**
 * @class ParameterManager
 * @brief Manages a collection of named, expression-based parameters.
 *
 * The ParameterManager provides an interface to register, update, resolve, and evaluate
 * named parameters whose values are defined by expression strings.
 * Expressions can reference other parameters and involve arithmetic, units, colors, and functions.
 *
 * This class ensures correct resolution order, handles circular dependency detection,
 * and caches resolved results for performance.
 *
 * ### Usage Example
 * @code
 * ParameterManager manager;
 *
 * manager.define(Parameter{ "BaseSize", "8px" });
 * manager.define(Parameter{ "Padding", "@BaseSize * 2" });
 *
 * Value result = manager.resolve("Padding"); // will be Length { 16, "px" }
 * @endcode
 */
class ParameterManager
{
public:
    /**
     * @struct ResolveContext
     * @brief Tracks all metadata related to resolving parameters.
     */
    struct ResolveContext
    {
        /// Names of parameters currently being resolved.
        std::set<std::string> visited;
    };

    ParameterManager();

    void reload();

    std::string replacePlaceholders(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Returns a view of all parameter names currently stored.
     *
     * @return A range view over the keys (names) of registered parameters.
     */
    std::ranges::view auto parameters() const
    {
        return std::ranges::views::keys(_parameters);
    }

    /**
     * @brief Returns the raw expression string for a given parameter.
     *
     * @param name The name of the parameter to retrieve.
     *
     * @return The associated expression string.
     *
     * @throws std::out_of_range if the parameter does not exist.
     */
    std::string expression(const std::string& name) const;

    /**
     * @brief Resolves the fully evaluated value of a parameter by name.
     *
     * @param name Name of the parameter to resolve.
     * @param context (optional) Resolution context to track visited parameters.
     *
     * @return The evaluated value of the parameter.
     *
     * @throws std::runtime_error on circular references or evaluation errors.
     * @throws std::out_of_range if the parameter does not exist.
     */
    Value resolve(const std::string& name, ResolveContext context = {}) const;

    /**
     * @brief Evaluates an arbitrary expression string in the current parameter context.
     *
     * Parameter references within the expression will be resolved.
     *
     * ### Supported Expression Features
     *
     * - **Numbers and Units**:
     *   - Supports `px`, `em`, `rem`, `%`
     *   - Examples: `10px`, `2 * (8em + 2em)`
     *
     * - **Arithmetic**:
     *   - Operators: `+`, `-`, `*`, `/`
     *   - Parentheses supported for grouping
     *   - Example: `(@BaseSize + 4px) * 2`
     *
     * - **Colors**:
     *   - Hex: `#rrggbb`
     *   - RGB: `rgb(255, 255, 255)`
     *   - RGBA: `rgba(255, 255, 255, 128)`
     *
     * - **Color Functions**:
     *   - Supported functions: `darken(color, amount)`, `lighten(color, amount)`
     *   - Example: `darken(#3366ff, 0.15)`
     *
     * - **Parameter References**:
     *   - Use `@ParameterName` to refer to another parameter
     *   - Example: `@PrimaryPadding * 2`, `darken(@BackgroundColor, 0.15)`
     *
     * ### Limitations
     * - Only operations between the same units are allowed (e.g., `10px + 2px` is valid, `10px + 1em` is not)
     * - Arithmetic on colors is not supported (e.g., `#ffffff + #000000` is invalid)
     * - Division by zero results in an error
     *
     * ### Examples
     * @code
     * evaluate("8px + 4px"); // → 12px
     *
     * define(Parameter{ "BaseSize", "8px" });
     * evaluate("@BaseSize * 2") // → 16px
     *
     * evaluate("darken(#ff00ff, 0.2)"); //
     *
     * evaluate("(2 + 3) * 4"); // → 20
     * @endcode
     *
     * @param expression A CSS-style expression string to evaluate.
     * @param context (optional) Resolution context to track nested dependencies.
     *
     * @return The evaluated result of the expression.
     */
    Value evaluate(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Retrieves a full Parameter object by name.
     *
     * @param parameter The name of the parameter to retrieve.
     *
     * @return The corresponding Parameter object.
     *
     * @throws std::out_of_range if the parameter does not exist.
     */
    Parameter parameter(const std::string& parameter) const;

    /**
     * @brief Removes a parameter from the manager.
     * @param parameter The parameter object to remove.
     */
    void remove(const Parameter& parameter);

    /**
     * @brief Adds or redefines a parameter in the manager.
     *
     * If the parameter already exists, it will be overwritten.
     * @param parameter The parameter to define or update.
     */
    void define(const Parameter& parameter);

private:
    /// Map of parameter names to definitions.
    std::map<std::string, Parameter> _parameters;
    /// Cache of already-resolved parameter values.
    mutable std::map<std::string, Value> _resolved;

    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");
    ParameterGrp::handle hUserTokensGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes/UserTokens");
    ParameterGrp::handle hThemeTokensGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes/Tokens");
};

}  // namespace Gui::StyleParameters

#endif  // STYLEPARAMETERS_H
