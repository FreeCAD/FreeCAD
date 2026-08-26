// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphQuery.h"
#include "GraphRevision.h"

#include <algorithm>
#include <deque>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace CadX
{
namespace
{
std::string queryKey(const QueryRequest& request)
{
    std::ostringstream key;
    key << static_cast<int>(request.operation) << '|' << request.limit << '|'
        << static_cast<int>(request.direction) << '|' << request.nativeType << '|'
        << request.label << '|' << request.labelContains << '|' << request.semanticPartKind
        << '|' << request.sourceDocumentUid << '|' << request.filterVisible << '|'
        << request.visible << '|' << request.startNodeId << '|' << request.targetNodeId << '|'
        << request.maxDepth;
    for (const auto kind : request.nodeKinds) {
        key << "|n:" << static_cast<int>(kind);
    }
    for (const auto kind : request.edgeKinds) {
        key << "|e:" << static_cast<int>(kind);
    }
    for (const auto& id : request.startNodeIds) {
        key << "|s:" << id;
    }
    return key.str();
}

std::string makeCursor(const GraphSnapshot& snapshot,
                       const QueryRequest& request,
                       std::size_t offset)
{
    return "cadx.cursor.v1|" + snapshot.header().graphRevision + "|"
        + sha256Revision(queryKey(request)) + "|" + std::to_string(offset);
}

bool parseCursor(const GraphSnapshot& snapshot,
                 const QueryRequest& request,
                 std::size_t& offset)
{
    const auto first = request.cursor.find('|');
    const auto second = request.cursor.find('|', first + 1);
    const auto third = request.cursor.find('|', second + 1);
    if (first == std::string::npos || second == std::string::npos
        || third == std::string::npos || request.cursor.find('|', third + 1) != std::string::npos) {
        return false;
    }
    if (request.cursor.substr(0, first) != "cadx.cursor.v1"
        || request.cursor.substr(first + 1, second - first - 1) != snapshot.header().graphRevision
        || request.cursor.substr(second + 1, third - second - 1) != sha256Revision(queryKey(request))) {
        return false;
    }
    try {
        const auto value = std::stoull(request.cursor.substr(third + 1));
        offset = static_cast<std::size_t>(value);
    }
    catch (...) {
        return false;
    }
    return true;
}
}  // namespace

QueryResult GraphQueryEngine::execute(const std::shared_ptr<const GraphSnapshot>& snapshot,
                                      const QueryRequest& request) const
{
    if (!snapshot) {
        return invalid("CADX_GRAPH_NOT_FOUND", "the requested graph is not available");
    }
    if (request.graphId != snapshot->header().graphId) {
        return invalid("CADX_GRAPH_NOT_FOUND", "the graph handle is unknown");
    }
    if (request.graphRevision != snapshot->header().graphRevision) {
        return invalid("CADX_GRAPH_REVISION_MISMATCH", "the requested revision is not current");
    }
    if (request.limit == 0 || request.limit > 100 || request.maxDepth > 4
        || request.startNodeIds.size() > 16 || request.nodeKinds.size() > 16
        || request.edgeKinds.size() > 16) {
        return invalid("CADX_QUERY_INVALID", "the query exceeds a bounded query limit");
    }
    std::size_t cursorOffset = 0;
    if (!request.cursor.empty()
        && (request.operation != QueryOperation::FindNodes || !parseCursor(*snapshot, request, cursorOffset))) {
        return invalid("CADX_QUERY_CURSOR_INVALID", "cursor is invalid for this graph query");
    }

    QueryResult result;
    if (request.operation == QueryOperation::Summary) {
        return result;
    }

    if (request.operation == QueryOperation::FindNodes) {
        std::vector<NodeId> candidates;
        if (request.nodeKinds.empty()) {
            candidates.reserve(snapshot->nodes().size());
            for (const auto& node : snapshot->nodes()) {
                candidates.push_back(node.id);
            }
        }
        else {
            for (auto kind : request.nodeKinds) {
                for (auto index : snapshot->nodesByKind(kind)) {
                    candidates.push_back(snapshot->nodes()[index].id);
                }
            }
        }
        sortNodeIds(*snapshot, candidates);
        std::vector<NodeId> matches;
        for (const auto& id : candidates) {
            const auto* node = snapshot->findNode(id);
            if (!node) {
                continue;
            }
            if (!request.nativeType.empty() && node->native.typeId != request.nativeType) {
                continue;
            }
            if (!request.sourceDocumentUid.empty()
                && node->native.documentUid != request.sourceDocumentUid) {
                continue;
            }
            if (request.filterVisible && node->presentation.visible != request.visible) {
                continue;
            }
            if (!request.label.empty()) {
                if (request.labelContains) {
                    if (node->display.normalizedLabel.find(request.label) == std::string::npos) {
                        continue;
                    }
                }
                else if (node->display.normalizedLabel != request.label) {
                    continue;
                }
            }
            if (!request.semanticPartKind.empty()) {
                const auto* definition = std::get_if<DefinitionPayload>(&node->payload);
                if (!definition || definition->semanticPartKind != request.semanticPartKind) {
                    continue;
                }
            }
            matches.push_back(id);
        }
        if (cursorOffset > matches.size()) {
            return invalid("CADX_QUERY_CURSOR_INVALID", "cursor offset is outside the query result");
        }
        const auto end = std::min(matches.size(), cursorOffset + request.limit);
        result.nodeIds.insert(result.nodeIds.end(), matches.begin() + cursorOffset, matches.begin() + end);
        if (end < matches.size()) {
            result.truncated = true;
            result.nextCursor = makeCursor(*snapshot, request, end);
        }
        result.returnedNodeCount = result.nodeIds.size();
        return result;
    }

    if (request.operation != QueryOperation::ShortestPath && request.startNodeIds.empty()) {
        return invalid("CADX_QUERY_INVALID", "at least one start node is required");
    }
    if (request.operation != QueryOperation::ShortestPath) {
        for (const auto& id : request.startNodeIds) {
            if (!snapshot->findNode(id)) {
                return invalid("CADX_QUERY_INVALID", "a start node does not exist");
            }
        }
    }

    auto appendEdge = [&](const EdgeRecord& edge, std::unordered_set<EdgeId>& seenEdges,
                          std::unordered_set<NodeId>& seenNodes) {
        if (!edgeAllowed(edge, request) || !seenEdges.insert(edge.id).second) {
            return;
        }
        result.edgeIds.push_back(edge.id);
        seenNodes.insert(edge.from);
        seenNodes.insert(edge.to);
    };

    if (request.operation == QueryOperation::Neighbors) {
        std::unordered_set<EdgeId> seenEdges;
        std::unordered_set<NodeId> seenNodes(request.startNodeIds.begin(), request.startNodeIds.end());
        for (const auto& id : request.startNodeIds) {
            if (request.direction == QueryDirection::Outgoing || request.direction == QueryDirection::Both) {
                for (auto index : snapshot->outgoing(id)) {
                    appendEdge(snapshot->edges()[index], seenEdges, seenNodes);
                }
            }
            if (request.direction == QueryDirection::Incoming || request.direction == QueryDirection::Both) {
                for (auto index : snapshot->incoming(id)) {
                    appendEdge(snapshot->edges()[index], seenEdges, seenNodes);
                }
            }
        }
        for (const auto& id : seenNodes) {
            result.nodeIds.push_back(id);
        }
        sortNodeIds(*snapshot, result.nodeIds);
        if (result.nodeIds.size() > request.limit) {
            result.nodeIds.resize(request.limit);
            result.truncated = true;
        }
        if (result.edgeIds.size() > request.limit) {
            result.edgeIds.resize(request.limit);
            result.truncated = true;
        }
        result.returnedNodeCount = result.nodeIds.size();
        result.returnedEdgeCount = result.edgeIds.size();
        return result;
    }

    if (request.operation == QueryOperation::Subgraph) {
        std::deque<std::pair<NodeId, std::size_t>> pending;
        std::unordered_set<NodeId> seenNodes;
        std::unordered_set<EdgeId> seenEdges;
        for (const auto& id : request.startNodeIds) {
            pending.emplace_back(id, 0);
            seenNodes.insert(id);
        }
        while (!pending.empty()) {
            const auto [id, depth] = pending.front();
            pending.pop_front();
            if (depth >= request.maxDepth) {
                continue;
            }
            for (auto index : snapshot->outgoing(id)) {
                const auto& edge = snapshot->edges()[index];
                if (!edgeAllowed(edge, request) || !seenEdges.insert(edge.id).second) {
                    continue;
                }
                result.edgeIds.push_back(edge.id);
                if (seenNodes.insert(edge.to).second) {
                    pending.emplace_back(edge.to, depth + 1);
                }
            }
            for (auto index : snapshot->incoming(id)) {
                const auto& edge = snapshot->edges()[index];
                if (!edgeAllowed(edge, request) || !seenEdges.insert(edge.id).second) {
                    continue;
                }
                result.edgeIds.push_back(edge.id);
                if (seenNodes.insert(edge.from).second) {
                    pending.emplace_back(edge.from, depth + 1);
                }
            }
        }
        result.nodeIds.assign(seenNodes.begin(), seenNodes.end());
        sortNodeIds(*snapshot, result.nodeIds);
        std::sort(result.edgeIds.begin(), result.edgeIds.end());
        if (result.nodeIds.size() > request.limit) {
            result.nodeIds.resize(request.limit);
            result.truncated = true;
        }
        if (result.edgeIds.size() > request.limit) {
            result.edgeIds.resize(request.limit);
            result.truncated = true;
        }
        result.returnedNodeCount = result.nodeIds.size();
        result.returnedEdgeCount = result.edgeIds.size();
        return result;
    }

    if (request.operation == QueryOperation::ShortestPath) {
        if (request.startNodeId.empty() || request.targetNodeId.empty()
            || !snapshot->findNode(request.startNodeId) || !snapshot->findNode(request.targetNodeId)) {
            return invalid("CADX_QUERY_INVALID", "shortest_path endpoints must exist");
        }
        std::deque<NodeId> pending {request.startNodeId};
        std::unordered_map<NodeId, std::pair<NodeId, EdgeId>> previous;
        std::unordered_set<NodeId> visited {request.startNodeId};
        std::size_t depth = 0;
        while (!pending.empty() && !visited.contains(request.targetNodeId) && depth <= request.maxDepth) {
            const auto levelSize = pending.size();
            for (std::size_t level = 0; level < levelSize; ++level) {
                const auto id = pending.front();
                pending.pop_front();
                for (auto index : snapshot->outgoing(id)) {
                    const auto& edge = snapshot->edges()[index];
                    if (!edgeAllowed(edge, request) || !visited.insert(edge.to).second) {
                        continue;
                    }
                    previous[edge.to] = {id, edge.id};
                    pending.push_back(edge.to);
                }
                for (auto index : snapshot->incoming(id)) {
                    const auto& edge = snapshot->edges()[index];
                    if (!edgeAllowed(edge, request) || !visited.insert(edge.from).second) {
                        continue;
                    }
                    previous[edge.from] = {id, edge.id};
                    pending.push_back(edge.from);
                }
            }
            ++depth;
        }
        if (!visited.contains(request.targetNodeId)) {
            result.returnedNodeCount = 0;
            return result;
        }
        NodeId current = request.targetNodeId;
        result.nodeIds.push_back(current);
        while (current != request.startNodeId) {
            const auto iterator = previous.find(current);
            if (iterator == previous.end()) {
                break;
            }
            result.edgeIds.push_back(iterator->second.second);
            current = iterator->second.first;
            result.nodeIds.push_back(current);
        }
        std::reverse(result.nodeIds.begin(), result.nodeIds.end());
        std::reverse(result.edgeIds.begin(), result.edgeIds.end());
        result.returnedNodeCount = result.nodeIds.size();
        result.returnedEdgeCount = result.edgeIds.size();
        return result;
    }

    return invalid("CADX_QUERY_INVALID", "unsupported graph operation");
}

bool GraphQueryEngine::edgeAllowed(const EdgeRecord& edge, const QueryRequest& request)
{
    return request.edgeKinds.empty()
        || std::find(request.edgeKinds.begin(), request.edgeKinds.end(), edge.kind)
            != request.edgeKinds.end();
}

void GraphQueryEngine::sortNodeIds(const GraphSnapshot& snapshot, std::vector<NodeId>& ids)
{
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    std::sort(ids.begin(), ids.end(), [&snapshot](const NodeId& left, const NodeId& right) {
        const auto* leftNode = snapshot.findNode(left);
        const auto* rightNode = snapshot.findNode(right);
        if (!leftNode || !rightNode) {
            return left < right;
        }
        if (leftNode->kind != rightNode->kind) {
            return static_cast<int>(leftNode->kind) < static_cast<int>(rightNode->kind);
        }
        if (leftNode->display.normalizedLabel != rightNode->display.normalizedLabel) {
            return leftNode->display.normalizedLabel < rightNode->display.normalizedLabel;
        }
        return left < right;
    });
}

QueryResult GraphQueryEngine::invalid(const char* code, const char* message)
{
    QueryResult result;
    result.ok = false;
    result.errorCode = code;
    result.diagnostic = message;
    return result;
}

}  // namespace CadX
