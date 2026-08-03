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
// Version: 4.0.1 (2006/08/15)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Platforms.h"

// common standard library headers
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>

// STL headers
#include <algorithm>
#include <exception>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

// memory management
#include "Wm4Memory.h"

namespace Wm4
{

class WM4_FOUNDATION_ITEM System
{
public:
    // swap byte order (size = 2, 4, or 8)
    static void SwapBytes (int iSize, void* pvValue);
    static void SwapBytes (int iSize, int iQuantity, void* pvValue);

    // The output is stored in little endian format.
    static bool IsBigEndian ();
    static void EndianCopy (int iSize, const void* pvSrc, void* pvDst);
    static void EndianCopy (int iSize, int iQuantity, const void* pvSrc,
        void* pvDst);

    // time utility (return value is seconds)
    static double GetTime ();

    // File support for buffer load and save.
    static bool Load (const char* acFilename, char*& racBuffer, int& riSize);
    static bool Save (const char* acFilename, const char* acBuffer,
        int iSize);
    static bool Append (const char* acFilename, char* acBuffer, int iSize);

    // Buffer read and write, file read and write, for character data.  The
    // endianness is irrelevant.
    static int Read1 (const char* acBuffer, int iQuantity, void* pvData);
    static int Write1 (char* acBuffer, int iQuantity, const void* pvData);
    static int Read1 (FILE* pkFile, int iQuantity, void* pvData);
    static int Write1 (FILE* pkFile, int iQuantity, const void* pvData);

    // Buffer read and write, file read and write.  The operations store the
    // results in little-endian order.  The return value is the number of
    // bytes read or written.  The function name suffix indicates the number
    // of bytes transferred per item.
    static int Read2le (const char* acBuffer, int iQuantity, void* pvData);
    static int Read4le (const char* acBuffer, int iQuantity, void* pvData);
    static int Read8le (const char* acBuffer, int iQuantity, void* pvData);
    static int Write2le (char* acBuffer, int iQuantity, const void* pvData);
    static int Write4le (char* acBuffer, int iQuantity, const void* pvData);
    static int Write8le (char* acBuffer, int iQuantity, const void* pvData);
    static int Read2le (FILE* pkFile, int iQuantity, void* pvData);
    static int Read4le (FILE* pkFile, int iQuantity, void* pvData);
    static int Read8le (FILE* pkFile, int iQuantity, void* pvData);
    static int Write2le (FILE* pkFile, int iQuantity, const void* pvData);
    static int Write4le (FILE* pkFile, int iQuantity, const void* pvData);
    static int Write8le (FILE* pkFile, int iQuantity, const void* pvData);

    // Buffer read and write, file read and write.  The operations store the
    // results in big-endian order.  The return value is the number of
    // bytes read or written.  The function name suffix indicates the number
    // of bytes transferred per item.
    static int Read2be (const char* acBuffer, int iQuantity, void* pvData);
    static int Read4be (const char* acBuffer, int iQuantity, void* pvData);
    static int Read8be (const char* acBuffer, int iQuantity, void* pvData);
    static int Write2be (char* acBuffer, int iQuantity, const void* pvData);
    static int Write4be (char* acBuffer, int iQuantity, const void* pvData);
    static int Write8be (char* acBuffer, int iQuantity, const void* pvData);
    static int Read2be (FILE* pkFile, int iQuantity, void* pvData);
    static int Read4be (FILE* pkFile, int iQuantity, void* pvData);
    static int Read8be (FILE* pkFile, int iQuantity, void* pvData);
    static int Write2be (FILE* pkFile, int iQuantity, const void* pvData);
    static int Write4be (FILE* pkFile, int iQuantity, const void* pvData);
    static int Write8be (FILE* pkFile, int iQuantity, const void* pvData);

