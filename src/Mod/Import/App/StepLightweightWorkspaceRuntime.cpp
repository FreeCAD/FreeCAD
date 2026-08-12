// SPDX-License-Identifier: LGPL-2.1-or-later

#include "StepLightweightWorkspaceRuntime.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/BoundBox.h>
#include <Base/FileInfo.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>

#include "StepLightweightManifest.h"

using namespace Import;
namespace PartApp = Part;

namespace
{

constexpr const char* lightweightProxyBoundsMinPropertyName = "LightweightProxyBoundsMin";
constexpr const char* lightweightProxyBoundsMaxPropertyName = "LightweightProxyBoundsMax";
constexpr const char* lightweightProxyLabel = "Lightweight proxy";
constexpr const char* lightweightProxyPropertyName = "LightweightProxyObject";
constexpr const char* lightweightProxyPrototypeLabel = "Lightweight proxy prototype";
constexpr const char* lightweightProxyPrototypePropertyName = "LightweightProxyPrototype";
constexpr const char* unloadedLoadSourceName = "unloaded";
constexpr const char* initialLoadSourceName = "initial";
constexpr const char* manualLoadSourceName = "manual";
constexpr const char* prefetchLoadSourceName = "prefetch";
constexpr const char* pinnedShardsFilename = "pinned_shards.txt";
constexpr const char* lightweightWorkspaceManifestPathPropertyName = "LightweightWorkspaceManifestPath";
constexpr const char* lightweightWorkspaceShardLinkPropertyName = "LightweightWorkspaceShardLink";
constexpr const char* lightweightWorkspaceShardDocumentPathPropertyName
    = "LightweightWorkspaceShardDocumentPath";
constexpr const char* lightweightWorkspaceShardObjectNamePropertyName
    = "LightweightWorkspaceShardObjectName";

std::unordered_map<std::string, std::uint64_t> shardAccessStamps;
enum class ShardLoadSource
{
    Initial,
    Manual,
    Prefetch
};

enum class WorkspaceDocumentRole
{
    None,
    Root,
    Shard
};

struct WorkspaceRuntimeMetrics
{
    std::size_t manualLoadEventCount = 0;
    std::size_t prefetchEventCount = 0;
    std::size_t trimmedShardEventCount = 0;
    std::size_t manualUnloadEventCount = 0;
};

struct WorkspaceSpatialShardEntry
{
    std::string documentPath;
    std::string normalizedDocumentPath;
    std::string linkObjectName;
    Base::Vector3d center;
    bool hasSpatialCenter = false;
    std::size_t manifestIndex = 0;
};

struct WorkspaceSpatialCellCoord
{
    int x = 0;
    int y = 0;
    int z = 0;
};

bool operator==(const WorkspaceSpatialCellCoord& left, const WorkspaceSpatialCellCoord& right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

struct WorkspaceSpatialCellCoordHash
{
    std::size_t operator()(const WorkspaceSpatialCellCoord& coord) const noexcept
    {
        std::size_t seed = std::hash<int> {}(coord.x);
        seed ^= std::hash<int> {}(coord.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int> {}(coord.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct WorkspaceSpatialIndex
{
    std::vector<WorkspaceSpatialShardEntry> shards;
    std::unordered_map<std::string, std::size_t> shardIndicesByPath;
    std::unordered_map<WorkspaceSpatialCellCoord,
                       std::vector<std::size_t>,
                       WorkspaceSpatialCellCoordHash>
        shardIndicesByCell;
    std::vector<std::size_t> shardIndicesWithoutSpatialCenter;
    Base::BoundBox3d spatialCenterBounds;
    double cellSize = 0.0;
    bool hasSpatialGrid = false;
};

std::unordered_map<std::string, ShardLoadSource> shardLoadSources;
std::unordered_map<std::string, WorkspaceRuntimeMetrics> workspaceRuntimeMetrics;
std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> workspacePinnedShards;
std::unordered_map<std::string, WorkspaceSpatialIndex> workspaceSpatialIndices;
std::unordered_map<std::string, std::unordered_set<std::string>> workspaceLoadedShardPaths;
std::unordered_set<std::string> initializedLoadedShardPathCaches;
std::unordered_map<std::string, std::size_t> workspacePendingInitialRestoreCounts;
std::uint64_t nextShardAccessStampValue = 1;
fastsignals::scoped_connection finishOpenConnection;
fastsignals::scoped_connection deleteDocumentConnection;
fastsignals::scoped_connection startSaveConnection;
fastsignals::scoped_connection finishSaveConnection;
std::unordered_set<const App::Document*> initializedWorkspaceDocuments;
std::unordered_set<const App::Document*> initializingWorkspaceDocuments;
bool runtimeHooksInitialized = false;

struct WorkspaceSaveState
{
    StepLightweightManifest manifest;
    std::vector<std::string> loadedShardPaths;
};

struct DocumentManifestCacheEntry
{
    std::string normalizedDocumentPath;
    std::size_t objectCount = 0;
    WorkspaceDocumentRole role = WorkspaceDocumentRole::None;
    StepLightweightManifest manifest;
    std::unordered_map<std::string, std::size_t> shardIndexByReferenceKey;
};

std::unordered_map<const App::Document*, WorkspaceSaveState> workspaceSaveStates;
std::unordered_map<const App::Document*, DocumentManifestCacheEntry> workspaceDocumentManifestCaches;

std::uint64_t accessStampForPath(const std::string& documentPath);
void clearWorkspaceSpatialIndex(const StepLightweightManifest& manifest);
bool rebuildWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc,
    WorkspaceSpatialIndex& spatialIndex
);
WorkspaceSpatialIndex* ensureWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc
);
void refreshWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc
);
void clearLinkedShardProxyContents(App::DocumentObject& object);
bool refreshLinkedShardAfterLoad(App::DocumentObject& object);
std::vector<std::string> captureLoadedWorkspaceShardPaths(const StepLightweightManifest& manifest);
void prepareWorkspaceDocumentForLazySave(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::vector<std::string>& loadedShardPaths
);
void restoreWorkspaceDocumentAfterLazySave(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::vector<std::string>& loadedShardPaths
);
void purgeWorkspaceTouchState(const App::Document& workspaceDoc);
std::size_t pendingInitialRestoreCountForDocument(const App::Document& doc);
void setPendingInitialRestoreCountForDocument(const App::Document& doc, std::size_t pendingCount);
void consumePendingInitialRestoreCountForDocument(const App::Document& doc, std::size_t consumedCount);
bool shouldDeferInitialRestore(const App::Document& workspaceDoc, const StepLightweightManifest& manifest);
std::size_t restoreDeferredInitialShardsNearPointImpl(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxRestoreCount
);
void clearLoadedShardPathCache(const StepLightweightManifest& manifest);
std::unordered_set<std::string>& ensureLoadedShardPathsForManifest(const StepLightweightManifest& manifest);
void markShardLoadedForManifest(const StepLightweightManifest& manifest, const std::string& documentPath);
void markShardUnloadedForManifest(const StepLightweightManifest& manifest, const std::string& documentPath);
WorkspaceSpatialCellCoord spatialCellCoordForPoint(
    const WorkspaceSpatialIndex& spatialIndex,
    const Base::Vector3d& point
);
std::vector<std::size_t> collectSpatialManifestIndicesNearPoint(
    const WorkspaceSpatialIndex& spatialIndex,
    const Base::Vector3d& focusPoint,
    std::size_t desiredCount
);
std::string shardReferenceKey(const std::string& normalizedDocumentPath, const std::string& objectName);
void rebuildDocumentManifestShardIndex(DocumentManifestCacheEntry& cacheEntry);
bool buildDocumentManifestCacheEntry(const App::Document& doc, DocumentManifestCacheEntry& cacheEntry);
const DocumentManifestCacheEntry* documentManifestCacheEntryForDocument(const App::Document& doc);
const StepLightweightShard* manifestShardForReference(
    const DocumentManifestCacheEntry& cacheEntry,
    const std::string& documentPath,
    const std::string& objectName
);

template<typename Candidate, typename BetterCandidate>
auto makeCandidatePriorityQueue(std::vector<Candidate>&& candidates, BetterCandidate betterCandidate)
{
    auto lowerPriority = [betterCandidate](const Candidate& left, const Candidate& right) {
        return betterCandidate(right, left);
    };

    return std::priority_queue<Candidate, std::vector<Candidate>, decltype(lowerPriority)>(
        lowerPriority,
        std::move(candidates)
    );
}

std::string normalizePath(const std::string& path)
{
    if (path.empty()) {
        return {};
    }

    auto fsPath = Base::FileInfo::stringToPath(path).lexically_normal();
    return Base::FileInfo::pathToString(fsPath);
}

std::string absolutePathFromOwner(const App::Document& ownerDoc, const std::string& path)
{
    if (path.empty()) {
        return {};
    }

    std::filesystem::path fsPath = Base::FileInfo::stringToPath(path);
    if (fsPath.is_relative()) {
        std::filesystem::path ownerDir
            = Base::FileInfo::stringToPath(Base::FileInfo(ownerDoc.FileName.getValue()).dirPath());
        fsPath = ownerDir / fsPath;
    }

    return Base::FileInfo::pathToString(fsPath.lexically_normal());
}

std::string shardReferenceKey(
    const std::string& normalizedDocumentPath,
    const std::string& objectName
)
{
    return normalizedDocumentPath + '\x1f' + objectName;
}

std::string normalizedMasterPath(const StepLightweightManifest& manifest)
{
    return normalizePath(manifest.masterDocumentPath);
}

std::string cacheDirectoryForManifest(const StepLightweightManifest& manifest)
{
    if (manifest.masterDocumentPath.empty()) {
        return {};
    }

    return Base::FileInfo::pathToString(
        Base::FileInfo::stringToPath(manifest.masterDocumentPath).parent_path().lexically_normal()
    );
}

bool shardDocumentPathBelongsToCacheDirectory(
    const std::string& documentPath,
    const std::string& cacheDirectory
)
{
    if (documentPath.empty() || cacheDirectory.empty()) {
        return false;
    }

    const std::filesystem::path documentFsPath
        = Base::FileInfo::stringToPath(documentPath).lexically_normal();
    if (documentFsPath.parent_path().filename() != "shards") {
        return false;
    }

    return Base::FileInfo::pathToString(documentFsPath.parent_path().parent_path())
        == normalizePath(cacheDirectory);
}

std::string persistedPinnedShardsPath(const StepLightweightManifest& manifest)
{
    const std::string cacheDirectory = cacheDirectoryForManifest(manifest);
    if (cacheDirectory.empty()) {
        return {};
    }

    return Base::FileInfo::pathToString(
        (Base::FileInfo::stringToPath(cacheDirectory) / pinnedShardsFilename).lexically_normal()
    );
}

const WorkspaceSpatialShardEntry* spatialShardEntryForDocumentPath(
    const WorkspaceSpatialIndex* spatialIndex,
    const std::string& documentPath
)
{
    if (!spatialIndex) {
        return nullptr;
    }

    const std::string normalizedDocumentPath = normalizePath(documentPath);
    if (normalizedDocumentPath.empty()) {
        return nullptr;
    }

    const auto spatialIt = spatialIndex->shardIndicesByPath.find(normalizedDocumentPath);
    if (spatialIt == spatialIndex->shardIndicesByPath.end()
        || spatialIt->second >= spatialIndex->shards.size()) {
        return nullptr;
    }

    return &spatialIndex->shards[spatialIt->second];
}

std::ptrdiff_t manifestShardIndexForDocumentPath(
    const StepLightweightManifest& manifest,
    const WorkspaceSpatialIndex* spatialIndex,
    const std::string& documentPath
)
{
    if (const auto* spatialEntry = spatialShardEntryForDocumentPath(spatialIndex, documentPath)) {
        return static_cast<std::ptrdiff_t>(spatialEntry->manifestIndex);
    }

    const std::string normalizedDocumentPath = normalizePath(documentPath);
    for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
        if (normalizePath(manifest.shards[index].documentPath) == normalizedDocumentPath) {
            return static_cast<std::ptrdiff_t>(index);
        }
    }

    return -1;
}

WorkspaceRuntimeMetrics& runtimeMetricsForManifest(const StepLightweightManifest& manifest)
{
    return workspaceRuntimeMetrics[normalizedMasterPath(manifest)];
}

std::unordered_set<std::string>& loadedShardPathsForManifest(const StepLightweightManifest& manifest)
{
    return workspaceLoadedShardPaths[normalizedMasterPath(manifest)];
}

void clearLoadedShardPathCache(const StepLightweightManifest& manifest)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (normalizedWorkspacePath.empty()) {
        return;
    }

    workspaceLoadedShardPaths.erase(normalizedWorkspacePath);
    initializedLoadedShardPathCaches.erase(normalizedWorkspacePath);
}

void pruneLoadedShardPathsForManifest(const StepLightweightManifest& manifest)
{
    auto& loadedPaths = loadedShardPathsForManifest(manifest);
    for (auto it = loadedPaths.begin(); it != loadedPaths.end();) {
        App::Document* doc = App::GetApplication().getDocumentByPath(
            it->c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (!doc || doc->testStatus(App::Document::PartialDoc)) {
            it = loadedPaths.erase(it);
        }
        else {
            ++it;
        }
    }
}

std::unordered_set<std::string>& ensureLoadedShardPathsForManifest(const StepLightweightManifest& manifest)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    auto& loadedPaths = loadedShardPathsForManifest(manifest);
    if (!normalizedWorkspacePath.empty()
        && initializedLoadedShardPathCaches.insert(normalizedWorkspacePath).second) {
        for (const auto& shard : manifest.shards) {
            App::Document* doc = App::GetApplication().getDocumentByPath(
                shard.documentPath.c_str(),
                App::Application::PathMatchMode::MatchCanonicalWarning
            );
            if (doc && !doc->testStatus(App::Document::PartialDoc)) {
                loadedPaths.insert(normalizePath(shard.documentPath));
            }
        }
    }

    pruneLoadedShardPathsForManifest(manifest);
    return loadedPaths;
}

void markShardLoadedForManifest(const StepLightweightManifest& manifest, const std::string& documentPath)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    if (normalizedDocumentPath.empty()) {
        return;
    }

