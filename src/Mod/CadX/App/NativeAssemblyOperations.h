// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphAudit.h"
#include "GraphStore.h"
#include "ToolResult.h"

#include <string>

namespace CadX
{

enum class CommitPublication
{
    Published,
    Reconciled,
    Failed,
};

// Publish the verified graph produced after a committed CAD mutation. If the
// optimistic publication loses a race, reconcile the committed snapshot as
// the current scope so callers never silently leave the graph at the old
// revision. A Reconciled result is intentionally reported as severe by the
// mutation coordinator because the optimistic invariant was violated.
CommitPublication publishCommittedGraph(GraphStore& graphs,
                                        const GraphScope& scope,
                                        std::shared_ptr<GraphSnapshot> snapshot,
                                        const std::string& expectedBaseRevision,
                                        std::string& diagnostic);

class NativeAssemblyOperations
{
public:
    NativeAssemblyOperations(GraphStore& graphs, GraphAuditLog& audit);

    ToolResult execute(const std::string& toolName, const std::string& argumentsJson) const;

private:
    GraphStore& _graphs;
    GraphAuditLog& _audit;
};

}  // namespace CadX
