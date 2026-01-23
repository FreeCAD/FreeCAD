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
#include "Wm4Query2.h"
#include "Wm4TInteger.h"

namespace Wm4
{

template <class Real>
class Query2TInteger : public Query2<Real>
{
public:
    // The components of the input vertices are truncated to 32-bit integer
    // values, so you should guarantee that the vertices are sufficiently
    // large to give a good distribution of numbers.  The value N in
    // TInteger<N> is chosen large enough so that the exact arithmetic is
    // correct for the functions.
    Query2TInteger (int iVQuantity, const Vector2<Real>* akVertex);

    // run-time type information
    virtual Query::Type GetType () const;

    // Queries about the relation of a point to various geometric objects.

    virtual int ToLine (const Vector2<Real>& rkP, int iV0, int iV1) const;

    virtual int ToCircumcircle (const Vector2<Real>& rkP, int iV0, int iV1,
        int iV2) const;

private:
    using Query2<Real>::m_akVertex;

    static TInteger<2> Dot (TInteger<2>& rkX0, TInteger<2>& rkY0,
        TInteger<2>& rkX1, TInteger<2>& rkY1);

    static TInteger<2> Det2 (TInteger<2>& rkX0, TInteger<2>& rkY0,
        TInteger<2>& rkX1, TInteger<2>& rkY1);

    static TInteger<4> Det3 (TInteger<4>& rkX0, TInteger<4>& rkY0,
        TInteger<4>& rkZ0, TInteger<4>& rkX1, TInteger<4>& rkY1,
        TInteger<4>& rkZ1, TInteger<4>& rkX2, TInteger<4>& rkY2,
        TInteger<4>& rkZ2);
};

}

#include "Wm4Query2TInteger.inl"

namespace Wm4
{
typedef Query2TInteger<float> Query2TIntegerf;
typedef Query2TInteger<double> Query2TIntegerd;

}