// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AssemblyGraphBuilder.h"

#include "GraphRevision.h"

#include <algorithm>
#include <cctype>

namespace CadX
{

GraphBuildResult AssemblyGraphBuilder::build(const AssemblyCapture& capture) const
{
    if (capture.documentBusy) {
        return {nullptr, "CADX_DOCUMENT_BUSY", "the FreeCAD document is recomputing or in a transaction"};
    }
    if (!capture.startGuardMatches || !capture.endGuardMatches) {
        return {nullptr, "CADX_CAPTURE_CHANGED", "the document or active Assembly changed during capture"};
    }
    if (capture.documentUid.empty() || capture.activeAssemblyObjectName.empty()) {
        return {nullptr, "CADX_NO_ACTIVE_ASSEMBLY", "capture does not identify an exact active Assembly"};
    }

    auto snapshot = std::make_shared<GraphSnapshot>();
    snapshot->header().documentUid = capture.documentUid;
    snapshot->header().documentName = capture.documentName;
    snapshot->header().activeAssemblyObjectName = capture.activeAssemblyObjectName;
    snapshot->header().activeAssemblyLabel = capture.activeAssemblyLabel;
    snapshot->header().activeViewId = capture.activeViewId;
    snapshot->header().cameraState = capture.cameraState;
    snapshot->header().activeAssemblyNodeId = capture.activeAssemblyNodeId;
    snapshot->header().diagnostics = capture.diagnostics;
    snapshot->header().graphId = "assembly-graph:" + sha256Revision(
        capture.documentUid + "|" + capture.activeAssemblyObjectName);

    snapshot->nodes() = capture.nodes;
    snapshot->edges() = capture.edges;
    for (auto& node : snapshot->nodes()) {
        if (node.id.empty()) {
            node.id = stableNodeId(node, node.kind == NodeKind::Occurrence ? "occurrence" : "definition");
        }
        if (!node.localPlacement.normalize() || !node.worldPlacement.normalize()) {
            return {nullptr, "CADX_GRAPH_INVARIANT_FAILED", "capture contains an invalid placement"};
        }
        if (node.display.normalizedLabel.empty()) {
            node.display.normalizedLabel = node.display.label;
            std::transform(node.display.normalizedLabel.begin(), node.display.normalizedLabel.end(),
                           node.display.normalizedLabel.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        }
    }
    for (auto& edge : snapshot->edges()) {
        if (edge.id.empty()) {
            edge.id = stableEdgeId(edge);
        }
    }
    if (snapshot->header().activeAssemblyNodeId.empty()) {
        for (const auto& node : snapshot->nodes()) {
            if (node.kind == NodeKind::AssemblyDefinition
                && node.native.objectName == capture.activeAssemblyObjectName) {
                snapshot->header().activeAssemblyNodeId = node.id;
                break;
            }
        }
    }
    std::string diagnostic;
    if (!snapshot->finalize(diagnostic)) {
        return {nullptr, "CADX_GRAPH_INVARIANT_FAILED", diagnostic};
    }
    return {std::move(snapshot), {}, {}};
}

NodeId AssemblyGraphBuilder::stableNodeId(const NodeRecord& node, const std::string& role)
{
    std::string identity = "cadx.node.v1|" + role + "|" + node.native.canonical();
    if (const auto* occurrence = std::get_if<OccurrencePayload>(&node.payload)) {
        for (const auto& pathElement : occurrence->occurrencePath) {
            identity += "|" + pathElement;
        }
    }
    return "node:" + sha256Revision(identity);
}

EdgeId AssemblyGraphBuilder::stableEdgeId(const EdgeRecord& edge)
{
    return "edge:" + sha256Revision("cadx.edge.v1|" + std::string(edgeKindName(edge.kind)) + "|"
                                      + edge.from + "|" + edge.to + "|" + edge.relation);
}

}  // namespace CadX
