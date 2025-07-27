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

#include "PreCompiled.h"

#include "ParameterManager.h"
#include "Parser.h"

#ifndef _PreComp_
#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <ranges>
#include <variant>
#endif

namespace Gui::StyleParameters
{

Length Length::operator+(const Length& rhs) const
{
    ensureEqualUnits(rhs);
    return {value + rhs.value, unit};
}

Length Length::operator-(const Length& rhs) const
{
    ensureEqualUnits(rhs);
    return {value - rhs.value, unit};
}

Length Length::operator-() const
{
    return {-value, unit};
}

Length Length::operator/(const Length& rhs) const
{
    if (rhs.value == 0) {
        THROWM(Base::RuntimeError, "Division by zero");
    }

    if (rhs.unit.empty() || unit.empty()) {
        return {value / rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value / rhs.value, unit};
}

Length Length::operator*(const Length& rhs) const
{
    if (rhs.unit.empty() || unit.empty()) {
        return {value * rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value * rhs.value, unit};
}

void Length::ensureEqualUnits(const Length& rhs) const
{
    if (unit != rhs.unit) {
        THROWM(Base::RuntimeError,
               fmt::format("Units mismatch left expression is '{}', right expression is '{}'",
                           unit,
                           rhs.unit));
    }
}

std::string Value::toString() const
{
    if (std::holds_alternative<Length>(*this)) {
        auto [value, unit] = std::get<Length>(*this);
        return fmt::format("{}{}", value, unit);
    }

    if (std::holds_alternative<QColor>(*this)) {
        auto color = std::get<QColor>(*this);
        return fmt::format("#{:0>6x}", 0xFFFFFF & color.rgb());  // NOLINT(*-magic-numbers)
    }

    return std::get<std::string>(*this);
}

ParameterSource::ParameterSource(const Metadata& metadata)
    : metadata(metadata)
{}

InMemoryParameterSource::InMemoryParameterSource(const std::list<Parameter>& parameters,
                                                 const Metadata& metadata)
    : ParameterSource(metadata)
{
    for (const auto& parameter : parameters) {
        InMemoryParameterSource::define(parameter);
    }
}

std::list<Parameter> InMemoryParameterSource::all() const
{
    auto values = parameters | std::ranges::views::values;

    return std::list<Parameter>(values.begin(), values.end());
}

std::optional<Parameter> InMemoryParameterSource::get(const std::string& name) const
{
    if (parameters.contains(name)) {
        return parameters.at(name);
    }

    return std::nullopt;
}

void InMemoryParameterSource::define(const Parameter& parameter)
{
    parameters[parameter.name] = parameter;
}

void InMemoryParameterSource::remove(const std::string& name)
{
    parameters.erase(name);
}

BuiltInParameterSource::BuiltInParameterSource(const Metadata& metadata)
    : ParameterSource(metadata)
{
    this->metadata.options |= ReadOnly;
}

std::list<Parameter> BuiltInParameterSource::all() const
{
    std::list<Parameter> result;

    for (const auto& name : params | std::views::keys) {
        result.push_back(*get(name));
    }

    return result;
}

std::optional<Parameter> BuiltInParameterSource::get(const std::string& name) const
{
    if (params.contains(name)) {
        unsigned long color = params.at(name)->GetUnsigned(name.c_str(), 0);

        return Parameter {
            .name = name,
            .value = fmt::format("#{:0>6x}", 0x00FFFFFF & (color >> 8)),  // NOLINT(*-magic-numbers)
        };
    }

    return std::nullopt;
}

UserParameterSource::UserParameterSource(ParameterGrp::handle hGrp, const Metadata& metadata)
    : ParameterSource(metadata)
    , hGrp(hGrp)
{}

std::list<Parameter> UserParameterSource::all() const
{
    std::list<Parameter> result;

    for (const auto& [token, value] : hGrp->GetASCIIMap()) {
        result.push_back({
            .name = token,
            .value = value,
        });
    }

    return result;
}

std::optional<Parameter> UserParameterSource::get(const std::string& name) const
{
    if (auto value = hGrp->GetASCII(name.c_str(), ""); !value.empty()) {
        return Parameter {
            .name = name,
            .value = value,
        };
    }

    return {};
}

void UserParameterSource::define(const Parameter& parameter)
{
    hGrp->SetASCII(parameter.name.c_str(), parameter.value);
}

void UserParameterSource::remove(const std::string& name)
{
    hGrp->RemoveASCII(name.c_str());
}

ParameterManager::ParameterManager() = default;

void ParameterManager::reload()
{
    _resolved.clear();
}

std::string ParameterManager::replacePlaceholders(const std::string& expression,
                                                  ResolveContext context) const
{
    static const QRegularExpression regex(QStringLiteral("@(\\w+)"));

    auto substituteWithCallback =
        [](const QRegularExpression& regex,
           const QString& input,
           const std::function<QString(const QRegularExpressionMatch&)>& callback) {
            QRegularExpressionMatchIterator it = regex.globalMatch(input);

            QString result;
            qsizetype lastIndex = 0;

            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();

                qsizetype start = match.capturedStart();
                qsizetype end = match.capturedEnd();

                result += input.mid(lastIndex, start - lastIndex);
                result += callback(match);

                lastIndex = end;
            }

            // Append any remaining text after the last match
            result += input.mid(lastIndex);

            return result;
        };

    // clang-format off
    return substituteWithCallback(
        regex,
        QString::fromStdString(expression),
        [&](const QRegularExpressionMatch& match) {
            auto tokenName = match.captured(1).toStdString();

            auto tokenValue = resolve(tokenName, context);
            context.visited.erase(tokenName);

            return QString::fromStdString(tokenValue.toString());
        }
    ).toStdString();
    // clang-format on
}

std::list<Parameter> ParameterManager::parameters() const
{
    std::set<Parameter, Parameter::NameComparator> result;

    // we need to traverse it in reverse order so more important tokens will take precedence
    for (const ParameterSource* source : _sources | std::views::reverse) {
        for (const Parameter& parameter : source->all()) {
            result.insert(parameter);
        }
    }

    return std::list(result.begin(), result.end());
}

std::optional<std::string> ParameterManager::expression(const std::string& name) const
{
    if (auto param = parameter(name)) {
        return param->value;
    }

    return {};
}

Value ParameterManager::resolve(const std::string& name, ResolveContext context) const
{
    std::optional<Parameter> maybeParameter = this->parameter(name);

    if (!maybeParameter) {
        Base::Console().warning("Requested non-existent design token '%s'.", name);
        return std::string {};
    }

    if (context.visited.contains(name)) {
        Base::Console().warning("The design token '%s' contains circular-reference.", name);
        return expression(name).value_or(std::string {});
    }

    const Parameter& token = *maybeParameter;

    if (!_resolved.contains(token.name)) {
        context.visited.insert(token.name);
        try {
            _resolved[token.name] = evaluate(token.value, context);
        }
        catch (Base::Exception&) {
            // in case of being unable to parse it, we need to treat it as a generic value
            _resolved[token.name] = replacePlaceholders(token.value, context);
        }
        context.visited.erase(token.name);
    }

    return _resolved[token.name];
}

Value ParameterManager::evaluate(const std::string& expression, ResolveContext context) const
{
    Parser parser(expression);
    return parser.parse()->evaluate({.manager = this, .context = std::move(context)});
}

std::optional<Parameter> ParameterManager::parameter(const std::string& name) const
{
    for (const ParameterSource* source : _sources) {
        if (const auto& parameter = source->get(name)) {
            return parameter;
        }
    }

    return {};
}

void ParameterManager::addSource(ParameterSource* source)
{
    _sources.push_front(source);
}

std::list<ParameterSource*> ParameterManager::sources() const
{
    return _sources;
}

}  // namespace Gui::StyleParameters