    // This is needed on the Macintosh because of its complicated application
    // structure.  In particular, this function is used in Xcode projects and
    // ignores the directory entry, but assumes that the data files required
    // by an application are added to the Resources folder of the projects.
    // The other platforms concatenate the directory and filename, the result
    // stored in class-static memory (so be careful with threads).
    static const char* GetPath (const char* acDirectory,
        const char* acFilename);

    // Maintain a list of directories that applications can search to find
    // data files.  The list is initially empty when an application starts,
    // so you must insert directories of interest, even the current working
    // directory.  The directory name should *not* end with a "/".  The
    // InsertDirectory and RemoveDirectory functions will manage that
    // character themselves.
    static void Initialize ();
    static void Terminate ();
    static int GetDirectoryQuantity ();
    static const char* GetDirectory (int i);
    static bool InsertDirectory (const char* acDirectory);
    static bool RemoveDirectory (const char* acDirectory);
    static void RemoveAllDirectories ();

    // The GetPath function searches the list of directories and returns the
    // fully decorated file name, if it exists, or return null, if it does
    // not exist.  The input iMode is selected from one of the three
    // enumerated values listed below.
    enum
    {
        SM_READ,
        SM_WRITE,
        SM_READ_WRITE
    };
    static const char* GetPath (const char* acFilename, int eMode);

    // Creation of colors, hides endianness.
    static unsigned int MakeRGB (unsigned char ucR, unsigned char ucG,
        unsigned char ucB);
    static unsigned int MakeRGBA (unsigned char ucR, unsigned char ucG,
        unsigned char ucB, unsigned char ucA);

    // Wrappers for standard I/O functions to support security enhancements
    // related to buffer overruns.  TO DO:  For now these support MSVC8.0
    // and fall back to the older standard I/O functions on other platforms.
    // Implementations may be provided for those other platforms to adhere
    // to the destination buffer size specification.
    static FILE* Fopen (const char* acFilename, const char* acMode);
    static int Fprintf (FILE* pkFile, const char* acFormat, ...);
    static int Fclose (FILE* pkFile);
    static const char* GetEnv (const char* acEnvVarName);
    static void* Memcpy (void* pvDst, size_t uiDstSize, const void* pvSrc,
        size_t uiSrcSize);
    static int Sprintf (char* acDst, size_t uiDstSize, const char* acFormat,
        ...);
    static char* Strcpy (char* acDst, size_t uiDstSize, const char* acSrc);
    static char* Strcat (char* acDst, size_t uiDstSize, const char* acSrc);
    static char* Strncpy (char* acDst, size_t uiDstSize, const char* acSrc,
        size_t uiSrcSize);
    static char* Strtok (char* acToken, const char* acDelimiters,
        char*& racNextToken);

private:
    enum { SYSTEM_MAX_PATH = 1024, SYSTEM_MAX_ENVVAR = 1024 };
    static char ms_acPath[SYSTEM_MAX_PATH];
    static char ms_acEnvVar[SYSTEM_MAX_ENVVAR];
    static std::vector<std::string>* ms_pkDirectories;

public:
    static char WM4_PATH[SYSTEM_MAX_ENVVAR];
};

// Allocation and deallocation of 2D arrays.  On deallocation, the array
// pointer is set to null.
template <class T> void Allocate (int iCols, int iRows, T**& raatArray);
template <class T> static void Deallocate (T**& raatArray);

// Allocation and deallocation of 3D arrays.  On deallocation, the array
// pointer is set to null.
template <class T> void Allocate (int iCols, int iRows, int iSlices,
    T***& raaatArray);
template <class T> void Deallocate (T***& raaatArray);

}

#include "Wm4System.inl"

// Miscellaneous support.  STL does not yet formally have hash structures.
// The includes are placed here because the hash structures use System
// wrappers for memcpy.
#include "Wm4THashSet.h"
#include "Wm4THashTable.h"
#include "Wm4TMinHeap.h"
#include "Wm4TStringHashTable.h"
#include "Wm4TSmallUnorderedSet.h"  // useful for meshes
#include "Wm4TTuple.h"  // tuples in any type and any dimension