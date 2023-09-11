#ifndef FILTEROUTPUTSTREAMBUF_H
#define FILTEROUTPUTSTREAMBUF_H

#include "zipios++/zipios-config.h"

#include "zipios++/meta-iostreams.h"

namespace zipios {

using std::streambuf ;

/** A FilterOutputStreambuf is a streambuf that filters the data that is written
    to it before it passes it on to the output streambuf it is connected to. */
class FilterOutputStreambuf : public streambuf {
public:

  /** Constructor.
      @param outbuf the streambuf to pass the filtered data on to.
      @param del_outbuf if true is specified outbuf will be deleted, when 
      the FilterOutputStreambuf is destructed.  */
  explicit FilterOutputStreambuf( streambuf *outbuf, bool del_outbuf = false ) ;

  /** Destructor. */
  virtual ~FilterOutputStreambuf() ;

protected:
  streambuf *_outbuf ;
  bool _del_outbuf ;
};


} // namespace 


#endif

/** \file
    Header file that defines FilterOutputStreambuf.
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
