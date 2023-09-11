#ifndef ZIPHEAD_H
#define ZIPHEAD_H

#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"
#include <string>
#include <vector>

#include "zipios++/fileentry.h"
#include "zipios++/zipios_defs.h"

namespace zipios {

using std::streampos ;

class ZipCDirEntry ;

/** A concrete implementation of the abstract FileEntry base class for
 ZipFile entries, specifically for representing the information
 present in the local headers of file entries in a zip file. */
class ZipLocalEntry : public FileEntry {
  friend istream &operator>> ( istream &is, ZipLocalEntry &zcdh ) ;
  friend ostream &operator<< ( ostream &os, const ZipLocalEntry &zlh ) ;
  friend bool operator== ( const ZipLocalEntry &zlh, const ZipCDirEntry &ze ) ;
public:
  inline ZipLocalEntry( const string &_filename = "",
			const vector< unsigned char > &_extra_field = 
			vector< unsigned char >() ) 
    : gp_bitfield( 0 ),
      _valid( false ) { 
    setDefaultExtract() ;
    setName( _filename ) ;
    setExtra( _extra_field ) ; 
  }

  void setDefaultExtract() ;
  inline ZipLocalEntry &operator=( const class ZipLocalEntry &src ) ;
  virtual string getComment() const ;
  virtual uint32 getCompressedSize() const ;
  virtual uint32 getCrc() const ;
  virtual vector< unsigned char > getExtra() const ;
  virtual StorageMethod getMethod() const ;
  virtual string getName() const ;
  virtual string getFileName() const ;
  virtual uint32 getSize() const ;
  virtual int getTime() const ;
  virtual bool isValid() const ;
  
  virtual bool isDirectory() const ;
  
  virtual void setComment( const string &comment ) ;
  virtual void setCompressedSize( uint32 size ) ;
  virtual void setCrc( uint32 crc ) ;
  virtual void setExtra( const vector< unsigned char > &extra ) ;
  virtual void setMethod( StorageMethod method ) ;
  virtual void setName( const string &name ) ;
  virtual void setSize( uint32 size ) ;
  virtual void setTime( int time ) ;
  
  virtual string toString() const ;

  int getLocalHeaderSize() const ;

  bool trailingDataDescriptor() const ;

  virtual FileEntry *clone() const ;

  virtual ~ZipLocalEntry() {}
protected:
  static const uint32 signature ;
  uint16 extract_version ;
  uint16 gp_bitfield     ;
  uint16 compress_method ;
  uint16 last_mod_ftime  ;
  uint16 last_mod_fdate  ;
  uint32 crc_32          ;
  uint32 compress_size   ;
  uint32 uncompress_size ;
  uint16 filename_len    ;
  uint16 extra_field_len ;

  string filename ;
  vector< unsigned char > extra_field ; 

  bool _valid ;
};

/** A struct containing fields for the entries in a zip file data
 descriptor, that trails the compressed data in files that were
 created by streaming, ie where the zip compressor cannot seek back
 to the local header and store the data. */
struct DataDescriptor {
  uint32 crc_32          ;
  uint32 compress_size   ;
  uint32 uncompress_size ;
};

/** Specialization of ZipLocalEntry, that add fields for storing the
    extra information, that is only present in the entries in the zip
    central directory and not in the local entry headers. */
class ZipCDirEntry : public ZipLocalEntry {
friend istream &operator>> ( istream &is, ZipCDirEntry &zcdh ) ;
friend ostream &operator<< ( ostream &os, const ZipCDirEntry &zcdh ) ;
friend bool operator== ( const ZipLocalEntry &zlh, const ZipCDirEntry &ze ) ;
public:

  inline ZipCDirEntry(  const string &_filename = "",
			const string &_file_comment = "",
			const vector< unsigned char > &_extra_field = 
			                vector< unsigned char >() ) 
    : ZipLocalEntry   ( _filename, _extra_field ),
      disk_num_start  ( 0x0 ),
      intern_file_attr( 0x0 ),
      extern_file_attr( 0x81B40000 ) 
    // FIXME: I don't understand the external mapping, simply
    // copied value for a file with -rw-rw-r-- permissions
    // compressed with info-zip
  { 
    setComment( _file_comment ) ;
    setDefaultWriter() ;
  } 

  void setDefaultWriter() ;

  inline ZipCDirEntry &operator=( const class ZipCDirEntry &src ) ;
  virtual string toString() const ;

  virtual string getComment() const ;

  virtual void setComment( const string &comment ) ;

