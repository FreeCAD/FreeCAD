
#include "zipios-config.h"

#include "meta-iostreams.h"

#include "zipoutputstreambuf.h"
#include "zipoutputstream.h"
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
#include <Base/FileInfo.h>
#endif

using std::ostream;

namespace zipios {

ZipOutputStream::ZipOutputStream( std::ostream &os ) 
  : std::ostream( nullptr ), 
// SGIs basic_ifstream calls istream with 0, but calls basic_ios constructor first??
    ofs( nullptr )
{
  ozf = new ZipOutputStreambuf( os.rdbuf() ) ;
  
  init( ozf ) ;
}


ZipOutputStream::ZipOutputStream( const std::string &filename )
  : std::ostream( nullptr ),
    ofs( nullptr )
{
#if defined(_WIN32) && defined(ZIPIOS_UTF8)
  std::wstring wsfilename = Base::FileInfo(filename).toStdWString();
  ofs = new std::ofstream( wsfilename.c_str(), std::ios::out | std::ios::binary ) ;
#else
  ofs = new std::ofstream( filename.c_str(), std::ios::out | std::ios::binary ) ;
#endif
  ozf = new ZipOutputStreambuf( ofs->rdbuf() ) ;
  this->init( ozf ) ;
}

void ZipOutputStream::closeEntry() {
  ozf->closeEntry() ;
}


void ZipOutputStream::close() {
  ozf->close() ;  
  if ( ofs )
    ofs->close() ;
}


void ZipOutputStream::finish() {
  ozf->finish() ;
}


void ZipOutputStream::putNextEntry( const ZipCDirEntry &entry ) {
  ozf->putNextEntry( entry ) ;
}

void ZipOutputStream::putNextEntry(const std::string& entryName) {
  putNextEntry( ZipCDirEntry(entryName));
}


void ZipOutputStream::setComment( const std::string &comment ) {
  ozf->setComment( comment ) ;
}


void ZipOutputStream::setLevel( int level ) {
  ozf->setLevel( level ) ;
}


void ZipOutputStream::setMethod( StorageMethod method ) {
  ozf->setMethod( method ) ;
}


ZipOutputStream::~ZipOutputStream() {
  // It's ok to call delete with a Null pointer.
  delete ozf ;
  delete ofs ;
}

} // namespace

/** \file
    Implementation of ZipOutputStream.
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
