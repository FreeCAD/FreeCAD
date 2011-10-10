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
Query2<Real>::Query2 (int iVQuantity, const Vector2<Real>* akVertex)
{
    assert(iVQuantity > 0 && akVertex);
    m_iVQuantity = iVQuantity;
    m_akVertex = akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
Query2<Real>::~Query2 ()
{
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query2<Real>::GetType () const
{
    return Query::QT_REAL;
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::GetQuantity () const
{
    return m_iVQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>* Query2<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToLine (int i, int iV0, int iV1) const
{
    return ToLine(m_akVertex[i],iV0,iV1);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToLine (const Vector2<Real>& rkP, int iV0, int iV1) const
{
    const Vector2<Real>& rkV0 = m_akVertex[iV0];
    const Vector2<Real>& rkV1 = m_akVertex[iV1];

    Real fX0 = rkP[0] - rkV0[0];
    Real fY0 = rkP[1] - rkV0[1];
    Real fX1 = rkV1[0] - rkV0[0];
    Real fY1 = rkV1[1] - rkV0[1];

    Real fDet2 = Det2(fX0,fY0,fX1,fY1);
    return (fDet2 > (Real)0.0 ? +1 : (fDet2 < (Real)0.0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToTriangle (int i, int iV0, int iV1, int iV2) const
{
    return ToTriangle(m_akVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToTriangle (const Vector2<Real>& rkP, int iV0, int iV1,
    int iV2) const
{
    int iSign0 = ToLine(rkP,iV1,iV2);
    if (iSign0 > 0)
    {
        return +1;
    }

    int iSign1 = ToLine(rkP,iV0,iV2);
    if (iSign1 < 0)
    {
        return +1;
    }

    int iSign2 = ToLine(rkP,iV0,iV1);
    if (iSign2 > 0)
    {
        return +1;
    }

    return ((iSign0 && iSign1 && iSign2) ? -1 : 0);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToCircumcircle (int i, int iV0, int iV1, int iV2) const
{
    return ToCircumcircle(m_akVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query2<Real>::ToCircumcircle (const Vector2<Real>& rkP, int iV0,
    int iV1, int iV2) const
{
    const Vector2<Real>& rkV0 = m_akVertex[iV0];
    const Vector2<Real>& rkV1 = m_akVertex[iV1];
    const Vector2<Real>& rkV2 = m_akVertex[iV2];

    Real fS0x = rkV0[0] + rkP[0];
    Real fD0x = rkV0[0] - rkP[0];
    Real fS0y = rkV0[1] + rkP[1];
    Real fD0y = rkV0[1] - rkP[1];
    Real fS1x = rkV1[0] + rkP[0];
    Real fD1x = rkV1[0] - rkP[0];
    Real fS1y = rkV1[1] + rkP[1];
    Real fD1y = rkV1[1] - rkP[1];
    Real fS2x = rkV2[0] + rkP[0];
    Real fD2x = rkV2[0] - rkP[0];
    Real fS2y = rkV2[1] + rkP[1];
    Real fD2y = rkV2[1] - rkP[1];
    Real fZ0 = fS0x*fD0x + fS0y*fD0y;
    Real fZ1 = fS1x*fD1x + fS1y*fD1y;
    Real fZ2 = fS2x*fD2x + fS2y*fD2y;
    Real fDet3 = Det3(fD0x,fD0y,fZ0,fD1x,fD1y,fZ1,fD2x,fD2y,fZ2);
    return (fDet3 < (Real)0.0 ? 1 : (fDet3 > (Real)0.0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
Real Query2<Real>::Dot (Real fX0, Real fY0, Real fX1, Real fY1)
{
    return fX0*fX1 + fY0*fY1;
}
//----------------------------------------------------------------------------
template <class Real>
Real Query2<Real>::Det2 (Real fX0, Real fY0, Real fX1, Real fY1)
{
    return fX0*fY1 - fX1*fY0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Query2<Real>::Det3 (Real fX0, Real fY0, Real fZ0, Real fX1, Real fY1,
    Real fZ1, Real fX2, Real fY2, Real fZ2)
{
    Real fC00 = fY1*fZ2 - fY2*fZ1;
    Real fC01 = fY2*fZ0 - fY0*fZ2;
    Real fC02 = fY0*fZ1 - fY1*fZ0;
    return fX0*fC00 + fX1*fC01 + fX2*fC02;
}
//----------------------------------------------------------------------------
} //namespace Wm4
