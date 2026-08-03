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
#include "Wm4TRational.h"

namespace Wm4
{

template <int VSIZE, int ISIZE>
class TRVector
{
public:
    // construction
    TRVector ();
    TRVector (const TRVector& rkV);

    // coordinate access
    operator const TRational<ISIZE>* () const;
    operator TRational<ISIZE>* ();
    TRational<ISIZE> operator[] (int i) const;
    TRational<ISIZE>& operator[] (int i);

    // assignment
    TRVector& operator= (const TRVector& rkV);

    // comparison
    bool operator== (const TRVector& rkV) const;
    bool operator!= (const TRVector& rkV) const;
    bool operator<  (const TRVector& rkV) const;
    bool operator<= (const TRVector& rkV) const;
    bool operator>  (const TRVector& rkV) const;
    bool operator>= (const TRVector& rkV) const;

    // arithmetic operations
    TRVector operator+ (const TRVector& rkV) const;
    TRVector operator- (const TRVector& rkV) const;
    TRVector operator* (const TRational<ISIZE>& rkR) const;
    TRVector operator/ (const TRational<ISIZE>& rkR) const;
    TRVector operator- () const;

    // arithmetic updates
    TRVector& operator+= (const TRVector& rkV);
    TRVector& operator-= (const TRVector& rkV);
    TRVector& operator*= (const TRational<ISIZE>& rkR);
    TRVector& operator/= (const TRational<ISIZE>& rkR);

    // vector operations
    TRational<ISIZE> SquaredLength () const;
    TRational<ISIZE> Dot (const TRVector& rkV) const;

protected:
    // support for comparisons
    int CompareArrays (const TRVector& rkV) const;

    TRational<ISIZE> m_akTuple[VSIZE];
};

template <int VSIZE, int ISIZE>
TRVector<VSIZE,ISIZE> operator* (const TRational<ISIZE>& rkR,
    const TRVector<VSIZE,ISIZE>& rkV);

}

#include "Wm4TRVector.inl"