// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <Base/Vector3D.h>
#include <Mod/Import/ImportGlobal.h>

namespace App
{
class Document;
class DocumentObject;
}

namespace Import
{

struct ImportExport StepLightweightWorkspaceShardState
{
    std::string documentPath;
    std::string objectName;
    std::string loadSource;
    bool isWorkspaceShard = false;
    bool isOpen = false;
    bool isPartial = false;
    bool isFullyLoaded = false;
    bool hasProxy = false;
    bool isPinned = false;
};

struct ImportExport StepLightweightWorkspaceState
{
    bool isWorkspaceDocument = false;
    std::string masterDocumentPath;
    std::size_t openShardCount = 0;
    std::size_t fullyLoadedShardCount = 0;
    std::size_t unloadedShardCount = 0;
    std::size_t proxyShardCount = 0;
    std::size_t pinnedShardCount = 0;
    std::size_t initialLoadedShardCount = 0;
    std::size_t manualLoadedShardCount = 0;
    std::size_t prefetchedShardCount = 0;
    std::size_t manualLoadEventCount = 0;
    std::size_t prefetchEventCount = 0;
    std::size_t trimmedShardEventCount = 0;
    std::size_t manualUnloadEventCount = 0;
    std::size_t pendingInitialRestoreCount = 0;
    std::vector<StepLightweightWorkspaceShardState> shards;
};

class ImportExport StepLightweightWorkspaceRuntime
{
public:
    static void init();
    static void initializeDocument(App::Document& workspaceDoc);
    static int configuredMaxLoadedShards();
    static int setConfiguredMaxLoadedShards(int maxLoadedShards);

    static bool isWorkspaceDocument(const App::Document& doc);
    static void resetWorkspaceMetrics(const App::Document& doc);
    static void synchronizeLinkedShardProxies(const App::Document& workspaceDoc);
    static StepLightweightWorkspaceState inspect(const App::Document& doc);
    static StepLightweightWorkspaceShardState inspectLinkedShard(const App::DocumentObject& object);

    static bool pinLinkedShard(App::DocumentObject& object);
    static bool unpinLinkedShard(App::DocumentObject& object);
    static std::size_t restorePinnedShards(const App::Document& workspaceDoc);
    static std::size_t enforceLoadedShardBudget(
        const App::Document& workspaceDoc,
        int maxLoadedShards = -1
    );

    static std::size_t prefetchLinkedShardNeighbors(App::DocumentObject& object);
    static std::size_t prefetchShardsNearPoint(
        const App::Document& workspaceDoc,
        const Base::Vector3d& focusPoint,
        int maxPrefetchCount = -1
    );
    static std::size_t restoreDeferredInitialShardsNearPoint(
        const App::Document& workspaceDoc,
        const Base::Vector3d& focusPoint,
        int maxRestoreCount = -1
    );
    static std::size_t rebalanceShardsNearPoint(
        const App::Document& workspaceDoc,
        const Base::Vector3d& focusPoint,
        int maxReplacementCount = -1
    );
    static std::size_t trimShardsOutsideRetainedPaths(
        const App::Document& workspaceDoc,
        const std::vector<std::string>& retainedDocumentPaths,
        int maxTrimCount = -1
    );
    static App::DocumentObject* loadLinkedShard(App::DocumentObject& object);
    static bool unloadLinkedShard(App::DocumentObject& object);

    static std::size_t trimLoadedShards(
        const App::Document& workspaceDoc,
        int maxLoadedShards = -1,
        const std::string& keepDocumentPath = {}
    );

    static void noteDocumentAccess(const App::Document& doc);
};

}  // namespace Import