    ensureLoadedShardPathsForManifest(manifest).insert(normalizedDocumentPath);
}

void markShardUnloadedForManifest(const StepLightweightManifest& manifest, const std::string& documentPath)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    if (normalizedDocumentPath.empty()) {
        return;
    }

    loadedShardPathsForManifest(manifest).erase(normalizedDocumentPath);
}

std::size_t pendingInitialRestoreCountForDocument(const App::Document& doc)
{
    const auto it = workspacePendingInitialRestoreCounts.find(normalizePath(doc.FileName.getValue()));
    return it == workspacePendingInitialRestoreCounts.end() ? 0 : it->second;
}

void setPendingInitialRestoreCountForDocument(const App::Document& doc, std::size_t pendingCount)
{
    const std::string normalizedDocumentPath = normalizePath(doc.FileName.getValue());
    if (normalizedDocumentPath.empty()) {
        return;
    }

    if (pendingCount == 0) {
        workspacePendingInitialRestoreCounts.erase(normalizedDocumentPath);
        return;
    }

    workspacePendingInitialRestoreCounts[normalizedDocumentPath] = pendingCount;
}

void consumePendingInitialRestoreCountForDocument(
    const App::Document& doc,
    std::size_t consumedCount
)
{
    if (consumedCount == 0) {
        return;
    }

    const std::string normalizedDocumentPath = normalizePath(doc.FileName.getValue());
    if (normalizedDocumentPath.empty()) {
        return;
    }

    const auto it = workspacePendingInitialRestoreCounts.find(normalizedDocumentPath);
    if (it == workspacePendingInitialRestoreCounts.end()) {
        return;
    }

    if (consumedCount >= it->second) {
        workspacePendingInitialRestoreCounts.erase(it);
        return;
    }

    it->second -= consumedCount;
}

std::unordered_map<std::string, std::uint64_t>& pinnedShardsForManifest(
    const StepLightweightManifest& manifest
)
{
    return workspacePinnedShards[normalizedMasterPath(manifest)];
}

bool isPinnedShardForManifest(const StepLightweightManifest& manifest, const std::string& documentPath)
{
    const auto it = workspacePinnedShards.find(normalizedMasterPath(manifest));
    if (it == workspacePinnedShards.end()) {
        return false;
    }

    return it->second.find(normalizePath(documentPath)) != it->second.end();
}

std::uint64_t pinnedShardPriorityForManifest(
    const StepLightweightManifest& manifest,
    const std::string& documentPath
)
{
    const auto workspaceIt = workspacePinnedShards.find(normalizedMasterPath(manifest));
    if (workspaceIt == workspacePinnedShards.end()) {
        return 0;
    }

    const auto pinnedIt = workspaceIt->second.find(normalizePath(documentPath));
    return pinnedIt == workspaceIt->second.end() ? 0 : pinnedIt->second;
}

std::uint64_t effectiveShardPriorityForManifest(
    const StepLightweightManifest& manifest,
    const std::string& documentPath
)
{
    const std::uint64_t accessStamp = accessStampForPath(documentPath);
    if (accessStamp != 0) {
        return accessStamp;
    }

    return pinnedShardPriorityForManifest(manifest, documentPath);
}

void setPinnedShardForManifest(
    const StepLightweightManifest& manifest,
    const std::string& documentPath,
    bool pinned,
    std::uint64_t priority = 0
)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    if (normalizedDocumentPath.empty()) {
        return;
    }

    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (normalizedWorkspacePath.empty()) {
        return;
    }

    if (pinned) {
        if (priority == 0) {
            priority = nextShardAccessStampValue++;
        }
        pinnedShardsForManifest(manifest)[normalizedDocumentPath] = priority;
        return;
    }

    const auto it = workspacePinnedShards.find(normalizedWorkspacePath);
    if (it == workspacePinnedShards.end()) {
        return;
    }

    it->second.erase(normalizedDocumentPath);
    if (it->second.empty()) {
        workspacePinnedShards.erase(it);
    }
}

void loadPersistedPinnedShardsForManifest(const StepLightweightManifest& manifest)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (normalizedWorkspacePath.empty()
        || workspacePinnedShards.find(normalizedWorkspacePath) != workspacePinnedShards.end()) {
        return;
    }

    std::unordered_map<std::string, std::uint64_t> pinnedShards;
    const std::string statePath = persistedPinnedShardsPath(manifest);
    if (!statePath.empty()) {
        std::ifstream stream(statePath);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) {
                continue;
            }

            std::istringstream input(line);
            std::string key;
            input >> key;
            if (key != "pinned") {
                continue;
            }

            std::string documentPath;
            input >> std::quoted(documentPath);
            if (!documentPath.empty()) {
                std::uint64_t priority = 0;
                input >> priority;
                const std::string normalizedDocumentPath = normalizePath(documentPath);
                pinnedShards[normalizedDocumentPath] = priority;
                if (priority != 0) {
                    shardAccessStamps[normalizedDocumentPath]
                        = std::max(accessStampForPath(normalizedDocumentPath), priority);
                    nextShardAccessStampValue = std::max(nextShardAccessStampValue, priority + 1);
                }
            }
        }
    }

    workspacePinnedShards[normalizedWorkspacePath] = std::move(pinnedShards);
}

void savePersistedPinnedShardsForManifest(const StepLightweightManifest& manifest)
{
    const std::string statePath = persistedPinnedShardsPath(manifest);
    if (statePath.empty()) {
        return;
    }

    const auto it = workspacePinnedShards.find(normalizedMasterPath(manifest));
    if (it == workspacePinnedShards.end() || it->second.empty()) {
        std::error_code error;
        std::filesystem::remove(Base::FileInfo::stringToPath(statePath), error);
        return;
    }

    std::ofstream stream(statePath, std::ios::trunc);
    if (!stream.is_open()) {
        return;
    }

    std::vector<std::pair<std::string, std::uint64_t>> pinnedShards(
        it->second.begin(),
        it->second.end()
    );
    std::sort(
        pinnedShards.begin(),
        pinnedShards.end(),
        [](const auto& left, const auto& right) {
            if (left.second != right.second) {
                return left.second > right.second;
            }

            return left.first < right.first;
        }
    );

    for (const auto& [documentPath, priority] : pinnedShards) {
        stream << "pinned " << std::quoted(documentPath) << ' ' << priority << '\n';
    }
}

const char* shardLoadSourceName(ShardLoadSource source)
{
    switch (source) {
        case ShardLoadSource::Manual:
            return manualLoadSourceName;
        case ShardLoadSource::Prefetch:
            return prefetchLoadSourceName;
        case ShardLoadSource::Initial:
        default:
            return initialLoadSourceName;
    }
}

bool trackedShardLoadSource(const std::string& documentPath, ShardLoadSource& source)
{
    const auto it = shardLoadSources.find(normalizePath(documentPath));
    if (it == shardLoadSources.end()) {
        return false;
    }

    source = it->second;
    return true;
}

void setTrackedShardLoadSource(
    const StepLightweightManifest& manifest,
    const std::string& documentPath,
    ShardLoadSource source
)
{
    shardLoadSources[normalizePath(documentPath)] = source;

    auto& metrics = runtimeMetricsForManifest(manifest);
    switch (source) {
        case ShardLoadSource::Manual:
            ++metrics.manualLoadEventCount;
            break;
        case ShardLoadSource::Prefetch:
            ++metrics.prefetchEventCount;
            break;
        case ShardLoadSource::Initial:
        default:
            break;
    }
}

void clearTrackedShardLoadSource(const std::string& documentPath)
{
    shardLoadSources.erase(normalizePath(documentPath));
}

bool linkedShardReference(
    const App::DocumentObject& object,
    std::string& documentPath,
    std::string& objectName
);

bool manifestForCacheDirectory(const std::string& cacheDirectory, StepLightweightManifest& manifest)
{
    if (cacheDirectory.empty()) {
        return false;
    }

    Base::FileInfo manifestFile(cacheDirectory + "/manifest.txt");
    return manifestFile.exists() && manifest.load(manifestFile.filePath());
}

bool manifestMatchesLinkedShardReference(
    const StepLightweightManifest& manifest,
    const std::string& documentPath,
    const std::string& objectName
)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    return std::any_of(
        manifest.shards.begin(),
        manifest.shards.end(),
        [&](const StepLightweightShard& shard) {
            return normalizePath(shard.documentPath) == normalizedDocumentPath
                && shard.assemblyObjectName == objectName;
        }
    );
}

bool manifestForWorkspaceMetadataObject(const App::Document& doc, StepLightweightManifest& manifest)
{
    for (auto* object : doc.getObjects()) {
        if (!object) {
            continue;
        }

        const auto* property = dynamic_cast<const App::PropertyString*>(
            object->getPropertyByName(lightweightWorkspaceManifestPathPropertyName)
        );
        if (!property) {
            continue;
        }

        const std::string manifestPath = absolutePathFromOwner(doc, property->getValue());
        if (manifestPath.empty()) {
            continue;
        }

        if (Base::FileInfo(manifestPath).exists() && manifest.load(manifestPath)) {
            return true;
        }
    }

    return false;
}

bool manifestForWorkspaceLinkTargets(const App::Document& doc, StepLightweightManifest& manifest)
{
    for (auto* object : doc.getObjects()) {
        if (!object) {
            continue;
        }

        const auto* workspaceShardProperty = dynamic_cast<const App::PropertyBool*>(
            object->getPropertyByName(lightweightWorkspaceShardLinkPropertyName)
        );
        if (workspaceShardProperty && !workspaceShardProperty->getValue()) {
            continue;
        }

        std::string documentPath;
        std::string objectName;
        if (!linkedShardReference(*object, documentPath, objectName)) {
            continue;
        }

        const std::filesystem::path shardPath
            = Base::FileInfo::stringToPath(documentPath).lexically_normal();
        if (shardPath.parent_path().filename() != "shards") {
            continue;
        }

        StepLightweightManifest candidate;
        if (!manifestForCacheDirectory(
                Base::FileInfo::pathToString(shardPath.parent_path().parent_path()),
                candidate
            )) {
            continue;
        }

        if (manifestMatchesLinkedShardReference(candidate, documentPath, objectName)) {
            manifest = std::move(candidate);
            return true;
        }
    }

    return false;
}

void rebuildDocumentManifestShardIndex(DocumentManifestCacheEntry& cacheEntry)
{
    cacheEntry.shardIndexByReferenceKey.clear();
    cacheEntry.shardIndexByReferenceKey.reserve(cacheEntry.manifest.shards.size());
    for (std::size_t index = 0; index < cacheEntry.manifest.shards.size(); ++index) {
        const auto& shard = cacheEntry.manifest.shards[index];
        const std::string normalizedDocumentPath = normalizePath(shard.documentPath);
        if (normalizedDocumentPath.empty() || shard.assemblyObjectName.empty()) {
            continue;
        }

        cacheEntry.shardIndexByReferenceKey.try_emplace(
            shardReferenceKey(normalizedDocumentPath, shard.assemblyObjectName),
            index
        );
    }
}

bool buildDocumentManifestCacheEntry(
    const App::Document& doc,
    DocumentManifestCacheEntry& cacheEntry
)
{
    cacheEntry = DocumentManifestCacheEntry();
    cacheEntry.normalizedDocumentPath = normalizePath(doc.FileName.getValue());
    cacheEntry.objectCount = doc.getObjects().size();
    if (cacheEntry.normalizedDocumentPath.empty()) {
        return false;
    }

    if (manifestForWorkspaceMetadataObject(doc, cacheEntry.manifest)
        || manifestForWorkspaceLinkTargets(doc, cacheEntry.manifest)) {
        cacheEntry.role = WorkspaceDocumentRole::Root;
        rebuildDocumentManifestShardIndex(cacheEntry);
        return true;
    }

    const std::filesystem::path fsPath
        = Base::FileInfo::stringToPath(doc.FileName.getValue()).lexically_normal();
    const std::string filename = fsPath.filename().string();

    std::filesystem::path cacheDir;
    if (filename == "master.fcstd") {
        cacheDir = fsPath.parent_path();
    }
    else if (fsPath.parent_path().filename() == "shards") {
        cacheDir = fsPath.parent_path().parent_path();
    }
    else {
        return false;
    }

    if (!manifestForCacheDirectory(Base::FileInfo::pathToString(cacheDir), cacheEntry.manifest)) {
        return false;
    }

    cacheEntry.role = filename == "master.fcstd"
        ? WorkspaceDocumentRole::Root
        : WorkspaceDocumentRole::Shard;
    rebuildDocumentManifestShardIndex(cacheEntry);
    return true;
}

