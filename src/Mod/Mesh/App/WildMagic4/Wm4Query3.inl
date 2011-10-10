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
Query3<Real>::Query3 (int iVQuantity, const Vector3<Real>* akVertex)
{
    assert(iVQuantity > 0 && akVertex);
    m_iVQuantity = iVQuantity;
    m_akVertex = akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
Query3<Real>::~Query3 ()
{
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query3<Real>::GetType () const
{
    return Query::QT_REAL;
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::GetQuantity () const
{
    return m_iVQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>* Query3<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToPlane (int i, int iV0, int iV1, int iV2) const
{
    return ToPlane(m_akVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToPlane (const Vector3<Real>& rkP, int iV0, int iV1,
    int iV2) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];

    Real fX0 = rkP[0] - rkV0[0];
    Real fY0 = rkP[1] - rkV0[1];
    Real fZ0 = rkP[2] - rkV0[2];
    Real fX1 = rkV1[0] - rkV0[0];
    Real fY1 = rkV1[1] - rkV0[1];
    Real fZ1 = rkV1[2] - rkV0[2];
    Real fX2 = rkV2[0] - rkV0[0];
    Real fY2 = rkV2[1] - rkV0[1];
    Real fZ2 = rkV2[2] - rkV0[2];

    Real fDet3 = Det3(fX0,fY0,fZ0,fX1,fY1,fZ1,fX2,fY2,fZ2);
    return (fDet3 > (Real)0.0 ? +1 : (fDet3 < (Real)0.0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToTetrahedron (int i, int iV0, int iV1, int iV2,
    int iV3) const
{
    return ToTetrahedron(m_akVertex[i],iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToTetrahedron (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    int iSign0 = ToPlane(rkP,iV1,iV2,iV3);
    if (iSign0 > 0)
    {
        return +1;
    }

    int iSign1 = ToPlane(rkP,iV0,iV2,iV3);
    if (iSign1 < 0)
    {
        return +1;
    }

    int iSign2 = ToPlane(rkP,iV0,iV1,iV3);
    if (iSign2 > 0)
    {
        return +1;
    }

    int iSign3 = ToPlane(rkP,iV0,iV1,iV2);
    if (iSign3 < 0)
    {
        return +1;
    }

    return ((iSign0 && iSign1 && iSign2 && iSign3) ? -1 : 0);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToCircumsphere (int i, int iV0, int iV1, int iV2,
    int iV3) const
{
    return ToCircumsphere(m_akVertex[i],iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3<Real>::ToCircumsphere (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];
    const Vector3<Real>& rkV3 = m_akVertex[iV3];

    Real fS0x = rkV0[0] + rkP[0];
    Real fD0x = rkV0[0] - rkP[0];
    Real fS0y = rkV0[1] + rkP[1];
    Real fD0y = rkV0[1] - rkP[1];
    Real fS0z = rkV0[2] + rkP[2];
    Real fD0z = rkV0[2] - rkP[2];
    Real fS1x = rkV1[0] + rkP[0];
    Real fD1x = rkV1[0] - rkP[0];
    Real fS1y = rkV1[1] + rkP[1];
    Real fD1y = rkV1[1] - rkP[1];
    Real fS1z = rkV1[2] + rkP[2];
    Real fD1z = rkV1[2] - rkP[2];
    Real fS2x = rkV2[0] + rkP[0];
    Real fD2x = rkV2[0] - rkP[0];
    Real fS2y = rkV2[1] + rkP[1];
    Real fD2y = rkV2[1] - rkP[1];
    Real fS2z = rkV2[2] + rkP[2];
    Real fD2z = rkV2[2] - rkP[2];
    Real fS3x = rkV3[0] + rkP[0];
    Real fD3x = rkV3[0] - rkP[0];
    Real fS3y = rkV3[1] + rkP[1];
    Real fD3y = rkV3[1] - rkP[1];
    Real fS3z = rkV3[2] + rkP[2];
    Real fD3z = rkV3[2] - rkP[2];
    Real fW0 = fS0x*fD0x + fS0y*fD0y + fS0z*fD0z;
    Real fW1 = fS1x*fD1x + fS1y*fD1y + fS1z*fD1z;
    Real fW2 = fS2x*fD2x + fS2y*fD2y + fS2z*fD2z;
    Real fW3 = fS3x*fD3x + fS3y*fD3y + fS3z*fD3z;
    Real fDet4 = Det4(fD0x,fD0y,fD0z,fW0,fD1x,fD1y,fD1z,fW1,fD2x,
        fD2y,fD2z,fW2,fD3x,fD3y,fD3z,fW3);

    return (fDet4 > (Real)0.0 ? 1 : (fDet4 < (Real)0.0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
Real Query3<Real>::Dot (Real fX0, Real fY0, Real fZ0, Real fX1, Real fY1,
    Real fZ1)
{
    return fX0*fX1 + fY0*fY1 + fZ0*fZ1;
}
//----------------------------------------------------------------------------
template <class Real>
Real Query3<Real>::Det3 (Real fX0, Real fY0, Real fZ0, Real fX1, Real fY1,
    Real fZ1, Real fX2, Real fY2, Real fZ2)
{
    Real fC00 = fY1*fZ2 - fY2*fZ1;
    Real fC01 = fY2*fZ0 - fY0*fZ2;
    Real fC02 = fY0*fZ1 - fY1*fZ0;
    return fX0*fC00 + fX1*fC01 + fX2*fC02;
}
//----------------------------------------------------------------------------
template <class Real>
Real Query3<Real>::Det4 (Real fX0, Real fY0, Real fZ0, Real fW0, Real fX1,
    Real fY1, Real fZ1, Real fW1, Real fX2, Real fY2, Real fZ2, Real fW2,
    Real fX3, Real fY3, Real fZ3, Real fW3)
{
    Real fA0 = fX0*fY1 - fX1*fY0;
    Real fA1 = fX0*fY2 - fX2*fY0;
    Real fA2 = fX0*fY3 - fX3*fY0;
    Real fA3 = fX1*fY2 - fX2*fY1;
    Real fA4 = fX1*fY3 - fX3*fY1;
    Real fA5 = fX2*fY3 - fX3*fY2;
    Real fB0 = fZ0*fW1 - fZ1*fW0;
    Real fB1 = fZ0*fW2 - fZ2*fW0;
    Real fB2 = fZ0*fW3 - fZ3*fW0;
    Real fB3 = fZ1*fW2 - fZ2*fW1;
    Real fB4 = fZ1*fW3 - fZ3*fW1;
    Real fB5 = fZ2*fW3 - fZ3*fW2;
    return fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
}
//----------------------------------------------------------------------------
} //namespace Wm4
