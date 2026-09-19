// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Base/FileInfo.h>
#include <Base/Uuid.h>

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace tests
{

/// RAII wrapper that creates a unique temporary directory on construction
/// and removes it (with all contents) on destruction.
class TempDirectory
{
public:
    explicit TempDirectory(const std::string& prefix = "fctest")
    {
        const auto base = std::filesystem::path(Base::FileInfo::getTempPath());
        const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()
        )
                                   .count();
        const auto candidate = base
            / (prefix + "_" + std::to_string(timestamp) + "_" + Base::Uuid::createUuid());

        std::error_code ec;
        if (!std::filesystem::create_directory(candidate, ec) || ec) {
            throw std::runtime_error(
                "Unable to create temporary test directory: " + candidate.string()
            );
        }
        _path = candidate;
    }

    ~TempDirectory()
    {
        if (!_path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(_path, ec);
        }
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    TempDirectory(TempDirectory&&) = default;
    TempDirectory& operator=(TempDirectory&&) = default;

    const std::filesystem::path& path() const
    {
        return _path;
    }

    std::string string() const
    {
        return Base::FileInfo::pathToString(_path);
    }

private:
    std::filesystem::path _path;
};

}  // namespace tests
