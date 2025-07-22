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
#include "Wm4ParametricSurface.h"
#include "Wm4Matrix2.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
ParametricSurface<Real>::ParametricSurface (Real fUMin, Real fUMax,
   Real fVMin, Real fVMax, bool bRectangular)
{
    assert(fUMin < fUMax && fVMin < fVMax);

    m_fUMin = fUMin;
    m_fUMax = fUMax;
    m_fVMin = fVMin;
    m_fVMax = fVMax;
    m_bRectangular = bRectangular;
}
//----------------------------------------------------------------------------
template <class Real>
ParametricSurface<Real>::~ParametricSurface ()
{
}
//----------------------------------------------------------------------------
template <class Real>
Real ParametricSurface<Real>::GetUMin () const
{
    return m_fUMin;
}
//----------------------------------------------------------------------------
template <class Real>
Real ParametricSurface<Real>::GetUMax () const
{
    return m_fUMax;
}
//----------------------------------------------------------------------------
template <class Real>
Real ParametricSurface<Real>::GetVMin () const
{
    return m_fVMin;
}
//----------------------------------------------------------------------------
template <class Real>
Real ParametricSurface<Real>::GetVMax () const
{
    return m_fVMax;
}
//----------------------------------------------------------------------------
template <class Real>
bool ParametricSurface<Real>::IsRectangular () const
{
    return m_bRectangular;
}
//----------------------------------------------------------------------------
template <class Real>
void ParametricSurface<Real>::GetFrame (Real fU, Real fV,
    Vector3<Real>& rkPosition, Vector3<Real>& rkTangent0,
    Vector3<Real>& rkTangent1, Vector3<Real>& rkNormal) const
{
    rkPosition = P(fU,fV);

    rkTangent0 = PU(fU,fV);
    rkTangent1 = PV(fU,fV);
    rkTangent0.Normalize();  // T0
    rkTangent1.Normalize();  // temporary T1 just to compute N
    rkNormal = rkTangent0.UnitCross(rkTangent1);  // N

    // The normalized first derivatives are not necessarily orthogonal.
    // Recompute T1 so that {T0,T1,N} is an orthonormal set.
    rkTangent1 = rkNormal.Cross(rkTangent0);
}
//----------------------------------------------------------------------------
template <class Real>
void ParametricSurface<Real>::ComputePrincipalCurvatureInfo (Real fU, Real fV,
    Real& rfCurv0, Real& rfCurv1, Vector3<Real>& rkDir0,
    Vector3<Real>& rkDir1)
{
    // Tangents:  T0 = (x_u,y_u,z_u), T1 = (x_v,y_v,z_v)
    // Normal:    N = Cross(T0,T1)/Length(Cross(T0,T1))
    // Metric Tensor:    G = +-                      -+
    //                       | Dot(T0,T0)  Dot(T0,T1) |
    //                       | Dot(T1,T0)  Dot(T1,T1) |
    //                       +-                      -+
    //
    // Curvature Tensor:  B = +-                          -+
    //                        | -Dot(N,T0_u)  -Dot(N,T0_v) |
    //                        | -Dot(N,T1_u)  -Dot(N,T1_v) |
    //                        +-                          -+
    //
    // Principal curvatures k are the generalized eigenvalues of
    //
    //     Bw = kGw
    //
    // If k is a curvature and w=(a,b) is the corresponding solution to
    // Bw = kGw, then the principal direction as a 3D vector is d = a*U+b*V.
    //
    // Let k1 and k2 be the principal curvatures.  The mean curvature
    // is (k1+k2)/2 and the Gaussian curvature is k1*k2.

    // derivatives
    Vector3<Real> kDerU = PU(fU,fV);
    Vector3<Real> kDerV = PV(fU,fV);
    Vector3<Real> kDerUU = PUU(fU,fV);
    Vector3<Real> kDerUV = PUV(fU,fV);
    Vector3<Real> kDerVV = PVV(fU,fV);

    // metric tensor
    Matrix2<Real> kMetricTensor;
    kMetricTensor[0][0] = kDerU.Dot(kDerU);
    kMetricTensor[0][1] = kDerU.Dot(kDerV);
    kMetricTensor[1][0] = kMetricTensor[0][1];
    kMetricTensor[1][1] = kDerV.Dot(kDerV);

    // curvature tensor
    Vector3<Real> kNormal = kDerU.UnitCross(kDerV);
    Matrix2<Real> kCurvatureTensor;
    kCurvatureTensor[0][0] = -kNormal.Dot(kDerUU);
    kCurvatureTensor[0][1] = -kNormal.Dot(kDerUV);
    kCurvatureTensor[1][0] = kCurvatureTensor[0][1];
    kCurvatureTensor[1][1] = -kNormal.Dot(kDerVV);

    // characteristic polynomial is 0 = det(B-kG) = c2*k^2+c1*k+c0
    Real fC0 = kCurvatureTensor.Determinant();
    Real fC1 = ((Real)2.0)*kCurvatureTensor[0][1]* kMetricTensor[0][1] -
        kCurvatureTensor[0][0]*kMetricTensor[1][1] -
        kCurvatureTensor[1][1]*kMetricTensor[0][0];
    Real fC2 = kMetricTensor.Determinant();

    // principal curvatures are roots of characteristic polynomial
    Real fTemp = Math<Real>::Sqrt(Math<Real>::FAbs(fC1*fC1 -
        ((Real)4.0)*fC0*fC2));
    Real fMult = ((Real)0.5)/fC2;
    rfCurv0 = -fMult*(fC1+fTemp);
    rfCurv1 = fMult*(-fC1+fTemp);

    // principal directions are solutions to (B-kG)w = 0
    // w1 = (b12-k1*g12,-(b11-k1*g11)) OR (b22-k1*g22,-(b12-k1*g12))
    Real fA0 = kCurvatureTensor[0][1] - rfCurv0*kMetricTensor[0][1];
    Real fA1 = rfCurv0*kMetricTensor[0][0] - kCurvatureTensor[0][0];
    Real fLength = Math<Real>::Sqrt(fA0*fA0+fA1*fA1);
    if (fLength >= Math<Real>::ZERO_TOLERANCE)
    {
        rkDir0 = fA0*kDerU + fA1*kDerV;
    }
    else
    {
        fA0 = kCurvatureTensor[1][1] - rfCurv0*kMetricTensor[1][1];
        fA1 = rfCurv0*kMetricTensor[0][1] - kCurvatureTensor[0][1];
        fLength = Math<Real>::Sqrt(fA0*fA0+fA1*fA1);
        if (fLength >= Math<Real>::ZERO_TOLERANCE)
        {
            rkDir0 = fA0*kDerU + fA1*kDerV;
        }
        else
        {
            // umbilic (surface is locally sphere, any direction principal)
            rkDir0 = kDerU;
        }
    }
    rkDir0.Normalize();

    // second tangent is cross product of first tangent and normal
    rkDir1 = rkDir0.Cross(kNormal);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class ParametricSurface<float>;

template WM4_FOUNDATION_ITEM
class ParametricSurface<double>;
//----------------------------------------------------------------------------
}
