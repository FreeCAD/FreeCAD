// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>

// windows.h defines IGNORE, a value of zipios::FileCollection::MatchPath
#ifdef IGNORE
# undef IGNORE
#endif
#include <zipios/zipfile.hpp>
#include <zipinputstream.hpp>

#include "FileInfo.h"
#include "Stream.h"
#include "ZipReader.h"

using Base::ZipReader;

namespace
{

// The archive from the start of an entry on. It keeps its own position and seeks the archive before
// every read, so that the streams of several entries don't get in each other's way.
class EntryWindow: public std::streambuf
{
public:
    EntryWindow(std::streambuf* archive, std::streamoff start)
        : archive(archive)
        , start(start)
    {}

protected:
    int_type underflow() override
    {
        if (archive->pubseekpos(start + position, std::ios::in) == pos_type(off_type(-1))) {
            return traits_type::eof();
        }
        const std::streamsize count = archive->sgetn(buffer.data(), buffer.size());
        if (count <= 0) {
            return traits_type::eof();
        }
        setg(buffer.data(), buffer.data(), buffer.data() + count);
        position += count;
        return traits_type::to_int_type(buffer[0]);
    }

    pos_type seekoff(off_type offset, std::ios::seekdir dir, std::ios::openmode which) override
    {
        if (dir == std::ios::cur) {
            offset += position - (egptr() - gptr());
        }
        else if (dir != std::ios::beg) {
            return pos_type(off_type(-1));
        }
        return seekpos(pos_type(offset), which);
    }

    pos_type seekpos(pos_type pos, std::ios::openmode /*which*/) override
    {
        if (pos < 0) {
            return pos_type(off_type(-1));
        }
        position = pos;
        setg(nullptr, nullptr, nullptr);
        return pos;
    }

private:
    static constexpr std::size_t bufferSize = 65536;
    std::streambuf* archive;
    std::streamoff start;
    std::streamoff position = 0;
    std::array<char, bufferSize> buffer {};
};

// The decompressed data of one entry
class EntryStream: public std::istream
{
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): false positive for windowStream
    EntryStream(std::streambuf* archive, std::streamoff start)
        : std::istream(nullptr)
        , window(archive, start)
        , windowStream(&window)
        , entry(windowStream)
    {
        rdbuf(entry.rdbuf());
    }

private:
    EntryWindow window;
    std::istream windowStream;
    zipios::ZipInputStream entry;
};

}  // namespace

ZipReader::ZipReader(std::istream& archive)
    : archive(archive)
    , directory(std::make_unique<zipios::ZipFile>(archive))
{}

ZipReader::ZipReader(std::unique_ptr<std::istream> archive)
    : ownedArchive(std::move(archive))
    , archive(*ownedArchive)
    , directory(std::make_unique<zipios::ZipFile>(*ownedArchive))
{}

ZipReader::ZipReader(const FileInfo& file)
    : ZipReader(std::make_unique<Base::ifstream>(file, std::ios::in | std::ios::binary))
{}

ZipReader::~ZipReader() = default;

std::vector<std::string> ZipReader::entryNames() const
{
    std::vector<std::string> names;
    for (const auto& entry : directory->entries()) {
        names.push_back(entry->getName());
    }
    return names;
}

bool ZipReader::hasEntry(const std::string& name) const
{
    return directory->getEntry(name) != nullptr;
}

std::size_t ZipReader::entrySize(const std::string& name) const
{
    const auto entry = directory->getEntry(name);
    return entry ? entry->getSize() : 0;
}

std::unique_ptr<std::istream> ZipReader::getInputStream(const std::string& name) const
{
    const auto entry = directory->getEntry(name);
    if (!entry) {
        return nullptr;
    }
    return std::make_unique<EntryStream>(archive.rdbuf(), entry->getEntryOffset());
}
