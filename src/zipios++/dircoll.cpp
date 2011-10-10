
#include "zipios-config.h"

#include "meta-iostreams.h"
#include <vector>
#include <sys/stat.h>

#include "dircoll.h"

#include "directory.h"


using namespace zipios;

DirectoryCollection::DirectoryCollection( const string &path, bool recursive, 
					  bool load_now ) 
  : _entries_loaded( false ),
    _recursive     ( recursive ),
    _filepath      ( path      )
{
  _filename = _filepath ;
  _valid = _filepath.isDirectory() ;

  if( _valid && load_now )
    loadEntries() ;
}

void DirectoryCollection::close() {
  _valid = false ;
}


ConstEntries DirectoryCollection::entries() const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid DirectoryCollection" ) ;

  loadEntries() ;

  return FileCollection::entries() ;
}


ConstEntryPointer
DirectoryCollection::getEntry( const string &name, 
			       MatchPath matchpath ) const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid DirectoryCollection" ) ;

  if ( matchpath != MATCH || _entries_loaded ) {
    loadEntries() ;
    return FileCollection::getEntry( name, matchpath ) ;
  } else {
    // avoid loading entries if possible.
    ConstEntryPointer ent ( new DirEntry( name, "", _filepath ) ) ;
    if ( ent->isValid() )
      return ent ;
    else
      return 0 ;
  }
}


istream *DirectoryCollection::getInputStream( const ConstEntryPointer &entry ) {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid DirectoryCollection" ) ;

  return getInputStream( entry->getName() ) ;
}


std::istream *DirectoryCollection::getInputStream( const string &entry_name, 
					      MatchPath matchpath ) 
{
  using std::ifstream ;

  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid DirectoryCollection" ) ;

  if ( matchpath != MATCH || _entries_loaded ) {
    loadEntries() ;

    ConstEntryPointer ent = getEntry( entry_name, matchpath ) ;
    
    if ( ent == 0 )
      return 0 ;
    else {
      string real_path( _filepath + entry_name ) ;
      return new ifstream( real_path.c_str(), ios::in | ios::binary ) ;
    }

  } else {
    // avoid loading entries if possible.
    string real_path( _filepath + entry_name ) ;
    ifstream *ifs = new ifstream( real_path.c_str(), ios::in | ios::binary ) ;
    if( ! *ifs ) {
      delete ifs ;
      return 0 ;
    } else 
      return ifs ;
  }  
}


int DirectoryCollection::size() const {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid DirectoryCollection" ) ;
  loadEntries() ;

  return _entries.size() ;
}

FileCollection *DirectoryCollection::clone() const {
  return new DirectoryCollection( *this ) ;
}

DirectoryCollection::~DirectoryCollection() {}


void DirectoryCollection::loadEntries() const {
  if( _entries_loaded )
    return ;

  const_cast< DirectoryCollection * >( this )->load( _recursive ) ;

  _entries_loaded = true ;
}


void DirectoryCollection::load( bool recursive, const FilePath &subdir ) {
  using namespace boost::filesystem ;
  BasicEntry *ent ;
  for ( dir_it it( _filepath + subdir ) ; it != dir_it() ; ++it ) {

    if ( *it == "." || *it == ".." || *it == "..." )
      continue ;

    if ( get< is_directory >( it ) && recursive ) {
      load( recursive, subdir + *it ) ;
    } else {
      _entries.push_back( ent = new BasicEntry( subdir + *it, "", _filepath ) ) ;
      ent->setSize( get< boost::filesystem::size >( it ) ) ;
    }

  }
}

// namespace

/** \file
    Implementation of DirectoryCollection.
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
