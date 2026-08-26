// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphStore.h"

#include <algorithm>

namespace CadX
{

GraphStore::GraphStore(GraphStoreLimits limits)
    : _limits(limits)
{
    _limits.retainedRevisionsPerScope = std::max<std::size_t>(1, _limits.retainedRevisionsPerScope);
}

StoreError GraphStore::publish(const GraphScope& scope,
                               std::shared_ptr<GraphSnapshot> snapshot,
                               std::string& diagnostic)
{
    return publishInternal(scope, std::move(snapshot), {}, false, diagnostic);
}

StoreError GraphStore::publishIfCurrent(const GraphScope& scope,
                                        std::shared_ptr<GraphSnapshot> snapshot,
                                        const std::string& expectedBaseRevision,
                                        std::string& diagnostic)
{
    return publishInternal(scope, std::move(snapshot), expectedBaseRevision, true, diagnostic);
}

StoreError GraphStore::publishInternal(const GraphScope& scope,
                                       std::shared_ptr<GraphSnapshot> snapshot,
                                       const std::string& expectedBaseRevision,
                                       bool requireCompareAndSwap,
                                       std::string& diagnostic)
{
    if (!snapshot) {
        diagnostic = "cannot publish an empty graph";
        return StoreError::LimitExceeded;
    }
    if (snapshot->nodes().size() > _limits.maxNodesPerSnapshot
        || snapshot->edges().size() > _limits.maxEdgesPerSnapshot
        || snapshot->estimatedBytes() > _limits.maxBytesPerSnapshot) {
        diagnostic = "graph exceeds its configured resource limit";
        return StoreError::LimitExceeded;
    }
    const auto& header = snapshot->header();
    if (scope.documentUid.empty() || scope.assemblyObjectName.empty()
        || header.graphId.empty()
        || header.documentUid != scope.documentUid
        || header.activeAssemblyObjectName != scope.assemblyObjectName) {
        diagnostic = "graph scope does not match the snapshot header";
        return StoreError::RevisionMismatch;
    }
    std::lock_guard lock(_mutex);
    auto iterator = _entries.find(header.graphId);
    if (iterator != _entries.end() && !sameScope(iterator->second.scope, scope)) {
        diagnostic = "graph scope does not match the existing graph handle";
        return StoreError::RevisionMismatch;
    }
    if (requireCompareAndSwap) {
        if (expectedBaseRevision.empty()) {
            if (iterator != _entries.end()) {
                diagnostic = "initial graph publication requires an empty graph scope";
                return StoreError::RevisionMismatch;
            }
        }
        else if (iterator == _entries.end() || !iterator->second.current
                 || iterator->second.current->header().graphRevision != expectedBaseRevision) {
            diagnostic = "graph publication base revision is stale";
            return StoreError::RevisionMismatch;
        }
    }
    auto& entry = _entries[header.graphId];
    entry.scope = scope;
    if (entry.current) {
        entry.retained.push_front(entry.current);
    }
    while (entry.retained.size() >= _limits.retainedRevisionsPerScope) {
        entry.retained.pop_back();
    }
    entry.current = std::move(snapshot);
    entry.stale = false;
    entry.staleDiagnostic.clear();
    evictIfNeeded();
    return StoreError::None;
}

GraphLookup GraphStore::lookup(const std::string& graphId,
                               const std::string& graphRevision,
                               bool allowStale) const
{
    std::lock_guard lock(_mutex);
    const auto iterator = _entries.find(graphId);
    if (iterator == _entries.end()) {
        return {StoreError::GraphNotFound, "the graph handle is unknown", nullptr};
    }
    const auto& entry = iterator->second;
    std::shared_ptr<const GraphSnapshot> match;
    if (entry.current && entry.current->header().graphRevision == graphRevision) {
        match = entry.current;
    }
    if (!match) {
        for (const auto& revision : entry.retained) {
            if (revision->header().graphRevision == graphRevision) {
                match = revision;
                break;
            }
        }
    }
    if (!match) {
        return {StoreError::RevisionMismatch, "the requested graph revision is not retained", nullptr};
    }
    if (entry.stale && !allowStale) {
        return {StoreError::GraphStale, entry.staleDiagnostic, nullptr};
    }
    return {StoreError::None, entry.staleDiagnostic, std::move(match)};
}

GraphLookup GraphStore::current(const GraphScope& scope, bool allowStale) const
{
    std::lock_guard lock(_mutex);
    for (const auto& [graphId, entry] : _entries) {
        if (!sameScope(entry.scope, scope) || !entry.current) {
            continue;
        }
        if (entry.stale && !allowStale) {
            return {StoreError::GraphStale, entry.staleDiagnostic, nullptr};
        }
        return {StoreError::None, entry.staleDiagnostic, entry.current};
    }
    return {StoreError::GraphNotFound, "no graph exists for this Assembly scope", nullptr};
}

bool GraphStore::markStale(const std::string& graphId, const std::string& diagnostic)
{
    std::lock_guard lock(_mutex);
    const auto iterator = _entries.find(graphId);
    if (iterator == _entries.end()) {
        return false;
    }
    iterator->second.stale = true;
    iterator->second.staleDiagnostic = diagnostic;
    return true;
}

bool GraphStore::markScopeStale(const GraphScope& scope, const std::string& diagnostic)
{
    std::lock_guard lock(_mutex);
    bool marked = false;
    for (auto& [graphId, entry] : _entries) {
        if (sameScope(entry.scope, scope)) {
            entry.stale = true;
            entry.staleDiagnostic = diagnostic;
            marked = true;
        }
    }
    return marked;
}

bool GraphStore::markSourceDocumentStale(const std::string& documentUid,
                                         const std::string& diagnostic)
{
    std::lock_guard lock(_mutex);
    bool marked = false;
    for (auto& [graphId, entry] : _entries) {
        if (!entry.current || entry.current->nodesBySourceDocument(documentUid).empty()) {
            continue;
        }
        entry.stale = true;
        entry.staleDiagnostic = diagnostic;
        marked = true;
    }
    return marked;
}

void GraphStore::removeDocument(const std::string& documentUid)
{
    std::lock_guard lock(_mutex);
    for (auto iterator = _entries.begin(); iterator != _entries.end();) {
        if (iterator->second.scope.documentUid == documentUid) {
            iterator = _entries.erase(iterator);
        }
        else {
            ++iterator;
        }
    }
}

std::size_t GraphStore::graphCount() const
{
    std::lock_guard lock(_mutex);
    return _entries.size();
}

std::size_t GraphStore::retainedBytes() const
{
    std::lock_guard lock(_mutex);
    std::size_t bytes = 0;
    for (const auto& [graphId, entry] : _entries) {
        if (entry.current) {
            bytes += entry.current->estimatedBytes();
        }
        for (const auto& revision : entry.retained) {
            bytes += revision->estimatedBytes();
        }
    }
    return bytes;
}

bool GraphStore::sameScope(const GraphScope& left, const GraphScope& right) noexcept
{
    return left.documentUid == right.documentUid
        && left.assemblyObjectName == right.assemblyObjectName;
}

void GraphStore::evictIfNeeded()
{
    // Retention is already bounded per scope.  If the process-wide cap is
    // reached, discard retained history first and keep every current graph.
    auto totalBytes = [this]() {
        std::size_t bytes = 0;
        for (const auto& [graphId, entry] : _entries) {
            if (entry.current) {
                bytes += entry.current->estimatedBytes();
            }
            for (const auto& revision : entry.retained) {
                bytes += revision->estimatedBytes();
            }
        }
        return bytes;
    };
    while (totalBytes() > _limits.maxBytesTotal) {
        auto candidate = _entries.end();
        for (auto iterator = _entries.begin(); iterator != _entries.end(); ++iterator) {
            if (iterator->second.retained.empty()) {
                continue;
            }
            if (candidate == _entries.end()
                || iterator->second.retained.back()->header().graphRevision
                    < candidate->second.retained.back()->header().graphRevision) {
                candidate = iterator;
            }
        }
        if (candidate == _entries.end()) {
            break;
        }
        candidate->second.retained.pop_back();
    }
}

}  // namespace CadX
