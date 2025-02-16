#ifndef fcoll_H
#define fcoll_H

#include "zipios-config.h"

#include <vector>
#include <string>

#include "fcollexceptions.h"
#include "fileentry.h"

namespace zipios {

using std::vector;

/** \anchor fcoll_anchor
    FileCollection is an abstract baseclass that represents a
    collection of files. The specializations of FileCollection
    represents different origins of file collections, such as
    directories, simple filename lists and compressed archives. */
class BaseExport FileCollection {
public:
  /** FileCollection constructor. */
  explicit FileCollection()
    : _filename( "-"   ),
      _entries ( 0     ),
      _valid   ( false ) {}

  /** Copy constructor. */
  inline FileCollection( const FileCollection &src ) ;

  /** Copy assignment operator. */
  inline const FileCollection &operator= ( const FileCollection &src ) ;
  
  /** Closes the FileCollection. */
  virtual void close() = 0 ;

  /** \anchor fcoll_entries_anchor
      Returns a vector of const pointers to the entries in the
      FileCollection.  
      @return a ConstEntries
      containing the entries of the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual ConstEntries entries() const ;

  enum MatchPath { IGNORE, MATCH } ;

  /** \anchor fcoll_getentry_anchor
      Returns a ConstEntryPointer to a FileEntry object for the entry 
      with the specified name. To ignore the path part of the filename in search of a
      match, specify FileCollection::IGNORE as the second argument.
      @param name A string containing the name of the entry to get.
      @param matchpath specify MATCH, if the path should match as well, 
      specify IGNORE, if the path should be ignored.
      @return A ConstEntryPointer to the found entry. The returned pointer
      equals zero if no entry is found.
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual ConstEntryPointer getEntry( const string &name, 
				     MatchPath matchpath = MATCH ) const ;
  /** \anchor fcoll_getinputstream
      Returns a pointer to an opened istream for the specified
      FileEntry. It is the callers responsibility to delete the stream
      when he is done with it. Returns 0, if there is no such
      FileEntry in the FileCollection.
      @param entry A ConstEntryPointer to the FileEntry to get an istream to.
      @return an open istream for the specified entry. The istream is allocated on
      heap and it is the users responsibility to delete it when he is done with it.
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual istream *getInputStream( const ConstEntryPointer &entry ) = 0 ;

  /** Returns a pointer to an opened istream for the specified entry name.
      It is the callers responsibility to delete the stream when he is done
      with it. Returns 0, if there is no entry with the specified name in the
      FileCollection.
      @param entry_name
      @param matchpath specify MATCH, if the path should match as well, 
                       specify IGNORE, if the path should be ignored.
      @return an open istream for the specified entry. The istream is 
      allocated on heap and it is the users responsibility to delete it when
      he is done with it.
      @throw InvalidStateException Thrown if the collection is invalid.
   */
  virtual istream *getInputStream( const string &entry_name, 
				     MatchPath matchpath = MATCH ) = 0 ;
  /** Returns the name of the FileCollection.
      @return the name of the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual string getName() const ;
  /** Returns the number of entries in the FileCollection.
      @return the number of entries in the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual int size() const ;

  /** The member function returns true if the collection is valid.
      @return true if the collection is valid.
   */ 
  bool isValid() const { return _valid ; }

  /** Create a heap allocated clone of the object this method is called for. The 
      caller is responsible for deallocating the clone when he is done with it.
      @return A heap allocated copy of the object this method is called for.
  */
  virtual FileCollection *clone() const = 0 ;


  /** FileCollection destructor. */
  virtual ~FileCollection () ;
protected:
  string _filename ;
  Entries _entries ;
  bool _valid ;
};


//
// Inline methods
//

FileCollection::FileCollection( const FileCollection &src )
  : _filename( src._filename ),
    _valid   ( src._valid    )
{
  _entries.reserve( src._entries.size() ) ;
  Entries::const_iterator it ;
  for ( it = src._entries.begin() ; it != src._entries.end() ; ++it )
    _entries.push_back( (*it)->clone() ) ;
}

const FileCollection &FileCollection::operator= ( const FileCollection &src ) {
  if ( this != &src ) {
    _filename = src._filename ;
    _valid    = src._valid    ;
    _entries.clear() ;
    _entries.reserve( src._entries.size() ) ;

    Entries::const_iterator it ;
    for ( it = src._entries.begin() ; it != src._entries.end() ; ++it )
      _entries.push_back( (*it)->clone() ) ;
  }
  return *this ;
}

inline ostream & operator<< (ostream &os, const FileCollection& collection) {
	os << "collection '" << collection.getName() << "' {" ;
	ConstEntries entries = collection.entries();
	ConstEntries::const_iterator it;
	bool isFirst=true;
	for (it=entries.begin(); it != entries.end(); ++it) {
		if(! isFirst)
			os << ", ";
		isFirst = false;
		os << (*it)->getName();
	}
	os << "}";
	return os;
}


} // namespace

#endif

/** \file
    Header file that defines FileCollection.
*/

/*
  Zipios++ - a small C++ library that provides easy access to .zip files.
  Copyright (C) 2000  Thomas Søndergaard
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/