const DocumentManifestCacheEntry* documentManifestCacheEntryForDocument(const App::Document& doc)
{
    const std::string normalizedDocumentPath = normalizePath(doc.FileName.getValue());
    if (normalizedDocumentPath.empty()) {
        workspaceDocumentManifestCaches.erase(&doc);
        return nullptr;
    }

    auto [it, inserted] = workspaceDocumentManifestCaches.try_emplace(&doc);
    auto& cacheEntry = it->second;
    if (inserted || cacheEntry.normalizedDocumentPath != normalizedDocumentPath
        || cacheEntry.objectCount != doc.getObjects().size()) {
        if (!buildDocumentManifestCacheEntry(doc, cacheEntry)) {
            workspaceDocumentManifestCaches.erase(it);
            return nullptr;
        }
    }

    return &cacheEntry;
}

const StepLightweightShard* manifestShardForReference(
    const DocumentManifestCacheEntry& cacheEntry,
    const std::string& documentPath,
    const std::string& objectName
)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    if (normalizedDocumentPath.empty() || objectName.empty()) {
        return nullptr;
    }

    const auto it = cacheEntry.shardIndexByReferenceKey.find(
        shardReferenceKey(normalizedDocumentPath, objectName)
    );
    if (it == cacheEntry.shardIndexByReferenceKey.end()
        || it->second >= cacheEntry.manifest.shards.size()) {
        return nullptr;
    }

    return &cacheEntry.manifest.shards[it->second];
}

WorkspaceDocumentRole workspaceDocumentRoleForDocument(
    const App::Document& doc,
    StepLightweightManifest& manifest
)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
    if (!cacheEntry) {
        return WorkspaceDocumentRole::None;
    }

    manifest = cacheEntry->manifest;
    return cacheEntry->role;
}

bool manifestForDocument(const App::Document& doc, StepLightweightManifest& manifest)
{
    return workspaceDocumentRoleForDocument(doc, manifest) != WorkspaceDocumentRole::None;
}

App::Document* workspaceRootDocument(
    const App::Document& doc,
    const StepLightweightManifest& manifest,
    WorkspaceDocumentRole role
)
{
    if (role == WorkspaceDocumentRole::Root) {
        return const_cast<App::Document*>(&doc);
    }
    if (role != WorkspaceDocumentRole::Shard) {
        return nullptr;
    }

    return App::GetApplication().getDocumentByPath(
        manifest.masterDocumentPath.c_str(),
        App::Application::PathMatchMode::MatchCanonicalWarning
    );
}

bool shouldDeferInitialRestore(
    const App::Document& workspaceDoc,
    const StepLightweightManifest& manifest
)
{
    const std::string normalizedDocumentPath = normalizePath(workspaceDoc.FileName.getValue());
    const std::string normalizedManifestMasterPath = normalizedMasterPath(manifest);
    return !normalizedDocumentPath.empty() && !normalizedManifestMasterPath.empty()
        && normalizedDocumentPath != normalizedManifestMasterPath;
}

const App::PropertyXLink* linkedShardProperty(const App::DocumentObject& object)
{
    return dynamic_cast<const App::PropertyXLink*>(object.getPropertyByName("LinkedObject"));
}

App::PropertyXLink* linkedShardProperty(App::DocumentObject& object)
{
    return dynamic_cast<App::PropertyXLink*>(object.getPropertyByName("LinkedObject"));
}

bool isWorkspaceShardLink(const App::DocumentObject& object)
{
    const auto* property = dynamic_cast<const App::PropertyBool*>(
        object.getPropertyByName(lightweightWorkspaceShardLinkPropertyName)
    );
    return property && property->getValue();
}

bool workspaceShardReferenceMetadata(
    const App::DocumentObject& object,
    std::string& documentPath,
    std::string& objectName
)
{
    if (!isWorkspaceShardLink(object)) {
        return false;
    }

    const auto* documentPathProperty = dynamic_cast<const App::PropertyString*>(
        object.getPropertyByName(lightweightWorkspaceShardDocumentPathPropertyName)
    );
    const auto* objectNameProperty = dynamic_cast<const App::PropertyString*>(
        object.getPropertyByName(lightweightWorkspaceShardObjectNamePropertyName)
    );
    if (!documentPathProperty || !objectNameProperty) {
        return false;
    }

    documentPath = absolutePathFromOwner(*object.getDocument(), documentPathProperty->getValue());
    objectName = objectNameProperty->getValue();
    return !documentPath.empty() && !objectName.empty();
}

bool linkedShardReference(
    const App::DocumentObject& object,
    std::string& documentPath,
    std::string& objectName
)
{
    if (workspaceShardReferenceMetadata(object, documentPath, objectName)) {
        return true;
    }

    const auto* property = linkedShardProperty(object);
    if (!property) {
        return false;
    }

    documentPath = absolutePathFromOwner(
        *object.getDocument(),
        property->getDocumentPath()[0] != '\0' ? property->getDocumentPath() : property->getFilePath()
    );
    objectName = property->getObjectName();
    return !documentPath.empty() && !objectName.empty();
}

bool clearLinkedShardTarget(App::DocumentObject& object)
{
    auto* property = linkedShardProperty(object);
    if (!property) {
        return false;
    }

    if (!property->getValue() && property->getDocumentPath()[0] == '\0'
        && property->getObjectName()[0] == '\0') {
        property->setAllowPartial(true);
        return false;
    }

    property->setAllowPartial(true);
    property->setValue(nullptr);
    return true;
}

bool ensureLinkedShardTarget(App::DocumentObject& object, bool* targetChanged = nullptr)
{
    if (targetChanged) {
        *targetChanged = false;
    }

    auto* property = linkedShardProperty(object);
    if (!property) {
        return false;
    }

    std::string documentPath;
    std::string objectName;
    if (!workspaceShardReferenceMetadata(object, documentPath, objectName)) {
        return false;
    }

    const std::string currentPath = property->getDocumentPath()[0] != '\0'
        ? absolutePathFromOwner(*object.getDocument(), property->getDocumentPath())
        : absolutePathFromOwner(*object.getDocument(), property->getFilePath());
    const std::string currentObjectName = property->getObjectName();
    property->setAllowPartial(true);
    if (normalizePath(currentPath) == normalizePath(documentPath) && currentObjectName == objectName
        && property->getValue()) {
        return true;
    }

    property->setValue(std::string(documentPath), std::string(objectName), {}, {});
    const bool hasValue = property->getValue() != nullptr;
    if (targetChanged) {
        *targetChanged = hasValue;
    }
    return hasValue;
}

App::DocumentObject* workspaceShardLinkObjectForDocumentPath(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::string& documentPath
)
{
    if (const auto* spatialEntry = spatialShardEntryForDocumentPath(
            ensureWorkspaceSpatialIndex(manifest, workspaceDoc),
            documentPath
        )) {
        return workspaceDoc.getObject(spatialEntry->linkObjectName.c_str());
    }

    return nullptr;
}

std::uint64_t nextAccessStamp()
{
    return nextShardAccessStampValue++;
}

void noteDocumentPathAccess(const std::string& documentPath)
{
    if (documentPath.empty()) {
        return;
    }
    shardAccessStamps[normalizePath(documentPath)] = nextAccessStamp();
}

std::uint64_t accessStampForPath(const std::string& documentPath)
{
    const auto it = shardAccessStamps.find(normalizePath(documentPath));
    return it == shardAccessStamps.end() ? 0 : it->second;
}

void updatePinnedShardPriorityForManifest(
    const StepLightweightManifest& manifest,
    const std::string& documentPath
)
{
    if (!isPinnedShardForManifest(manifest, documentPath)) {
        return;
    }

    const std::uint64_t priority = accessStampForPath(documentPath);
    if (priority == 0) {
        return;
    }

    auto& pinnedShards = pinnedShardsForManifest(manifest);
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    const auto it = pinnedShards.find(normalizedDocumentPath);
    if (it != pinnedShards.end() && it->second == priority) {
        return;
    }

    pinnedShards[normalizedDocumentPath] = priority;
    savePersistedPinnedShardsForManifest(manifest);
}

std::ptrdiff_t manifestShardIndex(
    const StepLightweightManifest& manifest,
    const std::string& documentPath,
    const std::string& objectName
)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
        const auto& shard = manifest.shards[index];
        if (normalizePath(shard.documentPath) == normalizedDocumentPath
            && shard.assemblyObjectName == objectName) {
            return static_cast<std::ptrdiff_t>(index);
        }
    }

    return -1;
}

bool lightweightProxyBounds(const App::DocumentObject& object, Base::BoundBox3d& bounds)
{
    const auto* minProperty = dynamic_cast<const App::PropertyVector*>(
        object.getPropertyByName(lightweightProxyBoundsMinPropertyName)
    );
    const auto* maxProperty = dynamic_cast<const App::PropertyVector*>(
        object.getPropertyByName(lightweightProxyBoundsMaxPropertyName)
    );
    if (!minProperty || !maxProperty) {
        return false;
    }

    const Base::Vector3d min = minProperty->getValue();
    const Base::Vector3d max = maxProperty->getValue();
    bounds = Base::BoundBox3d(min.x, min.y, min.z, max.x, max.y, max.z);
    return bounds.IsValid();
}

Base::Vector3d lightweightProxyBoundsCenter(const Base::BoundBox3d& bounds)
{
    return Base::Vector3d(
        (bounds.MinX + bounds.MaxX) * 0.5,
        (bounds.MinY + bounds.MaxY) * 0.5,
        (bounds.MinZ + bounds.MaxZ) * 0.5
    );
}

void clearWorkspaceSpatialIndex(const StepLightweightManifest& manifest)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (!normalizedWorkspacePath.empty()) {
        workspaceSpatialIndices.erase(normalizedWorkspacePath);
    }
}

bool rebuildWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc,
    WorkspaceSpatialIndex& spatialIndex
)
{
    spatialIndex.shards.clear();
    spatialIndex.shardIndicesByPath.clear();
    spatialIndex.shardIndicesByCell.clear();
    spatialIndex.shardIndicesWithoutSpatialCenter.clear();
    spatialIndex.spatialCenterBounds = Base::BoundBox3d();
    spatialIndex.cellSize = 0.0;
    spatialIndex.hasSpatialGrid = false;
    spatialIndex.shards.reserve(manifest.shards.size());
    spatialIndex.shardIndicesByPath.reserve(manifest.shards.size());

    std::size_t spatialCenterCount = 0;
    for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
        const auto& shard = manifest.shards[index];

        WorkspaceSpatialShardEntry entry;
        entry.documentPath = shard.documentPath;
        entry.normalizedDocumentPath = normalizePath(shard.documentPath);
        entry.linkObjectName = shard.linkObjectName;
        entry.manifestIndex = index;

        if (auto* linkObject = masterDoc.getObject(shard.linkObjectName.c_str())) {
            Base::BoundBox3d bounds;
            if (lightweightProxyBounds(*linkObject, bounds)) {
                entry.center = lightweightProxyBoundsCenter(bounds);
                entry.hasSpatialCenter = true;
                if (spatialCenterCount == 0) {
                    spatialIndex.spatialCenterBounds = bounds;
                }
                else {
                    spatialIndex.spatialCenterBounds.Add(bounds);
                }
                ++spatialCenterCount;
            }
        }

        spatialIndex.shardIndicesByPath[entry.normalizedDocumentPath]
            = spatialIndex.shards.size();
        spatialIndex.shards.push_back(std::move(entry));
    }

    if (spatialCenterCount != 0 && spatialIndex.spatialCenterBounds.IsValid()) {
        const double extentX
            = std::max(0.0, spatialIndex.spatialCenterBounds.MaxX - spatialIndex.spatialCenterBounds.MinX);
        const double extentY
            = std::max(0.0, spatialIndex.spatialCenterBounds.MaxY - spatialIndex.spatialCenterBounds.MinY);
        const double extentZ
            = std::max(0.0, spatialIndex.spatialCenterBounds.MaxZ - spatialIndex.spatialCenterBounds.MinZ);
        const double maxExtent = std::max({extentX, extentY, extentZ, 1.0});
        const double preferredCellsPerAxis
            = std::max(1.0, std::cbrt(static_cast<double>(spatialCenterCount)));
        spatialIndex.cellSize = std::max(1.0, maxExtent / preferredCellsPerAxis);
        spatialIndex.hasSpatialGrid = spatialIndex.cellSize > 0.0;
    }

    for (std::size_t index = 0; index < spatialIndex.shards.size(); ++index) {
        const auto& entry = spatialIndex.shards[index];
        if (!entry.hasSpatialCenter || !spatialIndex.hasSpatialGrid) {
            spatialIndex.shardIndicesWithoutSpatialCenter.push_back(index);
            continue;
        }

        spatialIndex.shardIndicesByCell[spatialCellCoordForPoint(spatialIndex, entry.center)].push_back(index);
    }

    return !spatialIndex.shards.empty();
}

WorkspaceSpatialIndex* ensureWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc
)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (normalizedWorkspacePath.empty()) {
        return nullptr;
    }

    auto [it, inserted] = workspaceSpatialIndices.try_emplace(normalizedWorkspacePath);
    WorkspaceSpatialIndex& spatialIndex = it->second;
    if (inserted || spatialIndex.shards.size() != manifest.shards.size()) {
        if (!rebuildWorkspaceSpatialIndex(manifest, masterDoc, spatialIndex)) {
            workspaceSpatialIndices.erase(it);
            return nullptr;
        }
    }

    return &spatialIndex;
}

