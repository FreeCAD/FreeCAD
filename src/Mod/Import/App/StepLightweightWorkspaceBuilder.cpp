// SPDX-License-Identifier: LGPL-2.1-or-later

#include "StepLightweightWorkspaceBuilder.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>

#include <TDF_ChildIterator.hxx>
#include <TDF_LabelSequence.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <Base/BoundBox.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "StepLightweightManifest.h"
#include "Tools.h"

using namespace Import;

namespace
{

constexpr const char* manifestFileName = "manifest.txt";
constexpr const char* masterFileName = "master.fcstd";
constexpr const char* lightweightProxyBoundsMinPropertyName = "LightweightProxyBoundsMin";
constexpr const char* lightweightProxyBoundsMaxPropertyName = "LightweightProxyBoundsMax";
constexpr const char* lightweightWorkspaceManifestPathPropertyName = "LightweightWorkspaceManifestPath";
constexpr const char* lightweightWorkspaceShardLinkPropertyName = "LightweightWorkspaceShardLink";
constexpr const char* lightweightWorkspaceShardDocumentPathPropertyName
    = "LightweightWorkspaceShardDocumentPath";
constexpr const char* lightweightWorkspaceShardObjectNamePropertyName
    = "LightweightWorkspaceShardObjectName";

std::string cacheDirectoryForStepFile(const Base::FileInfo& stepFile)
{
    return stepFile.filePath() + ".fcstepcache";
}

std::string shardDirectoryForStepFile(const Base::FileInfo& stepFile)
{
    return cacheDirectoryForStepFile(stepFile) + "/shards";
}

std::string manifestPathForStepFile(const Base::FileInfo& stepFile)
{
    return cacheDirectoryForStepFile(stepFile) + "/" + manifestFileName;
}

std::string masterDocumentPathForStepFile(const Base::FileInfo& stepFile)
{
    return cacheDirectoryForStepFile(stepFile) + "/" + masterFileName;
}

StepLightweightWorkspaceCacheStatus readyCacheStatusForManifest(
    const Base::FileInfo& stepFile,
    const StepLightweightManifest& manifest
)
{
    StepLightweightWorkspaceCacheStatus status;
    status.sourcePath = stepFile.filePath();
    status.cacheDirectory = cacheDirectoryForStepFile(stepFile);
    status.manifestPath = manifestPathForStepFile(stepFile);
    status.masterDocumentPath = manifest.masterDocumentPath;
    status.masterObjectName = manifest.masterObjectName;
    status.manifestVersion = manifest.version;
    status.shardCount = manifest.shards.size();
    status.hasCacheDirectory = true;
    status.hasManifest = true;
    status.manifestReadable = true;
    status.sourceMatches = true;
    status.hasMasterFile = true;
    status.hasAllShardFiles = true;
    status.isReady = true;
    return status;
}

void populateWorkspaceResultFromStatus(
    const StepLightweightWorkspaceCacheStatus& status,
    StepLightweightWorkspaceResult& result
)
{
    result.masterDocumentPath = status.masterDocumentPath;
    result.masterObjectName = status.masterObjectName;
    result.status = status;
}

std::uint64_t minBytesFromMiB(int minFileSizeMB)
{
    if (minFileSizeMB <= 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(minFileSizeMB) * 1024ULL * 1024ULL;
}

void analyzeLabelTree(
    TDF_Label label,
    const Handle(XCAFDoc_ShapeTool)& shapeTool,
    const Handle(XCAFDoc_ColorTool)& colorTool,
    bool importHidden,
    StepLightweightWorkspaceAnalysis& analysis,
    std::size_t maxShapeNodeCount
)
{
    if (label.IsNull()) {
        return;
    }
    if (!importHidden && !colorTool->IsVisible(label)) {
        return;
    }

    if (shapeTool->IsShape(label) && !shapeTool->IsSubShape(label)) {
        ++analysis.shapeNodeCount;
        if (maxShapeNodeCount > 0 && analysis.shapeNodeCount >= maxShapeNodeCount) {
            return;
        }
    }
    if (shapeTool->IsAssembly(label)) {
        ++analysis.assemblyCount;
    }

    for (TDF_ChildIterator it(label); it.More(); it.Next()) {
        analyzeLabelTree(
            it.Value(),
            shapeTool,
            colorTool,
            importHidden,
            analysis,
            maxShapeNodeCount
        );
        if (maxShapeNodeCount > 0 && analysis.shapeNodeCount >= maxShapeNodeCount) {
            return;
        }
    }
}

ImportOCAFOptions makeShardOptions(const ImportOCAFOptions& source)
{
    ImportOCAFOptions options = source;
    options.mode = ImportOCAF2::SingleDoc;
    options.merge = false;
    options.useLinkGroup = true;
    options.expandCompound = false;
    options.showProgress = false;
    return options;
}

Base::BoundBox3d proxyBoundsForShape(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return Base::BoundBox3d();
    }

    return Part::TopoShape(shape).getBoundBox();
}

App::PropertyVector* ensureVectorProperty(
    App::DocumentObject& object,
    const char* name,
    const char* doc
)
{
    auto* property = dynamic_cast<App::PropertyVector*>(object.getPropertyByName(name));
    if (!property) {
        object.addDynamicProperty(
            "App::PropertyVector",
            name,
            "Lightweight",
            doc,
            App::Prop_None,
            false,
            true
        );
        property = dynamic_cast<App::PropertyVector*>(object.getPropertyByName(name));
    }
    return property;
}

App::PropertyString* ensureStringProperty(
    App::DocumentObject& object,
    const char* name,
    const char* doc
)
{
    auto* property = dynamic_cast<App::PropertyString*>(object.getPropertyByName(name));
    if (!property) {
        object.addDynamicProperty(
            "App::PropertyString",
            name,
            "Lightweight",
            doc,
            App::Prop_None,
            false,
            true
        );
        property = dynamic_cast<App::PropertyString*>(object.getPropertyByName(name));
    }
    return property;
}

App::PropertyBool* ensureBoolProperty(
    App::DocumentObject& object,
    const char* name,
    const char* doc
)
{
    auto* property = dynamic_cast<App::PropertyBool*>(object.getPropertyByName(name));
    if (!property) {
        object.addDynamicProperty(
            "App::PropertyBool",
            name,
            "Lightweight",
            doc,
            App::Prop_None,
            false,
            true
        );
        property = dynamic_cast<App::PropertyBool*>(object.getPropertyByName(name));
    }
    return property;
}

}  // namespace

