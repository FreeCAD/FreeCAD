
#include "zipios++/zipios-config.h"

#include <algorithm>
#include <string>
#include <vector>

#include "zipios++/fcoll.h"

namespace zipios {

using std::find_if ;

// FIXME: make InvalidStateException message customized for
// subclasses. maybe make an InvalidStateException factory ;-)

ConstEntries FileCollection::entries() const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to get entries from an invalid FileCollection" ) ;

  // The constructor below is not in all vector impl. (not those
  // without member templates)
  // ConstEntries ( _entries.begin(), _entries.end() ) ;
  // Instead of using that we copy the vector manually
  ConstEntries cep_vec ;
  cep_vec.reserve( _entries.size() ) ;
  Entries::const_iterator cit ;
  for ( cit = _entries.begin() ; cit != _entries.end() ; ++cit )
    cep_vec.push_back( *cit ) ;

  return cep_vec ;
}

ConstEntryPointer FileCollection::getEntry( const string &name, 
					   MatchPath matchpath ) const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to get an entry from an invalid FileCollection" ) ;

  Entries::const_iterator iter ;
  if ( matchpath == MATCH )
    iter = find_if( _entries.begin(), _entries.end(), FileEntry::MatchName( name ) ) ;
  else
    iter = find_if( _entries.begin(), _entries.end(), FileEntry::MatchFileName( name ) ) ;
  if ( iter == _entries.end() )
    return 0 ;
  else
    return *iter ; 
}

string FileCollection::getName() const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to get the name of an invalid FileCollection" ) ;
  return _filename ;
}


int FileCollection::size() const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to get size of an invalid FileCollection" ) ;
  return _entries.size() ;
}

FileCollection::~FileCollection() {
}


} // namespace

/** \file
    Implementation of FileCollection.
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
