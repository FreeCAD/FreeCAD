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
TInteger<N>::TInteger (int i)
{
    if (i >= 0)
    {
        memset(m_asBuffer,0,TINT_BYTES);
    }
    else
    {
        memset(m_asBuffer,0xFF,TINT_BYTES);
    }
    System::Memcpy(m_asBuffer,sizeof(int),&i,sizeof(int));
#ifdef WM4_BIG_ENDIAN
    short sSave = m_asBuffer[0];
    m_asBuffer[0] = m_asBuffer[1];
    m_asBuffer[1] = sSave;
#endif
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>::TInteger (const TInteger& rkI)
{
    System::Memcpy(m_asBuffer,TINT_BYTES,rkI.m_asBuffer,TINT_BYTES);
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>::~TInteger ()
{
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator= (const TInteger& rkI)
{
    System::Memcpy(m_asBuffer,TINT_BYTES,rkI.m_asBuffer,TINT_BYTES);
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetSign () const
{
    return (m_asBuffer[TINT_LAST] & 0x8000) ? -1 : +1;
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator== (const TInteger& rkI) const
{
    return Compare(*this,rkI) == 0;
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator!= (const TInteger& rkI) const
{
    return Compare(*this,rkI) != 0;
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator< (const TInteger& rkI) const
{
    int iS0 = GetSign(), iS1 = rkI.GetSign();
    if (iS0 > 0)
    {
        if (iS1 > 0)
        {
            return Compare(*this,rkI) < 0;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (iS1 > 0)
        {
            return true;
        }
        else
        {
            return Compare(*this,rkI) < 0;
        }
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator<= (const TInteger& rkI) const
{
    int iS0 = GetSign(), iS1 = rkI.GetSign();
    if (iS0 > 0)
    {
        if (iS1 > 0)
        {
            return Compare(*this,rkI) <= 0;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (iS1 > 0)
        {
            return true;
        }
        else
        {
            return Compare(*this,rkI) <= 0;
        }
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator> (const TInteger& rkI) const
{
    int iS0 = GetSign(), iS1 = rkI.GetSign();
    if (iS0 > 0)
    {
        if (iS1 > 0)
        {
            return Compare(*this,rkI) > 0;
        }
        else
        {
            return true;
        }
    }
    else
    {
        if (iS1 > 0)
        {
            return false;
        }
        else
        {
            return Compare(*this,rkI) > 0;
        }
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::operator>= (const TInteger& rkI) const
{
    int iS0 = GetSign(), iS1 = rkI.GetSign();
    if (iS0 > 0)
    {
        if (iS1 > 0)
        {
            return Compare(*this,rkI) >= 0;
        }
        else
        {
            return true;
        }
    }
    else
    {
        if (iS1 > 0)
        {
            return false;
        }
        else
        {
            return Compare(*this,rkI) >= 0;
        }
    }
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::Compare (const TInteger<N>& rkI0, const TInteger<N>& rkI1)
{
    for (int i = TINT_LAST; i >= 0; i--)
    {
        unsigned int uiValue0 = (unsigned int)rkI0.m_asBuffer[i];
        unsigned int uiValue1 = (unsigned int)rkI1.m_asBuffer[i];
        if (uiValue0 < uiValue1)
        {
            return -1;
        }
        else if (uiValue0 > uiValue1)
        {
            return +1;
        }
    }
    return 0;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator- () const
{
    TInteger kResult = *this;

    // negate the bits
    int i;
    for (i = 0; i < TINT_SIZE; i++)
    {
        kResult.m_asBuffer[i] = ~kResult.m_asBuffer[i];
    }

    // add 1 (place in carry bit and add zero to 'result')
    unsigned int uiCarry = 1;
    for (i = 0; i < TINT_SIZE; i++)
    {
        unsigned int uiB1 = kResult.ToUnsignedInt(i);
        unsigned int uiSum = uiB1 + uiCarry;
        kResult.FromUnsignedInt(i,uiSum);
        uiCarry = (uiSum & 0x00010000) ? 1 : 0;
    }

    // test for overflow
    if (kResult.GetSign() == GetSign())
    {
        assert(kResult == 0);
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator+ (const TInteger& rkI) const
{
    TInteger kResult;

    unsigned int uiCarry = 0;
    for (int i = 0; i < TINT_SIZE; i++)
    {
        unsigned int uiB0 = ToUnsignedInt(i);
        unsigned int uiB1 = rkI.ToUnsignedInt(i);
        unsigned int uiSum = uiB0 + uiB1 + uiCarry;
        kResult.FromUnsignedInt(i,uiSum);
        uiCarry = (uiSum & 0x00010000) ? 1 : 0;
    }

    // test for overflow
    if (GetSign() == rkI.GetSign())
    {
        assert(kResult.GetSign() == GetSign());
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator- (const TInteger& rkI) const
{
    return *this + (-rkI);
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator* (const TInteger& rkI) const
{
    int iS0 = GetSign(), iS1 = rkI.GetSign(), iSProduct = iS0*iS1;
    TInteger kOp0 = (iS0 > 0 ? *this : -*this);
    TInteger kOp1 = (iS1 > 0 ? rkI : -rkI);

    // product of single-digit number with multiple-digit number
    unsigned short ausProduct[2*TINT_SIZE];
    unsigned short* pusPCurrent = ausProduct;

    // product of the two multiple-digit operands
    unsigned short ausResult[2*TINT_SIZE];
    unsigned short* pusRCurrent = ausResult;
    memset(ausResult,0,2*TINT_BYTES);

    for (int i0 = 0, iSize = 2*TINT_SIZE; i0 < TINT_SIZE; i0++, iSize--)
    {
        unsigned int uiB0 = kOp0.ToUnsignedInt(i0);
        if (uiB0 > 0)
        {
            unsigned short* pusPBuffer = pusPCurrent;
            unsigned int uiCarry = 0;
            int i1;
            for (i1 = 0; i1 < TINT_SIZE; i1++)
            {
                unsigned int uiB1 = kOp1.ToUnsignedInt(i1);
                unsigned int uiProd = uiB0*uiB1 + uiCarry;
                *pusPBuffer++ = (unsigned short)(uiProd & 0x0000FFFF);
                uiCarry = (uiProd & 0xFFFF0000) >> 16;
            }
            *pusPBuffer = (unsigned short)uiCarry;

            unsigned short* pusRBuffer = pusRCurrent;
            pusPBuffer = pusPCurrent;
            uiCarry = 0;
            unsigned int uiSum, uiTerm0, uiTerm1;
            for (i1 = 0; i1 <= TINT_SIZE; i1++)
            {
                uiTerm0 = (unsigned int)(*pusPBuffer++);
                uiTerm1 = (unsigned int)(*pusRBuffer);
                uiSum = uiTerm0 + uiTerm1 + uiCarry;
                *pusRBuffer++ = (unsigned short)(uiSum & 0x0000FFFF);
                uiCarry = (uiSum & 0x00010000) ? 1 : 0;
            }

            for (/**/; uiCarry > 0 && i1 < iSize; i1++)
            {
                uiTerm0 = (unsigned int)(*pusRBuffer);
                uiSum = uiTerm0 + uiCarry;
                *pusRBuffer++ = (unsigned short)(uiSum & 0x0000FFFF);
                uiCarry = (uiSum & 0x00010000) ? 1 : 0;
            }
        }

        pusPCurrent++;
        pusRCurrent++;
    }

    // Test for overflow.  You can test earlier inside the previous loop, but
    // testing here allows you to get an idea of how much overflow there is.
    // This information might be useful for an application to decide how large
    // to choose the integer size.
    for (int i = 2*TINT_SIZE-1; i >= TINT_SIZE; i--)
    {
        assert(ausResult[i] == 0);
    }
    assert((ausResult[TINT_LAST] & 0x8000) == 0);

    TInteger kResult;
    System::Memcpy(kResult.m_asBuffer,TINT_BYTES,ausResult,TINT_BYTES);
    if (iSProduct < 0)
    {
        kResult = -kResult;
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> operator* (int i, const TInteger<N>& rkI)
{
    return rkI*i;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator/ (const TInteger& rkI) const
{
    // TO DO.  On division by zero, return INVALID or signed INFINITY?
    TInteger kQ, kR;
    return (GetDivMod(*this,rkI,kQ,kR) ? kQ : 0);
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator% (const TInteger& rkI) const
{
    // TO DO.  On division by zero, return INVALID or signed INFINITY?
    TInteger kQ, kR;
    return (GetDivMod(*this,rkI,kQ,kR) ? kR : 0);
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator+= (const TInteger& rkI)
{
    *this = *this + rkI;
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator-= (const TInteger& rkI)
{
    *this = *this - rkI;
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator*= (const TInteger& rkI)
{
    *this = *this * rkI;
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator/= (const TInteger& rkI)
{
    *this = *this / rkI;
    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator<< (int iShift) const
{
    if (iShift < 0)
    {
        return 0;
    }
    if (iShift == 0)
    {
        return *this;
    }

    // number of 16-bit blocks to shift
    TInteger kResult;
    int iBlocks = iShift / 16;
    if (iBlocks > TINT_LAST)
    {
        return 0;
    }

    int i;
    if (iBlocks > 0)
    {
        int j = TINT_LAST-iBlocks;
        for (i = TINT_LAST; j >= 0; i--, j--)
        {
            kResult.m_asBuffer[i] = m_asBuffer[j];
        }

        for (/**/; i >= 0; i--)
        {
            kResult.m_asBuffer[i] = 0;
        }
    }

    // number of left-over bits to shift
    int iBits = iShift % 16;
    if (iBits > 0)
    {
        unsigned int uiLo, uiHi, uiValue;
        int iM1;
        for (i = TINT_LAST, iM1 = i-1; iM1 >= 0; i--, iM1--)
        {
            uiLo = ToUnsignedInt(iM1);
            uiHi = ToUnsignedInt(i);
            uiValue = (uiLo | (uiHi << 16));
            uiValue <<= iBits;
            kResult.FromUnsignedInt(i,((0xFFFF0000 & uiValue) >> 16));
        }

        uiValue = ToUnsignedInt(0);
        uiValue <<= iBits;
        kResult.FromUnsignedInt(0,(0x0000FFFF & uiValue));
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N> TInteger<N>::operator>> (int iShift) const
{
    if (iShift < 0)
    {
        return 0;
    }
    if (iShift == 0)
    {
        return *this;
    }

    // number of 16-bit blocks to shift
    TInteger kResult;
    int iBlocks = iShift/16;
    if (iBlocks > TINT_LAST)
    {
        return 0;
    }

    int i;
    if (iBlocks > 0)
    {
        int j = iBlocks;
        for (i = 0; j <= TINT_LAST; i++, j++)
        {
            kResult.m_asBuffer[i] = m_asBuffer[j];
        }

        if (GetSign() > 0)
        {
            for (/**/; i <= TINT_LAST; i++)
            {
                kResult.m_asBuffer[i] = 0;
            }
        }
        else
        {
            for (/**/; i <= TINT_LAST; i++)
            {
                kResult.m_asBuffer[i] = (short)(0x0000FFFF);
            }
        }
    }

    // number of left-over bits to shift
    int iBits = iShift % 16;
    if (iBits > 0)
    {
        unsigned int uiValue;
        int iP1;
        for (i = 0, iP1 = 1; iP1 <= TINT_LAST; i++, iP1++)
        {
            uiValue = ToUnsignedInt(i,iP1);
            uiValue >>= iBits;
            kResult.FromUnsignedInt(i,uiValue);
        }

        uiValue = ToUnsignedInt(TINT_LAST);
        if (GetSign() < 0)
        {
            uiValue |= 0xFFFF0000;  // sign extension
        }
        uiValue >>= iBits;
        kResult.FromUnsignedInt(TINT_LAST,uiValue);
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator<<= (int iShift)
{
    if (iShift <= 0)
    {
        return *this;
    }

    // number of 16-bit blocks to shift
    TInteger kResult;
    int iBlocks = iShift/16;
    if (iBlocks > TINT_LAST)
    {
        return *this;
    }

    int i;
    if (iBlocks > 0)
    {
        int j = TINT_LAST-iBlocks;
        for (i = TINT_LAST; j >= 0; i--, j--)
        {
            m_asBuffer[i] = m_asBuffer[j];
        }

        for (/**/; i >= 0; i--)
        {
            m_asBuffer[i] = 0;
        }
    }

    // number of left-over bits to shift
    int iBits = iShift % 16;
    if (iBits > 0)
    {
        unsigned int uiValue;
        int iM1;
        for (i = TINT_LAST, iM1 = i-1; iM1 >= 0; i--, iM1--)
        {
            uiValue = ToUnsignedInt(iM1,i);
            uiValue <<= iBits;
            FromUnsignedInt(i,((0xFFFF0000 & uiValue) >> 16));
        }

        uiValue = ToUnsignedInt(0);
        uiValue <<= iBits;
        FromUnsignedInt(0,(0x0000FFFF & uiValue));
    }

    return *this;
}
//----------------------------------------------------------------------------
template <int N>
TInteger<N>& TInteger<N>::operator>>= (int iShift)
{
    if (iShift <= 0)
    {
        return *this;
    }

    // number of 16-bit blocks to shift
    int iBlocks = iShift/16;
    if (iBlocks > TINT_LAST)
    {
        return *this;
    }

    int i;
    if (iBlocks > 0)
    {
        int j = iBlocks;
        for (i = 0, j = iBlocks; j <= TINT_LAST; i++, j++)
        {
            m_asBuffer[i] = m_asBuffer[j];
        }

        if (GetSign() > 0)
        {
            for (/**/; i <= TINT_LAST; i++)
            {
                m_asBuffer[i] = 0;
            }
        }
        else
        {
            for (/**/; i <= TINT_LAST; i++)
            {
                m_asBuffer[i] = (short)(0x0000FFFF);
            }
        }
    }

    // number of left-over bits to shift
    int iBits = iShift % 16;
    if (iBits > 0)
    {
        unsigned int uiValue;
        int iP1;
        for (i = 0, iP1 = 1; iP1 <= TINT_LAST; i++, iP1++)
        {
            uiValue = ToUnsignedInt(i,iP1);
            uiValue >>= iBits;
            FromUnsignedInt(i,uiValue);
        }

        uiValue = ToUnsignedInt(TINT_LAST);
        if (GetSign() < 0)
        {
            uiValue |= 0xFFFF0000;  // sign extension
        }
        uiValue >>= iBits;
        FromUnsignedInt(TINT_LAST,uiValue);
    }

    return *this;
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::GetDivMod (const TInteger& rkNumer, const TInteger& rkDenom,
    TInteger& rkQuo, TInteger& rkRem)
{
    if (rkDenom == 0)
    {
        assert(false);
        rkQuo = 0;
        rkRem = 0;
        return false;
    }

    if (rkNumer == 0)
    {
        rkQuo = 0;
        rkRem = 0;
        return true;
    }

    // work with the absolute values of the numerator and denominator
    int iS0 = rkNumer.GetSign(), iS1 = rkDenom.GetSign();
    TInteger kAbsNumer = iS0*rkNumer;
    TInteger kAbsDenom = iS1*rkDenom;

    int iCompare = Compare(kAbsNumer,kAbsDenom);
    if (iCompare < 0)
    {
        // numerator < denominator:  numerator = 0*denominator + numerator
        rkQuo = 0;
        rkRem = rkNumer;
        return true;
    }

    if (iCompare == 0)
    {
        // numerator == denominator:  numerator = 1*denominator + 0
        rkQuo = 1;
        rkRem = 0;
        return true;
    }

    // numerator > denominator, do the division to find quotient and remainder
    if (kAbsDenom > 0x0000FFFF)
    {
        DivMultiple(kAbsNumer,kAbsDenom,rkQuo,rkRem);
    }
    else
    {
        DivSingle(kAbsNumer,kAbsDenom.m_asBuffer[0],rkQuo,rkRem);
    }

    // apply the original signs of numerator and denominator
    rkQuo *= iS0*iS1;
    rkRem *= iS0;

#ifdef _DEBUG
    TInteger kTest = rkNumer - rkDenom*rkQuo - rkRem;
    assert(kTest == 0);
#endif
    return true;
}
//----------------------------------------------------------------------------
template <int N>
void TInteger<N>::DivSingle (const TInteger& rkNumer, short sDenom,
    TInteger& rkQuo, TInteger& rkRem)
{
    // denominator is a single "digit"
    unsigned int uiDenom = 0x0000FFFF & (unsigned int)sDenom;

    // numerator
    int iNStart = rkNumer.GetLeadingBlock();
    const short* psNumer = &rkNumer.m_asBuffer[iNStart];
    unsigned int uiDigit1 = 0;

    // quotient
    short* psQuo = &rkQuo.m_asBuffer[iNStart];
    rkQuo = 0;
    int iLastNonZero = -1;
    for (int i = iNStart; i >= 0; i--, psNumer--, psQuo--)
    {
        unsigned int uiDigitB = uiDigit1;
        uiDigit1 = 0x0000FFFF & (unsigned int)(*psNumer);
        unsigned int uiNumer = (uiDigitB << 16) | uiDigit1;
        unsigned int uiQuo = uiNumer/uiDenom;
        uiDigit1 = uiNumer - uiQuo*uiDenom;
        *psQuo = (short)(uiQuo & 0x0000FFFF);
        if (iLastNonZero == -1 && uiQuo > 0)
        {
            iLastNonZero = i;
        }
    }
    assert(iLastNonZero >= 0);

    // remainder
    size_t uiSize;
    rkRem = 0;
    if (uiDigit1 & 0xFFFF0000)
    {
        uiSize = 2*sizeof(short);
        System::Memcpy(rkRem.m_asBuffer,uiSize,&uiDigit1,uiSize);
#ifdef WM4_BIG_ENDIAN
        short sSave = rkRem.m_asBuffer[0];
        rkRem.m_asBuffer[0] = rkRem.m_asBuffer[1];
        rkRem.m_asBuffer[1] = sSave;
#endif
    }
    else
    {
        unsigned short usDigit1 = (unsigned short)uiDigit1;
        uiSize = sizeof(short);
        System::Memcpy(rkRem.m_asBuffer,uiSize,&usDigit1,uiSize);
    }
}
//----------------------------------------------------------------------------
template <int N>
void TInteger<N>::DivMultiple (const TInteger& rkNumer,
    const TInteger& rkDenom, TInteger& rkQuo, TInteger& rkRem)
{
    rkQuo = 0;
    rkRem = 0;

    // Normalization to allow good estimate of quotient.  TO DO:  It is
    // possible that the numerator is large enough that normalization causes
    // overflow when computing the product iAdjust*rkNumer; an assertion will
    // fire in this case.  Ideally the overflow would be allowed and the
    // digit in the overflow position becomes the first digit of the numerator
    // in the division algorithm.  This will require a mixture of TInteger<N>
    // and TInteger<N+1>, though.
    int iDInit = rkDenom.GetLeadingBlock();
    int iLeadingDigit = rkDenom.ToInt(iDInit);
    int iAdjust = 0x10000/(iLeadingDigit+1);
    TInteger kANum = iAdjust*rkNumer;
    TInteger kADen = iAdjust*rkDenom;
    assert(kADen.GetLeadingBlock() == iDInit);

    // get first two "digits" of denominator
    unsigned int uiD1 = kADen.ToUnsignedInt(iDInit);
    unsigned int uiD2 = kADen.ToUnsignedInt(iDInit-1);

    // determine the maximum necessary division steps
    int iNInit = kANum.GetLeadingBlock();
    assert(iNInit >= iDInit);
    int iQInit;
    unsigned int uiRHat;
    if (iNInit != iDInit)
    {
        iQInit = iNInit-iDInit-1;
        uiRHat = 1;
    }
    else
    {
        iQInit = 0;
        uiRHat = 0;
    }

    for (/**/; iQInit >= 0; iQInit--)
    {
        // get first three indices of remainder
        unsigned int uiN0, uiN1, uiN2;
        if (uiRHat > 0)
        {
            uiN0 = kANum.ToUnsignedInt(iNInit--);
            uiN1 = kANum.ToUnsignedInt(iNInit--);
            uiN2 = kANum.ToUnsignedInt(iNInit);
        }
        else
        {
            uiN0 = 0;
            uiN1 = kANum.ToUnsignedInt(iNInit--);
            uiN2 = kANum.ToUnsignedInt(iNInit);
        }

        // estimate the quotient
        unsigned int uiTmp = (uiN0 << 16) | uiN1;
        unsigned int uiQHat = (uiN0 != uiD1 ? uiTmp/uiD1 : 0x0000FFFF);
        unsigned int uiProd = uiQHat*uiD1;
        assert(uiTmp >= uiProd);
        uiRHat = uiTmp - uiProd;
        if (uiD2*uiQHat > 0x10000*uiRHat + uiN2)
        {
            uiQHat--;
            uiRHat += uiD1;
            if (uiD2*uiQHat > 0x10000*uiRHat + uiN2)
            {
                // If this block is entered, we have exactly the quotient for
                // the division.  The adjustment block of code later cannot
                // happen.
                uiQHat--;
                uiRHat += uiD1;
            }
        }

        // compute the quotient for this step of the division
        TInteger kLocalQuo;
        kLocalQuo.FromUnsignedInt(iQInit,uiQHat);

        // compute the remainder
        TInteger kProduct = kLocalQuo*kADen;
        kANum -= kProduct;
        if (kANum < 0)
        {
            uiQHat--;
            kANum += kADen;
            assert(kANum >= 0);
        }

        // set quotient digit
        rkQuo.FromUnsignedInt(iQInit,uiQHat);

        if (kANum >= kADen)
        {
            // prepare to do another division step
            iNInit = kANum.GetLeadingBlock();
        }
        else
        {
            // remainder is smaller than divisor, finished dividing
            break;
        }
    }

    // unnormalize the remainder
    if (kANum > 0)
    {
        short sDivisor = (short)(iAdjust & 0x0000FFFF);
        TInteger kShouldBeZero;
        DivSingle(kANum,sDivisor,rkRem,kShouldBeZero);
    }
    else
    {
        rkRem = 0;
    }
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetLeadingBlock () const
{
    for (int i = TINT_LAST; i >= 0; i--)
    {
        if (m_asBuffer[i] != 0)
        {
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetTrailingBlock () const
{
    for (int i = 0; i <= TINT_LAST; i++)
    {
        if (m_asBuffer[i] != 0)
        {
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetLeadingBit (int i) const
{
    assert(0 <= i && i <= TINT_LAST);
    if (i < 0 || i > TINT_LAST)
    {
        return -1;
    }

    // This is a binary search for the high-order bit of m_asBuffer[i].  The
    // return value is the index into the bits (0 <= index < 16).
    int iValue = (int)m_asBuffer[i];
    if ((iValue & 0xFF00) != 0)
    {
        if ((iValue & 0xF000) != 0)
        {
            if ((iValue & 0xC000) != 0)
            {
                if ((iValue & 0x8000) != 0)
                {
                    return 15;
                }
                else // (iValue & 0x4000) != 0
                {
                    return 14;
                }
            }
            else  // (iValue & 0x3000) != 0
            {
                if ((iValue & 0x2000) != 0)
                {
                    return 13;
                }
                else  // (iValue & 0x1000) != 0
                {
                    return 12;
                }
            }
        }
        else  // (iValue & 0x0F00) != 0
        {
            if ((iValue & 0x0C00) != 0)
            {
                if ((iValue & 0x0800) != 0)
                {
                    return 11;
                }
                else  // (iValue & 0x0400) != 0
                {
                    return 10;
                }
            }
            else  // (iValue & 0x0300) != 0
            {
                if ((iValue & 0x0200) != 0)
                {
                    return 9;
                }
                else  // (iValue & 0x0100) != 0
                {
                    return 8;
                }
            }
        }
    }
    else  // (iValue & 0x00FF)
    {
        if ((iValue & 0x00F0) != 0)
        {
            if ((iValue & 0x00C0) != 0)
            {
                if ((iValue & 0x0080) != 0)
                {
                    return 7;
                }
                else  // (iValue & 0x0040) != 0
                {
                    return 6;
                }
            }
            else  // (iValue & 0x0030) != 0
            {
                if ((iValue & 0x0020) != 0)
                {
                    return 5;
                }
                else  // (iValue & 0x0010) != 0
                {
                    return 4;
                }
            }
        }
        else  // (iValue & 0x000F) != 0
        {
            if ((iValue & 0x000C) != 0)
            {
                if ((iValue & 0x0008) != 0)
                {
                    return 3;
                }
                else  // (iValue & 0x0004) != 0
                {
                    return 2;
                }
            }
            else  // (iValue & 0x0003) != 0
            {
                if ((iValue & 0x0002) != 0)
                {
                    return 1;
                }
                else  // (iValue & 0x0001) != 0
                {
                    return 0;
                }
            }
        }
    }

    return -1;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetTrailingBit (int i) const
{
    assert(0 <= i && i <= TINT_LAST);
    if (i < 0 || i > TINT_LAST)
    {
        return -1;
    }

    // This is a binary search for the low-order bit of m_asBuffer[i].  The
    // return value is the index into the bits (0 <= index < 16).
    int iValue = (int)m_asBuffer[i];
    if ((iValue & 0x00FF) != 0)
    {
        if ((iValue & 0x000F) != 0)
        {
            if ((iValue & 0x0003) != 0)
            {
                if ((iValue & 0x0001) != 0)
                {
                    return 0;
                }
                else // (iValue & 0x0002) != 0
                {
                    return 1;
                }
            }
            else  // (iValue & 0x000C) != 0
            {
                if ((iValue & 0x0004) != 0)
                {
                    return 2;
                }
                else  // (iValue & 0x0080) != 0
                {
                    return 3;
                }
            }
        }
        else  // (iValue & 0x00F0) != 0
        {
            if ((iValue & 0x0030) != 0)
            {
                if ((iValue & 0x0010) != 0)
                {
                    return 4;
                }
                else  // (iValue & 0x0020) != 0
                {
                    return 5;
                }
            }
            else  // (iValue & 0x00C0) != 0
            {
                if ((iValue & 0x0040) != 0)
                {
                    return 6;
                }
                else  // (iValue & 0x0080) != 0
                {
                    return 7;
                }
            }
        }
    }
    else  // (iValue & 0xFF00)
    {
        if ((iValue & 0x0F00) != 0)
        {
            if ((iValue & 0x0300) != 0)
            {
                if ((iValue & 0x0100) != 0)
                {
                    return 8;
                }
                else  // (iValue & 0x0200) != 0
                {
                    return 9;
                }
            }
            else  // (iValue & 0x0C00) != 0
            {
                if ((iValue & 0x0400) != 0)
                {
                    return 10;
                }
                else  // (iValue & 0x0800) != 0
                {
                    return 11;
                }
            }
        }
        else  // (iValue & 0xF000) != 0
        {
            if ((iValue & 0x3000) != 0)
            {
                if ((iValue & 0x1000) != 0)
                {
                    return 12;
                }
                else  // (iValue & 0x2000) != 0
                {
                    return 13;
                }
            }
            else  // (iValue & 0xC000) != 0
            {
                if ((iValue & 0x4000) != 0)
                {
                    return 14;
                }
                else  // (iValue & 0x8000) != 0
                {
                    return 15;
                }
            }
        }
    }

    return -1;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetLeadingBit () const
{
    int iBlock = GetLeadingBlock();
    if (iBlock >= 0)
    {
        int iBit = GetLeadingBit(iBlock);
        if (iBit >= 0)
        {
            return iBit+16*iBlock;
        }
    }

    return -1;
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::GetTrailingBit () const
{
    int iBlock = GetTrailingBlock();
    if (iBlock >= 0)
    {
        int iBit = GetTrailingBit(iBlock);
        if (iBit >= 0)
        {
            return iBit+16*iBlock;
        }
    }

    return -1;
}
//----------------------------------------------------------------------------
template <int N>
void TInteger<N>::SetBit (int i, bool bOn)
{
    // assert(0 <= i && i <= TINT_LAST);
    int iBlock = i/16;
    int iBit = i%16;
    if (bOn)
    {
        m_asBuffer[iBlock] |= (1 << iBit);
    }
    else
    {
        m_asBuffer[iBlock] &= ~(1 << iBit);
    }
}
//----------------------------------------------------------------------------
template <int N>
bool TInteger<N>::GetBit (int i) const
{
    // assert(0 <= i && i <= TINT_LAST);
    int iBlock = i/16;
    int iBit = i%16;
    return (m_asBuffer[iBlock] & (1 << iBit)) != 0;
}
//----------------------------------------------------------------------------
template <int N>
unsigned int TInteger<N>::ToUnsignedInt (int i) const
{
    // assert(0 <= i && i <= TINT_LAST);
    return 0x0000FFFF & (unsigned int)m_asBuffer[i];
}
//----------------------------------------------------------------------------
template <int N>
void TInteger<N>::FromUnsignedInt (int i, unsigned int uiValue)
{
    // assert(0 <= i && i <= TINT_LAST);
    m_asBuffer[i] = (short)(uiValue & 0x0000FFFF);
}
//----------------------------------------------------------------------------
template <int N>
unsigned int TInteger<N>::ToUnsignedInt (int iLo, int iHi) const
{
    unsigned int uiLo = ToUnsignedInt(iLo);
    unsigned int uiHi = ToUnsignedInt(iHi);
    return (uiLo | (uiHi << 16));
}
//----------------------------------------------------------------------------
template <int N>
int TInteger<N>::ToInt (int i) const
{
    // assert(0 <= i && i <= TINT_LAST);
    return (int)(0x0000FFFF & (unsigned int)m_asBuffer[i]);
}
//----------------------------------------------------------------------------
} //namespace Wm4
