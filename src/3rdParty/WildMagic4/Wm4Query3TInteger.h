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
#include "Wm4TInteger.h"

namespace Wm4
{

template <class Real>
class Query3TInteger : public Query3<Real>
{
public:
    // The components of the input vertices are truncated to 32-bit integer
    // values, so you should guarantee that the vertices are sufficiently
    // large to give a good distribution of numbers.  The value N in
    // TInteger<N> is chosen large enough so that the exact arithmetic is
    // correct for the functions.
    Query3TInteger (int iVQuantity, const Vector3<Real>* akVertex);

    // run-time type information
    virtual Query::Type GetType () const;

    // Queries about the relation of a point to various geometric objects.

    virtual int ToPlane (const Vector3<Real>& rkP, int iV0, int iV1, int iV2)
        const;

    virtual int ToCircumsphere (const Vector3<Real>& rkP, int iV0, int iV1,
        int iV2, int iV3) const;

private:
    using Query3<Real>::m_akVertex;

    static TInteger<3> Dot (TInteger<3>& rkX0, TInteger<3>& rkY0,
        TInteger<3>& rkZ0, TInteger<3>& rkX1, TInteger<3>& rkY1,
        TInteger<3>& rkZ1);

    static TInteger<4> Det3 (TInteger<4>& rkX0, TInteger<4>& rkY0,
        TInteger<4>& rkZ0, TInteger<4>& rkX1, TInteger<4>& rkY1,
        TInteger<4>& rkZ1, TInteger<4>& rkX2, TInteger<4>& rkY2,
        TInteger<4>& rkZ2);

    static TInteger<6> Det4 (TInteger<6>& rkX0, TInteger<6>& rkY0,
        TInteger<6>& rkZ0, TInteger<6>& rkW0, TInteger<6>& rkX1,
        TInteger<6>& rkY1, TInteger<6>& rkZ1, TInteger<6>& rkW1,
        TInteger<6>& rkX2, TInteger<6>& rkY2, TInteger<6>& rkZ2,
        TInteger<6>& rkW2, TInteger<6>& rkX3, TInteger<6>& rkY3,
        TInteger<6>& rkZ3, TInteger<6>& rkW3);
};

}

#include "Wm4Query3TInteger.inl"

namespace Wm4
{
typedef Query3TInteger<float> Query3TIntegerf;
typedef Query3TInteger<double> Query3TIntegerd;

}