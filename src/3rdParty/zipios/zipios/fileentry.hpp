#pragma once
#ifndef ZIPIOS_FILEENTRY_HPP
#define ZIPIOS_FILEENTRY_HPP

/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/** \file
 * \brief Define the zipios::FileEntry class.
 *
 * The zipios::FileEntry class is an interface defining a virtual
 * representation to a file in a Zip archive or on disk.
 *
 * File entries are found in various file collections.
 *
 * \sa zipios::FileCollection
 */

#include "zipios/filepath.hpp"
#include "zipios/dosdatetime.hpp"

#include <memory>
#include <vector>


namespace zipios
{


enum class StorageMethod : uint8_t
{
    STORED      =  0,
    SHRUNK      =  1,
    REDUCED1    =  2,
    REDUCED2    =  3,
    REDUCED3    =  4,
    REDUCED4    =  5,
    IMPLODED    =  6,
    TOKENIZED   =  7,
    DEFLATED    =  8,
    DEFLATED64  =  9,
    OLD_TERSE   = 10,
    RESERVED11  = 11,
    BZIP2       = 12,
    REVERVED13  = 13,
    LZMA        = 14,
    RESERVED15  = 15,
    RESERVED16  = 16,
    RESERVED17  = 17,
    NEW_TERSE   = 18,
    LZ77        = 19,
    WAVPACK     = 97,
    PPMD_I_1    = 98
};


class FileEntry
{
public:
    typedef std::shared_ptr<FileEntry>      pointer_t;
    typedef std::vector<pointer_t>          vector_t;
    typedef std::vector<unsigned char>      buffer_t;
    typedef uint32_t                        crc32_t;

    // we use our own compression level type, it gets converted as
    // required when the level is to be used by a compression scheme
    typedef int                     CompressionLevel;

    static CompressionLevel const   COMPRESSION_LEVEL_DEFAULT   =  -3;
    static CompressionLevel const   COMPRESSION_LEVEL_SMALLEST  =  -2;
    static CompressionLevel const   COMPRESSION_LEVEL_FASTEST   =  -1;
    static CompressionLevel const   COMPRESSION_LEVEL_NONE      =   0;
    static CompressionLevel const   COMPRESSION_LEVEL_MINIMUM   =   1;
    static CompressionLevel const   COMPRESSION_LEVEL_MAXIMUM   = 100;

                                FileEntry(FilePath const & filename, std::string const & comment = std::string());
    virtual pointer_t           clone() const = 0;
    virtual                     ~FileEntry();

    virtual std::string         getComment() const;
    virtual std::size_t         getCompressedSize() const;
    virtual crc32_t             getCrc() const;
    std::streampos              getEntryOffset() const;
    virtual buffer_t            getExtra() const;
    virtual std::size_t         getHeaderSize() const;
    virtual CompressionLevel    getLevel() const;
    virtual StorageMethod       getMethod() const;
    virtual std::string         getName() const;
    virtual std::string         getFileName() const;
    virtual std::size_t         getSize() const;
    virtual DOSDateTime::dosdatetime_t
                                getTime() const;
    virtual std::time_t         getUnixTime() const;
    bool                        hasCrc() const;
    virtual bool                isDirectory() const;
    virtual bool                isEqual(FileEntry const & file_entry) const;
    virtual bool                isValid() const;
    virtual void                setComment(std::string const & comment);
    virtual void                setCompressedSize(size_t size);
    virtual void                setCrc(crc32_t crc);
    void                        setEntryOffset(std::streampos offset);
    virtual void                setExtra(buffer_t const & extra);
    virtual void                setLevel(CompressionLevel level);
    virtual void                setMethod(StorageMethod method);
    virtual void                setSize(size_t size);
    virtual void                setTime(DOSDateTime::dosdatetime_t time);
    virtual void                setUnixTime(std::time_t time);
    virtual std::string         toString() const;

    virtual void                read(std::istream & is);
    virtual void                write(std::ostream & os);

protected:
    FilePath                    m_filename;
    std::string                 m_comment;
    std::size_t                 m_uncompressed_size = 0;
    time_t                      m_unix_time = 0;
    std::streampos              m_entry_offset = 0;
    StorageMethod               m_compress_method = StorageMethod::STORED;
    CompressionLevel            m_compression_level = COMPRESSION_LEVEL_DEFAULT;
    uint32_t                    m_crc_32 = 0;
    buffer_t                    m_extra_field;
    bool                        m_has_crc_32 = false;
    bool                        m_valid = false;
};


std::ostream & operator << (std::ostream & os, FileEntry const & entry);


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
#endif
