#ifndef GZIPOUTPUTSTREAMBUF_H
#define GZIPOUTPUTSTREAMBUF_H

#include "zipios-config.h"

#include <vector>

#include <zlib.h>

#include "deflateoutputstreambuf.h"

namespace zipios {

/** GZIPOutputStreambuf is a zip output streambuf filter.  */
class BaseExport GZIPOutputStreambuf : public DeflateOutputStreambuf {
public:

  /** GZIPOutputStreambuf constructor. A newly constructed GZIPOutputStreambuf
      is ready to accept data.
      @param outbuf the streambuf to use for output.
      @param del_outbuf if true is specified outbuf will be deleted, when 
      the GZIPOutputStreambuf is destructed.  */
  explicit GZIPOutputStreambuf( streambuf *outbuf, bool del_outbuf = false ) ;

  void setFilename( const string &filename );
  void setComment( const string &comment );

  /** Calls finish. */
  void close() ;

  /** Finishes the compression. */
  void finish() ;

  /** Destructor. */
  virtual ~GZIPOutputStreambuf() ;

protected:
  virtual int overflow( int c = EOF ) ;
  virtual int sync() ;

private:
  void writeHeader();
  void writeTrailer();
  void writeInt(uint32 i);
  
  std::string _filename;
  std::string _comment;
  bool _open ;
};


} // namespace



#endif

/** \file
    Header file that defines ZipOutputStreambuf.
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
