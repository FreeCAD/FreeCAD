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
// Version: 4.0.0 (2006/06/28)

#pragma once

#ifndef WM4_MEMORY_MANAGER

// Use the default memory manager.
#define WM4_NEW new
#define WM4_DELETE delete

#else

// Overrides of the global new and delete operators.  These enhance the
// default memory manager by keeping track of information about allocations
// and deallocations.

#include "Wm4FoundationLIB.h"
#include "Wm4Platforms.h"

namespace Wm4
{

class WM4_FOUNDATION_ITEM Memory
{
public:
    // The memory chunks have information prepended of the following data
    // type.  The blocks are inserted and removed from a doubly linked list.
    struct Block
    {
        size_t Size;
        const char* File;
        unsigned int Line;
        bool IsArray;
        Block* Prev;
        Block* Next;
    };

    // read-write members
    static size_t& MaxAllowedBytes ();
    static bool& TrackSizes ();

    // read-only members
    static size_t GetNumNewCalls ();
    static size_t GetNumDeleteCalls ();
    static size_t GetNumBlocks ();
    static size_t GetNumBytes ();
    static size_t GetMaxAllocatedBytes ();
    static size_t GetMaxBlockSize ();
    static size_t GetHistogram (int i);

    // For iteration over the current list of memory blocks.
    static const Block* GetHead ();
    static const Block* GetTail ();

    // Generate a report about the current list memory blocks.
    static void GenerateReport (const char* acFilename);

private:
    // Count the number of times the memory allocation/deallocation system
    // has been entered.
    static size_t ms_uiNumNewCalls;
    static size_t ms_uiNumDeleteCalls;

    // Set this value in your application if you want to know when NumBytes
    // exceeds a maximum allowed number of bytes.  An 'assert' will be
    // triggered in Allocate when this happens.  The default value is 0, in
    // which case no comparison is made between NumBytes and MaxAllowedBytes.
    static size_t ms_uiMaxAllowedBytes;

    // The current number of allocated memory blocks.
    static size_t ms_uiNumBlocks;

    // The current number of allocated bytes.
    static size_t ms_uiNumBytes;

    // Doubly linked list of headers for the memory blocks.
    static Block* ms_pkHead;
    static Block* ms_pkTail;

    // Set this variable to 'true' if you want the ms_uiMaxBlockSize and
    // ms_auiHistogram[] elements to be computed.  The default is 'false'.
    static bool ms_bTrackSizes;

    // The maximum number of bytes allocated by the application.
    static size_t ms_uiMaxAllocatedBytes;

    // The size of the largest memory block allocated by the application.
    static size_t ms_uiMaxBlockSize;

    // Keep track of the number of allocated blocks of various sizes.  The
    // element Histogram[0] stores the number of allocated blocks of size 1.
    // The element Histogram[31] stores the number of allocated blocks of
    // size larger than pow(2,30).  For 1 <= i <= 30, the element Histogram[i]
    // stores the number of allocated blocks of size N, where
    // pow(2,i-1) < N <= pow(2,i).
    static size_t ms_auiHistogram[32];

// internal use
public:
    static void* Allocate (size_t uiSize, char* acFile, unsigned int uiLine,
        bool bIsArray);
    static void Deallocate (char* pcAddr, bool bIsArray);
    static void InsertBlock (Block* pkBlock);
    static void RemoveBlock (Block* pkBlock);
};

} //namespace Wm4

#include "Wm4Memory.inl"

#define WM4_NEW new(__FILE__,__LINE__)
#define WM4_DELETE delete

void* operator new (size_t uiSize);
void* operator new[](size_t uiSize);
void* operator new (size_t uiSize, char* acFile, unsigned int uiLine);
void* operator new[] (size_t uiSize, char* acFile, unsigned int uiLine);
void operator delete (void* pvAddr);
void operator delete[] (void* pvAddr);
void operator delete (void* pvAddr, char* acFile, unsigned int uiLine);
void operator delete[] (void* pvAddr, char* acFile, unsigned int uiLine);

#endif