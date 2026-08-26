// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphTypes.h"

#include <cstddef>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace CadX
{

struct GraphHeader
{
    std::string schemaVersion = "cadx.assembly-graph.v1";
    std::string graphId;
    std::string graphRevision;
    std::string presentationRevision;
    std::string documentUid;
    std::string documentName;
    std::string activeAssemblyNodeId;
    std::string activeAssemblyObjectName;
    std::string activeAssemblyLabel;
    std::string activeViewId;
    std::string cameraState;
    bool complete = true;
    bool stale = false;
    std::vector<std::string> diagnostics;
};

class GraphSnapshot
{
public:
    using NodeIndex = std::size_t;
    using EdgeIndex = std::size_t;

    GraphSnapshot() = default;

    GraphHeader& header() noexcept { return _header; }
    const GraphHeader& header() const noexcept { return _header; }

    std::vector<NodeRecord>& nodes() noexcept { return _nodes; }
    const std::vector<NodeRecord>& nodes() const noexcept { return _nodes; }
    std::vector<EdgeRecord>& edges() noexcept { return _edges; }
    const std::vector<EdgeRecord>& edges() const noexcept { return _edges; }

    bool finalize(std::string& diagnostic);
    bool validate(std::string& diagnostic) const;
    std::size_t estimatedBytes() const noexcept { return _estimatedBytes; }

    const NodeRecord* findNode(const NodeId& id) const noexcept;
    const EdgeRecord* findEdge(const EdgeId& id) const noexcept;
    const std::vector<NodeIndex>& nodesByKind(NodeKind kind) const noexcept;
    const std::vector<NodeIndex>& nodesBySourceDocument(const std::string& uid) const noexcept;
    const std::vector<EdgeIndex>& outgoing(const NodeId& id) const noexcept;
    const std::vector<EdgeIndex>& incoming(const NodeId& id) const noexcept;

private:
    void clearIndexes();
    bool validateContainmentAcyclic(std::string& diagnostic) const;

    GraphHeader _header;
    std::vector<NodeRecord> _nodes;
    std::vector<EdgeRecord> _edges;
    std::unordered_map<NodeId, NodeIndex> _nodeById;
    std::unordered_map<EdgeId, EdgeIndex> _edgeById;
    std::map<NodeKind, std::vector<NodeIndex>> _nodesByKind;
    std::unordered_map<std::string, std::vector<NodeIndex>> _nodesBySourceDocument;
    std::unordered_map<NodeId, std::vector<EdgeIndex>> _outgoing;
    std::unordered_map<NodeId, std::vector<EdgeIndex>> _incoming;
    std::size_t _estimatedBytes = 0;
};

}  // namespace CadX
