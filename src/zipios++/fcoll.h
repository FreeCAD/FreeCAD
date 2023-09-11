#ifndef fcoll_H
#define fcoll_H

#include "zipios++/zipios-config.h"

#include <vector>
#include <string>

#include "zipios++/fcollexceptions.h"
#include "zipios++/fileentry.h"

namespace zipios {

using std::vector;

/** \anchor fcoll_anchor
    FileCollection is an abstract baseclass that represents a
    collection of files. The specializations of FileCollection
    represents different origins of file collections, such as
    directories, simple filename lists and compressed archives. */
class FileCollection {
public:
  /** FileCollection constructor. */
  explicit FileCollection() 
    : _filename( "-"   ),
      _entries ( 0     ),
      _valid   ( false ) {}

  /** Copy constructor. */
  inline FileCollection( const FileCollection &src ) ;

  /** Copy assignment operator. */
  inline const FileCollection &operator= ( const FileCollection &src ) ;
  
  /** Closes the FileCollection. */
  virtual void close() = 0 ;

  /** \anchor fcoll_entries_anchor
      Returns a vector of const pointers to the entries in the
      FileCollection.  
      @return a ConstEntries
      containing the entries of the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual ConstEntries entries() const ;

  enum MatchPath { IGNORE, MATCH } ;

  /** \anchor fcoll_getentry_anchor
      Returns a ConstEntryPointer to a FileEntry object for the entry 
      with the specified name. To ignore the path part of the filename in search of a
      match, specify FileCollection::IGNORE as the second argument.
      @param name A string containing the name of the entry to get.
      @param matchpath Speficy MATCH, if the path should match as well, 
      specify IGNORE, if the path should be ignored.
      @return A ConstEntryPointer to the found entry. The returned pointer
      equals zero if no entry is found.
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual ConstEntryPointer getEntry( const string &name, 
				     MatchPath matchpath = MATCH ) const ;
  /** \anchor fcoll_getinputstream
      Returns a pointer to an opened istream for the specified
      FileEntry. It is the callers responsibility to delete the stream
      when he is done with it. Returns 0, if there is no such
      FileEntry in the FileCollection.
      @param entry A ConstEntryPointer to the FileEntry to get an istream to.
      @return an open istream for the specified entry. The istream is allocated on
      heap and it is the users responsibility to delete it when he is done with it.
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual istream *getInputStream( const ConstEntryPointer &entry ) = 0 ;
  /** Returns a pointer to an opened istream for the specified
      entry name. It is the callers responsibility to delete the stream
      when he is done with it. Returns 0, if there is no entry with the specified
      name in the FileCollection.
      @param matchpath Speficy MATCH, if the path should match as well, 
      specify IGNORE, if the path should be ignored.
      @return an open istream for the specified entry. The istream is allocated on
      heap and it is the users responsibility to delete it when he is done with it.
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual istream *getInputStream( const string &entry_name, 
				     MatchPath matchpath = MATCH ) = 0 ;
  /** Returns the name of the FileCollection.
      @return the name of the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual string getName() const ;
  /** Returns the number of entries in the FileCollection.
      @return the number of entries in the FileCollection. 
      @throw InvalidStateException Thrown if the collection is invalid. */
  virtual int size() const ;

  /** The member function returns true if the collection is valid.
      @return true if the collection is valid.
   */ 
  bool isValid() const { return _valid ; }

  /** Create a heap allocated clone of the object this method is called for. The 
      caller is responsible for deallocating the clone when he is done with it.
      @return A heap allocated copy of the object this method is called for.
  */
  virtual FileCollection *clone() const = 0 ;


  /** FileCollection destructor. */
  virtual ~FileCollection () ;
protected:
  string _filename ;
  Entries _entries ;
  bool _valid ;
};


//
// Inline methods
//

FileCollection::FileCollection( const FileCollection &src ) 
  : _filename( src._filename ),
    _valid   ( src._valid    )
{
  _entries.reserve( src._entries.size() ) ;
  Entries::const_iterator it ;
  for ( it = src._entries.begin() ; it != src._entries.end() ; ++it )
    _entries.push_back( (*it)->clone() ) ;
}

const FileCollection &FileCollection::operator= ( const FileCollection &src ) {
  if ( this != &src ) {
    _filename = src._filename ;
    _valid    = src._valid    ;
    _entries.clear() ;
    _entries.reserve( src._entries.size() ) ;
    
    Entries::const_iterator it ;
    for ( it = src._entries.begin() ; it != src._entries.end() ; ++it )
      _entries.push_back( (*it)->clone() ) ;
  }
  return *this ;
}

inline ostream & operator<< (ostream &os, const FileCollection& collection) {
	os << "collection '" << collection.getName() << "' {" ;
	ConstEntries entries = collection.entries();
	ConstEntries::const_iterator it;
	bool isFirst=true;
	for (it=entries.begin(); it != entries.end(); ++it) {
		if(! isFirst)
			os << ", ";
		isFirst = false;
		os << (*it)->getName();
	}
	os << "}";
	return os;
}


} // namespace

#endif


