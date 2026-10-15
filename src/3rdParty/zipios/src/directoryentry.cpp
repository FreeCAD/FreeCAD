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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 * \brief Implementation of zipios::DirectoryEntry.
 *
 * The declaration of a simple zipios::FileEntry used when reading
 * a directory from disk.
 */

#include "zipios/directoryentry.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"

#include <fstream>
#include <zlib.h>


namespace zipios
{

/** \class DirectoryEntry
 * \brief A file entry that does not use compression.
 *
 * DirectoryEntry is a FileEntry that is suitable as a base class for
 * basic entries, that do not support any form of compression and
 * in most cases represent a file in a directory.
 */


/** \brief Initialize a DirectoryEntry object.
 *
 * This constructor initializes a DirectoryEntry which represents a
 * file on disk. If the basepath + filename does not exist or is
 * neither a regular file or a directory, then this entry is created
 * but marked as invalid.
 *
 * \param[in] filename  The filename of the entry.
 * \param[in] comment  A comment for the entry.
 */
DirectoryEntry::DirectoryEntry(FilePath const & filename, std::string const & comment)
    : FileEntry(filename, comment)
{
    m_valid = m_filename.isRegular() || m_filename.isDirectory();
    if(m_valid)
    {
        m_uncompressed_size = m_filename.isDirectory() ? 0 : m_filename.fileSize();
        m_unix_time = m_filename.lastModificationTime();
    }
}


/** \brief Create a copy of the DirectoryEntry.
 *
 * The clone function creates a copy of this DirectoryEntry object.
 *
 * In most cases, when a collection is copied, a clone of each
 * entry is created to avoid potential problems with sharing
 * the same object between various lists.
 *
 * \return A shared pointer of the new DirectoryEntry object.
 */
DirectoryEntry::pointer_t DirectoryEntry::clone() const
{
    return std::make_shared<DirectoryEntry>(*this);
}


/** \brief Clean up a DirectoryEntry object.
 *
 * The destructor is defined as it has to be virtual.
 *
 * It will eventually clean up resources used by the DirectoryEntry class.
 */
DirectoryEntry::~DirectoryEntry()
{
}


/** \brief Compare two file entries for equality.
 *
 * This function compares most of the fields between two file
 * entries to see whether they are equal or not.
 *
 * \note
 * This function calls the base class isEqual() and also verifies
 * that the object comments are equal.
 *
 * \param[in] file_entry  The file entry to compare this against.
 *
 * \return true if both FileEntry objects are considered equal.
 */
bool DirectoryEntry::isEqual(FileEntry const & file_entry) const
{
    DirectoryEntry const * const de(dynamic_cast<DirectoryEntry const * const>(&file_entry));
    if(de == nullptr)
    {
        return false;
    }
    return FileEntry::isEqual(file_entry);
}


/** \brief Compute the CRC32 of this file.
 *
 * This function computers the CRC32 of this file and returns it.
 *
 * This is only a helper function. The CRC32 for the zip file is computed
 * on the fly as data is being streamed.
 *
 * \warning
 * This function recomputes the CRC32 on each call. It doesn't get cached.
 *
 * \return The CRC32 of this file.
 */
uint32_t DirectoryEntry::computeCRC32() const
{
    uint32_t result(crc32(0L, Z_NULL, 0));

    if(!m_filename.isDirectory())
    {
        // TODO: I tried to use std::basic_ifstream<Bytef> to avoid the
        //       reinterpret_cast<>(), but somehow that doesn't work at all
        //
        std::ifstream in;
        in.open(m_filename);
        if(!in.is_open())
        {
            throw IOException(
                  "Can't open file \""
                + m_filename.filename()
                + "\".");
        }

        for(;;)
        {
            Bytef buf[64 * 1024];
            in.read(reinterpret_cast<char *>(buf), sizeof(buf));
            if(in.gcount() == 0)
            {
                break;
            }
            result = crc32(result, buf, in.gcount());
        }
    }

    return result;
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
