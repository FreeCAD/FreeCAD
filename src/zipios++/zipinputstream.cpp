
#include "zipios-config.h"

#include "meta-iostreams.h"

#include "zipinputstreambuf.h"
#include "zipinputstream.h"
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
#include <Base/FileInfo.h>
#endif

using std::istream;

namespace zipios {

ZipInputStream::ZipInputStream( std::istream &is, std::streampos pos ) 
  : std::istream( nullptr ), 
// SGIs basic_ifstream calls istream with 0, but calls basic_ios constructor first??
    ifs( nullptr )
{
  izf = new ZipInputStreambuf( is.rdbuf(), pos ) ;
//  this->rdbuf( izf ) ; is replaced by:
  this->init( izf ) ;
}

ZipInputStream::ZipInputStream( const std::string &filename, std::streampos pos )
  : std::istream( nullptr ),
    ifs( nullptr )
{
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
  std::wstring wsname = Base::FileInfo(filename).toStdWString();
  ifs = new std::ifstream( wsname.c_str(), std::ios::in |std:: ios::binary ) ;
#else
  ifs = new std::ifstream( filename.c_str(), std::ios::in |std:: ios::binary ) ;
#endif
  izf = new ZipInputStreambuf( ifs->rdbuf(), pos ) ;
//  this->rdbuf( izf ) ; is replaced by:
  this->init( izf ) ;
}

int ZipInputStream::available() {
  return 1 ; // FIXME: Dummy implementation
}

void ZipInputStream::closeEntry() {
  izf->closeEntry() ;
}

void ZipInputStream::close() {
  izf->close() ;  
}

//    ZipLocalEntry *ZipInputStream::createZipCDirEntry( const string
//    &name ) {}

ConstEntryPointer ZipInputStream::getNextEntry() {
  clear() ; // clear eof and other flags.
  return izf->getNextEntry() ;
}

ZipInputStream::~ZipInputStream() {
  // It's ok to call delete with a Null pointer.
  delete izf ;
  delete ifs ;
}

} // namespace

/** \file
    Implementation of ZipInputStream.
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
