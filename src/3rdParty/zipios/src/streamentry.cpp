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
 * \brief Implementation of zipios::StreamEntry.
 *
 * The declaration of a simple zipios::FileEntry used when reading
 * from a istream.
 */

#include "zipios/streamentry.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"

#include <fstream>
#include <zlib.h>


namespace zipios
{

/** \class StreamEntry
 * \brief A file entry reading from a stream.
 *
 * StreamEntry is a FileEntry that is used when directly reading from a
 * stream instead of a file found on disk.
 */


/** \brief Initialize a StreamEntry object.
 *
 * This constructor initializes a StreamEntry which represents a
 * file in an std::istream. If the input stream is considered valid
 * (i.e. no error flag set), then the StreamEntry is marked as valid.
 *
 * The \p filename parameter is important since the file is to be saved
 * in a zip file and that feat requires a filename.
 *
 * \warning
 * The input \p is stream is saved as a reference in this object. The
 * stream must remain valid for the lifetime of this StreamEntry object.
 *
 * \param[in] is  The input stream (std::istream) to read from.
 * \param[in] filename  The filename of the entry.
 * \param[in] comment  A comment for the entry.
 */
StreamEntry::StreamEntry(
          std::istream & is
        , FilePath const & filename
        , std::string const & comment)
    : FileEntry(filename, comment)
    , f_istream(is)
{
    m_valid = static_cast<bool>(is);
    if(m_valid)
    {
        std::istream::pos_type const current(is.tellg());
        m_uncompressed_size = is.seekg(0, std::ios::end).tellg();
        is.seekg(current, std::ios::beg);

        m_unix_time = time(nullptr);
    }
}


/** \brief Create a copy of the StreamEntry.
 *
 * The clone function creates a copy of this StreamEntry object.
 *
 * In most cases, when a collection is copied, a clone of each
 * entry is created to avoid potential problems with sharing
 * the same object between various lists.
 *
 * \return A shared pointer of the new StreamEntry object.
 */
FileEntry::pointer_t StreamEntry::clone() const
{
    return std::make_shared<StreamEntry>(*this);
}


/** \brief Clean up a StreamEntry object.
 *
 * The destructor is defined as it has to be virtual.
 *
 * It will eventually clean up resources used by the StreamEntry class.
 */
StreamEntry::~StreamEntry()
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
bool StreamEntry::isEqual(FileEntry const & file_entry) const
{
    StreamEntry const * const se(dynamic_cast<StreamEntry const * const>(&file_entry));
    if(se == nullptr)
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
uint32_t StreamEntry::computeCRC32() const
{
    uint32_t result(crc32(0L, Z_NULL, 0));

    if(f_istream)
    {
        f_istream.seekg(0, std::ios::beg);
        for(;;)
        {
            Bytef buf[64 * 1024];
            f_istream.read(reinterpret_cast<char *>(buf), sizeof(buf));
            if(f_istream.gcount() == 0)
            {
                break;
            }
            result = crc32(result, buf, f_istream.gcount());
        }
    }

    return result;
}


/** \brief Retrieve a reference to the istream object.
 *
 * This function returns a reference to the internal istream object saved
 * when you constructor this object.
 *
 * \return A reference to the istream.
 */
std::istream & StreamEntry::getStream() const
{
    return f_istream;
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
