// SPDX-License-Identifier: BSL-1.0

// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Matrix3.h"

namespace Wm4
{

template <class Real>
class Quaternion
{
public:
    // A quaternion is q = w + x*i + y*j + z*k where (w,x,y,z) is not
    // necessarily a unit length vector in 4D.

    // construction
    Quaternion ();  // uninitialized
    Quaternion (Real fW, Real fX, Real fY, Real fZ);
    Quaternion (const Quaternion& rkQ);

    // quaternion for the input rotation matrix
    Quaternion (const Matrix3<Real>& rkRot);

    // quaternion for the rotation of the axis-angle pair
    Quaternion (const Vector3<Real>& rkAxis, Real fAngle);

    // quaternion for the rotation matrix with specified columns
    Quaternion (const Vector3<Real> akRotColumn[3]);

    // member access:  0 = w, 1 = x, 2 = y, 3 = z
    inline operator const Real* () const;
    inline operator Real* ();
    inline Real operator[] (int i) const;
    inline Real& operator[] (int i);
    inline Real W () const;
    inline Real& W ();
    inline Real X () const;
    inline Real& X ();
    inline Real Y () const;
    inline Real& Y ();
    inline Real Z () const;
    inline Real& Z ();

    // assignment
    inline Quaternion& operator= (const Quaternion& rkQ);

    // comparison
    bool operator== (const Quaternion& rkQ) const;
    bool operator!= (const Quaternion& rkQ) const;
    bool operator<  (const Quaternion& rkQ) const;
    bool operator<= (const Quaternion& rkQ) const;
    bool operator>  (const Quaternion& rkQ) const;
    bool operator>= (const Quaternion& rkQ) const;

    // arithmetic operations
    inline Quaternion operator+ (const Quaternion& rkQ) const;
    inline Quaternion operator- (const Quaternion& rkQ) const;
    inline Quaternion operator* (const Quaternion& rkQ) const;
    inline Quaternion operator* (Real fScalar) const;
    inline Quaternion operator/ (Real fScalar) const;
    inline Quaternion operator- () const;

    // arithmetic updates
    inline Quaternion& operator+= (const Quaternion& rkQ);
    inline Quaternion& operator-= (const Quaternion& rkQ);
    inline Quaternion& operator*= (Real fScalar);
    inline Quaternion& operator/= (Real fScalar);

    // conversion between quaternions, matrices, and axis-angle
    Quaternion& FromRotationMatrix (const Matrix3<Real>& rkRot);
    void ToRotationMatrix (Matrix3<Real>& rkRot) const;
    Quaternion& FromRotationMatrix (const Vector3<Real> akRotColumn[3]);
    void ToRotationMatrix (Vector3<Real> akRotColumn[3]) const;
    Quaternion& FromAxisAngle (const Vector3<Real>& rkAxis, Real fAngle);
    void ToAxisAngle (Vector3<Real>& rkAxis, Real& rfAngle) const;

    // functions of a quaternion
    inline Real Length () const;  // length of 4-tuple
    inline Real SquaredLength () const;  // squared length of 4-tuple
    inline Real Dot (const Quaternion& rkQ) const;  // dot product of 4-tuples
    inline Real Normalize ();  // make the 4-tuple unit length
    Quaternion Inverse () const;  // apply to non-zero quaternion
    Quaternion Conjugate () const;
    Quaternion Exp () const;  // apply to quaternion with w = 0
    Quaternion Log () const;  // apply to unit-length quaternion

    // rotation of a vector by a quaternion
    Vector3<Real> Rotate (const Vector3<Real>& rkVector) const;

    // spherical linear interpolation
    Quaternion& Slerp (Real fT, const Quaternion& rkP, const Quaternion& rkQ);

    Quaternion& SlerpExtraSpins (Real fT, const Quaternion& rkP,
        const Quaternion& rkQ, int iExtraSpins);

    // intermediate terms for spherical quadratic interpolation
    Quaternion& Intermediate (const Quaternion& rkQ0,
        const Quaternion& rkQ1, const Quaternion& rkQ2);

    // spherical quadratic interpolation
    Quaternion& Squad (Real fT, const Quaternion& rkQ0,
        const Quaternion& rkA0, const Quaternion& rkA1,
        const Quaternion& rkQ1);

    // Compute a quaternion that rotates unit-length vector V1 to unit-length
    // vector V2.  The rotation is about the axis perpendicular to both V1 and
    // V2, with angle of that between V1 and V2.  If V1 and V2 are parallel,
    // any axis of rotation will do, such as the permutation (z2,x2,y2), where
    // V2 = (x2,y2,z2).
    Quaternion& Align (const Vector3<Real>& rkV1, const Vector3<Real>& rkV2);

