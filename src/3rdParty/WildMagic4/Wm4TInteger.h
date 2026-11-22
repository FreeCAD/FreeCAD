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
#include "Wm4System.h"

namespace Wm4
{

template <int N> class TRational;

// N is the number of 32-bit words you want per TInteger.
template <int N>
class TInteger
{
public:
    // construction and destruction
    TInteger (int i = 0);
    TInteger (const TInteger& rkI);
    ~TInteger ();

    // assignment
    TInteger& operator= (const TInteger& rkI);

    // comparison
    bool operator== (const TInteger& rkI) const;
    bool operator!= (const TInteger& rkI) const;
    bool operator<  (const TInteger& rkI) const;
    bool operator<= (const TInteger& rkI) const;
    bool operator>  (const TInteger& rkI) const;
    bool operator>= (const TInteger& rkI) const;

    // arithmetic operations
    TInteger operator- () const;
    TInteger operator+ (const TInteger& rkI) const;
    TInteger operator- (const TInteger& rkI) const;
    TInteger operator* (const TInteger& rkI) const;
    TInteger operator/ (const TInteger& rkI) const;
    TInteger operator% (const TInteger& rkI) const;

    // arithmetic updates
    TInteger& operator+= (const TInteger& rkI);
    TInteger& operator-= (const TInteger& rkI);
    TInteger& operator*= (const TInteger& rkI);
    TInteger& operator/= (const TInteger& rkI);

    // shift operations
    TInteger operator<< (int iShift) const;
    TInteger operator>> (int iShift) const;

    // shift updates
    TInteger& operator<<= (int iShift);
    TInteger& operator>>= (int iShift);

private:
    // Support for comparisons.  The return value of Compare is -1 if I0 < I1,
    // is 0 if I0 == I1, or is +1 if I0 > I1.
    static int Compare (const TInteger& rkI0, const TInteger& rkI1);
    int GetSign () const;

    // support for division and modulo
    static bool GetDivMod (const TInteger& rkNumer, const TInteger& rkDenom,
        TInteger& rkQuotient, TInteger& rkRemainder);

    static void DivSingle (const TInteger& rkNumer, short usDenom,
        TInteger& rkQuo, TInteger& rkRem);

    static void DivMultiple (const TInteger& rkNumer, const TInteger& rkDenom,
        TInteger& rkQuo, TInteger& rkRem);

    // miscellaneous utilities
    int GetLeadingBlock () const;
    int GetTrailingBlock () const;
    int GetLeadingBit (int i) const;  // of m_asBuffer[i]
    int GetTrailingBit (int i) const;  // of m_asBuffer[i]
    int GetLeadingBit () const;  // of entire number
    int GetTrailingBit () const;  // of entire number
    void SetBit (int i, bool bOn);
    bool GetBit (int i) const;
    unsigned int ToUnsignedInt (int i) const;
    void FromUnsignedInt (int i, unsigned int uiValue);
    unsigned int ToUnsignedInt (int iLo, int iHi) const;
    int ToInt (int i) const;

    enum
    {
        TINT_SIZE = 2*N,
        TINT_BYTES = TINT_SIZE*sizeof(short),
        TINT_LAST = TINT_SIZE-1
    };

    short m_asBuffer[TINT_SIZE];

    // TRational needs access to private members of TInteger.
    friend class TRational<N>;
};

template <int N>
TInteger<N> operator* (int i, const TInteger<N>& rkI);

}

#include "Wm4TInteger.inl"