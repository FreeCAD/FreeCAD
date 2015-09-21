#ifndef FILTERINPUTSTREAMBUF_H
#define FILTERINPUTSTREAMBUF_H

#include "zipios-config.h"

#include "meta-iostreams.h"

namespace zipios {

using std::streambuf ;

/** An input streambuf filter is a streambuf that filters the input it
 gets from the streambuf it is attached to. FilterInputStreambuf is a base class to
 derive input streambuf filters from. */
class FilterInputStreambuf : public streambuf {
public:
  /** Constructor.
      @param inbuf the streambuf to use for input.
      @param del_inbuf if true is specified inbuf will be deleted, when 
      the FilterInputStreambuf is destructed.
  */
  explicit FilterInputStreambuf( streambuf *inbuf, bool del_inbuf = false ) ;
  /** Destructor. */
  virtual ~FilterInputStreambuf() ;

protected:
  int _s_pos ; // Position in this streambuf - FIXME: is this used?
  streambuf *_inbuf ;
  bool _del_inbuf ;
private:

  /** Copy-constructor is private to prevent copying. */
  FilterInputStreambuf( const FilterInputStreambuf &src ) ;

  /** Copy-assignment operator is private to prevent copying.  */
  const FilterInputStreambuf &operator= ( const FilterInputStreambuf &src ) ;
};


} // namespace 


#endif

/** \file
    Header file that defines FilterInputStreambuf.
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
