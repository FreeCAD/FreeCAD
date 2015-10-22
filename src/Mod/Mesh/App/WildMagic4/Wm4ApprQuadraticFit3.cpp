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

#include "Wm4FoundationPCH.h"
#include "Wm4ApprQuadraticFit3.h"
#include "Wm4Eigen.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Real QuadraticFit3 (int iQuantity, const Vector3<Real>* akPoint,
    Real afCoeff[10])
{
    Eigen<Real> kES(10);
    int iRow, iCol;
    for (iRow = 0; iRow < 10; iRow++)
    {
        for (iCol = 0; iCol < 10; iCol++)
        {
            kES(iRow,iCol) = (Real)0.0;
        }
    }

    for (int i = 0; i < iQuantity; i++)
    {
        Real fX = akPoint[i].X();
        Real fY = akPoint[i].Y();
        Real fZ = akPoint[i].Z();
        Real fX2 = fX*fX;
        Real fY2 = fY*fY;
        Real fZ2 = fZ*fZ;
        Real fXY = fX*fY;
        Real fXZ = fX*fZ;
        Real fYZ = fY*fZ;
        Real fX3 = fX*fX2;
        Real fXY2 = fX*fY2;
        Real fXZ2 = fX*fZ2;
        Real fX2Y = fX*fXY;
        Real fX2Z = fX*fXZ;
        Real fXYZ = fX*fY*fZ;
        Real fY3 = fY*fY2;
        Real fYZ2 = fY*fZ2;
        Real fY2Z = fY*fYZ;
        Real fZ3 = fZ*fZ2;
        Real fX4 = fX*fX3;
        Real fX2Y2 = fX*fXY2;
        Real fX2Z2 = fX*fXZ2;
        Real fX3Y = fX*fX2Y;
        Real fX3Z = fX*fX2Z;
        Real fX2YZ = fX*fXYZ;
        Real fY4 = fY*fY3;
        Real fY2Z2 = fY*fYZ2;
        Real fXY3 = fX*fY3;
        Real fXY2Z = fX*fY2Z;
        Real fY3Z = fY*fY2Z;
        Real fZ4 = fZ*fZ3;
        Real fXYZ2 = fX*fYZ2;
        Real fXZ3 = fX*fZ3;
        Real fYZ3 = fY*fZ3;

        kES(0,1) += fX;
        kES(0,2) += fY;
        kES(0,3) += fZ;
        kES(0,4) += fX2;
        kES(0,5) += fY2;
        kES(0,6) += fZ2;
        kES(0,7) += fXY;
        kES(0,8) += fXZ;
        kES(0,9) += fYZ;
        kES(1,4) += fX3;
        kES(1,5) += fXY2;
        kES(1,6) += fXZ2;
        kES(1,7) += fX2Y;
        kES(1,8) += fX2Z;
        kES(1,9) += fXYZ;
        kES(2,5) += fY3;
        kES(2,6) += fYZ2;
        kES(2,9) += fY2Z;
        kES(3,6) += fZ3;
        kES(4,4) += fX4;
        kES(4,5) += fX2Y2;
        kES(4,6) += fX2Z2;
        kES(4,7) += fX3Y;
        kES(4,8) += fX3Z;
        kES(4,9) += fX2YZ;
        kES(5,5) += fY4;
        kES(5,6) += fY2Z2;
        kES(5,7) += fXY3;
        kES(5,8) += fXY2Z;
        kES(5,9) += fY3Z;
        kES(6,6) += fZ4;
        kES(6,7) += fXYZ2;
        kES(6,8) += fXZ3;
        kES(6,9) += fYZ3;
        kES(9,9) += fY2Z2;
    }

    kES(0,0) = (Real)iQuantity;
    kES(1,1) = kES(0,4);
    kES(1,2) = kES(0,7);
    kES(1,3) = kES(0,8);
    kES(2,2) = kES(0,5);
    kES(2,3) = kES(0,9);
    kES(2,4) = kES(1,7);
    kES(2,7) = kES(1,5);
    kES(2,8) = kES(1,9);
    kES(3,3) = kES(0,6);
    kES(3,4) = kES(1,8);
    kES(3,5) = kES(2,9);
    kES(3,7) = kES(1,9);
    kES(3,8) = kES(1,6);
    kES(3,9) = kES(2,6);
    kES(7,7) = kES(4,5);
    kES(7,8) = kES(4,9);
    kES(7,9) = kES(5,8);
    kES(8,8) = kES(4,6);
    kES(8,9) = kES(6,7);
    kES(9,9) = kES(5,6);

    for (iRow = 0; iRow < 10; iRow++)
    {
        for (iCol = 0; iCol < iRow; iCol++)
        {
            kES(iRow,iCol) = kES(iCol,iRow);
        }
    }

    Real fInvQuantity = ((Real)1.0)/(Real)iQuantity;
    for (iRow = 0; iRow < 10; iRow++)
    {
        for (iCol = 0; iCol < 10; iCol++)
        {
            kES(iRow,iCol) *= fInvQuantity;
        }
    }

    kES.IncrSortEigenStuffN();

    GVector<Real> kEVector = kES.GetEigenvector(0);
    size_t uiSize = 10*sizeof(Real);
    System::Memcpy(afCoeff,uiSize,(Real*)kEVector,uiSize);

    // For exact fit, numeric round-off errors may make the minimum
    // eigenvalue just slightly negative.  Return absolute value since
    // application may rely on the return value being nonnegative.
    return Math<Real>::FAbs(kES.GetEigenvalue(0));
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadraticSphereFit3 (int iQuantity, const Vector3<Real>* akPoint,
    Vector3<Real>& rkCenter, Real& rfRadius)
{
    Eigen<Real> kES(5);
    int iRow, iCol;
    for (iRow = 0; iRow < 5; iRow++)
    {
        for (iCol = 0; iCol < 5; iCol++)
        {
            kES(iRow,iCol) = (Real)0.0;
        }
    }

    for (int i = 0; i < iQuantity; i++)
    {
        Real fX = akPoint[i].X();
        Real fY = akPoint[i].Y();
        Real fZ = akPoint[i].Z();
        Real fX2 = fX*fX;
        Real fY2 = fY*fY;
        Real fZ2 = fZ*fZ;
        Real fXY = fX*fY;
        Real fXZ = fX*fZ;
        Real fYZ = fY*fZ;
        Real fR2 = fX2+fY2+fZ2;
        Real fXR2 = fX*fR2;
        Real fYR2 = fY*fR2;
        Real fZR2 = fZ*fR2;
        Real fR4 = fR2*fR2;

        kES(0,1) += fX;
        kES(0,2) += fY;
        kES(0,3) += fZ;
        kES(0,4) += fR2;
        kES(1,1) += fX2;
        kES(1,2) += fXY;
        kES(1,3) += fXZ;
        kES(1,4) += fXR2;
        kES(2,2) += fY2;
        kES(2,3) += fYZ;
        kES(2,4) += fYR2;
        kES(3,3) += fZ2;
        kES(3,4) += fZR2;
        kES(4,4) += fR4;
    }

    kES(0,0) = (Real)iQuantity;

    for (iRow = 0; iRow < 5; iRow++)
    {
        for (iCol = 0; iCol < iRow; iCol++)
        {
            kES(iRow,iCol) = kES(iCol,iRow);
        }
    }

    Real fInvQuantity = ((Real)1.0)/(Real)iQuantity;
    for (iRow = 0; iRow < 5; iRow++)
    {
        for (iCol = 0; iCol < 5; iCol++)
        {
            kES(iRow,iCol) *= fInvQuantity;
        }
    }

    kES.IncrSortEigenStuffN();

    GVector<Real> kEVector = kES.GetEigenvector(0);
    Real fInv = ((Real)1.0)/kEVector[4];  // beware zero divide
    Real afCoeff[4];
    for (iRow = 0; iRow < 4; iRow++)
    {
        afCoeff[iRow] = fInv*kEVector[iRow];
    }

    rkCenter.X() = -((Real)0.5)*afCoeff[1];
    rkCenter.Y() = -((Real)0.5)*afCoeff[2];
    rkCenter.Z() = -((Real)0.5)*afCoeff[3];
    rfRadius = Math<Real>::Sqrt(Math<Real>::FAbs(rkCenter.X()*rkCenter.X() +
        rkCenter.Y()*rkCenter.Y() + rkCenter.Z()*rkCenter.Z() - afCoeff[0]));

    // For exact fit, numeric round-off errors may make the minimum
    // eigenvalue just slightly negative.  Return absolute value since
    // application may rely on the return value being nonnegative.
    return Math<Real>::FAbs(kES.GetEigenvalue(0));
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
float QuadraticFit3<float> (int, const Vector3<float>*, float[10]);

template WM4_FOUNDATION_ITEM
float QuadraticSphereFit3<float> (int, const Vector3<float>*,
    Vector3<float>&, float&);

template WM4_FOUNDATION_ITEM
double QuadraticFit3<double> (int, const Vector3<double>*, double[10]);

template WM4_FOUNDATION_ITEM
double QuadraticSphereFit3<double> (int, const Vector3<double>*,
    Vector3<double>&, double&);
//----------------------------------------------------------------------------
}
