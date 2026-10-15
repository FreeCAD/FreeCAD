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
 * \brief Implementation of the zipios::ZipLocalEntry class.
 *
 * This file is the implementation of the zipios::ZipLocalEntry class
 * which handles zipios::FileEntry's found in Zip archives.
 */

#include "ziplocalentry.hpp"

#include "zipios/zipiosexceptions.hpp"
#include "zipios/dosdatetime.hpp"

#include "zipios_common.hpp"


namespace zipios
{


/** \brief Various definitions for local blocks.
 *
 * The ZipLocalEntry needs a signature, a flag, and makes use
 * of a structure declaration (although we cannot really use
 * that structure.)
 */
namespace
{

/** \brief The signature of a local entry.
 *
 * This value represents the signature of a Zip archive block defining
 * a ZipLocalEntry.
 *
 * \code
 * "PK 3.4"
 * \endcode
 */
uint32_t const      g_signature = 0x04034b50;


/** \brief A bit in the general purpose flags.
 *
 * This mask is used to know whether the size and CRC are saved in
 * the header or after the header. At this time Zipios does not
 * support such trailing data as it makes use of the compressed
 * and uncompressed sizes to properly stream the output data.
 *
 * This is bit 3. (see point 4.4.4 in doc/zip-format.txt)
 */
uint16_t const      g_trailing_data_descriptor = 1 << 3;


/** \brief ZipLocalEntry Header
 *
 * This structure shows how the header of the ZipLocalEntry is defined.
 * Note that the file name and extra field have a variable size which
 * is defined in two 16 bit values just before they appear.
 *
 * The filename cannot be empty, however, the extra field can (and
 * usually is).
 *
 * \note
 * This structure is NOT used directly only for its sizeof() and
 * documentation because that way zipios can work on little and big
 * endians without the need to know the endianness of your computer.
 */
struct ZipLocalEntryHeader
{
    uint32_t            m_signature;
    uint16_t            m_extract_version;
    uint16_t            m_general_purpose_bitfield;
    uint16_t            m_compress_method;
    uint32_t            m_dosdatetime;
    uint32_t            m_crc_32;
    uint32_t            m_compressed_size;
    uint32_t            m_uncompressed_size;
    uint16_t            m_filename_len;
    uint16_t            m_extra_field_len;
    //uint8_t             m_filename[m_filename_len];
    //uint8_t             m_extra_field[m_extra_field_len];
};


} // no name namespace



/** \class ZipLocalEntry
 * \brief An implementation of the FileEntry for Zip archives.
 *
 * A concrete implementation of the abstract FileEntry base class for
 * ZipFile entries, specifically for representing the information
 * present in the local headers of file entries in a zip file.
 */



/** \brief Create a default ZipLocalEntry objects.
 *
 * This constructor is used to create a default ZipLocalEntry object.
 */
ZipLocalEntry::ZipLocalEntry()
    : FileEntry(FilePath(""))
{
}


/** \brief Copy of the ZipLocalEntry from any kind of FileEntry object.
 *
 * This function is used when copying a DirectoryEntry to a
 * ZipCentralDirectoryEntry object when creating a copy while
 * saving a Zip archive.
 *
 * \param[in] src  The source to copy in this new ZipLocalEntry.
 */
ZipLocalEntry::ZipLocalEntry(FileEntry const & src)
    : FileEntry(src)
    , m_is_directory(src.isDirectory())
{
}


/** \brief Create a clone of a ZipLocalEntry object.
 *
 * This function allocates a new ZipLocalEntry on the heap and returns
 * a copy of this ZipLocalEntry object in it.
 *
 * \return A new ZipLocalEntry which is a clone of this ZipLocalEntry object.
 */
FileEntry::pointer_t ZipLocalEntry::clone() const // LCOV_EXCL_LINE
{
    // It makes sense to keep the clone() function for this class
    // but since it is internal and never allocated as is (we use
    // the ZipCentralDirectoryEntry instead) it is marked as
    // non-reachable by the coverage tests
    return std::make_shared<ZipLocalEntry>(*this); // LCOV_EXCL_LINE
}


/** \brief Clean up a ZipLocalEntry object.
 *
 * This function ensures proper clean up of a ZipLocationEntry object.
 */
ZipLocalEntry::~ZipLocalEntry()
{
}


/** \brief Check whether the filename represents a directory.
 *
 * This function checks the last character of the filename, if it
 * is a separator ('/') then the function returns true meaning
 * that the file represents a directory.
 *
 * \return true if the entry represents a directory.
 */
bool ZipLocalEntry::isDirectory() const
{
    return m_is_directory;
}


/** \brief Compare two file entries for equality.
 *
 * This function compares most of the fields between two file
 * entries to see whether they are equal or not.
 *
 * \note
 * This function calls the base class isEqual() and also verifies
 * that the ZipLocalEntry fields are equal.
 *
 * \note
 * This function is also used to compare ZipCDirEntry since none
 * of the additional field participate in the comparison.
 *
 * \param[in] file_entry  The file entry to compare this against.
 *
 * \return true if both FileEntry objects are considered equal.
 */
bool ZipLocalEntry::isEqual(FileEntry const & file_entry) const
{
    ZipLocalEntry const * const ze(dynamic_cast<ZipLocalEntry const * const>(&file_entry));
    if(ze == nullptr)
    {
        return false;
    }
    return FileEntry::isEqual(file_entry)
        && m_extract_version          == ze->m_extract_version
        && m_general_purpose_bitfield == ze->m_general_purpose_bitfield
        && m_is_directory             == ze->m_is_directory;
        //&& m_compressed_size          == ze->m_compressed_size -- ignore in comparison
}


/** \brief Retrieve the size of the file when compressed.
 *
 * This function returns the compressed size of the entry. If the
 * entry is not stored in a compressed format, the uncompressed
 * size is returned.
 *
 * \return The compressed size of the entry.
 */
size_t ZipLocalEntry::getCompressedSize() const
{
    return m_compressed_size;
}


/** \brief Retrieve the size of the header.
 *
 * This function dynamically determines the size of the Zip archive
 * header necessary for this FileEntry.
 *
 * This function returns the size of the Zip archive header.
 *
 * \return The size of the header in bytes.
 */
size_t ZipLocalEntry::getHeaderSize() const
{
    // Note that the structure is 32 bytes because of an alignment
    // and attempting to use options to avoid the alignment would
    // not be portable so we use a hard coded value (yuck!)
    return 30 /* sizeof(ZipLocalEntryHeader) */
         + m_filename.length() + (m_is_directory ? 1 : 0)
         + m_extra_field.size();
}


/** \brief Set the size when the file is compressed.
 *
 * This function saves the compressed size of the entry in this object.
 *
 * By default the compressed size is viewed as the same as the
 * uncompressed size (i.e. as if STORED was used for the compression
 * method.)
 *
 * \param[in] size  Value to set the compressed size field of the entry to.
 */
void ZipLocalEntry::setCompressedSize(size_t size)
{
    m_compressed_size = size;
}


/** \brief Save the CRC of the entry.
 *
 * This function saves the CRC field in this FileEntry field.
 *
 * This function has the side of setting the m_has_crc_32 flag
 * to true meaning that the CRC was defined as expected.
 *
 * \param[in] crc  Value to set the CRC field to.
 */
void ZipLocalEntry::setCrc(uint32_t crc)
{
    m_crc_32 = crc;
    m_has_crc_32 = true;
}


/** \brief Is there a trailing data descriptor?
 *
 * This function checks the bit in the General Purpose Flags to know
 * whether the local entry header includes the compressed and uncompressed
 * sizes or whether these are defined after the compressed data.
 *
 * The trailing data buffer looks like this:
 *
 * \code
 *      signature (PK 8.7) -- OPTIONAL  -- 32 bit
 *      CRC 32                          -- 32 bit
 *      compressed size                 -- 32 or 64 bit
 *      uncompressed size               -- 32 or 64 bit
 * \endcode
 *
 * When a trailing data buffer is defined, the header has the compressed
 * and uncompressed sizes set to zero.
 *
 * \note
 * Zipios does not support such a scheme.
 *
 * \return true if this file makes use of a trailing data buffer.
 */
bool ZipLocalEntry::hasTrailingDataDescriptor() const
{
    return (m_general_purpose_bitfield & g_trailing_data_descriptor) != 0;
}


/** \brief Read one local entry from \p is.
 *
 * This function verifies that the input stream starts with a local entry
 * signature. If so, it reads the input stream for a complete local entry.
 *
 * Calling this function first marks the FileEntry object as invalid. If
 * the read succeeds in full, then the entry is again marked as valid.
 *
 * If a read fails, the function throws an exception as defined in
 * the various zipRead() functions.
 *
 * \note
 * Some of the data found in the local entry on disk are not kept in
 * this class because there is nothing we can do with it.
 *
 * \param[in] is  The input stream to read from.
 */
void ZipLocalEntry::read(std::istream & is)
{
    m_valid = false; // set to true upon successful completion.

    //    // Before reading anything we record the position in the stream
    //    // This is a field in the central directory entry, but not
    //    // in the local entry. After all, we know where we are, anyway.
    //    zlh.rel_offset_loc_head  = is.tellg() ;

    uint32_t signature(0);
    zipRead(is, signature);                             // 32
    if(g_signature != signature)
    {
        // put stream in error state and return
        is.setstate(std::ios::failbit);
        throw IOException("ZipLocalEntry::read() expected a signature but got some other data");
    }

    uint16_t compress_method(0);
    uint32_t dosdatetime(0);
    uint32_t compressed_size(0);
    uint32_t uncompressed_size(0);
    uint16_t filename_len(0);
    uint16_t extra_field_len(0);
    std::string filename;

    // See the ZipLocalEntryHeader for more details
    zipRead(is, m_extract_version);                 // 16
    zipRead(is, m_general_purpose_bitfield);        // 16
    zipRead(is, compress_method);                   // 16
    zipRead(is, dosdatetime);                       // 32
    zipRead(is, m_crc_32);                          // 32
    zipRead(is, compressed_size);                   // 32
    zipRead(is, uncompressed_size);                 // 32
    zipRead(is, filename_len);                      // 16
    zipRead(is, extra_field_len);                   // 16
    zipRead(is, filename, filename_len);            // string
    zipRead(is, m_extra_field, extra_field_len);    // buffer
    /** \todo add support for zip64, some of those parameters
     *        may be 0xFFFFF...FFFF in which case the 64 bit
     *        header should be read
     */

    // the FilePath() will remove the trailing slash so make sure
    // to defined the m_is_directory ahead of time!
    m_is_directory = !filename.empty() && filename.back() == g_separator;

    m_compress_method = static_cast<StorageMethod>(compress_method);
    DOSDateTime t;
    t.setDOSDateTime(dosdatetime);
    m_unix_time = t.getUnixTimestamp();
    m_compressed_size = compressed_size;
    m_uncompressed_size = uncompressed_size;
    m_filename = FilePath(filename);

    m_valid = true;
}


/** \brief Write a ZipLocalEntry to \p os.
 *
 * This function writes this ZipLocalEntry header to the specified
 * output stream.
 *
 * \exception IOException
 * If an error occurs while writing to the output stream, the function
 * throws an IOException.
 *
 * \param[in] os  The output stream where the ZipLocalEntry is written.
 */
void ZipLocalEntry::write(std::ostream & os)
{
    if(m_filename.length()  > 0x10000
    || m_extra_field.size() > 0x10000)
    {
        throw InvalidStateException("ZipLocalEntry::write(): file name or extra field too large to save in a Zip file.");
    }

    /** todo: add support for 64 bit zip archive
     */
#if INTPTR_MAX != INT32_MAX
    if(m_compressed_size   >= 0x100000000UL
    || m_uncompressed_size >= 0x100000000UL)
    {
        // these are really big files, we do not currently test such so ignore in coverage
        //
        // Note: The compressed size is known at the end, we seek back to
        //       this header and re-save it with the info; thus the error
        //       is caught then if it was not out of bounds earlier.
        throw InvalidStateException("The size of this file is too large to fit in a 32 bit zip archive."); // LCOV_EXCL_LINE
    }
#endif

    std::string filename(m_filename);
    if(m_is_directory)
    {
        filename += g_separator;
    }

    std::uint16_t compress_method(static_cast<uint8_t>(m_compress_method));
    if(m_compression_level == COMPRESSION_LEVEL_NONE)
    {
        compress_method = static_cast<uint8_t>(StorageMethod::STORED);
    }

    DOSDateTime t;
    t.setUnixTimestamp(m_unix_time);
    std::uint32_t dosdatetime(t.getDOSDateTime());       // type could use DOSDateTime::dosdatetime_t
    std::uint32_t compressed_size(m_compressed_size);
    std::uint32_t uncompressed_size(m_uncompressed_size);
    std::uint16_t filename_len(filename.length());
    std::uint16_t extra_field_len(m_extra_field.size());

    // See the ZipLocalEntryHeader for more details
    zipWrite(os, g_signature);                  // 32
    zipWrite(os, m_extract_version);            // 16
    zipWrite(os, m_general_purpose_bitfield);   // 16
    zipWrite(os, compress_method);              // 16
    zipWrite(os, dosdatetime);                  // 32
    zipWrite(os, m_crc_32);                     // 32
    zipWrite(os, compressed_size);              // 32
    zipWrite(os, uncompressed_size);            // 32
    zipWrite(os, filename_len);                 // 16
    zipWrite(os, extra_field_len);              // 16
    zipWrite(os, filename);                     // string
    zipWrite(os, m_extra_field);                // buffer
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