    // Decompose a quaternion into q = q_twist * q_swing, where q is 'this'
    // quaternion.  If V1 is the input axis and V2 is the rotation of V1 by
    // q, q_swing represents the rotation about the axis perpendicular to
    // V1 and V2 (see Quaternion::Align), and q_twist is a rotation about V1.
    void DecomposeTwistTimesSwing (const Vector3<Real>& rkV1,
        Quaternion& rkTwist, Quaternion& rkSwing);

    // Decompose a quaternion into q = q_swing * q_twist, where q is 'this'
    // quaternion.  If V1 is the input axis and V2 is the rotation of V1 by
    // q, q_swing represents the rotation about the axis perpendicular to
    // V1 and V2 (see Quaternion::Align), and q_twist is a rotation about V1.
    void DecomposeSwingTimesTwist (const Vector3<Real>& rkV1,
        Quaternion& rkSwing, Quaternion& rkTwist);

    // *** Find closest quaternions with unconstrained angles.

    // Closest quaternion of the form (cx + sx*i).
    Quaternion GetClosestX () const;

    // Closest quaternion of the form (cy + sy*j).
    Quaternion GetClosestY () const;

    // Closest quaternion of the form (cz + sz*k).
    Quaternion GetClosestZ () const;

    // Closest quaternion of the form (cx + sx*i)*(cy + sy*j).
    Quaternion GetClosestXY () const;

    // Closest quaternion of the form (cy + sy*j)*(cx + sx*i).
    Quaternion GetClosestYX () const;

    // Closest quaternion of the form (cz + sz*k)*(cx + sx*i).
    Quaternion GetClosestZX () const;

    // Closest quaternion of the form (cx + sx*i)*(cz + sz*k).
    Quaternion GetClosestXZ () const;

    // Closest quaternion of the form (cy + sy*j)*(cz + sz*k).
    Quaternion GetClosestYZ () const;

    // Closest quaternion of the form (cz + sz*k)*(cy + sy*j).
    Quaternion GetClosestZY () const;

    // Factor to (cx + sx*i)*(cy + sy*j)*(cz + sz*k).
    void FactorXYZ (Real& rfCx, Real& rfSx, Real& rfCy, Real& rfSy,
        Real& rfCz, Real& rfSz);

    // Factor to (cx + sx*i)*(cz + sz*k)*(cy + sy*j).
    void FactorXZY (Real& rfCx, Real& rfSx, Real& rfCz, Real& rfSz,
        Real& rfCy, Real& rfSy);

    // Factor to (cy + sy*j)*(cz + sz*k)*(cx + sx*i).
    void FactorYZX (Real& rfCy, Real& rfSy, Real& rfCz, Real& rfSz,
        Real& rfCx, Real& rfSx);

    // Factor to (cy + sy*j)*(cx + sx*i)*(cz + sz*k).
    void FactorYXZ (Real& rfCy, Real& rfSy, Real& rfCx, Real& rfSx,
        Real& rfCz, Real& rfSz);

    // Factor to (cz + sz*k)*(cx + sx*i)*(cy + sy*j).
    void FactorZXY (Real& rfCz, Real& rfSz, Real& rfCx, Real& rfSx,
        Real& rfCy, Real& rfSy);

    // Factor to (cz + sz*k)*(cy + sy*j)*(cx + sx*i).
    void FactorZYX (Real& rfCz, Real& rfSz, Real& rfCy, Real& rfSy,
        Real& rfCx, Real& rfSx);

    // *** Find closest quaternions with constrained angles.
    class Constraints
    {
    public:
        Constraints ()
        {
            // Members are uninitialized.
        }

        Constraints (Real fMinAngle, Real fMaxAngle)
        {
            SetAngles(fMinAngle,fMaxAngle);
        }

        void SetAngles (Real fMinAngle, Real fMaxAngle)
        {
            m_fMinAngle = fMinAngle;
            m_fMaxAngle = fMaxAngle;
            m_fCosMinAngle = Math<Real>::Cos(m_fMinAngle);
            m_fSinMinAngle = Math<Real>::Sin(m_fMinAngle);
            m_fCosMaxAngle = Math<Real>::Cos(m_fMaxAngle);
            m_fSinMaxAngle = Math<Real>::Sin(m_fMaxAngle);
            m_fDiffCosMaxMin = m_fCosMaxAngle - m_fCosMinAngle;
            m_fDiffSinMaxMin = m_fSinMaxAngle - m_fSinMinAngle;
            Real fAvrAngle = ((Real)0.5)*(m_fMinAngle + m_fMaxAngle);
            m_fCosAvrAngle = Math<Real>::Cos(fAvrAngle);
            m_fSinAvrAngle = Math<Real>::Sin(fAvrAngle);
        }

