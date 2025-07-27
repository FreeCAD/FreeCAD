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

#ifndef STYLEPARAMETERS_PARAMETERMANAGER_H
#define STYLEPARAMETERS_PARAMETERMANAGER_H

#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <QColor>

#include <App/Application.h>
#include <Base/Bitmask.h>
#include <Base/Parameter.h>

namespace Gui::StyleParameters
{

// Forward declaration for Parser
class Parser;

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
class GuiExport ParameterSource
{
public:
    using enum ParameterSourceOption;

    /**
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
        ParameterSourceOptions options {};
    };

    /// Metadata of the parameter source
    Metadata metadata;

    FC_DEFAULT_MOVE(ParameterSource);
    FC_DISABLE_COPY(ParameterSource);

    explicit ParameterSource(const Metadata& metadata);
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
     * Adds a new parameter to the source if it doesn't already exist or updates the value of an
     * existing parameter with the same name.
     *
     * @param[in] parameter The `Parameter` object to define or update in the source.
     */
    virtual void define([[maybe_unused]] const Parameter& parameter) {}

    /**
     * @brief Removes a parameter from the source by its name.
     *
     * Deletes the specific parameter from the source if it exists. If no parameter with the given
     * name is found, the method does nothing.
     *
     * @param[in] name The name of the parameter to remove.
     */
    virtual void remove([[maybe_unused]] const std::string& name) {}
};

/**
 * @brief In-memory parameter source that stores parameters in a map.
 *
 * This source is useful for temporary parameter storage or when you need to
 * define parameters programmatically without persisting them to disk.
 */
class GuiExport InMemoryParameterSource : public ParameterSource
{
    std::map<std::string, Parameter> parameters;

public:
    InMemoryParameterSource(const std::list<Parameter>& parameters, const Metadata& metadata);

    std::list<Parameter> all() const override;
    std::optional<Parameter> get(const std::string& name) const override;
    void define(const Parameter& parameter) override;
    void remove(const std::string& name) override;
};

/**
 * @brief Built-in parameter source that reads from FreeCAD's parameter system.
 *
 * This source provides access to predefined parameters that are stored in
 * FreeCAD's global parameter system. These parameters are typically defined
 * by the application and are read-only.
 */
class GuiExport BuiltInParameterSource : public ParameterSource
{
public:
    explicit BuiltInParameterSource(const Metadata& metadata = {});

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

/**
 * @brief User-defined parameter source that reads from user preferences.
 *
 * This source provides access to user-defined parameters that are stored
 * in the user's preference file. These parameters can be modified by the
 * user and persist across application sessions.
 */
class GuiExport UserParameterSource : public ParameterSource
{
    ParameterGrp::handle hGrp;

public:
    UserParameterSource(ParameterGrp::handle hGrp, const Metadata& metadata);

    std::list<Parameter> all() const override;
    std::optional<Parameter> get(const std::string& name) const override;
    void define(const Parameter& parameter) override;
    void remove(const std::string& name) override;
};

/**
 * @brief Central manager for style parameters that aggregates multiple sources.
 *
 * The ParameterManager is responsible for:
 * - Managing multiple parameter sources
 * - Resolving parameter references and expressions
 * - Caching resolved values for performance
 * - Handling circular references
 */
class GuiExport ParameterManager
{
    std::list<ParameterSource*> _sources;
    mutable std::map<std::string, Value> _resolved;

public:
    struct ResolveContext
    {
        /// Names of parameters currently being resolved.
        std::set<std::string> visited;
    };

    ParameterManager();

    /**
     * @brief Clears the internal cache of resolved values.
     *
     * Call this method when parameters have been modified to ensure
     * that the changes are reflected in subsequent resolutions.
     */
    void reload();

    /**
     * @brief Replaces parameter placeholders in a string with their resolved values.
     *
     * This method performs simple string substitution of @parameter references
     * with their actual values. It does not evaluate expressions, only performs
     * direct substitution.
     *
     * @param expression The string containing parameter placeholders
     * @param context Resolution context for handling circular references
     * @return The string with all placeholders replaced
     */
    std::string replacePlaceholders(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Returns all available parameters from all sources.
     *
     * Parameters are returned in order of source priority, with later sources
     * taking precedence over earlier ones.
     *
     * @return List of all available parameters
     */
    std::list<Parameter> parameters() const;

    /**
     * @brief Gets the raw expression string for a parameter.
     *
     * @param name The name of the parameter
     * @return The expression string if the parameter exists, empty otherwise
     */
    std::optional<std::string> expression(const std::string& name) const;

    /**
     * @brief Resolves a parameter to its final value.
     *
     * This method evaluates the parameter's expression and returns the computed
     * value. The result is cached for subsequent calls.
     *
     * @param name The name of the parameter to resolve
     * @param context Resolution context for handling circular references
     * @return The resolved value
     */
    Value resolve(const std::string& name, ResolveContext context = {}) const;

    /**
     * @brief Evaluates an expression string and returns the result.
     *
     * @param expression The expression to evaluate
     * @param context Resolution context for handling circular references
     * @return The evaluated value
     */
    Value evaluate(const std::string& expression, ResolveContext context = {}) const;

    /**
     * @brief Gets a parameter by name from any source.
     *
     * @param name The name of the parameter
     * @return The parameter if found, empty otherwise
     */
    std::optional<Parameter> parameter(const std::string& name) const;

    /**
     * @brief Adds a parameter source to the manager.
     *
     * Sources are evaluated in the order they are added, with later sources
     * taking precedence over earlier ones.
     *
     * @param source The parameter source to add
     */
    void addSource(ParameterSource* source);

    /**
     * @brief Returns all registered parameter sources.
     *
     * @return List of parameter sources in order of registration
     */
    std::list<ParameterSource*> sources() const;
};

}  // namespace Gui::StyleParameters

ENABLE_BITMASK_OPERATORS(Gui::StyleParameters::ParameterSourceOption);

#endif  // STYLEPARAMETERS_PARAMETERMANAGER_H 