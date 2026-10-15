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
 * \brief Implementation of zipios::DirectoryCollection.
 *
 * This file includes the implementation of the zipios::DirectoryCollection
 * class, which is used to read a directory from disk and create
 * a set of zipios::DirectoryEntry objects.
 */

#if !defined(ZIPIOS_WINDOWS) && (defined(_WINDOWS) || defined(WIN32) || defined(_WIN32) || defined(__WIN32))
#define ZIPIOS_WINDOWS
#endif

#include "zipios/directorycollection.hpp"

#include "zipios/zipiosexceptions.hpp"

#include <fstream>

#ifdef ZIPIOS_WINDOWS
#include <io.h>
#else
#include <dirent.h>
#include <errno.h>
#endif


namespace zipios
{

/** \class DirectoryCollection
 * \brief A collection generated from reading a directory.
 *
 * The DirectoryCollection class is a FileCollection that obtains
 * its entries from a directory on disk.
 */


/** \brief Initialize a DirectoryCollection object.
 *
 * The default constructor initializes an empty directory collection.
 * Note that an empty collection is invalid by default so there is
 * probably not much you will be able to do with such an object.
 */
DirectoryCollection::DirectoryCollection()
{
}


/** \brief Initialize a DirectoryCollection object.
 *
 * This function initializes a directory which represents a collection
 * of files from disk.
 *
 * By default recursive is true meaning that the specified directory
 * and all of its children are read in the collection.
 *
 * \warning
 * The specified path must be a valid directory path and name. If the
 * name represents a file, then the DirectoryCollection is marked as
 * invalid.
 *
 * \note
 * The file content is not loaded so the collection is fairly lightweight.
 *
 * \param[in] path  A directory path. If the name is not a valid
 *                  directory the created DirectoryCollection is
 *                  marked as being invalid.
 * \param[in] recursive  Whether to load all the files found in
 *                       sub-directories.
 */
DirectoryCollection::DirectoryCollection(
          std::string const & path
        , bool recursive)
    : m_recursive(recursive)
    , m_filepath(path)
{
    m_filename = m_filepath;
    m_valid = m_filepath.isDirectory() || m_filepath.isRegular();
}


/** \brief Clean up a DirectoryCollection object.
 *
 * The destructor ensures that the object is properly cleaned up.
 */
DirectoryCollection::~DirectoryCollection()
{
    close();
}


/** \brief Close the directory collection.
 *
 * This function marks the collection as invalid in effect rendering
 * the collection unusable.
 */
void DirectoryCollection::close()
{
    m_entries_loaded = false;
    m_filepath.clear();

    FileCollection::close();
}


/** \brief Retrieve a vector to the collection entries.
 *
 * This function makes sure that the directory collection is valid, if not
 * an exception is raised. If valid, then the function makes sure that
 * the entries were loaded and then it returns a copy of the vector
 * holding the entries.
 *
 * \note
 * The copy of the vector is required because of the implementation
 * of CollectionCollection which does not hold a vector of all the
 * entries defined in its children. It is also cleaner (albeit slower)
 * in case one wants to use the library in a thread environment.
 *
 * \return A copy of the internal FileEntry vector.
 */
FileEntry::vector_t DirectoryCollection::entries() const
{
    loadEntries();

    return FileCollection::entries();
}


/** \brief Get an entry from the collection.
 *
 * This function returns a shared pointer to a FileEntry object for
 * the entry with the specified name. To ignore the path part of the
 * filename while searching for a match, specify FileCollection::IGNORE
 * as the second argument.
 *
 * \note
 * The collection must be valid or the function raises an exception.
 *
 * \param[in] name  A string containing the name of the entry to get.
 * \param[in] matchpath  Specify MatchPath::MATCH, if the path should match
 *                       as well, specify MatchPath::IGNORE, if the path
 *                       should be ignored.
 *
 * \return A shared pointer to the found entry. The returned pointer
 *         is null if no entry is found.
 *
 * \sa mustBeValid()
 */
FileEntry::pointer_t DirectoryCollection::getEntry(std::string const & name, MatchPath matchpath) const
{
    loadEntries();

    return FileCollection::getEntry(name, matchpath);
}


/** \brief Retrieve pointer to an istream.
 *
 * This function returns a shared pointer to an istream defined from
 * the named entry, which is expected to be available in this collection.
 *
 * The function returns a null pointer if no FileEntry can be found from
 * the specified name or the FileEntry is marked as invalid.
 *
 * The returned istream represents a file on disk, although the filename
 * must exist in the collection or it will be ignored. A filename that
 * represents a directory cannot return an input stream and thus an error
 * is returned in that case.
 *
 * \note
 * The stream is always opened in binary mode.
 *
 * \param[in] entry_name  The name of the file to search in the collection.
 * \param[in] matchpath  Whether the full path or just the filename is matched.
 *
 * \return A shared pointer to an open istream for the specified entry.
 *
 * \sa CollectionCollection
 * \sa FileCollection
 * \sa ZipFile
 */
DirectoryCollection::stream_pointer_t DirectoryCollection::getInputStream(std::string const & entry_name, MatchPath matchpath)
{
    FileEntry::pointer_t ent(getEntry(entry_name, matchpath));
    if(ent == nullptr || ent->isDirectory())
    {
        return DirectoryCollection::stream_pointer_t();
    }

    DirectoryCollection::stream_pointer_t p(std::make_shared<std::ifstream>(ent->getName(), std::ios::in | std::ios::binary));
    return p;
}


/** \brief Create another DirectoryCollection.
 *
 * This function creates a clone of this DirectoryCollection. This is
 * a simple new DirectoryCollection of this collection.
 *
 * \return The function returns a shared pointer of the new collection.
 */
FileCollection::pointer_t DirectoryCollection::clone() const
{
    return std::make_shared<DirectoryCollection>(*this);
}


/** \brief This is an internal function that loads the file entries.
 *
 * This function is the top level which starts the process of loading
 * all the files found in the specified directory and sub-directories
 * if the DirectoryCollection was created with the recursive flag
 * set to true (the default.)
 */
void DirectoryCollection::loadEntries() const
{
    // WARNING: this has to stay here because the collection could get close()'d...
    mustBeValid();

    if(!m_entries_loaded)
    {
        m_entries_loaded = true;

        // if the read fails then the directory may have been deleted
        // in which case we want to invalidate this DirectoryCollection
        // object
        try
        {
            // include the root directory
            FileEntry::pointer_t entry(std::make_shared<DirectoryEntry>(m_filepath, ""));
            const_cast<DirectoryCollection *>(this)->m_entries.push_back(entry);

            // now read the data inside that directory
            if(m_filepath.isDirectory())
            {
                const_cast<DirectoryCollection *>(this)->load(FilePath());
            }
        }
        catch(...)
        {
            const_cast<DirectoryCollection *>(this)->close();
            throw;
        }
    }
}


/** \brief This is the function loading all the file entries.
 *
 * This function loads all the file entries found in the specified
 * directory. If the DirectoryCollection was created with the
 * recursive flag, then this function will load sub-directories
 * infinitum.
 *
 * \param[in] subdir  The directory to read.
 */
void DirectoryCollection::load(FilePath const & subdir)
{
#ifdef ZIPIOS_WINDOWS
    struct read_dir_t
    {
        read_dir_t(FilePath const & path)
        {
            /** \todo
             * Make necessary changes to support 64 bit and Unicode
             * (require utf8 -> wchar_t, then use _wfindfirsti64().)
             * We'll have to update the next() function too, of course.
             */
            m_handle = _findfirsti64(static_cast<std::string>(path).c_str(), &m_fileinfo);
            if(m_handle == 0)
            {
                if(errno == ENOENT)
                {
                    // this can happen, the directory is empty and thus has
                    // absolutely no information
                    m_read_first = true;
                }
                else
                {
                    throw IOException("an I/O error occurred while reading a directory");
                }
            }
        }

        ~read_dir_t()
        {
            // a completely empty directory may give us a "null pointer"
            // when calling _[w]findfirst[i64]()
            if(m_handle != 0)
            {
                _findclose(m_handle);
            }
        }

        std::string next()
        {
            if(m_read_first)
            {
                __int64 const r(_findnexti64(m_handle, &m_fileinfo));
                if(r != 0)
                {
                    if(errno != ENOENT)
                    {
                        throw IOException("an I/O error occurred while reading a directory");
                    }
                    return std::string();
                }
            }
            else
            {
                // the _findfirst() includes a response, use it!
                m_read_first = true;
            }

            return m_fileinfo.name;
        }

    private:
        long                    m_handle = 0;
        struct _finddatai64_t   m_fileinfo = {};
        bool                    m_read_first = 0;
    };
#else
    struct read_dir_t
    {
        read_dir_t(FilePath const & path)
            : m_dir(opendir(static_cast<std::string>(path).c_str()))
        {
            if(m_dir == nullptr)
            {
                throw IOException("an I/O error occurred while trying to access directory");
            }
        }

        ~read_dir_t()
        {
            closedir(m_dir);
        }

        std::string next()
        {
            // we must reset errno because readdir() does not change it
            // when the end of the directory is reached
            //
            // Note: readdir() is expected to be thread safe as long as
            //       each thread use a different m_dir parameter
            //
            errno = 0;
            struct dirent * entry(readdir(m_dir));
            if(entry == nullptr)
            {
                if(errno != 0)
                {
                    throw IOException("an I/O error occurred while reading a directory"); // LCOV_EXCL_LINE
                }
                return std::string();
            }

            return entry->d_name;
        }

    private:
        DIR *   m_dir = nullptr;
    };
#endif

    read_dir_t dir(m_filepath + subdir);
    for(;;)
    {
        std::string const & name(dir.next());
        if(name.empty())
        {
            break;
        }

        // skip the "." and ".." directories, they are never added to
        // a Zip archive
        if(name != "." && name != "..")
        {
            FileEntry::pointer_t entry(std::make_shared<DirectoryEntry>(m_filepath + subdir + name, ""));
            m_entries.push_back(entry);

            if(m_recursive && entry->isDirectory())
            {
                load(subdir + name);
            }
        }
    }
}


} // zipios namespace

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
