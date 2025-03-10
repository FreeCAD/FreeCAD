#ifndef FCOLLEXCEPTIONS_H
#define FCOLLEXCEPTIONS_H

#include "zipios-config.h"

#include <stdexcept>
#include <string>

namespace zipios {

using std::string ;
using std::exception ;

/** An IOException is used to signal an I/O error.
 */
class IOException : public exception {
public:
  IOException() throw () ;
  explicit IOException( const string &msg ) throw () ;
  IOException( const IOException &src ) throw () ;
  IOException &operator= ( const IOException &src ) throw () ;
  
  virtual const char *what() const throw () ;
  virtual ~IOException() throw () ;
private:
  string _what ;
};

/** An FCollException is used to signal a problem with a
 FileCollection. */
class FCollException : public exception {
public:
  FCollException() throw () ;
  explicit FCollException( const string &msg ) throw () ;
  FCollException( const FCollException &src ) throw () ;
  FCollException &operator= ( const FCollException &src ) throw () ;
  
  virtual const char *what() const throw () ;
  virtual ~FCollException() throw () ;
private:
  string _what ;
};

/** An object member function may throw this exception, if the
    operation it normally performs is inappropriate or impossible to
    perform because of the current state of the object. */
class InvalidStateException : public exception {
public:
  InvalidStateException() throw () ;
  explicit InvalidStateException( const string &msg ) throw () ;
  InvalidStateException( const InvalidStateException &src ) throw () ;
  InvalidStateException &operator= ( const InvalidStateException &src ) throw () ;
  
  virtual const char *what() const throw () ;
  virtual ~InvalidStateException() throw () ;
private:
  string _what ;
};

/** Basic exception */
class Exception : public exception {
public:
  Exception() throw () ;
  explicit Exception( const string &msg ) throw () ;
  Exception( const Exception &src ) throw () ;
  Exception &operator= ( const Exception &src ) throw () ;
  
  virtual const char *what() const throw () ;
  virtual ~Exception() throw () ;
private:
  string _what ;
};


} // namespace
#endif

/** \file
    Header file that defines a number of exceptions used by FileCollection and
    its subclasses.
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