StepLightweightWorkspaceBuilder::StepLightweightWorkspaceBuilder(
    Handle(TDocStd_Document) hDoc,
    const Base::FileInfo& stepFile,
    const ImportOCAFOptions& options
)
    : hDoc(hDoc)
    , stepFile(stepFile)
    , options(options)
{}

bool StepLightweightWorkspaceBuilder::restoreFromCache(
    const Base::FileInfo& stepFile,
    StepLightweightWorkspaceResult& result
)
{
    const StepLightweightWorkspaceCacheStatus status = inspectCache(stepFile);
    return restoreFromCache(status, result);
}

bool StepLightweightWorkspaceBuilder::restoreFromCache(
    const StepLightweightWorkspaceCacheStatus& status,
    StepLightweightWorkspaceResult& result
)
{
    if (!status.isReady) {
        return false;
    }

    populateWorkspaceResultFromStatus(status, result);
    result.restoredFromCache = true;
    return true;
}

StepLightweightWorkspaceCacheStatus StepLightweightWorkspaceBuilder::inspectCache(
    const Base::FileInfo& stepFile
)
{
    StepLightweightWorkspaceCacheStatus status;
    status.sourcePath = stepFile.filePath();
    status.cacheDirectory = cacheDirectoryForStepFile(stepFile);
    status.manifestPath = manifestPathForStepFile(stepFile);

    const Base::FileInfo cacheDir(status.cacheDirectory);
    status.hasCacheDirectory = cacheDir.exists();

    const Base::FileInfo manifestFile(status.manifestPath);
    status.hasManifest = manifestFile.exists();
    if (!status.hasManifest) {
        return status;
    }

    StepLightweightManifest manifest;
    if (!manifest.load(status.manifestPath)) {
        return status;
    }

    status.manifestReadable = true;
    status.manifestVersion = manifest.version;
    status.masterDocumentPath = manifest.masterDocumentPath;
    status.masterObjectName = manifest.masterObjectName;
    status.shardCount = manifest.shards.size();
    status.sourceMatches = manifest.matchesSource(stepFile);
    status.hasMasterFile = Base::FileInfo(manifest.masterDocumentPath).exists();

    for (const auto& shard : manifest.shards) {
        if (!Base::FileInfo(shard.documentPath).exists()) {
            ++status.missingShardCount;
        }
    }

    status.hasAllShardFiles = status.hasMasterFile && status.missingShardCount == 0;
    status.isReady = status.sourceMatches && status.hasAllShardFiles;
    return status;
}

