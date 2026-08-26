// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ToolDefinition.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace CadX
{

class ToolRegistry
{
public:
    using ThreadDispatcher = std::function<void(ThreadRequirement, std::function<void()>)>;
    bool registerDefinition(ToolDefinition definition, std::string& diagnostic);
    ToolResult execute(const std::string& name, const std::string& argumentsJson) const;
    std::vector<ToolDefinition> definitions() const;
    void setThreadDispatcher(ThreadDispatcher dispatcher);

private:
    mutable std::mutex _mutex;
    std::map<std::string, ToolDefinition> _definitions;
    ThreadDispatcher _dispatcher;
};

}  // namespace CadX
