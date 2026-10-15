#pragma once
#ifndef ZIPIOS_CONFIG_HPP
#define ZIPIOS_CONFIG_HPP

/*
  Zipios -- a small C++ library that provides easy access to .zip files.

  Copyright (C) 2000-2007  Thomas Sondergaard
  Copyright (c) 2015-2022  Made to Order Software Corp.  All Rights Reserved

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/** \file
 * \brief zipios configuration header.
 *
 * Various configuration parameters, all of which are defined using
 * system specific \#ifdef and similar preprocessor tests.
 *
 * This file also includes some general information such as the library
 * version and common functions and types that are public.
 */

#include <sys/stat.h>

#include <iostream>

#include <stdio.h>


#define    ZIPIOS_VERSION_MAJOR   @ZIPIOS_VERSION_MAJOR@
#define    ZIPIOS_VERSION_MINOR   @ZIPIOS_VERSION_MINOR@
#define    ZIPIOS_VERSION_PATCH   @ZIPIOS_VERSION_PATCH@
#define    ZIPIOS_VERSION_STRING  "@ZIPIOS_VERSION_MAJOR@.@ZIPIOS_VERSION_MINOR@.@ZIPIOS_VERSION_PATCH@"


namespace zipios
{


inline char const * getVersion()
{
    return ZIPIOS_VERSION_STRING;
}


typedef std::streamoff      offset_t;


inline size_t getBufferSize()
{
    return BUFSIZ;
}


} // zipios namespace


// Visual C++
#ifdef _MSC_VER

// Disable class-browser warning about truncated template-names
// Still needed? TBD
//#pragma warning( disable : 4786 )

#endif //_MSC_VER

#ifdef WIN32

// Needed for FilePath
#define S_ISREG(mode)   (((mode) & _S_IFREG) == _S_IFREG)
#define S_ISDIR(mode)   (((mode) & _S_IFDIR) == _S_IFDIR)
#define S_ISCHR(mode)   (((mode) & _S_IFCHR) == _S_IFCHR)
#define S_ISBLK(mode)   0
#define S_ISSOCK(mode)  0
#define S_ISFIFO(mode)  (((mode) & _S_IFIFO) == _S_IFIFO)

// todo: change to _stat64 or whatever is required to get full 64 bit support
typedef struct stat     os_stat_t;

#else

typedef struct stat     os_stat_t;

#endif

// vim: ts=4 sw=4 et
#endif
