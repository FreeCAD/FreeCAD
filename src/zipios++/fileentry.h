#ifndef FILEENTRY_H
#define FILEENTRY_H

#include "zipios++/zipios-config.h"

#include <string>
#include <vector>
#include "zipios++/meta-iostreams.h"

#include "zipios++/simplesmartptr.h"
#include "zipios++/zipios_defs.h"

namespace zipios {

using std::vector ;
using std::ostream ;
using std::istream ;
using std::string ;

/** The types used with FileEntry::setMethod and
    FileEntry::getMethod. The current entries are the types supported
    by the zip format. The numbering also matches the numbering used
    in the zip file format, ie STORED is indicated by a 0 in the
    method field in a zip file and so on. */
enum StorageMethod { STORED = 0, SHRUNK, REDUCED1, REDUCED2,
		     REDUCED3, REDUCED4, IMPLODED, RESERVED,
		     DEFLATED } ;

class FileEntry ;

/** \typedef typedef SimpleSmartPointer< FileEntry > EntryPointer
    EntryPointer is a SimpleSmartPointer for FileEntry's.  */
typedef SimpleSmartPointer< FileEntry > EntryPointer ;


/** ConstEntryPointer is a SimpleSmartPointer for const FileEntry's.  */
typedef SimpleSmartPointer< const FileEntry > ConstEntryPointer ;

/** Entries is a vector of EntryPointer's */
typedef vector< EntryPointer >      Entries ;

/** ConstEntries is a vector of ConstEntryPointer's */
typedef vector< EntryPointer > ConstEntries ;



/** A FileEntry represents an entry in a FileCollection. The interface
    is a copy of the ZipEntry interface from the java.util.zip
    package. The name has been changed to FileEntry, as FileCollection
    is a more general abstraction, that covers other types of file
    collections than just zip files. */
class FileEntry {
public:

  /* Default construcotr, copy constructor and copy assignment
     operator are sufficient. */

  /** Returns the comment of the entry, if it has one. Otherwise it
      returns an empty string. 
      @return the comment associated with the entry, if there is one.
  */
  virtual string getComment() const = 0 ;
  /** Returns the compressed size of the entry. If the entry is not
      stored in a compressed format, the uncompressed size is
      returned.
      @return the compressed size of the entry. If the entry is stored without 
      compression the uncompressed size is returned.
  */
  virtual uint32 getCompressedSize() const = 0 ;
  /** Returns the Crc for the entry, if it has one. FIXME: what is
      returned if it doesn't have one?
      @return the Crc for the entry, if it has one.
  */
  virtual uint32 getCrc() const = 0 ;
  /** Returns a vector of bytes of extra data that may be stored with
      the entry.
      @return A vector< unsigned char > of extra bytes that may potentially
      be associated with an entry.
  */
  virtual vector< unsigned char > getExtra() const = 0 ;
  /** Returns the method used to store the entry in the FileCollection.
      @return the storage method used to store the entry in the collection.
      @see StorageMethod.
   */
  virtual StorageMethod getMethod() const = 0 ;
  /** Returns the full filename of the entry, including a path if the
      entry is stored in a subfolder. 
      @return the filename of the entry, including path if the entry is stored
      in a sub-folder.
  */
  virtual string getName() const = 0 ;
  /** Returns the filename of the entry.  
      @return Returns the filename of the entry.
  */
  virtual string getFileName() const = 0 ;
  /** Returns the (uncompressed) size of the entry data.  
      @return Returns the (uncompressed) size of the entry.
   */
  virtual uint32 getSize() const = 0 ;
  /** Returns the date and time of FIXME: what?  
      @return the date and time of the entry.
  */
  virtual int getTime() const = 0 ;
  /** Any method or operator that initializes a FileEntry may set a
      flag, that specifies whether the read entry is valid or not. If
      it isn't this method returns false.  
      @return true if the FileEntry has been parsed succesfully.
   */
  virtual bool isValid() const = 0 ;
  //     virtual int hashCode() const = 0 ;
  /** Returns true if the entry is a directory. A directory entry is
      an entry which name ends with a separator ('/' for Unix systems,
      '\' for Windows and DOS systems.  
      @return true if the entry is a directory.
   */
  virtual bool isDirectory() const = 0 ;
  
  /** Sets the comment field for the FileEntry.
      @param comment string with the new comment.
  */
  virtual void setComment( const string &comment ) = 0 ;
  /** Set the compressed size field of the entry.
      @param size value to set the compressed size field of the entry to.
  */
  virtual void setCompressedSize( uint32 size ) = 0 ;
  /** Sets the crc field.
      @param crc value to set the crc field to.
  */
  virtual void setCrc( uint32 crc ) = 0 ;
  /** Sets the extra field.
      @param extra the extra field is set to this value.
  */
  virtual void setExtra( const vector< unsigned char > &extra ) = 0 ;
  /** Sets the storage method field for the entry.
      @param method the method field is set to the specified value.
  */
  virtual void setMethod( StorageMethod method ) = 0 ;
  /** Sets the name field for the entry.
      @param name the name field is set to the specified value.
  */
  virtual void setName( const string &name ) = 0 ;
  /**   Sets the size field for the entry.
      @param size the size field is set to this value.
  */
  virtual void setSize( uint32 size ) = 0 ;
  /** Sets the time field for the entry.
      @param time the time field is set to the specified value.
  */
  virtual void setTime( int time ) = 0 ;
  
  /** Returns a human-readable string representation of the entry.
      @return a human-readable string representation of the entry.
  */
  virtual string toString() const = 0 ;

  /** Create a heap allocated clone of the object this method is called for. The 
      caller is responsible for deallocating the clone when he is done with it.
      @return A heap allocated copy of the object this method is called for.
  */
  virtual FileEntry *clone() const = 0 ;
  
  /** FileEntry destructor. */
  virtual ~FileEntry() {}

//  protected:
  class MatchName ;
  class MatchFileName ;
protected:
  friend class SimpleSmartPointer< FileEntry > ;
  friend class SimpleSmartPointer< const FileEntry > ;
  void           ref() const { _refcount.ref() ;          }
  unsigned int unref() const { return _refcount.unref() ; }

  ReferenceCount< FileEntry > _refcount ;
};

/** Function object to be used with the STL find_if algorithm to
    find a FileEntry in a container, which name (as obtained with
    FileEntry::getName()) is identical to the name specified in the
    MatchName constructor. */
class FileEntry::MatchName {
public:
  explicit MatchName( const string &name ) : _name( name ) {}
  bool operator() ( const ConstEntryPointer &entry ) {
    return entry->getName() == _name ;
  }
private:
  string _name ;
};

/** Function object to be used with the STL find_if algorithm to
    find a FileEntry in a container, which name (as obtained with
    FileEntry::getFileName()) is identical to the name specified in the
    MatchName constructor. */
class FileEntry::MatchFileName {
public:
  explicit MatchFileName( const string &name ) : _name( name ) {}
  bool operator() ( const ConstEntryPointer &entry ) {
    return entry->getFileName() == _name ;
  }
private:
  string _name ;
};

ostream &operator<< ( ostream &os, const FileEntry &entry ) ;

inline ostream &operator<< ( ostream &os, const ConstEntryPointer &entry ) {
  os << *entry ;
  return os ;
}



} // namespace

#endif


/** \file
    Header file that defines FileEntry.
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
