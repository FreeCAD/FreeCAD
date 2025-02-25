#ifndef FILEPATH_H
#define FILEPATH_H

#include "zipios-config.h"

#include <stdexcept>
#include <string>

namespace zipios {

using namespace std    ;

/** FilePath represents a path to a file or directory name. FilePath has
    member functions to check if the file path is a valid file system entity,
    and to check what kind of file system entity it is, e.g. is it a file, a 
    directory, a pipe etc.
*/
class FilePath {
public:
  /** Constructor.
      @param path A string representation of the path.
      @param check_exists If true is specified the constructor will
      check the existence and type of the path immediately, instead of
      deferring that task until it is needed. */
  FilePath( const string &path = "", bool check_exists = false ) ;

  inline FilePath &operator= ( const string &rhs ) ;

  inline operator string() const ;

  /** Concatenates FilePath objects. A file separator is inserted
      if appropriate. */
  inline FilePath operator+ ( const FilePath &name ) const ;

  /** Returns filename of the FilePath object by pruning the path
      off. */
  inline FilePath filename() const ;


  /** @return true If the path is a valid file system entity. */
  inline bool exists()         const ;

  /** @return true if the path is a regular file. */
  inline bool isRegular()      const ;

  /** @return true if the path is a directory. */
  inline bool isDirectory()    const ;

  /** @return true if the path is character special (a character
      device file).  */
  inline bool isCharSpecial()  const ;

  /** @return true if the path is block special (a block device
      file). */
  inline bool isBlockSpecial() const ;

  /** @return true if the path is a socket. */
  inline bool isSocket()       const ;

  /** @return true if the path is a Fifo (a pipe). */
  inline bool isFifo()         const ;

protected:

  /** Prunes the trailing separator of a specified path. */
  inline void pruneTrailingSeparator() ;

  /** This function sets _checked to true, stats the path, to see if
  it exists and to determine what type of file it is. All the query
  functions check if _checked is true, and if it isn't they call
  check(). This means stat'ing is deferred until it becomes
  necessary. */
  void check() const ;

  static const char _separator;

  // FIXME: Should be bitfield
  mutable bool   _checked   ;
  mutable bool   _exists    ;
  mutable bool   _is_reg    ;
  mutable bool   _is_dir    ;
  mutable bool   _is_char   ;
  mutable bool   _is_block  ;
  mutable bool   _is_socket ;
  mutable bool   _is_fifo   ;
  string _path              ;
};


//
// Inline member functions
//

FilePath &FilePath::operator= ( const string &rhs ) {
  _path = rhs ;
  pruneTrailingSeparator() ;
  return *this ;
}

void FilePath::pruneTrailingSeparator() {
  if ( _path.size() > 0 )
    if ( _path[ _path.size() -1 ] == _separator )
      _path.erase( _path.size() - 1 ) ; 
}

FilePath::operator string() const { 
  return _path ;
} 


FilePath FilePath::operator+ ( const FilePath &name ) const { 
  if ( _path.size() > 0 )
    return _path + _separator + name._path ; 
  else
    return name._path ;
}


FilePath FilePath::filename() const {
  string::size_type pos ;
  pos = _path.find_last_of( _separator ) ;
  if ( pos != string::npos )
    return _path.substr( pos + 1);
  else 
    return _path ;
}


bool FilePath::exists() const {
  if ( ! _checked )
    check() ;
  return _exists ;
}


bool FilePath::isRegular() const {
  if ( ! _checked )
    check() ;
  return _is_reg ;
}


bool FilePath::isDirectory() const {
  if ( ! _checked )
    check() ;
  return _is_dir ;
}


bool FilePath::isCharSpecial() const {
  if ( ! _checked )
    check() ;
  return _is_char ;
}


bool FilePath::isBlockSpecial() const {
  if ( ! _checked )
    check() ;
  return _is_block ;
}


bool FilePath::isSocket() const {
  if ( ! _checked )
    check() ;
  return _is_socket ;
}


bool FilePath::isFifo() const {
  if ( ! _checked )
    check() ;
  return _is_fifo ;
}


} // namespace
#endif

/** \file
    Header file that defines FilePath.
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
