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
#include "Wm4TInteger.h"

namespace Wm4
{

// N is the number of 32-bit words per TInteger numerator/denominator
template <int N>
class TRational
{
public:
    // construction
    TRational ();  // default rational is 0/1
    TRational (const TRational& rkR);
    TRational (const TInteger<N>& rkNumer);
    TRational (const TInteger<N>& rkNumer, const TInteger<N>& rkDenom);

    // construction converters
    TRational (int iNumer);
    TRational (int iNumer, int iDenom);
    TRational (float fValue);
    TRational (double dValue);

    // member access
    TInteger<N>& Numer ();
    TInteger<N>& Denom ();
    const TInteger<N>& Numer () const;
    const TInteger<N>& Denom () const;

    // assignment
    TRational& operator= (const TRational& rkR);

    // comparison
    bool operator== (const TRational& rkR) const;
    bool operator!= (const TRational& rkR) const;
    bool operator<= (const TRational& rkR) const;
    bool operator<  (const TRational& rkR) const;
    bool operator>= (const TRational& rkR) const;
    bool operator>  (const TRational& rkR) const;

    // arithmetic operations
    TRational operator+ (const TRational& rkR) const;
    TRational operator- (const TRational& rkR) const;
    TRational operator* (const TRational& rkR) const;
    TRational operator/ (const TRational& rkR) const;
    TRational operator- () const;

    // arithmetic updates
    TRational& operator+= (const TRational& rkR);
    TRational& operator-= (const TRational& rkR);
    TRational& operator*= (const TRational& rkR);
    TRational& operator/= (const TRational& rkR);
#if 0
    // conversions to float and double
    void ConvertTo (float& rfValue) const;
    void ConvertTo (double& rdValue) const;
#endif
    // compute the absolute value of the rational number
    TRational Abs () const;

private:
    // miscellaneous utilities
    void EliminatePowersOfTwo ();

    static void GetPositiveFloat (const TInteger<N>& rkDenom,
        TInteger<N>& rkQuo, TInteger<N>& rkRem, int iBlock,
        unsigned int& ruiExponent, unsigned int& ruiMantissa);

    static void GetPositiveDouble (const TInteger<N>& rkDenom,
        TInteger<N>& rkQuo, TInteger<N>& rkRem, int iBlock,
        unsigned int& ruiExponent, unsigned int& ruiMantissaHi,
        unsigned int& ruiMantissaLo);

    TInteger<N> m_kNumer, m_kDenom;
};

template <int N>
TRational<N> operator+ (const TInteger<N>& rkI, const TRational<N>& rkR);

template <int N>
TRational<N> operator- (const TInteger<N>& rkI, const TRational<N>& rkR);

template <int N>
TRational<N> operator* (const TInteger<N>& rkI, const TRational<N>& rkR);

template <int N>
TRational<N> operator/ (const TInteger<N>& rkI, const TRational<N>& rkR);

}

#include "Wm4TRational.inl"