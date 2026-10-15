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
 * \brief Implementation of zipios::FileEntry.
 *
 * This file includes the implementation of the zipios::FileEntry.
 * Many of the functions in zipios::FileEntry are pure virtual so
 * only their documentation appears here.
 *
 * zipios::FileEntry is used to build collections of files.
 */

#include "zipios/fileentry.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"


namespace zipios
{

/** \enum StorageMethod
 * \brief The types used with FileEntry::setMethod and FileEntry::getMethod.
 *
 * The current entries are the types supported by the zip format. The
 * numbering matches one to one the numbering used in the zip file format,
 * i.e. STORED is indicated by a 0 in the method field in a zip file and
 * so on.
 *
 * The zipios library only supports STORED and DEFLATED at this time.
 */


/** \class FileEntry
 * \brief A FileEntry represents an entry in a FileCollection.
 *
 * The interface is a copy of the ZipEntry interface from the java.util.zip
 * package. The name has been changed to FileEntry, as FileCollection
 * is a more general abstraction, that covers other types of file
 * collections than just zip files.
 *
 * \note
 * The hashCode() supported in Java is not included as we do not have an
 * equivalent in this library.
 */


/** \typedef int FileEntry::CompressionLevel
 * \brief The compression level to be used to save an entry.
 *
 * Values defined using this time represent the compression level to
 * be used when compressing an entry.
 *
 * If unchanged, use the DEFAULT_COMPRESSION value.
 *
 * It is possible to change the compression level to NO_COMPRESSION or
 * use the setMethod() with STORED to avoid any compression (i.e. create
 * a zip file which awfully looks like a tarball).
 *
 * \todo
 * These values are one to one mapped to zlib compression values. This
 * is likely to change once we start offering other compression schemes
 * for a number defined between 0 and 100 instead.
 */


/** \brief Initialize a FileEntry object.
 *
 * This function initializes a FileEntry object. By default you may define
 * the filename of the FileEntry object.
 *
 * In case of an on disk directory, the filename should be the path to the
 * file that can be read from disk, otherwise the FileEntry object will be
 * viewed as invalid. There is, otherwise, no restriction to the filename.
 *
 * \param[in] filename  The file entry filename.
 * \param[in] comment  The comment attached to the file.
 */
FileEntry::FileEntry(FilePath const & filename, std::string const & comment)
    : m_filename(filename)
    , m_comment(comment)
{
}


/** \fn FileEntry::pointer_t FileEntry::clone() const;
 * \brief Create a clone of a file entry.
 *
 * This function creates a heap allocated clone of the object
 * this method is called for.
 *
 * Note that the object is expected to overload this function in
 * order to create a clone of the correct type.
 *
 * \return A smart pointer to the clone.
 */


/** \brief Clean up a FileEntry object.
 *
 * The destructor is defined as it has to be virtual.
 *
 * It will eventually clean up resources used by the FileEntry class.
 */
FileEntry::~FileEntry()
{
}


/** \brief Retrieve the comment of the file entry.
 *
 * This function returns the comment of this entry.
 *
 * If the entry was not assigned a comment, this function returns
 * an empty string. All entries can be given a comment, although
 * for most it will be ignored unless you save the file to a Zip
 * archive.
 *
 * \return The comment associated with this entry, if there is one.
 */
std::string FileEntry::getComment() const
{
    return m_comment;
}


/** \brief Retrieve the size of the file when compressed.
 *
 * This function returns the compressed size of the entry. If the
 * entry is not stored in a compressed format, the uncompressed
 * size is returned.
 *
 * \return The compressed size of the entry.
 */
size_t FileEntry::getCompressedSize() const
{
    return getSize();
}




/** \brief Return the CRC of the entry.
 *
 * This function returns the CRC 32 of this entry, if it has one.
 *
 * The CRC is set only after the file is compressed so it may
 * not always be available. The hasCrc() function can be used
 * to know whether it was set before.
 *
 * \return The CRC32 for the entry, if it has one.
 */
FileEntry::crc32_t FileEntry::getCrc() const
{
    /** \FIXME
     * Should we throw if m_has_crc_32 is false?
     */
    return m_crc_32;
}


/** \brief Get the offset of this entry in a Zip archive.
 *
 * This function retrieves the offset at which this FileEntry
 * resides in the Zip archive it is attached to.
 *
 * Note that in case of a Zip archive embedded in another file,
 * the offset is virtual (relative to the start of the Zip archive
 * in the larger file.)
 *
 * \return The position in the Zip archive.
 */
std::streampos FileEntry::getEntryOffset() const
{
    return m_entry_offset;
}


/** \brief Some extra data to be stored along the entry.
 *
 * This function returns a copy of the vector of bytes of extra data
 * that are stored with the entry.
 *
 * This buffer should be generated using the still non-existent
 * ZipExtra class. This includes definitions of additional meta
 * data necessary on various operating systems. For example, Linux
 * makes use of the "UT" (Universal Time) to save the atime, ctime,
 * and mtime parameters, and "ux" (Unix) to save the Unix permissions
 * and user identifier (uid) and group identifier (gid).
 *
 * \return A buffer_t of extra bytes that are associated with this entry.
 */
FileEntry::buffer_t FileEntry::getExtra() const
{
    return m_extra_field;
}


/** \brief Retrieve the size of the header.
 *
 * This function determines the size of the Zip archive header necessary
 * for that file.
 *
 * By default the function returns zero meaning that no header is defined.
 *
 * \return The size of the header in bytes.
 */
size_t FileEntry::getHeaderSize() const
{
    return 0;
}


/** \brief Retrieve the compression level.
 *
 * Use this function to read the compression level to use to compress
 * a file.
 *
 * Note that the compression level is rarely saved in the
 * destination file, so after reading a file from a Zip archive this
 * parameter is set to the default compression level which does not
 * represent the level used to create the file.
 *
 * The compression level is a number between 1 and 100 if compression
 * is wanted. 0 for no compression. A few negative numbers represent
 * various default compression levels.
 *
 * \return The compression level to use to write this entry to a Zip archive.
 *
 * \sa CompressionLevel
 * \sa setLevel()
 */
FileEntry::CompressionLevel FileEntry::getLevel() const
{
    if(isDirectory())
    {
        return COMPRESSION_LEVEL_NONE;
    }
    return m_compression_level;
}


/** \brief Return the method used to create this entry.
 *
 * This function returns the method used to store the entry data in
 * the FileCollection it is attached to.
 *
 * \return the storage method used to store the entry in a collection.
 *
 * \sa StorageMethod
 * \sa setMethod()
 */
StorageMethod FileEntry::getMethod() const
{
    if(isDirectory())
    {
        // make sure we do not return anything else than STORED
        // for a directory
        return StorageMethod::STORED;
    }
    return m_compress_method;
}




/** \brief Return the filename of the entry.
 *
 * The function returns the full filename of the entry, including
 * a path if the entry is stored in a sub-folder.
 *
 * \return The filename of the entry including its path.
 */
std::string FileEntry::getName() const
{
    return m_filename;
}


/** \brief Return the basename of this entry.
 *
 * This function returns the filename only of the entry.
 *
 * \warning
 * The function returns the last segment of the filename whether it is
 * a regular file or a directory so one can search for a directory with
 * the MATCH or IGNORE search options.
 *
 * \return The filename of the entry.
 */
std::string FileEntry::getFileName() const
{
    return m_filename.filename();
}


/** \brief Retrieve the size of the file when uncompressed.
 *
 * This function returns the uncompressed size of the entry data.
 *
 * \return Returns the uncompressed size of the entry.
 */
std::size_t FileEntry::getSize() const
{
    return m_uncompressed_size;
}


/** \brief Get the MS-DOS date/time of this entry.
 *
 * This function returns the date and time of the entry in MS-DOS
 * date/time format.
 *
 * \note
 * An MS-DOS date is limited to 127 years starting on 1980.
 * So it will be over after Dec 31, 2107.
 *
 * \note
 * In 32 bits, the Unix date is limited to 03:14:07 on Tuesday,
 * 19 January 2038. Please switch to a 64 bit OS soon.
 *
 * \return The date and time of the entry in MS-DOS format.
 */
DOSDateTime::dosdatetime_t FileEntry::getTime() const
{
    if(m_unix_time == 0)
    {
        return 0;
    }

    DOSDateTime t;
    t.setUnixTimestamp(m_unix_time);
    return t.getDOSDateTime();
}


/** \brief Get the Unix date/time of this entry.
 *
 * This function returns the date and time of the entry in Unix
 * date/time format (see time()).
 *
 * \note
 * The FileEntry object saves the time as a Unix time_t value,
 * however, the Zip file format uses the DOS time format. So
 * for a Zip file, the precision of the date is to the next
 * even second. Yet, this function may return a value which
 * is odd when the time comes from a file on disk.
 *
 * \note
 * Unless you have an old 32 bit system that defines time_t
 * as a 32 bit value, a Unix date can be considered infinite.
 * Otherwise it is limited to some time in 2038.
 *
 * \return The date and time of the entry as a time_t value.
 */
std::time_t FileEntry::getUnixTime() const
{
    return m_unix_time;
}


/** \brief Check whether the CRC32 was defined.
 *
 * This function returns true if the setCrc() function was called earlier
 * with a valid CRC32 and the FileEntry implementation supports a CRC (i.e.
 * a DirectoryEntry does not have a CRC).
 *
 * \return true if a CRC32 is defined in this class.
 */
bool FileEntry::hasCrc() const
{
    return m_has_crc_32;
}


/** \brief Check whether the filename represents a directory.
 *
 * This function checks the last character of the filename, if it
 * is a separator ('/') then the function returns true meaning
 * that the file represents a directory.
 *
 * \return true if the entry represents a directory.
 */
bool FileEntry::isDirectory() const
{
    return m_filename.isDirectory();
}


/** \brief Compare two file entries for equality.
 *
 * This function compares most of the fields between two file
 * entries to see whether they are equal or not.
 *
 * Note that this does not compare the file contents, only the
 * file entry meta data.
 *
 * \warning
 * The Extra buffer is ignored in the comparison. There are two
 * reasons for this: (1) it is unlikely that such a parameter
 * should count in the comparison (just like the compressed
 * size of the file) and (2) the comparison is not trivial as
 * each chunk in the buffer needs to be separately compared
 * (we may offer that capability once we have a ZipExtra class.)
 *
 * \param[in] file_entry  The file entry to compare this against.
 *
 * \return true if both FileEntry objects are considered equal.
 */
bool FileEntry::isEqual(FileEntry const & file_entry) const
{
    return m_filename          == file_entry.m_filename
        && m_comment           == file_entry.m_comment
        && m_uncompressed_size == file_entry.m_uncompressed_size
        && m_unix_time         == file_entry.m_unix_time
        && m_compress_method   == file_entry.m_compress_method
        && m_crc_32            == file_entry.m_crc_32
        && m_has_crc_32        == file_entry.m_has_crc_32
        && m_valid             == file_entry.m_valid;
        //&& m_extra_field       == file_entry.m_extra_field -- ignored in comparison
}


/** \brief Check whether this entry is valid.
 *
 * Any method or operator that initializes a FileEntry may set a
 * flag that specifies whether the file entry is valid or not. If
 * it is not this method returns false.
 *
 * \return true if the FileEntry has been parsed successfully.
 */
bool FileEntry::isValid() const
{
    return m_valid;
}


/** \brief Set the comment field for the FileEntry.
 *
 * This function sets the comment of this FileEntry. Note that
 * all implementations of the FileEntry may not include support
 * for a comment. In that case this function does nothing.
 *
 * \param[in] comment  A string with the new comment.
 */
void FileEntry::setComment(std::string const & comment)
{
    // WARNING: we do NOT check the maximum size here because it can depend
    //          on the output format which is just zip now but could be a
    //          bit extended later (i.e. Zip64)
    m_comment = comment;
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
void FileEntry::setCompressedSize(size_t size)
{
    static_cast<void>(size);
}


/** \brief Save the CRC of the entry.
 *
 * This function saves the CRC field in this FileEntry field.
 *
 * \param crc value to set the crc field to.
 */
void FileEntry::setCrc(crc32_t crc)
{
    static_cast<void>(crc);
}


/** \brief Defines the position of the entry in a Zip archive.
 *
 * This function defines the position of the FileEntry in a
 * Zip archive. By default the position is set to zero.
 *
 * The offset is generally read from a Zip directory entry.
 *
 * When used to seek in a file, the FileCollection will add
 * the start offset defined in the VirtualSeeker. In other words
 * this is the position in the Zip archive itself, not the final
 * position in the file you are reading the archive from.
 *
 * \param[in] offset  The new archive entry offset.
 */
void FileEntry::setEntryOffset(std::streampos offset)
{
    m_entry_offset = offset;
}


/** \brief Set the extra field buffer.
 *
 * This function is used to set the extra field.
 *
 * Only one type of file entry supports an extra field buffer.
 * The others do nothing when this function is called.
 *
 * \param[in] extra  The extra field is set to this value.
 */
void FileEntry::setExtra(buffer_t const & extra)
{
    m_extra_field = extra;
}


/** \brief Define the level of compression to use by this FileEntry.
 *
 * This function saves the level of compression the library should use
 * to compress the file before saving it in the output file.
 *
 * \note
 * If the StorageMethod is set to STORED, then the compression level is
 * ignored, but it is left unchanged.
 *
 * \exception InvalidStateException
 * This function raises this exception if the specified level is out of
 * the allowed range.
 *
 * \param[in] level  The compression level to use to compress the file data.
 */
void FileEntry::setLevel(CompressionLevel level)
{
    if(level < COMPRESSION_LEVEL_DEFAULT || level > COMPRESSION_LEVEL_MAXIMUM)
    {
        throw InvalidStateException("level must be between COMPRESSION_LEVEL_DEFAULT and COMPRESSION_LEVEL_MAXIMUM inclusive");
    }
    if(isDirectory())
    {
        if(level >= COMPRESSION_LEVEL_MINIMUM)
        {
            throw InvalidStateException("directories cannot be marked with a compression level other than COMPRESSION_LEVEL_NONE (defaults will also work");
        }
        m_compression_level = COMPRESSION_LEVEL_NONE;
    }
    else
    {
        m_compression_level = level;
    }
}


/** \brief Sets the storage method field for the entry.
 *
 * This function sets the method with which the file data is to
 * be compressed.
 *
 * The method is ignored in a file entry which cannot be compressed.
 * (or more precisely, the method is forced as STORED.)
 *
 * \exception InvalidStateException
 * This exception is raised if the \p method parameter does not represent
 * a supported method. At this time the library only supports STORED and
 * DEFLATED. The getMethod() may return more types as read from a Zip
 * archive, but it is not possible to set such types using this function.
 *
 * \param[in] method  The method field is set to the specified value.
 */
void FileEntry::setMethod(StorageMethod method)
{
    switch(method)
    {
    case StorageMethod::STORED:
    //case StorageMethod::SHRUNK:
    //case StorageMethod::REDUCED1:
    //case StorageMethod::REDUCED2:
    //case StorageMethod::REDUCED3:
    //case StorageMethod::REDUCED4:
    //case StorageMethod::IMPLODED:
    //case StorageMethod::TOKENIZED:
    case StorageMethod::DEFLATED:
    //case StorageMethod::DEFLATED64:
    //case StorageMethod::OLD_TERSE:
    //case StorageMethod::RESERVED11:
    //case StorageMethod::BZIP2:
    //case StorageMethod::REVERVED13:
    //case StorageMethod::LZMA:
    //case StorageMethod::RESERVED15:
    //case StorageMethod::RESERVED16:
    //case StorageMethod::RESERVED17:
    //case StorageMethod::NEW_TERSE:
    //case StorageMethod::LZ77:
    //case StorageMethod::WAVPACK:
    //case StorageMethod::PPMD_I_1:
        break;

    default:
        throw InvalidStateException("unknown method");

    }

    if(isDirectory())
    {
        // never compress directories
        m_compress_method = StorageMethod::STORED;
    }
    else
    {
        m_compress_method = method;
    }
}


/** \brief Sets the size field for the entry.
 *
 * This function is used to save the size of this file on disk
 * when uncompressed.
 *
 * \param[in] size  The size field is set to this value.
 */
void FileEntry::setSize(size_t size)
{
    m_uncompressed_size = size;
}


/** \brief Set the FileEntry time using a DOS time.
 *
 * This function saves the specified \p dosdatetime value as the last modification
 * date and time of this entry. This is generally used when reading that
 * information * from a Zip archive. Otherwise you probably want to use
 * the setUnixTime() instead since it is one to one compatible with the
 * value handle by time(), stat(), and other OS functions.
 *
 * \param[in] dosdatetime  Set time field as is using this MS-DOS date/time value.
 */
void FileEntry::setTime(DOSDateTime::dosdatetime_t dosdatetime)
{
    DOSDateTime t;
    t.setDOSDateTime(dosdatetime);
    setUnixTime(t.getUnixTimestamp());
}


/** \brief Sets the time field in Unix time format for the entry.
 *
 * This function is used to set the last modification time of this
 * entry. In most cases this comes from the stat structure field
 * named st_mtime. If you are creating a file directly in memory,
 * you may use the return value of <code>time(nullptr);</code>.
 *
 * \param[in] time  The time field is set to the specified value.
 */
void FileEntry::setUnixTime(std::time_t time)
{
    m_unix_time = time;
}


/** \brief Returns a human-readable string representation of the entry.
 *
 * This function transforms the basic information of the entry in a
 * string. Note that most of the information is lost as the function
 * is likely to only display the filename and the size of the file,
 * nothing more.
 *
 * \return A human-readable string representation of the entry.
 */
std::string FileEntry::toString() const
{
    OutputStringStream sout;
    sout << m_filename;
    if(isDirectory())
    {
        sout << " (directory)";
    }
    else
    {
        sout << " ("
             << m_uncompressed_size << " byte"
             << (m_uncompressed_size == 1 ? "" : "s");
        size_t const compressed_size(getCompressedSize());
        if(compressed_size != m_uncompressed_size)
        {
             // this is not currently accessible since only the
             // ZipLocalEntry and ZipCentralDirectoryEntry have
             // a compressed size
             sout << ",  " // LCOV_EXCL_LINE
                  << compressed_size << " byte" // LCOV_EXCL_LINE
                  << (compressed_size == 1 ? "" : "s") // LCOV_EXCL_LINE
                  << " compressed"; // LCOV_EXCL_LINE
        }
        sout << ")";
    }
    return sout.str();
}


/** \brief Read this FileEntry from the input stream.
 *
 * This function is called when the FileEntry should be initialized from
 * the specified input stream.
 *
 * \exception IOException
 * The default implementation raise an IOException error because there is
 * no reading the FileEntry from anywhere.
 *
 * \param[in,out] is  The input stream.
 */
void FileEntry::read(std::istream & is)
{
    static_cast<void>(is);
    throw IOException("FileEntry::read(): read not available with this type of FileEntry.");
}


/** \brief Write this FileEntry to the output stream.
 *
 * This function is called when the FileEntry should be saved in the
 * specified output stream.
 *
 * \exception IOException
 * The default implementation raise an IOException error because there is
 * no writing the FileEntry anywhere.
 *
 * \param[in,out] os  The output stream.
 */
void FileEntry::write(std::ostream & os)
{
    static_cast<void>(os);
    throw IOException("FileEntry::write(): write not available with this type of FileEntry.");
}


/** \brief Output an entry as a string to a stream.
 *
 * This function transforms the FileEntry basic information into a string
 * and prints the result to the specified output stream.
 *
 * \param[in,out] os  The output stream.
 * \param[in] entry  The entry to print out.
 *
 * \return A reference to the output stream.
 *
 * \sa FileEntry::toString()
 */
std::ostream & operator << (std::ostream & os, FileEntry const & entry)
{
    os << entry.toString();
    return os;
}


} // namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
