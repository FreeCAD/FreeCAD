// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <istream>
#include <memory>
#include <string>
#include <vector>

#include <FCGlobal.h>

namespace zipios
{
class ZipFile;
}

namespace Base
{

class FileInfo;

/// Reads the entries of a zip archive by name. The archive stream has to stay open while the reader
/// and the streams it returns are used.
class BaseExport ZipReader
{
public:
    /// Throws a zipios::Exception if the stream doesn't hold a zip archive that can be read
    explicit ZipReader(std::istream& archive);
    explicit ZipReader(std::unique_ptr<std::istream> archive);
    /// Opens the file, also when its path isn't ASCII on Windows
    explicit ZipReader(const FileInfo& file);
    ~ZipReader();

    ZipReader(const ZipReader&) = delete;
    ZipReader(ZipReader&&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;
    ZipReader& operator=(ZipReader&&) = delete;

    /// The names of the entries, in the order of the archive
    std::vector<std::string> entryNames() const;
    bool hasEntry(const std::string& name) const;
    /// The uncompressed size, 0 if there is no such entry
    std::size_t entrySize(const std::string& name) const;
    /// Returns nullptr if there is no such entry. The streams of several entries can be read at the
    /// same time.
    std::unique_ptr<std::istream> getInputStream(const std::string& name) const;

private:
    std::unique_ptr<std::istream> ownedArchive;
    std::istream& archive;
    std::unique_ptr<zipios::ZipFile> directory;
};

}  // namespace Base