/**
   \mainpage Zipios++

   \image html   zipios++.jpg
   \image latex  zipios++.eps width=10cm
   
   \section intro Introduction
   
   Zipios++ is a java.util.zip-like C++ library for reading and
   writing Zip files. Access to individual entries is provided through
   standard C++ iostreams. A simple read-only virtual file system that
   mounts regular directories and zip files is also provided.
   
   The source code is released under the <A
   HREF="http://www.gnu.org/copyleft/lesser.html">GNU Lesser General Public
   License</A>.
   
   \section status Status

   Spanned archives are not supported, and support is not planned.
   

   The library has been tested and appears to be working with
   <UL>
   <LI><A HREF="http://www.freebsd.org/ports/archivers.html#zipios++-0.1.5">FreeBSD stable and current / gcc 2.95.3</A></LI>
   <LI>Red Hat Linux release 7.0  / gcc 2.96</LI>
   <LI>Red Hat Linux release 6.2 (Zoot) / egcs-2.91.66</LI>
   <LI>Linux Mandrake release 7.0 (Air) / gcc 2.95.2</LI>
   <LI>SGI IRIX64 6.5 / gcc 2.95.2</LI>
   <LI>SGI IRIX64 6.5 / MIPSpro Compilers: Version 7.30</LI>
   </UL>

   If you are aware of any other platforms that Zipios++ works on,
   please let me know (thomass@deltadata.dk).

   \section documentation Documentation 
   This web page is the front page to the library documentation which
   is generated from the source files using <A
   HREF="http://www.stack.nl/~dimitri/doxygen/index.html">Doxygen</A>. Use
   the links at the top of the page to browse the API
   documentation. The documentation is also available in
   a printer-friendly format <A HREF="refman.pdf">[pdf]</A>.
   
   \subsection zipfiles Zip file access
   The two most important classes are \ref zipfile_anchor "ZipFile" and 
   \ref ZipInputStream_anchor "ZipInputStream". ZipInputStream is an istream
   for reading zipfiles. It can be instantiated directly, without the
   use of ZipFile. \ref ZipInputStream_getnextentry_anchor 
   "ZipInputStream::getNextEntry()" positions the new ZipInputStream at the
   first entry. The following entry can be accessed by calling
   \ref ZipInputStream_getnextentry_anchor "ZipInputStream::getNextEntry()"
   again.
   
   ZipFile scans the central directory of a zipfile and provides an
   interface to access that directory. The user may search for entries
   with a particular filename using \ref fcoll_getentry_anchor "ZipFile::getEntry()", 
   or simply get the complete list of entries
   with \ref fcoll_entries_anchor "ZipFile::entries()". To get an
   istream (ZipInputStream) to a particular entry simply use
   \ref fcoll_getinputstream "ZipFile::getInputStream()".
   
   \ref example_zip_anchor "example_zip.cpp" demonstrates the central
   elements of Zipios++.
   
   A Zip file appended to another file, e.g. a binary program, with the program
   \ref appendzip_anchor "appendzip", can be read with 
   \ref zipfile_openembeddedzipfile "ZipFile::openEmbeddedZipFile()".

   \subsection filecollections FileCollections
   
   A ZipFile is actually just a special kind of 
   \ref fcoll_anchor "FileCollection" that
   obtains its entries from a .zip Zip archive. Zipios++ also implements
   a \ref dircol_anchor "DirectoryCollection" that obtains its entries 
   from a specified directory, and a \ref collcoll_anchor "CollectionCollection" 
   that obtains its entries from
   other collections. Using a single CollectionCollection any number of
   other FileCollections can be placed under its control and accessed
   through the same single interface that is used to access a ZipFile or
   a DirectoryCollection. A singleton (a unique global instance)
   CollectionCollection can be obtained through
   
   \ref collcoll_inst_anchor "CollectionCollection::inst()" ;

   To save typing CollectionCollection has been typedef'ed to CColl. In
   the initialization part of an application FileCollections can be
   created, and placed under CColll::inst()'s control using
   
   \ref collcoll_addcoll_anchor "CColl::inst().addCollection()"
   
   and later an istream can be obtained using

   \ref fcoll_getinputstream "CColl::inst().getInputStream()".
   
   \section download Download 
   Go to Zipios++ project page on SourceForge for tar balls and ChangeLog.
   <A HREF="http://sourceforge.net/project/?group_id=5418" >
   http://sourceforge.net/project/?group_id=5418</A>
   
   \section links Links
   <A HREF="ftp://ftp.freesoftware.com/pub/infozip/zlib/zlib.html">zlib</A>. 
   The compression library that Zipios++ uses to perform the actual 
   decompression.
   
   <A HREF="http://java.sun.com/products/jdk/1.3/docs/api/index.html">
   Java 2 Platform, Standard Edition, v 1.3 API Specification
   </A>. Zipios++ is heavily inspired by the java.util.zip package.
   
   <A
   HREF="http://www.geocities.com/SiliconValley/Lakes/2160/fformats/files/zip.txt">
   PKWARE zip format 
   </A>. A description of the zip file format.
   
   \section bugs Bugs 
   Submit bug reports and patches to thomass@deltadata.dk 
   
   
   
   \htmlonly
   Project hosted by <A HREF="http://sourceforge.net">
   <img src="http://sourceforge.net/sflogo.php?group_id=5418&type=1" >
   </A><p>
   Logo created with <A HREF="http://www.webgfx.ch/titlepic.htm">
   <img src="webgfx.gif" >
   </A>
   \endhtmlonly */


/** \file
    Header file that defines FileCollection.
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