void refreshWorkspaceSpatialIndex(
    const StepLightweightManifest& manifest,
    App::Document& masterDoc
)
{
    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    if (normalizedWorkspacePath.empty()) {
        return;
    }

    auto [it, inserted] = workspaceSpatialIndices.try_emplace(normalizedWorkspacePath);
    if (!rebuildWorkspaceSpatialIndex(manifest, masterDoc, it->second)) {
        workspaceSpatialIndices.erase(it);
    }
}

WorkspaceSpatialCellCoord spatialCellCoordForPoint(
    const WorkspaceSpatialIndex& spatialIndex,
    const Base::Vector3d& point
)
{
    if (!spatialIndex.hasSpatialGrid || spatialIndex.cellSize <= 0.0) {
        return {};
    }

    return {
        static_cast<int>(std::floor((point.x - spatialIndex.spatialCenterBounds.MinX)
                                    / spatialIndex.cellSize)),
        static_cast<int>(std::floor((point.y - spatialIndex.spatialCenterBounds.MinY)
                                    / spatialIndex.cellSize)),
        static_cast<int>(std::floor((point.z - spatialIndex.spatialCenterBounds.MinZ)
                                    / spatialIndex.cellSize)),
    };
}

std::vector<std::size_t> collectSpatialManifestIndicesNearPoint(
    const WorkspaceSpatialIndex& spatialIndex,
    const Base::Vector3d& focusPoint,
    std::size_t desiredCount
)
{
    if (desiredCount == 0) {
        return {};
    }

    std::unordered_set<std::size_t> seenManifestIndices;
    if (!spatialIndex.hasSpatialGrid || spatialIndex.shardIndicesByCell.empty()) {
        std::vector<std::size_t> manifestIndices;
        manifestIndices.reserve(
            std::min(desiredCount, spatialIndex.shards.size())
                + spatialIndex.shardIndicesWithoutSpatialCenter.size()
        );
        for (const auto& shard : spatialIndex.shards) {
            if (seenManifestIndices.insert(shard.manifestIndex).second) {
                manifestIndices.push_back(shard.manifestIndex);
            }
            if (manifestIndices.size() >= desiredCount) {
                break;
            }
        }
        for (const auto index : spatialIndex.shardIndicesWithoutSpatialCenter) {
            if (index < spatialIndex.shards.size()
                && seenManifestIndices.insert(spatialIndex.shards[index].manifestIndex).second) {
                manifestIndices.push_back(spatialIndex.shards[index].manifestIndex);
            }
        }
        return manifestIndices;
    }

    std::vector<std::size_t> manifestIndices;
    manifestIndices.reserve(
        std::min(desiredCount, spatialIndex.shards.size())
            + spatialIndex.shardIndicesWithoutSpatialCenter.size()
    );
    const WorkspaceSpatialCellCoord focusCell = spatialCellCoordForPoint(spatialIndex, focusPoint);
    const int maxShell = 6;
    for (int shell = 0; shell <= maxShell && manifestIndices.size() < desiredCount; ++shell) {
        for (int dx = -shell; dx <= shell; ++dx) {
            for (int dy = -shell; dy <= shell; ++dy) {
                for (int dz = -shell; dz <= shell; ++dz) {
                    if (std::max({std::abs(dx), std::abs(dy), std::abs(dz)}) != shell) {
                        continue;
                    }

                    const WorkspaceSpatialCellCoord cell {
                        focusCell.x + dx,
                        focusCell.y + dy,
                        focusCell.z + dz,
                    };
                    const auto cellIt = spatialIndex.shardIndicesByCell.find(cell);
                    if (cellIt == spatialIndex.shardIndicesByCell.end()) {
                        continue;
                    }

                    for (const auto index : cellIt->second) {
                        if (index < spatialIndex.shards.size()
                            && seenManifestIndices.insert(spatialIndex.shards[index].manifestIndex).second) {
                            manifestIndices.push_back(spatialIndex.shards[index].manifestIndex);
                        }
                    }

                    if (manifestIndices.size() >= desiredCount) {
                        break;
                    }
                }
                if (manifestIndices.size() >= desiredCount) {
                    break;
                }
            }
            if (manifestIndices.size() >= desiredCount) {
                break;
            }
        }
    }

    if (manifestIndices.size() < desiredCount) {
        for (const auto& shard : spatialIndex.shards) {
            if (manifestIndices.size() >= desiredCount) {
                break;
            }

            if (!seenManifestIndices.insert(shard.manifestIndex).second) {
                continue;
            }

            manifestIndices.push_back(shard.manifestIndex);
        }
    }

    for (const auto index : spatialIndex.shardIndicesWithoutSpatialCenter) {
        if (index < spatialIndex.shards.size()
            && seenManifestIndices.insert(spatialIndex.shards[index].manifestIndex).second) {
            manifestIndices.push_back(spatialIndex.shards[index].manifestIndex);
        }
    }

    return manifestIndices;
}

std::vector<std::string> captureLoadedWorkspaceShardPaths(const StepLightweightManifest& manifest)
{
    const auto& loadedPaths = ensureLoadedShardPathsForManifest(manifest);
    std::vector<std::string> loadedShardPaths(loadedPaths.begin(), loadedPaths.end());
    std::sort(loadedShardPaths.begin(), loadedShardPaths.end());
    return loadedShardPaths;
}

void prepareWorkspaceDocumentForLazySave(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::vector<std::string>& loadedShardPaths
)
{
    for (const auto& documentPath : loadedShardPaths) {
        if (auto* linkObject = workspaceShardLinkObjectForDocumentPath(
                workspaceDoc,
                manifest,
                documentPath
            )) {
            clearLinkedShardProxyContents(*linkObject);
            clearLinkedShardTarget(*linkObject);
            linkObject->purgeTouched();
        }
    }

    workspaceDoc.purgeTouched();
    refreshWorkspaceSpatialIndex(manifest, workspaceDoc);
}

void restoreWorkspaceDocumentAfterLazySave(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::vector<std::string>& loadedShardPaths
)
{
    for (const auto& documentPath : loadedShardPaths) {
        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (!shardDoc || shardDoc->testStatus(App::Document::PartialDoc)) {
            continue;
        }

        if (auto* linkObject = workspaceShardLinkObjectForDocumentPath(
                workspaceDoc,
                manifest,
                documentPath
            )) {
            refreshLinkedShardAfterLoad(*linkObject);
            linkObject->purgeTouched();
        }
    }

    StepLightweightWorkspaceRuntime::synchronizeLinkedShardProxies(workspaceDoc);
    workspaceDoc.purgeTouched();
}

void purgeWorkspaceTouchState(const App::Document& workspaceDoc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return;
    }

    App::Document* masterDoc
        = workspaceRootDocument(workspaceDoc, cacheEntry->manifest, cacheEntry->role);
    if (!masterDoc) {
        masterDoc = const_cast<App::Document*>(&workspaceDoc);
    }

    for (const auto& shard : cacheEntry->manifest.shards) {
        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            shardDoc->purgeTouched();
        }
    }

    masterDoc->purgeTouched();
}

bool isLightweightProxyObject(const App::DocumentObject& object)
{
    const auto* marker = dynamic_cast<const App::PropertyBool*>(
        object.getPropertyByName(lightweightProxyPropertyName)
    );
    if (marker && marker->getValue()) {
        return true;
    }

    const std::string label = object.Label.getValue();
    if (label == lightweightProxyPrototypeLabel) {
        return false;
    }

    return label.rfind(lightweightProxyLabel, 0) == 0
        && (dynamic_cast<const App::Link*>(&object) || dynamic_cast<const PartApp::Feature*>(&object));
}

bool hasLightweightProxyChildren(const App::DocumentObject& object)
{
    const auto* part = dynamic_cast<const App::Part*>(&object);
    if (!part) {
        return false;
    }

    for (auto* child : part->Group.getValues()) {
        if (child && isLightweightProxyObject(*child)) {
            return true;
        }
    }

    return false;
}

bool refreshLinkedShardAfterLoad(App::DocumentObject& object)
{
    bool targetChanged = false;
    if (!ensureLinkedShardTarget(object, &targetChanged)) {
        return false;
    }

    if (targetChanged || hasLightweightProxyChildren(object)) {
        object.recomputeFeature(true);
    }

    return true;
}

bool hasLightweightProxy(const App::DocumentObject& object)
{
    std::string documentPath;
    std::string objectName;
    if (!linkedShardReference(object, documentPath, objectName)) {
        return false;
    }

    Base::BoundBox3d bounds;
    if (!lightweightProxyBounds(object, bounds)) {
        return false;
    }

    App::Document* openDoc = App::GetApplication().getDocumentByPath(
        documentPath.c_str(),
        App::Application::PathMatchMode::MatchCanonicalWarning
    );
    return !openDoc || openDoc->testStatus(App::Document::PartialDoc);
}

void updateShardOpenState(
    StepLightweightWorkspaceShardState& shardState,
    const std::string& documentPath
)
{
    App::Document* openDoc = App::GetApplication().getDocumentByPath(
        documentPath.c_str(),
        App::Application::PathMatchMode::MatchCanonicalWarning
    );
    if (!openDoc) {
        return;
    }

    shardState.isOpen = true;
    shardState.isPartial = openDoc->testStatus(App::Document::PartialDoc);
    shardState.isFullyLoaded = !shardState.isPartial;
}

void clearLinkedShardProxyContents(App::DocumentObject& object)
{
    auto* part = dynamic_cast<App::Part*>(&object);
    auto* doc = object.getDocument();
    if (!part || !doc) {
        return;
    }

    std::vector<App::DocumentObject*> group = part->Group.getValues();
    for (auto* child : group) {
        if (!child) {
            continue;
        }
        doc->removeObject(child->getNameInDocument());
    }
}

void clearProxyContentsForShardInWorkspaceDocument(
    App::Document& workspaceDoc,
    const std::string& documentPath
)
{
    const std::string normalizedDocumentPath = normalizePath(documentPath);
    for (auto* object : workspaceDoc.getObjects()) {
        if (!object) {
            continue;
        }

        std::string linkedPath;
        std::string objectName;
        if (!linkedShardReference(*object, linkedPath, objectName)) {
            continue;
        }

        if (normalizePath(linkedPath) == normalizedDocumentPath) {
            clearLinkedShardProxyContents(*object);
            clearLinkedShardTarget(*object);
        }
    }
}

void clearProxyContentsForShardInWorkspaceDocument(
    App::Document& workspaceDoc,
    const StepLightweightManifest& manifest,
    const std::string& documentPath
)
{
    if (auto* linkObject = workspaceShardLinkObjectForDocumentPath(workspaceDoc, manifest, documentPath)) {
        clearLinkedShardProxyContents(*linkObject);
        clearLinkedShardTarget(*linkObject);
        return;
    }

    clearProxyContentsForShardInWorkspaceDocument(workspaceDoc, documentPath);
}

bool removeLightweightProxyObjects(App::DocumentObject& object)
{
    auto* part = dynamic_cast<App::Part*>(&object);
    auto* doc = object.getDocument();
    if (!part || !doc) {
        return false;
    }

    std::vector<std::string> proxyNames;
    for (auto* child : part->Group.getValues()) {
        if (child && isLightweightProxyObject(*child)) {
            proxyNames.push_back(child->getNameInDocument());
        }
    }

    for (const auto& proxyName : proxyNames) {
        doc->removeObject(proxyName.c_str());
    }

    return !proxyNames.empty();
}

void removeLightweightProxyPrototypes(App::Document& document)
{
    std::vector<std::string> prototypeNames;
    for (auto* object : document.getObjects()) {
        auto* feature = dynamic_cast<PartApp::Feature*>(object);
        if (!feature) {
            continue;
        }

        const auto* marker = dynamic_cast<const App::PropertyBool*>(
            feature->getPropertyByName(lightweightProxyPrototypePropertyName)
        );
        if (marker && marker->getValue()) {
            prototypeNames.push_back(feature->getNameInDocument());
        }
    }

    for (const auto& prototypeName : prototypeNames) {
        document.removeObject(prototypeName.c_str());
    }
}

std::size_t fullyLoadedShardCountForManifest(const StepLightweightManifest& manifest)
{
    return ensureLoadedShardPathsForManifest(manifest).size();
}

bool closeLoadedShardInWorkspaceDocument(
    App::Document* rootDoc,
    const StepLightweightManifest& manifest,
    const std::string& documentPath,
    App::Document* doc
)
{
    if (!doc) {
        return false;
    }

    shardAccessStamps.erase(normalizePath(documentPath));
    if (!App::GetApplication().closeDocument(doc->getName())) {
        return false;
    }

    if (rootDoc) {
        clearProxyContentsForShardInWorkspaceDocument(*rootDoc, manifest, documentPath);
    }
    clearTrackedShardLoadSource(documentPath);
    markShardUnloadedForManifest(manifest, documentPath);
    ++runtimeMetricsForManifest(manifest).trimmedShardEventCount;
    return true;
}

