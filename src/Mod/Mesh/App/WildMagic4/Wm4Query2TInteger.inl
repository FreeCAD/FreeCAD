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
Query2TInteger<Real>::Query2TInteger (int iVQuantity,
    const Vector2<Real>* akVertex)
    :
    Query2<Real>(iVQuantity,akVertex)
{
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query2TInteger<Real>::GetType () const
{
    return Query::QT_INTEGER;
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TInteger<Real>::ToLine (const Vector2<Real>& rkP, int iV0, int iV1)
    const
{
    const Vector2<Real>& rkV0 = m_akVertex[iV0];
    const Vector2<Real>& rkV1 = m_akVertex[iV1];

    TInteger<2> kX0((int)rkP[0] - (int)rkV0[0]);
    TInteger<2> kY0((int)rkP[1] - (int)rkV0[1]);
    TInteger<2> kX1((int)rkV1[0] - (int)rkV0[0]);
    TInteger<2> kY1((int)rkV1[1] - (int)rkV0[1]);

    TInteger<2> kDet2 = Det2(kX0,kY0,kX1,kY1);
    return (kDet2 > 0 ? +1 : (kDet2 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query2TInteger<Real>::ToCircumcircle (const Vector2<Real>& rkP, int iV0,
    int iV1, int iV2) const
{
    const Vector2<Real>& rkV0 = m_akVertex[iV0];
    const Vector2<Real>& rkV1 = m_akVertex[iV1];
    const Vector2<Real>& rkV2 = m_akVertex[iV2];

    int aiP[2] = { (int)rkP[0], (int)rkP[1] };
    int aiV0[2] = { (int)rkV0[0], (int)rkV0[1] };
    int aiV1[2] = { (int)rkV1[0], (int)rkV1[1] };
    int aiV2[2] = { (int)rkV2[0], (int)rkV2[1] };

    TInteger<4> kS0x(aiV0[0] + aiP[0]);
    TInteger<4> kD0x(aiV0[0] - aiP[0]);
    TInteger<4> kS0y(aiV0[1] + aiP[1]);
    TInteger<4> kD0y(aiV0[1] - aiP[1]);
    TInteger<4> kS1x(aiV1[0] + aiP[0]);
    TInteger<4> kD1x(aiV1[0] - aiP[0]);
    TInteger<4> kS1y(aiV1[1] + aiP[1]);
    TInteger<4> kD1y(aiV1[1] - aiP[1]);
    TInteger<4> kS2x(aiV2[0] + aiP[0]);
    TInteger<4> kD2x(aiV2[0] - aiP[0]);
    TInteger<4> kS2y(aiV2[1] + aiP[1]);
    TInteger<4> kD2y(aiV2[1] - aiP[1]);
    TInteger<4> kZ0 = kS0x*kD0x + kS0y*kD0y;
    TInteger<4> kZ1 = kS1x*kD1x + kS1y*kD1y;
    TInteger<4> kZ2 = kS2x*kD2x + kS2y*kD2y;
    TInteger<4> kDet3 = Det3(kD0x,kD0y,kZ0,kD1x,kD1y,kZ1,kD2x,kD2y,kZ2);
    return (kDet3 < 0 ? 1 : (kDet3 > 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
TInteger<2> Query2TInteger<Real>::Dot (TInteger<2>& rkX0, TInteger<2>& rkY0,
    TInteger<2>& rkX1, TInteger<2>& rkY1)
{
    return rkX0*rkX1 + rkY0*rkY1;
}
//----------------------------------------------------------------------------
template <class Real>
TInteger<2> Query2TInteger<Real>::Det2 (TInteger<2>& rkX0, TInteger<2>& rkY0,
    TInteger<2>& rkX1, TInteger<2>& rkY1)
{
    return rkX0*rkY1 - rkX1*rkY0;
}
//----------------------------------------------------------------------------
template <class Real>
TInteger<4> Query2TInteger<Real>::Det3 (TInteger<4>& rkX0, TInteger<4>& rkY0,
    TInteger<4>& rkZ0, TInteger<4>& rkX1, TInteger<4>& rkY1,
    TInteger<4>& rkZ1, TInteger<4>& rkX2, TInteger<4>& rkY2,
    TInteger<4>& rkZ2)
{
    TInteger<4> kC00 = rkY1*rkZ2 - rkY2*rkZ1;
    TInteger<4> kC01 = rkY2*rkZ0 - rkY0*rkZ2;
    TInteger<4> kC02 = rkY0*rkZ1 - rkY1*rkZ0;
    return rkX0*kC00 + rkX1*kC01 + rkX2*kC02;
}
//----------------------------------------------------------------------------
} //namespace Wm4
