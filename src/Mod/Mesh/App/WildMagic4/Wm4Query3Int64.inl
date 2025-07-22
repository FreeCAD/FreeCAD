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
Query3Int64<Real>::Query3Int64 (int iVQuantity, const Vector3<Real>* akVertex)
    :
    Query3<Real>(iVQuantity,akVertex)
{
}
//----------------------------------------------------------------------------
template <class Real>
Query::Type Query3Int64<Real>::GetType () const
{
    return Query::QT_INT64;
}
//----------------------------------------------------------------------------
template <class Real>
int Query3Int64<Real>::ToPlane (const Vector3<Real>& rkP, int iV0, int iV1,
    int iV2) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];

    Integer64 iX0 = (Integer64)rkP[0] - (Integer64)rkV0[0];
    Integer64 iY0 = (Integer64)rkP[1] - (Integer64)rkV0[1];
    Integer64 iZ0 = (Integer64)rkP[2] - (Integer64)rkV0[2];
    Integer64 iX1 = (Integer64)rkV1[0] - (Integer64)rkV0[0];
    Integer64 iY1 = (Integer64)rkV1[1] - (Integer64)rkV0[1];
    Integer64 iZ1 = (Integer64)rkV1[2] - (Integer64)rkV0[2];
    Integer64 iX2 = (Integer64)rkV2[0] - (Integer64)rkV0[0];
    Integer64 iY2 = (Integer64)rkV2[1] - (Integer64)rkV0[1];
    Integer64 iZ2 = (Integer64)rkV2[2] - (Integer64)rkV0[2];

    Integer64 iDet3 = Det3(iX0,iY0,iZ0,iX1,iY1,iZ1,iX2,iY2,iZ2);
    return (iDet3 > 0 ? +1 : (iDet3 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
int Query3Int64<Real>::ToCircumsphere (const Vector3<Real>& rkP, int iV0,
    int iV1, int iV2, int iV3) const
{
    const Vector3<Real>& rkV0 = m_akVertex[iV0];
    const Vector3<Real>& rkV1 = m_akVertex[iV1];
    const Vector3<Real>& rkV2 = m_akVertex[iV2];
    const Vector3<Real>& rkV3 = m_akVertex[iV3];

    Integer64 aiP[3] = { (Integer64)rkP[0], (Integer64)rkP[1],
        (Integer64)rkP[2] };
    Integer64 aiV0[3] = { (Integer64)rkV0[0], (Integer64)rkV0[1],
        (Integer64)rkV0[2] };
    Integer64 aiV1[3] = { (Integer64)rkV1[0], (Integer64)rkV1[1],
        (Integer64)rkV1[2] };
    Integer64 aiV2[3] = { (Integer64)rkV2[0], (Integer64)rkV2[1],
        (Integer64)rkV2[2] };
    Integer64 aiV3[3] = { (Integer64)rkV3[0], (Integer64)rkV3[1],
        (Integer64)rkV3[2] };

    Integer64 iS0x = aiV0[0] + aiP[0];
    Integer64 iD0x = aiV0[0] - aiP[0];
    Integer64 iS0y = aiV0[1] + aiP[1];
    Integer64 iD0y = aiV0[1] - aiP[1];
    Integer64 iS0z = aiV0[2] + aiP[2];
    Integer64 iD0z = aiV0[2] - aiP[2];
    Integer64 iS1x = aiV1[0] + aiP[0];
    Integer64 iD1x = aiV1[0] - aiP[0];
    Integer64 iS1y = aiV1[1] + aiP[1];
    Integer64 iD1y = aiV1[1] - aiP[1];
    Integer64 iS1z = aiV1[2] + aiP[2];
    Integer64 iD1z = aiV1[2] - aiP[2];
    Integer64 iS2x = aiV2[0] + aiP[0];
    Integer64 iD2x = aiV2[0] - aiP[0];
    Integer64 iS2y = aiV2[1] + aiP[1];
    Integer64 iD2y = aiV2[1] - aiP[1];
    Integer64 iS2z = aiV2[2] + aiP[2];
    Integer64 iD2z = aiV2[2] - aiP[2];
    Integer64 iS3x = aiV3[0] + aiP[0];
    Integer64 iD3x = aiV3[0] - aiP[0];
    Integer64 iS3y = aiV3[1] + aiP[1];
    Integer64 iD3y = aiV3[1] - aiP[1];
    Integer64 iS3z = aiV3[2] + aiP[2];
    Integer64 iD3z = aiV3[2] - aiP[2];
    Integer64 iW0 = iS0x*iD0x + iS0y*iD0y + iS0z*iD0z;
    Integer64 iW1 = iS1x*iD1x + iS1y*iD1y + iS1z*iD1z;
    Integer64 iW2 = iS2x*iD2x + iS2y*iD2y + iS2z*iD2z;
    Integer64 iW3 = iS3x*iD3x + iS3y*iD3y + iS3z*iD3z;
    Integer64 iDet4 = Det4(iD0x,iD0y,iD0z,iW0,iD1x,iD1y,iD1z,iW1,iD2x,
        iD2y,iD2z,iW2,iD3x,iD3y,iD3z,iW3);

    return (iDet4 > 0 ? 1 : (iDet4 < 0 ? -1 : 0));
}
//----------------------------------------------------------------------------
template <class Real>
Integer64 Query3Int64<Real>::Dot (Integer64 iX0, Integer64 iY0, Integer64 iZ0,
    Integer64 iX1, Integer64 iY1, Integer64 iZ1)
{
    return iX0*iX1 + iY0*iY1 + iZ0*iZ1;
}
//----------------------------------------------------------------------------
template <class Real>
Integer64 Query3Int64<Real>::Det3 (Integer64 iX0, Integer64 iY0,
    Integer64 iZ0, Integer64 iX1, Integer64 iY1, Integer64 iZ1,
    Integer64 iX2, Integer64 iY2, Integer64 iZ2)
{
    Integer64 iC00 = iY1*iZ2 - iY2*iZ1;
    Integer64 iC01 = iY2*iZ0 - iY0*iZ2;
    Integer64 iC02 = iY0*iZ1 - iY1*iZ0;
    return iX0*iC00 + iX1*iC01 + iX2*iC02;
}
//----------------------------------------------------------------------------
template <class Real>
Integer64 Query3Int64<Real>::Det4 (Integer64 iX0, Integer64 iY0,
    Integer64 iZ0, Integer64 iW0, Integer64 iX1, Integer64 iY1, Integer64 iZ1,
    Integer64 iW1, Integer64 iX2, Integer64 iY2, Integer64 iZ2, Integer64 iW2,
    Integer64 iX3, Integer64 iY3, Integer64 iZ3, Integer64 iW3)
{
    Integer64 iA0 = iX0*iY1 - iX1*iY0;
    Integer64 iA1 = iX0*iY2 - iX2*iY0;
    Integer64 iA2 = iX0*iY3 - iX3*iY0;
    Integer64 iA3 = iX1*iY2 - iX2*iY1;
    Integer64 iA4 = iX1*iY3 - iX3*iY1;
    Integer64 iA5 = iX2*iY3 - iX3*iY2;
    Integer64 iB0 = iZ0*iW1 - iZ1*iW0;
    Integer64 iB1 = iZ0*iW2 - iZ2*iW0;
    Integer64 iB2 = iZ0*iW3 - iZ3*iW0;
    Integer64 iB3 = iZ1*iW2 - iZ2*iW1;
    Integer64 iB4 = iZ1*iW3 - iZ3*iW1;
    Integer64 iB5 = iZ2*iW3 - iZ3*iW2;
    return iA0*iB5-iA1*iB4+iA2*iB3+iA3*iB2-iA4*iB1+iA5*iB0;
}
//----------------------------------------------------------------------------
} //namespace Wm4
