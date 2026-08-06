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
// Version: 4.0.1 (2006/07/25)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Math.h"

namespace Wm4
{

template <class Real>
class Vector4
{
public:
    // construction
    Vector4 ();  // uninitialized
    Vector4 (Real fX, Real fY, Real fZ, Real fW);
    Vector4 (const Real* afTuple);
    Vector4 (const Vector4& rkV);

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
    Real W () const;
    Real& W ();

    // assignment
    Vector4& operator= (const Vector4& rkV);

    // comparison
    bool operator== (const Vector4& rkV) const;
    bool operator!= (const Vector4& rkV) const;
    bool operator<  (const Vector4& rkV) const;
    bool operator<= (const Vector4& rkV) const;
    bool operator>  (const Vector4& rkV) const;
    bool operator>= (const Vector4& rkV) const;

    // arithmetic operations
    Vector4 operator+ (const Vector4& rkV) const;
    Vector4 operator- (const Vector4& rkV) const;
    Vector4 operator* (Real fScalar) const;
    Vector4 operator/ (Real fScalar) const;
    Vector4 operator- () const;

    // arithmetic updates
    Vector4& operator+= (const Vector4& rkV);
    Vector4& operator-= (const Vector4& rkV);
    Vector4& operator*= (Real fScalar);
    Vector4& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const Vector4& rkV) const;
    Real Normalize ();

    // special vectors
    WM4_FOUNDATION_ITEM static const Vector4 ZERO;
    WM4_FOUNDATION_ITEM static const Vector4 UNIT_X;  // (1,0,0,0)
    WM4_FOUNDATION_ITEM static const Vector4 UNIT_Y;  // (0,1,0,0)
    WM4_FOUNDATION_ITEM static const Vector4 UNIT_Z;  // (0,0,1,0)
    WM4_FOUNDATION_ITEM static const Vector4 UNIT_W;  // (0,0,0,1)
    WM4_FOUNDATION_ITEM static const Vector4 ONE;     // (1,1,1,1)

private:
    // support for comparisons
    int CompareArrays (const Vector4& rkV) const;

    Real m_afTuple[4];
};

// arithmetic operations
template <class Real>
Vector4<Real> operator* (Real fScalar, const Vector4<Real>& rkV);

// debugging output
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector4<Real>& rkV);

}

#include "Wm4Vector4.inl"

namespace Wm4
{
typedef Vector4<float> Vector4f;
typedef Vector4<double> Vector4d;

}