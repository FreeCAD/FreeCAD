#ifndef OUTPUTSTRINGSTREAM_H
#define OUTPUTSTRINGSTREAM_H

#include "zipios-config.h"

#include "meta-iostreams.h"
#include <string>

namespace zipios {

#if defined (HAVE_STD_IOSTREAM) && defined (USE_STD_IOSTREAM)

typedef std::ostringstream OutputStringStream ;

#else

/** OutputStringStream is typedefed to ostringstream if sstream is
    part of the standard library (unless Zipios++ has been explicitly
    configured not to use it). If sstream is not present
    OutputStringStream is a subclass of ostrstream from
    strstream.h. In this case OutputStringStream specializes the str()
    method, such that the caller does not have to concern himself with
    null-terminating the string and unfreezing the ostrstream. */
class OutputStringStream : public std::ostrstream {
public:

  /** Specialization of ostrstream::str() that takes care of
      null-terminating the string and unfreezing the ostrstream.  */
  inline std::string str() {
    *this << std::ends ; // null terminate ostrstream
    string o_str( ostrstream::str() ) ;
    freeze( 0 ) ;
    return o_str ;
  }
private:
  // To avoid invoking such a member function in the base
  // class if there is one!
  std::string str() const ; 
};

#endif

} // namespace 


#endif

/** \file
    Header file that defines OutputStringStream.
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
