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
#include "Wm4ApprPolyFit3.h"
#include "Wm4LinearSystem.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Real* PolyFit3 (int iSamples, const Real* afX, const Real* afY,
    const Real* afW, int iXDegree, int iYDegree)
{
    int iXBound = iXDegree + 1;
    int iYBound = iYDegree + 1;
    int iQuantity = iXBound*iYBound;
    Real* afCoeff = WM4_NEW Real[iQuantity];

    int i0, j0, i1, j1, iS;

    // powers of x, y
    Real** aafXP;
    Real** aafYP;
    Allocate<Real>(2*iXDegree+1,iSamples,aafXP);
    Allocate<Real>(2*iYDegree+1,iSamples,aafYP);
    for (iS = 0; iS < iSamples; iS++)
    {
        aafXP[iS][0] = (Real)1.0;
        for (i0 = 1; i0 <= 2*iXDegree; i0++)
        {
            aafXP[iS][i0] = afX[iS]*aafXP[iS][i0-1];
        }

        aafYP[iS][0] = (Real)1.0;
        for (j0 = 1; j0 <= 2*iYDegree; j0++)
        {
            aafYP[iS][j0] = afY[iS]*aafYP[iS][j0-1];
        }
    }

    // Vandermonde matrix and right-hand side of linear system
    GMatrix<Real> kA(iQuantity,iQuantity);
    Real* afB = WM4_NEW Real[iQuantity];

    for (j0 = 0; j0 <= iYDegree; j0++)
    {
        for (i0 = 0; i0 <= iXDegree; i0++)
        {
            int iIndex0 = i0+iXBound*j0;
            Real fSum = (Real)0.0;
            for (iS = 0; iS < iSamples; iS++)
            {
                fSum += afW[iS]*aafXP[iS][i0]*aafYP[iS][j0];
            }

            afB[iIndex0] = fSum;

            for (j1 = 0; j1 <= iYDegree; j1++)
            {
                for (i1 = 0; i1 <= iXDegree; i1++)
                {
                    int iIndex1 = i1+iXBound*j1;
                    fSum = (Real)0.0;
                    for (iS = 0; iS < iSamples; iS++)
                    {
                        fSum += aafXP[iS][i0+i1]*aafYP[iS][j0+j1];
                    }

                    kA(iIndex0,iIndex1) = fSum;
                }
            }
        }
    }

    // solve for the polynomial coefficients
    bool bHasSolution = LinearSystem<Real>().Solve(kA,afB,afCoeff);
    if (!bHasSolution) throw std::exception();
    assert(bHasSolution);
    (void)bHasSolution;  // avoid compiler warning in release build
    WM4_DELETE[] afB;
    Deallocate<Real>(aafXP);
    Deallocate<Real>(aafYP);

    return afCoeff;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
float* PolyFit3<float> (int, const float*, const float*, const float*,
    int, int);

template WM4_FOUNDATION_ITEM
double* PolyFit3<double> (int, const double*, const double*, const double*,
    int, int);
//----------------------------------------------------------------------------
}
