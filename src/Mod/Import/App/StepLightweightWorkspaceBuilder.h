// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <string>

#include <TDocStd_Document.hxx>

#include <Base/BoundBox.h>
#include <Base/FileInfo.h>
#include <Mod/Import/ImportGlobal.h>

#include "ImportOCAF2.h"
#include "StepLightweightManifest.h"

namespace App
{
class Document;
class DocumentObject;
}

namespace Import
{

struct ImportExport StepLightweightWorkspaceCacheStatus
{
    std::string sourcePath;
    std::string cacheDirectory;
    std::string manifestPath;
    std::string masterDocumentPath;
    std::string masterObjectName;
    int manifestVersion = 0;
    std::size_t shardCount = 0;
    std::size_t missingShardCount = 0;
    bool hasCacheDirectory = false;
    bool hasManifest = false;
    bool manifestReadable = false;
    bool sourceMatches = false;
    bool hasMasterFile = false;
    bool hasAllShardFiles = false;
    bool isReady = false;
};

struct ImportExport StepLightweightWorkspaceResult
{
    std::string masterDocumentPath;
    std::string masterObjectName;
    StepLightweightWorkspaceCacheStatus status;
    bool restoredFromCache = false;
};

struct ImportExport StepLightweightWorkspaceAnalysis
{
    std::size_t rootCount = 0;
    std::size_t shapeNodeCount = 0;
    std::size_t assemblyCount = 0;
};

class ImportExport StepLightweightWorkspaceBuilder
{
public:
    StepLightweightWorkspaceBuilder(
        Handle(TDocStd_Document) hDoc,
        const Base::FileInfo& stepFile,
        const ImportOCAFOptions& options
    );

    static bool restoreFromCache(
        const Base::FileInfo& stepFile,
        StepLightweightWorkspaceResult& result
    );
    static bool restoreFromCache(
        const StepLightweightWorkspaceCacheStatus& status,
        StepLightweightWorkspaceResult& result
    );
    static StepLightweightWorkspaceCacheStatus inspectCache(const Base::FileInfo& stepFile);
    static StepLightweightWorkspaceAnalysis analyze(
        Handle(TDocStd_Document) hDoc,
        bool importHidden,
        std::size_t maxShapeNodeCount = 0
    );
    static bool exceedsFileSizeThreshold(const Base::FileInfo& stepFile, int minFileSizeMB);
    static bool exceedsThresholds(
        const Base::FileInfo& stepFile,
        const StepLightweightWorkspaceAnalysis& analysis,
        int minFileSizeMB,
        int minNodeCount
    );
    static StepLightweightWorkspaceResult prepare(
        Handle(TDocStd_Document) hDoc,
        const Base::FileInfo& stepFile,
        const ImportOCAFOptions& options,
        bool rebuild = false
    );

    StepLightweightWorkspaceResult build();

private:
    std::string cacheDirectory() const;
    std::string shardDirectory() const;
    std::string manifestPath() const;
    std::string masterDocumentPath() const;
    std::string sanitizeFileName(const std::string& value) const;
    std::string makeUniqueShardPath(const std::string& label, std::size_t index) const;
    std::string makeUniqueObjectName(const std::string& label, std::size_t index) const;
    std::vector<TDF_Label> collectRootLabels() const;
    App::Document* createDocument(const std::string& objectName) const;
    void saveDocumentAs(App::Document& doc, const std::string& filePath) const;
    App::DocumentObject* createAssemblyObject(
        App::Document* doc,
        const std::string& objectName,
        const std::string& label
    ) const;
    void addObjectToAssembly(App::DocumentObject* assembly, App::DocumentObject* child) const;
    App::DocumentObject* createAssemblyLink(
        App::DocumentObject* assembly,
        const StepLightweightShard& shard,
        const Base::BoundBox3d& proxyBounds
    ) const;

private:
    Handle(TDocStd_Document) hDoc;
    Base::FileInfo stepFile;
    ImportOCAFOptions options;
};

}  // namespace Import
