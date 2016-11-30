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
#include "Wm4QuadricSurface.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
QuadricSurface<Real>::QuadricSurface ()
{
    memset(m_afCoeff,0,10*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
QuadricSurface<Real>::QuadricSurface (const Real afCoeff[10])
{
    for (int i = 0; i < 10; i++)
    {
        m_afCoeff[i] = afCoeff[i];
    }

    // compute A, B, C
    m_fC = m_afCoeff[0];
    m_kB[0] = m_afCoeff[1];
    m_kB[1] = m_afCoeff[2];
    m_kB[2] = m_afCoeff[3];
    m_kA[0][0] = m_afCoeff[4];
    m_kA[0][1] = ((Real)0.5)*m_afCoeff[5];
    m_kA[0][2] = ((Real)0.5)*m_afCoeff[6];
    m_kA[1][0] = m_kA[0][1];
    m_kA[1][1] = m_afCoeff[7];
    m_kA[1][2] = ((Real)0.5)*m_afCoeff[8];
    m_kA[2][0] = m_kA[0][2];
    m_kA[2][1] = m_kA[1][2];
    m_kA[2][2] = m_afCoeff[9];
}
//----------------------------------------------------------------------------
template <class Real>
const Real* QuadricSurface<Real>::GetCoefficients () const
{
    return m_afCoeff;
}
//----------------------------------------------------------------------------
template <class Real>
const Matrix3<Real>& QuadricSurface<Real>::GetA () const
{
    return m_kA;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& QuadricSurface<Real>::GetB () const
{
    return m_kB;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::GetC () const
{
    return m_fC;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::F (const Vector3<Real>& rkP) const
{
    Real fF = rkP.Dot(m_kA*rkP + m_kB) + m_fC;
    return fF;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FX (const Vector3<Real>& rkP) const
{
    Real fSum = m_kA[0][0]*rkP[0] + m_kA[0][1]*rkP[1] + m_kA[0][2]*rkP[2];
    Real fFX = ((Real)2.0)*fSum + m_kB[0];
    return fFX;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FY (const Vector3<Real>& rkP) const
{
    Real fSum = m_kA[1][0]*rkP[0] + m_kA[1][1]*rkP[1] + m_kA[1][2]*rkP[2];
    Real fFY = ((Real)2.0)*fSum + m_kB[1];
    return fFY;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FZ (const Vector3<Real>& rkP) const
{
    Real fSum = m_kA[2][0]*rkP[0] + m_kA[2][1]*rkP[1] + m_kA[2][2]*rkP[2];
    Real fFZ = ((Real)2.0)*fSum + m_kB[2];
    return fFZ;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FXX (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFXX = ((Real)2.0)*m_kA[0][0];
    return fFXX;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FXY (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFXY = ((Real)2.0)*m_kA[0][1];
    return fFXY;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FXZ (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFXZ = ((Real)2.0)*m_kA[0][2];
    return fFXZ;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FYY (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFYY = ((Real)2.0)*m_kA[1][1];
    return fFYY;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FYZ (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFYZ = ((Real)2.0)*m_kA[1][2];
    return fFYZ;
}
//----------------------------------------------------------------------------
template <class Real>
Real QuadricSurface<Real>::FZZ (const Vector3<Real>& rkP) const
{
    (void)rkP;
    Real fFZZ = ((Real)2.0)*m_kA[2][2];
    return fFZZ;
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::GetType () const
{
    // Convert the coefficients to their rational representations and
    // compute various derived quantities.
    RReps kReps(m_afCoeff);

    // use Sturm sequences to determine the signs of the roots
    int iPositiveRoots, iNegativeRoots, iZeroRoots;
    GetRootSigns(kReps,iPositiveRoots,iNegativeRoots,iZeroRoots);

    // classify the solution set to the equation
    int eType = QT_NONE;
    switch (iZeroRoots)
    {
    case 0:
        eType = ClassifyZeroRoots0(kReps,iPositiveRoots);
        break;
    case 1:
        eType = ClassifyZeroRoots1(kReps,iPositiveRoots);
        break;
    case 2:
        eType = ClassifyZeroRoots2(kReps,iPositiveRoots);
        break;
    case 3:
        eType = ClassifyZeroRoots3(kReps);
        break;
    }
    return eType;
}
//----------------------------------------------------------------------------
template <class Real>
void QuadricSurface<Real>::GetRootSigns (RReps& rkReps,
    int& riPositiveRoots, int& riNegativeRoots, int& riZeroRoots)
{
    // use Sturm sequences to determine the signs of the roots
    int iSignChangeMI, iSignChange0, iSignChangePI, iDistinctNonzeroRoots;
    Rational akValue[4];
    if (rkReps.c0 != 0)
    {
        rkReps.c3 = Rational(2,9)*rkReps.c2*rkReps.c2 -
            Rational(2,3)*rkReps.c1;
        rkReps.c4 = rkReps.c0 - Rational(1,9)*rkReps.c1*rkReps.c2;

        if (rkReps.c3 != 0)
        {
            rkReps.c5 = -(rkReps.c1 + ((Rational(2)*rkReps.c2*rkReps.c3 +
                Rational(3)*rkReps.c4)*rkReps.c4)/(rkReps.c3*rkReps.c3));

            akValue[0] = 1;
            akValue[1] = -rkReps.c3;
            akValue[2] = rkReps.c5;
            iSignChangeMI = 1 + GetSignChanges(3,akValue);

            akValue[0] = -rkReps.c0;
            akValue[1] = rkReps.c1;
            akValue[2] = rkReps.c4;
            akValue[3] = rkReps.c5;
            iSignChange0 = GetSignChanges(4,akValue);

            akValue[0] = 1;
            akValue[1] = rkReps.c3;
            akValue[2] = rkReps.c5;
            iSignChangePI = GetSignChanges(3,akValue);
        }
        else
        {
            akValue[0] = -rkReps.c0;
            akValue[1] = rkReps.c1;
            akValue[2] = rkReps.c4;
            iSignChange0 = GetSignChanges(3,akValue);

            akValue[0] = 1;
            akValue[1] = rkReps.c4;
            iSignChangePI = GetSignChanges(2,akValue);
            iSignChangeMI = 1 + iSignChangePI;
        }

        riPositiveRoots = iSignChange0 - iSignChangePI;
        assert(riPositiveRoots >= 0);
        riNegativeRoots = iSignChangeMI - iSignChange0;
        assert(riNegativeRoots >= 0);
        riZeroRoots = 0;

        iDistinctNonzeroRoots = riPositiveRoots + riNegativeRoots;
        if (iDistinctNonzeroRoots == 2)
        {
            if (riPositiveRoots == 2)
            {
                riPositiveRoots = 3;
            }
            else if (riNegativeRoots == 2)
            {
                riNegativeRoots = 3;
            }
            else
            {
                // One root is positive and one is negative.  One root has
                // multiplicity 2, the other of multiplicity 1.  Distinguish
                // between the two cases by computing the sign of the
                // polynomial at the inflection point L = c2/3.
                Rational kX = Rational(1,3)*rkReps.c2;
                Rational kPoly = kX*(kX*(kX-rkReps.c2)+rkReps.c1)-rkReps.c0;
                if (kPoly > 0)
                {
                    riPositiveRoots = 2;
                }
                else
                {
                    riNegativeRoots = 2;
                }
            }
        }
        else if (iDistinctNonzeroRoots == 1)
        {
            // root of multiplicity 3
            if (riPositiveRoots == 1)
            {
                riPositiveRoots = 3;
            }
            else
            {
                riNegativeRoots = 3;
            }
        }

        return;
    }

    if (rkReps.c1 != 0)
    {
        rkReps.c3 = Rational(1,4)*rkReps.c2*rkReps.c2 - rkReps.c1;

        akValue[0] = -1;
        akValue[1] = rkReps.c3;
        iSignChangeMI = 1 + GetSignChanges(2,akValue);

        akValue[0] = rkReps.c1;
        akValue[1] = -rkReps.c2;
        akValue[2] = rkReps.c3;
        iSignChange0 = GetSignChanges(3,akValue);

        akValue[0] = 1;
        akValue[1] = rkReps.c3;
        iSignChangePI = GetSignChanges(2,akValue);

        riPositiveRoots = iSignChange0 - iSignChangePI;
        assert( riPositiveRoots >= 0 );
        riNegativeRoots = iSignChangeMI - iSignChange0;
        assert( riNegativeRoots >= 0 );
        riZeroRoots = 1;

        iDistinctNonzeroRoots = riPositiveRoots + riNegativeRoots;
        if (iDistinctNonzeroRoots == 1)
        {
            riPositiveRoots = 2;
        }

        return;
    }

    if (rkReps.c2 != 0)
    {
        riZeroRoots = 2;
        if (rkReps.c2 > 0)
        {
            riPositiveRoots = 1;
            riNegativeRoots = 0;
        }
        else
        {
            riPositiveRoots = 0;
            riNegativeRoots = 1;
        }
        return;
    }

    riPositiveRoots = 0;
    riNegativeRoots = 0;
    riZeroRoots = 3;
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::GetSignChanges (int iQuantity,
    const Rational* akValue)
{
    int iSignChanges = 0;
    Rational kZero(0);

    Rational kPrev = akValue[0];
    for (int i = 1; i < iQuantity; i++)
    {
        Rational kNext = akValue[i];
        if (kNext != kZero)
        {
            if (kPrev*kNext < kZero)
            {
                iSignChanges++;
            }

            kPrev = kNext;
        }
    }

    return iSignChanges;
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots0 (const RReps& rkReps,
    int iPositiveRoots)
{
    // inverse matrix is
    // +-                      -+
    // |  Sub00  -Sub01   Sub02 |
    // | -Sub01   Sub11  -Sub12 | * (1/det)
    // |  Sub02  -Sub12   Sub22 |
    // +-                      -+
    Rational kFourDet = Rational(4)*rkReps.c0;

    Rational kQForm = rkReps.b0*(rkReps.Sub00*rkReps.b0 -
        rkReps.Sub01*rkReps.b1 + rkReps.Sub02*rkReps.b2) -
        rkReps.b1*(rkReps.Sub01*rkReps.b0 - rkReps.Sub11*rkReps.b1 +
        rkReps.Sub12*rkReps.b2) + rkReps.b2*(rkReps.Sub02*rkReps.b0 -
        rkReps.Sub12*rkReps.b1 + rkReps.Sub22*rkReps.b2);

    Rational kR = Rational(1,4)*kQForm/kFourDet - rkReps.c;
    if (kR > 0)
    {
        if (iPositiveRoots == 3)
        {
            return QT_ELLIPSOID;
        }
        else if (iPositiveRoots == 2)
        {
            return QT_HYPERBOLOID_ONE_SHEET;
        }
        else if (iPositiveRoots == 1)
        {
            return QT_HYPERBOLOID_TWO_SHEETS;
        }
        else
        {
            return QT_NONE;
        }
    }
    else if (kR < 0)
    {
        if (iPositiveRoots == 3)
        {
            return QT_NONE;
        }
        else if (iPositiveRoots == 2)
        {
            return QT_HYPERBOLOID_TWO_SHEETS;
        }
        else if (iPositiveRoots == 1)
        {
            return QT_HYPERBOLOID_ONE_SHEET;
        }
        else
        {
            return QT_ELLIPSOID;
        }
    }

    // else kR == 0
    if (iPositiveRoots == 3 || iPositiveRoots == 0)
    {
        return QT_POINT;
    }

    return QT_ELLIPTIC_CONE;
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots1 (const RReps& rkReps,
    int iPositiveRoots)
{
    // Generate an orthonormal set {p0,p1,p2}, where p0 is an eigenvector
    // of A corresponding to eigenvalue zero.
    QSVector kP0, kP1, kP2;

    if (rkReps.Sub00 != 0 || rkReps.Sub01 != 0 || rkReps.Sub02 != 0)
    {
        // rows 1 and 2 are linearly independent
        kP0 = QSVector(rkReps.Sub00,-rkReps.Sub01,rkReps.Sub02);
        kP1 = QSVector(rkReps.a01,rkReps.a11,rkReps.a12);
        kP2 = kP0.Cross(kP1);
        return ClassifyZeroRoots1(rkReps,iPositiveRoots,kP0,kP1,kP2);
    }

    if (rkReps.Sub01 != 0 || rkReps.Sub11 != 0 || rkReps.Sub12 != 0)
    {
        // rows 2 and 0 are linearly independent
        kP0 = QSVector(-rkReps.Sub01,rkReps.Sub11,-rkReps.Sub12);
        kP1 = QSVector(rkReps.a02,rkReps.a12,rkReps.a22);
        kP2 = kP0.Cross(kP1);
        return ClassifyZeroRoots1(rkReps,iPositiveRoots,kP0,kP1,kP2);
    }

    // rows 0 and 1 are linearly independent
    kP0 = QSVector(rkReps.Sub02,-rkReps.Sub12,rkReps.Sub22);
    kP1 = QSVector(rkReps.a00,rkReps.a01,rkReps.a02);
    kP2 = kP0.Cross(kP1);
    return ClassifyZeroRoots1(rkReps,iPositiveRoots,kP0,kP1,kP2);
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots1 (const RReps& rkReps,
    int iPositiveRoots, const QSVector& rkP0, const QSVector& rkP1,
    const QSVector& rkP2)
{
    Rational kE0 = rkP0.X()*rkReps.b0 + rkP0.Y()*rkReps.b1 +
        rkP0.Z()*rkReps.b2;

    if (kE0 != 0)
    {
        if (iPositiveRoots == 1)
        {
            return QT_HYPERBOLIC_PARABOLOID;
        }
        else
        {
            return QT_ELLIPTIC_PARABOLOID;
        }
    }

    // matrix F
    Rational kF11 = rkP1.X()*(rkReps.a00*rkP1.X() + rkReps.a01*rkP1.Y() +
        rkReps.a02*rkP1.Z()) + rkP1.Y()*(rkReps.a01*rkP1.X() +
        rkReps.a11*rkP1.Y() + rkReps.a12*rkP1.Z()) + rkP1.Z()*(
        rkReps.a02*rkP1.X() + rkReps.a12*rkP1.Y() + rkReps.a22*rkP1.Z());

    Rational kF12 = rkP2.X()*(rkReps.a00*rkP1.X() + rkReps.a01*rkP1.Y() +
        rkReps.a02*rkP1.Z()) + rkP2.Y()*(rkReps.a01*rkP1.X() +
        rkReps.a11*rkP1.Y() + rkReps.a12*rkP1.Z()) + rkP2.Z()*(
        rkReps.a02*rkP1.X() + rkReps.a12*rkP1.Y() + rkReps.a22*rkP1.Z());

    Rational kF22 = rkP2.X()*(rkReps.a00*rkP2.X() + rkReps.a01*rkP2.Y() +
        rkReps.a02*rkP2.Z()) + rkP2.Y()*(rkReps.a01*rkP2.X() +
        rkReps.a11*rkP2.Y() + rkReps.a12*rkP2.Z()) + rkP2.Z()*(
        rkReps.a02*rkP2.X() + rkReps.a12*rkP2.Y() + rkReps.a22*rkP2.Z());

    // vector g
    Rational kG1 = rkP1.X()*rkReps.b0 + rkP1.Y()*rkReps.b1 +
        rkP1.Z()*rkReps.b2;
    Rational kG2 = rkP2.X()*rkReps.b0 + rkP2.Y()*rkReps.b1 +
        rkP2.Z()*rkReps.b2;

    // compute g^T*F^{-1}*g/4 - c
    Rational kFourDet = Rational(4)*(kF11*kF22 - kF12*kF12);
    Rational kR = (kG1*(kF22*kG1-kF12*kG2)+kG2*(kF11*kG2-kF12*kG1))/kFourDet
        - rkReps.c;

    if (kR > 0)
    {
        if (iPositiveRoots == 2)
        {
            return QT_ELLIPTIC_CYLINDER;
        }
        else if (iPositiveRoots == 1)
        {
            return QT_HYPERBOLIC_CYLINDER;
        }
        else
        {
            return QT_NONE;
        }
    }
    else if (kR < 0)
    {
        if (iPositiveRoots == 2)
        {
            return QT_NONE;
        }
        else if (iPositiveRoots == 1)
        {
            return QT_HYPERBOLIC_CYLINDER;
        }
        else
        {
            return QT_ELLIPTIC_CYLINDER;
        }
    }

    // else kR == 0
    return (iPositiveRoots == 1 ? QT_TWO_PLANES : QT_LINE);
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots2 (const RReps& rkReps,
    int iPositiveRoots)
{
    // Generate an orthonormal set {p0,p1,p2}, where p0 and p1 are
    // eigenvectors of A corresponding to eigenvalue zero.  The vector p2 is
    // an eigenvector of A corresponding to eigenvalue c2.
    QSVector kP0, kP1, kP2;

    if (rkReps.a00 != 0 || rkReps.a01 != 0 || rkReps.a02 != 0)
    {
        // row 0 is not zero
        kP2 = QSVector(rkReps.a00,rkReps.a01,rkReps.a02);
    }
    else if (rkReps.a01 != 0 || rkReps.a11 != 0 || rkReps.a12 != 0)
    {
        // row 1 is not zero
        kP2 = QSVector(rkReps.a01,rkReps.a11,rkReps.a12);
    }
    else
    {
        // row 2 is not zero
        kP2 = QSVector(rkReps.a02,rkReps.a12,rkReps.a22);
    }

    if (kP2.X() != 0)
    {
        kP1.X() = kP2.Y();
        kP1.Y() = -kP2.X();
        kP1.Z() = 0;
    }
    else
    {
        kP1.X() = 0;
        kP1.Y() = kP2.Z();
        kP1.Z() = -kP2.Y();
    }
    kP0 = kP1.Cross(kP2);

    return ClassifyZeroRoots2(rkReps,iPositiveRoots,kP0,kP1,kP2);
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots2 (const RReps& rkReps,
    int iPositiveRoots, const QSVector& rkP0, const QSVector& rkP1,
    const QSVector& rkP2)
{
    Rational kE0 = rkP0.X()*rkReps.b0 + rkP0.Y()*rkReps.b1 +
        rkP0.Z()*rkReps.b1;

    if (kE0 != 0)
    {
        return QT_PARABOLIC_CYLINDER;
    }

    Rational kE1 = rkP1.X()*rkReps.b0 + rkP1.Y()*rkReps.b1 +
        rkP1.Z()*rkReps.b1;

    if (kE1 != 0)
    {
        return QT_PARABOLIC_CYLINDER;
    }

    Rational kF2 = rkReps.c2*(rkP2.Dot(rkP2));
    Rational kE2 = rkP2.X()*rkReps.b0 + rkP2.Y()*rkReps.b1 +
        rkP2.Z()*rkReps.b1;

    Rational kR = kE2*kE2/(Rational(4)*kF2) - rkReps.c;
    if (kR > 0)
    {
        if (iPositiveRoots == 1)
        {
            return QT_TWO_PLANES;
        }
        else
        {
            return QT_NONE;
        }
    }
    else if (kR < 0)
    {
        if (iPositiveRoots == 1)
        {
            return QT_NONE;
        }
        else
        {
            return QT_TWO_PLANES;
        }
    }

    // else kR == 0
    return QT_PLANE;
}
//----------------------------------------------------------------------------
template <class Real>
int QuadricSurface<Real>::ClassifyZeroRoots3 (const RReps& rkReps)
{
    if (rkReps.b0 != 0 || rkReps.b1 != 0 || rkReps.b2 != 0)
    {
        return QT_PLANE;
    }

    return QT_NONE;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class QuadricSurface<float>;

template WM4_FOUNDATION_ITEM
class QuadricSurface<double>;
//----------------------------------------------------------------------------
}
