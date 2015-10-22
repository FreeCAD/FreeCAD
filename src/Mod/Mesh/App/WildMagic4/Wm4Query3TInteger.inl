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
Query3TInteger<Real>::Query3TInteger (int iVQuantity,
    const Vector3<Real>* akVertex)
    :
    Query3<Real>(iVQuantity,akVertex)
{
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query3TInteger<Real>::GetType () const
{
    return Query::QT_INTEGER;
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TInteger<Real>::ToPlane (const Vector3<Real>& rkP, int iV0, int iV1,
    int iV2) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];

    TInteger<4> kX0((int)rkP[0] - (int)rkV0[0]);
    TInteger<4> kY0((int)rkP[1] - (int)rkV0[1]);
    TInteger<4> kZ0((int)rkP[2] - (int)rkV0[2]);
    TInteger<4> kX1((int)rkV1[0] - (int)rkV0[0]);
    TInteger<4> kY1((int)rkV1[1] - (int)rkV0[1]);
    TInteger<4> kZ1((int)rkV1[2] - (int)rkV0[2]);
    TInteger<4> kX2((int)rkV2[0] - (int)rkV0[0]);
    TInteger<4> kY2((int)rkV2[1] - (int)rkV0[1]);
    TInteger<4> kZ2((int)rkV2[2] - (int)rkV0[2]);

    TInteger<4> kDet3 = Det3(kX0,kY0,kZ0,kX1,kY1,kZ1,kX2,kY2,kZ2);
    return (kDet3 > 0 ? +1 : (kDet3 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query3TInteger<Real>::ToCircumsphere (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];
    const Vector3<Real>& rkV3 = m_akVertex[iV3];

    int aiP[3] = { (int)rkP[0], (int)rkP[1], (int)rkP[2] };
    int aiV0[3] = { (int)rkV0[0], (int)rkV0[1], (int)rkV0[2] };
    int aiV1[3] = { (int)rkV1[0], (int)rkV1[1], (int)rkV1[2] };
    int aiV2[3] = { (int)rkV2[0], (int)rkV2[1], (int)rkV2[2] };
    int aiV3[3] = { (int)rkV3[0], (int)rkV3[1], (int)rkV3[2] };

    TInteger<6> kS0x(aiV0[0] + aiP[0]);
    TInteger<6> kD0x(aiV0[0] - aiP[0]);
    TInteger<6> kS0y(aiV0[1] + aiP[1]);
    TInteger<6> kD0y(aiV0[1] - aiP[1]);
    TInteger<6> kS0z(aiV0[2] + aiP[2]);
    TInteger<6> kD0z(aiV0[2] - aiP[2]);
    TInteger<6> kS1x(aiV1[0] + aiP[0]);
    TInteger<6> kD1x(aiV1[0] - aiP[0]);
    TInteger<6> kS1y(aiV1[1] + aiP[1]);
    TInteger<6> kD1y(aiV1[1] - aiP[1]);
    TInteger<6> kS1z(aiV1[2] + aiP[2]);
    TInteger<6> kD1z(aiV1[2] - aiP[2]);
    TInteger<6> kS2x(aiV2[0] + aiP[0]);
    TInteger<6> kD2x(aiV2[0] - aiP[0]);
    TInteger<6> kS2y(aiV2[1] + aiP[1]);
    TInteger<6> kD2y(aiV2[1] - aiP[1]);
    TInteger<6> kS2z(aiV2[2] + aiP[2]);
    TInteger<6> kD2z(aiV2[2] - aiP[2]);
    TInteger<6> kS3x(aiV3[0] + aiP[0]);
    TInteger<6> kD3x(aiV3[0] - aiP[0]);
    TInteger<6> kS3y(aiV3[1] + aiP[1]);
    TInteger<6> kD3y(aiV3[1] - aiP[1]);
    TInteger<6> kS3z(aiV3[2] + aiP[2]);
    TInteger<6> kD3z(aiV3[2] - aiP[2]);
    TInteger<6> kW0 = kS0x*kD0x + kS0y*kD0y + kS0z*kD0z;
    TInteger<6> kW1 = kS1x*kD1x + kS1y*kD1y + kS1z*kD1z;
    TInteger<6> kW2 = kS2x*kD2x + kS2y*kD2y + kS2z*kD2z;
    TInteger<6> kW3 = kS3x*kD3x + kS3y*kD3y + kS3z*kD3z;
    TInteger<6> kDet4 = Det4(kD0x,kD0y,kD0z,kW0,kD1x,kD1y,kD1z,kW1,kD2x,kD2y,
        kD2z,kW2,kD3x,kD3y,kD3z,kW3);

    return (kDet4 > 0 ? 1 : (kDet4 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
TInteger<3> Query3TInteger<Real>::Dot (TInteger<3>& rkX0, TInteger<3>& rkY0,
    TInteger<3>& rkZ0, TInteger<3>& rkX1, TInteger<3>& rkY1,
    TInteger<3>& rkZ1)
{
    return rkX0*rkX1 + rkY0*rkY1 + rkZ0*rkZ1;
}
//----------------------------------------------------------------------------
template <class Real>
TInteger<4> Query3TInteger<Real>::Det3 (TInteger<4>& rkX0, TInteger<4>& rkY0,
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
template <class Real>
TInteger<6> Query3TInteger<Real>::Det4 (TInteger<6>& rkX0, TInteger<6>& rkY0,
    TInteger<6>& rkZ0, TInteger<6>& rkW0, TInteger<6>& rkX1,
    TInteger<6>& rkY1, TInteger<6>& rkZ1, TInteger<6>& rkW1,
    TInteger<6>& rkX2, TInteger<6>& rkY2, TInteger<6>& rkZ2,
    TInteger<6>& rkW2, TInteger<6>& rkX3, TInteger<6>& rkY3,
    TInteger<6>& rkZ3, TInteger<6>& rkW3)
{
    TInteger<6> kA0 = rkX0*rkY1 - rkX1*rkY0;
    TInteger<6> kA1 = rkX0*rkY2 - rkX2*rkY0;
    TInteger<6> kA2 = rkX0*rkY3 - rkX3*rkY0;
    TInteger<6> kA3 = rkX1*rkY2 - rkX2*rkY1;
    TInteger<6> kA4 = rkX1*rkY3 - rkX3*rkY1;
    TInteger<6> kA5 = rkX2*rkY3 - rkX3*rkY2;
    TInteger<6> kB0 = rkZ0*rkW1 - rkZ1*rkW0;
    TInteger<6> kB1 = rkZ0*rkW2 - rkZ2*rkW0;
    TInteger<6> kB2 = rkZ0*rkW3 - rkZ3*rkW0;
    TInteger<6> kB3 = rkZ1*rkW2 - rkZ2*rkW1;
    TInteger<6> kB4 = rkZ1*rkW3 - rkZ3*rkW1;
    TInteger<6> kB5 = rkZ2*rkW3 - rkZ3*rkW2;
    return kA0*kB5-kA1*kB4+kA2*kB3+kA3*kB2-kA4*kB1+kA5*kB0;
}
//----------------------------------------------------------------------------
} //namespace Wm4
