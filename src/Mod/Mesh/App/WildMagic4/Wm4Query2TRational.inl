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
template <class Real>
Query2TRational<Real>::Query2TRational (int iVQuantity,
    const Vector2<Real>* akVertex)
    :
    Query2<Real>(iVQuantity,akVertex)
{
    m_akRVertex = WM4_NEW RVector[m_iVQuantity];
    m_abEvaluated = WM4_NEW bool[m_iVQuantity];
    memset(m_abEvaluated,0,m_iVQuantity*sizeof(bool));
}
//----------------------------------------------------------------------------
template <class Real>
Query2TRational<Real>::~Query2TRational ()
{
    WM4_DELETE[] m_akRVertex;
    WM4_DELETE[] m_abEvaluated;
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query2TRational<Real>::GetType () const
{
    return Query::QT_RATIONAL;
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToLine (int i, int iV0, int iV1) const
{
    int aiIndex[3] = { i, iV0, iV1 };
    Convert(3,aiIndex);
    return ToLine(m_akRVertex[i],iV0,iV1);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToLine (const Vector2<Real>& rkP, int iV0, int iV1)
    const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    int aiIndex[2] = { iV0, iV1 };
    Convert(2,aiIndex);
    return ToLine(kRP,iV0,iV1);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToTriangle (int i, int iV0, int iV1, int iV2)
    const
{
    int aiIndex[4] = { i, iV0, iV1, iV2 };
    Convert(4,aiIndex);
    return ToTriangle(m_akRVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToTriangle (const Vector2<Real>& rkP, int iV0,
    int iV1, int iV2) const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    int aiIndex[3] = { iV0, iV1, iV2 };
    Convert(3,aiIndex);
    return ToTriangle(kRP,iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToCircumcircle (int i, int iV0, int iV1, int iV2)
    const
{
    int aiIndex[4] = { i, iV0, iV1, iV2 };
    Convert(4,aiIndex);
    return ToCircumcircle(m_akRVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToCircumcircle (const Vector2<Real>& rkP,
    int iV0, int iV1, int iV2) const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    int aiIndex[3] = { iV0, iV1, iV2 };
    Convert(3,aiIndex);
    return ToCircumcircle(kRP,iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
void Query2TRational<Real>::Convert (int iQuantity, int* aiIndex) const
{
    for (int i = 0; i < iQuantity; i++)
    {
        int j = aiIndex[i];
        if (!m_abEvaluated[j])
        {
            m_abEvaluated[j] = true;
            m_akRVertex[j][0] = Rational(m_akVertex[j][0]);
            m_akRVertex[j][1] = Rational(m_akVertex[j][1]);
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToLine (const RVector& rkRP, int iV0,
    int iV1) const
{
    Rational kX0 = rkRP[0] - m_akRVertex[iV0][0];
    Rational kY0 = rkRP[1] - m_akRVertex[iV0][1];
    Rational kX1 = m_akRVertex[iV1][0] - m_akRVertex[iV0][0];
    Rational kY1 = m_akRVertex[iV1][1] - m_akRVertex[iV0][1];

    Rational kDet2 = Det2(kX0,kY0,kX1,kY1);
    return (kDet2 > 0 ? +1 : (kDet2 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToTriangle (const RVector& rkRP, int iV0, int iV1,
    int iV2) const
{
    int iSign0 = ToLine(rkRP,iV1,iV2);
    if (iSign0 > 0)
    {
        return +1;
    }

    int iSign1 = ToLine(rkRP,iV0,iV2);
    if (iSign1 < 0)
    {
        return +1;
    }

    int iSign2 = ToLine(rkRP,iV0,iV1);
    if (iSign2 > 0)
    {
        return +1;
    }

    return ((iSign0 && iSign1 && iSign2) ? -1 : 0);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TRational<Real>::ToCircumcircle (const RVector& rkRP, int iV0,
    int iV1, int iV2) const
{
    RVector& rkRV0 = m_akRVertex[iV0];
    RVector& rkRV1 = m_akRVertex[iV1];
    RVector& rkRV2 = m_akRVertex[iV2];

    Rational kS0x = rkRV0[0] + rkRP[0];
    Rational kD0x = rkRV0[0] - rkRP[0];
    Rational kS0y = rkRV0[1] + rkRP[1];
    Rational kD0y = rkRV0[1] - rkRP[1];
    Rational kS1x = rkRV1[0] + rkRP[0];
    Rational kD1x = rkRV1[0] - rkRP[0];
    Rational kS1y = rkRV1[1] + rkRP[1];
    Rational kD1y = rkRV1[1] - rkRP[1];
    Rational kS2x = rkRV2[0] + rkRP[0];
    Rational kD2x = rkRV2[0] - rkRP[0];
    Rational kS2y = rkRV2[1] + rkRP[1];
    Rational kD2y = rkRV2[1] - rkRP[1];
    Rational kZ0 = kS0x*kD0x + kS0y*kD0y;
    Rational kZ1 = kS1x*kD1x + kS1y*kD1y;
    Rational kZ2 = kS2x*kD2x + kS2y*kD2y;
    Rational kDet3 = Det3(kD0x,kD0y,kZ0,kD1x,kD1y,kZ1,kD2x,kD2y,kZ2);
    return (kDet3 < 0 ? 1 : (kDet3 > 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
typename Query2TRational<Real>::Rational Query2TRational<Real>::Dot (
    Rational& rkX0, Rational& rkY0, Rational& rkX1, Rational& rkY1)
{
    return rkX0*rkX1 + rkY0*rkY1;
}
//----------------------------------------------------------------------------
template <class Real>
typename Query2TRational<Real>::Rational Query2TRational<Real>::Det2 (
    Rational& rkX0, Rational& rkY0, Rational& rkX1, Rational& rkY1)
{
    return rkX0*rkY1 - rkX1*rkY0;
}
//----------------------------------------------------------------------------
template <class Real>
typename Query2TRational<Real>::Rational Query2TRational<Real>::Det3 (
    Rational& rkX0, Rational& rkY0, Rational& rkZ0, Rational& rkX1,
    Rational& rkY1, Rational& rkZ1, Rational& rkX2, Rational& rkY2,
    Rational& rkZ2)
{
    Rational kC00 = rkY1*rkZ2 - rkY2*rkZ1;
    Rational kC01 = rkY2*rkZ0 - rkY0*rkZ2;
    Rational kC02 = rkY0*rkZ1 - rkY1*rkZ0;
    return rkX0*kC00 + rkX1*kC01 + rkX2*kC02;
}
//----------------------------------------------------------------------------
} //namespace Wm4
