// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <ctime>
#include <stdexcept>
#include <zlib.h>

#include <zipios/fileentry.hpp>
#include <zipcentraldirectoryentry.hpp>
#include <zipoutputstream.hpp>

#include "FileInfo.h"
#include "Stream.h"
#include "ZipOutputStream.h"

using Base::GZipOutputStream;
using Base::ZipOutputStream;

namespace
{

// Collects the data for a Zipios stream. Zipios fails a flush, which std::endl does all the time,
// so a flush only empties this buffer into the Zipios stream.
class FlushableBuffer: public std::streambuf
{
public:
    explicit FlushableBuffer(std::streambuf* target)
        : target(target)
    {
        setp(buffer.data(), buffer.data() + buffer.size());
    }

    /// The number of bytes passed on since the last reset
    std::streamsize passedOn() const
    {
        return passed;
    }

    void resetPassedOn()
    {
        passed = 0;
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (sync() != 0) {
            return traits_type::eof();
        }
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }
        return traits_type::not_eof(ch);
    }

    int sync() override
    {
        const std::streamsize count = pptr() - pbase();
        if (count > 0 && target->sputn(pbase(), count) != count) {
            return -1;
        }
        passed += count;
        setp(buffer.data(), buffer.data() + buffer.size());
        return 0;
    }

private:
    static constexpr std::size_t bufferSize = 65536;
    std::streambuf* target;
    std::streamsize passed = 0;
    std::array<char, bufferSize> buffer {};
};

// A zip entry with just a name. Unlike zipios::DirectoryEntry it doesn't look the name up in the
// file system.
class NamedEntry: public zipios::FileEntry
{
public:
    explicit NamedEntry(const std::string& name)
        : FileEntry(zipios::FilePath(name))
    {}

    pointer_t clone() const override
    {
        return std::make_shared<NamedEntry>(*this);
    }

    bool isDirectory() const override
    {
        return !m_filename.empty() && std::string(m_filename).back() == '/';
    }
};

// Compresses with zlib in the gzip format. zipios::GZIPOutputStream writes no header when the
// data fits in its buffer.
class GZipBuffer: public std::streambuf
{
public:
    explicit GZipBuffer(std::streambuf* target)
        : target(target)
    {
        constexpr int gzipFormat = 16;
        constexpr int memoryLevel = 8;
        if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + gzipFormat, memoryLevel, Z_DEFAULT_STRATEGY)
            != Z_OK) {
            throw std::runtime_error("Unable to initialize zlib");
        }
        setp(input.data(), input.data() + input.size());
    }

    ~GZipBuffer() override
    {
        deflateEnd(&zs);
    }

    GZipBuffer(const GZipBuffer&) = delete;
    GZipBuffer(GZipBuffer&&) = delete;
    GZipBuffer& operator=(const GZipBuffer&) = delete;
    GZipBuffer& operator=(GZipBuffer&&) = delete;

    /// Writes the rest of the data and the end of the gzip stream
    bool finish()
    {
        if (finished) {
            return true;
        }
        finished = true;
        return compress(Z_FINISH);
    }

protected:
    int_type overflow(int_type ch) override
    {
        if (finished || !compress(Z_NO_FLUSH)) {
            return traits_type::eof();
        }
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }
        return traits_type::not_eof(ch);
    }

    int sync() override
    {
        return finished || compress(Z_NO_FLUSH) ? 0 : -1;
    }

