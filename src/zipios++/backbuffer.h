#ifndef BACKBUFFER_H
#define BACKBUFFER_H

#include "zipios++/zipios-config.h"

#include <algorithm>

#include "zipios++/meta-iostreams.h"
#include <vector>

#include "zipios++/fcollexceptions.h"
#include "zipios++/ziphead.h"
#include "zipios++/zipheadio.h"
#include "zipios++/virtualseeker.h"
#include "zipios_common.h"

namespace zipios {

using std::ios ;
using std::cerr ;
using std::endl ;

/** A BackBuffer instance is useful for reading the last part of a
    file in an efficient manner, when it is not known exactly how far
    back (towards the front!) to go, to find the start of the desired
    data block.  BackBuffer is a vector< unsigned char > that fills
    itself with data from a file by reading chunks from the end of the
    file progressing towards the start. Upon construction the
    BackBuffer instance is associated with a file and a chunksize can
    be specified. To read a chunk of the file into the BackBuffer call
    readChunk(). */
class BackBuffer : public vector< unsigned char > {
public:
  /** BackBuffer constructor.
      @param is The istream to read the data from. The stream must be seekable,
      as BackBuffer will reposition the file position to read chunks from the back
      of the file.
      @param chunk_size specifies the size of the chunks to read the file into 
      the BackBuffer in.
      @throw FCollException Thrown if the VirtualSeeker vs that has been specified is 
      invalid for the istream is.  */
  inline explicit BackBuffer( istream &is, VirtualSeeker vs = VirtualSeeker(), 
			      int chunk_size = 1024 ) ;
  /** Reads another chunk and returns the size of the chunk that has
      been read. Returns 0 on I/O failure.
      @param read_pointer When a new chunk is read in the already
      stored bytes change position in the BackBuffer. read_pointer is
      assumed by readChunk() to be a pointer into a position in the
      BackBuffer, and is updated to point to the same position in the file
      as it pointed to before the new chunk was read. */
  inline int readChunk( int &read_pointer ) ;

private:
  VirtualSeeker _vs ;
  int _chunk_size ;
  istream &_is ;
  streampos _file_pos ;
  
};

BackBuffer::BackBuffer( istream &is, VirtualSeeker vs, int chunk_size ) 
  : _vs        ( vs         ),
    _chunk_size( chunk_size ),
    _is        ( is         ) 
{
  _vs.vseekg( is, 0, ios::end ) ;
  _file_pos = _vs.vtellg( is ) ;
  // Only happens if _vs.startOffset() is a position
  // in the file that lies after _vs.endOffset(), which
  // is clearly not a valid situation.
  if ( _file_pos < 0 )
    throw FCollException( "Invalid virtual file endings" ) ;
}

int BackBuffer::readChunk( int &read_pointer ) {
  // Update chunk_size and file position
  _chunk_size = min<int> ( static_cast< int >( _file_pos ), _chunk_size ) ;
  _file_pos -= _chunk_size ;
  _vs.vseekg( _is, _file_pos, ios::beg ) ;
  // Make space for _chunk_size new bytes first in buffer
  insert ( begin(), _chunk_size, static_cast< char > ( 0 ) ) ; 
  // Read in the next _chunk_size of bytes

  readByteSeq ( _is, &( (*this)[ 0 ] ), _chunk_size ) ;
  read_pointer += _chunk_size ;

  if ( _is.good() )
    return _chunk_size ;
  else
    return 0 ;
}

}
#endif

/** \file
    The header file for BackBuffer
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