std::size_t trimLoadedShardsImpl(
    const App::Document& workspaceDoc,
    int maxLoadedShards,
    const std::string& keepDocumentPath,
    bool allowPinnedTrim
)
{
    StepLightweightManifest manifest;
    const WorkspaceDocumentRole role = workspaceDocumentRoleForDocument(workspaceDoc, manifest);
    if (role == WorkspaceDocumentRole::None) {
        return 0;
    }
    loadPersistedPinnedShardsForManifest(manifest);
    App::Document* rootDoc = workspaceRootDocument(workspaceDoc, manifest, role);

    if (maxLoadedShards < 0) {
        maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    }

    struct LoadedShardCandidate
    {
        std::string documentPath;
        App::Document* doc = nullptr;
        bool isPinned = false;
    };

    const auto& loadedPaths = ensureLoadedShardPathsForManifest(manifest);
    std::vector<LoadedShardCandidate> fullyLoadedDocs;
    fullyLoadedDocs.reserve(loadedPaths.size());
    for (const auto& documentPath : loadedPaths) {
        App::Document* doc = App::GetApplication().getDocumentByPath(
            documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (!doc || doc->testStatus(App::Document::PartialDoc)) {
            continue;
        }
        fullyLoadedDocs.push_back(
            {
                documentPath,
                doc,
                isPinnedShardForManifest(manifest, documentPath),
            }
        );
    }

    if (maxLoadedShards < 0 || static_cast<int>(fullyLoadedDocs.size()) <= maxLoadedShards) {
        return 0;
    }

    const std::string normalizedKeepPath = normalizePath(keepDocumentPath);
    const auto lowerPriorityCandidate = [&](const LoadedShardCandidate& left,
                                            const LoadedShardCandidate& right) {
        const bool keepLeft = !normalizedKeepPath.empty() && left.documentPath == normalizedKeepPath;
        const bool keepRight = !normalizedKeepPath.empty() && right.documentPath == normalizedKeepPath;
        if (keepLeft != keepRight) {
            return !keepLeft && keepRight;
        }

        if (left.isPinned != right.isPinned) {
            return !left.isPinned && right.isPinned;
        }

        const std::uint64_t leftPriority
            = effectiveShardPriorityForManifest(manifest, left.documentPath);
        const std::uint64_t rightPriority
            = effectiveShardPriorityForManifest(manifest, right.documentPath);
        if (leftPriority != rightPriority) {
            return leftPriority < rightPriority;
        }

        return left.documentPath < right.documentPath;
    };
    int loadedDocCount = static_cast<int>(fullyLoadedDocs.size());
    auto candidateQueue = makeCandidatePriorityQueue(
        std::move(fullyLoadedDocs),
        lowerPriorityCandidate
    );

    std::size_t closed = 0;
    while (loadedDocCount > maxLoadedShards && !candidateQueue.empty()) {
        const auto candidate = candidateQueue.top();
        candidateQueue.pop();
        if (!normalizedKeepPath.empty() && candidate.documentPath == normalizedKeepPath) {
            break;
        }
        if (candidate.isPinned && !allowPinnedTrim) {
            break;
        }

        if (closeLoadedShardInWorkspaceDocument(
                rootDoc,
                manifest,
                candidate.documentPath,
                candidate.doc
            )) {
            ++closed;
            --loadedDocCount;
        }
    }

    return closed;
}

std::size_t trimLoadedShardsOutsideRetainedPathsImpl(
    const App::Document& workspaceDoc,
    const std::vector<std::string>& retainedDocumentPaths,
    int maxTrimCount
)
{
    StepLightweightManifest manifest;
    const WorkspaceDocumentRole role = workspaceDocumentRoleForDocument(workspaceDoc, manifest);
    if (role == WorkspaceDocumentRole::None || maxTrimCount == 0) {
        return 0;
    }
    loadPersistedPinnedShardsForManifest(manifest);
    App::Document* rootDoc = workspaceRootDocument(workspaceDoc, manifest, role);

    std::unordered_set<std::string> retainedPaths;
    retainedPaths.reserve(retainedDocumentPaths.size());
    for (const auto& retainedDocumentPath : retainedDocumentPaths) {
        const std::string normalizedRetainedPath = normalizePath(retainedDocumentPath);
        if (!normalizedRetainedPath.empty()) {
            retainedPaths.insert(normalizedRetainedPath);
        }
    }

    struct LoadedShardCandidate
    {
        std::string documentPath;
        App::Document* doc = nullptr;
        std::uint64_t priority = 0;
    };

    const auto& loadedPaths = ensureLoadedShardPathsForManifest(manifest);
    std::vector<LoadedShardCandidate> candidates;
    candidates.reserve(loadedPaths.size());
    for (const auto& documentPath : loadedPaths) {
        if (isPinnedShardForManifest(manifest, documentPath)) {
            continue;
        }

        if (retainedPaths.find(documentPath) != retainedPaths.end()) {
            continue;
        }

        App::Document* doc = App::GetApplication().getDocumentByPath(
            documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (!doc || doc->testStatus(App::Document::PartialDoc)) {
            continue;
        }

        candidates.push_back(
            {
                documentPath,
                doc,
                effectiveShardPriorityForManifest(manifest, documentPath),
            }
        );
    }

    const auto lowerPriorityCandidate = [](const LoadedShardCandidate& left,
                                           const LoadedShardCandidate& right) {
        if (left.priority != right.priority) {
            return left.priority < right.priority;
        }
        return left.documentPath < right.documentPath;
    };
    auto candidateQueue = makeCandidatePriorityQueue(
        std::move(candidates),
        lowerPriorityCandidate
    );

    std::size_t closed = 0;
    while (!candidateQueue.empty()) {
        if (maxTrimCount >= 0 && closed >= static_cast<std::size_t>(maxTrimCount)) {
            break;
        }

        const auto candidate = candidateQueue.top();
        candidateQueue.pop();

        if (closeLoadedShardInWorkspaceDocument(rootDoc, manifest, candidate.documentPath, candidate.doc)) {
            ++closed;
        }
    }

    return closed;
}

std::size_t prefetchAdjacentShardLinks(
    App::DocumentObject& object,
    const StepLightweightManifest& manifest,
    const std::string& keepDocumentPath
)
{
    auto* workspaceDoc = object.getDocument();
    if (!workspaceDoc) {
        return 0;
    }

    const int maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    if (maxLoadedShards <= 0) {
        return 0;
    }

    std::string documentPath;
    std::string objectName;
    if (!linkedShardReference(object, documentPath, objectName)) {
        return 0;
    }

    const std::ptrdiff_t currentIndex = manifestShardIndex(manifest, documentPath, objectName);
    if (currentIndex < 0) {
        return 0;
    }

    const auto workspaceState = StepLightweightWorkspaceRuntime::inspect(*workspaceDoc);
    if (workspaceState.fullyLoadedShardCount >= static_cast<std::size_t>(maxLoadedShards)) {
        return 0;
    }

    std::size_t availableSlots
        = static_cast<std::size_t>(maxLoadedShards) - workspaceState.fullyLoadedShardCount;
    std::size_t prefetched = 0;

    auto* spatialIndex = ensureWorkspaceSpatialIndex(manifest, *workspaceDoc);
    Base::BoundBox3d selectedBounds;
    const bool hasSelectedBounds = lightweightProxyBounds(object, selectedBounds);
    Base::Vector3d selectedCenter = hasSelectedBounds ? lightweightProxyBoundsCenter(selectedBounds)
                                                      : Base::Vector3d();
    bool hasSelectedCenter = hasSelectedBounds;
    const std::size_t normalizedCurrentIndex = static_cast<std::size_t>(currentIndex);
    if (!hasSelectedCenter && spatialIndex
        && normalizedCurrentIndex < spatialIndex->shards.size()
        && spatialIndex->shards[normalizedCurrentIndex].hasSpatialCenter) {
        selectedCenter = spatialIndex->shards[normalizedCurrentIndex].center;
        hasSelectedCenter = true;
    }

    struct PrefetchCandidate
    {
        std::size_t index = 0;
        bool hasSpatialDistance = false;
        double spatialDistanceSquared = std::numeric_limits<double>::max();
        std::size_t manifestOffset = 0;
    };

    std::vector<PrefetchCandidate> candidates;
    const std::size_t desiredCandidateCount
        = std::max<std::size_t>(32, availableSlots * 12);
    const std::vector<std::size_t> candidateIndices
        = hasSelectedCenter && spatialIndex
        ? collectSpatialManifestIndicesNearPoint(*spatialIndex, selectedCenter, desiredCandidateCount)
        : std::vector<std::size_t> {};
    const bool useSpatialSubset = !candidateIndices.empty();
    candidates.reserve(useSpatialSubset ? candidateIndices.size() : manifest.shards.size());
    auto appendCandidate = [&](std::size_t index) {
        if (index >= manifest.shards.size()) {
            return;
        }

        const auto& candidate = manifest.shards[index];
        if (normalizePath(candidate.documentPath) == normalizePath(keepDocumentPath)) {
            return;
        }

        PrefetchCandidate prefetchCandidate;
        prefetchCandidate.index = index;
        prefetchCandidate.manifestOffset = index >= normalizedCurrentIndex
            ? index - normalizedCurrentIndex
            : normalizedCurrentIndex - index;

        if (hasSelectedCenter) {
            if (spatialIndex && index < spatialIndex->shards.size()
                && spatialIndex->shards[index].hasSpatialCenter) {
                const Base::Vector3d& candidateCenter = spatialIndex->shards[index].center;
                const double dx = candidateCenter.x - selectedCenter.x;
                const double dy = candidateCenter.y - selectedCenter.y;
                const double dz = candidateCenter.z - selectedCenter.z;
                prefetchCandidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                prefetchCandidate.hasSpatialDistance = true;
            }
            else if (auto* candidateLink = workspaceDoc->getObject(candidate.linkObjectName.c_str())) {
                Base::BoundBox3d candidateBounds;
                if (lightweightProxyBounds(*candidateLink, candidateBounds)) {
                    const Base::Vector3d candidateCenter = lightweightProxyBoundsCenter(candidateBounds);
                    const double dx = candidateCenter.x - selectedCenter.x;
                    const double dy = candidateCenter.y - selectedCenter.y;
                    const double dz = candidateCenter.z - selectedCenter.z;
                    prefetchCandidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                    prefetchCandidate.hasSpatialDistance = true;
                }
            }
        }

        candidates.push_back(std::move(prefetchCandidate));
    };

    if (useSpatialSubset) {
        for (const auto index : candidateIndices) {
            appendCandidate(index);
        }
    }
    else {
        for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
            appendCandidate(index);
        }
    }

    const auto betterPrefetchCandidate = [](const PrefetchCandidate& left,
                                            const PrefetchCandidate& right) {
        if (left.hasSpatialDistance != right.hasSpatialDistance) {
            return left.hasSpatialDistance && !right.hasSpatialDistance;
        }
        if (left.hasSpatialDistance
            && left.spatialDistanceSquared != right.spatialDistanceSquared) {
            return left.spatialDistanceSquared < right.spatialDistanceSquared;
        }
        if (left.manifestOffset != right.manifestOffset) {
            return left.manifestOffset < right.manifestOffset;
        }
        return left.index < right.index;
    };
    auto candidateQueue = makeCandidatePriorityQueue(
        std::move(candidates),
        betterPrefetchCandidate
    );

    auto tryPrefetch = [&](std::size_t candidateIndex) {
        if (availableSlots == 0 || candidateIndex >= manifest.shards.size()) {
            return;
        }

        const auto& candidate = manifest.shards[candidateIndex];
        if (normalizePath(candidate.documentPath) == normalizePath(keepDocumentPath)) {
            return;
        }

        App::Document* candidateDoc = App::GetApplication().getDocumentByPath(
            candidate.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (candidateDoc && !candidateDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, candidate.documentPath);
            return;
        }

        candidateDoc = App::GetApplication().openDocument(candidate.documentPath.c_str());
        if (!candidateDoc) {
            return;
        }
        markShardLoadedForManifest(manifest, candidate.documentPath);

        auto* candidateLink = workspaceDoc->getObject(candidate.linkObjectName.c_str());
        if (!candidateLink) {
            return;
        }

        noteDocumentPathAccess(candidate.documentPath);
        refreshLinkedShardAfterLoad(*candidateLink);
        setTrackedShardLoadSource(manifest, candidate.documentPath, ShardLoadSource::Prefetch);
        --availableSlots;
        ++prefetched;
    };

    while (!candidateQueue.empty()) {
        if (availableSlots == 0) {
            break;
        }

        const auto candidate = candidateQueue.top();
        candidateQueue.pop();
        tryPrefetch(candidate.index);
    }

    if (prefetched != 0) {
        consumePendingInitialRestoreCountForDocument(*workspaceDoc, prefetched);
    }

    return prefetched;
}

std::size_t prefetchShardsNearPointImpl(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxPrefetchCount
)
{
    StepLightweightManifest manifest;
    const WorkspaceDocumentRole role = workspaceDocumentRoleForDocument(workspaceDoc, manifest);
    if (role == WorkspaceDocumentRole::None) {
        return 0;
    }
    loadPersistedPinnedShardsForManifest(manifest);

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc) {
        return 0;
    }

    const int maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    if (maxLoadedShards <= 0) {
        return 0;
    }

    std::size_t availableSlots = static_cast<std::size_t>(maxLoadedShards)
        - std::min(fullyLoadedShardCountForManifest(manifest), static_cast<std::size_t>(maxLoadedShards));
    if (availableSlots == 0) {
        return 0;
    }

    if (maxPrefetchCount >= 0) {
        availableSlots = std::min(availableSlots, static_cast<std::size_t>(maxPrefetchCount));
    }

    auto* spatialIndex = ensureWorkspaceSpatialIndex(manifest, *masterDoc);

    struct PrefetchCandidate
    {
        std::string linkObjectName;
        std::string documentPath;
        bool hasSpatialDistance = false;
        double spatialDistanceSquared = std::numeric_limits<double>::max();
        std::size_t manifestIndex = 0;
    };

    std::vector<PrefetchCandidate> candidates;
    const std::size_t desiredCandidateCount
        = std::max<std::size_t>(64, availableSlots * 16);
    const std::vector<std::size_t> candidateIndices = spatialIndex
        ? collectSpatialManifestIndicesNearPoint(*spatialIndex, focusPoint, desiredCandidateCount)
        : std::vector<std::size_t> {};
    const bool useSpatialSubset = !candidateIndices.empty();
    candidates.reserve(useSpatialSubset ? candidateIndices.size() : manifest.shards.size());
    auto appendCandidate = [&](std::size_t index) {
        if (index >= manifest.shards.size()) {
            return;
        }

        const auto& shard = manifest.shards[index];
        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, shard.documentPath);
            return;
        }

        PrefetchCandidate candidate;
        candidate.linkObjectName = shard.linkObjectName;
        candidate.documentPath = shard.documentPath;
        candidate.manifestIndex = index;

        if (spatialIndex && index < spatialIndex->shards.size()
            && spatialIndex->shards[index].hasSpatialCenter) {
            const Base::Vector3d& center = spatialIndex->shards[index].center;
            const double dx = center.x - focusPoint.x;
            const double dy = center.y - focusPoint.y;
            const double dz = center.z - focusPoint.z;
            candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
            candidate.hasSpatialDistance = true;
        }
        else if (auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str())) {
            Base::BoundBox3d bounds;
            if (lightweightProxyBounds(*linkObject, bounds)) {
                const Base::Vector3d center = lightweightProxyBoundsCenter(bounds);
                const double dx = center.x - focusPoint.x;
                const double dy = center.y - focusPoint.y;
                const double dz = center.z - focusPoint.z;
                candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                candidate.hasSpatialDistance = true;
            }
        }

        candidates.push_back(std::move(candidate));
    };

    if (useSpatialSubset) {
        for (const auto index : candidateIndices) {
            appendCandidate(index);
        }
    }
    else {
        for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
            appendCandidate(index);
        }
    }

    const auto betterPrefetchCandidate = [](const PrefetchCandidate& left,
                                            const PrefetchCandidate& right) {
        if (left.hasSpatialDistance != right.hasSpatialDistance) {
            return left.hasSpatialDistance && !right.hasSpatialDistance;
        }
        if (left.hasSpatialDistance
            && left.spatialDistanceSquared != right.spatialDistanceSquared) {
            return left.spatialDistanceSquared < right.spatialDistanceSquared;
        }
        return left.manifestIndex < right.manifestIndex;
    };
    auto candidateQueue = makeCandidatePriorityQueue(
        std::move(candidates),
        betterPrefetchCandidate
    );

    std::size_t prefetched = 0;
    while (!candidateQueue.empty()) {
        if (availableSlots == 0) {
            break;
        }

        const auto candidate = candidateQueue.top();
        candidateQueue.pop();

        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            candidate.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, candidate.documentPath);
            continue;
        }

        shardDoc = App::GetApplication().openDocument(candidate.documentPath.c_str());
        if (!shardDoc) {
            continue;
        }
        markShardLoadedForManifest(manifest, candidate.documentPath);

        auto* linkObject = masterDoc->getObject(candidate.linkObjectName.c_str());
        if (!linkObject) {
            continue;
        }

        noteDocumentPathAccess(candidate.documentPath);
        refreshLinkedShardAfterLoad(*linkObject);
        setTrackedShardLoadSource(manifest, candidate.documentPath, ShardLoadSource::Prefetch);
        --availableSlots;
        ++prefetched;
    }

    if (prefetched != 0 && masterDoc) {
        consumePendingInitialRestoreCountForDocument(*masterDoc, prefetched);
    }

    return prefetched;
}

