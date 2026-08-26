// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphAudit.h"

#include "GraphRevision.h"
#include "GraphSnapshot.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <utility>

namespace CadX
{
namespace
{
std::string escape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        switch (character) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += character; break;
        }
    }
    return result;
}

std::int64_t nowMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace

std::string GraphAuditEvent::toJson() const
{
    return "{\"schema_version\":\"cadx.graph-audit.v1\",\"sequence\":"
        + std::to_string(sequence) + ",\"timestamp_ms\":" + std::to_string(nowMilliseconds())
        + ",\"stage\":\"" + escape(stage) + "\",\"status\":\"" + escape(status)
        + "\",\"operation\":\"" + escape(operation) + "\",\"graph_id\":\""
        + escape(graphId) + "\",\"graph_revision\":\"" + escape(graphRevision)
        + "\",\"presentation_revision\":\"" + escape(presentationRevision)
        + "\",\"semantic_hash\":\"" + escape(semanticHash)
        + "\",\"presentation_hash\":\"" + escape(presentationHash)
        + "\",\"node_count\":" + std::to_string(nodeCount)
        + ",\"edge_count\":" + std::to_string(edgeCount) + ",\"error_code\":\""
        + escape(errorCode) + "\",\"diagnostic\":\"" + escape(diagnostic)
        + "\",\"operation_id\":\"" + escape(operationId)
        + "\",\"parent_revision\":\"" + escape(parentRevision)
        + "\",\"final_revision\":\"" + escape(finalRevision)
        + "\",\"predicted_delta_hash\":\"" + escape(predictedDeltaHash)
        + "\",\"observed_delta_hash\":\"" + escape(observedDeltaHash)
        + "\",\"physical_verdict\":\"" + escape(physicalVerdict)
        + "\",\"transaction_status\":\"" + escape(transactionStatus) + "\"}";
}

GraphAuditEvent makeMutationAuditEvent(const std::string& stage,
                                       const std::string& status,
                                       const std::string& operation,
                                       const std::string& operationId,
                                       const std::string& parentRevision,
                                       const std::string& finalRevision,
                                       const std::string& predictedDeltaHash,
                                       const std::string& observedDeltaHash,
                                       const std::string& physicalVerdict,
                                       const std::string& transactionStatus,
                                       const GraphSnapshot* snapshot,
                                       const std::string& errorCode,
                                       const std::string& diagnostic)
{
    auto event = makeGraphAuditEvent(stage, status, snapshot, operation, errorCode, diagnostic);
    event.operationId = operationId;
    event.parentRevision = parentRevision;
    event.finalRevision = finalRevision;
    event.predictedDeltaHash = predictedDeltaHash;
    event.observedDeltaHash = observedDeltaHash;
    event.physicalVerdict = physicalVerdict;
    event.transactionStatus = transactionStatus;
    return event;
}

GraphAuditEvent makeGraphAuditEvent(const std::string& stage,
                                    const std::string& status,
                                    const GraphSnapshot* snapshot,
                                    const std::string& operation,
                                    const std::string& errorCode,
                                    const std::string& diagnostic)
{
    GraphAuditEvent event;
    event.stage = stage;
    event.status = status;
    event.operation = operation;
    event.errorCode = errorCode;
    event.diagnostic = diagnostic;
    if (snapshot) {
        event.graphId = snapshot->header().graphId;
        event.graphRevision = snapshot->header().graphRevision;
        event.presentationRevision = snapshot->header().presentationRevision;
        event.semanticHash = sha256Revision(canonicalSemantic(*snapshot));
        event.presentationHash = sha256Revision(canonicalPresentation(*snapshot));
        event.nodeCount = snapshot->nodes().size();
        event.edgeCount = snapshot->edges().size();
    }
    return event;
}

GraphAuditLog::GraphAuditLog(std::string path)
    : _path(std::move(path))
{}

GraphAuditLog::GraphAuditLog(GraphAuditLog&& other) noexcept
{
    std::lock_guard lock(other._mutex);
    _path = std::move(other._path);
    _lastError = std::move(other._lastError);
    _nextSequence = other._nextSequence;
}

GraphAuditLog& GraphAuditLog::operator=(GraphAuditLog&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    std::scoped_lock lock(_mutex, other._mutex);
    _path = std::move(other._path);
    _lastError = std::move(other._lastError);
    _nextSequence = other._nextSequence;
    return *this;
}

GraphAuditLog GraphAuditLog::fromEnvironment()
{
    const auto* path = std::getenv("CADX_GRAPH_AUDIT_LOG");
    if (path && *path) {
        return GraphAuditLog(path);
    }
    const auto* enabled = std::getenv("CADX_GRAPH_AUDIT");
    if (enabled && std::string(enabled) == "1") {
        return GraphAuditLog("/tmp/cadx-graph-audit.jsonl");
    }
    return {};
}

void GraphAuditLog::record(GraphAuditEvent event)
{
    if (!enabled()) {
        return;
    }
    std::lock_guard lock(_mutex);
    event.sequence = _nextSequence++;
    std::ofstream output(_path, std::ios::app);
    if (!output) {
        _lastError = "unable to open graph audit log: " + _path;
        return;
    }
    output << event.toJson() << '\n';
    output.flush();
    if (!output) {
        _lastError = "unable to flush graph audit log: " + _path;
    }
}

}  // namespace CadX
