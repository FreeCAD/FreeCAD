#ifndef ZIPIOS_CONFIG_H
#define ZIPIOS_CONFIG_H

#include <FCConfig.h>

// undef IGNORE that is pulled in by Windows.h
#ifdef IGNORE
#undef IGNORE
#endif

#ifdef _MSC_VER

// This is fine for VC++ 5.0 sp 3
#define HAVE_STD_IOSTREAM
#define USE_STD_IOSTREAM

// Visual C++

#ifdef _MSC_VER

// Disable class-browser warning about truncated template-names
#pragma warning( disable : 4786 )

#endif //_MSC_VER

// Needed for FilePath
#ifndef S_ISREG
#define S_ISREG(mode)	(((mode) & _S_IFREG) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode)	(((mode) & _S_IFDIR) == _S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode)	(((mode) & _S_IFCHR) == _S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(mode)	0
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(mode)	0
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode)	(((mode) & _S_IFIFO) == _S_IFIFO)
#endif


// Convenient place to include any debugging-headers
#include <assert.h>

#else // gcc and others
#ifndef S_ISSOCK
#define S_ISSOCK(mode)	0
#endif
#include <stdint.h>
# if HAVE_CONFIG_H
#	  include <config.h>
# endif // HAVE_CONFIG_H

#endif //_MSC_VER

#endif // ZIPIOS_CONFIG_H

/** \file
    Configuration header file that allows compatibility with win32 compilers
*/

/*
  Zipios++ - a small C++ library that provides easy access to .zip files.
  Copyright (C) 2000  1. Thomas Søndergaard 2. Kevin Shea
  Written by Kevin Shea
  
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