std::size_t restoreDeferredInitialShardsNearPointImpl(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxRestoreCount
)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return 0;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc || !shouldDeferInitialRestore(*masterDoc, manifest)) {
        return 0;
    }

    const int maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    if (maxLoadedShards <= 0) {
        setPendingInitialRestoreCountForDocument(*masterDoc, 0);
        return 0;
    }

    std::size_t pendingRestoreCount = pendingInitialRestoreCountForDocument(*masterDoc);
    if (pendingRestoreCount == 0) {
        return 0;
    }

    std::size_t availableSlots = static_cast<std::size_t>(maxLoadedShards)
        - std::min(fullyLoadedShardCountForManifest(manifest), static_cast<std::size_t>(maxLoadedShards));
    if (availableSlots == 0) {
        setPendingInitialRestoreCountForDocument(*masterDoc, 0);
        return 0;
    }

    pendingRestoreCount = std::min(pendingRestoreCount, availableSlots);
    if (maxRestoreCount >= 0) {
        pendingRestoreCount = std::min(
            pendingRestoreCount,
            static_cast<std::size_t>(maxRestoreCount)
        );
    }
    if (pendingRestoreCount == 0) {
        setPendingInitialRestoreCountForDocument(*masterDoc, 0);
        return 0;
    }

    auto* spatialIndex = ensureWorkspaceSpatialIndex(manifest, *masterDoc);

    struct RestoreCandidate
    {
        std::string linkObjectName;
        std::string documentPath;
        bool hasSpatialDistance = false;
        double spatialDistanceSquared = std::numeric_limits<double>::max();
        std::size_t manifestIndex = 0;
    };

    std::vector<RestoreCandidate> candidates;
    const std::size_t desiredCandidateCount
        = std::max<std::size_t>(64, pendingRestoreCount * 16);
    const std::vector<std::size_t> candidateIndices = spatialIndex
        ? collectSpatialManifestIndicesNearPoint(*spatialIndex, focusPoint, desiredCandidateCount)
        : std::vector<std::size_t> {};
    const bool useSpatialSubset = !candidateIndices.empty();
    candidates.reserve(useSpatialSubset ? candidateIndices.size() : manifest.shards.size());
    auto appendCandidate = [&](std::size_t index) {
        if (index >= manifest.shards.size()) {
            return;
        }

        const auto& shard = manifest.shards[index];
        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, shard.documentPath);
            return;
        }

        RestoreCandidate candidate;
        candidate.linkObjectName = shard.linkObjectName;
        candidate.documentPath = shard.documentPath;
        candidate.manifestIndex = index;

        if (spatialIndex && index < spatialIndex->shards.size()
            && spatialIndex->shards[index].hasSpatialCenter) {
            const Base::Vector3d& center = spatialIndex->shards[index].center;
            const double dx = center.x - focusPoint.x;
            const double dy = center.y - focusPoint.y;
            const double dz = center.z - focusPoint.z;
            candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
            candidate.hasSpatialDistance = true;
        }
        else if (auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str())) {
            Base::BoundBox3d bounds;
            if (lightweightProxyBounds(*linkObject, bounds)) {
                const Base::Vector3d center = lightweightProxyBoundsCenter(bounds);
                const double dx = center.x - focusPoint.x;
                const double dy = center.y - focusPoint.y;
                const double dz = center.z - focusPoint.z;
                candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                candidate.hasSpatialDistance = true;
            }
        }

        candidates.push_back(std::move(candidate));
    };

    if (useSpatialSubset) {
        for (const auto index : candidateIndices) {
            appendCandidate(index);
        }
    }
    else {
        for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
            appendCandidate(index);
        }
    }

    const auto betterRestoreCandidate = [](const RestoreCandidate& left,
                                           const RestoreCandidate& right) {
        if (left.hasSpatialDistance != right.hasSpatialDistance) {
            return left.hasSpatialDistance && !right.hasSpatialDistance;
        }
        if (left.hasSpatialDistance
            && left.spatialDistanceSquared != right.spatialDistanceSquared) {
            return left.spatialDistanceSquared < right.spatialDistanceSquared;
        }
        return left.manifestIndex < right.manifestIndex;
    };
    auto candidateQueue = makeCandidatePriorityQueue(
        std::move(candidates),
        betterRestoreCandidate
    );

    std::size_t restored = 0;
    while (!candidateQueue.empty()) {
        if (restored >= pendingRestoreCount) {
            break;
        }

        const auto candidate = candidateQueue.top();
        candidateQueue.pop();

        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            candidate.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, candidate.documentPath);
            continue;
        }

        shardDoc = App::GetApplication().openDocument(candidate.documentPath.c_str());
        if (!shardDoc) {
            continue;
        }
        markShardLoadedForManifest(manifest, candidate.documentPath);

        auto* linkObject = masterDoc->getObject(candidate.linkObjectName.c_str());
        if (!linkObject) {
            continue;
        }

        noteDocumentPathAccess(candidate.documentPath);
        refreshLinkedShardAfterLoad(*linkObject);
        setTrackedShardLoadSource(manifest, candidate.documentPath, ShardLoadSource::Initial);
        ++restored;
    }

    if (restored != 0) {
        consumePendingInitialRestoreCountForDocument(*masterDoc, restored);
    }

    return restored;
}

std::size_t rebalanceShardsNearPointImpl(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxReplacementCount
)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return 0;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc) {
        return 0;
    }

    const int maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    if (maxLoadedShards <= 0 || maxReplacementCount == 0) {
        return 0;
    }

    std::size_t changed
        = prefetchShardsNearPointImpl(workspaceDoc, focusPoint, maxReplacementCount);
    if (maxReplacementCount > 0
        && changed >= static_cast<std::size_t>(maxReplacementCount)) {
        return changed;
    }

    std::size_t remainingChanges = maxReplacementCount < 0
        ? std::numeric_limits<std::size_t>::max()
        : static_cast<std::size_t>(maxReplacementCount) - changed;
    if (remainingChanges == 0) {
        return changed;
    }

    auto* spatialIndex = ensureWorkspaceSpatialIndex(manifest, *masterDoc);
    const auto& loadedPaths = ensureLoadedShardPathsForManifest(manifest);
    const std::size_t desiredUnloadedCandidateCount
        = std::max<std::size_t>(64, remainingChanges * 16);
    const std::vector<std::size_t> unloadedCandidateIndices = spatialIndex
        ? collectSpatialManifestIndicesNearPoint(
              *spatialIndex,
              focusPoint,
              desiredUnloadedCandidateCount
          )
        : std::vector<std::size_t> {};

    struct SpatialShardCandidate
    {
        std::string linkObjectName;
        App::Document* doc = nullptr;
        std::string documentPath;
        bool hasSpatialDistance = false;
        double spatialDistanceSquared = std::numeric_limits<double>::max();
        std::size_t manifestIndex = 0;
    };

    auto buildCandidates = [&]() {
        std::vector<SpatialShardCandidate> loadedCandidates;
        std::vector<SpatialShardCandidate> unloadedCandidates;

        loadedCandidates.reserve(loadedPaths.size());
        unloadedCandidates.reserve(
            unloadedCandidateIndices.empty() ? manifest.shards.size() : unloadedCandidateIndices.size()
        );

        auto buildCandidate = [&](std::size_t index, bool onlyIfLoaded) {
            if (index >= manifest.shards.size()) {
                return;
            }

            const auto& shard = manifest.shards[index];
            SpatialShardCandidate candidate;
            candidate.linkObjectName = shard.linkObjectName;
            candidate.documentPath = shard.documentPath;
            candidate.manifestIndex = index;

            if (spatialIndex && index < spatialIndex->shards.size()
                && spatialIndex->shards[index].hasSpatialCenter) {
                const Base::Vector3d& center = spatialIndex->shards[index].center;
                const double dx = center.x - focusPoint.x;
                const double dy = center.y - focusPoint.y;
                const double dz = center.z - focusPoint.z;
                candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                candidate.hasSpatialDistance = true;
            }
            else if (auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str())) {
                Base::BoundBox3d bounds;
                if (lightweightProxyBounds(*linkObject, bounds)) {
                    const Base::Vector3d center = lightweightProxyBoundsCenter(bounds);
                    const double dx = center.x - focusPoint.x;
                    const double dy = center.y - focusPoint.y;
                    const double dz = center.z - focusPoint.z;
                    candidate.spatialDistanceSquared = dx * dx + dy * dy + dz * dz;
                    candidate.hasSpatialDistance = true;
                }
            }

            candidate.doc = App::GetApplication().getDocumentByPath(
                shard.documentPath.c_str(),
                App::Application::PathMatchMode::MatchCanonicalWarning
            );
            const bool isFullyLoaded = candidate.doc && !candidate.doc->testStatus(App::Document::PartialDoc);
            if (isFullyLoaded) {
                if (!onlyIfLoaded) {
                    markShardLoadedForManifest(manifest, shard.documentPath);
                    return;
                }
                if (!isPinnedShardForManifest(manifest, shard.documentPath)) {
                    loadedCandidates.push_back(std::move(candidate));
                }
            }
            else if (!onlyIfLoaded) {
                unloadedCandidates.push_back(std::move(candidate));
            }
        };

        for (const auto& documentPath : loadedPaths) {
            const std::ptrdiff_t manifestIndex = manifestShardIndexForDocumentPath(
                manifest,
                spatialIndex,
                documentPath
            );
            if (manifestIndex >= 0) {
                buildCandidate(static_cast<std::size_t>(manifestIndex), true);
            }
        }

        if (!unloadedCandidateIndices.empty()) {
            for (const auto index : unloadedCandidateIndices) {
                buildCandidate(index, false);
            }
        }
        else {
            for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
                buildCandidate(index, false);
            }
        }

        return std::make_pair(std::move(loadedCandidates), std::move(unloadedCandidates));
    };

    const auto betterUnloadedCandidate = [](const SpatialShardCandidate& left,
                                            const SpatialShardCandidate& right) {
        if (left.hasSpatialDistance != right.hasSpatialDistance) {
            return left.hasSpatialDistance && !right.hasSpatialDistance;
        }
        if (left.hasSpatialDistance
            && left.spatialDistanceSquared != right.spatialDistanceSquared) {
            return left.spatialDistanceSquared < right.spatialDistanceSquared;
        }
        return left.manifestIndex < right.manifestIndex;
    };
    const auto betterLoadedCandidate = [](const SpatialShardCandidate& left,
                                          const SpatialShardCandidate& right) {
        if (left.hasSpatialDistance != right.hasSpatialDistance) {
            return left.hasSpatialDistance && !right.hasSpatialDistance;
        }
        if (left.hasSpatialDistance
            && left.spatialDistanceSquared != right.spatialDistanceSquared) {
            return left.spatialDistanceSquared > right.spatialDistanceSquared;
        }
        return left.manifestIndex < right.manifestIndex;
    };

    while (remainingChanges > 0) {
        auto [loadedCandidates, unloadedCandidates] = buildCandidates();
        if (loadedCandidates.empty() || unloadedCandidates.empty()) {
            break;
        }

        auto loadedQueue = makeCandidatePriorityQueue(
            std::move(loadedCandidates),
            betterLoadedCandidate
        );
        auto unloadedQueue = makeCandidatePriorityQueue(
            std::move(unloadedCandidates),
            betterUnloadedCandidate
        );

        const auto farthestLoaded = loadedQueue.top();
        const auto nearestUnloaded = unloadedQueue.top();
        if (!farthestLoaded.hasSpatialDistance || !nearestUnloaded.hasSpatialDistance
            || nearestUnloaded.spatialDistanceSquared >= farthestLoaded.spatialDistanceSquared) {
            break;
        }

        if (!closeLoadedShardInWorkspaceDocument(
                masterDoc,
                manifest,
                farthestLoaded.documentPath,
                farthestLoaded.doc
            )) {
            break;
        }

        App::Document* shardDoc = App::GetApplication().openDocument(nearestUnloaded.documentPath.c_str());
        if (!shardDoc) {
            break;
        }
        markShardLoadedForManifest(manifest, nearestUnloaded.documentPath);

        auto* nearestLinkObject = masterDoc->getObject(nearestUnloaded.linkObjectName.c_str());
        if (!nearestLinkObject) {
            break;
        }

        noteDocumentPathAccess(nearestUnloaded.documentPath);
        refreshLinkedShardAfterLoad(*nearestLinkObject);
        setTrackedShardLoadSource(manifest, nearestUnloaded.documentPath, ShardLoadSource::Prefetch);
        ++changed;
        --remainingChanges;
    }

    return changed;
}

