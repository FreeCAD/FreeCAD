#ifndef COLLCOLL_H
#define COLLCOLL_H

#include "zipios++/zipios-config.h"

#include <string>
#include <vector>

#include "zipios++/fcoll.h"

namespace zipios {

using std::string ;

/** \anchor collcoll_anchor
    CollectionCollection is a FileCollection that consists of an
    arbitrary number of FileCollections. With a CollectionCollection
    the user can use multiple FileCollections transparently, making it
    easy for a program to keep some of its files in a zip archive and
    others stored in ordinary files. CollectionCollection can be used
    to create a simple virtual filesystem, where all collections are
    mounted in /. If more than one collection contain a file with
    the same path only the one in the first added collection will
    be accessible.
*/
class CollectionCollection : public FileCollection {
public:
  /** \anchor collcoll_inst_anchor
      This static method provides a singleton instance of a CollectionCollection.
      The instance is instantiated the first time the method is called.
      @return A pointer to a singleton CollectionCollection instance.
   */
  static inline CollectionCollection &inst() ;

  /** Constructor.
   */
  explicit CollectionCollection() ;

  /** Copy constructor. */
  inline CollectionCollection( const CollectionCollection &src ) ;

  /** Copy assignment operator. */
  inline const CollectionCollection &operator= ( const CollectionCollection &src ) ;

  /** \anchor collcoll_addcoll_anchor
      Adds a collection.
      @param collection The collection to add.
      @return true if the collection was added succesfully and
      the added collection is valid.
   */
  bool addCollection( const FileCollection &collection ) ;

  /** Adds the collection pointed to by collection. The CollectionCollection
      will call delete on the pointer when it is destructed, so the caller
      should make absolutely sure to only pass in a collection created with new
      and be sure to leave it alone after adding it. If the collection is not
      added false is returned and the caller remains responsible for the
      collection pointed to by collection.
      @param collection A pointer to the collection to add.
      @return true if the collection was added succesfully and
      the added collection is valid. */
  bool addCollection( FileCollection *collection ) ;

  virtual void close() ;

  virtual ConstEntries entries() const ;

  virtual ConstEntryPointer getEntry( const string &name, 
				      MatchPath matchpath = MATCH ) const ;

  virtual istream *getInputStream( const ConstEntryPointer &entry ) ;

  virtual istream *getInputStream( const string &entry_name, 
				   MatchPath matchpath = MATCH ) ;

  /** Returns the number in entries in all collections kept by
      the CollectionCollection object */
  virtual int size() const ;
  
  virtual FileCollection *clone() const ;

  virtual ~CollectionCollection() ;

protected:
  /** A protected getEntry member function, that not only
      finds an entry that match the name, if such an entry exists
      in the collection, it also returns, which collection it was found
      in.
   */
  void getEntry( const string &name,
		 ConstEntryPointer &cep, 
		 std::vector< FileCollection * >::const_iterator &it, 
		 MatchPath matchpath = MATCH ) const ;
  
  vector< FileCollection * > _collections ;
private:
  static CollectionCollection *_inst ;
};


/** Shortcut name for a CollectionCollection. If the static method
inst is used, it is often used a lot, so it's handy with a short name for
CollectionCollection */
typedef CollectionCollection CColl ;


//
// Inline (member) functions
//

CollectionCollection &CollectionCollection::inst() {
  if( _inst != 0 )
    return *_inst ;
  else
    return *( _inst = new CollectionCollection ) ;
}

CollectionCollection::CollectionCollection( const CollectionCollection &src ) 
  : FileCollection( src )
{
  _collections.reserve( src._collections.size() ) ;
  std::vector< FileCollection * >::const_iterator it ;
  for ( it = src._collections.begin() ; it != src._collections.end() ; ++it )
    _collections.push_back( (*it)->clone() ) ;
}


const CollectionCollection &
CollectionCollection::operator= ( const CollectionCollection &src ) {
  this->FileCollection::operator=( src ) ;
//    FileCollection::=( static_cast< FileCollection >( src ) ) ; 

  if ( this != &src ) {
    // Destroy current contents.
    std::vector< FileCollection * >::const_iterator it ;
    for ( it = _collections.begin() ; it != _collections.end() ; ++it )
      delete *it ;
    //  Then copy src's content.
    _collections.clear() ;
    _collections.reserve( src._collections.size() ) ;
    for ( it = src._collections.begin() ; it != src._collections.end() ; ++it )
      _collections.push_back( (*it)->clone() ) ;
  }
  return *this ;
}

} // namespace



#endif

/** \file
    Header file that defines CollectionCollection.
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