StepLightweightWorkspaceAnalysis StepLightweightWorkspaceBuilder::analyze(
    Handle(TDocStd_Document) hDoc,
    bool importHidden,
    std::size_t maxShapeNodeCount
)
{
    StepLightweightWorkspaceAnalysis analysis;
    auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
    auto colorTool = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());

    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes(labels);
    for (Standard_Integer index = 1; index <= labels.Length(); ++index) {
        TDF_Label label = labels.Value(index);
        if (!importHidden && !colorTool->IsVisible(label)) {
            continue;
        }

        ++analysis.rootCount;
        analyzeLabelTree(
            label,
            shapeTool,
            colorTool,
            importHidden,
            analysis,
            maxShapeNodeCount
        );
        if (maxShapeNodeCount > 0 && analysis.shapeNodeCount >= maxShapeNodeCount) {
            break;
        }
    }

    return analysis;
}

bool StepLightweightWorkspaceBuilder::exceedsFileSizeThreshold(
    const Base::FileInfo& stepFile,
    int minFileSizeMB
)
{
    const std::uint64_t minBytes = minBytesFromMiB(minFileSizeMB);
    if (minBytes == 0) {
        return false;
    }

    return static_cast<std::uint64_t>(stepFile.size()) >= minBytes;
}

bool StepLightweightWorkspaceBuilder::exceedsThresholds(
    const Base::FileInfo& stepFile,
    const StepLightweightWorkspaceAnalysis& analysis,
    int minFileSizeMB,
    int minNodeCount
)
{
    const bool dueToFileSize = exceedsFileSizeThreshold(stepFile, minFileSizeMB);
    const bool dueToNodeCount = minNodeCount > 0
        && analysis.shapeNodeCount >= static_cast<std::size_t>(minNodeCount);
    return dueToFileSize || dueToNodeCount;
}

StepLightweightWorkspaceResult StepLightweightWorkspaceBuilder::prepare(
    Handle(TDocStd_Document) hDoc,
    const Base::FileInfo& stepFile,
    const ImportOCAFOptions& options,
    bool rebuild
)
{
    StepLightweightWorkspaceResult result;
    if (!rebuild) {
        const StepLightweightWorkspaceCacheStatus status = inspectCache(stepFile);
        if (restoreFromCache(status, result)) {
            return result;
        }
    }

    StepLightweightWorkspaceBuilder builder(hDoc, stepFile, options);
    return builder.build();
}

std::string StepLightweightWorkspaceBuilder::cacheDirectory() const
{
    return cacheDirectoryForStepFile(stepFile);
}

std::string StepLightweightWorkspaceBuilder::shardDirectory() const
{
    return shardDirectoryForStepFile(stepFile);
}

