
#include "zipios-config.h"

#include "meta-iostreams.h"

#include <zlib.h>

#include "fcollexceptions.h"
#include "deflateoutputstreambuf.h"

#include "outputstringstream.h"

namespace zipios {

using std::cerr ;
using std::endl ;

DeflateOutputStreambuf::DeflateOutputStreambuf( streambuf *outbuf, bool user_init, 
						bool del_outbuf ) 
  : FilterOutputStreambuf( outbuf, del_outbuf ),
    _zs_initialized ( false            ),
    _invecsize      ( 1000             ),
    _invec          ( _invecsize       ),
    _outvecsize     ( 1000             ),
    _outvec         ( _outvecsize      )
{
  // NOTICE: It is important that this constructor and the methods it
  // calls doesn't do anything with the output streambuf _outbuf The
  // reason is that this class can be subclassed, and the subclass
  // should get a chance to write to the buffer first

  // zlib init:
  _zs.zalloc = Z_NULL ;
  _zs.zfree  = Z_NULL ;
  _zs.opaque = Z_NULL ;

  if ( user_init && ! init() )
    cerr << "DeflateOutputStreambuf::reset() failed!\n" ; // FIXME: throw something

}


DeflateOutputStreambuf::~DeflateOutputStreambuf() {
  closeStream() ;
}


// This method is called in the constructor, so it must not write
// anything to the output streambuf _outbuf (see notice in
// constructor)
bool DeflateOutputStreambuf::init( int comp_level ) {
  static const int default_mem_level = 8 ;

  // _zs.next_in and avail_in must be set according to
  // zlib.h (inline doc).
  _zs.next_in  = reinterpret_cast< unsigned char * >( &( _invec[ 0 ] ) ) ;
  _zs.avail_in = 0 ;

  _zs.next_out  = reinterpret_cast< unsigned char * >( &( _outvec[ 0 ] ) ) ;
  _zs.avail_out = _outvecsize ;

  int err ;
  if( _zs_initialized ) {                    // just reset it
    endDeflation() ;
    err = deflateReset( &_zs ) ;
    // FIXME: bug, for deflateReset we do not update the compression level
  } else {                                   // init it
    err = deflateInit2( &_zs, comp_level, Z_DEFLATED, -MAX_WBITS, 
			default_mem_level, Z_DEFAULT_STRATEGY ) ;
    /* windowBits is passed < 0 to tell that no zlib header should be
       written. */
    _zs_initialized = true ;
  }

  // streambuf init:
  setp( &( _invec[ 0 ] ), &( _invec[ 0 ] ) + _invecsize ) ;

  _crc32 = crc32( 0, Z_NULL, 0 ) ;
  _overflown_bytes = 0 ;

  if ( err == Z_OK )
    return true ;
  else
    return false ;
}


bool DeflateOutputStreambuf::closeStream() {
  int err = Z_OK ;
  if( _zs_initialized ) {
    endDeflation() ;
    err = deflateEnd( &_zs ) ;
    _zs_initialized = false ;
  }
  
  if ( err == Z_OK )
    return true ;
  else {
    cerr << "DeflateOutputStreambuf::closeStream(): deflateEnd failed" ;
#ifdef HAVE_ZERROR
    cerr << ": " << zError( err ) ;
#endif
    cerr << endl ;
    return false ;
  }
}


int DeflateOutputStreambuf::overflow( int c ) {
  _zs.avail_in = pptr() - pbase() ;
  _zs.next_in = reinterpret_cast< unsigned char * >( &( _invec[ 0 ] ) ) ;

  _crc32 = crc32( _crc32, _zs.next_in, _zs.avail_in ) ; // update crc32
  _overflown_bytes += _zs.avail_in ;

  _zs.next_out  = reinterpret_cast< unsigned char * >( &( _outvec[ 0 ] ) ) ;
  _zs.avail_out = _outvecsize ;

  // Deflate until _invec is empty.
  int err = Z_OK ;
  while ( ( _zs.avail_in > 0 || _zs.avail_out == 0 ) && err == Z_OK ) {
    if ( _zs.avail_out == 0 )
      flushOutvec() ;

    err = deflate( &_zs, Z_NO_FLUSH ) ;
  }

  flushOutvec() ;
  
  // Update 'put' pointers
  setp( &( _invec[ 0 ] ), &( _invec[ 0 ] ) + _invecsize ) ;
  
  if( err != Z_OK && err != Z_STREAM_END ) {
#if defined (HAVE_STD_IOSTREAM) && defined (USE_STD_IOSTREAM)
    // Throw an exception to make istream set badbit
    OutputStringStream msgs ;
    msgs << "Deflation failed" ;
#ifdef HAVE_ZERROR
    msgs << ": " << zError( err ) ;
#endif
    throw IOException( msgs.str() ) ;
#endif
    cerr << "Deflation failed\n" ;
    return EOF ;
  }

  if ( c != EOF ) {
    *pptr() = c ;
    pbump( 1 ) ;
  }

  return 0 ;
}

int DeflateOutputStreambuf::sync() {
  // FIXME: Do something
//    return overflow() ;
  return 0 ;
}


bool DeflateOutputStreambuf::flushOutvec() {
  int deflated_bytes = _outvecsize - _zs.avail_out ;
  int bc = _outbuf->sputn( &( _outvec[ 0 ] ), deflated_bytes ) ;

  _zs.next_out = reinterpret_cast< unsigned char * >( &( _outvec[ 0 ] ) ) ;
  _zs.avail_out = _outvecsize ;

  return deflated_bytes == bc ;
}


void DeflateOutputStreambuf::endDeflation() {
  overflow() ;

  _zs.next_out  = reinterpret_cast< unsigned char * >( &( _outvec[ 0 ] ) ) ;
  _zs.avail_out = _outvecsize ;

  // Deflate until _invec is empty.
  int err = Z_OK ;

  while ( err == Z_OK ) {
    if ( _zs.avail_out == 0 )
      flushOutvec() ;

    err = deflate( &_zs, Z_FINISH ) ;
  }

  flushOutvec() ;

  if ( err != Z_STREAM_END ) {
    cerr << "DeflateOutputStreambuf::endDeflation(): deflation failed:\n" ;
#ifdef HAVE_ZERROR
    cerr << ": " << zError( err ) ;
#endif
    cerr << endl ;
  }
}


} // namespace

/** \file
    Implementation of DeflateOutputStreambuf.
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
