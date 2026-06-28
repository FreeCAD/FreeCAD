// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Base/FileInfo.h>

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
        auto base = std::filesystem::path(Base::FileInfo::getTempPath());
        for (int suffix = 0;; ++suffix) {
            auto candidate = base / (prefix + "_" + std::to_string(suffix));
            std::error_code ec;
            if (std::filesystem::create_directory(candidate, ec)) {
                _path = candidate;
                return;
            }
            if (ec && ec != std::errc::file_exists) {
                throw std::runtime_error(
                    "Unable to create temporary test directory: " + candidate.string()
                );
            }
        }
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