std::string StepLightweightWorkspaceBuilder::manifestPath() const
{
    return manifestPathForStepFile(stepFile);
}

std::string StepLightweightWorkspaceBuilder::masterDocumentPath() const
{
    return masterDocumentPathForStepFile(stepFile);
}

std::string StepLightweightWorkspaceBuilder::sanitizeFileName(const std::string& value) const
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') {
            result.push_back(static_cast<char>(ch));
        }
        else {
            result.push_back('_');
        }
    }

    while (!result.empty() && result.front() == '_') {
        result.erase(result.begin());
    }
    while (!result.empty() && result.back() == '_') {
        result.pop_back();
    }

    return result.empty() ? std::string("Shard") : result;
}

std::string StepLightweightWorkspaceBuilder::makeUniqueShardPath(
    const std::string& label,
    std::size_t index
) const
{
    std::ostringstream stream;
    stream << shardDirectory() << '/' << sanitizeFileName(label) << '_' << index << ".fcstd";
    return stream.str();
}

std::string StepLightweightWorkspaceBuilder::makeUniqueObjectName(
    const std::string& label,
    std::size_t index
) const
{
    std::ostringstream stream;
    stream << sanitizeFileName(label) << '_' << index;
    return stream.str();
}

std::vector<TDF_Label> StepLightweightWorkspaceBuilder::collectRootLabels() const
{
    std::vector<TDF_Label> labelsOut;
    auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
    auto colorTool = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());

    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes(labels);
    labelsOut.reserve(labels.Length());
    for (Standard_Integer index = 1; index <= labels.Length(); ++index) {
        TDF_Label label = labels.Value(index);
        if (!options.importHidden && !colorTool->IsVisible(label)) {
            continue;
        }
        labelsOut.push_back(label);
    }

    if (labelsOut.size() == 1) {
        TDF_Label rootLabel = labelsOut.front();
        TDF_Label referredLabel;
        if (!shapeTool->GetReferredShape(rootLabel, referredLabel)) {
            referredLabel = rootLabel;
        }

        TDF_LabelSequence components;
        if (shapeTool->IsAssembly(referredLabel) && shapeTool->GetComponents(referredLabel, components)
            && components.Length() > 1) {
            std::vector<TDF_Label> componentLabels;
            componentLabels.reserve(components.Length());
            for (Standard_Integer index = 1; index <= components.Length(); ++index) {
                TDF_Label componentLabel = components.Value(index);
                if (!options.importHidden && !colorTool->IsVisible(componentLabel)) {
                    continue;
                }
                componentLabels.push_back(componentLabel);
            }
            if (componentLabels.size() > 1) {
                return componentLabels;
            }
        }
    }

    return labelsOut;
}

App::Document* StepLightweightWorkspaceBuilder::createDocument(const std::string& objectName) const
{
    App::DocumentInitFlags initFlags {.createView = false};
    return App::GetApplication().newDocument(objectName.c_str(), objectName.c_str(), initFlags);
}

void StepLightweightWorkspaceBuilder::saveDocumentAs(
    App::Document& doc,
    const std::string& filePath
) const
{
    Base::FileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        fileInfo.deleteFile();
    }
    if (!doc.saveAs(filePath.c_str())) {
        throw Base::FileException("Cannot save lightweight workspace document", filePath.c_str());
    }
}

App::DocumentObject* StepLightweightWorkspaceBuilder::createAssemblyObject(
    App::Document* doc,
    const std::string& objectName,
    const std::string& label
) const
{
    auto* assembly = doc->addObject("Assembly::AssemblyObject", objectName.c_str());
    if (!assembly) {
        throw Base::RuntimeError("Cannot create Assembly::AssemblyObject");
    }
    assembly->Label.setValue(label.c_str());
    return assembly;
}

void StepLightweightWorkspaceBuilder::addObjectToAssembly(
    App::DocumentObject* assembly,
    App::DocumentObject* child
) const
{
    auto* part = dynamic_cast<App::Part*>(assembly);
    if (!part) {
        throw Base::RuntimeError("Lightweight workspace assembly root is not an App::Part");
    }
    part->addObject(child);
}

