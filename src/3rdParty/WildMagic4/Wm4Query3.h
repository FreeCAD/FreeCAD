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
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Query3 : public Query
{
public:
    // The base class handles floating-point queries.
    Query3 (int iVQuantity, const Vector3<Real>* akVertex);
    virtual ~Query3 ();

    // run-time type information
    virtual Query::Type GetType () const;

    // member access
    int GetQuantity () const;
    const Vector3<Real>* GetVertices () const;

    // Queries about the relation of a point to various geometric objects.

    // returns
    //   +1, on positive side of plane
    //   -1, on negative side of line
    //    0, on the plane
    virtual int ToPlane (int i, int iV0, int iV1, int iV2) const;
    virtual int ToPlane (const Vector3<Real>& rkP, int iV0, int iV1, int iV2)
        const;

    // returns
    //   +1, outside tetrahedron
    //   -1, inside tetrahedron
    //    0, on tetrahedron
    virtual int ToTetrahedron (int i, int iV0, int iV1, int iV2, int iV3)
        const;
    virtual int ToTetrahedron (const Vector3<Real>& rkP, int iV0, int iV1,
        int iV2, int iV3) const;

    // returns
    //   +1, outside circumsphere of tetrahedron
    //   -1, inside circumsphere of tetrahedron
    //    0, on circumsphere of tetrahedron
    virtual int ToCircumsphere (int i, int iV0, int iV1, int iV2, int iV3)
        const;
    virtual int ToCircumsphere (const Vector3<Real>& rkP, int iV0, int iV1,
        int iV2, int iV3) const;

protected:
    // input points
    int m_iVQuantity;
    const Vector3<Real>* m_akVertex;

    static Real Dot (Real fX0, Real fY0, Real fZ0, Real fX1, Real fY1,
        Real fZ1);

    static Real Det3 (Real fX0, Real fY0, Real fZ0, Real fX1, Real fY1,
        Real fZ1, Real fX2, Real fY2, Real fZ2);

    static Real Det4 (Real fX0, Real fY0, Real fZ0, Real fW0, Real fX1,
        Real fY1, Real fZ1, Real fW1, Real fX2, Real fY2, Real fZ2, Real fW2,
        Real fX3, Real fY3, Real fZ3, Real fW3);
};

}

#include "Wm4Query3.inl"

namespace Wm4
{
typedef Query3<float> Query3f;
typedef Query3<double> Query3d;

}