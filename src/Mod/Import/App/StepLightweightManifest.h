// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Mod/Import/ImportGlobal.h>

namespace Base
{
class FileInfo;
}

namespace Import
{

struct ImportExport StepLightweightShard
{
    std::string documentPath;
    std::string assemblyObjectName;
    std::string linkObjectName;
    std::string label;
};

class ImportExport StepLightweightManifest
{
public:
    static constexpr int CurrentVersion = 1;

    bool load(const std::string& manifestPath);
    bool save(const std::string& manifestPath) const;
    bool matchesSource(const Base::FileInfo& file) const;
    bool hasAllShardFiles() const;

    int version = CurrentVersion;
    std::string sourcePath;
    std::uint64_t sourceSize = 0;
    std::int64_t sourceMTime = 0;
    std::string masterDocumentPath;
    std::string masterObjectName;
    std::vector<StepLightweightShard> shards;
};

}  // namespace Import
