// SPDX-License-Identifier: LGPL-2.1-or-later

// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.1 (2006/11/14)

#pragma once

// Platform-specific information goes in this header file.  The defines to
// control which platform is included are:
//
// _WIN32      :  Microsoft Windows XP
// WIN32       :  Microsoft Windows XP
// __MINGW32__ :  Minimalist GNU for Windows
// __CYGWIN__  :  Cygwin
// __APPLE__   :  Macintosh OS X (10.2.3 or higher required)
// __sgi       :  Silicon Graphics Irix
// __sun       :  Sun Solaris
// <none>      :  Linux
//
// Add others as needed.

//----------------------------------------------------------------------------
// Minimalist GNU for Windows
//----------------------------------------------------------------------------
#if defined(__MINGW32__)

typedef long long Integer64;

#elif defined(__CYGWIN__)
//----------------------------------------------------------------------------
// Cygwin
//----------------------------------------------------------------------------

#ifndef _WIN32
#define _WIN32
#endif

#ifndef WIN32
#define WIN32
#endif

typedef long long Integer64;

//----------------------------------------------------------------------------
// Microsoft Windows 2000/XP platform
//----------------------------------------------------------------------------
#elif defined(_WIN32) || defined(WIN32)

#if defined(_MSC_VER)

// Microsoft Visual C++ specific pragmas.
#if _MSC_VER
#define WM4_USING_VC80
#endif

#if defined(WM4_USING_VC6)

// Disable the warning "non dll-interface class FOO used as base for
// dll-interface class BAR."  These occur in the derivations
// class Binary2D : public ImageInt2D; class Binary3D : public ImageInt3D;
//#pragma warning( disable : 4275 )

// Disable the warning about truncating the debug names to 255 characters.
// This warning shows up often with STL code in MSVC6, but not MSVC7.
#pragma warning( disable : 4786 )

// This warning is disabled because MSVC6 warns about not finding
// implementations for the pure virtual functions that occur in the template
// classes 'template <class Real>' when explicitly instantiating the classes.
// NOTE:  If you create your own template classes that will be explicitly
// instantiated, you should re-enable the warning to make sure that in fact
// all your member data and functions have been defined and implemented.
#pragma warning( disable : 4661 )

#endif

// The use of WM4_FOUNDATION_ITEM to export an entire class generates warnings
// when member data and functions involving templates or inlines occur.  To
// avoid the warning, WM4_FOUNDATION_ITEM can be applied only to those items
// that really need to be exported.
#pragma warning( disable : 4251 ) 

typedef __int64 Integer64;

#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Macintosh OS X platform
//----------------------------------------------------------------------------
#elif defined(__APPLE__)

#if defined(__BIG_ENDIAN__)
#define WM4_BIG_ENDIAN
#else
#define WM4_LITTLE_ENDIAN
#endif

#include <stdint.h>
typedef int64_t Integer64;
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Linux platform
//----------------------------------------------------------------------------
#else

#include <cstdint>
typedef int64_t Integer64;

#endif