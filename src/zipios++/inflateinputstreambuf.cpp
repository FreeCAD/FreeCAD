
#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"

#include <zlib.h>

#include "zipios++/fcollexceptions.h"
#include "zipios++/inflateinputstreambuf.h"

#include "outputstringstream.h"

namespace zipios {

using std::cerr ;
using std::endl ;

InflateInputStreambuf::InflateInputStreambuf( streambuf *inbuf, int s_pos, bool del_inbuf ) 
  : FilterInputStreambuf( inbuf, del_inbuf ),
    _zs_initialized ( false            ),
    _invecsize      ( 1000             ),
    _invec          ( _invecsize       ),
    _outvecsize     ( 1000             ),
    _outvec         ( _outvecsize      )
{
  // NOTICE: It is important that this constructor and the methods it
  // calls doesn't do anything with the input streambuf _inbuf, other
  // than repositioning it to the specified position. The reason is
  // that this class can be subclassed, and the subclass should get a
  // chance to read from the buffer first)

  // zlib init:
  _zs.zalloc = Z_NULL ;
  _zs.zfree  = Z_NULL ;
  _zs.opaque = Z_NULL ;

  reset( s_pos ) ;
  // We're not checking the return value of reset() and throwing
  // an exception in case of an error, because we cannot catch the exception
  // in the constructors of subclasses with all compilers.
}

InflateInputStreambuf::~InflateInputStreambuf() {
  // Dealloc z_stream stuff
  int err = inflateEnd( &_zs ) ;
  if( err != Z_OK ) {
    cerr << "~inflatebuf: inflateEnd failed" ;
#ifdef HAVE_ZERROR
    cerr << ": " << zError( err ) ;
#endif
    cerr << endl ;
  }
}


int InflateInputStreambuf::underflow() {
  // If not underflow don't fill buffer
  if ( gptr() < egptr() )
    return static_cast< unsigned char >( *gptr() ) ;

  // Prepare _outvec and get array pointers
  _zs.avail_out = _outvecsize ; 
  _zs.next_out  = reinterpret_cast< unsigned char * >( &( _outvec[ 0 ] ) ) ;

  // Inflate until _outvec is full
  // eof (or I/O prob) on _inbuf will break out of loop too.
  int err = Z_OK ;
  while ( _zs.avail_out > 0 && err == Z_OK ) {
    if ( _zs.avail_in == 0 ) { // fill _invec
      int bc = _inbuf->sgetn( &(_invec[ 0 ] ) , 
			      _invecsize ) ;
      // FIXME: handle i/o problems.
      _zs.next_in  = reinterpret_cast< unsigned char * >( &( _invec[0] ) ) ;
      _zs.avail_in = bc ;
      // If we could not read any new data (bc == 0) and inflate isn't
      // done it will return Z_BUF_ERROR and thus breaks out of the
      // loop. This means we don't have to respond to the situation
      // where we can't read more bytes here.
    }

    err = inflate( &_zs, Z_NO_FLUSH ) ;
  }
  // Normally the number of inflated bytes will be the
  // full length of the output buffer, but if we can't read
  // more input from the _inbuf streambuf, we end up with
  // less.
  int inflated_bytes = _outvecsize - _zs.avail_out ;
  setg( &( _outvec[ 0 ] ),
	&( _outvec[ 0 ] ),
	&( _outvec[ 0 ] ) + inflated_bytes ) ;
  // FIXME: look at the error returned from inflate here, if there is
  // some way to report it to the InflateInputStreambuf user.
  // Until I find out I'll just print a warning to stdout.
  if( err != Z_OK && err != Z_STREAM_END ) {
#if defined (HAVE_STD_IOSTREAM) && defined (USE_STD_IOSTREAM)
    // Throw an exception to make istream set badbit
    OutputStringStream msgs ;
    msgs << "InflateInputStreambuf: inflate failed" ;
#ifdef HAVE_ZERROR
    msgs << ": " << zError( err ) ;
#endif
    throw IOException( msgs.str() ) ;
#endif
    // If HAVE_STD_IOSTREAM not defined we just return eof
    // if no output is produced, and that happens anyway
  }
  if (inflated_bytes > 0 )
    return static_cast< unsigned char >( *gptr() ) ;
  else 
    return EOF ; // traits_type::eof() ;
}



// This method is called in the constructor, so it must not
// read anything from the input streambuf _inbuf (see notice in constructor)
bool InflateInputStreambuf::reset( int stream_position ) {
  if ( stream_position >= 0 ) { // reposition _inbuf
    _inbuf->pubseekpos( stream_position ) ;
  }

  // _zs.next_in and avail_in must be set according to
  // zlib.h (inline doc).
  _zs.next_in  = reinterpret_cast< unsigned char * >( &( _invec[0] ) ) ;
  _zs.avail_in = 0 ;
  
  int err ;
  if( _zs_initialized ) {                    // just reset it
    err = inflateReset( &_zs ) ;
  } else {                                   // init it
    err = inflateInit2( &_zs, -MAX_WBITS ) ;
    /* windowBits is passed < 0 to tell that there is no zlib header.
     Note that in this case inflate *requires* an extra "dummy" byte
     after the compressed stream in order to complete decompression
     and return Z_STREAM_END.  We always have an extra "dummy" byte,
     because there is always some trailing data after the compressed
     data (either the next entry or the central directory.  */
    _zs_initialized = true ;
  }

  // streambuf init:
  // The important thing here, is that 
  // - the pointers are not NULL (which would mean unbuffered)
  // - and that gptr() is not less than  egptr() (so we trigger underflow
  //   the first time data is read).
  setg( &( _outvec[ 0 ] ),
	&( _outvec[ 0 ] ) + _outvecsize,
	&( _outvec[ 0 ] ) + _outvecsize ) ;

  if ( err == Z_OK )
    return true ;
  else
    return false ;
}

} // namespace

/** \file
    Implementation of InflateInputStreambuf.
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
