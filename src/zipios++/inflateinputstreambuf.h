#ifndef INFLATEINPUTSTREAMBUF_H
#define INFLATEINPUTSTREAMBUF_H

#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"
#include <vector>

#include <zlib.h>

#include "zipios++/filterinputstreambuf.h"

namespace zipios {

using std::vector ;

/** InflateInputStreambuf is an input stream filter, that inflates the input
    from the attached input stream. Deflation/Inflation is a
    compression/decompression method used in gzip and zip. The zlib
    library is used to perform the actual inflation, this class only
    wraps the functionality in an input stream filter. */
class InflateInputStreambuf : public FilterInputStreambuf {
public:
  /** InflateInputStreambuf constructor.
      @param inbuf the streambuf to use for input.
      @param s_pos a position to reset the inbuf to before reading. Specify
      -1 to read from the current position.
      @param del_inbuf if true is specified inbuf will be deleted, when 
      the InflateInputStreambuf is destructed.
  */
  explicit InflateInputStreambuf( streambuf *inbuf, int s_pos = -1, bool del_inbuf = false ) ;
  virtual ~InflateInputStreambuf() ;

  /** Resets the zlib stream and purges input and output buffers.
      repositions the input streambuf at stream_position.
      @param stream_position a position to reset the inbuf to before reading. Specify
      -1 to read from the current position.
  */
  bool reset( int stream_position = -1 ) ;
protected:
  virtual int underflow() ;
private:
  z_stream _zs ;
  bool _zs_initialized ;
  const int _invecsize ;
  vector< char > _invec ;
protected: // FIXME: reconsider design?
  const int _outvecsize ;
  vector< char > _outvec ;

private:

  /** Copy-constructor is private to prevent copying. */
  InflateInputStreambuf( const InflateInputStreambuf &src ) ;

  /** Copy-assignment operator is private to prevent copying.  */
  const InflateInputStreambuf &operator= ( const InflateInputStreambuf &src ) ;

};


} // namespace



#endif

/** \file
    Header file that defines InflateInputStreambuf.
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
