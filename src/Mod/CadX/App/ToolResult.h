// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

namespace CadX
{

struct ToolResult
{
    bool ok = false;
    std::string schemaVersion = "cadx.tool-result.v1";
    std::string payloadJson = "{}";
    std::string errorCode;
    std::string message;
    bool retryable = false;

    static ToolResult success(std::string schema, std::string payload);
    static ToolResult failure(std::string code, std::string message, bool retryable = false);
    std::string toJson() const;
};

}  // namespace CadX