std::size_t restoreInitialShards(const App::Document& workspaceDoc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return 0;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    const int maxLoadedShards = StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();
    if (maxLoadedShards <= 0) {
        return 0;
    }

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc) {
        return 0;
    }

    std::size_t restored = 0;
    std::size_t loaded = fullyLoadedShardCountForManifest(manifest);
    for (const auto& shard : manifest.shards) {
        if (loaded >= static_cast<std::size_t>(maxLoadedShards)) {
            break;
        }

        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            noteDocumentPathAccess(shard.documentPath);
            markShardLoadedForManifest(manifest, shard.documentPath);
            ++loaded;
            continue;
        }

        shardDoc = App::GetApplication().openDocument(shard.documentPath.c_str());
        if (!shardDoc) {
            continue;
        }
        markShardLoadedForManifest(manifest, shard.documentPath);

        auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str());
        if (!linkObject) {
            continue;
        }

        noteDocumentPathAccess(shard.documentPath);
        refreshLinkedShardAfterLoad(*linkObject);
        ++loaded;
        ++restored;
    }

    purgeWorkspaceTouchState(workspaceDoc);
    return restored;
}

}  // namespace

void StepLightweightWorkspaceRuntime::init()
{
    if (runtimeHooksInitialized) {
        return;
    }

    runtimeHooksInitialized = true;
    finishOpenConnection = App::GetApplication().signalFinishOpenDocument.connect([]() {
        for (auto* doc : App::GetApplication().getDocuments()) {
            if (doc) {
                const auto* cacheEntry = documentManifestCacheEntryForDocument(*doc);
                if (cacheEntry && cacheEntry->role == WorkspaceDocumentRole::Root) {
                    StepLightweightWorkspaceRuntime::initializeDocument(*doc);
                }
                else if (cacheEntry && cacheEntry->role == WorkspaceDocumentRole::Shard
                         && !doc->testStatus(App::Document::PartialDoc)) {
                    markShardLoadedForManifest(cacheEntry->manifest, doc->FileName.getValue());
                }
            }
        }
    });
    startSaveConnection = App::GetApplication().signalStartSaveDocument.connect(
        [](const App::Document& doc, const std::string&) {
            const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
            if (!cacheEntry || cacheEntry->role != WorkspaceDocumentRole::Root) {
                return;
            }
            const auto& manifest = cacheEntry->manifest;
            const WorkspaceDocumentRole role = cacheEntry->role;

            App::Document* workspaceDoc = workspaceRootDocument(doc, manifest, role);
            if (!workspaceDoc) {
                return;
            }

            WorkspaceSaveState saveState;
            saveState.manifest = manifest;
            saveState.loadedShardPaths = captureLoadedWorkspaceShardPaths(manifest);
            if (saveState.loadedShardPaths.empty()) {
                workspaceSaveStates.erase(&doc);
                return;
            }

            prepareWorkspaceDocumentForLazySave(*workspaceDoc, manifest, saveState.loadedShardPaths);
            workspaceSaveStates[&doc] = std::move(saveState);
        }
    );
    finishSaveConnection = App::GetApplication().signalFinishSaveDocument.connect(
        [](const App::Document& doc, const std::string&) {
            const auto it = workspaceSaveStates.find(&doc);
            if (it == workspaceSaveStates.end()) {
                return;
            }

            StepLightweightManifest manifest = it->second.manifest;
            std::vector<std::string> loadedShardPaths = std::move(it->second.loadedShardPaths);
            workspaceSaveStates.erase(it);

            const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
            const WorkspaceDocumentRole role = cacheEntry ? cacheEntry->role : WorkspaceDocumentRole::None;
            App::Document* workspaceDoc = workspaceRootDocument(doc, manifest, role);
            if (!workspaceDoc) {
                return;
            }

            restoreWorkspaceDocumentAfterLazySave(*workspaceDoc, manifest, loadedShardPaths);
        }
    );
    deleteDocumentConnection = App::GetApplication().signalDeleteDocument.connect(
        [](const App::Document& doc) {
            const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
            if (cacheEntry && cacheEntry->role != WorkspaceDocumentRole::None) {
                clearWorkspaceSpatialIndex(cacheEntry->manifest);
                if (cacheEntry->role == WorkspaceDocumentRole::Shard) {
                    markShardUnloadedForManifest(cacheEntry->manifest, doc.FileName.getValue());
                }
            }
            workspacePendingInitialRestoreCounts.erase(normalizePath(doc.FileName.getValue()));
            workspaceSaveStates.erase(&doc);
            workspaceDocumentManifestCaches.erase(&doc);
            initializedWorkspaceDocuments.erase(&doc);
            initializingWorkspaceDocuments.erase(&doc);
        }
    );
}

int StepLightweightWorkspaceRuntime::configuredMaxLoadedShards()
{
    Part::OCAF::ImportExportSettings settings;
    return settings.getLightweightWorkspaceMaxLoadedShards();
}

int StepLightweightWorkspaceRuntime::setConfiguredMaxLoadedShards(int maxLoadedShards)
{
    Part::OCAF::ImportExportSettings settings;
    settings.setLightweightWorkspaceMaxLoadedShards(maxLoadedShards);
    return settings.getLightweightWorkspaceMaxLoadedShards();
}

void StepLightweightWorkspaceRuntime::initializeDocument(App::Document& workspaceDoc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role != WorkspaceDocumentRole::Root) {
        return;
    }
    const auto& manifest = cacheEntry->manifest;
    if (initializedWorkspaceDocuments.find(&workspaceDoc) != initializedWorkspaceDocuments.end()
        || initializingWorkspaceDocuments.find(&workspaceDoc) != initializingWorkspaceDocuments.end()) {
        return;
    }

    initializingWorkspaceDocuments.insert(&workspaceDoc);
    try {
        resetWorkspaceMetrics(workspaceDoc);
        restorePinnedShards(workspaceDoc);
        if (shouldDeferInitialRestore(workspaceDoc, manifest)) {
            const int maxLoadedShards = configuredMaxLoadedShards();
            if (maxLoadedShards > 0) {
                const std::size_t loadedShardCount = fullyLoadedShardCountForManifest(manifest);
                const std::size_t pendingRestoreCount = loadedShardCount
                    >= static_cast<std::size_t>(maxLoadedShards)
                    ? 0
                    : static_cast<std::size_t>(maxLoadedShards) - loadedShardCount;
                setPendingInitialRestoreCountForDocument(workspaceDoc, pendingRestoreCount);
            }
            else {
                setPendingInitialRestoreCountForDocument(workspaceDoc, 0);
            }
        }
        else {
            setPendingInitialRestoreCountForDocument(workspaceDoc, 0);
            restoreInitialShards(workspaceDoc);
        }
        synchronizeLinkedShardProxies(workspaceDoc);
        purgeWorkspaceTouchState(workspaceDoc);
        initializedWorkspaceDocuments.insert(&workspaceDoc);
    }
    catch (...) {
        initializingWorkspaceDocuments.erase(&workspaceDoc);
        throw;
    }
    initializingWorkspaceDocuments.erase(&workspaceDoc);
}

void StepLightweightWorkspaceRuntime::synchronizeLinkedShardProxies(const App::Document& workspaceDoc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc) {
        return;
    }

    bool removedLegacyProxyObjects = false;
    for (const auto& shard : manifest.shards) {
        auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str());
        if (!linkObject) {
            continue;
        }

        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard.documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        const bool isFullyLoaded = shardDoc && !shardDoc->testStatus(App::Document::PartialDoc);
        if (!isFullyLoaded) {
            clearLinkedShardTarget(*linkObject);
        }

        removedLegacyProxyObjects = removeLightweightProxyObjects(*linkObject) || removedLegacyProxyObjects;
    }

    if (removedLegacyProxyObjects) {
        removeLightweightProxyPrototypes(*masterDoc);
    }

    refreshWorkspaceSpatialIndex(manifest, *masterDoc);
}

bool StepLightweightWorkspaceRuntime::isWorkspaceDocument(const App::Document& doc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return false;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;

    if (role == WorkspaceDocumentRole::Root) {
        return true;
    }

    const std::string docPath = normalizePath(doc.FileName.getValue());
    if (docPath == normalizePath(manifest.masterDocumentPath)) {
        return true;
    }

    return std::any_of(
        manifest.shards.begin(),
        manifest.shards.end(),
        [&](const StepLightweightShard& shard) {
            return normalizePath(shard.documentPath) == docPath;
        }
    );
}

