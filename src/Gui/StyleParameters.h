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
#include <Base/Bitmask.h>
#include <Base/Parameter.h>

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
 * @brief This struct represents any valid value that can be used as the parameter value.
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

class ParameterSource;

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
 * Parameter names must be unique and follow **CamelCase**.
 */
struct Parameter
{
    /// Comparator that assumes that parameters are equal as long as name is the same
    struct NameComparator
    {
        bool operator()(const Parameter& lhs, const Parameter& rhs) const
        {
            return lhs.name < rhs.name;
        }
    };

    /// Name of the parameter, name should follow CamelCase
    std::string name;
    /// Expression associated with the parameter
    std::string value;
};

enum class ParameterSourceOption
{
    // clang-format off
    /// Parameters are read-only and the source does not allow editing
    ReadOnly     = 1 << 0,
    /// Parameters are expected to be edited by the user, not only theme developers
    UserEditable = 1 << 1,
    // clang-format on
};

using ParameterSourceOptions = Base::Flags<ParameterSourceOption>;

/**
 * @brief Abstract base class representing a source of style parameters.
 *
 * A `ParameterSource` is responsible for managing a collection of named parameters. Each source
 * has metadata describing its type, characteristics, and behavior.
 *
 * ### Key Responsibilities
 * - Define, update, and remove parameters within the source.
 * - Provide access to all parameters or specific ones by name.
 * - Act as a backend for parameter management, feeding the `ParameterManager` with available
 *   parameter data.
 *
 * ### Metadata
 * Each parameter source includes metadata consisting of:
 * - `name`: Name of the source, for identification purposes.
 * - `options`: Flags specifying optional behavior (e.g., `ReadOnly`, `UserEditable`).
 *
 * ### Notes on Usage
 * - Subclasses of `ParameterSource` (e.g., `BuiltInParameterSource`, `UserParameterSource`)
 *   implement different storage mechanisms and behaviors based on whether parameters are
 *   pre-defined, user-defined, or loaded in memory.
 * - Parameters can be retrieved and manipulated globally through the `ParameterManager`, which
 *   aggregates multiple `ParameterSource` instances.
 *
 * #### Example
 * @code{.cpp}
 * // Create an in-memory parameter source
 * InMemoryParameterSource source({
 *     Parameter{ "BasePadding", "16px" },
 *     Parameter{ "DefaultColor", "#ff00ff" },
 * });
 *
 * source.define(Parameter{ "Margin", "4px" }); // Adds a new parameter
 *
 * auto padding = source.get("BasePadding"); // Retrieves parameter named "BasePadding"
 * auto parametersList = source.all();       // Retrieve all parameters
 * @endcode
 *
 * ### Subclass Requirements
 * Derived classes must implement:
 * - `all()` - Retrieve all parameters in the source.
 * - `get()` - Retrieve a specific parameter.
 * - `define()` - Add or update a parameter, can be left empty for readonly sources.
 * - `remove()` - Remove a parameter by name, can be left empty for readonly sources.
 */
class ParameterSource
{
public:
    using enum ParameterSourceOption;


    /**
     * @struct Metadata
     * @brief Contains metadata information about a `ParameterSource`.
     *
     * The `Metadata` struct provides a way to describe the characteristics and identity
     * of a `ParameterSource`. It includes a name for identification and a set of options
     * that define the source's behavior and restrictions.
     */
    struct Metadata
    {
        /// The name of the parameter source. Should be marked for translation using QT_TR_NOOP
        std::string name;
        /// Flags defining the behavior and properties of the parameter source.
        ParameterSourceOptions options;
    };

    /// Metadata of the parameter source
    Metadata metadata;

    explicit ParameterSource(const Metadata& metadata = {});
    virtual ~ParameterSource() = default;

    /**
     * @brief Retrieves a list of all parameters available in the source.
     *
     * This method returns every parameter defined within this source, enabling iteration and bulk
     * access to all stored values.
     *
     * @return A list containing all `Parameter` objects stored in the source.
     */
    virtual std::list<Parameter> all() const = 0;

    /**
     * @brief Retrieves a specific parameter by its name.
     *
     * @param[in] name The name of the parameter to retrieve.
     * @return An optional containing the requested parameter if it exists, or empty if not.
     */
    virtual std::optional<Parameter> get(const std::string& name) const = 0;

    /**
     * @brief Defines or updates a parameter in the source.
     *
     * Adds a new parameter to the source if it doesn't already exist, or updates the value of an
     * existing parameter with the same name.
     *
     * @param[in] parameter The `Parameter` object to define or update in the source.
     */
    virtual void define(const Parameter& parameter) {}

