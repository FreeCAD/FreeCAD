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

#include "Wm4FoundationPCH.h"

#ifdef WM4_MEMORY_MANAGER

// Enable this #define if you want to trap mismatches of new/delete[] or
// new[]/delete.  Xcode 2.3 has mismatches of this form in its STL.  For
// example, Program::Load creates a std::ifstream called kIStr.  At the end
// of that function call, kIStr.close() triggers an assert in the function
// Memory::Deallocate because pkBlock->IsArray is 'false' but 'bIsArray' is
// true.  This implies a mismatch of new/delete[].
//#define WM4_ENABLE_NEW_DELETE_MISMATCH_ASSERT

// Enable this #define the consistency checks in Memory::GenerateReport.
// Xcode 2.3 triggers the last assert in this function.  It appears that
// Xcode's standard libraries have lots of leaks.
//#define WM4_ENABLE_CONSISTENCY_CHECK

#include <cassert>
#include <cstdlib>
#include <fstream>
#include "Wm4Memory.h"
using namespace Wm4;

size_t Memory::ms_uiNumNewCalls = 0;
size_t Memory::ms_uiNumDeleteCalls = 0;
size_t Memory::ms_uiMaxAllowedBytes = 0;
size_t Memory::ms_uiNumBlocks = 0;
size_t Memory::ms_uiNumBytes = 0;
Memory::Block* Memory::ms_pkHead = 0;
Memory::Block* Memory::ms_pkTail = 0;
bool Memory::ms_bTrackSizes = false;
size_t Memory::ms_uiMaxAllocatedBytes = 0;
size_t Memory::ms_uiMaxBlockSize = 0;
size_t Memory::ms_auiHistogram[32] =
{
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

//----------------------------------------------------------------------------
void* Memory::Allocate (size_t uiSize, char* acFile, unsigned int uiLine,
    bool bIsArray)
{
    ms_uiNumNewCalls++;

    // The 'assert' used to be enabled to trap attempts to allocate zero
    // bytes.  However, the DirectX function D3DXCheckNewDelete may pass in
    // a value of zero.
    // assert(uiSize > 0);

    // Allocate additional storage for the block header information.
    size_t uiExtendedSize = sizeof(Block) + uiSize;
    char* pcAddr = (char*)malloc(uiExtendedSize);

    // Save the allocation information.
    Block* pkBlock = (Block*)pcAddr;
    pkBlock->Size = uiSize;
    pkBlock->File = acFile;
    pkBlock->Line = uiLine;
    pkBlock->IsArray = bIsArray;
    InsertBlock(pkBlock);

    // Move the pointer to the start of what the user expects from 'new'.
    pcAddr += sizeof(Block);

    // Keep track of number of allocated blocks and bytes.
    ms_uiNumBlocks++;
    ms_uiNumBytes += uiSize;

    if (ms_uiMaxAllowedBytes > 0 && ms_uiNumBytes > ms_uiMaxAllowedBytes)
    {
        // The allocation has exceeded the maximum number of bytes.
        assert(false);
    }

    // Keep track of the maximum number of bytes allocated.
    if (ms_uiNumBytes > ms_uiMaxAllocatedBytes)
    {
        ms_uiMaxAllocatedBytes = ms_uiNumBytes;
    }

    // Keep track of the distribution of sizes for allocations.
    if (ms_bTrackSizes)
    {
        // Keep track of the largest block ever allocated.
        if (uiSize > ms_uiMaxBlockSize)
        {
            ms_uiMaxBlockSize = uiSize;
        }

        unsigned int uiTwoPowerI = 1;
        int i;
        for (i = 0; i <= 30; i++, uiTwoPowerI <<= 1)
        {
            if (uiSize <= uiTwoPowerI)
            {
                ms_auiHistogram[i]++;
                break;
            }
        }
        if (i == 31)
        {
            ms_auiHistogram[i]++;
        }
    }

    return (void*)pcAddr;
}
//----------------------------------------------------------------------------
void Memory::Deallocate (char* pcAddr, bool bIsArray)
{
    ms_uiNumDeleteCalls++;

    if (!pcAddr)
    {
        return;
    }

    // Move the pointer to the start of the actual allocated block.
    pcAddr -= sizeof(Block);

    // Get the allocation information and remove the block.  The removal
    // only modifies the Prev and Next pointers, so the block information is
    // accessible after the call.
    Block* pkBlock = (Block*)pcAddr;
    RemoveBlock(pkBlock);

#ifdef WM4_ENABLE_NEW_DELETE_MISMATCH_ASSERT
    // Check for correct pairing of new/delete or new[]/delete[].
    assert(pkBlock->IsArray == bIsArray);
#endif

    // Keep track of number of allocated blocks and bytes.  If the number of
    // blocks is zero at this time, a delete has been called twice on the
    // same pointer.  If the number of bytes is too small at this time, some
    // internal problem has occurred within this class and needs to be
    // diagnosed.
    assert(ms_uiNumBlocks > 0 && ms_uiNumBytes >= pkBlock->Size);
    ms_uiNumBlocks--;
    ms_uiNumBytes -= pkBlock->Size;

    // Deallocate the memory block.
    free(pcAddr);
}
//----------------------------------------------------------------------------
void Memory::InsertBlock (Block* pkBlock)
{
    // New blocks are inserted at the tail of the doubly linked list.
    if (ms_pkTail)
    {
        pkBlock->Prev = ms_pkTail;
        pkBlock->Next = 0;
        ms_pkTail->Next = pkBlock;
        ms_pkTail = pkBlock;
    }
    else
    {
        pkBlock->Prev = 0;
        pkBlock->Next = 0;
        ms_pkHead = pkBlock;
        ms_pkTail = pkBlock;
    }
}
//----------------------------------------------------------------------------
void Memory::RemoveBlock (Block* pkBlock)
{
    if (pkBlock->Prev)
    {
        pkBlock->Prev->Next = pkBlock->Next;
    }
    else
    {
        ms_pkHead = pkBlock->Next;
    }
    
    if (pkBlock->Next)
    {
        pkBlock->Next->Prev = pkBlock->Prev;
    }
    else
    {
        ms_pkTail = pkBlock->Prev;
    }
}
//----------------------------------------------------------------------------
void Memory::GenerateReport (const char* acFilename)
{
    std::ofstream kOStr(acFilename);
    assert(kOStr);
    if (!kOStr)
    {
        return;
    }

    // Total calls.
    kOStr << "Total number of 'new' calls = "
        << (unsigned int)ms_uiNumNewCalls << std::endl;
    kOStr << "Total number of 'delete' calls = "
        << (unsigned int)ms_uiNumDeleteCalls << std::endl;
    kOStr << "Maximum number of allocated bytes = "
        << (unsigned int)ms_uiMaxAllocatedBytes << std::endl << std::endl;

    // Remaining counts.
    kOStr << "Remaining number of blocks = "
        << (unsigned int)ms_uiNumBlocks << std::endl;
    kOStr << "Remaining number of bytes  = "
        << (unsigned int)ms_uiNumBytes << std::endl << std::endl;

    // Count the blocks and bytes from known and unknown sources.
    size_t uiNumKnownBlocks = 0;
    size_t uiNumKnownBytes = 0;
    size_t uiNumUnknownBlocks = 0;
    size_t uiNumUnknownBytes = 0;
    Block* pkBlock = ms_pkHead;
    while (pkBlock)
    {
        if (pkBlock->File)
        {
            uiNumKnownBlocks++;
            uiNumKnownBytes += pkBlock->Size;
        }
        else
        {
            uiNumUnknownBlocks++;
            uiNumUnknownBytes += pkBlock->Size;
        }
        pkBlock = pkBlock->Next;
    }

#ifdef WM4_ENABLE_CONSISTENCY_CHECK
    // consistency check
    assert(uiNumKnownBlocks + uiNumUnknownBlocks == ms_uiNumBlocks);
    assert(uiNumKnownBytes + uiNumUnknownBytes == ms_uiNumBytes);
#endif

    kOStr << "Remaining number of known blocks = "
        << (unsigned int)uiNumKnownBlocks << std::endl;
    kOStr << "Remaining number of known bytes  = "
        << (unsigned int)uiNumKnownBytes << std::endl << std::endl;

    kOStr << "Remaining number of unknown blocks = "
        << (unsigned int)uiNumUnknownBlocks << std::endl;
    kOStr << "Remaining number of unknown bytes  = "
        << (unsigned int)uiNumUnknownBytes << std::endl << std::endl;

    // Report the information for each block.
    pkBlock = ms_pkHead;
    size_t uiIndex = 0;
    while (pkBlock)
    {
        kOStr << "block = " << (unsigned int)uiIndex << std::endl;
        kOStr << "size  = " << (unsigned int)pkBlock->Size << std::endl;
        if (pkBlock->File)
        {
            kOStr << "file  = " << pkBlock->File << std::endl;
            kOStr << "line  = " << pkBlock->Line << std::endl;
        }
        else
        {
            kOStr << "file  = unknown" << std::endl;
            kOStr << "line  = unknown" << std::endl;
        }
        kOStr << "array = " << pkBlock->IsArray << std::endl << std::endl;
        pkBlock = pkBlock->Next;
        uiIndex++;
    }
    kOStr.close();

#ifdef WM4_ENABLE_CONSISTENCY_CHECK
    // consistency check
    assert(uiIndex == ms_uiNumBlocks);
#endif
}
//----------------------------------------------------------------------------
void* operator new (size_t uiSize)
{
    return Memory::Allocate(uiSize,0,0,false);
}
//----------------------------------------------------------------------------
void* operator new[](size_t uiSize)
{
    return Memory::Allocate(uiSize,0,0,true);
}
//----------------------------------------------------------------------------
void* operator new (size_t uiSize, char* acFile, unsigned int uiLine)
{
    return Memory::Allocate(uiSize,acFile,uiLine,false);
}
//----------------------------------------------------------------------------
void* operator new[] (size_t uiSize, char* acFile, unsigned int uiLine)
{
    return Memory::Allocate(uiSize,acFile,uiLine,true);
}
//----------------------------------------------------------------------------
void operator delete (void* pvAddr)
{
    Memory::Deallocate((char*)pvAddr,false);
}
//----------------------------------------------------------------------------
void operator delete[] (void* pvAddr)
{
    Memory::Deallocate((char*)pvAddr,true);
}
//----------------------------------------------------------------------------
void operator delete (void* pvAddr, char*, unsigned int)
{
    Memory::Deallocate((char*)pvAddr,false);
}
//----------------------------------------------------------------------------
void operator delete[] (void* pvAddr, char*, unsigned int)
{
    Memory::Deallocate((char*)pvAddr,true);
}
//----------------------------------------------------------------------------

#endif