        bool IsValid (Real fX, Real fY) const
        {
            // (x,y) must be unit-length.

            // Test whether (x,y) satisfies the constraints.
            Real fXm = fX - m_fCosMinAngle;
            Real fYm = fY - m_fSinMinAngle;
            if (fXm*m_fDiffSinMaxMin >= fYm*m_fDiffCosMaxMin)
            {
                return true;
            }

            // Test whether (-x,-y) satisfies the constraints.
            Real fXp = fX + m_fCosMinAngle;
            Real fYp = fY + m_fSinMinAngle;
            if (fXp*m_fDiffSinMaxMin <= fYp*m_fDiffCosMaxMin)
            {
                return true;
            }

            return false;
        }

        Real m_fMinAngle;       // in [-PI/2,PI/2]
        Real m_fMaxAngle;       // in [m_fMinAngle/2,PI/2]
        Real m_fCosMinAngle;    // = cos(m_fMinAngle)
        Real m_fSinMinAngle;    // = sin(m_fMinAngle)
        Real m_fCosMaxAngle;    // = cos(m_fMaxAngle)
        Real m_fSinMaxAngle;    // = sin(m_fMaxAngle)
        Real m_fDiffCosMaxMin;  // = cos(m_fMaxAngle) - cos(m_fMinAngle)
        Real m_fDiffSinMaxMin;  // = sin(m_fMaxAngle) - sin(m_fMinAngle)
        Real m_fCosAvrAngle;    // = cos((m_fMinAngle + m_fMaxAngle)/2)
        Real m_fSinAvrAngle;    // = sin((m_fMinAngle + mM_faxAngle)/2)
    };

    // Closest constrained quaternion of the form (cx + sx*i).
    Quaternion GetClosestX (const Constraints& rkXCon) const;

    // Closest constrained quaternion of the form (cy + sy*j).
    Quaternion GetClosestY (const Constraints& rkYCon) const;

    // Closest constrained quaternion of the form (cz + sz*k).
    Quaternion GetClosestZ (const Constraints& rkZCon) const;

    // Closest constrained quaternion of the form (cx + sx*i)*(cy + sy*j).
    Quaternion GetClosestXY (const Constraints& rkXCon,
        const Constraints& rkYCon) const;

    // Closest constrained quaternion of the form (cy + sy*j)*(cx + sx*i).
    Quaternion GetClosestYX (const Constraints& rkYCon,
        const Constraints& rkXCon) const;

    // Closest constrained quaternion of the form (cz + sz*k)*(cx + sx*i).
    Quaternion GetClosestZX (const Constraints& rkZCon,
        const Constraints& rkXCon) const;

    // Closest constrained quaternion of the form (cx + sx*i)*(cz + sz*k).
    Quaternion GetClosestXZ (const Constraints& rkXCon,
        const Constraints& rkZCon) const;

    // Closest constrained quaternion of the form (cz + sz*k)*(cy + sy*j).
    Quaternion GetClosestZY (const Constraints& rkZCon,
        const Constraints& rkYCon) const;

    // Closest constrained quaternion of the form (cy + sy*j)*(cz + sz*k).
    Quaternion GetClosestYZ (const Constraints& rkYCon,
        const Constraints& rkZCon) const;

    // special values
    WM4_FOUNDATION_ITEM static const Quaternion IDENTITY;
    WM4_FOUNDATION_ITEM static const Quaternion ZERO;

private:
    // support for comparisons
    int CompareArrays (const Quaternion& rkQ) const;

    // Closest unconstrained quaternion of the form:
    //   (cx + sx*i) when iAxis = 1,
    //   (cy + sy*j) when iAxis = 2,
    //   (cz + sz*k) when iAxis = 3
    Quaternion GetClosest (int iAxis) const;

    // Closest constrained quaternion of the form:
    //   (cx + sx*i) when iAxis = 1,
    //   (cy + sy*j) when iAxis = 2,
    //   (cz + sz*k) when iAxis = 3
    Quaternion GetClosest (int iAxis, const Constraints& rkCon) const;

    // support for FromRotationMatrix
    WM4_FOUNDATION_ITEM static int ms_iNext[3];

    // support for closest quaternions
    WM4_FOUNDATION_ITEM static Real ms_fTolerance;
    WM4_FOUNDATION_ITEM static Real ms_fRootTwo;
    WM4_FOUNDATION_ITEM static Real ms_fRootHalf;

    Real m_afTuple[4];
};

template <class Real>
inline Quaternion<Real> operator* (Real fScalar, const Quaternion<Real>& rkQ);

#include "Wm4Quaternion.inl"

typedef Quaternion<float> Quaternionf;
typedef Quaternion<double> Quaterniond;

}