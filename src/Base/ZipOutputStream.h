// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <ostream>
#include <string>

#include <FCGlobal.h>

namespace zipios
{
class FileEntry;
class ZipOutputStream;
}  // namespace zipios

namespace Base
{

class FileInfo;

/// Writes a zip archive entry by entry. Unlike zipios::ZipOutputStream it can be flushed, and it
/// also opens a file whose path isn't ASCII on Windows.
class BaseExport ZipOutputStream: public std::ostream
{
public:
    explicit ZipOutputStream(std::ostream& archive);
    explicit ZipOutputStream(const FileInfo& file);
    ~ZipOutputStream() override;

    ZipOutputStream(const ZipOutputStream&) = delete;
    ZipOutputStream(ZipOutputStream&&) = delete;
    ZipOutputStream& operator=(const ZipOutputStream&) = delete;
    ZipOutputStream& operator=(ZipOutputStream&&) = delete;

    void putNextEntry(const std::string& name);
    void closeEntry();
    void setComment(const std::string& comment);
    /// The zlib compression level of the entries that follow, from 0 (none) to 9 (smallest), or
    /// -1 for the default
    void setLevel(int level);
    /// Writes the end of the archive
    void close();

private:
    void finishEntry();

    std::unique_ptr<std::ostream> file;
    std::unique_ptr<zipios::ZipOutputStream> zip;
    std::unique_ptr<std::streambuf> buffer;
    std::shared_ptr<zipios::FileEntry> currentEntry;
    int compressionLevel {-1};
};

/// Writes gzip compressed data
class BaseExport GZipOutputStream: public std::ostream
{
public:
    explicit GZipOutputStream(std::ostream& target);
    ~GZipOutputStream() override;

    GZipOutputStream(const GZipOutputStream&) = delete;
    GZipOutputStream(GZipOutputStream&&) = delete;
    GZipOutputStream& operator=(const GZipOutputStream&) = delete;
    GZipOutputStream& operator=(GZipOutputStream&&) = delete;

    /// Writes the end of the compressed data
    void close();

private:
    std::unique_ptr<std::streambuf> buffer;
};

}  // namespace Base
