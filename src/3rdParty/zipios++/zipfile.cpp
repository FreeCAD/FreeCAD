
#include "zipios-config.h"

#include "meta-iostreams.h"

#include "fcoll.h"
#include "zipfile.h"
#include "zipinputstream.h"
#include "zipios_defs.h"

#include "backbuffer.h"
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
#include <Base/FileInfo.h>
#endif

namespace zipios {

//
// Public
//

ZipFile ZipFile::openEmbeddedZipFile( const string &name ) {
  // open zipfile, read 4 last bytes close file
  // create ZipFile object.
  ifstream ifs( name.c_str(), ios::in | ios::binary ) ;
  ifs.seekg( -4, ios::end ) ;
  uint32 start_offset = readUint32( ifs ) ;
  ifs.close() ;
  return ZipFile( name, start_offset, 4 ) ; 
}


ZipFile::ZipFile( const string &name , int s_off, int e_off
		  /* , ios::open_mode mode */ ) 
  : _vs( s_off, e_off ) {

  _filename = name ;
  
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
  std::wstring wsname = Base::FileInfo(name).toStdWString();
  ifstream _zipfile( wsname.c_str(), ios::in | ios::binary ) ;
#else
  ifstream _zipfile( name.c_str(), ios::in | ios::binary ) ;
#endif
  init( _zipfile ) ;
}


FileCollection *ZipFile::clone() const {
  return new ZipFile( *this ) ;
}


ZipFile::~ZipFile() {
  close() ;
}

void ZipFile::close() {
  _valid = false ;

}

istream *ZipFile::getInputStream( const ConstEntryPointer &entry ) {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid FileCollection" ) ;
  return getInputStream( entry->getName() ) ;
}

istream *ZipFile::getInputStream( const string &entry_name, 
				  MatchPath matchpath ) {
  if ( ! _valid )
    throw InvalidStateException( "Attempt to use an invalid ZipFile" ) ;

  ConstEntryPointer ent = getEntry( entry_name, matchpath ) ;
  
  if ( !ent )
    return nullptr ;
  else
    return new ZipInputStream( _filename,	
			   static_cast< const ZipCDirEntry * >( ent.get() )->
			   getLocalHeaderOffset() + _vs.startOffset() ) ;
}


//
// Private
//

bool ZipFile::init( istream &_zipfile ) {

  // Check stream error state
  if ( ! _zipfile ) {
    setError ( "Error reading from file" ) ;
    return false ;
  }
  
  _valid = readCentralDirectory( _zipfile ) ;

  return _valid ;
}


bool ZipFile::readCentralDirectory ( istream &_zipfile ) {
  // Find and read eocd. 
  if ( ! readEndOfCentralDirectory( _zipfile ) )
    throw FCollException( "Unable to find zip structure: End-of-central-directory" ) ;

  // Position read pointer to start of first entry in central dir.
  _vs.vseekg( _zipfile,  _eocd.offset(), ios::beg ) ;

  int entry_num = 0 ;
  // Giving the default argument in the next line to keep Visual C++ quiet
  _entries.resize ( _eocd.totalCount(), nullptr ) ;
  while ( ( entry_num < _eocd.totalCount() ) ) {
    ZipCDirEntry *ent = new ZipCDirEntry ; 
    _entries[ entry_num ] = ent ;
    _zipfile >>  *ent ;
    if ( ! _zipfile ) {
      if ( _zipfile.bad()  ) 
	throw IOException( "Error reading zip file while reading zip file central directory" ) ;
      else if ( _zipfile.fail() )
	throw FCollException( "Zip file consistency problem. Failure while reading zip file central directory" ) ;
      else if ( _zipfile.eof()  )
	throw IOException( "Premature end of file while reading zip file central directory" ) ;
    }
    ++entry_num ;
  }

  // Consistency check. eocd should start here
  
  int pos = _vs.vtellg( _zipfile ) ;
  _vs.vseekg( _zipfile, 0, ios::end ) ;
  int remaining = static_cast< int >( _vs.vtellg( _zipfile ) ) - pos ;
  if ( remaining != _eocd.eocdOffSetFromEnd() )
    throw FCollException( "Zip file consistency problem. Zip file data fields are inconsistent with zip file layout" ) ;

  // Consistency check 2, are local headers consistent with
  // cd headers
  if ( ! confirmLocalHeaders( _zipfile ) )
    throw FCollException( "Zip file consistency problem. Zip file data fields are inconsistent with zip file layout" ) ;
  
  return true ;
}


bool ZipFile::readEndOfCentralDirectory ( istream &_zipfile ) {
  BackBuffer bb( _zipfile, _vs ) ;
  int read_p = -1 ;
  bool found = false ;
  while ( ! found ) {
    if ( read_p < 0 )
      if ( ! bb.readChunk ( read_p ) ) {
	found = false ;
	break ;
      }
    if ( _eocd.read( bb, read_p ) ) {
      found = true ;
      break ;
    }
    --read_p ;
  }

  return found ;
}

bool ZipFile::confirmLocalHeaders( istream &_zipfile ) {
  Entries::const_iterator it ;
  ZipCDirEntry *ent ;
  int inconsistencies = 0 ;
  ZipLocalEntry zlh ;
  for ( it = _entries.begin() ; it != _entries.end() ; it++ ) {
    ent = static_cast< ZipCDirEntry * >( (*it).get()  ) ;
    _vs.vseekg( _zipfile, ent->getLocalHeaderOffset(), ios::beg ) ;
    _zipfile >> zlh ;
    if ( ! _zipfile || zlh != *ent ) {
      inconsistencies++ ;
      _zipfile.clear() ;
    }
  }
  return ! inconsistencies ;
}

void ZipFile::setError ( string error_str ) {
  _valid = false ;
#ifdef _USE_EXCEPTIONS
    throw  error_str ; // define exception class instead.
#else
    cerr << error_str << endl ; // define operator<< for exception class if such a class replaces string
#endif
}


}

/** \file
    The implementation of ZipFile.
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
