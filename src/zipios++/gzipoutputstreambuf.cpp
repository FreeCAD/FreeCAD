
#include <time.h>

#include "zipios++/zipios-config.h"

#include <algorithm>
#include "zipios++/meta-iostreams.h"

#include <zlib.h>

#include "zipios++/gzipoutputstreambuf.h"

namespace zipios {

using std::ios ;
using std::cerr ;
using std::endl ;

GZIPOutputStreambuf::GZIPOutputStreambuf( streambuf *outbuf, bool del_outbuf )
    : DeflateOutputStreambuf( outbuf, true, del_outbuf ),
      _open      ( false ) 
{
}

void GZIPOutputStreambuf::setFilename( const string &filename ) {
    _filename = filename ;
}

void GZIPOutputStreambuf::setComment( const string &comment ) {
    _comment = comment ;
}

void GZIPOutputStreambuf::close() {
    finish() ;
}

void GZIPOutputStreambuf::finish() {
    if( ! _open )
        return ;
    
    closeStream();  
    writeTrailer();
  
    _open = false ;
}

GZIPOutputStreambuf::~GZIPOutputStreambuf() {
    finish() ;
}

int GZIPOutputStreambuf::overflow( int c ) {
    if (!_open) {
        writeHeader();
        _open = true;
    }
    return DeflateOutputStreambuf::overflow( c ) ;
}

int GZIPOutputStreambuf::sync() {
    return DeflateOutputStreambuf::sync() ;
}

void GZIPOutputStreambuf::writeHeader() {
    unsigned char flg = 0x00;
    flg |= (_filename == "") ? 0x00 : 0x08;
    flg |= (_comment  == "") ? 0x00 : 0x10;

    ostream os( _outbuf ) ;
    os << (unsigned char)0x1f;  // Magic #
    os << (unsigned char)0x8b;  // Magic #
    os << (unsigned char)0x08;  // Deflater.DEFLATED
    os << flg;                  // FLG
    os << (unsigned char)0x00;  // MTIME
    os << (unsigned char)0x00;  // MTIME
    os << (unsigned char)0x00;  // MTIME
    os << (unsigned char)0x00;  // MTIME
    os << (unsigned char)0x00;  // XFLG
    os << (unsigned char)0x00;  // OS
        
    if (_filename != "") {
        os << _filename.c_str();// Filename
        os << (unsigned char)0x00;
    }
    
    if (_comment != "") {
        os << _comment.c_str(); // Comment
        os << (unsigned char)0x00;
    }
}

void GZIPOutputStreambuf::writeTrailer() {
    writeInt(getCrc32());
    writeInt(getCount());
}

void GZIPOutputStreambuf::writeInt(uint32 i) {
    ostream os( _outbuf ) ;
    os << (unsigned char)( i        & 0xFF);
    os << (unsigned char)((i >>  8) & 0xFF);
    os << (unsigned char)((i >> 16) & 0xFF);
    os << (unsigned char)((i >> 24) & 0xFF);
}

} // namespace

/** \file
    Implementation of GZIPOutputStreambuf.
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
