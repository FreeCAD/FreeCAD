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
 * \brief Implementation of the zipios::ZipOutputStream class.
 *
 * The zipios::ZipOutputStream class is used as a filter to compress
 * data being written to a Zip archive.
 */

#include "zipoutputstream.hpp"
#include "zipcentraldirectoryentry.hpp"

#include <fstream>


namespace zipios
{

/** \class ZipOutputStream
 * \brief A ZipOutputStream to allow for data to be compressed with zlib.
 *
 * ZipOutputStream is an internal ostream implementation used to save a
 * collection of files to a Zip archive file.
 */



/** \brief Initialize a ZipOutputStream object.
 *
 * The ZipOutputStream constructor create an output stream that will
 * be used to save Zip data to a file.
 *
 * \param[in] os  The output stream to use to write the Zip archive.
 */
ZipOutputStream::ZipOutputStream(std::ostream & os)
    : m_ozf(std::make_unique<ZipOutputStreambuf>(os.rdbuf()))
{
    init(m_ozf.get());
}


/** \brief Clean up a ZipOutputStream object.
 *
 * The destructor makes sure that all resources allocated by the
 * ZipOutputStream object.
 */
ZipOutputStream::~ZipOutputStream()
{
}


/** Closes the current entry updates its header with the relevant
  size information and positions the stream write pointer for the
  next entry header. Puts the stream in EOF state. Call
  putNextEntry() to clear the EOF stream state flag. */
void ZipOutputStream::closeEntry()
{
    m_ozf->closeEntry();
}


/** \brief Close the current stream.
 *
 * This function calls close() on the internal stream. After this
 * call any attempt in writing to the file will fail.
 *
 * The result is to ensure that the Zip archive file is complete
 * and all buffers flushed to file.
 */
void ZipOutputStream::close()
{
    m_ozf->close();
}


/** \brief Finish up the output by flushing anything left.
 *
 * This function closes the current entry (if one is open) by writing
 * the Zip Central Directory Structure closing the ZipOutputStream.
 * The output stream that the zip archive is being written to is
 * not closed.
 */
void ZipOutputStream::finish()
{
    m_ozf->finish();
}


/** \brief Add an entry to the output stream.
 *
 * This function saves the header of the entry and returns. The caller
 * is expected to save the actual data of the file.
 *
 * \code
 *      os.putNextEntry(entry);
 *      FileCollection::stream_pointer_t is(collection->getInputEntry(entry->getName()));
 *      os << is->rdbuf();
 * \endcode
 *
 * \warning
 * The internal class keeps a copy of the shared pointer so changing
 * the entry from the outside may affect the results and invalidate
 * the resulting Zip archive file. Since this class is now internal,
 * it should not be a major problem since it is created and destroyed
 * before the user would have a chance from doing anything to the file.
 *
 * \param[in] entry  The FileEntry to add to the output stream.
 */
void ZipOutputStream::putNextEntry(FileEntry::pointer_t entry)
{
    // if we do not yet have a ZipCentralDirectoryEntry object, create
    // one from the input entry (the input entry is actually expected
    // to be a DirectoryEntry!)
    ZipCentralDirectoryEntry * central_directory_entry(dynamic_cast<ZipCentralDirectoryEntry *>(entry.get()));
    if(central_directory_entry == nullptr)
    {
        entry = std::make_shared<ZipCentralDirectoryEntry>(*entry);
    }

    m_ozf->putNextEntry(entry);
}


/** \brief Set the global comment.
 *
 * This function is used to setup the Global Comment of the Zip archive
 * file.
 *
 * This comment is saved at the very end of the file, attached to the
 * EndOfCentralDirectory block.
 *
 * \param[in] comment  The global comment to save in the Zip archive.
 */
void ZipOutputStream::setComment(std::string const & comment)
{
    m_ozf->setComment(comment);
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
