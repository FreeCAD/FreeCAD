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

struct Length
{
    double value;
    std::string unit;

    Length operator+(const Length& rhs) const;
    Length operator-(const Length& rhs) const;
    Length operator-() const;

    Length operator/(const Length& rhs) const;
    Length operator*(const Length& rhs) const;

private:
    void ensureEqualUnits(const Length& rhs) const;
};

struct Value : std::variant<Length, QColor, std::string>
{
    using std::variant<Length, QColor, std::string>::variant;

    std::string toString() const;
};

enum class ParameterSource : std::uint8_t
{
    Predefined,
    Theme,
    User,
};

struct Token
{
    std::string name;
    std::string value;
    ParameterSource source = ParameterSource::User;
};

class ParameterManager
{
public:
    struct ResolveContext
    {
        std::set<std::string> visited;
    };

    ParameterManager();

    void reload();

    std::string replacePlaceholders(const std::string& expression, ResolveContext context = {}) const;

    std::ranges::view auto tokens() const
    {
        return std::ranges::views::keys(_tokens);
    }

    std::string expression(const std::string& name) const;

    Value resolve(const std::string& name, ResolveContext context = {}) const;
    Value evaluate(const std::string& expression, ResolveContext context = {}) const;

    Token token(const std::string& token) const;

    void remove(const Token& token);
    void set(const Token& token);

private:
    std::map<std::string, Token> _tokens;
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
