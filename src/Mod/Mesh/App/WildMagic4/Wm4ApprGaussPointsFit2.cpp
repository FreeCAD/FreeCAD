// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4ApprGaussPointsFit2.h"
#include "Wm4Eigen.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Box2<Real> GaussPointsFit2 (int iQuantity, const Vector2<Real>* akPoint)
{
    Box2<Real> kBox(Vector2<Real>::ZERO,Vector2<Real>::UNIT_X,
        Vector2<Real>::UNIT_Y,(Real)1.0,(Real)1.0);

    // compute the mean of the points
    kBox.Center = akPoint[0];
    int i;
    for (i = 1; i < iQuantity; i++)
    {
        kBox.Center += akPoint[i];
    }
    Real fInvQuantity = ((Real)1.0)/iQuantity;
    kBox.Center *= fInvQuantity;

    // compute the covariance matrix of the points
    Real fSumXX = (Real)0.0, fSumXY = (Real)0.0, fSumYY = (Real)0.0;
    for (i = 0; i < iQuantity; i++)
    {
        Vector2<Real> kDiff = akPoint[i] - kBox.Center;
        fSumXX += kDiff.X()*kDiff.X();
        fSumXY += kDiff.X()*kDiff.Y();
        fSumYY += kDiff.Y()*kDiff.Y();
    }

    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumYY *= fInvQuantity;

    // setup the eigensolver
    Eigen<Real> kES(2);
    kES(0,0) = fSumXX;
    kES(0,1) = fSumXY;
    kES(1,0) = fSumXY;
    kES(1,1) = fSumYY;
    kES.IncrSortEigenStuff2();

    for (i = 0; i < 2; i++)
    {
        kBox.Extent[i] = kES.GetEigenvalue(i);
        kES.GetEigenvector(i,kBox.Axis[i]);
    }

    return kBox;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
Box2<float> GaussPointsFit2<float> (int, const Vector2<float>*);

template WM4_FOUNDATION_ITEM
Box2<double> GaussPointsFit2<double> (int, const Vector2<double>*);
//----------------------------------------------------------------------------
}
