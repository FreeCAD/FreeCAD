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

namespace Wm4
{
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational ()
    :
    m_kNumer(0),
    m_kDenom(1)
{
    // default construction produces the zero rational number
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (const TRational& rkR)
    :
    m_kNumer(rkR.m_kNumer),
    m_kDenom(rkR.m_kDenom)
{
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (const TInteger<N>& rkNumer)
    :
    m_kNumer(rkNumer),
    m_kDenom(1)
{
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (const TInteger<N>& rkNumer,
    const TInteger<N>& rkDenom)
    :
    m_kNumer(rkNumer),
    m_kDenom(rkDenom)
{
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (int iNumer)
    :
    m_kNumer(iNumer),
    m_kDenom(1)
{
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (int iNumer, int iDenom)
    :
    m_kNumer(iNumer),
    m_kDenom(iDenom)
{
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TRational<N>::Numer ()
{
    return m_kNumer;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TRational<N>::Denom ()
{
    return m_kDenom;
}
//----------------------------------------------------------------------------
template <int N>
const TInteger<N>& TRational<N>::Numer () const
{
    return m_kNumer;
}
//----------------------------------------------------------------------------
template <int N>
const TInteger<N>& TRational<N>::Denom () const
{
    return m_kDenom;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>& TRational<N>::operator= (const TRational& rkR)
{
    m_kNumer = rkR.m_kNumer;
    m_kDenom = rkR.m_kDenom;
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator== (const TRational& rkR) const
{
    return (m_kNumer*rkR.m_kDenom == m_kDenom*rkR.m_kNumer);
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator!= (const TRational& rkR) const
{
    return (m_kNumer*rkR.m_kDenom != m_kDenom*rkR.m_kNumer);
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator<= (const TRational& rkR) const
{
    TInteger<N> kProd0 = m_kNumer*rkR.m_kDenom;
    TInteger<N> kProd1 = m_kDenom*rkR.m_kNumer;
    if (m_kDenom > 0)
    {
        return (rkR.m_kDenom > 0 ? kProd0 <= kProd1 : kProd0 >= kProd1);
    }
    else
    {
        return (rkR.m_kDenom > 0 ? kProd0 >= kProd1 : kProd0 <= kProd1);
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator< (const TRational& rkR) const
{
    TInteger<N> kProd0 = m_kNumer*rkR.m_kDenom;
    TInteger<N> kProd1 = m_kDenom*rkR.m_kNumer;
    if (m_kDenom > 0)
    {
        return (rkR.m_kDenom > 0 ? kProd0 < kProd1 : kProd0 > kProd1);
    }
    else
    {
        return (rkR.m_kDenom > 0 ? kProd0 > kProd1 : kProd0 < kProd1);
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator>= (const TRational& rkR) const
{
    TInteger<N> kProd0 = m_kNumer*rkR.m_kDenom;
    TInteger<N> kProd1 = m_kDenom*rkR.m_kNumer;
    if (m_kDenom > 0)
    {
        return (rkR.m_kDenom > 0 ? kProd0 >= kProd1 : kProd0 <= kProd1);
    }
    else
    {
        return (rkR.m_kDenom > 0 ? kProd0 <= kProd1 : kProd0 >= kProd1);
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TRational<N>::operator> (const TRational& rkR) const
{
    TInteger<N> kProd0 = m_kNumer*rkR.m_kDenom;
    TInteger<N> kProd1 = m_kDenom*rkR.m_kNumer;
    if (m_kDenom > 0)
    {
        return (rkR.m_kDenom > 0 ? kProd0 > kProd1 : kProd0 < kProd1);
    }
    else
    {
        return (rkR.m_kDenom > 0 ? kProd0 < kProd1 : kProd0 > kProd1);
    }
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::operator+ (const TRational& rkR) const
{
    TRational kSum;
    kSum.m_kNumer = m_kNumer*rkR.m_kDenom + m_kDenom*rkR.m_kNumer;
    kSum.m_kDenom = m_kDenom*rkR.m_kDenom;
    kSum.EliminatePowersOfTwo();
    return kSum;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::operator- (const TRational& rkR) const
{
    TRational kDiff;
    kDiff.m_kNumer = m_kNumer*rkR.m_kDenom - m_kDenom*rkR.m_kNumer;
    kDiff.m_kDenom = m_kDenom*rkR.m_kDenom;
    kDiff.EliminatePowersOfTwo();
    return kDiff;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::operator* (const TRational& rkR) const
{
    TRational kProd;
    kProd.m_kNumer = m_kNumer*rkR.m_kNumer;
    kProd.m_kDenom = m_kDenom*rkR.m_kDenom;
    kProd.EliminatePowersOfTwo();
    return kProd;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::operator/ (const TRational& rkR) const
{
    TRational kQuot;
    kQuot.m_kNumer = m_kNumer*rkR.m_kDenom;
    kQuot.m_kDenom = m_kDenom*rkR.m_kNumer;
    kQuot.EliminatePowersOfTwo();
    return kQuot;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::operator- () const
{
    TRational kNeg;
    kNeg.m_kNumer = -m_kNumer;
    kNeg.m_kDenom = m_kDenom;
    return kNeg;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> operator+ (const TInteger<N>& rkI, const TRational<N>& rkR)
{
    TRational<N> kSum;
    kSum.Numer() = rkI*rkR.Denom() + rkR.Numer();
    kSum.Denom() = rkR.Denom();
    return kSum;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> operator- (const TInteger<N>& rkI, const TRational<N>& rkR)
{
    TRational<N> kDiff;
    kDiff.Numer() = rkI*rkR.Denom() - rkR.Numer();
    kDiff.Denom() = rkR.Denom();
    return kDiff;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> operator* (const TInteger<N>& rkI, const TRational<N>& rkR)
{
    TRational<N> kProd;
    kProd.Numer() = rkI*rkR.Numer();
    kProd.Denom() = rkR.Denom();
    return kProd;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> operator/ (const TInteger<N>& rkI, const TRational<N>& rkR)
{
    TRational<N> kQuot;
    kQuot.Numer() = rkR.Denom()*rkI;
    kQuot.Denom() = rkR.Numer();
    return kQuot;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>& TRational<N>::operator+= (const TRational& rkR)
{
    *this = *this + rkR;
    EliminatePowersOfTwo();
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>& TRational<N>::operator-= (const TRational& rkR)
{
    *this = *this - rkR;
    EliminatePowersOfTwo();
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>& TRational<N>::operator*= (const TRational& rkR)
{
    *this = (*this)*rkR;
    EliminatePowersOfTwo();
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N>& TRational<N>::operator/= (const TRational& rkR)
{
    *this = (*this)/rkR;
    EliminatePowersOfTwo();
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TRational<N> TRational<N>::Abs () const
{
    return (*this >= 0 ? *this : -(*this));
}
//----------------------------------------------------------------------------
template <int N>
void TRational<N>::EliminatePowersOfTwo ()
{
    if ((m_kNumer.m_asBuffer[0] & 1) > 0
    ||  (m_kDenom.m_asBuffer[0] & 1) > 0)
    {
        // neither term is divisible by 2 (quick out)
        return;
    }

    int iBlock0 = m_kNumer.GetTrailingBlock();
    if (iBlock0 == -1)
    {
        // numerator is zero
        m_kDenom = 1;
        return;
    }

    int iBlock1 = m_kDenom.GetTrailingBlock();
    assert(iBlock1 >= 0);  // denominator should never be zero
    int iMinBlock = (iBlock0 < iBlock1 ? iBlock0 : iBlock1);
    int iBit0 = m_kNumer.GetTrailingBit(iBlock0);
    int iBit1 = m_kDenom.GetTrailingBit(iBlock1);
    int iMinBit = (iBit0 < iBit1 ? iBit0 : iBit1);
    int iShift = 16*iMinBlock + iMinBit;
    m_kNumer >>= iShift;
    m_kDenom >>= iShift;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// conversions between rational numbers and 'float'
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (float fValue)
{
    TInteger<N> kOne(1);
    m_kDenom = kOne;
    if (fValue == 0.0f)
    {
        m_kNumer = TInteger<N>(0);
        return;
    }

    // value = sign * 1.mantissa * 2^exponent
#if 0
    unsigned int uiBits = *(unsigned int*)&fValue;
#else
    union {float f; unsigned int i;} value = {fValue};
    unsigned int uiBits = value.i;
#endif
    unsigned int uiSign = (0x80000000u & uiBits);
    unsigned int uiExponent = ((0x7F800000 & uiBits) >> 23);
    unsigned int uiMantissa = (0x007FFFFF & uiBits);

    // create 1.mantissa
    TRational kFraction(1,2);
    TInteger<N> kTwo(2);
    m_kNumer = kOne;
    unsigned int uiMask;
    for (uiMask = 0x00400000; uiMask; uiMask >>= 1, kFraction /= kTwo)
    {
        if (uiMantissa & uiMask)
        {
            *this += kFraction;
        }
    }

    // multiply by 2^exponent
    TRational kMultiplier;
    TInteger<N> kPower(2);
    int i, iDelay = 0;
    if (uiExponent & 0x00000080)
    {
        kMultiplier = 2;
        for (i = 0; i <= 6; i++, uiExponent >>= 1, iDelay++)
        {
            if (uiExponent & 1)
            {
                while (--iDelay >= 0)
                {
                    kPower *= kPower;
                }

                kMultiplier *= kPower;
                iDelay = 0;
            }
        }
    }
    else
    {
        kMultiplier = 1;
        for (i = 0; i <= 6; i++, uiExponent >>= 1, iDelay++)
        {
            if (!(uiExponent & 1))
            {
                while (--iDelay >= 0)
                {
                    kPower *= kPower;
                }

                kMultiplier /= kPower;
                iDelay = 0;
            }
        }
    }

    *this *= kMultiplier;

    EliminatePowersOfTwo();

    if (uiSign)
    {
        m_kNumer = -m_kNumer;
    }
}
//----------------------------------------------------------------------------
template <int N>
void TRational<N>::ConvertTo (float& rfValue) const
{
    assert(m_kDenom != 0);
    if (m_kNumer == 0)
    {
        rfValue = 0.0f;
        return;
    }

    unsigned int uiResult;

    // compute the sign of the number
    int iS0 = m_kNumer.GetSign(), iS1 = m_kDenom.GetSign();
    int iSign = iS0*iS1;
    TInteger<N> kAbsNumer = iS0*m_kNumer;
    TInteger<N> kAbsDenom = iS1*m_kDenom;

    // The rational number is N/D = Q + R/D.  We need to extract 24 bits for
    // 1.mantissa and determine the biased exponent.
    TInteger<N> kQuo, kRem;
    bool bSuccess = TInteger<N>::GetDivMod(kAbsNumer,kAbsDenom,kQuo,kRem);
    assert(bSuccess);
    static_cast<void>(bSuccess);

    unsigned int uiExponent = 0, uiMantissa = 0;

    int iBlock = kQuo.GetLeadingBlock();
    if (iBlock >= 0)
    {
        // quotient is positive
        if (iBlock >= 8)
        {
            // quotient larger than the maximum float in magnitude
            if (iSign > 0)
            {
                uiResult = 0x7F7FFFFFu;  // FLT_MAX
                rfValue = *(float*)&uiResult;
            }
            else
            {
                uiResult = 0xFF7FFFFFu;  // -FLT_MAX
                rfValue = *(float*)&uiResult;
            }
            return;
        }

        if (iBlock == 7)
        {
            unsigned int uiValue = kQuo.ToUnsignedInt(iBlock-1,iBlock);
            if ((uiValue & 0xFFFFFF00u) == 0xFFFFFF00u)
            {
                // quotient larger or equal to the maximum float in magnitude
                if (iSign > 0)
                {
                    uiResult = 0x7F7FFFFFu;  // FLT_MAX
                    rfValue = *(float*)&uiResult;
                }
                else
                {
                    uiResult = 0xFF7FFFFFu;  // -FLT_MAX
                    rfValue = *(float*)&uiResult;
                }
                return;
            }
        }

        // quotient smaller than the maximum float
        GetPositiveFloat(kAbsDenom,kQuo,kRem,iBlock,uiExponent,uiMantissa);
        uiResult = uiExponent | uiMantissa;
        if (iSign < 0)
        {
            uiResult |= 0x80000000;
        }
        rfValue = *(float*)&uiResult;
        return;
    }

    // remainder provides all of 1.mantissa
    for (iBlock = 0; iBlock < 8; iBlock++)
    {
        // Multiply by 2^{16} to search for 1-bits.  We could do this in one
        // step by using 2^{128}, but this could require an intermediate
        // term of N=16.  The smaller multipliers keep the intermediate terms
        // small.
        kRem *= 0x10000;
        TInteger<N> kNRem;
        bSuccess = TInteger<N>::GetDivMod(kRem,kAbsDenom,kQuo,kNRem);
        assert(bSuccess);
        kRem = kNRem;
        if (kQuo != 0)
        {
            break;
        }
    }

    if (iBlock == 8 || (iBlock == 7 && kQuo.ToUnsignedInt(0) >= 4))
    {
        // rational number smaller than the minimum floating point number
        if (iSign > 0)
        {
            uiResult = 0x00800000;  // FLT_MIN
            rfValue = *(float*)&uiResult;
        }
        else
        {
            uiResult = 0x80800000;  // -FLT_MIN
            rfValue = *(float*)&uiResult;
        }
        return;
    }

    // get representation of scaled remainder
    GetPositiveFloat(kAbsDenom,kQuo,kRem,0,uiExponent,uiMantissa);

    // adjust exponent by how many blocks were used to scale the remainder
    uiExponent >>= 23;
    uiExponent -= 127;
    uiExponent -= 16*(iBlock+1);
    uiExponent += 127;
    uiExponent <<= 23;

    uiResult = uiExponent | uiMantissa;
    if (iSign < 0)
    {
        uiResult |= 0x80000000u;
    }

    rfValue = *(float*)&uiResult;
}
//----------------------------------------------------------------------------
template <int N>
void TRational<N>::GetPositiveFloat (const TInteger<N>& rkDenom,
    TInteger<N>& rkQuo, TInteger<N>& rkRem, int iBlock,
    unsigned int& ruiExponent, unsigned int& ruiMantissa)
{
    // assert(rkDenom > 0 && rkQuo > 0);

    // quotient smaller than the maximum float
    int iFirstBlockBit = rkQuo.GetLeadingBit(iBlock);
    int iFirstBit = iFirstBlockBit+16*iBlock;
    unsigned int uiMask;
    ruiExponent = ((iFirstBit + 127) << 23);
    iFirstBit--;
    ruiMantissa = 0;
    if (iFirstBit >= 23)
    {
        // quotient provides all of 1.mantissa
        for (uiMask = 0x00400000; uiMask; uiMask >>= 1, iFirstBit--)
        {
            if (rkQuo.GetBit(iFirstBit))
            {
                ruiMantissa |= uiMask;
            }
        }
    }
    else
    {
        // quotient contribution to 1.mantissa
        for (uiMask = 0x00400000; iFirstBit >= 0; uiMask >>= 1, iFirstBit--)
        {
            if (rkQuo.GetBit(iFirstBit))
            {
                ruiMantissa |= uiMask;
            }
        }

        // remainder contribution to 1.mantissa
        for (/**/; uiMask; uiMask >>= 1)
        {
            rkRem *= 2;
            TInteger<N> kNRem;
            bool bSuccess = TInteger<N>::GetDivMod(rkRem,rkDenom,rkQuo,kNRem);
            assert(bSuccess);
            (void)bSuccess;  // avoid compiler warning in release build
            if (rkQuo != 0)
            {
                ruiMantissa |= uiMask;
            }
            rkRem = kNRem;
        }
    }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// conversions between rational numbers and 'double'
//----------------------------------------------------------------------------
template <int N>
TRational<N>::TRational (double dValue)
{
    TInteger<N> kOne(1);
    m_kDenom = kOne;
    if (dValue == 0.0)
    {
        m_kNumer = TInteger<N>(0);
        return;
    }

    // value = sign * 1.mantissa * 2^exponent
#if 0
    unsigned int* auiBits = (unsigned int*)&dValue;
#else
    union {double *d; unsigned int *i;} value = {&dValue};
    unsigned int* auiBits = value.i;
#endif
#ifdef WM4_BIG_ENDIAN
    unsigned int uiSave = auiBits[0];
    auiBits[0] = auiBits[1];
    auiBits[1] = uiSave;
#endif
    unsigned int uiSign = (0x80000000u & auiBits[1]);
    unsigned int uiExponent = ((0x7FF00000 & auiBits[1]) >> 20);
    unsigned int uiMantissaHi = (0x000FFFFF & auiBits[1]);
    unsigned int uiMantissaLo = auiBits[0];

    // create 1.mantissa
    TRational kFraction(1,2);
    TInteger<N> kTwo(2);
    m_kNumer = kOne;
    unsigned int uiMask;
    for (uiMask = 0x00080000; uiMask; uiMask >>= 1, kFraction /= kTwo)
    {
        if (uiMantissaHi & uiMask)
        {
            *this += kFraction;
        }
    }
    for (uiMask = 0x80000000u; uiMask; uiMask >>= 1, kFraction /= kTwo)
    {
        if (uiMantissaLo & uiMask)
        {
            *this += kFraction;
        }
    }

    // multiply by 2^exponent
    TRational kMultiplier;
    TInteger<N> kPower(2);
    int i, iDelay = 0;
    if (uiExponent & 0x400)
    {
        kMultiplier = 2;
        for (i = 0; i <= 9; i++, uiExponent >>= 1, iDelay++)
        {
            if (uiExponent & 1)
            {
                while (--iDelay >= 0)
                {
                    kPower *= kPower;
                }

                kMultiplier *= kPower;
                iDelay = 0;
            }
        }
    }
    else
    {
        kMultiplier = 1;
        for (i = 0; i <= 9; i++, uiExponent >>= 1, iDelay++)
        {
            if (!(uiExponent & 1))
            {
                while (--iDelay >= 0)
                {
                    kPower *= kPower;
                }

                kMultiplier /= kPower;
                iDelay = 0;
            }
        }
    }

    *this *= kMultiplier;

    EliminatePowersOfTwo();

    if (uiSign)
    {
        m_kNumer = -m_kNumer;
    }
}
//----------------------------------------------------------------------------
template <int N>
void TRational<N>::ConvertTo (double& rdValue) const
{
    assert(m_kDenom != 0);
    if (m_kNumer == 0)
    {
        rdValue = 0.0;
        return;
    }

    unsigned int auiResult[2];

    // compute the sign of the number
    int iS0 = m_kNumer.GetSign(), iS1 = m_kDenom.GetSign();
    int iSign = iS0*iS1;
    TInteger<N> kAbsNumer = iS0*m_kNumer;
    TInteger<N> kAbsDenom = iS1*m_kDenom;

    // The rational number is N/D = Q + R/D.  We need to extract 53 bits for
    // 1.mantissa and determine the biased exponent.
    TInteger<N> kQuo, kRem;
    bool bSuccess = TInteger<N>::GetDivMod(kAbsNumer,kAbsDenom,kQuo,kRem);
    assert(bSuccess);
    static_cast<void>(bSuccess);

    unsigned int uiExponent = 0, uiMantissaHi = 0, uiMantissaLo;

    int iBlock = kQuo.GetLeadingBlock();
    if (iBlock >= 0)
    {
        // quotient is positive
        if (iBlock >= 64)
        {
            // quotient larger than the maximum double in magnitude
            if (iSign > 0)
            {
                auiResult[0] = 0xFFFFFFFF;  // DBL_MAX
                auiResult[1] = 0x7FEFFFFF;
#ifdef WM4_BIG_ENDIAN
                uiSave = auiResult[0];
                auiResult[0] = auiResult[1];
                auiResult[1] = uiSave;
#endif
                rdValue = *(double*)auiResult;
            }
            else
            {
                auiResult[0] = 0xFFFFFFFF;  // -DBL_MAX
                auiResult[1] = 0xFFEFFFFF;
#ifdef WM4_BIG_ENDIAN
                uiSave = auiResult[0];
                auiResult[0] = auiResult[1];
                auiResult[1] = uiSave;
#endif
                rdValue = *(double*)auiResult;
            }
            return;
        }

        if (iBlock == 63)
        {
            unsigned int uiValueHi = kQuo.ToUnsignedInt(iBlock-1,iBlock);
            unsigned int uiValueLo = kQuo.ToUnsignedInt(iBlock-3,iBlock-2);
            if ((uiValueHi & 0xFFFFFFFF) == 0xFFFFFFFF
            &&  (uiValueLo & 0xFFFFF800) == 0xFFFFF800)
            {
                // quotient larger or equal to the maximum float in magnitude
                if (iSign > 0)
                {
                    auiResult[0] = 0xFFFFFFFF;  // DBL_MAX
                    auiResult[1] = 0x7FEFFFFF;
#ifdef WM4_BIG_ENDIAN
                    uiSave = auiResult[0];
                    auiResult[0] = auiResult[1];
                    auiResult[1] = uiSave;
#endif
                    rdValue = *(double*)auiResult;
                }
                else
                {
                    auiResult[0] = 0xFFFFFFFF;  // -DBL_MAX
                    auiResult[1] = 0xFFEFFFFF;
#ifdef WM4_BIG_ENDIAN
                    uiSave = auiResult[0];
                    auiResult[0] = auiResult[1];
                    auiResult[1] = uiSave;
#endif
                    rdValue = *(double*)auiResult;
                }
                return;
            }
        }

        // quotient smaller than the maximum float
        GetPositiveDouble(kAbsDenom,kQuo,kRem,iBlock,uiExponent,uiMantissaHi,
            uiMantissaLo);
        auiResult[1] = uiExponent | uiMantissaHi;
        auiResult[0] = uiMantissaLo;
        if (iSign < 0)
        {
            auiResult[1] |= 0x80000000u;
        }
#ifdef WM4_BIG_ENDIAN
        uiSave = auiResult[0];
        auiResult[0] = auiResult[1];
        auiResult[1] = uiSave;
#endif
        rdValue = *(double*)auiResult;
        return;
    }

    // remainder provides all of 1.mantissa
    for (iBlock = 0; iBlock < 64; iBlock++)
    {
        // Multiply by 2^{16} to search for 1-bits.  We could do this in one
        // step by using 2^{1024}, but this could require an intermediate
        // term of large N.  The smaller multipliers keep the intermediate
        // terms small.
        kRem *= 0x10000;
        TInteger<N> kNRem;
        bSuccess = TInteger<N>::GetDivMod(kRem,kAbsDenom,kQuo,kNRem);
        assert(bSuccess);
        kRem = kNRem;
        if (kQuo != 0)
        {
            break;
        }
    }

    if (iBlock == 64 || (iBlock == 63 && kQuo.ToUnsignedInt(0) >= 4 /*?*/))
    {
        // rational number smaller than the minimum floating point number
        if (iSign > 0)
        {
            auiResult[1] = 0x00100000;  // DBL_MIN
            auiResult[0] = 0x00000000;
#ifdef WM4_BIG_ENDIAN
            uiSave = auiResult[0];
            auiResult[0] = auiResult[1];
            auiResult[1] = uiSave;
#endif
            rdValue = *(double*)auiResult;
        }
        else
        {
            auiResult[1] = 0x80100000;  // -DBL_MIN
            auiResult[0] = 0x00000000;
#ifdef WM4_BIG_ENDIAN
            uiSave = auiResult[0];
            auiResult[0] = auiResult[1];
            auiResult[1] = uiSave;
#endif
            rdValue = *(double*)auiResult;
        }
        return;
    }

    // get representation of scaled remainder
    GetPositiveDouble(kAbsDenom,kQuo,kRem,0,uiExponent,uiMantissaHi,
        uiMantissaLo);

    // adjust exponent by how many blocks were used to scale the remainder
    uiExponent >>= 20;
    uiExponent -= 1023;
    uiExponent -= 16*(iBlock+1);
    uiExponent += 1023;
    uiExponent <<= 20;

    auiResult[1] = uiExponent | uiMantissaHi;
    auiResult[0] = uiMantissaLo;
    if (iSign < 0)
    {
        auiResult[1] |= 0x80000000u;
    }
#ifdef WM4_BIG_ENDIAN
    uiSave = auiResult[0];
    auiResult[0] = auiResult[1];
    auiResult[1] = uiSave;
#endif
    rdValue = *(double*)auiResult;
}
//----------------------------------------------------------------------------
template <int N>
void TRational<N>::GetPositiveDouble (const TInteger<N>& rkDenom,
    TInteger<N>& rkQuo, TInteger<N>& rkRem, int iBlock,
    unsigned int& ruiExponent, unsigned int& ruiMantissaHi,
    unsigned int& ruiMantissaLo)
{
    // assert(rkDenom > 0 && rkQuo > 0);

    // quotient smaller than the maximum double
    int iFirstBlockBit = rkQuo.GetLeadingBit(iBlock);
    int iFirstBit = iFirstBlockBit+16*iBlock;
    unsigned int uiMask;
    ruiExponent = ((iFirstBit + 1023) << 20);
    iFirstBit--;
    ruiMantissaHi = 0;
    ruiMantissaLo = 0;
    if (iFirstBit >= 52)
    {
        // quotient provides all of 1.mantissa
        for (uiMask = 0x00080000; uiMask; uiMask >>= 1, iFirstBit--)
        {
            if (rkQuo.GetBit(iFirstBit))
            {
                ruiMantissaHi |= uiMask;
            }
        }
        for (uiMask = 0x80000000u; uiMask; uiMask >>= 1, iFirstBit--)
        {
            if (rkQuo.GetBit(iFirstBit))
            {
                ruiMantissaLo |= uiMask;
            }
        }
    }
    else
    {
        // quotient contribution to 1.mantissa
        bool bUsingHi = true;
        uiMask = 0x00080000;
        for (/**/; uiMask && iFirstBit >= 0; uiMask >>= 1, iFirstBit--)
        {
            if (rkQuo.GetBit(iFirstBit))
            {
                ruiMantissaHi |= uiMask;
            }
        }

        if (iFirstBit > 0)
        {
            bUsingHi = false;
            uiMask = 0x80000000u;
            for (/**/; iFirstBit >= 0; uiMask >>= 1, iFirstBit--)
            {
                if (rkQuo.GetBit(iFirstBit))
                {
                    ruiMantissaLo |= uiMask;
                }
            }
        }

        // remainder contribution to 1.mantissa
        TInteger<N> kNRem;
        bool bSuccess;
        if (bUsingHi)
        {
            for (/**/; uiMask; uiMask >>= 1)
            {
                rkRem *= 2;
                bSuccess = TInteger<N>::GetDivMod(rkRem,rkDenom,rkQuo,kNRem);
                assert(bSuccess);
                if (rkQuo != 0)
                {
                    ruiMantissaHi |= uiMask;
                }
                rkRem = kNRem;
            }

            for (uiMask = 0x80000000u; uiMask; uiMask >>= 1)
            {
                rkRem *= 2;
                bSuccess = TInteger<N>::GetDivMod(rkRem,rkDenom,rkQuo,kNRem);
                assert(bSuccess);
                if (rkQuo != 0)
                {
                    ruiMantissaLo |= uiMask;
                }
                rkRem = kNRem;
            }
        }
        else
        {
            for (/**/; uiMask; uiMask >>= 1)
            {
                rkRem *= 2;
                bSuccess = TInteger<N>::GetDivMod(rkRem,rkDenom,rkQuo,kNRem);
                assert(bSuccess);
                if (rkQuo != 0)
                {
                    ruiMantissaLo |= uiMask;
                }
                rkRem = kNRem;
            }
        }
        static_cast<void>(bSuccess);
    }
}
//----------------------------------------------------------------------------
} //namespace Wm4
