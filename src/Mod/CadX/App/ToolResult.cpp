// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ToolResult.h"

namespace CadX
{

namespace
{
std::string escape(const std::string& value)
{
    std::string result;
    for (char character : value) {
        switch (character) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            default: result += character; break;
        }
    }
    return result;
}
}  // namespace

ToolResult ToolResult::success(std::string schema, std::string payload)
{
    ToolResult result;
    result.ok = true;
    result.schemaVersion = std::move(schema);
    result.payloadJson = std::move(payload);
    return result;
}

ToolResult ToolResult::failure(std::string code, std::string messageValue, bool retryableValue)
{
    ToolResult result;
    result.errorCode = std::move(code);
    result.message = std::move(messageValue);
    result.retryable = retryableValue;
    return result;
}

std::string ToolResult::toJson() const
{
    if (ok) {
        return payloadJson;
    }
    return "{\"schema_version\":\"cadx.tool-result.v1\",\"ok\":false,\"error\":{\"code\":\""
        + escape(errorCode) + "\",\"message\":\"" + escape(message)
        + "\",\"retryable\":" + (retryable ? "true" : "false") + "}}";
}

}  // namespace CadX