private:
    bool compress(int flush)
    {
        zs.next_in = reinterpret_cast<Bytef*>(pbase());
        zs.avail_in = static_cast<uInt>(pptr() - pbase());
        int status = Z_OK;
        do {
            zs.next_out = reinterpret_cast<Bytef*>(output.data());
            zs.avail_out = static_cast<uInt>(output.size());
            status = deflate(&zs, flush);
            if (status == Z_STREAM_ERROR) {
                return false;
            }
            const auto count = static_cast<std::streamsize>(output.size() - zs.avail_out);
            if (count > 0 && target->sputn(output.data(), count) != count) {
                return false;
            }
        } while (zs.avail_out == 0 || (flush == Z_FINISH && status != Z_STREAM_END));
        setp(input.data(), input.data() + input.size());
        return true;
    }

    static constexpr std::size_t bufferSize = 65536;
    std::streambuf* target;
    z_stream zs {};
    bool finished = false;
    std::array<char, bufferSize> input {};
    std::array<char, bufferSize> output {};
};

}  // namespace

ZipOutputStream::ZipOutputStream(std::ostream& archive)
    : std::ostream(nullptr)
    , zip(std::make_unique<zipios::ZipOutputStream>(archive))
    , buffer(std::make_unique<FlushableBuffer>(zip->rdbuf()))
{
    rdbuf(buffer.get());
}

ZipOutputStream::ZipOutputStream(const FileInfo& file)
    : std::ostream(nullptr)
    , file(std::make_unique<Base::ofstream>(file, std::ios::out | std::ios::binary))
    , zip(std::make_unique<zipios::ZipOutputStream>(*this->file))
    , buffer(std::make_unique<FlushableBuffer>(zip->rdbuf()))
{
    rdbuf(buffer.get());
}

ZipOutputStream::~ZipOutputStream()
{
    try {
        close();
    }
    catch (...) {
    }
}

void ZipOutputStream::putNextEntry(const std::string& name)
{
    finishEntry();
    auto entry = std::make_shared<zipios::ZipCentralDirectoryEntry>(NamedEntry(name));
    entry->setUnixTime(std::time(nullptr));
    // Zipios maps the levels 1 to 100 to the zlib levels 1 to 9
    constexpr int maxLevel = 9;
    if (compressionLevel == 0) {
        entry->setMethod(zipios::StorageMethod::STORED);
        entry->setLevel(zipios::FileEntry::COMPRESSION_LEVEL_NONE);
    }
    else {
        entry->setMethod(zipios::StorageMethod::DEFLATED);
        if (compressionLevel < 0) {
            entry->setLevel(zipios::FileEntry::COMPRESSION_LEVEL_DEFAULT);
        }
        else {
            const int level = std::min(compressionLevel, maxLevel);
            entry->setLevel(1 + ((level - 1) * 99 + 7) / 8);  // NOLINT(readability-magic-numbers)
        }
    }
    zip->putNextEntry(entry);
    currentEntry = entry;
}

void ZipOutputStream::closeEntry()
{
    finishEntry();
    zip->closeEntry();
}

void ZipOutputStream::finishEntry()
{
    flush();
    auto data = static_cast<FlushableBuffer*>(buffer.get());
    // Zipios writes no data at all for an empty deflated entry, which unzip, 7-Zip and Zipios
    // itself reject. Stored is fine, and Zipios writes the header again when it closes the entry.
    if (currentEntry && data->passedOn() == 0) {
        currentEntry->setMethod(zipios::StorageMethod::STORED);
        currentEntry->setLevel(zipios::FileEntry::COMPRESSION_LEVEL_NONE);
    }
    currentEntry.reset();
    data->resetPassedOn();
}

void ZipOutputStream::setComment(const std::string& comment)
{
    zip->setComment(comment);
}

void ZipOutputStream::setLevel(int level)
{
    compressionLevel = level;
}

void ZipOutputStream::close()
{
    finishEntry();
    zip->close();
}

GZipOutputStream::GZipOutputStream(std::ostream& target)
    : std::ostream(nullptr)
    , buffer(std::make_unique<GZipBuffer>(target.rdbuf()))
{
    rdbuf(buffer.get());
}

GZipOutputStream::~GZipOutputStream()
{
    close();
}

void GZipOutputStream::close()
{
    if (!static_cast<GZipBuffer*>(buffer.get())->finish()) {
        setstate(std::ios::badbit);
    }
}
