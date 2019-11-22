#ifndef META_IOSTREAMS_H
#define META_IOSTREAMS_H

// Includes the different iostream libraries

#include "zipios-config.h"

#include <iostream>
#include <fstream>

#if defined (HAVE_STD_IOSTREAM) && defined (USE_STD_IOSTREAM)
#include <sstream>
#else
#include <strstream>
#endif

#endif

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
