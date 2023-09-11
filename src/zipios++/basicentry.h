#ifndef BASICENTRY_H
#define BASICENTRY_H

#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"
#include <string>

#include "zipios++/fcollexceptions.h"
#include "zipios++/fileentry.h"
#include "zipios++/filepath.h"
#include "zipios++/zipios_defs.h"

namespace zipios {

/** BasicEntry is a FileEntry that is suitable as a base class for
    basic entries, that e.g. do not support any form of compression */
class BasicEntry : public FileEntry {
public:
  /** Constructor.
      @param filename the filename of the entry.
      @param comment a comment for the entry.
   */
  explicit BasicEntry( const string &filename, const string &comment,
		       const FilePath &basepath = FilePath() ) ;
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
  
  //     virtual int hashCode() const ;
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
  
  virtual FileEntry *clone() const ;

  virtual ~BasicEntry() ;
protected:
  string _filename ;
  string _comment ;
  int _size ;
  bool _valid ;
  FilePath _basepath ;

};

}
#endif

/** \file
    Header file that defines BasicEntry.
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
