// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace CadX
{

class GraphSnapshot;

struct GraphAuditEvent
{
    std::uint64_t sequence = 0;
    std::string stage;
    std::string status;
    std::string operation;
    std::string graphId;
    std::string graphRevision;
    std::string presentationRevision;
    std::string semanticHash;
    std::string presentationHash;
    std::size_t nodeCount = 0;
    std::size_t edgeCount = 0;
    std::string errorCode;
    std::string diagnostic;
    std::string operationId;
    std::string parentRevision;
    std::string finalRevision;
    std::string predictedDeltaHash;
    std::string observedDeltaHash;
    std::string physicalVerdict;
    std::string transactionStatus;

    std::string toJson() const;
};

GraphAuditEvent makeGraphAuditEvent(const std::string& stage,
                                    const std::string& status,
                                    const GraphSnapshot* snapshot = nullptr,
                                    const std::string& operation = {},
                                    const std::string& errorCode = {},
                                    const std::string& diagnostic = {});

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
                                       const GraphSnapshot* snapshot = nullptr,
                                       const std::string& errorCode = {},
                                       const std::string& diagnostic = {});

// Writes one flushed JSON object per line when CADX_GRAPH_AUDIT_LOG is set.
// CADX_GRAPH_AUDIT=1 uses /tmp/cadx-graph-audit.jsonl as a convenient default.
// The log is evidence, not graph authority: hashes and revisions are always
// computed from the immutable snapshot itself.
class GraphAuditLog
{
public:
    GraphAuditLog() = default;
    explicit GraphAuditLog(std::string path);
    GraphAuditLog(const GraphAuditLog&) = delete;
    GraphAuditLog& operator=(const GraphAuditLog&) = delete;
    GraphAuditLog(GraphAuditLog&& other) noexcept;
    GraphAuditLog& operator=(GraphAuditLog&& other) noexcept;

    static GraphAuditLog fromEnvironment();

    void record(GraphAuditEvent event);
    bool enabled() const noexcept { return !_path.empty(); }
    const std::string& path() const noexcept { return _path; }
    const std::string& lastError() const noexcept { return _lastError; }

private:
    std::string _path;
    std::string _lastError;
    std::uint64_t _nextSequence = 1;
    mutable std::mutex _mutex;
};

}  // namespace CadX
