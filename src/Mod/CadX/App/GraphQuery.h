// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphSnapshot.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace CadX
{

enum class QueryOperation
{
    Summary,
    FindNodes,
    Neighbors,
    Subgraph,
    ShortestPath,
};

enum class QueryDirection
{
    Incoming,
    Outgoing,
    Both,
};

struct QueryRequest
{
    std::string graphId;
    std::string graphRevision;
    QueryOperation operation = QueryOperation::Summary;
    std::size_t limit = 50;
    std::size_t maxDepth = 0;
    QueryDirection direction = QueryDirection::Both;
    std::vector<NodeKind> nodeKinds;
    std::vector<EdgeKind> edgeKinds;
    std::vector<NodeId> startNodeIds;
    NodeId startNodeId;
    NodeId targetNodeId;
    std::string nativeType;
    std::string label;
    bool labelContains = false;
    std::string semanticPartKind;
    std::string sourceDocumentUid;
    bool filterVisible = false;
    bool visible = false;
    std::string cursor;
};

struct QueryResult
{
    bool ok = true;
    std::string errorCode;
    std::string diagnostic;
    std::vector<NodeId> nodeIds;
    std::vector<EdgeId> edgeIds;
    std::size_t returnedNodeCount = 0;
    std::size_t returnedEdgeCount = 0;
    bool truncated = false;
    std::string nextCursor;
};

class GraphQueryEngine
{
public:
    QueryResult execute(const std::shared_ptr<const GraphSnapshot>& snapshot,
                        const QueryRequest& request) const;

private:
    static bool edgeAllowed(const EdgeRecord& edge, const QueryRequest& request);
    static void sortNodeIds(const GraphSnapshot& snapshot, std::vector<NodeId>& ids);
    static QueryResult invalid(const char* code, const char* message);
};

}  // namespace CadX
