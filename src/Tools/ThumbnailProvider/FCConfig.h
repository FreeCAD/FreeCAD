// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
/** \file FCConfig.h
 *  \brief Include all needed defines and macros
 *  Here all defines and macro switching is done for FreeCAD.
 *  Every used library has its own section to define the configuration.
 *  This file keeps the makefiles and project files cleaner.
 */


#pragma once


//**************************************************************************
// switching the operating systems

// First check for *WIN64* since the *WIN32* are also set on 64-bit platforms
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
# ifndef FC_OS_WIN32
#  define FC_OS_WIN32
# endif
# ifndef FC_OS_WIN64
#  define FC_OS_WIN64
# endif
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# ifndef FC_OS_WIN32
#  define FC_OS_WIN32
# endif
# if defined(__MINGW32__)
#  if HAVE_CONFIG_H
#   include <config.h>
#  endif  // HAVE_CONFIG_H
# endif
#elif defined(__MWERKS__) && defined(__INTEL__)
# ifndef FC_OS_WIN32
#  define FC_OS_WIN32
# endif
#elif defined(__APPLE__)
# ifndef FC_OS_MACOSX
#  define FC_OS_MACOSX
# endif
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__GLIBC__)
# ifndef FC_OS_LINUX
#  define FC_OS_LINUX
# endif
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# ifndef FC_OS_BSD
#  define FC_OS_BSD
# endif
#elif defined(__CYGWIN__)
# ifndef FC_OS_CYGWIN
#  define FC_OS_CYGWIN
// Avoid conflicts with Inventor
#  define HAVE_INT8_T
#  define HAVE_UINT8_T
#  define HAVE_INT16_T
#  define HAVE_UINT16_T
#  define HAVE_INT32_T
#  define HAVE_UINT32_T
#  define HAVE_INT64_T
#  define HAVE_UINT64_T
#  define HAVE_INTPTR_T
#  define HAVE_UINTPTR_T
# endif
#else
# error "FreeCAD is not ported to this OS yet. For help see www.freecad.org"
#endif

#ifdef FC_OS_WIN32
# define PATHSEP '\\'
#else
# define PATHSEP '/'
#endif

//**************************************************************************
// Standard types for Windows

#if defined(__MINGW32__)
// nothing specific here
#elif defined(FC_OS_WIN64) || defined(FC_OS_WIN32)

# ifndef HAVE_INT8_T
#  define HAVE_INT8_T
typedef signed char int8_t;
# endif

# ifndef HAVE_UINT8_T
#  define HAVE_UINT8_T
typedef unsigned char uint8_t;
# endif

# ifndef HAVE_INT16_T
#  define HAVE_INT16_T
typedef short int16_t;
# endif

# ifndef HAVE_UINT16_T
#  define HAVE_UINT16_T
typedef unsigned short uint16_t;
# endif

# ifndef HAVE_INT32_T
#  define HAVE_INT32_T
typedef int int32_t;
# endif

# ifndef HAVE_UINT32_T
#  define HAVE_UINT32_T
typedef unsigned int uint32_t;
# endif

# ifndef HAVE_INT64_T
#  define HAVE_INT64_T
typedef __int64 int64_t;
# endif

# ifndef HAVE_UINT64_T
#  define HAVE_UINT64_T
typedef unsigned __int64 uint64_t;
# endif

#endif


//**************************************************************************
// Windows import export DLL defines
#ifndef BaseExport
# define BaseExport
#endif
#ifndef GuiExport
# define GuiExport
#endif
#ifndef AppExport
# define AppExport
#endif
#ifndef DataExport
# define DataExport
#endif


//**************************************************************************
// point at which warnings of overly long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
# pragma warning(disable : 4251)
# pragma warning(disable : 4996)  // suppress deprecated warning for e.g. open()
# if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4267)
# endif
#endif
