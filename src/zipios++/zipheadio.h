#ifndef ZIPHEADIO_H
#define ZIPHEADIO_H

#include "zipios-config.h"

#include "meta-iostreams.h"
#include <string>
#include <vector>

#include "ziphead.h"
#include "zipios_defs.h"

namespace zipios {

// byte order conversion functions. 
// ztohs (zip-to-host-short)
#ifdef MY_BIG_ENDIAN

inline uint16 ztohs ( unsigned char *buf ) {
  uint16 out ;
//    *( reinterpret_cast<unsigned char *>( &out )     ) = *( buf  + 1 );
//    *( reinterpret_cast<unsigned char *>( &out ) + 1 ) = *( buf      );
  out = ( static_cast< uint16 >( buf[ 0 ] ) << 8  ) + 
        ( static_cast< uint16 >( buf[ 1 ] )       )  ; 

  return out;
}

// ztohl (zip-to-host-long)
inline uint32 ztohl ( unsigned char *buf ) {
  uint32 out;
  out = ( static_cast< uint32 >( buf[ 0 ] ) << 24 ) +  
        ( static_cast< uint32 >( buf[ 1 ] ) << 16 ) + 
        ( static_cast< uint32 >( buf[ 2 ] ) << 8  ) + 
        ( static_cast< uint32 >( buf[ 3 ] )       )  ; 
  
  return out;
}

#else

inline uint16 ztohs ( unsigned char *buf ) {
  uint16 out ;
  out = ( static_cast< uint16 >( buf[ 1 ] ) << 8  ) + 
        ( static_cast< uint16 >( buf[ 0 ] )       )  ; 
  return out;
}

// ztohl (zip-to-host-long)
inline uint32 ztohl ( unsigned char *buf ) {
  uint32 out;
  out = ( static_cast< uint32 >( buf[ 3 ] ) << 24 ) +  
        ( static_cast< uint32 >( buf[ 2 ] ) << 16 ) + 
        ( static_cast< uint32 >( buf[ 1 ] ) << 8  ) + 
        ( static_cast< uint32 >( buf[ 0 ] )       )  ; 
//    cerr << "buf : " << static_cast< int >( buf[ 0 ] ) ;
//    cerr << " "      << static_cast< int >( buf[ 1 ] ) ;
//    cerr << " "      << static_cast< int >( buf[ 2 ] ) ;
//    cerr << " "      << static_cast< int >( buf[ 3 ] ) << endl ;
//    cerr << "uint32 " << out << endl ;
  return out;
}


#endif

// htozl (host-to-zip-long)
inline uint32 htozl ( unsigned char *buf ) {
  return ztohl( buf ) ;
}

// htozs (host-to-zip-short)
inline uint16 htozs ( unsigned char *buf ) {
  return ztohs( buf ) ;
}


inline uint32 readUint32 ( istream &is ) {
  static const int buf_len = sizeof ( uint32 ) ;
  unsigned char buf [ buf_len ] ;
  int rsf = 0 ;
  // fix endless loop on (almost) empty streams, 20080509 wmayer
  int cnt = 0;
  while ( rsf < buf_len && (cnt++ < buf_len) ) {
    is.read ( reinterpret_cast< char * >( buf ) + rsf, buf_len - rsf ) ;
    rsf += is.gcount () ;
  }
  return  ztohl ( buf ) ;
}

inline void writeUint32 ( uint32 host_val, ostream &os ) {
  uint32 val = htozl( reinterpret_cast< unsigned char * >( &host_val ) ) ;
  os.write( reinterpret_cast< char * >( &val ), sizeof( uint32 ) ) ;
}

inline uint16 readUint16 ( istream &is ) {
  static const int buf_len = sizeof ( uint16 ) ;
  unsigned char buf [ buf_len ] ;
  int rsf = 0 ;
  while ( rsf < buf_len ) {
    is.read ( reinterpret_cast< char * >( buf ) + rsf, buf_len - rsf ) ;
    rsf += is.gcount () ;
  }
  return  ztohs ( buf ) ;
}

inline void writeUint16 ( uint16 host_val, ostream &os ) {
  uint16 val = htozs( reinterpret_cast< unsigned char * >( &host_val ) ) ;
  os.write( reinterpret_cast< char * >( &val ), sizeof( uint16 ) ) ;
}

inline void readByteSeq ( istream &is, string &con, int count ) {
  char *buf = new char [ count + 1 ] ;
  int rsf = 0 ;
  while ( rsf < count && is ) {
    is.read ( buf + rsf, count - rsf ) ;
    rsf += is.gcount() ;
  }
  buf [ count ] = '\0' ;

  con = buf ;
  delete [] buf ;
}

inline void writeByteSeq( ostream &os, const string &con ) {
  os << con ;
}

inline void readByteSeq ( istream &is, unsigned char *buf, int count ) {
  int rsf = 0 ;

  while ( rsf < count && is ) {
    is.read ( reinterpret_cast< char * >( buf ) + rsf, count - rsf ) ;
    rsf += is.gcount() ;
  }
}

inline void writeByteSeq ( ostream &os, const unsigned char *buf, int count ) {
  os.rdbuf()->sputn( reinterpret_cast< const char * >( buf ), count ) ;
}

inline void readByteSeq ( istream &is, vector < unsigned char > &vec, int count ) {
  unsigned char *buf = new unsigned char [ count ] ;
  int rsf = 0 ;
  while ( rsf < count && is ) {
    is.read ( reinterpret_cast< char * >( buf ) + rsf, count - rsf ) ;
    rsf += is.gcount() ;
  }
  
  vec.insert ( vec.end (), buf, buf + count ) ;
  delete [] buf ;
}

inline void writeByteSeq ( ostream &os, const vector < unsigned char > &vec ) {
  if(!vec.empty())
    os.rdbuf()->sputn( reinterpret_cast< const char * >( &( vec[ 0 ] ) ), vec.size() ) ;
}

istream& operator>> ( istream &is, ZipLocalEntry &zlh         ) ;
istream& operator>> ( istream &is, DataDescriptor &dd          ) ;
istream& operator>> ( istream &is, ZipCDirEntry &zcdh           ) ;
//  istream& operator>> ( istream &is, EndOfCentralDirectory &eocd ) ;

ostream &operator<< ( ostream &os, const ZipLocalEntry &zlh ) ;
ostream &operator<< ( ostream &os, const ZipCDirEntry &zcdh ) ;
ostream &operator<< ( ostream &os, const EndOfCentralDirectory &eocd ) ;


} // namespace

#endif

/** \file
    Header file that defines I/O functions for the header structures
    defined in ziphead.h.
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
