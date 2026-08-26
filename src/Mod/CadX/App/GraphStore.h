// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphSnapshot.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace CadX
{

enum class StoreError
{
    None,
    GraphNotFound,
    RevisionMismatch,
    GraphStale,
    LimitExceeded,
};

struct GraphScope
{
    std::string documentUid;
    std::string assemblyObjectName;

    std::string key() const { return documentUid + "\x1f" + assemblyObjectName; }
};

struct GraphLookup
{
    StoreError error = StoreError::None;
    std::string diagnostic;
    std::shared_ptr<const GraphSnapshot> snapshot;

    explicit operator bool() const noexcept { return snapshot != nullptr && error == StoreError::None; }
};

struct GraphStoreLimits
{
    std::size_t maxNodesPerSnapshot = 10'000;
    std::size_t maxEdgesPerSnapshot = 40'000;
    std::size_t maxBytesPerSnapshot = 64ULL * 1024ULL * 1024ULL;
    std::size_t maxBytesTotal = 256ULL * 1024ULL * 1024ULL;
    std::size_t retainedRevisionsPerScope = 3;
};

class GraphStore
{
public:
    explicit GraphStore(GraphStoreLimits limits = {});

    StoreError publish(const GraphScope& scope,
                       std::shared_ptr<GraphSnapshot> snapshot,
                       std::string& diagnostic);
    StoreError publishIfCurrent(const GraphScope& scope,
                                std::shared_ptr<GraphSnapshot> snapshot,
                                const std::string& expectedBaseRevision,
                                std::string& diagnostic);
    GraphLookup lookup(const std::string& graphId,
                       const std::string& graphRevision,
                       bool allowStale = false) const;
    GraphLookup current(const GraphScope& scope, bool allowStale = false) const;
    bool markStale(const std::string& graphId, const std::string& diagnostic);
    bool markScopeStale(const GraphScope& scope, const std::string& diagnostic);
    bool markSourceDocumentStale(const std::string& documentUid, const std::string& diagnostic);
    void removeDocument(const std::string& documentUid);
    std::size_t graphCount() const;
    std::size_t retainedBytes() const;
    const GraphStoreLimits& limits() const noexcept { return _limits; }

private:
    struct Entry
    {
        GraphScope scope;
        std::shared_ptr<const GraphSnapshot> current;
        std::deque<std::shared_ptr<const GraphSnapshot>> retained;
        bool stale = false;
        std::string staleDiagnostic;
    };

    void evictIfNeeded();
    StoreError publishInternal(const GraphScope& scope,
                               std::shared_ptr<GraphSnapshot> snapshot,
                               const std::string& expectedBaseRevision,
                               bool requireCompareAndSwap,
                               std::string& diagnostic);
    static bool sameScope(const GraphScope& left, const GraphScope& right) noexcept;

    GraphStoreLimits _limits;
    mutable std::mutex _mutex;
    std::unordered_map<std::string, Entry> _entries;
};

}  // namespace CadX
