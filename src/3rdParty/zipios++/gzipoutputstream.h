#ifndef GZIPOUTPUTSTREAM_H
#define GZIPOUTPUTSTREAM_H

#include "zipios-config.h"

#include "meta-iostreams.h"

#include <string>

#include "gzipoutputstreambuf.h"

namespace zipios {

/** \anchor GZIPOutputStream_anchor
    GZIPOutputStream is an ostream that writes the output to a gz file. The
    interface approximates the interface of the Java GZIPOutputStream. */
class BaseExport GZIPOutputStream : public std::ostream {
public:

  /** GZIPOutputStream constructor.
      @param os ostream to which the compressed zip archive is written.
    */
  explicit GZIPOutputStream( std::ostream &os ) ;

  /** GZIPOutputStream constructor.
      @param filename filename to write the gzip archive to.
   */
  explicit GZIPOutputStream( const std::string &filename ) ;

  void setFilename( const string &filename );
  void setComment( const string &comment );
  
  /** Calls finish and closes the stream. */
  void close() ;

  /** Finishes the stream. */
  void finish() ;

  /** Destructor. */
  virtual ~GZIPOutputStream() ;

private:
  std::ofstream *ofs ;
  GZIPOutputStreambuf *ozf ;
};
 
} // namespace.

#endif

/** \file 
    Header file that defines GZIPOutputStream.
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
