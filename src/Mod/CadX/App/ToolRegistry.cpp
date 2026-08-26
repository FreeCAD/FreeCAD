// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ToolRegistry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>
#include <regex>

namespace CadX
{
namespace
{

bool sameJson(const QJsonValue& left, const QJsonValue& right)
{
    return QJsonDocument(left.toObject()).toJson(QJsonDocument::Compact)
               == QJsonDocument(right.toObject()).toJson(QJsonDocument::Compact)
        && left == right;
}

bool validateSchema(const QJsonValue& value,
                    const QJsonObject& schema,
                    const QJsonObject& definitions,
                    const std::string& path,
                    std::string& diagnostic)
{
    if (schema.contains("$ref")) {
        const auto reference = schema.value("$ref");
        const auto prefix = QStringLiteral("#/$defs/");
        if (!reference.isString() || !reference.toString().startsWith(prefix)) {
            diagnostic = path + ": unsupported schema reference";
            return false;
        }
        const auto definition = definitions.value(reference.toString().mid(prefix.size()));
        if (!definition.isObject()) {
            diagnostic = path + ": unresolved schema reference";
            return false;
        }
        return validateSchema(value, definition.toObject(), definitions, path, diagnostic);
    }

    if (schema.value("oneOf").isArray()) {
        std::string branchDiagnostic;
        for (const auto& branch : schema.value("oneOf").toArray()) {
            if (branch.isObject()
                && validateSchema(value, branch.toObject(), definitions, path, branchDiagnostic)) {
                return true;
            }
        }
        diagnostic = path + ": value does not match any allowed schema variant";
        return false;
    }

    if (schema.contains("const") && value != schema.value("const")) {
        diagnostic = path + ": value does not match the required constant";
        return false;
    }
    if (schema.value("enum").isArray()) {
        bool matched = false;
        for (const auto& allowed : schema.value("enum").toArray()) {
            if (value == allowed) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            diagnostic = path + ": value is not in the allowed set";
            return false;
        }
    }

    const auto type = schema.value("type").toString();
    if (type == "object") {
        if (!value.isObject()) {
            diagnostic = path + ": expected an object";
            return false;
        }
        const auto object = value.toObject();
        const auto properties = schema.value("properties").toObject();
        const auto required = schema.value("required").toArray();
        for (const auto& requiredValue : required) {
            if (!requiredValue.isString() || !object.contains(requiredValue.toString())) {
                diagnostic = path + ": missing required property";
                return false;
            }
        }
        if (schema.value("additionalProperties").isBool()
            && !schema.value("additionalProperties").toBool()) {
            for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
                if (!properties.contains(iterator.key())) {
                    diagnostic = path + ": unexpected property '" + iterator.key().toStdString() + "'";
                    return false;
                }
            }
        }
        for (auto iterator = properties.begin(); iterator != properties.end(); ++iterator) {
            if (!object.contains(iterator.key()) || !iterator.value().isObject()) {
                continue;
            }
            if (!validateSchema(object.value(iterator.key()),
                                iterator.value().toObject(),
                                definitions,
                                path + "." + iterator.key().toStdString(),
                                diagnostic)) {
                return false;
            }
        }
        return true;
    }
    if (type == "array") {
        if (!value.isArray()) {
            diagnostic = path + ": expected an array";
            return false;
        }
        const auto array = value.toArray();
        const auto minimum = schema.value("minItems");
        const auto maximum = schema.value("maxItems");
        if (minimum.isDouble() && array.size() < minimum.toInt()) {
            diagnostic = path + ": too few items";
            return false;
        }
        if (maximum.isDouble() && array.size() > maximum.toInt()) {
            diagnostic = path + ": too many items";
            return false;
        }
        if (schema.value("uniqueItems").toBool()) {
            for (qsizetype left = 0; left < array.size(); ++left) {
                for (qsizetype right = left + 1; right < array.size(); ++right) {
                    if (sameJson(array.at(left), array.at(right))) {
                        diagnostic = path + ": items must be unique";
                        return false;
                    }
                }
            }
        }
        const auto itemSchema = schema.value("items");
        if (itemSchema.isObject()) {
            for (qsizetype index = 0; index < array.size(); ++index) {
                if (!validateSchema(array.at(index),
                                    itemSchema.toObject(),
                                    definitions,
                                    path + "[" + std::to_string(index) + "]",
                                    diagnostic)) {
                    return false;
                }
            }
        }
        return true;
    }
    if (type == "string") {
        if (!value.isString()) {
            diagnostic = path + ": expected a string";
            return false;
        }
        const auto length = value.toString().size();
        if (schema.value("minLength").isDouble()
            && length < schema.value("minLength").toInt()) {
            diagnostic = path + ": string is too short";
            return false;
        }
        if (schema.value("maxLength").isDouble()
            && length > schema.value("maxLength").toInt()) {
            diagnostic = path + ": string is too long";
            return false;
        }
        return true;
    }
    if (type == "boolean") {
        if (!value.isBool()) {
            diagnostic = path + ": expected a boolean";
            return false;
        }
        return true;
    }
    if (type == "number" || type == "integer") {
        if (!value.isDouble() || !std::isfinite(value.toDouble())
            || (type == "integer" && std::floor(value.toDouble()) != value.toDouble())) {
            diagnostic = path + ": expected a finite " + type.toStdString();
            return false;
        }
        const auto number = value.toDouble();
        if (schema.value("minimum").isDouble() && number < schema.value("minimum").toDouble()) {
            diagnostic = path + ": number is below the minimum";
            return false;
        }
        if (schema.value("exclusiveMinimum").isDouble()
            && number <= schema.value("exclusiveMinimum").toDouble()) {
            diagnostic = path + ": number is not above the exclusive minimum";
            return false;
        }
        if (schema.value("maximum").isDouble() && number > schema.value("maximum").toDouble()) {
            diagnostic = path + ": number is above the maximum";
            return false;
        }
        if (schema.value("exclusiveMaximum").isDouble()
            && number >= schema.value("exclusiveMaximum").toDouble()) {
            diagnostic = path + ": number is not below the exclusive maximum";
            return false;
        }
        return true;
    }
    return true;
}

bool validateArguments(const std::string& schemaJson,
                       const std::string& argumentsJson,
                       std::string& diagnostic)
{
    QJsonParseError schemaError;
    const auto schemaDocument = QJsonDocument::fromJson(
        QByteArray::fromStdString(schemaJson), &schemaError);
    if (schemaError.error != QJsonParseError::NoError || !schemaDocument.isObject()) {
        diagnostic = "tool schema is not a JSON object";
        return false;
    }
    QJsonParseError argumentsError;
    const auto argumentsDocument = QJsonDocument::fromJson(
        QByteArray::fromStdString(argumentsJson), &argumentsError);
    if (argumentsError.error != QJsonParseError::NoError || !argumentsDocument.isObject()) {
        diagnostic = "tool arguments must be a JSON object";
        return false;
    }
    const auto schema = schemaDocument.object();
    return validateSchema(argumentsDocument.object(),
                          schema,
                          schema.value("$defs").toObject(),
                          "$",
                          diagnostic);
}

}  // namespace

