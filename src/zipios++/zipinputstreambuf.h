#ifndef ZIPINPUTSTREAMBUF_H
#define ZIPINPUTSTREAMBUF_H

#include "zipios++/zipios-config.h"

#include <vector>

#include <zlib.h>

#include "zipios++/fcoll.h"
#include "zipios++/inflateinputstreambuf.h"
#include "zipios++/ziphead.h"

namespace zipios {

/** ZipInputStreambuf is a zip input streambuf filter.
 */
class ZipInputStreambuf : public InflateInputStreambuf {
public:
  /** ZipInputStreambuf constructor.
      @param inbuf the streambuf to use for input.
      @param s_pos a position to reset the inbuf to before reading. Specify
      -1 to read from the current position.
      @param del_inbuf if true is specified inbuf will be deleted, when 
      the ZipInputStreambuf is destructed.
  */
  explicit ZipInputStreambuf( streambuf *inbuf, int s_pos = -1, bool del_inbuf = false ) ;

  /** Closes the current entry, and positions the stream read pointer at 
      the beginning of the next entry (if there is one). */
  void closeEntry() ;
  /** Closes the streambuf. */
  void close() ;

  /** Opens the next entry in the zip archive and returns a const pointer to a 
      FileEntry object for the entry.
      @return a const FileEntry * containing information about the (now) current 
      entry.
  */
  ConstEntryPointer getNextEntry() ;

  /** Destructor. */
  virtual ~ZipInputStreambuf() ;
protected:
  virtual int underflow() ;
private:
  bool _open_entry ;
  ZipLocalEntry _curr_entry ;
  int _data_start ; // Don't forget entry header has a length too.
  int _remain ; // For STORED entry only. the number of bytes that
  // hasn't been put in the _outvec yet.

  /** Copy-constructor is private to prevent copying. */
  ZipInputStreambuf( const ZipInputStreambuf &src ) ;

  /** Copy-assignment operator is private to prevent copying.  */
  const ZipInputStreambuf &operator= ( const ZipInputStreambuf &src ) ;

};


} // namespace



#endif

/** \file
    Header file that defines ZipInputStreambuf.
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
