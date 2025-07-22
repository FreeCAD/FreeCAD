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

namespace Wm4
{
//----------------------------------------------------------------------------
inline size_t& Memory::MaxAllowedBytes ()
{
    return ms_uiMaxAllowedBytes;
}
//----------------------------------------------------------------------------
inline bool& Memory::TrackSizes ()
{
    return ms_bTrackSizes;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetNumNewCalls ()
{
    return ms_uiNumNewCalls;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetNumDeleteCalls ()
{
    return ms_uiNumDeleteCalls;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetNumBlocks ()
{
    return ms_uiNumBlocks;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetNumBytes ()
{
    return ms_uiNumBytes;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetMaxAllocatedBytes ()
{
    return ms_uiMaxAllocatedBytes;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetMaxBlockSize ()
{
    return ms_uiMaxBlockSize;
}
//----------------------------------------------------------------------------
inline size_t Memory::GetHistogram (int i)
{
    if (0 <= i && i <= 31)
    {
        return ms_auiHistogram[i];
    }

    return 0;
}
//----------------------------------------------------------------------------
inline const Memory::Block* Memory::GetHead ()
{
    return ms_pkHead;
}
//----------------------------------------------------------------------------
inline const Memory::Block* Memory::GetTail ()
{
    return ms_pkTail;
}
//----------------------------------------------------------------------------
} //namespace Wm4