void StepLightweightWorkspaceRuntime::resetWorkspaceMetrics(const App::Document& doc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
    if (!cacheEntry) {
        return;
    }
    const auto& manifest = cacheEntry->manifest;

    const std::string normalizedWorkspacePath = normalizedMasterPath(manifest);
    const std::string cacheDirectory = cacheDirectoryForManifest(manifest);

    workspaceRuntimeMetrics[normalizedWorkspacePath] = WorkspaceRuntimeMetrics();
    workspacePinnedShards.erase(normalizedWorkspacePath);
    clearWorkspaceSpatialIndex(manifest);
    clearLoadedShardPathCache(manifest);
    workspacePendingInitialRestoreCounts.erase(normalizePath(doc.FileName.getValue()));

    for (auto it = shardLoadSources.begin(); it != shardLoadSources.end();) {
        if (shardDocumentPathBelongsToCacheDirectory(it->first, cacheDirectory)) {
            it = shardLoadSources.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto it = shardAccessStamps.begin(); it != shardAccessStamps.end();) {
        if (shardDocumentPathBelongsToCacheDirectory(it->first, cacheDirectory)) {
            it = shardAccessStamps.erase(it);
        }
        else {
            ++it;
        }
    }
}

StepLightweightWorkspaceState StepLightweightWorkspaceRuntime::inspect(const App::Document& doc)
{
    StepLightweightWorkspaceState state;

    const auto* cacheEntry = documentManifestCacheEntryForDocument(doc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return state;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    state.isWorkspaceDocument = true;
    state.masterDocumentPath = role == WorkspaceDocumentRole::Root
        ? normalizePath(doc.FileName.getValue())
        : manifest.masterDocumentPath;
    state.pendingInitialRestoreCount = role == WorkspaceDocumentRole::Root
        ? pendingInitialRestoreCountForDocument(doc)
        : 0;
    App::Document* masterDoc = workspaceRootDocument(doc, manifest, role);
    const auto metricsIt = workspaceRuntimeMetrics.find(normalizedMasterPath(manifest));
    if (metricsIt != workspaceRuntimeMetrics.end()) {
        state.manualLoadEventCount = metricsIt->second.manualLoadEventCount;
        state.prefetchEventCount = metricsIt->second.prefetchEventCount;
        state.trimmedShardEventCount = metricsIt->second.trimmedShardEventCount;
        state.manualUnloadEventCount = metricsIt->second.manualUnloadEventCount;
    }
    state.shards.reserve(manifest.shards.size());

    for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
        const auto& shard = manifest.shards[index];
        StepLightweightWorkspaceShardState shardState;
        shardState.documentPath = shard.documentPath;
        shardState.objectName = shard.assemblyObjectName;
        shardState.isWorkspaceShard = true;
        shardState.isPinned = isPinnedShardForManifest(manifest, shard.documentPath);
        updateShardOpenState(shardState, shard.documentPath);
        if (shardState.isPinned) {
            ++state.pinnedShardCount;
        }

        if (masterDoc) {
            if (auto* linkObject = masterDoc->getObject(shard.linkObjectName.c_str())) {
                shardState.hasProxy = hasLightweightProxy(*linkObject);
                if (shardState.hasProxy) {
                    ++state.proxyShardCount;
                }
            }
        }

        if (shardState.isOpen) {
            ++state.openShardCount;
            if (shardState.isFullyLoaded) {
                ++state.fullyLoadedShardCount;

                ShardLoadSource loadSource = ShardLoadSource::Initial;
                trackedShardLoadSource(shard.documentPath, loadSource);
                shardState.loadSource = shardLoadSourceName(loadSource);
                switch (loadSource) {
                    case ShardLoadSource::Manual:
                        ++state.manualLoadedShardCount;
                        break;
                    case ShardLoadSource::Prefetch:
                        ++state.prefetchedShardCount;
                        break;
                    case ShardLoadSource::Initial:
                    default:
                        ++state.initialLoadedShardCount;
                        break;
                }
            }
        }
        else {
            clearTrackedShardLoadSource(shard.documentPath);
        }

        if (!shardState.isFullyLoaded) {
            shardState.loadSource = unloadedLoadSourceName;
            ++state.unloadedShardCount;
        }

        state.shards.push_back(std::move(shardState));
    }

    return state;
}

StepLightweightWorkspaceShardState StepLightweightWorkspaceRuntime::inspectLinkedShard(
    const App::DocumentObject& object
)
{
    StepLightweightWorkspaceShardState shardState;

    std::string documentPath;
    std::string objectName;
    if (!linkedShardReference(object, documentPath, objectName)) {
        return shardState;
    }

    shardState.documentPath = documentPath;
    shardState.objectName = objectName;

    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return shardState;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    const auto* shard = manifestShardForReference(*cacheEntry, documentPath, objectName);
    if (!shard) {
        return shardState;
    }

    shardState.isWorkspaceShard = true;
    shardState.isPinned = isPinnedShardForManifest(manifest, documentPath);
    updateShardOpenState(shardState, documentPath);
    if (auto* masterDoc = workspaceRootDocument(*object.getDocument(), manifest, role)) {
        if (auto* linkObject = masterDoc->getObject(shard->linkObjectName.c_str())) {
            shardState.hasProxy = hasLightweightProxy(*linkObject);
        }
    }
    if (shardState.isFullyLoaded) {
        ShardLoadSource loadSource = ShardLoadSource::Initial;
        trackedShardLoadSource(documentPath, loadSource);
        shardState.loadSource = shardLoadSourceName(loadSource);
    }
    else {
        shardState.loadSource = unloadedLoadSourceName;
    }
    return shardState;
}

bool StepLightweightWorkspaceRuntime::pinLinkedShard(App::DocumentObject& object)
{
    const StepLightweightWorkspaceShardState shardState = inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard) {
        return false;
    }

    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    if (!cacheEntry) {
        return false;
    }
    const auto& manifest = cacheEntry->manifest;
    loadPersistedPinnedShardsForManifest(manifest);

    std::uint64_t priority = accessStampForPath(shardState.documentPath);
    if (priority == 0) {
        noteDocumentPathAccess(shardState.documentPath);
        priority = accessStampForPath(shardState.documentPath);
    }
    setPinnedShardForManifest(manifest, shardState.documentPath, true, priority);
    savePersistedPinnedShardsForManifest(manifest);
    return true;
}

bool StepLightweightWorkspaceRuntime::unpinLinkedShard(App::DocumentObject& object)
{
    const StepLightweightWorkspaceShardState shardState = inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard) {
        return false;
    }

    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    if (!cacheEntry) {
        return false;
    }
    const auto& manifest = cacheEntry->manifest;
    loadPersistedPinnedShardsForManifest(manifest);

    setPinnedShardForManifest(manifest, shardState.documentPath, false);
    savePersistedPinnedShardsForManifest(manifest);
    return true;
}

std::size_t StepLightweightWorkspaceRuntime::restorePinnedShards(const App::Document& workspaceDoc)
{
    const auto* cacheEntry = documentManifestCacheEntryForDocument(workspaceDoc);
    if (!cacheEntry || cacheEntry->role == WorkspaceDocumentRole::None) {
        return 0;
    }
    const auto& manifest = cacheEntry->manifest;
    const WorkspaceDocumentRole role = cacheEntry->role;
    loadPersistedPinnedShardsForManifest(manifest);

    const int maxLoadedShards = configuredMaxLoadedShards();
    enforceLoadedShardBudget(workspaceDoc, maxLoadedShards);
    if (maxLoadedShards <= 0) {
        return 0;
    }

    App::Document* masterDoc = workspaceRootDocument(workspaceDoc, manifest, role);
    if (!masterDoc) {
        return 0;
    }

    std::vector<const StepLightweightShard*> pinnedShards;
    pinnedShards.reserve(manifest.shards.size());
    for (const auto& shard : manifest.shards) {
        if (isPinnedShardForManifest(manifest, shard.documentPath)) {
            pinnedShards.push_back(&shard);
        }
    }
    std::stable_sort(
        pinnedShards.begin(),
        pinnedShards.end(),
        [&](const StepLightweightShard* left, const StepLightweightShard* right) {
            const std::uint64_t leftPriority
                = effectiveShardPriorityForManifest(manifest, left->documentPath);
            const std::uint64_t rightPriority
                = effectiveShardPriorityForManifest(manifest, right->documentPath);
            if (leftPriority != rightPriority) {
                return leftPriority > rightPriority;
            }

            return left->documentPath < right->documentPath;
        }
    );

    std::size_t restored = 0;
    for (const auto* shard : pinnedShards) {
        App::Document* shardDoc = App::GetApplication().getDocumentByPath(
            shard->documentPath.c_str(),
            App::Application::PathMatchMode::MatchCanonicalWarning
        );
        if (shardDoc && !shardDoc->testStatus(App::Document::PartialDoc)) {
            markShardLoadedForManifest(manifest, shard->documentPath);
            continue;
        }

        if (fullyLoadedShardCountForManifest(manifest) >= static_cast<std::size_t>(maxLoadedShards)) {
            trimLoadedShards(*masterDoc, maxLoadedShards - 1, shard->documentPath);
            if (fullyLoadedShardCountForManifest(manifest)
                >= static_cast<std::size_t>(maxLoadedShards)) {
                break;
            }
        }

        shardDoc = App::GetApplication().openDocument(shard->documentPath.c_str());
        if (!shardDoc) {
            continue;
        }
        markShardLoadedForManifest(manifest, shard->documentPath);

        if (auto* linkObject = masterDoc->getObject(shard->linkObjectName.c_str())) {
            noteDocumentPathAccess(shard->documentPath);
            refreshLinkedShardAfterLoad(*linkObject);
            updatePinnedShardPriorityForManifest(manifest, shard->documentPath);
            ++restored;
        }
    }

    purgeWorkspaceTouchState(workspaceDoc);
    return restored;
}

std::size_t StepLightweightWorkspaceRuntime::prefetchLinkedShardNeighbors(App::DocumentObject& object)
{
    const StepLightweightWorkspaceShardState shardState = inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard) {
        return 0;
    }

    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    if (!cacheEntry) {
        return 0;
    }
    const auto& manifest = cacheEntry->manifest;

    const std::size_t prefetched
        = prefetchAdjacentShardLinks(object, manifest, shardState.documentPath);
    if (prefetched > 0) {
        purgeWorkspaceTouchState(*object.getDocument());
    }
    return prefetched;
}

std::size_t StepLightweightWorkspaceRuntime::prefetchShardsNearPoint(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxPrefetchCount
)
{
    const std::size_t prefetched
        = prefetchShardsNearPointImpl(workspaceDoc, focusPoint, maxPrefetchCount);
    if (prefetched > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return prefetched;
}

std::size_t StepLightweightWorkspaceRuntime::restoreDeferredInitialShardsNearPoint(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxRestoreCount
)
{
    const std::size_t restored
        = restoreDeferredInitialShardsNearPointImpl(workspaceDoc, focusPoint, maxRestoreCount);
    if (restored > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return restored;
}

std::size_t StepLightweightWorkspaceRuntime::rebalanceShardsNearPoint(
    const App::Document& workspaceDoc,
    const Base::Vector3d& focusPoint,
    int maxReplacementCount
)
{
    const std::size_t rebalanced
        = rebalanceShardsNearPointImpl(workspaceDoc, focusPoint, maxReplacementCount);
    if (rebalanced > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return rebalanced;
}

std::size_t StepLightweightWorkspaceRuntime::trimShardsOutsideRetainedPaths(
    const App::Document& workspaceDoc,
    const std::vector<std::string>& retainedDocumentPaths,
    int maxTrimCount
)
{
    const std::size_t trimmed = trimLoadedShardsOutsideRetainedPathsImpl(
        workspaceDoc,
        retainedDocumentPaths,
        maxTrimCount
    );
    if (trimmed > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return trimmed;
}

App::DocumentObject* StepLightweightWorkspaceRuntime::loadLinkedShard(App::DocumentObject& object)
{
    const StepLightweightWorkspaceShardState shardState = inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard) {
        return nullptr;
    }

    App::Document* doc = App::GetApplication().openDocument(shardState.documentPath.c_str());
    if (!doc) {
        return nullptr;
    }

    noteDocumentPathAccess(shardState.documentPath);
    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    const bool hasManifest = cacheEntry != nullptr;
    const auto* manifest = hasManifest ? &cacheEntry->manifest : nullptr;
    if (hasManifest) {
        markShardLoadedForManifest(*manifest, shardState.documentPath);
        updatePinnedShardPriorityForManifest(*manifest, shardState.documentPath);
    }
    if (!shardState.isFullyLoaded && hasManifest) {
        setTrackedShardLoadSource(*manifest, shardState.documentPath, ShardLoadSource::Manual);
        consumePendingInitialRestoreCountForDocument(*object.getDocument(), 1);
    }
    refreshLinkedShardAfterLoad(object);
    trimLoadedShards(*doc, -1, shardState.documentPath);

    if (hasManifest) {
        prefetchAdjacentShardLinks(object, *manifest, shardState.documentPath);
    }
    purgeWorkspaceTouchState(*object.getDocument());
    doc->purgeTouched();
    return doc->getObject(shardState.objectName.c_str());
}

bool StepLightweightWorkspaceRuntime::unloadLinkedShard(App::DocumentObject& object)
{
    const StepLightweightWorkspaceShardState shardState = inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard) {
        return false;
    }

    App::Document* doc = App::GetApplication().getDocumentByPath(
        shardState.documentPath.c_str(),
        App::Application::PathMatchMode::MatchCanonicalWarning
    );
    if (!doc) {
        return false;
    }

    shardAccessStamps.erase(normalizePath(shardState.documentPath));
    const auto* cacheEntry = documentManifestCacheEntryForDocument(*object.getDocument());
    const bool hasManifest = cacheEntry != nullptr;
    const auto* manifest = hasManifest ? &cacheEntry->manifest : nullptr;
    const bool closed = App::GetApplication().closeDocument(doc->getName());
    if (closed) {
        clearLinkedShardProxyContents(object);
        clearLinkedShardTarget(object);
        clearTrackedShardLoadSource(shardState.documentPath);
        if (hasManifest) {
            markShardUnloadedForManifest(*manifest, shardState.documentPath);
            ++runtimeMetricsForManifest(*manifest).manualUnloadEventCount;
        }
        purgeWorkspaceTouchState(*object.getDocument());
    }
    return closed;
}

std::size_t StepLightweightWorkspaceRuntime::trimLoadedShards(
    const App::Document& workspaceDoc,
    int maxLoadedShards,
    const std::string& keepDocumentPath
)
{
    const std::size_t trimmed
        = trimLoadedShardsImpl(workspaceDoc, maxLoadedShards, keepDocumentPath, false);
    if (trimmed > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return trimmed;
}

std::size_t StepLightweightWorkspaceRuntime::enforceLoadedShardBudget(
    const App::Document& workspaceDoc,
    int maxLoadedShards
)
{
    const std::size_t trimmed = trimLoadedShardsImpl(workspaceDoc, maxLoadedShards, {}, true);
    if (trimmed > 0) {
        purgeWorkspaceTouchState(workspaceDoc);
    }
    return trimmed;
}

void StepLightweightWorkspaceRuntime::noteDocumentAccess(const App::Document& doc)
{
    if (!doc.FileName.getValue() || doc.testStatus(App::Document::PartialDoc)) {
        return;
    }

    StepLightweightManifest manifest;
    if (!manifestForDocument(doc, manifest)) {
        return;
    }

    const std::string documentPath = normalizePath(doc.FileName.getValue());
    for (const auto& shard : manifest.shards) {
        if (normalizePath(shard.documentPath) == documentPath) {
            noteDocumentPathAccess(documentPath);
            updatePinnedShardPriorityForManifest(manifest, documentPath);
            return;
        }
    }
}
