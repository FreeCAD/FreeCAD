// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphSnapshot.h"

#include "GraphRevision.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_set>

namespace CadX
{
namespace
{
bool finitePlacement(const Placement& placement)
{
    const double length = std::sqrt(placement.qx * placement.qx + placement.qy * placement.qy
                                    + placement.qz * placement.qz + placement.qw * placement.qw);
    return std::isfinite(placement.x) && std::isfinite(placement.y) && std::isfinite(placement.z)
        && std::isfinite(length) && std::abs(length - 1.0) < 1e-8;
}

}  // namespace

bool GraphSnapshot::finalize(std::string& diagnostic)
{
    if (!validate(diagnostic)) {
        return false;
    }
    std::sort(_nodes.begin(), _nodes.end(), [](const NodeRecord& left, const NodeRecord& right) {
        if (left.kind != right.kind) {
            return static_cast<int>(left.kind) < static_cast<int>(right.kind);
        }
        if (left.display.normalizedLabel != right.display.normalizedLabel) {
            return left.display.normalizedLabel < right.display.normalizedLabel;
        }
        return left.id < right.id;
    });
    std::sort(_edges.begin(), _edges.end(), [](const EdgeRecord& left, const EdgeRecord& right) {
        if (left.kind != right.kind) {
            return static_cast<int>(left.kind) < static_cast<int>(right.kind);
        }
        if (left.from != right.from) {
            return left.from < right.from;
        }
        if (left.to != right.to) {
            return left.to < right.to;
        }
        return left.id < right.id;
    });

    clearIndexes();
    for (NodeIndex index = 0; index < _nodes.size(); ++index) {
        const auto& node = _nodes[index];
        _nodeById.emplace(node.id, index);
        _nodesByKind[node.kind].push_back(index);
        if (!node.native.documentUid.empty()) {
            _nodesBySourceDocument[node.native.documentUid].push_back(index);
        }
        _estimatedBytes += sizeof(NodeRecord) + node.id.size() + node.native.documentUid.size()
            + node.native.objectName.size() + node.native.typeId.size()
            + node.display.label.size() + node.display.normalizedLabel.size();
    }
    for (EdgeIndex index = 0; index < _edges.size(); ++index) {
        const auto& edge = _edges[index];
        _edgeById.emplace(edge.id, index);
        _outgoing[edge.from].push_back(index);
        _incoming[edge.to].push_back(index);
        _estimatedBytes += sizeof(EdgeRecord) + edge.id.size() + edge.from.size() + edge.to.size();
    }
    for (auto& [kind, indexes] : _nodesByKind) {
        std::sort(indexes.begin(), indexes.end(), [this](NodeIndex left, NodeIndex right) {
            return _nodes[left].id < _nodes[right].id;
        });
    }
    _header.graphRevision = sha256Revision(canonicalSemantic(*this));
    _header.presentationRevision = sha256Revision(canonicalPresentation(*this));
    return true;
}

bool GraphSnapshot::validate(std::string& diagnostic) const
{
    if (_header.graphId.empty() || _header.documentUid.empty()
        || _header.activeAssemblyNodeId.empty()) {
        diagnostic = "graph header is missing a stable scope identity";
        return false;
    }
    std::unordered_set<NodeId> nodeIds;
    std::size_t assemblyRoots = 0;
    for (const auto& node : _nodes) {
        if (node.id.empty() || !nodeIds.insert(node.id).second) {
            diagnostic = "graph contains a duplicate or empty node id";
            return false;
        }
        if (node.kind == NodeKind::AssemblyDefinition) {
            ++assemblyRoots;
        }
        if (!finitePlacement(node.localPlacement) || !finitePlacement(node.worldPlacement)) {
            diagnostic = "graph contains a non-finite or non-normalized placement";
            return false;
        }
    }
    if (assemblyRoots != 1 || !nodeIds.contains(_header.activeAssemblyNodeId)) {
        diagnostic = "graph must contain exactly one active Assembly root";
        return false;
    }
    std::unordered_set<EdgeId> edgeIds;
    std::unordered_map<NodeId, std::size_t> definitionEdges;
    for (const auto& edge : _edges) {
        if (edge.id.empty() || !edgeIds.insert(edge.id).second || !nodeIds.contains(edge.from)
            || !nodeIds.contains(edge.to)) {
            diagnostic = "graph contains an invalid edge endpoint or duplicate edge id";
            return false;
        }
        if (edge.kind == EdgeKind::InstanceOf) {
            ++definitionEdges[edge.from];
        }
    }
    for (const auto& node : _nodes) {
        if ((node.kind == NodeKind::Occurrence || node.kind == NodeKind::AssemblyOccurrence)
            && !node.unresolved && definitionEdges[node.id] != 1) {
            diagnostic = "every resolved occurrence must have exactly one INSTANCE_OF edge";
            return false;
        }
    }
    if (!validateContainmentAcyclic(diagnostic)) {
        return false;
    }
    return true;
}

bool GraphSnapshot::validateContainmentAcyclic(std::string& diagnostic) const
{
    std::unordered_map<NodeId, std::vector<NodeId>> children;
    for (const auto& edge : _edges) {
        if (edge.kind == EdgeKind::Contains || edge.kind == EdgeKind::NestedOccurrence) {
            children[edge.from].push_back(edge.to);
        }
    }
    std::unordered_set<NodeId> visiting;
    std::unordered_set<NodeId> visited;
    std::function<bool(const NodeId&)> visit = [&](const NodeId& id) {
        if (visiting.contains(id)) {
            diagnostic = "graph containment is cyclic";
            return false;
        }
        if (visited.contains(id)) {
            return true;
        }
        visiting.insert(id);
        for (const auto& child : children[id]) {
            if (!visit(child)) {
                return false;
            }
        }
        visiting.erase(id);
        visited.insert(id);
        return true;
    };
    for (const auto& node : _nodes) {
        if (!visit(node.id)) {
            return false;
        }
    }
    return true;
}

void GraphSnapshot::clearIndexes()
{
    _nodeById.clear();
    _edgeById.clear();
    _nodesByKind.clear();
    _nodesBySourceDocument.clear();
    _outgoing.clear();
    _incoming.clear();
    _estimatedBytes = 0;
}

const NodeRecord* GraphSnapshot::findNode(const NodeId& id) const noexcept
{
    const auto iterator = _nodeById.find(id);
    return iterator == _nodeById.end() ? nullptr : &_nodes[iterator->second];
}

const EdgeRecord* GraphSnapshot::findEdge(const EdgeId& id) const noexcept
{
    const auto iterator = _edgeById.find(id);
    return iterator == _edgeById.end() ? nullptr : &_edges[iterator->second];
}

const std::vector<GraphSnapshot::NodeIndex>& GraphSnapshot::nodesByKind(NodeKind kind) const noexcept
{
    static const std::vector<NodeIndex> empty;
    const auto iterator = _nodesByKind.find(kind);
    return iterator == _nodesByKind.end() ? empty : iterator->second;
}

const std::vector<GraphSnapshot::NodeIndex>& GraphSnapshot::nodesBySourceDocument(
    const std::string& uid) const noexcept
{
    static const std::vector<NodeIndex> empty;
    const auto iterator = _nodesBySourceDocument.find(uid);
    return iterator == _nodesBySourceDocument.end() ? empty : iterator->second;
}

const std::vector<GraphSnapshot::EdgeIndex>& GraphSnapshot::outgoing(const NodeId& id) const noexcept
{
    static const std::vector<EdgeIndex> empty;
    const auto iterator = _outgoing.find(id);
    return iterator == _outgoing.end() ? empty : iterator->second;
}

const std::vector<GraphSnapshot::EdgeIndex>& GraphSnapshot::incoming(const NodeId& id) const noexcept
{
    static const std::vector<EdgeIndex> empty;
    const auto iterator = _incoming.find(id);
    return iterator == _incoming.end() ? empty : iterator->second;
}

}  // namespace CadX
