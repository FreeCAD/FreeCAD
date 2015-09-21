#ifndef ZIPFILE_H
#define ZIPFILE_H

#include "zipios-config.h"

#include <vector>
#include "meta-iostreams.h"

#include "fcoll.h"
#include "ziphead.h"
#include "virtualseeker.h"

namespace zipios {

using std::ifstream ;

/** \anchor zipfile_anchor
    ZipFile is a FileCollection, where the files are stored
 in a .zip file.  */
class BaseExport ZipFile : public FileCollection {
public:
  /** \anchor zipfile_openembeddedzipfile
      Opens a Zip archive embedded in another file, by writing the zip
      archive to the end of the file followed by the start offset of
      the zip file. The offset must be written in zip-file byte-order
      (little endian). The program appendzip, which is part of the
      Zipios++ distribution can be used to append a Zip archive to a
      file, e.g. a binary program. 
      @throw FColException Thrown if the specified file name is not a valid zip 
      archive.
      @throw IOException Thrown if an I/O problem is encountered, while the directory
      of the specified zip archive is being read. */
  static ZipFile openEmbeddedZipFile( const string &name ) ;

  /** Default constructor.
   */
  ZipFile() {}

  /* Default Copy constructor and copy assignment operator are sufficient. */

  /** Constructor. Opens the zip file name. If the zip "file" is
      embedded in a file that contains other data, e.g. a binary
      program, the offset of the zip file start and end must be
      specified. 
      @param name The filename of the zip file to open.
      @param s_off Offset relative to the start of the file, that 
      indicates the beginning of the zip file.
      @param e_off Offset relative to the end of the file, that
      indicates the end of the zip file. The offset is a positive number,
      even though the offset is towards the beginning of the file.
      @throw FColException Thrown if the specified file name is not a valid zip 
      archive.
      @throw IOException Thrown if an I/O problem is encountered, while the directory
      of the specified zip archive is being read. */
  explicit ZipFile( const string &name, int s_off = 0, int e_off = 0
		    /* , ios::open_mode mode  = ios::in | ios::binary */ ) ;

  virtual FileCollection *clone() const ;

  /** Destructor. */
  virtual ~ZipFile() ;

  virtual void close() ;

  virtual istream *getInputStream( const ConstEntryPointer &entry ) ;
  virtual istream *getInputStream( const string &entry_name, 
				     MatchPath matchpath = MATCH ) ;
private:
  VirtualSeeker _vs ;
  EndOfCentralDirectory  _eocd ;

  bool init( istream &_zipfile ) ;
  bool readCentralDirectory( istream &_zipfile ) ;
  bool readEndOfCentralDirectory( istream &_zipfile ) ;
  bool confirmLocalHeaders( istream &_zipfile ) ;
  void setError( string error_str ) ;
};

}

#endif

/** \file
    Header file that defines ZipFile.
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
