#ifndef ZIPINPUTSTREAM_H
#define ZIPINPUTSTREAM_H

#include "zipios-config.h"

#include "meta-iostreams.h"
#include <string>

#include "ziphead.h"
#include "zipinputstreambuf.h"

namespace zipios {

using std::ifstream ;

/** \anchor ZipInputStream_anchor
    ZipInputStream is an istream that gets it's input from a zip file. The
    interface approximates the interface of the Java
    ZipInputStream. */
class BaseExport ZipInputStream : public istream {
public:

  /** ZipInputStream constructor.
      @param is istream from which the compressed zip archive can be read.
      @param pos position to reposition the istream to before reading.  */
  explicit ZipInputStream( std::istream &is, std::streampos pos = 0 ) ;

  /** ZipInputStream constructor.
      @param filename filename of a valid zip file.
      @param pos position to reposition the istream to before reading.
   */
  explicit ZipInputStream( const std::string &filename, std::streampos pos = 0 ) ;
  
  int available() ;
  /** Closes the current entry, and positions the stream read pointer at 
      the beginning of the next entry (if there is one). */
  void closeEntry() ;

  /** Closes the istream. */
  void close() ;

//    ZipLocalEntry *createZipCDirEntry( const string &name ) ;

  /** \anchor ZipInputStream_getnextentry_anchor
      Opens the next entry in the zip archive and returns a const pointer to a 
      FileEntry object for the entry.
      @return a const FileEntry * containing information about the (now) current 
      entry.
  */
  ConstEntryPointer getNextEntry() ;

  /** Destructor. */
  virtual ~ZipInputStream() ;

private:
  ifstream *ifs ;
  ZipInputStreambuf *izf ;

  /** Copy-constructor is private to prevent copying. */
  ZipInputStream( const ZipInputStream &src ) ;

  /** Copy-assignment operator is private to prevent copying.  */
  const ZipInputStream &operator= ( const ZipInputStream &src ) ;

};
 
} // namespace.

#endif

/** \file 
    Header file that defines ZipInputStream.
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