    /**
     * @brief Removes a parameter from the source by its name.
     *
     * Deletes the specific parameter from the source if it exists. If no parameter with the given
     * name is found, the method does nothing.
     *
     * @param[in] name The name of the parameter to remove.
     */
    virtual void remove(const std::string& name) {}
};

class InMemoryParameterSource : public ParameterSource
{
public:
    InMemoryParameterSource(const std::list<Parameter>& parameters, const Metadata& metadata = {});
    ~InMemoryParameterSource() override = default;

    std::list<Parameter> all() const override;
    std::optional<Parameter> get(const std::string& name) const override;

    void define(const Parameter& parameter) override;
    void remove(const std::string& name) override;

private:
    std::map<std::string, Parameter> parameters;
};

class BuiltInParameterSource : public ParameterSource
{
public:
    BuiltInParameterSource(const Metadata& metadata = {});
    ~BuiltInParameterSource() override = default;

    std::list<Parameter> all() const override;
    std::optional<Parameter> get(const std::string& name) const override;

private:
    ParameterGrp::handle hGrpThemes =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Themes");
    ParameterGrp::handle hGrpView =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    std::map<std::string, ParameterGrp::handle> params = {
        {"ThemeAccentColor1", hGrpThemes},
        {"ThemeAccentColor2", hGrpThemes},
        {"ThemeAccentColor3", hGrpThemes},
        {"BackgroundColor", hGrpView},
    };
};

class UserParameterSource : public ParameterSource
{
public:
    UserParameterSource(ParameterGrp::handle hGrp, const Metadata& metadata = {});
    ~UserParameterSource() override = default;

    std::list<Parameter> all() const override;
    std::optional<Parameter> get(const std::string& name) const override;

    void define(const Parameter& parameter) override;
    void remove(const std::string& name) override;

private:
    ParameterGrp::handle hGrp;
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
 * InMemoryParameterSource source({
 *     Parameter{ "BaseSize", "8px" },
 *     Parameter{ "Padding", "@BaseSize * 2" },
 * });
 *
 * ParameterManager manager;
 * manager.addSource(&source);
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

    /// Refreshes caches and reloads all parameters if necessary.
    void reload();

    /**
     * @brief Replaces placeholders in a given expression string with their resolved values.
     *
     * This method processes CSS-like expressions containing parameter placeholders, replaces
     * placeholders (written as `@ParameterName`) with their resolved values, and returns
     * the updated string. It is particularly useful for evaluating expressions that depend
     * on dynamically defined parameters while ensuring proper handling of references and
     * dependencies.
     *
     * @param[in] expression The input string containing placeholders to resolve.
     * @param[in] context (optional) A `ResolveContext` for tracking visited parameters, used to
     *
     * @return Original expression but with placeholders substituted with proper values.
     */
    std::string replacePlaceholders(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Returns a list of all parameters available in the manager.
     *
     * @return List of parameters available in the manager.
     */
    std::list<Parameter> parameters() const;

    /**
     * @brief Returns the raw expression string for a given parameter.
     *
     * @param[in] name The name of the parameter to retrieve.
     *
     * @return The associated expression string.
     */
    std::optional<std::string> expression(const std::string& name) const;

    /**
     * @brief Resolves the fully evaluated value of a parameter by name.
     *
     * @param[in] name Name of the parameter to resolve.
     * @param[in] context (optional) Resolution context to track visited parameters.
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
     * @param[in] expression A CSS-style expression string to evaluate.
     * @param[in] context (optional) Resolution context to track nested dependencies.
     *
     * @return The evaluated result of the expression.
     */
    Value evaluate(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Retrieves a full Parameter object by name.
     *
     * @param[in] parameter The name of the parameter to retrieve.
     *
     * @return The corresponding Parameter object.
     */
    std::optional<Parameter> parameter(const std::string& parameter) const;

    /**
     * @brief Adds a parameter source to the parameter manager.
     *
     * @param[in] source Pointer to the ParameterSource object to be added.
     */
    void addSource(ParameterSource* source);

    /**
     * @brief Lists currently used sources.
     *
     * @return Vector of currently used sources.
     */
    std::vector<ParameterSource*> sources() const;

private:
    /// List of parameter sources.
    std::vector<ParameterSource*> _sources;
    /// Cache of already-resolved parameter values.
    mutable std::map<std::string, Value> _resolved;
};

}  // namespace Gui::StyleParameters

ENABLE_BITMASK_OPERATORS(Gui::StyleParameters::ParameterSourceOption);

#endif  // STYLEPARAMETERS_H
