
#include "zipios-config.h"

#include "meta-iostreams.h"
#include <iterator>
#include <string>

#include "zipios_common.h"
#include "zipheadio.h"

#include "outputstringstream.h"

namespace zipios {

std::istream& operator>> ( std::istream &is, ZipLocalEntry &zlh         ) {
  zlh._valid = false ; // set to true upon successful completion.
  if ( ! is )
    return is ;

//    // Before reading anything we record the position in the stream
//    // This is a field in the central directory entry, but not
//    // in the local entry. After all, we know where we are, anyway.
//    zlh.rel_offset_loc_head  = is.tellg() ;

  if ( zlh.signature != readUint32( is ) ) {
    // put stream in error state and return
    is.setstate ( std::ios::failbit ) ;
    return is ;
  }
  
  zlh.extract_version = readUint16( is ) ;
  zlh.gp_bitfield     = readUint16( is ) ;
  zlh.compress_method = readUint16( is ) ;
  zlh.last_mod_ftime  = readUint16( is ) ;
  zlh.last_mod_fdate  = readUint16( is ) ;
  zlh.crc_32          = readUint32( is ) ;
  zlh.compress_size   = readUint32( is ) ;
  zlh.uncompress_size = readUint32( is ) ;
  zlh.filename_len    = readUint16( is ) ;
  zlh.extra_field_len = readUint16( is ) ;

  // Read filename and extra_field
  readByteSeq( is, zlh.filename, zlh.filename_len ) ;
  readByteSeq( is, zlh.extra_field, zlh.extra_field_len ) ; 

  if ( is )
    zlh._valid = true ;
  return is ;
}


std::istream& operator>> ( std::istream &is, DataDescriptor & ) {
  return is ;
}


std::istream& operator>> ( std::istream &is, ZipCDirEntry &zcdh ) {
  zcdh._valid = false ; // set to true upon successful completion.
  if ( ! is ) 
    return is ;

  if ( zcdh.signature != readUint32( is ) ) {
    // put stream in error state and return
    is.setstate ( std::ios::failbit ) ;
    return is ;
  }
  
  zcdh.writer_version       = readUint16( is ) ;
  zcdh.extract_version      = readUint16( is ) ;
  zcdh.gp_bitfield          = readUint16( is ) ;
  zcdh.compress_method      = readUint16( is ) ;
  zcdh.last_mod_ftime       = readUint16( is ) ;
  zcdh.last_mod_fdate       = readUint16( is ) ;
  zcdh.crc_32               = readUint32( is ) ;
  zcdh.compress_size        = readUint32( is ) ;
  zcdh.uncompress_size      = readUint32( is ) ;
  zcdh.filename_len         = readUint16( is ) ;
  zcdh.extra_field_len      = readUint16( is ) ;
  zcdh.file_comment_len     = readUint16( is ) ; 
  zcdh.disk_num_start       = readUint16( is ) ;
  zcdh.intern_file_attr     = readUint16( is ) ;
  zcdh.extern_file_attr     = readUint32( is ) ;
  zcdh.rel_offset_loc_head  = readUint32( is ) ;

  // Read filename and extra_field
  readByteSeq( is, zcdh.filename, zcdh.filename_len ) ;
  readByteSeq( is, zcdh.extra_field, zcdh.extra_field_len ) ; 
  readByteSeq( is, zcdh.file_comment, zcdh.file_comment_len ) ;

  if ( is )
    zcdh._valid = true ;
  return is ;
}

std::ostream &operator<< ( std::ostream &os, const ZipLocalEntry &zlh ) {
  if ( ! os )
    return os ;

  writeUint32( zlh.signature      , os ) ;
  writeUint16( zlh.extract_version, os ) ;
  writeUint16( zlh.gp_bitfield    , os ) ;
  writeUint16( zlh.compress_method, os ) ;
  writeUint16( zlh.last_mod_ftime , os ) ;
  writeUint16( zlh.last_mod_fdate , os ) ;
  writeUint32( zlh.crc_32         , os ) ;
  writeUint32( zlh.compress_size  , os ) ;
  writeUint32( zlh.uncompress_size, os ) ;
  writeUint16( zlh.filename_len   , os ) ;
  writeUint16( zlh.extra_field_len, os ) ;
 

  // Write filename and extra_field
  writeByteSeq( os, zlh.filename ) ;
  writeByteSeq( os, zlh.extra_field ) ; 

  return os ;
}

std::ostream &operator<< ( std::ostream &os, const ZipCDirEntry &zcdh ) {
  if ( ! os ) 
    return os ;

  writeUint32( zcdh.signature          , os ) ;
  writeUint16( zcdh.writer_version     , os ) ;
  writeUint16( zcdh.extract_version    , os ) ;
  writeUint16( zcdh.gp_bitfield        , os ) ;
  writeUint16( zcdh.compress_method    , os ) ;
  writeUint16( zcdh.last_mod_ftime     , os ) ;
  writeUint16( zcdh.last_mod_fdate     , os ) ;
  writeUint32( zcdh.crc_32             , os ) ;
  writeUint32( zcdh.compress_size      , os ) ;
  writeUint32( zcdh.uncompress_size    , os ) ;
  writeUint16( zcdh.filename_len       , os ) ;
  writeUint16( zcdh.extra_field_len    , os ) ;
  writeUint16( zcdh.file_comment_len   , os ) ;
  writeUint16( zcdh.disk_num_start     , os ) ;
  writeUint16( zcdh.intern_file_attr   , os ) ;
  writeUint32( zcdh.extern_file_attr   , os ) ;
  writeUint32( zcdh.rel_offset_loc_head, os ) ;

  // Write filename and extra_field
  writeByteSeq( os, zcdh.filename ) ;
  writeByteSeq( os, zcdh.extra_field ) ; 
  writeByteSeq( os, zcdh.file_comment ) ;

  return os ;
}

std::ostream &operator<< ( std::ostream &os, const EndOfCentralDirectory &eocd ) {
  if ( ! os ) 
    return os ;

  writeUint32( eocd.signature       , os ) ;
  writeUint16( eocd.disk_num        , os ) ;
  writeUint16( eocd.cdir_disk_num   , os ) ;
  writeUint16( eocd.cdir_entries    , os ) ;
  writeUint16( eocd.cdir_tot_entries, os ) ;
  writeUint32( eocd.cdir_size       , os ) ;
  writeUint32( eocd.cdir_offset     , os ) ;
  writeUint16( eocd.zip_comment_len , os ) ;
  
  writeByteSeq( os, eocd.zip_comment ) ;

  return os ;
}



} // namespace



/** \file
    Implementation of I/O functions for the header structures
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
