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

#include "Wm4FoundationLIB.h"
#include "Wm4Query3.h"

namespace Wm4
{

template <class Real>
class Query3Int64 : public Query3<Real>
{
public:
    // The components of the input vertices are truncated to 64-bit integer
    // values, so you should guarantee that the vertices are sufficiently
    // large to give a good distribution of numbers.
    Query3Int64 (int iVQuantity, const Vector3<Real>* akVertex);

    // run-time type information
    virtual Query::Type GetType () const;

    // Queries about the relation of a point to various geometric objects.

    virtual int ToPlane (const Vector3<Real>& rkP, int iV0, int iV1, int iV2)
        const;

    virtual int ToCircumsphere (const Vector3<Real>& rkP, int iV0, int iV1,
        int iV2, int iV3) const;

private:
    using Query3<Real>::m_akVertex;

    static Integer64 Dot (Integer64 iX0, Integer64 iY0, Integer64 iZ0,
        Integer64 iX1, Integer64 iY1, Integer64 iZ1);

    static Integer64 Det3 (Integer64 iX0, Integer64 iY0, Integer64 iZ0,
        Integer64 iX1, Integer64 iY1, Integer64 iZ1, Integer64 iX2,
        Integer64 iY2, Integer64 iZ2);

    static Integer64 Det4 (Integer64 iX0, Integer64 iY0, Integer64 iZ0,
        Integer64 iW0, Integer64 iX1, Integer64 iY1, Integer64 iZ1,
        Integer64 iW1, Integer64 iX2, Integer64 iY2, Integer64 iZ2,
        Integer64 iW2, Integer64 iX3, Integer64 iY3, Integer64 iZ3,
        Integer64 iW3);
};

}

#include "Wm4Query3Int64.inl"

namespace Wm4
{
typedef Query3Int64<float> Query3Int64f;
typedef Query3Int64<double> Query3Int64d;

}