bool ToolRegistry::registerDefinition(ToolDefinition definition, std::string& diagnostic)
{
    static const std::regex namePattern("[a-z][a-z0-9_.-]{2,127}");
    if (!std::regex_match(definition.name, namePattern) || definition.description.empty()
        || definition.inputSchemaJson.empty() || definition.outputSchemaVersion.empty()
        || !definition.executor || definition.resultSizeLimit == 0) {
        diagnostic = "malformed tool definition";
        return false;
    }
    QJsonParseError schemaError;
    const auto schema = QJsonDocument::fromJson(
        QByteArray::fromStdString(definition.inputSchemaJson), &schemaError);
    if (schemaError.error != QJsonParseError::NoError || !schema.isObject()) {
        diagnostic = "tool schema must be a JSON object";
        return false;
    }
    std::lock_guard lock(_mutex);
    if (_definitions.contains(definition.name)) {
        diagnostic = "duplicate tool name";
        return false;
    }
    _definitions.emplace(definition.name, std::move(definition));
    return true;
}

ToolResult ToolRegistry::execute(const std::string& name, const std::string& argumentsJson) const
{
    ToolDefinition definition;
    {
        std::lock_guard lock(_mutex);
        const auto iterator = _definitions.find(name);
        if (iterator == _definitions.end()) {
            return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", "unknown tool");
        }
        definition = iterator->second;
    }
    if (argumentsJson.size() > 64 * 1024) {
        return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", "tool arguments exceed the byte limit");
    }
    std::string validationDiagnostic;
    if (!validateArguments(definition.inputSchemaJson, argumentsJson, validationDiagnostic)) {
        return ToolResult::failure("CADX_TOOL_ARGUMENTS_INVALID", validationDiagnostic);
    }
    ToolResult result;
    ThreadDispatcher dispatcher;
    {
        std::lock_guard lock(_mutex);
        dispatcher = _dispatcher;
    }
    if (dispatcher && definition.threadRequirement == ThreadRequirement::MainThread) {
        dispatcher(definition.threadRequirement, [&]() { result = definition.executor(argumentsJson); });
    }
    else {
        result = definition.executor(argumentsJson);
    }
    if (result.ok && result.toJson().size() > definition.resultSizeLimit) {
        return ToolResult::failure("CADX_QUERY_RESULT_TOO_LARGE", "tool result exceeds the byte limit", true);
    }
    return result;
}

void ToolRegistry::setThreadDispatcher(ThreadDispatcher dispatcher)
{
    std::lock_guard lock(_mutex);
    _dispatcher = std::move(dispatcher);
}

std::vector<ToolDefinition> ToolRegistry::definitions() const
{
    std::lock_guard lock(_mutex);
    std::vector<ToolDefinition> result;
    result.reserve(_definitions.size());
    for (const auto& [name, definition] : _definitions) {
        result.push_back(definition);
    }
    return result;
}

}  // namespace CadX
