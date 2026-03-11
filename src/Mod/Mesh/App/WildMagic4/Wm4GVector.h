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
#include "Wm4Math.h"

namespace Wm4
{

template <class Real>
class GVector
{
public:
    // construction
    GVector (int iSize = 0);
    GVector (int iSize, const Real* afTuple);
    GVector (const GVector& rkV);
    ~GVector ();

    // coordinate access
    void SetSize (int iSize);
    int GetSize () const;
    operator const Real* () const;
    operator Real* ();
    Real operator[] (int i) const;
    Real& operator[] (int i);

    // assignment
    GVector& operator= (const GVector& rkV);

    // comparison
    bool operator== (const GVector& rkV) const;
    bool operator!= (const GVector& rkV) const;
    bool operator<  (const GVector& rkV) const;
    bool operator<= (const GVector& rkV) const;
    bool operator>  (const GVector& rkV) const;
    bool operator>= (const GVector& rkV) const;

    // arithmetic operations
    GVector operator+ (const GVector& rkV) const;
    GVector operator- (const GVector& rkV) const;
    GVector operator* (Real fScalar) const;
    GVector operator/ (Real fScalar) const;
    GVector operator- () const;

    // arithmetic updates
    GVector& operator+= (const GVector& rkV);
    GVector& operator-= (const GVector& rkV);
    GVector& operator*= (Real fScalar);
    GVector& operator/= (Real fScalar);

    // vector operations
    Real Length () const;
    Real SquaredLength () const;
    Real Dot (const GVector& rkV) const;
    Real Normalize ();

protected:
    // support for comparisons
    int CompareArrays (const GVector& rkV) const;

    int m_iSize;
    Real* m_afTuple;
};

template <class Real>
GVector<Real> operator* (Real fScalar, const GVector<Real>& rkV);

} //namespace Wm4

#include "Wm4GVector.inl"

namespace Wm4
{
typedef GVector<float> GVectorf;
typedef GVector<double> GVectord;
}