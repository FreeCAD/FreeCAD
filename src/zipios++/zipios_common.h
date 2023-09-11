#ifndef ZIPIOS_COMMON_H
#define ZIPIOS_COMMON_H

#include "zipios++/zipios-config.h"

#include <vector>

namespace zipios {

using std::vector;

static const char separator = '/' ;

template< class Type >
void operator += ( vector< Type > &v1, const vector< Type > &v2 ) {
  typename std::vector<Type>::const_iterator cit ;
  for ( cit = v2.begin() ; cit != v2.end() ; ++cit )
    v1.push_back( *cit ) ;
}

template< class T >
inline const T& min( const T& a, const T& b ) {
  return b < a ? b : a ;
}


} // namespace

#endif

/** \file
    Header file containing miscellaneous small functions.
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
