#ifndef ZIPOUTPUTSTREAM_H
#define ZIPOUTPUTSTREAM_H

#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"

#include <string>

#include "zipios++/ziphead.h"
#include "zipios++/zipoutputstreambuf.h"

namespace zipios {

/** \anchor ZipOutputStream_anchor
    ZipOutputStream is an ostream that writes the output to a zip file. The
    interface approximates the interface of the Java ZipOutputStream. */
class ZipOutputStream : public std::ostream {
public:

  /** ZipOutputStream constructor.
      @param os ostream to which the compressed zip archive is written.
      @param pos position to reposition the ostream to before reading.  */
  explicit ZipOutputStream( std::ostream &os ) ;

  /** ZipOutputStream constructor.
      @filename filename to write the zip archive to. */
  explicit ZipOutputStream( const std::string &filename ) ;
  
  /** Closes the current entry updates its header with the relevant
      size information and positions the stream write pointer for the
      next entry header. Puts the stream in EOF state. Call
      putNextEntry() to clear the EOF stream state flag. */
  void closeEntry() ;

  /** Calls finish and if the ZipOutputStream was created with a
      filename as a parameter that file is closed as well. If the
      ZipOutputStream was created with an ostream as its first
      parameter nothing but the call to finish happens. */
  void close() ;

  /** Closes the current entry (if one is open), then writes the Zip
      Central Directory Structure closing the ZipOutputStream. The
      output stream that the zip archive is being written to is not
      closed. */
  void finish() ;

  /** \anchor ZipOutputStream_putnextentry_anchor
      Begins writing the next entry.
  */
  void putNextEntry( const ZipCDirEntry &entry ) ;

  /** \anchor ZipOutputStream_putnextentry2_anchor
      Begins writing the next entry.
  */
  void putNextEntry(const std::string& entryName);

  /** Sets the global comment for the Zip archive. */
  void setComment( const std::string& comment ) ;

  /** Sets the compression level to be used for subsequent entries. */
  void setLevel( int level ) ;

  /** Sets the compression method to be used. only STORED and DEFLATED are
      supported. */
  void setMethod( StorageMethod method ) ;

  /** Destructor. */
  virtual ~ZipOutputStream() ;

private:
  std::ofstream *ofs ;
  ZipOutputStreambuf *ozf ;
};
 
} // namespace.

#endif

/** \file 
    Header file that defines ZipOutputStream.
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
