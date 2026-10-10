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
 * \brief Implementation of zipios::CollectionCollection.
 *
 * This class derives from zipios::FileCollection. It allows to many
 * any number of collections within one collection.
 */

#include "zipios/collectioncollection.hpp"

#include "zipios/zipiosexceptions.hpp"

#include "zipios_common.hpp"


namespace zipios
{


namespace
{

/** \brief Search for an entry.
 *
 * This function searches for an entry that match the given name.
 * If that entry exists, the \p it parameter will be pointing
 * to it.
 *
 * The \p cep parameter is also set to the object found.
 *
 * \param[in] collections  The collections to search for the specified name.
 * \param[in] name  The name of the entry to search.
 * \param[out] cep  The pointer to the entry found.
 * \param[out] file_collection  A reference to a smarter pointer where we
 *                              can save the found file collection.
 * \param[in] matchpath  How the name of the entry is compared with \p name.
 */
void matchEntry(
      CollectionCollection::vector_t collections
    , std::string const & name
    , FileEntry::pointer_t & cep
    , FileCollection::pointer_t & file_collection
    , CollectionCollection::MatchPath matchpath)
{
    for(auto it = collections.begin(); it != collections.end(); ++it)
    {
        cep = (*it)->getEntry(name, matchpath);
        if(cep)
        {
            file_collection = *it;
            return;
        }
    }
    cep.reset();
    file_collection.reset();
}

} // no name namespace


/** \class CollectionCollection
 * \brief A collection of collections.
 *
 * CollectionCollection is a FileCollection that consists of an
 * arbitrary number of FileCollection's. With a CollectionCollection
 * the user can use multiple FileCollection objects transparently, making
 * it easy for a program to keep some of its files in a zip archive and
 * others stored in ordinary files. CollectionCollection can be used
 * to create a simple virtual filesystem, where all collections are
 * mounted on /. If more than one collection contain a file with
 * the same path only the one in the first added collection is
 * accessible.
 */



/** \brief Initialize a CollectionCollection object.
 *
 * The constructor initializes the CollectionCollection as a valid
 * collection.
 */
CollectionCollection::CollectionCollection()
{
    m_valid = true; // we are valid even though we are empty!
}


/** \brief Copy a CollectionCollection in another.
 *
 * This function copies a collection of collections in another. Note
 * that all the children get cloned so the copy can be edited without
 * modify the source and vice versa.
 *
 * \param[in] rhs  The source to copy in the new CollectionCollection.
 */
CollectionCollection::CollectionCollection(CollectionCollection const & rhs)
    : FileCollection(rhs)
{
    m_collections.reserve(rhs.m_collections.size());
    for(auto it = rhs.m_collections.begin(); it != rhs.m_collections.end(); ++it)
    {
        m_collections.push_back((*it)->clone());
    }
}


/** \brief Copy assignment operator.
 *
 * This assignment operator copies \p rhs to this collection replacing
 * the file entries that exist in this collection.
 *
 * Note that the source file entries are cloned in the destination so
 * modifying this collection will not modify the source.
 *
 * \param[in] rhs  The source to copy in this collection.
 */
CollectionCollection & CollectionCollection::operator = (CollectionCollection const & rhs)
{
    FileCollection::operator = (rhs);

    if(this != &rhs)
    {
        m_collections.clear();
        // A call to the CollectionCollection::size() function has side
        // effects, try to avoid them at this time
        //m_collections.reserve(rhs.m_collections.size());
        for(auto it = rhs.m_collections.begin(); it != rhs.m_collections.end(); ++it)
        {
            m_collections.push_back((*it)->clone());
        }
    }

    return *this;
}


/** \brief Create a clone of this object.
 *
 * This function creates a heap allocated clone of the CollectionCollection.
 *
 * Note that all the collections that this CollectionCollection points
 * to are all going to get cloned.
 *
 * \return A shared pointer to a copy of this CollectionCollection.
 */
FileCollection::pointer_t CollectionCollection::clone() const
{
    return std::make_shared<CollectionCollection>(*this);
}


/** \brief Clean up this CollectionCollection object.
 *
 * This function ensures that the CollectionCollection object
 * is cleaned up before deallocating the memory.
 */
CollectionCollection::~CollectionCollection()
{
    close();
}


/** \brief Add a FileCollection to this CollectionCollection.
 *
 * This function adds a collection in this CollectionCollection.
 * Since a CollectionCollection is itself a FileCollection, you
 * may add a CollectionCollection to another CollectionCollection.
 *
 * \note
 * The FileCollection to be added must be valid or it will be ignored.
 *
 * \param[in] collection  The collection to add.
 *
 * \return true if the collection was added successfully.
 *
 * \sa addCollection(FileCollection::pointer_t collection);
 */
bool CollectionCollection::addCollection(FileCollection const & collection)
{
    mustBeValid();

    /** \TODO
     * At this time the function verifies that you are not trying to add
     * a CollectionCollection to itself. However, this test is currently
     * really weak. We need to check whether any collection in the
     * input \p collection represents this collection.
     */
    if(this == &collection || !collection.isValid())
    {
        return false;
    }

    m_collections.push_back(collection.clone());

    return true;
}


/** \brief Add a collection to this CollectionCollection.
 *
 * This function adds the collection pointed to by \p collection to
 * this CollectionCollection.
 *
 * The CollectionCollection makes a clone of the specified \p collection
 * to make sure management of the child collection works as expected.
 *
 * If the collection does not get added, the function returns false.
 * This happens when the \p collection parameter represents an invalid
 * collection.
 *
 * \exception InvalidException
 * The function raises InvalidException if the \p collection parameter
 * is a null pointer.
 *
 * \param[in] collection  A pointer to the collection to add.
 *
 * \return true if the collection was added successfully.
 *
 * \sa addCollection(FileCollection const & collection);
 */
bool CollectionCollection::addCollection(FileCollection::pointer_t collection)
{
    if(collection == nullptr)
    {
        // TBD: should we return false instead?
        throw InvalidException("CollectionCollection::addCollection(): called with a null collection pointer");
    }

    return addCollection(*collection);
}


/** \brief Close the CollectionCollection object.
 *
 * This function marks the collection as invalid in effect rendering
 * the collection unusable. Note that all the collections that you
 * previously added to this collection all get marked as invalid
 * (i.e. their close() function gets called.) This has the nice side
 * effect to release memory immediately.
 *
 * \note
 * This is different from creating an empty CollectionCollection
 * which is empty and valid.
 */
void CollectionCollection::close()
{
    // make sure to close all the children first
    // (although I would imagine that the m_collections.clear() should
    // be enough, unless someone else has a reference to another one
    // of the sub-collections--but I do not think one can get such as
    // reference at this point, remember that the addCollection()
    // creates a clone of the collection being added.)
    for(auto it = m_collections.begin(); it != m_collections.end(); ++it)
    {
        // each collection in the collection must be valid since we
        // may hit any one of them
        (*it)->close();
    }
    m_collections.clear();

    FileCollection::close();
}


/** \brief Retrieve a vector to all the collection entries.
 *
 * This function gathers the entries of all the children collections
 * and add them to a vector that it then returns.
 *
 * The CollectionCollection itself has no entries.
 *
 * It is possible to define a CollectionCollection as a child of
 * another CollectionCollection. The process repeats infinitum
 * as required.
 *
 * \return A copy of all the entries found in the child Collections.
 */
FileEntry::vector_t CollectionCollection::entries() const
{
    mustBeValid();

    FileEntry::vector_t all_entries;
    for(auto it = m_collections.begin(); it != m_collections.end(); ++it)
    {
        all_entries += (*it)->entries();
    }

    return all_entries;
}


/** \brief Get an entry from the collection.
 *
 * This function returns a shared pointer to a FileEntry object for
 * the entry with the specified name. To ignore the path part of the
 * filename while searching for a match, specify
 * FileCollection::MatchPath::IGNORE as the second argument.
 * (the default is FileCollection::MatchPath::MATCH.
 *
 * \warning
 * In case of the CollectionCollection, the matching goes from child
 * collection to child collection in the order they were added to
 * the CollectionCollection. The first match is returned and at this
 * point there is nothing linking a FileEntry to its collection so
 * you will NOT be able to retrieve an istream to access that
 * FileEntry data. To do that, you must directly call the
 * getInputStream() function. We may fix that problem at a later
 * time and offer the getInputStream directly on the FileEntry
 * instead of the collection. This is problematic at this point
 * since, as we can see in the zipfile.cpp, we need to have
 * access to the m_zs offset.
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
FileEntry::pointer_t CollectionCollection::getEntry(std::string const & name, MatchPath matchpath) const
{
    mustBeValid();

    // Returns the first matching entry.
    FileCollection::pointer_t file_collection;
    FileEntry::pointer_t cep;

    matchEntry(m_collections, name, cep, file_collection, matchpath);

    return cep;
}


/** \brief Retrieve pointer to an istream.
 *
 * This function returns a shared pointer to an istream defined from the
 * named entry, which is expected to be available in this collection.
 *
 * The function returns a NULL pointer if there is no entry with the
 * specified name in this CollectionCollection. Note that the name is
 * searched in all the child collections of the CollectionCollection.
 *
 * Note that the function returns a smart pointer to an istream. In
 * general the CollectionCollection will not hold a copy of that pointer
 * meaning that if you call getInputStream() multiple times with the same
 * \p entry_name parameter, you get distinct istream instances each
 * time.
 *
 * By default the \p entry_name parameter is expected to match the full
 * path and filename (MatchPath::MATCH). If you are looking for a file
 * and want to ignore the directory name, set the matchpath parameter
 * to MatchPath::IGNORE.
 *
 * \param[in] entry_name  The name of the file to search in the collection.
 * \param[in] matchpath  Whether the full path or just the filename is matched.
 *
 * \return A shared pointer to an open istream for the specified entry.
 *
 * \sa FileCollection
 * \sa DirectoryCollection
 * \sa ZipFile
 */
CollectionCollection::stream_pointer_t CollectionCollection::getInputStream(std::string const & entry_name, MatchPath matchpath)
{
    mustBeValid();

    FileCollection::pointer_t file_collection;
    FileEntry::pointer_t cep;

    matchEntry(m_collections, entry_name, cep, file_collection, matchpath);

    return cep ? file_collection->getInputStream(entry_name) : nullptr;
}


/** \brief Return the size of the of this collection.
 *
 * This function computes the total size of this collection which
 * is to sum of the size of its child collections.
 *
 * \warning
 * This function has the side effect of loading all the data from
 * DirectoryCollection objects.
 *
 * \return The total size of the collection.
 */
size_t CollectionCollection::size() const
{
    mustBeValid();

    size_t sz(0);
    for(auto it = m_collections.begin(); it != m_collections.end(); ++it)
    {
        sz += (*it)->size();
    }

    return sz;
}


/** \brief Check whether the collection is valid.
 *
 * This function verifies that the collection is valid. If not, an
 * exception is raised. Many other functions from the various collection
 * functions are calling this function before accessing data.
 *
 * \exception InvalidStateException
 * This exception is raised if the m_valid field is currently false and
 * thus most of the collection data is considered invalid.
 */
void CollectionCollection::mustBeValid() const
{
    // self must be valid
    FileCollection::mustBeValid();

    for(auto it = m_collections.begin(); it != m_collections.end(); ++it)
    {
        // each collection in the collection must be valid since we
        // may hit any one of them
        (*it)->mustBeValid();
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
