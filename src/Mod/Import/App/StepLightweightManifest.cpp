// SPDX-License-Identifier: LGPL-2.1-or-later

#include "StepLightweightManifest.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>

using namespace Import;

bool StepLightweightManifest::load(const std::string& manifestPath)
{
    std::ifstream stream(manifestPath);
    if (!stream.is_open()) {
        return false;
    }

    StepLightweightManifest parsed;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream input(line);
        std::string key;
        input >> key;
        if (key == "version") {
            input >> parsed.version;
        }
        else if (key == "source_path") {
            input >> std::quoted(parsed.sourcePath);
        }
        else if (key == "source_size") {
            input >> parsed.sourceSize;
        }
        else if (key == "source_mtime") {
            input >> parsed.sourceMTime;
        }
        else if (key == "master_path") {
            input >> std::quoted(parsed.masterDocumentPath);
        }
        else if (key == "master_object") {
            input >> std::quoted(parsed.masterObjectName);
        }
        else if (key == "shard") {
            StepLightweightShard shard;
            input >> std::quoted(shard.documentPath) >> std::quoted(shard.assemblyObjectName)
                >> std::quoted(shard.linkObjectName) >> std::quoted(shard.label);
            parsed.shards.push_back(std::move(shard));
        }
    }

    if (parsed.version != CurrentVersion || parsed.sourcePath.empty()
        || parsed.masterDocumentPath.empty() || parsed.masterObjectName.empty()
        || parsed.shards.empty()) {
        return false;
    }

    *this = std::move(parsed);
    return true;
}

bool StepLightweightManifest::save(const std::string& manifestPath) const
{
    std::ofstream stream(manifestPath, std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    stream << "version " << version << '\n';
    stream << "source_path " << std::quoted(sourcePath) << '\n';
    stream << "source_size " << sourceSize << '\n';
    stream << "source_mtime " << sourceMTime << '\n';
    stream << "master_path " << std::quoted(masterDocumentPath) << '\n';
    stream << "master_object " << std::quoted(masterObjectName) << '\n';
    for (const auto& shard : shards) {
        stream << "shard " << std::quoted(shard.documentPath) << ' '
               << std::quoted(shard.assemblyObjectName) << ' ' << std::quoted(shard.linkObjectName)
               << ' ' << std::quoted(shard.label) << '\n';
    }

    return stream.good();
}

bool StepLightweightManifest::matchesSource(const Base::FileInfo& file) const
{
    if (!file.exists()) {
        return false;
    }

    if (sourcePath != file.filePath() || sourceSize != file.size()) {
        return false;
    }

    return sourceMTime == static_cast<std::int64_t>(file.lastModified().getTime_t());
}

bool StepLightweightManifest::hasAllShardFiles() const
{
    if (!Base::FileInfo(masterDocumentPath).exists()) {
        return false;
    }

    for (const auto& shard : shards) {
        if (!Base::FileInfo(shard.documentPath).exists()) {
            return false;
        }
    }

    return true;
}
