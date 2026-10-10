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
 * \brief Implementation of zipios::FilePath.
 *
 * This file includes the zipios::FilePath implementation which makes it
 * a little easier to handle the stat() system call on any file for any
 * system.
 */

#include "zipios/filepath.hpp"

#include "zipios_common.hpp"



namespace zipios
{


namespace
{


/** \brief Prune the trailing separator if present.
 *
 * This function is used to ensure that the FilePath does NOT end
 * with a separator.
 *
 * \warning
 * At this time the path is not canonicalized properly. We expect \p path
 * to not include double separators one after another. However, passing
 * such a path to the FilePath will keep it as is.
 *
 * \param[in] path  The path to prune of one trailing separator.
 *
 * \return The path as is or without the last '/'.
 */
std::string pruneTrailingSeparator(std::string path)
{
    if(path.size() > 0)
    {
        if(path[path.size() - 1] == g_separator)
        {
            path.erase(path.size() - 1);
        }
    }

    return path;
}


} // no name namespace



/** \class FilePath
 * \brief Handle a file path and name and its statistics.
 *
 * The FilePath class represents a path to a file or directory name.
 * FilePath has member functions to check if the file path is a valid
 * file system entity, and to check what kind of file system entity
 * it is, e.g. is it a file, a directory, a pipe, etc.
 *
 * It also knows of the last modification time and size of the file.
 *
 * \warning
 * The information about a file is cached so at the time it gets used
 * the file on disk may have changed, it may even have been deleted.
 */


/** \brief Initialize a FilePath object.
 *
 * The constructor saves the path and if \p check_exists is true, read
 * the file statistics, especially the st_mode.
 *
 * \param[in] path  A string representation of the path.
 *
 * \sa exists()
 * \sa pruneTrailingSeparator()
 */
FilePath::FilePath(std::string const & path)
    : m_path(pruneTrailingSeparator(path))
{
}


/** \brief Read the file mode.
 *
 * This function sets m_checked to true, stat()'s the path, to see if
 * it exists and to determine what type of file it is. All the query
 * functions call check() before they test a flag to make sure it
 * is set appropriately.
 *
 * This means stat()'ing is deferred until it becomes necessary. But also
 * it is cached meaning that if the file changes in between we get the
 * old flags.
 */
void FilePath::check() const
{
    if(!m_checked)
    {
        m_checked = true;

        /** \TODO
         * Under MS-Windows, we need to use _wstat() to make it work in
         * Unicode (i.e. UTF-8 to wchar_t then call _wstat()...) Also we
         * want to use the 64 bit variant to make sure that we get a
         * valid size. Any other reference to the stat() command should
         * be replace by using a FilePath().
         *
         * See zipios/zipios-config.hpp.in
         */
        m_stat = {};
        m_exists = stat(m_path.c_str(), &m_stat) == 0;
    }
}


/** \brief Replace the path with a new path.
 *
 * This function replaces the internal path of this FilePath with
 * the new specified path.
 *
 * \param[in] path  The new path to save in this object.
 *
 * \return A reference to this object.
 */
FilePath & FilePath::operator = (std::string const & path)
{
    m_path = pruneTrailingSeparator(path);
    m_checked = false;
    return *this;
}


/** \brief Retrieve the path.
 *
 * This operator can be used to retrieve a copy of the path.
 *
 * \return The m_path string returned as is (i.e. the whole path).
 */
FilePath::operator std::string () const
{
    return m_path;
}


/** \brief Append the a child name to this path.
 *
 * This function concatenates two FilePath objects and returns
 * another FilePath.
 *
 * A file separator is inserted between both names if appropriate.
 *
 * \warning
 * Note that the function allows you to append two full paths,
 * or even a relative path (left) to a full path (right), which
 * may result in a new path that is not quite sensible.
 *
 * \param[in] rhs  The right hand side.
 *
 * \return The lhs and rhs concatenated.
 */
FilePath FilePath::operator + (FilePath const & rhs) const
{
    if(m_path.empty())
    {
        return rhs;
    }

    if(rhs.m_path.empty())
    {
        return *this;
    }

    if(rhs.m_path[0] == g_separator)
    {
        return m_path + rhs.m_path;
    }

    return m_path + g_separator + rhs.m_path;
}


/** \brief Check whether two FilePath represent the same file.
 *
 * This function compares a FilePath object (this) and a C-string
 * to know whether the two are the same.
 *
 * A null pointer as the C-string is viewed as an empty string.
 *
 * \param[in] rhs  The right hand side to compare with.
 *
 * \sa operator == (FilePath const & rhs);
 */
bool FilePath::operator == (char const * rhs) const
{
    return m_path == rhs;
}


/** \brief Check whether two FilePath represent the same file.
 *
 * This function compares a FilePath object (this) and a C-string
 * to know whether the two are the same.
 *
 * A null pointer as the C-string is viewed as an empty string.
 *
 * \param[in] lhs  The left hand side to compare with.
 * \param[in] rhs  The right hand side to compare with.
 *
 * \sa operator == (FilePath const & rhs);
 */
bool operator == (char const * lhs, FilePath const & rhs)
{
    return lhs == rhs.m_path;
}


/** \brief Check whether two FilePath represent the same file.
 *
 * This function compares a FilePath object (this) against
 * a string representing a path to know whether the two are
 * the equal.
 *
 * \param[in] rhs  The right hand side to compare with.
 *
 * \sa operator == (FilePath const & rhs);
 */
bool FilePath::operator == (std::string const & rhs) const
{
    return m_path == rhs;
}


/** \brief Check whether two FilePath represent the same file.
 *
 * This function compares a FilePath object (this) against
 * a string representing a path to know whether the two are
 * the equal.
 *
 * \param[in] lhs  The left hand side to compare with.
 * \param[in] rhs  The right hand side to compare with.
 *
 * \sa operator == (FilePath const & rhs);
 */
bool operator == (std::string const & lhs, FilePath const & rhs)
{
    return lhs == rhs.m_path;
}


/** \brief Check whether two FilePath represent the same file.
 *
 * This function compares two FilePath objects (this and rhs)
 * to know whether the two are the same.
 *
 * \note
 * It is important to know that the compare is rather primitive.
 * The two paths must be equal character by character instead
 * of actually representing exactly the same file. Also relative
 * paths will likely be equal and these may not represent the
 * same file at all.
 *
 * \param[in] rhs  The right hand side to compare with.
 *
 * \sa operator == (char const * rhs);
 * \sa operator == (std::string const & rhs);
 */
bool FilePath::operator == (FilePath const & rhs) const
{
    return m_path == rhs.m_path;
}


/** \brief Clear the filename.
 *
 * This function clears the path to an empty string.
 */
void FilePath::clear()
{
    m_path.clear();
    m_checked = false;
}


/** \brief Retrieve the basename.
 *
 * This function returns the filename part of the FilePath
 * object by pruning the path off.
 *
 * \return Return the basename of this FilePath filename.
 */
std::string FilePath::filename() const
{
    std::string::size_type const pos(m_path.find_last_of(g_separator));
    if(pos != std::string::npos)
    {
        return m_path.substr(pos + 1);
    }

    return m_path;
}


/** \brief Get the length of the string.
 *
 * This function returns the length of the string used to
 * represent this FilePath path and filename.
 *
 * \return The length of the string representing this file path.
 *
 * \sa size()
 */
size_t FilePath::length() const
{
    return m_path.length();
}


/** \brief Get the length of the string.
 *
 * This function returns the length of the string used to
 * represent this FilePath path and filename.
 *
 * \note
 * This is an overloaded function that calls the length() function.
 * It is defined because the string represents an array of bytes
 * and as such the size() function may be used.
 *
 * \return The length of the string representing this file path.
 *
 * \sa length()
 */
size_t FilePath::size() const
{
    return length();
}


/** \brief Check whether the filename is empty.
 *
 * This function returns true if the filename is empty. In other words, this
 * function returns true if the filename is not currently defined.
 *
 * \note
 * This function returns true after a call to the clear() function.
 *
 * \warning
 * If you are trying to know whether the file itself is empty, please use
 * the fileSize() instead and check whether it is zero or not:
 *
 *     if(file_path.fileSize() == 0) ...empty file...
 *
 * \return true when the file path is empty.
 *
 * \sa clear()
 * \sa fileSize()
 */
bool FilePath::empty() const
{
    return m_path.empty();
}


/** \brief Check whether the file exists.
 *
 * This function calls check() and then returns true if the file
 * exists on disk.
 *
 * \return true If the path is a valid file system entity.
 */
bool FilePath::exists() const
{
    check();
    return m_exists;
}


/** \brief Check whether the file is a regular file.
 *
 * This function returns true if the file exists and is a
 * regular file.
 *
 * \return true if the path is a regular file.
 */
bool FilePath::isRegular() const
{
    check();
    return m_exists && S_ISREG(m_stat.st_mode);
}


/** \brief Check whether the file is a directory.
 *
 * This function returns true if the file exists and is a
 * directory.
 *
 * \return true if the path is a directory.
 */
bool FilePath::isDirectory() const
{
    check();
    return m_exists && S_ISDIR(m_stat.st_mode);
}


/** \brief Check whether the file is a character special file.
 *
 * This function returns true if the file exists and is a
 * character special file.
 *
 * \return true if the path is character special (a character device file).
 */
bool FilePath::isCharSpecial() const
{
    check();
    return m_exists && S_ISCHR(m_stat.st_mode);
}


/** \brief Check whether the file is a block special file.
 *
 * This function returns true if the file exists and is a
 * block special file.
 *
 * \return true if the path is block special (a block device file).
 */
bool FilePath::isBlockSpecial() const
{
    check();
    return m_exists && S_ISBLK(m_stat.st_mode);
}


/** \brief Check whether the file is a socket.
 *
 * This function returns true if the file exists and is a
 * socket file.
 *
 * \return true if the path is a socket.
 */
bool FilePath::isSocket() const
{
    check();
    return m_exists && S_ISSOCK(m_stat.st_mode);
}


/** \brief Check whether the file is a pipe.
 *
 * This function returns true if the file exists and is a
 * pipe file.
 *
 * \return true if the path is a FIFO.
 */
bool FilePath::isFifo() const
{
    check();
    return m_exists && S_ISFIFO(m_stat.st_mode);
}


/** \brief Get the size of the file.
 *
 * This function returns the size of the file. The size may be a 64 bit
 * size on 64 bit systems.
 *
 * \note
 * If the file represents a directory, the size will be zero.
 *
 * \note
 * If the file is not considered valid, the size returned is zero.
 *
 * \warning
 * There is also a function called size() which actually checks the
 * length of the path and not the size of the file.
 *
 * \return The last modification as a Unix time.
 *
 * \sa size()
 */
size_t FilePath::fileSize() const
{
    check();
    return m_stat.st_size;
}


/** \brief Get the last modification time of the file.
 *
 * This function returns the last modification time of the specified
 * file.
 *
 * \note
 * If the file is not considered valid, the time returned is zero.
 *
 * \return The last modification as a Unix time.
 */
std::time_t FilePath::lastModificationTime() const
{
    check();
    return m_stat.st_mtime;
}


/** \brief Print out a FilePath.
 *
 * This function prints out the name of the file that this FilePath
 * represents.
 *
 * \param[in,out] os  The output stream.
 * \param[in] path  The path to print out.
 *
 * \return A copy of the \p os stream reference.
 */
std::ostream & operator << (std::ostream & os, FilePath const & path)
{
    os << static_cast<std::string>(path);
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
