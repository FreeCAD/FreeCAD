
#include "zipios-config.h"

#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include "filepath.h"

namespace zipios {

using namespace std ;

const char FilePath::_separator = '/' ;


FilePath::FilePath( const string &path, bool check_exists )
  : _checked( false ),
    _path( path ) {
  pruneTrailingSeparator() ;
  if ( check_exists ) 
    exists() ;
}


void FilePath::check() const {
  _checked     = true  ;  
  _exists      = false ;
  _is_reg      = false ;
  _is_dir      = false ;
  _is_char     = false ; 
  _is_block    = false ;
  _is_socket   = false ;
  _is_fifo     = false ;

#if defined (__GNUC__)
  struct stat buf ;
  if ( stat( _path.c_str(), &buf ) != -1 ) {
#else
  struct _stat buf ;
  if ( _stat( _path.c_str(), &buf ) != -1 ) {
#endif
    _exists    = true ;
#if defined(BOOST_WINNT)
    _is_reg    = _S_IFREG & buf.st_mode ;
    _is_dir    = _S_IFDIR & buf.st_mode ;
    _is_char   = _S_IFCHR & buf.st_mode ;
#else
    _is_reg    = S_ISREG ( buf.st_mode ) ;
    _is_dir    = S_ISDIR ( buf.st_mode ) ;
    _is_char   = S_ISCHR ( buf.st_mode ) ;
    _is_block  = S_ISBLK ( buf.st_mode ) ;
    _is_socket = S_ISSOCK( buf.st_mode ) ;
    _is_fifo   = S_ISFIFO( buf.st_mode ) ;
#endif
  } 
}

} // namespace

/** \file
    Implementation of FilePath.
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
