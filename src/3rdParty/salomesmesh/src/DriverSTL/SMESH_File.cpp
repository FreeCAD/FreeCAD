// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// File      : SMESH_File.cxx
// Created   : Wed Mar 10 11:23:25 2010
// Author    : Edward AGAPOV (eap)
//

#include "SMESH_File.hxx"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <boost/filesystem.hpp>

namespace boofs = boost::filesystem;

//================================================================================
/*!
 * \brief Creator opening the file for reading by default
 */
//================================================================================

SMESH_File::SMESH_File(const std::string& name, bool open)
  :_name(name), _size(-1), 
#ifdef WIN32
   _file(INVALID_HANDLE_VALUE),
#else
   _file(-1),
#endif
   _map(0), _pos(0), _end(0)
{
  if ( open ) this->open();
}

//================================================================================
/*!
 * \brief Destructor closing the file
 */
//================================================================================

SMESH_File::~SMESH_File()
{
  close();
}

//================================================================================
/*!
 * \brief Open file for reading. Return true if there is something to read
 */
//================================================================================

bool SMESH_File::open()
{
  int length = size();
  if ( !_map && length > 0 )
  {
#ifdef WIN32
    _file = CreateFileA(_name.data(), GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    bool ok = ( _file != INVALID_HANDLE_VALUE );
#else
    _file = ::open(_name.data(), O_RDONLY );
    bool ok = ( _file >= 0 );
#endif
    if ( ok )
    {
#ifdef WIN32
      _mapObj = CreateFileMapping(_file, NULL, PAGE_READONLY, 0, (DWORD)length, NULL);
      _map = (void*) MapViewOfFile( _mapObj, FILE_MAP_READ, 0, 0, 0 );
#else
      _map = ::mmap(0,length,PROT_READ,MAP_PRIVATE,_file,0);
      if ( _map == MAP_FAILED ) _map = NULL;
#endif
      if ( _map != NULL )
      {
        _size = length;
        _pos = (char*) _map;
        _end = _pos + _size;
      }
      else
      {
#ifdef WIN32
        CloseHandle(_mapObj);
        CloseHandle(_file);
#else
        ::close(_file);
#endif
      }
    }
    else if ( _error.empty() )
    {
      _error = "Can't open for reading an existing file " + _name;
    }
  }
  return _pos;
}

//================================================================================
/*!
 * \brief Close the file
 */
//================================================================================

void SMESH_File::close()
{
  if ( _map != NULL )
  {
#ifdef WIN32
    UnmapViewOfFile(_map);
    CloseHandle(_mapObj);
    CloseHandle(_file);
#else
    ::munmap(_map, _size);
    ::close(_file);
#endif
    _map = NULL;
    _pos = _end = 0;
    _size = -1;
  }
  //else if ( _file >= 0 )
  else if ( _file != 0 )
  {
#ifdef WIN32
    if(_file != INVALID_HANDLE_VALUE) {
      CloseHandle(_file);
      _file = INVALID_HANDLE_VALUE;
    }
#else
    if(_file != -1) {
      ::close(_file);
      _file = -1;
    }
#endif
  }
}

//================================================================================
/*!
 * \brief Remove the file
 */
//================================================================================

bool SMESH_File::remove()
{
  close();

  boost::system::error_code err;
  boofs::remove( _name, err );
  _error = err.message();

  return !err;
}

//================================================================================
/*!
 * \brief Return file size
 */
//================================================================================

long SMESH_File::size()
{
  if ( _size >= 0 ) return _size; // size of an open file

  boost::system::error_code err;
  boost::uintmax_t size = boofs::file_size( _name, err );
  _error = err.message();

  return err ? -1 : (long) size;
}

//================================================================================
/*!
 * \brief Check existence
 */
//================================================================================

bool SMESH_File::exists()
{
  boost::system::error_code err;
  bool res = boofs::exists( _name, err );
  _error = err.message();

  return err ? false : res;
}

//================================================================================
/*!
 * \brief Check existence
 */
//================================================================================

bool SMESH_File::isDirectory()
{
  boost::system::error_code err;
  bool res = boofs::is_directory( _name, err );
  _error = err.message();

  return err ? false : res;
}

//================================================================================
/*!
 * \brief Set cursor to the given position
 */
//================================================================================

void SMESH_File::setPos(const char* pos)
{
  if ( pos > (const char*)_map && pos < _end )
    _pos = (char*) pos;
}

//================================================================================
/*!
 * \brief Skip till current line end and return the skipped string
 */
//================================================================================

std::string SMESH_File::getLine()
{
  std::string line;
  const char* p = _pos;
  while ( !eof() )
    if ( *(++_pos) == '\n' )
      break;
  line.append( p, _pos );
  if ( !eof() ) _pos++;
  return line;
}

//================================================================================
/*!
 * \brief Move cursor to the file beginning
 */
//================================================================================

void SMESH_File::rewind()
{
  _pos = (char*) _map;
}

//================================================================================
/*!
 * \brief Fill vector by reading out integers from file. Vector size gives number
 * of integers to read
 */
//================================================================================

bool SMESH_File::getInts(std::vector<int>& ints)
{
  size_t i = 0;
  while ( i < ints.size() )
  {
    while ( !isdigit( *_pos ) && !eof()) ++_pos;
    if ( eof() ) break;
    if ( _pos[-1] == '-' ) --_pos;
    ints[ i++ ] = strtol( _pos, (char**)&_pos, 10 );
  }
  return ( i == ints.size() );
}

//================================================================================
/*!
 * \brief Open for binary writing only.
 */
//================================================================================

bool SMESH_File::openForWriting()
{
#ifdef WIN32

  _file = CreateFileA(_name.c_str(),          // name of the write
                      GENERIC_WRITE,          // open for writing
                      0,                      // do not share
                      NULL,                   // default security
                      OPEN_ALWAYS,            // CREATE NEW or OPEN EXISTING
                      FILE_ATTRIBUTE_NORMAL,  // normal file
                      NULL);                  // no attr. template
  return ( _file != INVALID_HANDLE_VALUE );

#else

  _file = ::open( _name.c_str(),
                  O_WRONLY | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); // rw-r--r--
  return _file >= 0;

#endif
}

//================================================================================
/*!
 * \brief Write binary data
 */
//================================================================================

bool SMESH_File::writeRaw(const void* data, size_t size)
{
#ifdef WIN32

  DWORD nbWritten = 0;
  BOOL err = WriteFile( _file, data, size, & nbWritten, NULL);

  return (( err == FALSE ) &&
          ( nbWritten == (DWORD) size ));

#else

  ssize_t nbWritten = ::write( _file, data, size );
  return ( nbWritten == (ssize_t) size );

#endif
}
