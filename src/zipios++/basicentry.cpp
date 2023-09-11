
#include "zipios++/zipios-config.h"

#include <cassert>

#include "zipios++/meta-iostreams.h"
#include <string>

#include "zipios_common.h"
#include "zipios++/basicentry.h"
#include "zipios++/zipios_defs.h"

#include "outputstringstream.h"

namespace zipios {

using std::ifstream ;
using std::ios ;

//
// Public definitions
//

BasicEntry::BasicEntry( const string &filename, const string &comment,
		       const FilePath &basepath ) 
  : _filename ( filename ),
    _comment  ( comment  ),
    _basepath ( basepath )
{
  string full_path = _basepath + _filename ;
  ifstream is( full_path.c_str(), ios::in | ios::binary ) ;
  if ( ! is ) {
    _valid = false ;
  } else {
    is.seekg( 0, ios::end ) ;
    _size = is.tellg() ;
    is.close() ;
    _valid = true ;
  }
}

string BasicEntry::getComment() const {
  return _comment ;
}

uint32 BasicEntry::getCompressedSize() const {
  return getSize() ;
}

uint32 BasicEntry::getCrc() const {
  return 0 ;
}

vector< unsigned char > BasicEntry::getExtra() const {
  return vector< unsigned char > () ;
}

StorageMethod BasicEntry::getMethod() const {
  return STORED ;
}

string BasicEntry::getName() const {
  return _filename ;
}

string BasicEntry::getFileName() const {
  if ( isDirectory() )
    return string() ;
  string::size_type pos ;
  pos = _filename.find_last_of( separator ) ;
  if ( pos != string::npos ) { // separator found!
    // isDirectory() check means pos should not be last, so pos+1 is ok 
    return _filename.substr(pos + 1) ;
  } else {
    return _filename ;
  }
}

uint32 BasicEntry::getSize() const {
  return _size ;
}

int BasicEntry::getTime() const {
  return 0 ; // FIXME later
}

bool BasicEntry::isValid() const {
  return _valid ;
}

//     virtual int hashCode() const {}
bool BasicEntry::isDirectory() const {
  assert( _filename.size() != 0 ) ;
  return  _filename[ _filename.size() - 1 ] == separator ;
}


void BasicEntry::setComment( const string &comment ) {
  _comment = comment ;
}

void BasicEntry::setCompressedSize( uint32  ) {
}

void BasicEntry::setCrc( uint32  ) {
}

void BasicEntry::setExtra( const vector< unsigned char > & ) {
}

void BasicEntry::setMethod( StorageMethod ) {
}

void BasicEntry::setName( const string &name ) {
  _filename = name ;
}

void BasicEntry::setSize( uint32 size ) {
  _size = size ;
}

void BasicEntry::setTime( int  ) {
}


string BasicEntry::toString() const {
  OutputStringStream sout ;
  sout << _filename << " (" << _size << " bytes)" ;
  return sout.str() ;
}

FileEntry *BasicEntry::clone() const {
  return new BasicEntry( *this ) ;
}

BasicEntry::~BasicEntry() {
}


} // namespace

/** \file
    Implementation of BasicEntry.
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
