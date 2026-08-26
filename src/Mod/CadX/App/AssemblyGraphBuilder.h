// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "AssemblyCapture.h"
#include "GraphSnapshot.h"

#include <memory>
#include <string>

namespace CadX
{

struct GraphBuildResult
{
    std::shared_ptr<GraphSnapshot> snapshot;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept { return snapshot != nullptr && errorCode.empty(); }
};

class AssemblyGraphBuilder
{
public:
    GraphBuildResult build(const AssemblyCapture& capture) const;

private:
    static NodeId stableNodeId(const NodeRecord& node, const std::string& role);
    static EdgeId stableEdgeId(const EdgeRecord& edge);
};

}  // namespace CadX