App::DocumentObject* StepLightweightWorkspaceBuilder::createAssemblyLink(
    App::DocumentObject* assembly,
    const StepLightweightShard& shard,
    const Base::BoundBox3d& proxyBounds
) const
{
    auto* part = dynamic_cast<App::Part*>(assembly);
    if (!part) {
        throw Base::RuntimeError("Lightweight workspace master is not an App::Part");
    }

    auto* link = part->addObject("Assembly::AssemblyLink", shard.linkObjectName.c_str());
    if (!link) {
        throw Base::RuntimeError("Cannot create Assembly::AssemblyLink");
    }

    auto* linkedObject = dynamic_cast<App::PropertyXLink*>(link->getPropertyByName("LinkedObject"));
    auto* loadMode = dynamic_cast<App::PropertyEnumeration*>(link->getPropertyByName("LoadMode"));
    if (!linkedObject || !loadMode) {
        throw Base::RuntimeError("AssemblyLink is missing workspace properties");
    }

    linkedObject->setAllowPartial(true);
    loadMode->setValue("Auto");
    link->Label.setValue(shard.label.c_str());

    if (proxyBounds.IsValid()) {
        auto* minProperty = ensureVectorProperty(
            *link,
            lightweightProxyBoundsMinPropertyName,
            "Minimum corner for unloaded lightweight shard proxy."
        );
        auto* maxProperty = ensureVectorProperty(
            *link,
            lightweightProxyBoundsMaxPropertyName,
            "Maximum corner for unloaded lightweight shard proxy."
        );
        if (!minProperty || !maxProperty) {
            throw Base::RuntimeError("Cannot create lightweight shard proxy properties");
        }

        minProperty->setValue(proxyBounds.MinX, proxyBounds.MinY, proxyBounds.MinZ);
        maxProperty->setValue(proxyBounds.MaxX, proxyBounds.MaxY, proxyBounds.MaxZ);
    }

    auto* workspaceShardProperty = ensureBoolProperty(
        *link,
        lightweightWorkspaceShardLinkPropertyName,
        "Marks the link as a lightweight workspace shard so it can restore lazily."
    );
    if (!workspaceShardProperty) {
        throw Base::RuntimeError("Cannot create lightweight shard marker property");
    }
    workspaceShardProperty->setValue(true);

    auto* shardDocumentPathProperty = ensureStringProperty(
        *link,
        lightweightWorkspaceShardDocumentPathPropertyName,
        "Canonical shard document path for lightweight workspace restore."
    );
    auto* shardObjectNameProperty = ensureStringProperty(
        *link,
        lightweightWorkspaceShardObjectNamePropertyName,
        "Canonical shard root object name for lightweight workspace restore."
    );
    if (!shardDocumentPathProperty || !shardObjectNameProperty) {
        throw Base::RuntimeError("Cannot create lightweight shard restore metadata");
    }
    shardDocumentPathProperty->setValue(shard.documentPath.c_str());
    shardObjectNameProperty->setValue(shard.assemblyObjectName.c_str());

    return link;
}

