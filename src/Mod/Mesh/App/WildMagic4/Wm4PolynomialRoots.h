// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4GMatrix.h"
#include "Wm4Vector3.h"
#include "Wm4Polynomial1.h"

namespace Wm4
{

// Methods are
//
// A: algebraic using closed-form expressions (fast, typically not robust)
// B: bisection (after root bounding, slow and robust)
// N: Newton's/bisection hybrid (after root bounding, medium and robust)
// E: eigenvalues of a companion matrix (fast and robust)

// Root bounds:
//
// For a monic polynomial
//   x^n + a[n-1]*x^{n-1} + ... + a[1]*x + a[0]
// the Cauchy bound is
//   M = 1 + max{|a[0]|,...,|a[n-1]|}.
// All real-value roots must lie in the interval [-M,M].  For a non-monic
// polynomial,
//   b[n]*x^n + b[n-1]*x^{n-1} + ... + b[1]*x + b[0],
// if b[n] is not zero, divide through by it and calculate Cauchy's
// bound:
//   1 + max{|b[0]/b[n]|,...,|b[n-1]/b[n]|}.

template <class Real>
class WM4_FOUNDATION_ITEM PolynomialRoots
{
public:
    // construction and destruction
    PolynomialRoots (Real fEpsilon);
    ~PolynomialRoots ();

    // member access
    int GetCount () const;
    const Real* GetRoots () const;
    Real GetRoot (int i) const;
    Real& Epsilon ();
    Real Epsilon () const;

    // for FindE functions, default is 128
    int& MaxIterations ();
    int MaxIterations () const;

    // linear equations:  c1*x+c0 = 0
    bool FindA (Real fC0, Real fC1);
    Real GetBound (Real fC0, Real fC1);

    // quadratic equations: c2*x^2+c1*x+c0 = 0
    bool FindA (Real fC0, Real fC1, Real fC2);
    Real GetBound (Real fC0, Real fC1, Real fC2);

    // cubic equations: c3*x^3+c2*x^2+c1*x+c0 = 0
    bool FindA (Real fC0, Real fC1, Real fC2, Real fC3);
    bool FindE (Real fC0, Real fC1, Real fC2, Real fC3, bool bDoBalancing);
    Real GetBound (Real fC0, Real fC1, Real fC2, Real fC3);

    // Solve A*r^3 + B*r = C where A > 0 and B > 0.  This equation always has
    // exactly one root.
    Real SpecialCubic (Real fA, Real fB, Real fC);

    // quartic equations: c4*x^4+c3*x^3+c2*x^2+c1*x+c0 = 0
    bool FindA (Real fC0, Real fC1, Real fC2, Real fC3, Real fC4);
    bool FindE (Real fC0, Real fC1, Real fC2, Real fC3, Real fC4,
        bool bDoBalancing);
    Real GetBound (Real fC0, Real fC1, Real fC2, Real fC3, Real fC4);

    // general equations: sum_{i=0}^{degree} c(i)*x^i = 0
    bool FindB (const Polynomial1<Real>& rkPoly, int iDigits);
    bool FindN (const Polynomial1<Real>& rkPoly, int iDigits);
    bool FindE (const Polynomial1<Real>& rkPoly, bool bDoBalancing);
    Real GetBound (const Polynomial1<Real>& rkPoly);

    // find roots on specified intervals
    bool FindB (const Polynomial1<Real>& rkPoly, Real fXMin, Real fXMax,
        int iDigits);

    bool FindN (const Polynomial1<Real>& rkPoly, Real fXMin, Real fXMax,
        int iDigits);

    bool AllRealPartsNegative (const Polynomial1<Real>& rkPoly);
    bool AllRealPartsPositive (const Polynomial1<Real>& rkPoly);

    // Count the number of roots on [t0,t1].  Uses Sturm sequences to do the
    // counting.  It is allowed to pass in t0 = -Math<Real>::MAX_REAL or
    // t1 = Math<Real>::MAX_REAL.  The value of m_fEpsilon is used as a
    // threshold on the value of a Sturm polynomial at an end point.  If
    // smaller, that value is assumed to be zero.  The return value is the
    // number of roots.  If there are infinitely many, -1 is returned.
    int GetRootCount (const Polynomial1<Real>& rkPoly, Real fT0, Real fT1);

private:
    // support for FindB
    bool Bisection (const Polynomial1<Real>& rkPoly, Real fXMin, Real fXMax,
        int iDigitsAccuracy, Real& rfRoot);

    // support for FindE
    void GetHouseholderVector (int iSize, const Vector3<Real>& rkU,
        Vector3<Real>& rkV);

    void PremultiplyHouseholder (GMatrix<Real>& rkMat, GVector<Real>& rkW,
        int iRMin, int iRMax, int iCMin, int iCMax, int iVSize,
        const Vector3<Real>& rkV);

    void PostmultiplyHouseholder (GMatrix<Real>& rkMat, GVector<Real>& rkW,
        int iRMin, int iRMax, int iCMin, int iCMax, int iVSize,
        const Vector3<Real>& rkV);

    void FrancisQRStep (GMatrix<Real>& rkH, GVector<Real>& rkW);

    Real GetRowNorm (int iRow, GMatrix<Real>& rkMat);
    Real GetColNorm (int iCol, GMatrix<Real>& rkMat);
    void ScaleRow (int iRow, Real fScale, GMatrix<Real>& rkMat);
    void ScaleCol (int iCol, Real fScale, GMatrix<Real>& rkMat);
    void Balance3 (GMatrix<Real>& rkMat);
    bool IsBalanced3 (GMatrix<Real>& rkMat);
    void BalanceCompanion3 (GMatrix<Real>& rkMat);
    bool IsBalancedCompanion3 (Real fA10, Real fA21, Real fA02, Real fA12,
        Real fA22);
    bool QRIteration3 (GMatrix<Real>& rkMat);

    void BalanceCompanion4 (GMatrix<Real>& rkMat);
    bool IsBalancedCompanion4 (Real fA10, Real fA21, Real fA32, Real fA03,
        Real fA13, Real fA23, Real fA33);
    bool QRIteration4 (GMatrix<Real>& rkMat);

    // support for testing if all roots have negative real parts
    bool AllRealPartsNegative (int iDegree, Real* afCoeff);


    Real m_fEpsilon;
    int m_iCount, m_iMaxRoot;
    Real* m_afRoot;
    int m_iMaxIterations;
};

typedef PolynomialRoots<float> PolynomialRootsf;
typedef PolynomialRoots<double> PolynomialRootsd;

}