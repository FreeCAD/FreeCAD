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
// Version: 4.0.2 (2006/07/25)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Math.h"

namespace Wm4
{

template <class Real>
class Vector3
{
public:
    // construction
    Vector3 ();  // uninitialized
    Vector3 (Real fX, Real fY, Real fZ);
    Vector3 (const Real* afTuple);
    Vector3 (const Vector3& rkV);

    // coordinate access
    operator const Real* () const;
    operator Real* ();
    Real operator[] (int i) const;
    Real& operator[] (int i);
    Real X () const;
    Real& X ();
    Real Y () const;
    Real& Y ();
    Real Z () const;
    Real& Z ();

    // assignment
    Vector3& operator= (const Vector3& rkV);

    // comparison
    bool operator== (const Vector3& rkV) const;
    bool operator!= (const Vector3& rkV) const;
    bool operator<  (const Vector3& rkV) const;
    bool operator<= (const Vector3& rkV) const;
    bool operator>  (const Vector3& rkV) const;
    bool operator>= (const Vector3& rkV) const;

    // arithmetic operations
    Vector3 operator+ (const Vector3& rkV) const;
    Vector3 operator- (const Vector3& rkV) const;
    Vector3 operator* (Real fScalar) const;
    Vector3 operator/ (Real fScalar) const;
    Vector3 operator- () const;

    // arithmetic updates
    Vector3& operator+= (const Vector3& rkV);
    Vector3& operator-= (const Vector3& rkV);
    Vector3& operator*= (Real fScalar);
    Vector3& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector3& rkV) const;
    Real Normalize ();

    // The cross products are computed using the right-handed rule.  Be aware
    // that some graphics APIs use a left-handed rule.  If you have to compute
    // a cross product with these functions and send the result to the API
    // that expects left-handed, you will need to change sign on the vector
    // (replace each component value c by -c).
    Vector3 Cross (const Vector3& rkV) const;
    Vector3 UnitCross (const Vector3& rkV) const;

    // Compute the barycentric coordinates of the point with respect to the
    // tetrahedron <V0,V1,V2,V3>, P = b0*V0 + b1*V1 + b2*V2 + b3*V3, where
    // b0 + b1 + b2 + b3 = 1.
    void GetBarycentrics (const Vector3& rkV0, const Vector3& rkV1,
        const Vector3& rkV2, const Vector3& rkV3, Real afBary[4]) const;

    // Gram-Schmidt orthonormalization.  Take linearly independent vectors
    // U, V, and W and compute an orthonormal set (unit length, mutually
    // perpendicular).
    static void Orthonormalize (Vector3& rkU, Vector3& rkV, Vector3& rkW);
    static void Orthonormalize (Vector3* akV);

    // Input W must be a nonzero vector. The output is an orthonormal basis
    // {U,V,W}.  The input W is normalized by this function.  If you know
    // W is already unit length, use GenerateComplementBasis to compute U
    // and V.
    static void GenerateOrthonormalBasis (Vector3& rkU, Vector3& rkV,
        Vector3& rkW);

    // Input W must be a unit-length vector.  The output vectors {U,V} are
    // unit length and mutually perpendicular, and {U,V,W} is an orthonormal
    // basis.
    static void GenerateComplementBasis (Vector3& rkU, Vector3& rkV,
        const Vector3& rkW);

    // Compute the extreme values.
    static void ComputeExtremes (int iVQuantity, const Vector3* akPoint,
        Vector3& rkMin, Vector3& rkMax);

    // special vectors
    WM4_FOUNDATION_ITEM static const Vector3 ZERO;    // (0,0,0)
    WM4_FOUNDATION_ITEM static const Vector3 UNIT_X;  // (1,0,0)
    WM4_FOUNDATION_ITEM static const Vector3 UNIT_Y;  // (0,1,0)
    WM4_FOUNDATION_ITEM static const Vector3 UNIT_Z;  // (0,0,1)
    WM4_FOUNDATION_ITEM static const Vector3 ONE;     // (1,1,1)

private:
    // support for comparisons
    int CompareArrays (const Vector3& rkV) const;

    Real m_afTuple[3];
};

// arithmetic operations
template <class Real>
Vector3<Real> operator* (Real fScalar, const Vector3<Real>& rkV);

// debugging output
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector3<Real>& rkV);

}

#include "Wm4Vector3.inl"

namespace Wm4
{
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

}