StepLightweightWorkspaceResult StepLightweightWorkspaceBuilder::build()
{
    Base::FileInfo cacheDir(cacheDirectory());
    if (cacheDir.exists()) {
        cacheDir.deleteDirectoryRecursive();
    }
    if (!cacheDir.createDirectories()) {
        throw Base::FileException("Cannot create lightweight cache directory", cacheDir.filePath());
    }

    Base::FileInfo shardDir(shardDirectory());
    if (!shardDir.createDirectories()) {
        throw Base::FileException(
            "Cannot create lightweight shard directory",
            shardDir.filePath()
        );
    }

    auto rootLabels = collectRootLabels();
    if (rootLabels.empty()) {
        throw Base::RuntimeError("No top-level STEP shapes available for lightweight workspace");
    }

    StepLightweightManifest manifest;
    manifest.sourcePath = stepFile.filePath();
    manifest.sourceSize = stepFile.size();
    manifest.sourceMTime = static_cast<std::int64_t>(stepFile.lastModified().getTime_t());
    manifest.masterDocumentPath = masterDocumentPath();
    auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
    std::vector<Base::BoundBox3d> shardProxyBounds;
    shardProxyBounds.reserve(rootLabels.size());

    ImportOCAFOptions shardOptions = makeShardOptions(options);
    for (std::size_t index = 0; index < rootLabels.size(); ++index) {
        TDF_Label label = rootLabels[index];
        std::string labelName = Tools::labelName(label);
        if (labelName.empty()) {
            std::ostringstream fallback;
            fallback << stepFile.fileNamePure() << "_Shard" << (index + 1);
            labelName = fallback.str();
        }

        std::string shardObjectName = makeUniqueObjectName(labelName, index + 1);
        std::string shardAssemblyName = shardObjectName + "_Assembly";
        std::string shardFilePath = makeUniqueShardPath(labelName, index + 1);
        const Base::BoundBox3d proxyBounds = proxyBoundsForShape(shapeTool->GetShape(label));

        App::Document* shardDoc = createDocument(shardObjectName);
        try {
            ImportOCAFExt ocaf(hDoc, shardDoc, stepFile.fileNamePure());
            ocaf.setImportOptions(shardOptions);
            auto* imported = ocaf.loadLabel(label);
            if (!imported) {
                throw Base::RuntimeError("Failed to import lightweight workspace shard");
            }

            auto* shardAssembly = createAssemblyObject(shardDoc, shardAssemblyName, labelName);
            addObjectToAssembly(shardAssembly, imported);
            shardAssembly->recomputeFeature(true);
            saveDocumentAs(*shardDoc, shardFilePath);

            StepLightweightShard shard;
            shard.documentPath = shardFilePath;
            shard.assemblyObjectName = shardAssembly->getNameInDocument();
            shard.linkObjectName = shardObjectName + "_Link";
            shard.label = labelName;
            manifest.shards.push_back(std::move(shard));
            shardProxyBounds.push_back(proxyBounds);
        }
        catch (...) {
            App::GetApplication().closeDocument(shardDoc->getName());
            throw;
        }
        App::GetApplication().closeDocument(shardDoc->getName());
    }

    App::Document* masterDoc = createDocument(stepFile.fileNamePure() + "_Workspace");
    try {
        auto* masterAssembly = createAssemblyObject(masterDoc, "Assembly", stepFile.fileNamePure());
        manifest.masterObjectName = masterAssembly->getNameInDocument();
        auto* manifestPathProperty = ensureStringProperty(
            *masterAssembly,
            lightweightWorkspaceManifestPathPropertyName,
            "Path to the lightweight workspace manifest backing this assembly."
        );
        if (!manifestPathProperty) {
            throw Base::RuntimeError("Cannot create lightweight workspace manifest property");
        }
        manifestPathProperty->setValue(manifestPath().c_str());
        for (std::size_t index = 0; index < manifest.shards.size(); ++index) {
            createAssemblyLink(masterAssembly, manifest.shards[index], shardProxyBounds[index]);
        }
        masterAssembly->recomputeFeature(true);
        saveDocumentAs(*masterDoc, manifest.masterDocumentPath);
    }
    catch (...) {
        App::GetApplication().closeDocument(masterDoc->getName());
        throw;
    }
    App::GetApplication().closeDocument(masterDoc->getName());

    if (!manifest.save(manifestPath())) {
        throw Base::FileException("Cannot save lightweight workspace manifest", manifestPath().c_str());
    }

    StepLightweightWorkspaceResult result;
    populateWorkspaceResultFromStatus(readyCacheStatusForManifest(stepFile, manifest), result);
    return result;
}
