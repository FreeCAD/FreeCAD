
#include "zipios-config.h"

#include "meta-iostreams.h"

#include "fcollexceptions.h"

namespace zipios {

using std::cerr ;
using std::endl ;

IOException::IOException() throw () 
  : _what( "I/O exception" ) {}

IOException::IOException( const string &msg ) throw () 
  : _what( msg ) {}

IOException::IOException( const IOException &src ) throw () 
  : std::exception(), _what( src._what ) {}


IOException &IOException::operator= ( const IOException &src ) throw () {
  _what = src._what ;
  return *this ;
}

  
const char *IOException::what() const throw () {
  return _what.c_str() ;
}

IOException::~IOException() throw () {}






FCollException::FCollException() throw () 
  : _what( "FileCollection exception" ) {}

FCollException::FCollException( const string &msg ) throw () 
  : _what( msg ) {}

FCollException::FCollException( const FCollException &src ) throw () 
  : std::exception(),_what( src._what ) {}


FCollException &FCollException::operator= ( const FCollException &src ) throw () {
  _what = src._what ;
  return *this ;
}

  
const char *FCollException::what() const throw () {
  return _what.c_str() ;
}

FCollException::~FCollException() throw () {}






InvalidStateException::InvalidStateException() throw () 
  : _what( "InvalidState exception" ) {}

InvalidStateException::InvalidStateException( const string &msg ) throw () 
  : _what( msg ) {}

InvalidStateException::
InvalidStateException( const InvalidStateException &src ) throw () 
  : std::exception(), _what( src._what ) {}


InvalidStateException &InvalidStateException::
operator= ( const InvalidStateException &src ) throw () {
  _what = src._what ;
  return *this ;
}

  
const char *InvalidStateException::what() const throw () {
  return _what.c_str() ;
}

InvalidStateException::~InvalidStateException() throw () {} 





Exception::Exception() throw () 
  : _what( "Exception" ) {}

Exception::Exception( const string &msg ) throw () 
  : _what( msg ) {}

Exception::
Exception( const Exception &src ) throw () 
  : std::exception(),_what( src._what ) {}


Exception &Exception::
operator= ( const Exception &src ) throw () {
  _what = src._what ;
  return *this ;
}

  
const char *Exception::what() const throw () {
  return _what.c_str() ;
}

Exception::~Exception() throw () {} 


} // namespace

/** \file
    Implementation of a number of Exceptions used by FileCollection and its
    subclasses.
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