  virtual uint32 getLocalHeaderOffset() const ;
  virtual void   setLocalHeaderOffset( uint32 offset ) ;

  int getCDirHeaderSize() const ;

  virtual FileEntry *clone() const ;

  virtual ~ZipCDirEntry() {}
private:
  static const uint32 signature ;
  uint16 writer_version      ;

  uint16 file_comment_len    ; 
  uint16 disk_num_start      ;
  uint16 intern_file_attr    ;
  uint32 extern_file_attr    ;

  uint32 rel_offset_loc_head ;

  string file_comment ;
};

/** The end of the Central directory structure. This structure is
    stored in the end of the zipfile, and contains information about
    the zipfile, including the position of the start of the central
    directory. */
class EndOfCentralDirectory {
  friend ostream &operator<< ( ostream &os, const EndOfCentralDirectory &eocd ) ;
public:
  explicit EndOfCentralDirectory( const string &_zip_comment = "", 
				  uint16 _disk_num = 0, uint16 _cdir_disk_num = 0, 
				  uint16 _cdir_entries = 0, 
				  uint16 _cdir_tot_entries = 0, 
				  uint32 _cdir_size = 0, uint32 _cdir_offset = 0 )
    :   disk_num         ( _disk_num           ),
	cdir_disk_num    ( _cdir_disk_num      ),
	cdir_entries     ( _cdir_entries       ),
	cdir_tot_entries ( _cdir_tot_entries   ),
	cdir_size        ( _cdir_size          ),
	cdir_offset      ( _cdir_offset        ),
	zip_comment_len  ( _zip_comment.size() ),
	zip_comment      ( _zip_comment        )  {}

  uint32    offset() const          { return cdir_offset ;          }
  uint16    totalCount() const      { return cdir_tot_entries ;     }
  void setCDirSize( uint32 size )   { cdir_size = size ;            }
  void setOffset( uint32 offset )   { cdir_offset = offset ;        }

  void setTotalCount( uint16 c )    { cdir_entries = c ; cdir_tot_entries = c ; }
  int  eocdOffSetFromEnd() const { return eocd_offset_from_end ; }
  bool read( vector<unsigned char> &buf, int pos ) ;
private:
  static const uint32 signature;
  uint16 disk_num         ;
  uint16 cdir_disk_num    ;
  uint16 cdir_entries     ;
  uint16 cdir_tot_entries ;
  uint32 cdir_size        ;
  uint32 cdir_offset      ;
  uint16 zip_comment_len  ;
  
  streampos eocd_offset_from_end ; // Not a Zip defined field
  string zip_comment;
  bool checkSignature( unsigned char *buf ) const ;
  inline bool checkSignature( uint32 sig ) const ;
};


bool operator== ( const ZipLocalEntry &zlh, const ZipCDirEntry &ze ) ;
inline bool operator== ( const ZipCDirEntry &ze, const ZipLocalEntry &zlh ) {
  return zlh == ze ;
}
inline bool operator!= ( const ZipLocalEntry &zlh, const ZipCDirEntry &ze ) {
  return ! ( zlh == ze ) ;
}
inline bool operator!= ( const ZipCDirEntry &ze, const ZipLocalEntry &zlh ) {
  return ! ( zlh == ze ) ;
}

// Inline member functions

ZipCDirEntry &ZipCDirEntry::operator=( const class ZipCDirEntry &src ) {
  writer_version      = src.writer_version      ;
  extract_version     = src.extract_version     ;
  gp_bitfield         = src.gp_bitfield         ;
  compress_method     = src.compress_method     ;
  last_mod_ftime      = src.last_mod_ftime      ;
  last_mod_fdate      = src.last_mod_fdate      ;
  crc_32              = src.crc_32              ;
  compress_size       = src.compress_size       ; 
  uncompress_size     = src.uncompress_size     ;
  filename_len        = src.filename_len        ;
  extra_field_len     = src.extra_field_len     ;
  file_comment_len    = src.file_comment_len    ; 
  disk_num_start      = src.disk_num_start      ;
  intern_file_attr    = src.intern_file_attr    ;
  extern_file_attr    = src.extern_file_attr    ;
  rel_offset_loc_head = src.rel_offset_loc_head ;

  filename     = src.filename     ;
  extra_field  = src.extra_field  ; 
  file_comment = src.file_comment ;

  return *this ;
}

bool EndOfCentralDirectory::checkSignature ( uint32 sig ) const {
  return signature == sig ;
}


} // namespace

#endif


/** \file
    Header file containing classes and functions for reading the central
    directory and local header fields in a zip archive.
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
