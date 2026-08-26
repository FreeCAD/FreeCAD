// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ToolResult.h"

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace CadX
{

enum class ToolClassification
{
    Read,
    Mutation,
    Presentation,
};

enum class ThreadRequirement
{
    Any,
    MainThread,
    Worker,
};

using ToolExecutor = std::function<ToolResult(const std::string& argumentsJson)>;

struct ToolDefinition
{
    std::string name;
    std::string description;
    ToolClassification classification = ToolClassification::Read;
    std::string inputSchemaJson;
    std::string outputSchemaVersion;
    ToolExecutor executor;
    ThreadRequirement threadRequirement = ThreadRequirement::Any;
    std::size_t resultSizeLimit = 128 * 1024;
};

}  // namespace CadX
