// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LightweightWorkspaceProxyViewOptimizer.h"
#include "ViewProviderAssemblyLink.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <QElapsedTimer>
#include <QTimer>
#include <unordered_map>
#include <unordered_set>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <Base/BoundBox.h>
#include <Base/FileInfo.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Inventor/SbBox.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/nodes/SoCamera.h>
#include <Mod/Import/App/StepLightweightWorkspaceRuntime.h>

namespace
{

constexpr auto navigationPrefetchIntervalMs = 150;
constexpr auto navigationIdleDelayMs = 450;
constexpr auto navigationWarmPrefetchDelayMs = 1200;
constexpr auto navigationWarmPrefetchIntervalMs = 700;
constexpr auto navigationFarShardSwapStabilityDelayMs = 900;
constexpr auto navigationRebalanceIntervalMs = 900;
constexpr auto navigationRetentionGraceMs = 1500;
constexpr auto repeatedFocusRetryMs = 400;
constexpr auto proxyVisibilityRefreshMs = 600;
constexpr int maxProxyVisibilityPathUpdatesPerCycle = 64;
constexpr int maxWorkspaceLoadActionsPerCycle = 1;
constexpr auto maxViewTrimCountPerCycle = 1;
constexpr auto maxWarmPrefetchCountPerCycle = 1;
constexpr double retainedDistanceScale = 1.5;
constexpr int maxIndexedCellsPerAxis = 8;
constexpr int maxIndexedCellsTotal = 256;
constexpr const char* lightweightProxyBoundsMinPropertyName = "LightweightProxyBoundsMin";
constexpr const char* lightweightProxyBoundsMaxPropertyName = "LightweightProxyBoundsMax";
constexpr const char* lightweightWorkspaceShardLinkPropertyName = "LightweightWorkspaceShardLink";
constexpr const char* lightweightWorkspaceShardDocumentPathPropertyName
    = "LightweightWorkspaceShardDocumentPath";
constexpr const char* lightweightWorkspaceShardObjectNamePropertyName
    = "LightweightWorkspaceShardObjectName";

fastsignals::scoped_connection finishOpenConnection;
fastsignals::scoped_connection activateViewConnection;
fastsignals::scoped_connection newObjectConnection;
fastsignals::scoped_connection deletedObjectConnection;
fastsignals::scoped_connection changedObjectConnection;
fastsignals::scoped_connection deleteDocumentConnection;
bool initialized = false;
QTimer* navigationPrefetchTimer = nullptr;

struct ActiveViewPrefetchState
{
    std::string workspaceDocumentPath;
    Base::Vector3d observedFocalPoint;
    bool hasObservedFocalPoint = false;
    qint64 lastMovementMs = 0;
    Base::Vector3d attemptFocalPoint;
    bool hasAttemptFocalPoint = false;
    double movementTolerance = 0.0;
    qint64 lastAttemptMs = 0;
    std::unordered_map<std::string, qint64> retainedShardTimestamps;
    qint64 lastWarmPrefetchAttemptMs = 0;
    Base::Vector3d lastProxyVisibilityFocalPoint;
    bool hasLastProxyVisibilityFocalPoint = false;
    qint64 lastProxyVisibilityUpdateMs = 0;
    Base::Vector3d lastRebalanceFocalPoint;
    bool hasLastRebalanceFocalPoint = false;
    qint64 lastRebalanceAttemptMs = 0;
    Base::Vector3d cachedRetainedFocalPoint;
    Base::Vector3d cachedRetainedCameraPosition;
    std::array<double, 4> cachedRetainedCameraOrientation {0.0, 0.0, 0.0, 1.0};
    double cachedRetainRadius = 0.0;
    std::size_t cachedRetainedViewRevision = 0;
    bool hasCachedRetainedDocumentPaths = false;
    std::vector<std::string> cachedRetainedDocumentPaths;
    bool pendingRestoreNearFocus = false;
    bool pendingTrimOutsideRetainedPaths = false;
    bool pendingRebalanceNearFocus = false;
    bool pendingWarmPrefetchNearFocus = false;
};

struct WorkspaceViewCellCoord
{
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const WorkspaceViewCellCoord& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct WorkspaceViewShardEntry
{
    std::string documentPath;
    std::string objectName;
    Base::BoundBox3d bounds;
    bool hasBounds = false;
    bool alwaysInclude = false;
    std::vector<WorkspaceViewCellCoord> indexedCells;
};

struct WorkspaceViewCellCoordHash
{
    std::size_t operator()(const WorkspaceViewCellCoord& coord) const
    {
        std::size_t seed = 0;
        seed ^= std::hash<int> {}(coord.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int> {}(coord.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int> {}(coord.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct WorkspaceViewIndex
{
    std::size_t objectCount = 0;
    std::vector<WorkspaceViewShardEntry> shards;
    std::unordered_map<WorkspaceViewCellCoord, std::vector<std::size_t>, WorkspaceViewCellCoordHash>
        shardIndicesByCell;
    std::vector<std::size_t> shardIndicesAlwaysInclude;
    std::unordered_map<std::string, std::vector<std::size_t>> shardIndicesByPath;
    std::unordered_map<std::string, std::size_t> shardIndexByObjectName;
    Base::BoundBox3d spatialBounds;
    double cellSize = 1.0;
    bool hasSpatialGrid = false;
    bool needsFullRebuild = false;
    std::size_t revision = 0;
    std::unordered_set<std::string> dirtyObjectNames;
};

ActiveViewPrefetchState activeViewPrefetchState;
QElapsedTimer activeViewPrefetchClock;
std::unordered_map<const App::Document*, WorkspaceViewIndex> workspaceViewIndices;
std::unordered_map<const App::Document*, std::unordered_set<std::string>> workspaceVisibleProxyPaths;
std::unordered_map<const App::Document*, std::unordered_set<std::string>> workspaceDesiredProxyPaths;
std::unordered_map<const App::Document*, std::vector<std::string>> workspacePendingProxyPathsToShow;
std::unordered_map<const App::Document*, std::vector<std::string>> workspacePendingProxyPathsToHide;

std::string normalizePathForCompare(const std::string& path)
{
    if (path.empty()) {
        return {};
    }

    return Base::FileInfo::pathToString(Base::FileInfo::stringToPath(path).lexically_normal());
}

bool focalPointMovedEnough(
    const Base::Vector3d& left,
    const Base::Vector3d& right,
    double movementTolerance
)
{
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    const double dz = left.z - right.z;
    const double distanceSquared = dx * dx + dy * dy + dz * dz;
    return distanceSquared >= movementTolerance * movementTolerance;
}

void clearCachedRetainedDocumentPaths()
{
    activeViewPrefetchState.cachedRetainedFocalPoint = Base::Vector3d();
    activeViewPrefetchState.cachedRetainedCameraPosition = Base::Vector3d();
    activeViewPrefetchState.cachedRetainedCameraOrientation = {0.0, 0.0, 0.0, 1.0};
    activeViewPrefetchState.cachedRetainRadius = 0.0;
    activeViewPrefetchState.cachedRetainedViewRevision = 0;
    activeViewPrefetchState.hasCachedRetainedDocumentPaths = false;
    activeViewPrefetchState.cachedRetainedDocumentPaths.clear();
}

void clearPendingWorkspaceLoadActions()
{
    activeViewPrefetchState.pendingRestoreNearFocus = false;
    activeViewPrefetchState.pendingTrimOutsideRetainedPaths = false;
    activeViewPrefetchState.pendingRebalanceNearFocus = false;
    activeViewPrefetchState.pendingWarmPrefetchNearFocus = false;
}

bool observeFocusPoint(
    const App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance
)
{
    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    if (activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath
        || !activeViewPrefetchState.hasObservedFocalPoint) {
        activeViewPrefetchState.workspaceDocumentPath = normalizedDocumentPath;
        activeViewPrefetchState.observedFocalPoint = focalPoint;
        activeViewPrefetchState.hasObservedFocalPoint = true;
        activeViewPrefetchState.lastMovementMs = activeViewPrefetchClock.elapsed();
        activeViewPrefetchState.hasAttemptFocalPoint = false;
        activeViewPrefetchState.movementTolerance = movementTolerance;
        activeViewPrefetchState.retainedShardTimestamps.clear();
        activeViewPrefetchState.lastWarmPrefetchAttemptMs = 0;
        activeViewPrefetchState.hasLastProxyVisibilityFocalPoint = false;
        activeViewPrefetchState.lastProxyVisibilityUpdateMs = 0;
        activeViewPrefetchState.hasLastRebalanceFocalPoint = false;
        activeViewPrefetchState.lastRebalanceAttemptMs = 0;
        clearCachedRetainedDocumentPaths();
        clearPendingWorkspaceLoadActions();
        return true;
    }

    if (focalPointMovedEnough(
            activeViewPrefetchState.observedFocalPoint,
            focalPoint,
            movementTolerance
        )) {
        activeViewPrefetchState.observedFocalPoint = focalPoint;
        activeViewPrefetchState.lastMovementMs = activeViewPrefetchClock.elapsed();
        clearCachedRetainedDocumentPaths();
        clearPendingWorkspaceLoadActions();
    }

    activeViewPrefetchState.movementTolerance = movementTolerance;
    return false;
}

bool shouldRetryFocusPoint(
    const App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance,
    bool documentChanged
)
{
    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    const qint64 now = activeViewPrefetchClock.isValid() ? activeViewPrefetchClock.elapsed() : 0;

    if (documentChanged || activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath
        || !activeViewPrefetchState.hasAttemptFocalPoint) {
        return true;
    }

    if ((now - activeViewPrefetchState.lastMovementMs) < navigationIdleDelayMs) {
        return false;
    }

    if (focalPointMovedEnough(
            activeViewPrefetchState.attemptFocalPoint,
            focalPoint,
            movementTolerance
        )) {
        return true;
    }

    return (now - activeViewPrefetchState.lastAttemptMs) >= repeatedFocusRetryMs;
}

void noteFocusPointAttempt(
    const App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance
)
{
    if (!activeViewPrefetchClock.isValid()) {
        activeViewPrefetchClock.start();
    }

    activeViewPrefetchState.workspaceDocumentPath
        = normalizePathForCompare(doc.FileName.getValue());
    activeViewPrefetchState.observedFocalPoint = focalPoint;
    activeViewPrefetchState.hasObservedFocalPoint = true;
    activeViewPrefetchState.attemptFocalPoint = focalPoint;
    activeViewPrefetchState.hasAttemptFocalPoint = true;
    activeViewPrefetchState.movementTolerance = movementTolerance;
    activeViewPrefetchState.lastAttemptMs = activeViewPrefetchClock.elapsed();
}

bool shouldRefreshProxyVisibility(
    const App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance,
    bool documentChanged
)
{
    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    const qint64 now = activeViewPrefetchClock.isValid() ? activeViewPrefetchClock.elapsed() : 0;

    if (documentChanged || activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath
        || !activeViewPrefetchState.hasLastProxyVisibilityFocalPoint) {
        return true;
    }

    if (focalPointMovedEnough(
            activeViewPrefetchState.lastProxyVisibilityFocalPoint,
            focalPoint,
            movementTolerance
        )) {
        return true;
    }

    return (now - activeViewPrefetchState.lastProxyVisibilityUpdateMs) >= proxyVisibilityRefreshMs;
}

void noteProxyVisibilityUpdate(const Base::Vector3d& focalPoint)
{
    if (!activeViewPrefetchClock.isValid()) {
        activeViewPrefetchClock.start();
    }

    activeViewPrefetchState.lastProxyVisibilityFocalPoint = focalPoint;
    activeViewPrefetchState.hasLastProxyVisibilityFocalPoint = true;
    activeViewPrefetchState.lastProxyVisibilityUpdateMs = activeViewPrefetchClock.elapsed();
}

void resetFocusPointAttempt()
{
    activeViewPrefetchState.workspaceDocumentPath.clear();
    activeViewPrefetchState.observedFocalPoint = Base::Vector3d();
    activeViewPrefetchState.hasObservedFocalPoint = false;
    activeViewPrefetchState.lastMovementMs = 0;
    activeViewPrefetchState.attemptFocalPoint = Base::Vector3d();
    activeViewPrefetchState.hasAttemptFocalPoint = false;
    activeViewPrefetchState.movementTolerance = 0.0;
    activeViewPrefetchState.lastAttemptMs = 0;
    activeViewPrefetchState.retainedShardTimestamps.clear();
    activeViewPrefetchState.lastWarmPrefetchAttemptMs = 0;
    activeViewPrefetchState.lastProxyVisibilityFocalPoint = Base::Vector3d();
    activeViewPrefetchState.hasLastProxyVisibilityFocalPoint = false;
    activeViewPrefetchState.lastProxyVisibilityUpdateMs = 0;
    activeViewPrefetchState.lastRebalanceFocalPoint = Base::Vector3d();
    activeViewPrefetchState.hasLastRebalanceFocalPoint = false;
    activeViewPrefetchState.lastRebalanceAttemptMs = 0;
    clearCachedRetainedDocumentPaths();
    clearPendingWorkspaceLoadActions();
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

bool isWorkspaceViewIndexRelevantProperty(const App::Property& property)
{
    const char* propertyName = property.getName();
    return propertyName
        && (std::strcmp(propertyName, lightweightProxyBoundsMinPropertyName) == 0
            || std::strcmp(propertyName, lightweightProxyBoundsMaxPropertyName) == 0
            || std::strcmp(propertyName, lightweightWorkspaceShardLinkPropertyName) == 0
            || std::strcmp(propertyName, lightweightWorkspaceShardDocumentPathPropertyName) == 0
            || std::strcmp(propertyName, lightweightWorkspaceShardObjectNamePropertyName) == 0);
}

bool buildWorkspaceViewShardEntry(
    const App::DocumentObject& object,
    WorkspaceViewShardEntry& entry
)
{
    const auto shardState = Import::StepLightweightWorkspaceRuntime::inspectLinkedShard(object);
    if (!shardState.isWorkspaceShard || shardState.documentPath.empty()) {
        return false;
    }

    entry.documentPath = normalizePathForCompare(shardState.documentPath);
    entry.objectName = object.getNameInDocument();
    entry.hasBounds = lightweightProxyBounds(object, entry.bounds);
    entry.alwaysInclude = false;
    entry.indexedCells.clear();
    return !entry.documentPath.empty();
}

void eraseWorkspaceViewShardIndexReference(std::vector<std::size_t>& indices, std::size_t index)
{
    indices.erase(std::remove(indices.begin(), indices.end(), index), indices.end());
}

WorkspaceViewCellCoord workspaceViewCellCoordForPoint(
    const WorkspaceViewIndex& viewIndex,
    const Base::Vector3d& point
)
{
    const auto cellCoordForAxis = [&viewIndex](double value, double minValue) {
        return static_cast<int>(std::floor((value - minValue) / viewIndex.cellSize));
    };

    return {
        cellCoordForAxis(point.x, viewIndex.spatialBounds.MinX),
        cellCoordForAxis(point.y, viewIndex.spatialBounds.MinY),
        cellCoordForAxis(point.z, viewIndex.spatialBounds.MinZ),
    };
}

void removeWorkspaceViewShardPathReference(
    WorkspaceViewIndex& viewIndex,
    const WorkspaceViewShardEntry& shardEntry,
    std::size_t index
)
{
    const auto pathIt = viewIndex.shardIndicesByPath.find(shardEntry.documentPath);
    if (pathIt == viewIndex.shardIndicesByPath.end()) {
        return;
    }

    eraseWorkspaceViewShardIndexReference(pathIt->second, index);
    if (pathIt->second.empty()) {
        viewIndex.shardIndicesByPath.erase(pathIt);
    }
}

void removeWorkspaceViewShardSpatialReference(
    WorkspaceViewIndex& viewIndex,
    const WorkspaceViewShardEntry& shardEntry,
    std::size_t index
)
{
    if (shardEntry.alwaysInclude) {
        eraseWorkspaceViewShardIndexReference(viewIndex.shardIndicesAlwaysInclude, index);
    }

    for (const auto& cellCoord : shardEntry.indexedCells) {
        const auto cellIt = viewIndex.shardIndicesByCell.find(cellCoord);
        if (cellIt == viewIndex.shardIndicesByCell.end()) {
            continue;
        }

        eraseWorkspaceViewShardIndexReference(cellIt->second, index);
        if (cellIt->second.empty()) {
            viewIndex.shardIndicesByCell.erase(cellIt);
        }
    }
}

void unindexWorkspaceViewShardEntry(WorkspaceViewIndex& viewIndex, std::size_t index)
{
    if (index >= viewIndex.shards.size()) {
        return;
    }

    const auto& shardEntry = viewIndex.shards[index];
    const auto objectIt = viewIndex.shardIndexByObjectName.find(shardEntry.objectName);
    if (objectIt != viewIndex.shardIndexByObjectName.end() && objectIt->second == index) {
        viewIndex.shardIndexByObjectName.erase(objectIt);
    }

    removeWorkspaceViewShardPathReference(viewIndex, shardEntry, index);
    removeWorkspaceViewShardSpatialReference(viewIndex, shardEntry, index);
}

void indexWorkspaceViewShardEntry(WorkspaceViewIndex& viewIndex, std::size_t index)
{
    if (index >= viewIndex.shards.size()) {
        return;
    }

    auto& shardEntry = viewIndex.shards[index];
    shardEntry.alwaysInclude = false;
    shardEntry.indexedCells.clear();
    viewIndex.shardIndexByObjectName[shardEntry.objectName] = index;
    viewIndex.shardIndicesByPath[shardEntry.documentPath].push_back(index);

    if (!shardEntry.hasBounds) {
        shardEntry.alwaysInclude = true;
        viewIndex.shardIndicesAlwaysInclude.push_back(index);
        return;
    }

    if (!viewIndex.hasSpatialGrid) {
        return;
    }

    const WorkspaceViewCellCoord minCell = workspaceViewCellCoordForPoint(
        viewIndex,
        Base::Vector3d(shardEntry.bounds.MinX, shardEntry.bounds.MinY, shardEntry.bounds.MinZ)
    );
    const WorkspaceViewCellCoord maxCell = workspaceViewCellCoordForPoint(
        viewIndex,
        Base::Vector3d(shardEntry.bounds.MaxX, shardEntry.bounds.MaxY, shardEntry.bounds.MaxZ)
    );

    const int xCount = std::max(1, maxCell.x - minCell.x + 1);
    const int yCount = std::max(1, maxCell.y - minCell.y + 1);
    const int zCount = std::max(1, maxCell.z - minCell.z + 1);
    if (xCount > maxIndexedCellsPerAxis || yCount > maxIndexedCellsPerAxis
        || zCount > maxIndexedCellsPerAxis
        || (xCount * yCount * zCount) > maxIndexedCellsTotal) {
        shardEntry.alwaysInclude = true;
        viewIndex.shardIndicesAlwaysInclude.push_back(index);
        return;
    }

    shardEntry.indexedCells.reserve(static_cast<std::size_t>(xCount * yCount * zCount));
    for (int x = minCell.x; x <= maxCell.x; ++x) {
        for (int y = minCell.y; y <= maxCell.y; ++y) {
            for (int z = minCell.z; z <= maxCell.z; ++z) {
                const WorkspaceViewCellCoord cellCoord {x, y, z};
                viewIndex.shardIndicesByCell[cellCoord].push_back(index);
                shardEntry.indexedCells.push_back(cellCoord);
            }
        }
    }
}

void addWorkspaceViewShardEntry(WorkspaceViewIndex& viewIndex, WorkspaceViewShardEntry&& shardEntry)
{
    viewIndex.shards.push_back(std::move(shardEntry));
    indexWorkspaceViewShardEntry(viewIndex, viewIndex.shards.size() - 1);
}

void removeWorkspaceViewShardEntry(WorkspaceViewIndex& viewIndex, std::size_t index)
{
    if (index >= viewIndex.shards.size()) {
        return;
    }

    const std::size_t lastIndex = viewIndex.shards.size() - 1;
    if (index != lastIndex) {
        unindexWorkspaceViewShardEntry(viewIndex, lastIndex);
    }
    unindexWorkspaceViewShardEntry(viewIndex, index);

    if (index != lastIndex) {
        viewIndex.shards[index] = std::move(viewIndex.shards.back());
        indexWorkspaceViewShardEntry(viewIndex, index);
    }

    viewIndex.shards.pop_back();
}

void updateWorkspaceViewShardEntry(
    WorkspaceViewIndex& viewIndex,
    std::size_t index,
    WorkspaceViewShardEntry&& shardEntry
)
{
    if (index >= viewIndex.shards.size()) {
        return;
    }

    unindexWorkspaceViewShardEntry(viewIndex, index);
    viewIndex.shards[index] = std::move(shardEntry);
    indexWorkspaceViewShardEntry(viewIndex, index);
}

void markWorkspaceViewIndexDirty(
    const App::Document& doc,
    const char* objectName,
    bool needsFullRebuild = false
)
{
    auto it = workspaceViewIndices.find(&doc);
    if (it == workspaceViewIndices.end()) {
        return;
    }

    if (needsFullRebuild) {
        it->second.needsFullRebuild = true;
    }

    if (objectName && *objectName) {
        it->second.dirtyObjectNames.insert(objectName);
    }
}

bool viewVolumeBounds(const SbViewVolume& viewVolume, Base::BoundBox3d& bounds)
{
    const float nearDistance = viewVolume.getNearDist();
    const float farDistance = nearDistance + viewVolume.getDepth();
    const std::array<SbVec2f, 4> planeCorners {
        SbVec2f(0.0f, 0.0f),
        SbVec2f(1.0f, 0.0f),
        SbVec2f(0.0f, 1.0f),
        SbVec2f(1.0f, 1.0f),
    };

    bool hasPoint = false;
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;
    const auto extendBounds = [&](const SbVec3f& point) {
        if (!hasPoint) {
            minX = maxX = point[0];
            minY = maxY = point[1];
            minZ = maxZ = point[2];
            hasPoint = true;
            return;
        }

        minX = std::min(minX, static_cast<double>(point[0]));
        minY = std::min(minY, static_cast<double>(point[1]));
        minZ = std::min(minZ, static_cast<double>(point[2]));
        maxX = std::max(maxX, static_cast<double>(point[0]));
        maxY = std::max(maxY, static_cast<double>(point[1]));
        maxZ = std::max(maxZ, static_cast<double>(point[2]));
    };

    for (const float distance : {nearDistance, farDistance}) {
        for (const auto& corner : planeCorners) {
            extendBounds(viewVolume.getPlanePoint(distance, corner));
        }
    }

    if (!hasPoint) {
        return false;
    }

    bounds = Base::BoundBox3d(minX, minY, minZ, maxX, maxY, maxZ);
    return bounds.IsValid();
}

bool retainedPathCacheCameraState(
    const Gui::View3DInventorViewer& viewer,
    Base::Vector3d& cameraPosition,
    std::array<double, 4>& cameraOrientation
)
{
    auto* renderManager = viewer.getSoRenderManager();
    auto* camera = renderManager ? renderManager->getCamera() : nullptr;
    if (!renderManager || !camera) {
        return false;
    }

    const SbVec3f position = camera->position.getValue();
    cameraPosition = Base::Vector3d(position[0], position[1], position[2]);

    float q0 = 0.0f;
    float q1 = 0.0f;
    float q2 = 0.0f;
    float q3 = 1.0f;
    camera->orientation.getValue().getValue(q0, q1, q2, q3);
    cameraOrientation = {
        static_cast<double>(q0),
        static_cast<double>(q1),
        static_cast<double>(q2),
        static_cast<double>(q3),
    };
    return true;
}

bool retainedPathCacheCameraOrientationMatches(const std::array<double, 4>& left, const std::array<double, 4>& right)
{
    constexpr double orientationTolerance = 1e-6;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::abs(left[index] - right[index]) > orientationTolerance) {
            return false;
        }
    }
    return true;
}

bool shouldReuseCachedRetainedDocumentPaths(
    const App::Document& doc,
    const WorkspaceViewIndex& viewIndex,
    const Base::Vector3d& focalPoint,
    const Base::Vector3d& cameraPosition,
    const std::array<double, 4>& cameraOrientation,
    double retainRadius,
    double movementTolerance
)
{
    if (!activeViewPrefetchState.hasCachedRetainedDocumentPaths) {
        return false;
    }

    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    if (activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath
        || activeViewPrefetchState.cachedRetainedViewRevision != viewIndex.revision) {
        return false;
    }

    if (std::abs(activeViewPrefetchState.cachedRetainRadius - retainRadius) > 1e-6) {
        return false;
    }

    if (focalPointMovedEnough(
            activeViewPrefetchState.cachedRetainedFocalPoint,
            focalPoint,
            movementTolerance
        )) {
        return false;
    }

    const double cameraPositionTolerance = std::max(1e-3, retainRadius * 1e-4);
    if (focalPointMovedEnough(
            activeViewPrefetchState.cachedRetainedCameraPosition,
            cameraPosition,
            cameraPositionTolerance
        )) {
        return false;
    }

    return retainedPathCacheCameraOrientationMatches(
        activeViewPrefetchState.cachedRetainedCameraOrientation,
        cameraOrientation
    );
}

void cacheRetainedDocumentPaths(
    const WorkspaceViewIndex& viewIndex,
    const Base::Vector3d& focalPoint,
    const Base::Vector3d& cameraPosition,
    const std::array<double, 4>& cameraOrientation,
    double retainRadius,
    const std::vector<std::string>& retainedDocumentPaths
)
{
    activeViewPrefetchState.cachedRetainedFocalPoint = focalPoint;
    activeViewPrefetchState.cachedRetainedCameraPosition = cameraPosition;
    activeViewPrefetchState.cachedRetainedCameraOrientation = cameraOrientation;
    activeViewPrefetchState.cachedRetainRadius = retainRadius;
    activeViewPrefetchState.cachedRetainedViewRevision = viewIndex.revision;
    activeViewPrefetchState.hasCachedRetainedDocumentPaths = true;
    activeViewPrefetchState.cachedRetainedDocumentPaths = retainedDocumentPaths;
}

Base::BoundBox3d boundsAroundFocusPoint(const Base::Vector3d& focusPoint, double retainRadius)
{
    return Base::BoundBox3d(
        focusPoint.x - retainRadius,
        focusPoint.y - retainRadius,
        focusPoint.z - retainRadius,
        focusPoint.x + retainRadius,
        focusPoint.y + retainRadius,
        focusPoint.z + retainRadius
    );
}

void appendAllWorkspaceViewShardIndices(
    const WorkspaceViewIndex& viewIndex,
    std::unordered_set<std::size_t>& seenShardIndices,
    std::vector<std::size_t>& candidateIndices
)
{
    candidateIndices.reserve(candidateIndices.size() + viewIndex.shards.size());
    for (std::size_t index = 0; index < viewIndex.shards.size(); ++index) {
        if (seenShardIndices.insert(index).second) {
            candidateIndices.push_back(index);
        }
    }
}

void appendWorkspaceViewShardCandidatesForBounds(
    const WorkspaceViewIndex& viewIndex,
    const Base::BoundBox3d& queryBounds,
    std::unordered_set<std::size_t>& seenShardIndices,
    std::vector<std::size_t>& candidateIndices
)
{
    if (!queryBounds.IsValid() || !viewIndex.hasSpatialGrid || viewIndex.shardIndicesByCell.empty()) {
        appendAllWorkspaceViewShardIndices(viewIndex, seenShardIndices, candidateIndices);
        return;
    }

    const WorkspaceViewCellCoord minCell = workspaceViewCellCoordForPoint(
        viewIndex,
        Base::Vector3d(queryBounds.MinX, queryBounds.MinY, queryBounds.MinZ)
    );
    const WorkspaceViewCellCoord maxCell = workspaceViewCellCoordForPoint(
        viewIndex,
        Base::Vector3d(queryBounds.MaxX, queryBounds.MaxY, queryBounds.MaxZ)
    );

    const std::size_t xCount = static_cast<std::size_t>(std::max(1, maxCell.x - minCell.x + 1));
    const std::size_t yCount = static_cast<std::size_t>(std::max(1, maxCell.y - minCell.y + 1));
    const std::size_t zCount = static_cast<std::size_t>(std::max(1, maxCell.z - minCell.z + 1));
    if ((xCount * yCount * zCount) >= viewIndex.shards.size()) {
        appendAllWorkspaceViewShardIndices(viewIndex, seenShardIndices, candidateIndices);
        return;
    }

    for (int x = minCell.x; x <= maxCell.x; ++x) {
        for (int y = minCell.y; y <= maxCell.y; ++y) {
            for (int z = minCell.z; z <= maxCell.z; ++z) {
                const auto cellIt = viewIndex.shardIndicesByCell.find({x, y, z});
                if (cellIt == viewIndex.shardIndicesByCell.end()) {
                    continue;
                }

                for (const auto index : cellIt->second) {
                    if (seenShardIndices.insert(index).second) {
                        candidateIndices.push_back(index);
                    }
                }
            }
        }
    }
}

bool rebuildWorkspaceViewIndex(const App::Document& doc, WorkspaceViewIndex& viewIndex)
{
    viewIndex.objectCount = doc.getObjects().size();
    viewIndex.shards.clear();
    viewIndex.shardIndicesByCell.clear();
    viewIndex.shardIndicesAlwaysInclude.clear();
    viewIndex.shardIndicesByPath.clear();
    viewIndex.shardIndexByObjectName.clear();
    viewIndex.cellSize = 1.0;
    viewIndex.hasSpatialGrid = false;
    viewIndex.needsFullRebuild = false;
    viewIndex.dirtyObjectNames.clear();
    viewIndex.shards.reserve(viewIndex.objectCount);

    bool hasSpatialBounds = false;
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;

    for (auto* object : doc.getObjects()) {
        if (!object) {
            continue;
        }

        const auto shardState = Import::StepLightweightWorkspaceRuntime::inspectLinkedShard(*object);
        if (!shardState.isWorkspaceShard || shardState.documentPath.empty()) {
            continue;
        }

        WorkspaceViewShardEntry entry;
        if (!buildWorkspaceViewShardEntry(*object, entry)) {
            continue;
        }

        viewIndex.shards.push_back(std::move(entry));
        if (!viewIndex.shards.back().hasBounds) {
            continue;
        }

        const auto& bounds = viewIndex.shards.back().bounds;
        if (!hasSpatialBounds) {
            minX = bounds.MinX;
            minY = bounds.MinY;
            minZ = bounds.MinZ;
            maxX = bounds.MaxX;
            maxY = bounds.MaxY;
            maxZ = bounds.MaxZ;
            hasSpatialBounds = true;
            continue;
        }

        minX = std::min(minX, bounds.MinX);
        minY = std::min(minY, bounds.MinY);
        minZ = std::min(minZ, bounds.MinZ);
        maxX = std::max(maxX, bounds.MaxX);
        maxY = std::max(maxY, bounds.MaxY);
        maxZ = std::max(maxZ, bounds.MaxZ);
    }

    if (hasSpatialBounds) {
        viewIndex.spatialBounds = Base::BoundBox3d(minX, minY, minZ, maxX, maxY, maxZ);
        const double spanX = std::max(1.0, maxX - minX);
        const double spanY = std::max(1.0, maxY - minY);
        const double spanZ = std::max(1.0, maxZ - minZ);
        const double maxSpan = std::max({spanX, spanY, spanZ});
        const double targetCellsPerAxis
            = std::max(1.0, std::cbrt(static_cast<double>(viewIndex.shards.size())));
        viewIndex.cellSize = std::max(1.0, maxSpan / targetCellsPerAxis);
        viewIndex.hasSpatialGrid = viewIndex.cellSize > 0.0;
    }

    if (viewIndex.hasSpatialGrid) {
        for (std::size_t index = 0; index < viewIndex.shards.size(); ++index) {
            indexWorkspaceViewShardEntry(viewIndex, index);
        }
    }
    else {
        for (std::size_t index = 0; index < viewIndex.shards.size(); ++index) {
            indexWorkspaceViewShardEntry(viewIndex, index);
        }
    }

    ++viewIndex.revision;
    return !viewIndex.shards.empty();
}

bool applyDirtyWorkspaceViewIndexUpdates(const App::Document& doc, WorkspaceViewIndex& viewIndex)
{
    if (viewIndex.dirtyObjectNames.empty()) {
        viewIndex.objectCount = doc.getObjects().size();
        return !viewIndex.shards.empty();
    }

    const std::vector<std::string> dirtyObjectNames(
        viewIndex.dirtyObjectNames.begin(),
        viewIndex.dirtyObjectNames.end()
    );
    viewIndex.dirtyObjectNames.clear();
    bool modified = false;

    for (const auto& objectName : dirtyObjectNames) {
        const auto shardIndexIt = viewIndex.shardIndexByObjectName.find(objectName);
        const bool hadIndexedShard = shardIndexIt != viewIndex.shardIndexByObjectName.end();
        App::DocumentObject* object = doc.getObject(objectName.c_str());
        WorkspaceViewShardEntry updatedEntry;

        if (!object || !buildWorkspaceViewShardEntry(*object, updatedEntry)) {
            if (hadIndexedShard) {
                removeWorkspaceViewShardEntry(viewIndex, shardIndexIt->second);
                modified = true;
            }
            continue;
        }

        if (hadIndexedShard) {
            updateWorkspaceViewShardEntry(viewIndex, shardIndexIt->second, std::move(updatedEntry));
            modified = true;
        }
        else {
            addWorkspaceViewShardEntry(viewIndex, std::move(updatedEntry));
            modified = true;
        }
    }

    viewIndex.objectCount = doc.getObjects().size();
    if (modified) {
        ++viewIndex.revision;
    }
    return !viewIndex.shards.empty();
}

WorkspaceViewIndex* ensureWorkspaceViewIndex(const App::Document& doc)
{
    auto [it, inserted] = workspaceViewIndices.try_emplace(&doc);
    WorkspaceViewIndex& viewIndex = it->second;
    if (inserted || viewIndex.needsFullRebuild || viewIndex.objectCount != doc.getObjects().size()) {
        if (!rebuildWorkspaceViewIndex(doc, viewIndex)) {
            workspaceViewIndices.erase(it);
            workspaceVisibleProxyPaths.erase(&doc);
            workspaceDesiredProxyPaths.erase(&doc);
            workspacePendingProxyPathsToShow.erase(&doc);
            workspacePendingProxyPathsToHide.erase(&doc);
            return nullptr;
        }
    }
    else if (!applyDirtyWorkspaceViewIndexUpdates(doc, viewIndex)) {
        workspaceViewIndices.erase(it);
        workspaceVisibleProxyPaths.erase(&doc);
        workspaceDesiredProxyPaths.erase(&doc);
        workspacePendingProxyPathsToShow.erase(&doc);
        workspacePendingProxyPathsToHide.erase(&doc);
        return nullptr;
    }

    return &viewIndex;
}

void clearWorkspaceViewIndex(const App::Document& doc)
{
    workspaceViewIndices.erase(&doc);
    workspaceVisibleProxyPaths.erase(&doc);
    workspaceDesiredProxyPaths.erase(&doc);
    workspacePendingProxyPathsToShow.erase(&doc);
    workspacePendingProxyPathsToHide.erase(&doc);
}

AssemblyGui::ViewProviderAssemblyLink* workspaceShardViewProvider(
    const App::Document& doc,
    const WorkspaceViewShardEntry& shardEntry
)
{
    if (shardEntry.objectName.empty()) {
        return nullptr;
    }

    auto* object = doc.getObject(shardEntry.objectName.c_str());
    if (!object) {
        return nullptr;
    }

    return Gui::Application::Instance->getViewProvider<AssemblyGui::ViewProviderAssemblyLink>(object);
}

std::unordered_set<std::string>& ensureWorkspaceVisibleProxyPaths(
    const App::Document& doc,
    const WorkspaceViewIndex& viewIndex
)
{
    auto [visibleIt, insertedVisible] = workspaceVisibleProxyPaths.try_emplace(&doc);
    auto& visiblePaths = visibleIt->second;
    if (insertedVisible) {
        visiblePaths.reserve(viewIndex.shards.size());
        for (const auto& shardEntry : viewIndex.shards) {
            if (!shardEntry.documentPath.empty()) {
                visiblePaths.insert(shardEntry.documentPath);
            }
        }
    }

    return visiblePaths;
}

void scheduleProxyVisibilityForRetainedPaths(
    const App::Document& doc,
    const WorkspaceViewIndex& viewIndex,
    const std::vector<std::string>& retainedDocumentPaths
)
{
    std::unordered_set<std::string> desiredVisiblePaths;
    desiredVisiblePaths.reserve(retainedDocumentPaths.size());
    for (const auto& retainedDocumentPath : retainedDocumentPaths) {
        const std::string normalizedRetainedPath = normalizePathForCompare(retainedDocumentPath);
        if (!normalizedRetainedPath.empty()) {
            desiredVisiblePaths.insert(normalizedRetainedPath);
        }
    }

    auto& visiblePaths = ensureWorkspaceVisibleProxyPaths(doc, viewIndex);
    if (visiblePaths == desiredVisiblePaths) {
        workspaceDesiredProxyPaths.erase(&doc);
        workspacePendingProxyPathsToShow.erase(&doc);
        workspacePendingProxyPathsToHide.erase(&doc);
        return;
    }

    std::vector<std::string> pendingHidePaths;
    pendingHidePaths.reserve(visiblePaths.size());
    for (const auto& documentPath : visiblePaths) {
        if (desiredVisiblePaths.find(documentPath) == desiredVisiblePaths.end()) {
            pendingHidePaths.push_back(documentPath);
        }
    }

    std::vector<std::string> pendingShowPaths;
    pendingShowPaths.reserve(desiredVisiblePaths.size());
    for (const auto& documentPath : desiredVisiblePaths) {
        if (visiblePaths.find(documentPath) == visiblePaths.end()) {
            pendingShowPaths.push_back(documentPath);
        }
    }

    workspaceDesiredProxyPaths[&doc] = std::move(desiredVisiblePaths);
    workspacePendingProxyPathsToShow[&doc] = std::move(pendingShowPaths);
    workspacePendingProxyPathsToHide[&doc] = std::move(pendingHidePaths);
}

bool processPendingProxyVisibilityUpdates(
    const App::Document& doc,
    const WorkspaceViewIndex& viewIndex
)
{
    auto pendingShowIt = workspacePendingProxyPathsToShow.find(&doc);
    auto pendingHideIt = workspacePendingProxyPathsToHide.find(&doc);
    if (pendingShowIt == workspacePendingProxyPathsToShow.end()
        && pendingHideIt == workspacePendingProxyPathsToHide.end()) {
        return false;
    }

    auto& visiblePaths = ensureWorkspaceVisibleProxyPaths(doc, viewIndex);

    const auto updateVisibilityForPath = [&](const std::string& documentPath, bool visible) {
        const auto pathIt = viewIndex.shardIndicesByPath.find(documentPath);
        if (pathIt == viewIndex.shardIndicesByPath.end()) {
            return;
        }

        for (const auto index : pathIt->second) {
            if (index >= viewIndex.shards.size()) {
                continue;
            }

            if (auto* viewProvider = workspaceShardViewProvider(doc, viewIndex.shards[index])) {
                viewProvider->setLightweightPlaceholderVisible(visible);
            }
        }
    };

    int updatedPathCount = 0;

    if (pendingHideIt != workspacePendingProxyPathsToHide.end()) {
        auto& pendingHidePaths = pendingHideIt->second;
        while (!pendingHidePaths.empty()
               && updatedPathCount < maxProxyVisibilityPathUpdatesPerCycle) {
            std::string documentPath = std::move(pendingHidePaths.back());
            pendingHidePaths.pop_back();
            if (visiblePaths.erase(documentPath) == 0) {
                continue;
            }

            updateVisibilityForPath(documentPath, false);
            ++updatedPathCount;
        }
    }

    if (pendingShowIt != workspacePendingProxyPathsToShow.end()) {
        auto& pendingShowPaths = pendingShowIt->second;
        while (!pendingShowPaths.empty()
               && updatedPathCount < maxProxyVisibilityPathUpdatesPerCycle) {
            std::string documentPath = std::move(pendingShowPaths.back());
            pendingShowPaths.pop_back();
            if (!visiblePaths.insert(documentPath).second) {
                continue;
            }

            updateVisibilityForPath(documentPath, true);
            ++updatedPathCount;
        }
    }

    pendingShowIt = workspacePendingProxyPathsToShow.find(&doc);
    pendingHideIt = workspacePendingProxyPathsToHide.find(&doc);
    const bool hasPendingShow = pendingShowIt != workspacePendingProxyPathsToShow.end()
        && !pendingShowIt->second.empty();
    const bool hasPendingHide = pendingHideIt != workspacePendingProxyPathsToHide.end()
        && !pendingHideIt->second.empty();
    if (!hasPendingShow && !hasPendingHide) {
        workspaceDesiredProxyPaths.erase(&doc);
        workspacePendingProxyPathsToShow.erase(&doc);
        workspacePendingProxyPathsToHide.erase(&doc);
    }

    return updatedPathCount != 0;
}

bool collectRetainedDocumentPathsForView(
    const WorkspaceViewIndex& viewIndex,
    const Gui::View3DInventorViewer& viewer,
    const Base::Vector3d& focusPoint,
    double retainRadius,
    std::vector<std::string>& retainedDocumentPaths
)
{
    auto* renderManager = viewer.getSoRenderManager();
    auto* camera = renderManager ? renderManager->getCamera() : nullptr;
    if (!renderManager || !camera) {
        return false;
    }

    const float aspectRatio = renderManager->getViewportRegion().getViewportAspectRatio();
    const SbViewVolume viewVolume = camera->getViewVolume(aspectRatio);
    const double retainRadiusSquared = retainRadius * retainRadius;
    const Base::BoundBox3d focusBounds = boundsAroundFocusPoint(focusPoint, retainRadius);
    Base::BoundBox3d frustumBounds;

    retainedDocumentPaths.clear();
    std::unordered_set<std::size_t> candidateIndexSet;
    std::vector<std::size_t> candidateIndices;
    candidateIndices.reserve(viewIndex.shards.size() / 8 + viewIndex.shardIndicesAlwaysInclude.size() + 8);
    for (const auto index : viewIndex.shardIndicesAlwaysInclude) {
        if (candidateIndexSet.insert(index).second) {
            candidateIndices.push_back(index);
        }
    }
    appendWorkspaceViewShardCandidatesForBounds(
        viewIndex,
        focusBounds,
        candidateIndexSet,
        candidateIndices
    );
    if (viewVolumeBounds(viewVolume, frustumBounds)) {
        appendWorkspaceViewShardCandidatesForBounds(
            viewIndex,
            frustumBounds,
            candidateIndexSet,
            candidateIndices
        );
    }

    retainedDocumentPaths.reserve(candidateIndices.size());
    for (const auto index : candidateIndices) {
        if (index >= viewIndex.shards.size()) {
            continue;
        }

        const auto& shardEntry = viewIndex.shards[index];
        if (shardEntry.documentPath.empty()) {
            continue;
        }

        if (!shardEntry.hasBounds) {
            retainedDocumentPaths.push_back(shardEntry.documentPath);
            continue;
        }

        const Base::Vector3d center = lightweightProxyBoundsCenter(shardEntry.bounds);
        const double dx = center.x - focusPoint.x;
        const double dy = center.y - focusPoint.y;
        const double dz = center.z - focusPoint.z;
        const double distanceSquared = dx * dx + dy * dy + dz * dz;

        bool retain = distanceSquared <= retainRadiusSquared;
        if (!retain) {
            const SbBox3f viewBox(
                SbVec3f(shardEntry.bounds.MinX, shardEntry.bounds.MinY, shardEntry.bounds.MinZ),
                SbVec3f(shardEntry.bounds.MaxX, shardEntry.bounds.MaxY, shardEntry.bounds.MaxZ)
            );
            retain = viewVolume.intersect(viewBox);
        }

        if (retain) {
            retainedDocumentPaths.push_back(shardEntry.documentPath);
        }
    }

    return true;
}

bool collectRetainedDocumentPathsForViewCached(
    const App::Document& doc,
    const WorkspaceViewIndex& viewIndex,
    const Gui::View3DInventorViewer& viewer,
    const Base::Vector3d& focusPoint,
    double retainRadius,
    double movementTolerance,
    std::vector<std::string>& retainedDocumentPaths
)
{
    Base::Vector3d cameraPosition;
    std::array<double, 4> cameraOrientation {0.0, 0.0, 0.0, 1.0};
    if (!retainedPathCacheCameraState(viewer, cameraPosition, cameraOrientation)) {
        clearCachedRetainedDocumentPaths();
        return false;
    }

    if (shouldReuseCachedRetainedDocumentPaths(
            doc,
            viewIndex,
            focusPoint,
            cameraPosition,
            cameraOrientation,
            retainRadius,
            movementTolerance
        )) {
        retainedDocumentPaths = activeViewPrefetchState.cachedRetainedDocumentPaths;
        return true;
    }

    if (!collectRetainedDocumentPathsForView(
            viewIndex,
            viewer,
            focusPoint,
            retainRadius,
            retainedDocumentPaths
        )) {
        clearCachedRetainedDocumentPaths();
        return false;
    }

    cacheRetainedDocumentPaths(
        viewIndex,
        focusPoint,
        cameraPosition,
        cameraOrientation,
        retainRadius,
        retainedDocumentPaths
    );
    return true;
}

void appendRecentlyRetainedDocumentPaths(std::vector<std::string>& retainedDocumentPaths)
{
    if (!activeViewPrefetchClock.isValid()) {
        return;
    }

    const qint64 now = activeViewPrefetchClock.elapsed();
    std::unordered_set<std::string> retainedPathSet;
    retainedPathSet.reserve(
        retainedDocumentPaths.size() + activeViewPrefetchState.retainedShardTimestamps.size()
    );
    for (const auto& retainedDocumentPath : retainedDocumentPaths) {
        const std::string normalizedPath = normalizePathForCompare(retainedDocumentPath);
        if (normalizedPath.empty()) {
            continue;
        }

        retainedPathSet.insert(normalizedPath);
        activeViewPrefetchState.retainedShardTimestamps[normalizedPath] = now;
    }

    for (auto it = activeViewPrefetchState.retainedShardTimestamps.begin();
         it != activeViewPrefetchState.retainedShardTimestamps.end();) {
        if ((now - it->second) > navigationRetentionGraceMs) {
            it = activeViewPrefetchState.retainedShardTimestamps.erase(it);
            continue;
        }
        ++it;
    }

    for (const auto& [normalizedPath, lastRetainedMs] :
         activeViewPrefetchState.retainedShardTimestamps) {
        if ((now - lastRetainedMs) > navigationRetentionGraceMs) {
            continue;
        }
        if (retainedPathSet.insert(normalizedPath).second) {
            retainedDocumentPaths.push_back(normalizedPath);
        }
    }
}

bool shouldAttemptWarmPrefetch(const App::Document& doc)
{
    if (!activeViewPrefetchClock.isValid()
        || !activeViewPrefetchState.hasObservedFocalPoint) {
        return false;
    }

    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    if (activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath) {
        return false;
    }

    const qint64 now = activeViewPrefetchClock.elapsed();
    if ((now - activeViewPrefetchState.lastMovementMs) < navigationWarmPrefetchDelayMs) {
        return false;
    }

    return (now - activeViewPrefetchState.lastWarmPrefetchAttemptMs)
        >= navigationWarmPrefetchIntervalMs;
}

bool hasStableFocusForFarShardSwap(const App::Document& doc, bool documentChanged)
{
    if (!activeViewPrefetchClock.isValid() || documentChanged
        || !activeViewPrefetchState.hasObservedFocalPoint) {
        return false;
    }

    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    if (activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath) {
        return false;
    }

    const qint64 now = activeViewPrefetchClock.elapsed();
    return (now - activeViewPrefetchState.lastMovementMs) >= navigationFarShardSwapStabilityDelayMs;
}

void noteWarmPrefetchAttempt()
{
    if (!activeViewPrefetchClock.isValid()) {
        activeViewPrefetchState.lastWarmPrefetchAttemptMs = 0;
        return;
    }

    activeViewPrefetchState.lastWarmPrefetchAttemptMs = activeViewPrefetchClock.elapsed();
}

bool shouldAttemptRebalance(
    const App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance,
    bool documentChanged
)
{
    const std::string normalizedDocumentPath = normalizePathForCompare(doc.FileName.getValue());
    const qint64 now = activeViewPrefetchClock.isValid() ? activeViewPrefetchClock.elapsed() : 0;

    if (documentChanged || activeViewPrefetchState.workspaceDocumentPath != normalizedDocumentPath
        || !activeViewPrefetchState.hasLastRebalanceFocalPoint) {
        return true;
    }

    if (focalPointMovedEnough(
            activeViewPrefetchState.lastRebalanceFocalPoint,
            focalPoint,
            movementTolerance
        )) {
        return true;
    }

    return (now - activeViewPrefetchState.lastRebalanceAttemptMs) >= navigationRebalanceIntervalMs;
}

void noteRebalanceAttempt(const Base::Vector3d& focalPoint)
{
    if (!activeViewPrefetchClock.isValid()) {
        activeViewPrefetchClock.start();
    }

    activeViewPrefetchState.lastRebalanceFocalPoint = focalPoint;
    activeViewPrefetchState.hasLastRebalanceFocalPoint = true;
    activeViewPrefetchState.lastRebalanceAttemptMs = activeViewPrefetchClock.elapsed();
}

bool processPendingWorkspaceLoadActions(
    App::Document& doc,
    const Base::Vector3d& focalPoint,
    double movementTolerance,
    const std::vector<std::string>& retainedDocumentPaths
)
{
    int processedLoadActions = 0;
    while (processedLoadActions < maxWorkspaceLoadActionsPerCycle) {
        if (activeViewPrefetchState.pendingRestoreNearFocus) {
            activeViewPrefetchState.pendingRestoreNearFocus = false;
            noteFocusPointAttempt(doc, focalPoint, movementTolerance);
            if (Import::StepLightweightWorkspaceRuntime::restoreDeferredInitialShardsNearPoint(
                    doc,
                    focalPoint,
                    1
                ) != 0) {
                ++processedLoadActions;
                continue;
            }
        }

        if (activeViewPrefetchState.pendingRebalanceNearFocus) {
            activeViewPrefetchState.pendingRebalanceNearFocus = false;
            noteRebalanceAttempt(focalPoint);
            if (Import::StepLightweightWorkspaceRuntime::rebalanceShardsNearPoint(
                    doc,
                    focalPoint,
                    1
                ) != 0) {
                ++processedLoadActions;
                continue;
            }
        }

        if (activeViewPrefetchState.pendingTrimOutsideRetainedPaths) {
            activeViewPrefetchState.pendingTrimOutsideRetainedPaths = false;
            if (!retainedDocumentPaths.empty()
                && Import::StepLightweightWorkspaceRuntime::trimShardsOutsideRetainedPaths(
                        doc,
                        retainedDocumentPaths,
                        maxViewTrimCountPerCycle
                    ) != 0) {
                ++processedLoadActions;
                continue;
            }
        }

        if (activeViewPrefetchState.pendingWarmPrefetchNearFocus) {
            activeViewPrefetchState.pendingWarmPrefetchNearFocus = false;
            noteWarmPrefetchAttempt();
            if (Import::StepLightweightWorkspaceRuntime::prefetchShardsNearPoint(
                    doc,
                    focalPoint,
                    maxWarmPrefetchCountPerCycle
                ) != 0) {
                ++processedLoadActions;
                continue;
            }
        }

        break;
    }

    return processedLoadActions != 0;
}

}  // namespace

using namespace AssemblyGui;

void LightweightWorkspaceProxyViewOptimizer::init()
{
    if (initialized) {
        return;
    }

    initialized = true;
    finishOpenConnection = App::GetApplication().signalFinishOpenDocument.connect([]() {
        QTimer::singleShot(0, []() {
            LightweightWorkspaceProxyViewOptimizer::refreshAllOpenDocuments();
            LightweightWorkspaceProxyViewOptimizer::optimizeActiveView();
        });
    });
    activateViewConnection = Gui::Application::Instance->signalActivateView.connect(
        [](const Gui::MDIView*) {
            resetFocusPointAttempt();
            QTimer::singleShot(0, []() { LightweightWorkspaceProxyViewOptimizer::optimizeActiveView(); });
        }
    );
    newObjectConnection = App::GetApplication().signalNewObject.connect(
        [](const App::DocumentObject& object) {
            auto* doc = object.getDocument();
            if (!doc || !Import::StepLightweightWorkspaceRuntime::isWorkspaceDocument(*doc)) {
                return;
            }

            markWorkspaceViewIndexDirty(*doc, object.getNameInDocument(), true);
        }
    );
    deletedObjectConnection = App::GetApplication().signalDeletedObject.connect(
        [](const App::DocumentObject& object) {
            auto* doc = object.getDocument();
            if (!doc || !Import::StepLightweightWorkspaceRuntime::isWorkspaceDocument(*doc)) {
                return;
            }

            markWorkspaceViewIndexDirty(*doc, object.getNameInDocument(), true);
        }
    );
    changedObjectConnection = App::GetApplication().signalChangedObject.connect(
        [](const App::DocumentObject& object, const App::Property& property) {
            auto* doc = object.getDocument();
            if (!doc || !Import::StepLightweightWorkspaceRuntime::isWorkspaceDocument(*doc)
                || !isWorkspaceViewIndexRelevantProperty(property)) {
                return;
            }

            markWorkspaceViewIndexDirty(*doc, object.getNameInDocument());
        }
    );
    deleteDocumentConnection = App::GetApplication().signalDeleteDocument.connect(
        [](const App::Document& doc) {
            clearWorkspaceViewIndex(doc);
            if (activeViewPrefetchState.workspaceDocumentPath
                == normalizePathForCompare(doc.FileName.getValue())) {
                resetFocusPointAttempt();
            }
        }
    );

    activeViewPrefetchClock.start();
    navigationPrefetchTimer = new QTimer();
    navigationPrefetchTimer->setInterval(navigationPrefetchIntervalMs);
    QObject::connect(
        navigationPrefetchTimer,
        &QTimer::timeout,
        []() { LightweightWorkspaceProxyViewOptimizer::optimizeActiveView(); }
    );
    navigationPrefetchTimer->start();

    QTimer::singleShot(0, []() { LightweightWorkspaceProxyViewOptimizer::refreshAllOpenDocuments(); });
}

void LightweightWorkspaceProxyViewOptimizer::refreshDocument(const App::Document& doc)
{
    if (!Import::StepLightweightWorkspaceRuntime::isWorkspaceDocument(doc)) {
        clearWorkspaceViewIndex(doc);
        return;
    }

    Import::StepLightweightWorkspaceRuntime::synchronizeLinkedShardProxies(doc);
    auto& viewIndex = workspaceViewIndices[&doc];
    viewIndex.needsFullRebuild = true;
    viewIndex.dirtyObjectNames.clear();
    workspaceDesiredProxyPaths.erase(&doc);
    workspacePendingProxyPathsToShow.erase(&doc);
    workspacePendingProxyPathsToHide.erase(&doc);
    ensureWorkspaceViewIndex(doc);
}

void LightweightWorkspaceProxyViewOptimizer::refreshAllOpenDocuments()
{
    for (auto* doc : App::GetApplication().getDocuments()) {
        if (doc) {
            refreshDocument(*doc);
        }
    }
}

void LightweightWorkspaceProxyViewOptimizer::optimizeActiveView()
{
    auto* mdiView = Gui::Application::Instance->activeView();
    auto* view = qobject_cast<Gui::View3DInventor*>(mdiView);
    if (!view) {
        resetFocusPointAttempt();
        return;
    }

    auto* viewer = view->getViewer();
    if (!viewer) {
        return;
    }

    auto* guiDoc = viewer->getDocument();
    if (!guiDoc) {
        resetFocusPointAttempt();
        return;
    }

    App::Document* doc = guiDoc->getDocument();
    if (!doc || !Import::StepLightweightWorkspaceRuntime::isWorkspaceDocument(*doc)) {
        resetFocusPointAttempt();
        return;
    }

    WorkspaceViewIndex* viewIndex = ensureWorkspaceViewIndex(*doc);
    if (!viewIndex || viewIndex->shards.empty()) {
        resetFocusPointAttempt();
        return;
    }

    const SbVec3f focalPoint = viewer->getFocalPoint();
    const Base::Vector3d focusPoint(focalPoint[0], focalPoint[1], focalPoint[2]);
    const double movementTolerance = std::max(1.0, static_cast<double>(viewer->getMaxDimension()) * 0.05);
    const bool documentChanged = observeFocusPoint(*doc, focusPoint, movementTolerance);
    std::vector<std::string> retainedDocumentPaths;
    bool hasRetainedDocumentPaths = false;
    if (shouldRefreshProxyVisibility(*doc, focusPoint, movementTolerance, documentChanged)) {
        const double retainRadius
            = std::max(1.0, static_cast<double>(viewer->getMaxDimension()) * retainedDistanceScale);
        if (collectRetainedDocumentPathsForViewCached(
                *doc,
                *viewIndex,
                *viewer,
                focusPoint,
                retainRadius,
                movementTolerance,
                retainedDocumentPaths
            )) {
            scheduleProxyVisibilityForRetainedPaths(*doc, *viewIndex, retainedDocumentPaths);
            appendRecentlyRetainedDocumentPaths(retainedDocumentPaths);
            noteProxyVisibilityUpdate(focusPoint);
            hasRetainedDocumentPaths = true;
        }
    }

    processPendingProxyVisibilityUpdates(*doc, *viewIndex);

    if (viewer->isAnimating() || viewer->isSpinning()) {
        return;
    }

    if (!shouldRetryFocusPoint(*doc, focusPoint, movementTolerance, documentChanged)) {
        return;
    }

    activeViewPrefetchState.pendingRestoreNearFocus = true;
    const bool stableForFarShardSwap = hasStableFocusForFarShardSwap(*doc, documentChanged);

    if (!hasRetainedDocumentPaths) {
        const double retainRadius
            = std::max(1.0, static_cast<double>(viewer->getMaxDimension()) * retainedDistanceScale);
        if (collectRetainedDocumentPathsForViewCached(
                *doc,
                *viewIndex,
                *viewer,
                focusPoint,
                retainRadius,
                movementTolerance,
                retainedDocumentPaths
            )) {
            appendRecentlyRetainedDocumentPaths(retainedDocumentPaths);
            hasRetainedDocumentPaths = true;
        }
    }

    if (stableForFarShardSwap && hasRetainedDocumentPaths) {
        activeViewPrefetchState.pendingTrimOutsideRetainedPaths = true;
    }

    if (stableForFarShardSwap
        && shouldAttemptRebalance(*doc, focusPoint, movementTolerance, documentChanged)) {
        activeViewPrefetchState.pendingRebalanceNearFocus = true;
    }
    if (shouldAttemptWarmPrefetch(*doc)) {
        activeViewPrefetchState.pendingWarmPrefetchNearFocus = true;
    }

    if (processPendingWorkspaceLoadActions(
            *doc,
            focusPoint,
            movementTolerance,
            retainedDocumentPaths
        )) {
        return;
    }
}
