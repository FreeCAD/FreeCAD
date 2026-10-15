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
 * \brief Declare zipios::ZipEndOfCentralDirectory which handles entries found
 *        in a Zip archive directory.
 *
 * This header file contains the zipios::ZipLocalEntry, which is used
 * to handle entries found in a Zip archive.
 */

#include "zipendofcentraldirectory.hpp"

#include "zipios/zipiosexceptions.hpp"


namespace zipios
{


/** \class ZipEndOfCentralDirectory
 * \brief Marker at the end of a Zip archive file.
 *
 * This class is used to read and write the end of the Central Directory
 * structure. In most cases, this structure is stored at the end of the
 * zip archive file, and contains some global information about the file,
 * including the position of the start of the Central Directory.
 */


/** \brief Private definitions of the ZipEndOfCentralDirectory class.
 *
 * This name space includes definitions exclusively used by the
 * ZipEndOfCentralDirectory class.
 */
namespace
{


/** \brief Signature of the ZipEndOfCentralDirectory structure block.
 *
 * This variable is used to define the signature of the
 * ZipEndOfCentralDirectory structure on disk. It is used to
 * create such a block or to detect that such a block exists.
 *
 * The four byte signature represents the following value:
 *
 * "PK 5.6" -- End of Central Directory
 */
uint32_t const g_signature = 0x06054b50;


} // no name namespace


/** \brief Initialize an ZipEndOfCentralDirectory object.
 *
 * This function initializes an ZipEndOfCentralDirectory object. By default,
 * all the numbers are set to zero and the global Zip file comment is
 * set to the empty string.
 *
 * \param[in] zip_comment  The global comment of a Zip archive.
 */
ZipEndOfCentralDirectory::ZipEndOfCentralDirectory(std::string const & zip_comment)
    : m_zip_comment(zip_comment)
{
}


/** \brief Retrieve the size of the Central Directory in bytes.
 *
 * This function returns the size of the Central Directory
 * structure in the file. This size varies because each entry
 * includes data that change in size (i.e. filename, comment,
 * extra data.)
 *
 * \return The size, in bytes, of the Central Directory.
 *
 * \sa setCentralDirectorySize()
 */
size_t ZipEndOfCentralDirectory::getCentralDirectorySize() const
{
    return m_central_directory_size;
}


/** \brief Retrieve the number of entries.
 *
 * This function returns the number of entries that will be found
 * in the Central Directory.
 *
 * Since Zipios has no support for split Zip archive files (i.e. one
 * large archive written on multiple disks), the total number of entries,
 * or the number of entries in this archive is always exactly the same.
 *
 * \return The total number of entries archived in this Zip file.
 *
 * \sa setCount()
 */
size_t ZipEndOfCentralDirectory::getCount() const
{
    return m_central_directory_entries;
}


/** \brief Retrieve the offset of the Central Directory.
 *
 * This function is expected to be called after a call to read().
 * It includes the offset of the central directory, which in most
 * cases appears before the ZipEndOfCentralDirectory block.
 *
 * \warning
 * There is getOffsetFromEnd() which returns the offset of the
 * ZipEndOfCentralDirectory itself and not the Central Directory.
 *
 * \return The offset in the Zip archive of the Central Directory.
 *
 * \sa getOffsetFromEnd()
 * \sa setOffset()
 */
offset_t ZipEndOfCentralDirectory::getOffset() const
{
    return m_central_directory_offset;
}


/** \brief Define the size of the central directory.
 *
 * When creating a Zip archive, it is necessary to call this function
 * to define the size of the Central Directory block. This size
 * cannot be inferred or calculated without wasting a lot of time
 * re-reading the Central Directory, hence the function to avoid
 * doing such.
 *
 * \param[in] size  The size of the Central Directory.
 *
 * \sa getCentralDirectorySize()
 */
void ZipEndOfCentralDirectory::setCentralDirectorySize(size_t size)
{
    m_central_directory_size = size;
}


/** \brief Set the number of entries.
 *
 * This function is used to define the number of entries one will find
 * in the central directory.
 *
 * \note
 * The maximum number of entries is 65535. (until we add support for
 * 64 bit Zip archives.)
 *
 * \param[in] count  The number of entries in the Central Directory.
 *
 * \sa getCount()
 */
void ZipEndOfCentralDirectory::setCount(size_t count)
{
    m_central_directory_entries = count;
}


/** \brief Offset of the Central Directory.
 *
 * This function defines the offset at which the Central Directory
 * starts. Before writing the Central Directory, we expect the user
 * to call tell() and save the value using this function. This is
 * important when creating a Zip archive.
 *
 * \note
 * Only the offset of the Central Directory can be changed by
 * this function.
 *
 * \param[in] start_offset  The start offset of the Central Directory.
 *
 * \sa getOffset();
 */
void ZipEndOfCentralDirectory::setOffset(offset_t start_offset)
{
    m_central_directory_offset = start_offset;
}


/** \brief Attempt to read an ZipEndOfCentralDirectory structure.
 *
 * This function tries to read an ZipEndOfCentralDirectory structure from the
 * specified buffer. This function expects a BackBuffer, which is used
 * because that is generally the fastest way to read the data (instead of
 * scanning the entire file).
 *
 * \note
 * If a read from the buffer fails, then an exception is raised. Since
 * we are reading from a buffer, it should not happen except if the
 * ZipEndOfCentralDirectory indicates that there is a comment and the comment
 * is not there or some characters are missing.
 *
 * \exception FileCollectionException
 * This exception is raised if the number of entries is not equal to
 * the total number of entries, as expected.
 *
 * \param[in] buf  The buffer with the file data.
 * \param[in] pos  The position at which we are expected to check.
 *
 * \return true if the ZipEndOfCentralDirectory was found, false otherwise.
 */
bool ZipEndOfCentralDirectory::read(::zipios::buffer_t const & buf, size_t pos)
{
    // the number of bytes we are going to read in the buffer
    // (including the signature)
    ssize_t const HEADER_SIZE(static_cast<ssize_t>(sizeof(uint32_t) * 3 + sizeof(uint16_t) * 5));

    // enough data in the buffer?
    //
    // Note: this quick check assumes a 0 length comment which is possible;
    //       if there is a comment and we find the signature too early, then
    //       it will throw
    //
    if(static_cast<ssize_t>(buf.size() - pos) < HEADER_SIZE)
    {
        return false;
    }

    // first read and check the signature
    uint32_t signature;
    zipRead(buf, pos, signature);               // 32
    if(signature != g_signature)
    {
        return false;
    }

    // next we read the other parameters
    uint16_t disk_number;
    uint16_t central_directory_entries;
    uint16_t central_directory_total_entries;
    uint32_t central_directory_size;
    uint32_t central_directory_offset;
    uint16_t comment_len;

    zipRead(buf, pos, disk_number);                         // 16
    zipRead(buf, pos, disk_number);                         // 16
    zipRead(buf, pos, central_directory_entries);           // 16
    zipRead(buf, pos, central_directory_total_entries);     // 16
    zipRead(buf, pos, central_directory_size);              // 32
    zipRead(buf, pos, central_directory_offset);            // 32
    zipRead(buf, pos, comment_len);                         // 16
    zipRead(buf, pos, m_zip_comment, comment_len);          // string

    // note that if disk_number is defined, then these following two
    // numbers should differ too
    if(central_directory_entries != central_directory_total_entries)
    {
        throw FileCollectionException("ZipEndOfCentralDirectory with a number of entries and total entries that differ is not supported, spanned zip files are not supported");
    }

    m_central_directory_entries = central_directory_entries;
    m_central_directory_size    = central_directory_size;
    m_central_directory_offset  = central_directory_offset;

    return true;
}


/** \brief Write the ZipEndOfCentralDirectory structure to a stream.
 *
 * This function writes the currently defined end of central
 * directory to disk. This entry is expected to be written at
 * the very end of a Zip archive file.
 *
 * \note
 * If the output pointer is not valid, the function will throw
 * via the various zipWrite() it uses.
 *
 * \note
 * The function does not change the output pointer of the stream
 * before writing to it.
 *
 * \exception FileCollectionException
 * This function throws this exception if the data cannot be saved. In
 * general this means there are too many entries, the size is too large
 * or the comment is more than 64Kb (some of which will be resolved with
 * Zip64 support.)
 *
 * \param[in] os  The output stream where the data is to be saved.
 */
void ZipEndOfCentralDirectory::write(std::ostream & os)
{
    /** \todo
     * Add support for 64 bit Zip archive. This would allow for pretty
     * much all the following conditions to be dropped out.
     */
    if(m_zip_comment.length() > 65535)
    {
        throw InvalidStateException("the Zip archive comment is too large");
    }
    if(m_central_directory_entries > 65535)
    {
        throw InvalidStateException("the number of entries in the Zip archive is too large");
    }
// Solaris defines _ILP32 for 32 bit platforms
#if INTPTR_MAX != INT32_MAX
    if(m_central_directory_size   >= 0x100000000UL
    || m_central_directory_offset >= 0x100000000L)
    {
        throw FileCollectionException("the Zip archive size or offset are too large"); // LCOV_EXCL_LINE
    }
#endif

    uint16_t const disk_number(0);
    uint16_t const central_directory_entries(m_central_directory_entries);
    uint32_t const central_directory_size(m_central_directory_size);
    uint32_t const central_directory_offset(m_central_directory_offset);
    uint16_t const comment_len(m_zip_comment.length());

    // the total number of entries, across all disks is the same in our
    // case so we use one number for both fields

    zipWrite(os, g_signature);                      // 32
    zipWrite(os, disk_number);                      // 16
    zipWrite(os, disk_number);                      // 16
    zipWrite(os, central_directory_entries);        // 16
    zipWrite(os, central_directory_entries);        // 16
    zipWrite(os, central_directory_size);           // 32
    zipWrite(os, central_directory_offset);         // 32
    zipWrite(os, comment_len);                      // 16
    zipWrite(os, m_zip_comment);                    // string
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
