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
Query3TRational<Real>::Query3TRational (int iVQuantity,
    const Vector3<Real>* akVertex)
    :
    Query3<Real>(iVQuantity,akVertex)
{
    m_akRVertex = WM4_NEW RVector[m_iVQuantity];
    m_abEvaluated = WM4_NEW bool[m_iVQuantity];
    memset(m_abEvaluated,0,m_iVQuantity*sizeof(bool));
}
//----------------------------------------------------------------------------
template <class Real>
Query3TRational<Real>::~Query3TRational ()
{
    WM4_DELETE[] m_akRVertex;
    WM4_DELETE[] m_abEvaluated;
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query3TRational<Real>::GetType () const
{
    return Query::QT_RATIONAL;
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToPlane (int i, int iV0, int iV1, int iV2) const
{
    int aiIndex[4] = { i, iV0, iV1, iV2 };
    Convert(4,aiIndex);
    return ToPlane(m_akRVertex[i],iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToPlane (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2) const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    kRP[2] = Rational(rkP[2]);
    int aiIndex[3] = { iV0, iV1, iV2 };
    Convert(3,aiIndex);
    return ToPlane(kRP,iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToTetrahedron (int i, int iV0, int iV1, int iV2,
    int iV3) const
{
    int aiIndex[5] = { i, iV0, iV1, iV2, iV3 };
    Convert(5,aiIndex);
    return ToTetrahedron(m_akRVertex[i],iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToTetrahedron (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    kRP[2] = Rational(rkP[2]);
    int aiIndex[4] = { iV0, iV1, iV2, iV3 };
    Convert(4,aiIndex);
    return ToTetrahedron(kRP,iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToCircumsphere (int i, int iV0, int iV1, int iV2,
    int iV3) const
{
    int aiIndex[5] = { i, iV0, iV1, iV2, iV3 };
    Convert(5,aiIndex);
    return ToCircumsphere(m_akRVertex[i],iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToCircumsphere (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    RVector kRP;
    kRP[0] = Rational(rkP[0]);
    kRP[1] = Rational(rkP[1]);
    kRP[2] = Rational(rkP[2]);
    int aiIndex[4] = { iV0, iV1, iV2, iV3 };
    Convert(4,aiIndex);
    return ToCircumsphere(kRP,iV0,iV1,iV2,iV3);
}
//----------------------------------------------------------------------------
template <class Real>
void Query3TRational<Real>::Convert (int iQuantity, int* aiIndex) const
{
    for (int i = 0; i < iQuantity; i++)
    {
        int j = aiIndex[i];
        if (!m_abEvaluated[j])
        {
            m_abEvaluated[j] = true;
            m_akRVertex[j][0] = Rational(m_akVertex[j][0]);
            m_akRVertex[j][1] = Rational(m_akVertex[j][1]);
            m_akRVertex[j][2] = Rational(m_akVertex[j][2]);
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToPlane (const RVector& rkRP, int iV0, int iV1,
    int iV2) const
{
    Rational kX0 = rkRP[0] - m_akRVertex[iV0][0];
    Rational kY0 = rkRP[1] - m_akRVertex[iV0][1];
    Rational kZ0 = rkRP[2] - m_akRVertex[iV0][2];
    Rational kX1 = m_akRVertex[iV1][0] - m_akRVertex[iV0][0];
    Rational kY1 = m_akRVertex[iV1][1] - m_akRVertex[iV0][1];
    Rational kZ1 = m_akRVertex[iV1][2] - m_akRVertex[iV0][2];
    Rational kX2 = m_akRVertex[iV2][0] - m_akRVertex[iV0][0];
    Rational kY2 = m_akRVertex[iV2][1] - m_akRVertex[iV0][1];
    Rational kZ2 = m_akRVertex[iV2][2] - m_akRVertex[iV0][2];

    Rational kDet3 = Det3(kX0,kY0,kZ0,kX1,kY1,kZ1,kX2,kY2,kZ2);
    return (kDet3 > 0 ? +1 : (kDet3 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToTetrahedron (const RVector& rkRP, int iV0,
    int iV1, int iV2, int iV3) const
{
    int iSign0 = ToPlane(rkRP,iV1,iV2,iV3);
    if (iSign0 > 0)
    {
        return +1;
    }

    int iSign1 = ToPlane(rkRP,iV0,iV2,iV3);
    if (iSign1 < 0)
    {
        return +1;
    }

    int iSign2 = ToPlane(rkRP,iV0,iV1,iV3);
    if (iSign2 > 0)
    {
        return +1;
    }

    int iSign3 = ToPlane(rkRP,iV0,iV1,iV2);
    if (iSign3 < 0)
    {
        return +1;
    }

    return ((iSign0 && iSign1 && iSign2 && iSign3) ? -1 : 0);
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TRational<Real>::ToCircumsphere (const RVector& rkRP, int iV0,
    int iV1, int iV2, int iV3) const
{
    RVector& rkRV0 = m_akRVertex[iV0];
    RVector& rkRV1 = m_akRVertex[iV1];
    RVector& rkRV2 = m_akRVertex[iV2];
    RVector& rkRV3 = m_akRVertex[iV3];

    Rational kS0x = rkRV0[0] + rkRP[0];
    Rational kD0x = rkRV0[0] - rkRP[0];
    Rational kS0y = rkRV0[1] + rkRP[1];
    Rational kD0y = rkRV0[1] - rkRP[1];
    Rational kS0z = rkRV0[2] + rkRP[2];
    Rational kD0z = rkRV0[2] - rkRP[2];
    Rational kS1x = rkRV1[0] + rkRP[0];
    Rational kD1x = rkRV1[0] - rkRP[0];
    Rational kS1y = rkRV1[1] + rkRP[1];
    Rational kD1y = rkRV1[1] - rkRP[1];
    Rational kS1z = rkRV1[2] + rkRP[2];
    Rational kD1z = rkRV1[2] - rkRP[2];
    Rational kS2x = rkRV2[0] + rkRP[0];
    Rational kD2x = rkRV2[0] - rkRP[0];
    Rational kS2y = rkRV2[1] + rkRP[1];
    Rational kD2y = rkRV2[1] - rkRP[1];
    Rational kS2z = rkRV2[2] + rkRP[2];
    Rational kD2z = rkRV2[2] - rkRP[2];
    Rational kS3x = rkRV3[0] + rkRP[0];
    Rational kD3x = rkRV3[0] - rkRP[0];
    Rational kS3y = rkRV3[1] + rkRP[1];
    Rational kD3y = rkRV3[1] - rkRP[1];
    Rational kS3z = rkRV3[2] + rkRP[2];
    Rational kD3z = rkRV3[2] - rkRP[2];
    Rational kW0 = kS0x*kD0x + kS0y*kD0y + kS0z*kD0z;
    Rational kW1 = kS1x*kD1x + kS1y*kD1y + kS1z*kD1z;
    Rational kW2 = kS2x*kD2x + kS2y*kD2y + kS2z*kD2z;
    Rational kW3 = kS3x*kD3x + kS3y*kD3y + kS3z*kD3z;
    Rational kDet4 = Det4(kD0x,kD0y,kD0z,kW0,kD1x,kD1y,kD1z,kW1,kD2x,
        kD2y,kD2z,kW2,kD3x,kD3y,kD3z,kW3);

    return (kDet4 > 0 ? 1 : (kDet4 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
typename Query3TRational<Real>::Rational Query3TRational<Real>::Dot (
    Rational& rkX0, Rational& rkY0, Rational& rkZ0, Rational& rkX1,
    Rational& rkY1, Rational& rkZ1)
{
    return rkX0*rkX1 + rkY0*rkY1 + rkZ0*rkZ1;
}
//----------------------------------------------------------------------------
template <class Real>
typename Query3TRational<Real>::Rational Query3TRational<Real>::Det3 (
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
template <class Real>
typename Query3TRational<Real>::Rational Query3TRational<Real>::Det4 (
    Rational& rkX0, Rational& rkY0, Rational& rkZ0, Rational& rkW0,
    Rational& rkX1, Rational& rkY1, Rational& rkZ1, Rational& rkW1,
    Rational& rkX2, Rational& rkY2, Rational& rkZ2, Rational& rkW2,
    Rational& rkX3, Rational& rkY3, Rational& rkZ3, Rational& rkW3)
{
    Rational kA0 = rkX0*rkY1 - rkX1*rkY0;
    Rational kA1 = rkX0*rkY2 - rkX2*rkY0;
    Rational kA2 = rkX0*rkY3 - rkX3*rkY0;
    Rational kA3 = rkX1*rkY2 - rkX2*rkY1;
    Rational kA4 = rkX1*rkY3 - rkX3*rkY1;
    Rational kA5 = rkX2*rkY3 - rkX3*rkY2;
    Rational kB0 = rkZ0*rkW1 - rkZ1*rkW0;
    Rational kB1 = rkZ0*rkW2 - rkZ2*rkW0;
    Rational kB2 = rkZ0*rkW3 - rkZ3*rkW0;
    Rational kB3 = rkZ1*rkW2 - rkZ2*rkW1;
    Rational kB4 = rkZ1*rkW3 - rkZ3*rkW1;
    Rational kB5 = rkZ2*rkW3 - rkZ3*rkW2;
    return kA0*kB5-kA1*kB4+kA2*kB3+kA3*kB2-kA4*kB1+kA5*kB0;
}
//----------------------------------------------------------------------------
} //namespace Wm4
