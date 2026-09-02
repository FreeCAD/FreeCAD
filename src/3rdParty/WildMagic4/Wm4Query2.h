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
#include "Wm4Query.h"
#include "Wm4Vector2.h"

namespace Wm4
{

template <class Real>
class Query2 : public Query
{
public:
    // The base class handles floating-point queries.
    Query2 (int iVQuantity, const Vector2<Real>* akVertex);
    virtual ~Query2 ();

    // run-time type information
    virtual Query::Type GetType () const;

    // member access
    int GetQuantity () const;
    const Vector2<Real>* GetVertices () const;

    // Queries about the relation of a point to various geometric objects.

    // returns
    //   +1, on right of line
    //   -1, on left of line
    //    0, on the line
    virtual int ToLine (int i, int iV0, int iV1) const;
    virtual int ToLine (const Vector2<Real>& rkP, int iV0, int iV1) const;

    // returns
    //   +1, outside triangle
    //   -1, inside triangle
    //    0, on triangle
    virtual int ToTriangle (int i, int iV0, int iV1, int iV2) const;
    virtual int ToTriangle (const Vector2<Real>& rkP, int iV0, int iV1,
        int iV2) const;

    // returns
    //   +1, outside circumcircle of triangle
    //   -1, inside circumcircle of triangle
    //    0, on circumcircle of triangle
    virtual int ToCircumcircle (int i, int iV0, int iV1, int iV2) const;
    virtual int ToCircumcircle (const Vector2<Real>& rkP, int iV0, int iV1,
        int iV2) const;

protected:
    // input points
    int m_iVQuantity;
    const Vector2<Real>* m_akVertex;

    static Real Dot (Real fX0, Real fY0, Real fX1, Real fY1);

    static Real Det2 (Real fX0, Real fY0, Real fX1, Real fY1);

    static Real Det3 (Real iX0, Real iY0, Real iZ0, Real iX1, Real iY1,
        Real iZ1, Real iX2, Real iY2, Real iZ2);
};

}

#include "Wm4Query2.inl"

namespace Wm4
{
typedef Query2<float> Query2f;
typedef Query2<double> Query2d;
}