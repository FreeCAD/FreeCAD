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

#include "ParameterManager.h"
#include "Parser.h"

#include <QFile>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <fmt/ranges.h>

#include <QRegularExpression>
#include <QString>
#include <ranges>
#include <utility>
#include <variant>

#include <Base/Console.h>

FC_LOG_LEVEL_INIT("Gui", true, true)

namespace Gui::StyleParameters
{

namespace
{

/// Converts a YAML node to a StyleParameters expression string.
/// Scalars are returned as-is; sequences become unnamed tuples "(a, b, ...)";
/// maps become named tuples "(key1: val1, key2: val2, ...)". Recursive.
std::string yamlNodeToExpression(const YAML::Node& node)
{
    if (node.IsScalar()) {
        return node.as<std::string>();
    }

    if (node.IsSequence()) {
        std::vector<std::string> parts;
        parts.reserve(node.size());
        for (const auto& element : node) {
            parts.push_back(yamlNodeToExpression(element));
        }
        return fmt::format("({})", fmt::join(parts, ", "));
    }

    if (node.IsMap()) {
        std::vector<std::string> parts;
        parts.reserve(node.size());
        for (auto it = node.begin(); it != node.end(); ++it) {
            parts.push_back(
                fmt::format("{}: {}", it->first.as<std::string>(), yamlNodeToExpression(it->second))
            );
        }
        return fmt::format("({})", fmt::join(parts, ", "));
    }

    return "";
}

/// Formats a Value for QSS output.
/// Tuples become space-separated values (e.g. "10px 5px 10px 5px"),
/// all other types delegate to toString().
std::string toQss(const Value& value)
{
    if (value.holds<Tuple>()) {
        const auto& tuple = value.get<Tuple>();

        std::vector<std::string> parts;
        parts.reserve(tuple.elements.size());

        for (const auto& [name, elem] : tuple.elements) {
            parts.push_back(toQss(*elem));
        }

        return fmt::format("{}", fmt::join(parts, " "));
    }

    return value.toString();
}

}  // namespace

ParameterSource::ParameterSource(const Metadata& metadata)
    : metadata(metadata)
{}

InMemoryParameterSource::InMemoryParameterSource(
    const std::list<Parameter>& parameters,
    const Metadata& metadata
)
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

YamlParameterSource::YamlParameterSource(const std::string& filePath, const Metadata& metadata)
    : ParameterSource(metadata)
{
    changeFilePath(filePath);
}

void YamlParameterSource::changeFilePath(const std::string& path)
{
    this->filePath = path;
    reload();
}

void YamlParameterSource::reload()
{
    QFile file(QString::fromStdString(filePath));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        FC_TRACE("StyleParameters: Unable to open file " << filePath);
        return;
    }

    if (filePath.starts_with(":/")) {
        this->metadata.options |= ReadOnly;
    }

    QTextStream in(&file);
    std::string content = in.readAll().toStdString();

    YAML::Node root = YAML::Load(content);
    parameters.clear();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const auto key = it->first.as<std::string>();
        const auto value = yamlNodeToExpression(it->second);

        parameters[key] = Parameter {
            .name = key,
            .value = value,
        };
    }
}

std::list<Parameter> YamlParameterSource::all() const
{
    std::list<Parameter> result;
    for (const auto& param : parameters | std::views::values) {
        result.push_back(param);
    }
    return result;
}

std::optional<Parameter> YamlParameterSource::get(const std::string& name) const
{
    if (auto it = parameters.find(name); it != parameters.end()) {
        return it->second;
    }

    return std::nullopt;
}

void YamlParameterSource::define(const Parameter& param)
{
    parameters[param.name] = param;
}

void YamlParameterSource::remove(const std::string& name)
{
    parameters.erase(name);
}

void YamlParameterSource::flush()
{
    YAML::Node root;
    for (const auto& [name, param] : parameters) {
        root[name] = param.value;
    }

    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        FC_WARN("StyleParameters: Unable to open file " << filePath);
        return;
    }

    QTextStream out(&file);
    out << QString::fromStdString(YAML::Dump(root));
}

ParameterManager::ParameterManager() = default;

void ParameterManager::reload()
{
    _resolved.clear();
}

std::string ParameterManager::replacePlaceholders(
    const std::string& expression,
    ResolveContext context
) const
{
    // Matches @TokenName (group name) or @{expression} (group expression)
    static const QRegularExpression regex("@(?:(?P<name>\\w+)|({(?P<expression>(?>[^{}]+|(?2))+)}))");

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
        [&](const QRegularExpressionMatch& match) -> QString {
            // Group 1: @TokenName
            if (!match.captured("name").isEmpty()) {
                auto tokenName = match.captured(1).toStdString();
                auto tokenValue = resolve(tokenName, context);

                if (!tokenValue) {
                    Base::Console().warning("Requested non-existent style parameter token '%s'.\n", tokenName);
                    return QStringLiteral("");
                }

                context.visited.erase(tokenName);
                return QString::fromStdString(toQss(*tokenValue));
            }

            // Group 2: @{expression}
            auto exprBody = match.captured("expression").toStdString();
            try {
                Value result = evaluate(exprBody, context);
                return QString::fromStdString(toQss(result));
            }
            catch (Base::Exception& e) {
                Base::Console().warning(
                    "Failed to evaluate inline expression '@{%s}': %s\n",
                    exprBody,
                    e.what()
                );
                return QStringLiteral("");
            }
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

std::optional<Value> ParameterManager::resolve(const std::string& name, ResolveContext context) const
{
    std::optional<Parameter> maybeParameter = this->parameter(name);

    if (!maybeParameter) {
        return std::nullopt;
    }

    if (context.visited.contains(name)) {
        Base::Console().warning("The style parameter '%s' contains circular-reference.\n", name);
        return expression(name);
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
