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
inline EdgeKey::EdgeKey (int iV0, int iV1)
{
    if (iV0 < iV1)
    {
        // v0 is minimum
        V[0] = iV0;
        V[1] = iV1;
    }
    else
    {
        // v1 is minimum
        V[0] = iV1;
        V[1] = iV0;
    }
}
//----------------------------------------------------------------------------
inline bool EdgeKey::operator< (const EdgeKey& rkKey) const
{
    if (V[1] < rkKey.V[1])
    {
        return true;
    }

    if (V[1] > rkKey.V[1])
    {
        return false;
    }

    return V[0] < rkKey.V[0];
}
//----------------------------------------------------------------------------
inline EdgeKey::operator size_t () const
{
    return V[0] | (V[1] << 16);
}
//----------------------------------------------------------------------------
} //namespace Wm4
