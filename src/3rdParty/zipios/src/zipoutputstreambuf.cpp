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
 * \brief Implementation of the zipios::ZipOutputStreambuf class.
 *
 * This file includes the functions necessary to write data to a Zip
 * archive.
 */

#include "zipoutputstreambuf.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "ziplocalentry.hpp"
#include "zipendofcentraldirectory.hpp"


namespace zipios
{


namespace
{


/** \brief Helper function used to write the central directory.
 *
 * When you create a Zip archive, it includes a central directory where
 * all the meta data about each file is saved. This function saves an
 * array of entries in an output stream to generate the Zip file
 * central directory.
 *
 * \param[in] os  The output stream.
 * \param[in] entries  The array of entries to save in this central directory.
 * \param[in] comment  The zip archive global comment.
 */
void writeZipCentralDirectory(
      std::ostream & os
    , FileEntry::vector_t & entries
    , std::string const & comment)
{
    ZipEndOfCentralDirectory eocd(comment);
    eocd.setOffset(os.tellp());  // start position
    eocd.setCount(entries.size());

    std::size_t central_directory_size(0);
    for(auto it = entries.begin(); it != entries.end(); ++it)
    {
        (*it)->write(os);
        central_directory_size += (*it)->getHeaderSize();
    }

    eocd.setCentralDirectorySize(central_directory_size);
    eocd.write(os);
}


} // no name namespace


/** \class ZipOutputStreambuf
 * \brief Handle the output buffer of a zip archive.
 *
 * The ZipOutputStreambuf class is a zip archive output
 * streambuf filter.
 */


/** \brief Initialize a ZipOutputStreambuf object.
 *
 * Note that a new initialized ZipOutputStreambuf is not ready to
 * accept data, putNextEntry() must be invoked at least once first.
 *
 * \param[in] outbuf  The streambuf to use for output.
 */
ZipOutputStreambuf::ZipOutputStreambuf(std::streambuf * outbuf)
    : DeflateOutputStreambuf(outbuf)
{
}


/** \brief Clean up the buffer.
 *
 * This function cleans up this output buffer. In general this ensures
 * that the data still cached gets flushed.
 *
 * \warning
 * This function may gobble up some important exceptions. If you want
 * to make sure that the file is properly written, you must call the
 * finish() function (or the close() function) to fully terminate the
 * file. If these functions do not fail, then the output file is
 * considered valid and you can keep it. The finish() function can fail
 * because of a comment or a file which are too large, for example.
 */
ZipOutputStreambuf::~ZipOutputStreambuf()
{
    // avoid possible exceptions when writing the central directory
    try
    {
        finish();
    }
    catch(...)
    {
    }
}


/** \brief Close this buffer entry.
 *
 * Closes the current output buffer entry and positions the stream
 * write pointer at the beginning of the next entry.
 */
void ZipOutputStreambuf::closeEntry()
{
    if(!m_open_entry)
    {
        return;
    }

    switch(m_compression_level)
    {
    case FileEntry::COMPRESSION_LEVEL_NONE:
        overflow(); // flush
        break;

    default:
        closeStream();
        break;

    }

    updateEntryHeaderInfo();
    setEntryClosedState();
}


/** \brief Close the output stream buffer.
 *
 * This function calls finish to make sure that any cached
 * data is saved and then close the stream buffer.
 */
void ZipOutputStreambuf::close()
{
    finish();
}


/** \brief Finish up an output stream buffer.
 *
 * Closes the current entry (if one is open), then writes the Zip
 * Central Directory Structure closing the ZipOutputStream. The
 * output stream (std::ostream) that the zip archive is being
 * written to is not closed.
 */
void ZipOutputStreambuf::finish()
{
    if(!m_open)
    {
        return;
    }
    m_open = false;

    std::ostream os(m_outbuf);
    closeEntry();
    writeZipCentralDirectory(os, m_entries, m_zip_comment);
}


/** \brief Start saving an entry in the output buffer.
 *
 * Opens the next entry in the zip archive and returns a const pointer to a
 * FileEntry object for the entry.
 *
 * If a previous entry was still open, the function calls closeEntry()
 * first.
 *
 * \param[in] entry  The entry to be saved and made current.
 */
void ZipOutputStreambuf::putNextEntry(FileEntry::pointer_t entry)
{
    closeEntry();

    // if the method is STORED force uncompressed data
    if(entry->getMethod() == StorageMethod::STORED)
    {
        // force to "no compression" when the method is STORED
        m_compression_level = FileEntry::COMPRESSION_LEVEL_NONE;
    }
    else
    {
        // get the user defined compression level
        m_compression_level = entry->getLevel();
    }
    m_overflown_bytes = 0;
    switch(m_compression_level)
    {
    case FileEntry::COMPRESSION_LEVEL_NONE:
        setp(&m_invec[0], &m_invec[0] + getBufferSize());
        break;

    default:
        init(m_compression_level);
        break;

    }

    m_entries.push_back(entry);

    std::ostream os(m_outbuf);

    // Update entry header info
    entry->setEntryOffset(os.tellp());
    /** \TODO
     * Rethink the design as we have to force a call to the correct
     * write() function?
     */
    static_cast<ZipLocalEntry *>(entry.get())->ZipLocalEntry::write(os);

    m_open_entry = true;
}


/** \brief Set the archive comment.
 *
 * This function saves a global comment for the Zip archive.
 *
 * You may set it to an empty string which means that no comment
 * will be saved.
 *
 * The comment is saved when the first entry is saved so it
 * has to be put in there early on.
 *
 * \param[in] comment  The comment to save in the Zip archive.
 */
void ZipOutputStreambuf::setComment(std::string const & comment)
{
    m_zip_comment = comment;
}


//
// Protected and private methods
//

/** \brief Implementation of the overflow() function.
 *
 * When writing to a buffer, the overflow() function gets called when
 * there is no more room in the output buffer. The buffer is expected
 * to flush the data to disk and reset the buffer availability.
 *
 * \exception IOException
 * This function generates an exception if saving the data to the output
 * fails.
 *
 * \param[in] c  The character that made it all happen. Maybe EOF.
 *
 * \return EOF if the function fails, 0 otherwise.
 */
int ZipOutputStreambuf::overflow(int c)
{
    std::size_t const size(pptr() - pbase());
    m_overflown_bytes += size;
    switch(m_compression_level)
    {
    case FileEntry::COMPRESSION_LEVEL_NONE:
    {
        // Ok, we are STORED, so we handle it ourselves to avoid "side
        // effects" from zlib, which adds markers every now and then.
        m_crc32 = crc32(m_crc32, reinterpret_cast<Bytef const *>(&m_invec[0]), size); // update crc32
        size_t const bc(m_outbuf->sputn(&m_invec[0], size));
        if(size != bc)
        {
            // Without implementing our own stream in our test, this
            // cannot really be reached because it is all happening
            // inside the same loop in ZipFile::saveCollectionToArchive()
            throw IOException("ZipOutputStreambuf::overflow(): write to buffer failed."); // LCOV_EXCL_LINE
        }
        setp(&m_invec[0], &m_invec[0] + getBufferSize());

        if(c != EOF)
        {
            *pptr() = c;
            pbump(1);
        }

        return 0;
    }

    default:
        return DeflateOutputStreambuf::overflow(c);

    }
}



/** \brief Implement the sync() functionality.
 *
 * This virtual function is reimplemented to make sure that the system
 * does not run a default sync() function.
 *
 * This function calls the DeflateOutputStreambuf::sync() function which
 * returns -1 because it will not "synchronize" the input buffer.
 */
int ZipOutputStreambuf::sync() // LCOV_EXCL_LINE
{
    return DeflateOutputStreambuf::sync(); // LCOV_EXCL_LINE
}



/** \brief Mark the current entry as closed.
 *
 * After the putNextEntry() call and saving of the file content, the
 * closeEntry() function can be called to close the entry. The entry
 * is really closed when this setEntryClosedState() is called.
 */
void ZipOutputStreambuf::setEntryClosedState()
{
    m_open_entry = false;
    m_crc32 = crc32(0, nullptr, 0);

    /** \FIXME
     * Update put pointers to trigger overflow on write. Overflow
     * should then return EOF while m_open_entry is false.
     */
}


/** \brief Save the header information.
 *
 * This function saves parameters that are now available in the header
 * of the local entry.
 *
 * These parameters include:
 *
 * \li The uncompressed size of the entry
 * \li The compressed size of the entry
 * \li The CRC32 of the input file (before the compression)
 */
void ZipOutputStreambuf::updateEntryHeaderInfo()
{
    if(!m_open_entry)
    {
        return;
    }

    std::ostream os(m_outbuf);
    int const curr_pos(os.tellp());

    // update fields in m_entries.back()
    FileEntry::pointer_t entry(m_entries.back());
    entry->setSize(getSize());
    entry->setCrc(getCrc32());
    /** \TODO
     * Rethink the design as we have to force a call to the correct
     * getHeaderSize() function?
     */
    entry->setCompressedSize(curr_pos - entry->getEntryOffset() - static_cast<ZipLocalEntry *>(entry.get())->ZipLocalEntry::getHeaderSize());

    // write ZipLocalEntry header to header position
    os.seekp(entry->getEntryOffset());
    /** \TODO
     * Rethink the design as we have to force a call to the correct write()
     * function?
     */
    static_cast<ZipLocalEntry *>(entry.get())->ZipLocalEntry::write(os);
    os.seekp(curr_pos);
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
