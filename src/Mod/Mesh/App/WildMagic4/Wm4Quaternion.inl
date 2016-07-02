// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion ()
{
    // uninitialized for performance in array construction
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (Real fW, Real fX, Real fY, Real fZ)
{
    m_afTuple[0] = fW;
    m_afTuple[1] = fX;
    m_afTuple[2] = fY;
    m_afTuple[3] = fZ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Quaternion& rkQ)
{
    m_afTuple[0] = rkQ.m_afTuple[0];
    m_afTuple[1] = rkQ.m_afTuple[1];
    m_afTuple[2] = rkQ.m_afTuple[2];
    m_afTuple[3] = rkQ.m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Matrix3<Real>& rkRot)
{
    FromRotationMatrix(rkRot);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Vector3<Real>& rkAxis, Real fAngle)
{
    FromAxisAngle(rkAxis,fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>::Quaternion (const Vector3<Real> akRotColumn[3])
{
    FromRotationMatrix(akRotColumn);
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>::operator const Real* () const
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>::operator Real* ()
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::operator[] (int i) const
{
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real& Quaternion<Real>::operator[] (int i)
{
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::W () const
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real& Quaternion<Real>::W ()
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::X () const
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real& Quaternion<Real>::X ()
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::Y () const
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real& Quaternion<Real>::Y ()
{
    return m_afTuple[2];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::Z () const
{
    return m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real& Quaternion<Real>::Z ()
{
    return m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>& Quaternion<Real>::operator= (const Quaternion& rkQ)
{
    m_afTuple[0] = rkQ.m_afTuple[0];
    m_afTuple[1] = rkQ.m_afTuple[1];
    m_afTuple[2] = rkQ.m_afTuple[2];
    m_afTuple[3] = rkQ.m_afTuple[3];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
int Quaternion<Real>::CompareArrays (const Quaternion& rkQ) const
{
    return memcmp(m_afTuple,rkQ.m_afTuple,4*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator== (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator!= (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) != 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator< (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator<= (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator> (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Quaternion<Real>::operator>= (const Quaternion& rkQ) const
{
    return CompareArrays(rkQ) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator+ (
    const Quaternion& rkQ) const
{
    Quaternion kSum;
    for (int i = 0; i < 4; i++)
    {
        kSum.m_afTuple[i] = m_afTuple[i] + rkQ.m_afTuple[i];
    }
    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator- (
    const Quaternion& rkQ) const
{
    Quaternion kDiff;
    for (int i = 0; i < 4; i++)
    {
        kDiff.m_afTuple[i] = m_afTuple[i] - rkQ.m_afTuple[i];
    }
    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator* (
    const Quaternion& rkQ) const
{
    // NOTE:  Multiplication is not generally commutative, so in most
    // cases p*q != q*p.

    Quaternion kProd;

    kProd.m_afTuple[0] =
        m_afTuple[0]*rkQ.m_afTuple[0] -
        m_afTuple[1]*rkQ.m_afTuple[1] -
        m_afTuple[2]*rkQ.m_afTuple[2] -
        m_afTuple[3]*rkQ.m_afTuple[3];

    kProd.m_afTuple[1] =
        m_afTuple[0]*rkQ.m_afTuple[1] +
        m_afTuple[1]*rkQ.m_afTuple[0] +
        m_afTuple[2]*rkQ.m_afTuple[3] -
        m_afTuple[3]*rkQ.m_afTuple[2];

    kProd.m_afTuple[2] =
        m_afTuple[0]*rkQ.m_afTuple[2] +
        m_afTuple[2]*rkQ.m_afTuple[0] +
        m_afTuple[3]*rkQ.m_afTuple[1] -
        m_afTuple[1]*rkQ.m_afTuple[3];

    kProd.m_afTuple[3] =
        m_afTuple[0]*rkQ.m_afTuple[3] +
        m_afTuple[3]*rkQ.m_afTuple[0] +
        m_afTuple[1]*rkQ.m_afTuple[2] -
        m_afTuple[2]*rkQ.m_afTuple[1];

    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator* (Real fScalar) const
{
    Quaternion kProd;
    for (int i = 0; i < 4; i++)
    {
        kProd.m_afTuple[i] = fScalar*m_afTuple[i];
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator/ (Real fScalar) const
{
    Quaternion kQuot;
    int i;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 4; i++)
        {
            kQuot.m_afTuple[i] = fInvScalar*m_afTuple[i];
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            kQuot.m_afTuple[i] = Math<Real>::MAX_REAL;
        }
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> Quaternion<Real>::operator- () const
{
    Quaternion kNeg;
    for (int i = 0; i < 4; i++)
    {
        kNeg.m_afTuple[i] = -m_afTuple[i];
    }
    return kNeg;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real> operator* (Real fScalar, const Quaternion<Real>& rkQ)
{
    Quaternion<Real> kProd;
    for (int i = 0; i < 4; i++)
    {
        kProd[i] = fScalar*rkQ[i];
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>& Quaternion<Real>::operator+= (const Quaternion& rkQ)
{
    for (int i = 0; i < 4; i++)
    {
        m_afTuple[i] += rkQ.m_afTuple[i];
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>& Quaternion<Real>::operator-= (const Quaternion& rkQ)
{
    for (int i = 0; i < 4; i++)
    {
        m_afTuple[i] -= rkQ.m_afTuple[i];
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>& Quaternion<Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < 4; i++)
    {
        m_afTuple[i] *= fScalar;
    }
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
inline Quaternion<Real>& Quaternion<Real>::operator/= (Real fScalar)
{
    int i;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < 4; i++)
        {
            m_afTuple[i] *= fInvScalar;
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            m_afTuple[i] = Math<Real>::MAX_REAL;
        }
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::FromRotationMatrix (
    const Matrix3<Real>& rkRot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    Real fTrace = rkRot(0,0) + rkRot(1,1) + rkRot(2,2);
    Real fRoot;

    if (fTrace > (Real)0.0)
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = Math<Real>::Sqrt(fTrace + (Real)1.0);  // 2w
        m_afTuple[0] = ((Real)0.5)*fRoot;
        fRoot = ((Real)0.5)/fRoot;  // 1/(4w)
        m_afTuple[1] = (rkRot(2,1)-rkRot(1,2))*fRoot;
        m_afTuple[2] = (rkRot(0,2)-rkRot(2,0))*fRoot;
        m_afTuple[3] = (rkRot(1,0)-rkRot(0,1))*fRoot;
    }
    else
    {
        // |w| <= 1/2
        int i = 0;
        if ( rkRot(1,1) > rkRot(0,0) )
        {
            i = 1;
        }
        if ( rkRot(2,2) > rkRot(i,i) )
        {
            i = 2;
        }
        int j = ms_iNext[i];
        int k = ms_iNext[j];

        fRoot = Math<Real>::Sqrt(rkRot(i,i)-rkRot(j,j)-rkRot(k,k)+(Real)1.0);
        Real* apfQuat[3] = { &m_afTuple[1], &m_afTuple[2], &m_afTuple[3] };
        *apfQuat[i] = ((Real)0.5)*fRoot;
        fRoot = ((Real)0.5)/fRoot;
        m_afTuple[0] = (rkRot(k,j)-rkRot(j,k))*fRoot;
        *apfQuat[j] = (rkRot(j,i)+rkRot(i,j))*fRoot;
        *apfQuat[k] = (rkRot(k,i)+rkRot(i,k))*fRoot;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToRotationMatrix (Matrix3<Real>& rkRot) const
{
    Real fTx  = ((Real)2.0)*m_afTuple[1];
    Real fTy  = ((Real)2.0)*m_afTuple[2];
    Real fTz  = ((Real)2.0)*m_afTuple[3];
    Real fTwx = fTx*m_afTuple[0];
    Real fTwy = fTy*m_afTuple[0];
    Real fTwz = fTz*m_afTuple[0];
    Real fTxx = fTx*m_afTuple[1];
    Real fTxy = fTy*m_afTuple[1];
    Real fTxz = fTz*m_afTuple[1];
    Real fTyy = fTy*m_afTuple[2];
    Real fTyz = fTz*m_afTuple[2];
    Real fTzz = fTz*m_afTuple[3];

    rkRot(0,0) = (Real)1.0-(fTyy+fTzz);
    rkRot(0,1) = fTxy-fTwz;
    rkRot(0,2) = fTxz+fTwy;
    rkRot(1,0) = fTxy+fTwz;
    rkRot(1,1) = (Real)1.0-(fTxx+fTzz);
    rkRot(1,2) = fTyz-fTwx;
    rkRot(2,0) = fTxz-fTwy;
    rkRot(2,1) = fTyz+fTwx;
    rkRot(2,2) = (Real)1.0-(fTxx+fTyy);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::FromRotationMatrix (
    const Vector3<Real> akRotColumn[3])
{
    Matrix3<Real> kRot;
    for (int iCol = 0; iCol < 3; iCol++)
    {
        kRot(0,iCol) = akRotColumn[iCol][0];
        kRot(1,iCol) = akRotColumn[iCol][1];
        kRot(2,iCol) = akRotColumn[iCol][2];
    }
    return FromRotationMatrix(kRot);
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToRotationMatrix (Vector3<Real> akRotColumn[3]) const
{
    Matrix3<Real> kRot;
    ToRotationMatrix(kRot);
    for (int iCol = 0; iCol < 3; iCol++)
    {
        akRotColumn[iCol][0] = kRot(0,iCol);
        akRotColumn[iCol][1] = kRot(1,iCol);
        akRotColumn[iCol][2] = kRot(2,iCol);
    }
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::FromAxisAngle (
    const Vector3<Real>& rkAxis, Real fAngle)
{
    // assert:  axis[] is unit length
    //
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fHalfAngle = ((Real)0.5)*fAngle;
    Real fSin = Math<Real>::Sin(fHalfAngle);
    m_afTuple[0] = Math<Real>::Cos(fHalfAngle);
    m_afTuple[1] = fSin*rkAxis[0];
    m_afTuple[2] = fSin*rkAxis[1];
    m_afTuple[3] = fSin*rkAxis[2];

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::ToAxisAngle (Vector3<Real>& rkAxis, Real& rfAngle)
    const
{
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real fSqrLength = m_afTuple[1]*m_afTuple[1] + m_afTuple[2]*m_afTuple[2]
        + m_afTuple[3]*m_afTuple[3];

    if (fSqrLength > Math<Real>::ZERO_TOLERANCE)
    {
        rfAngle = ((Real)2.0)*Math<Real>::ACos(m_afTuple[0]);
        Real fInvLength = Math<Real>::InvSqrt(fSqrLength);
        rkAxis[0] = m_afTuple[1]*fInvLength;
        rkAxis[1] = m_afTuple[2]*fInvLength;
        rkAxis[2] = m_afTuple[3]*fInvLength;
    }
    else
    {
        // angle is 0 (mod 2*pi), so any axis will do
        rfAngle = (Real)0.0;
        rkAxis[0] = (Real)1.0;
        rkAxis[1] = (Real)0.0;
        rkAxis[2] = (Real)0.0;
    }
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::Length () const
{
    return Math<Real>::Sqrt(
        m_afTuple[0]*m_afTuple[0] +
        m_afTuple[1]*m_afTuple[1] +
        m_afTuple[2]*m_afTuple[2] +
        m_afTuple[3]*m_afTuple[3]);
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::SquaredLength () const
{
    return
        m_afTuple[0]*m_afTuple[0] +
        m_afTuple[1]*m_afTuple[1] +
        m_afTuple[2]*m_afTuple[2] +
        m_afTuple[3]*m_afTuple[3];
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::Dot (const Quaternion& rkQ) const
{
    Real fDot = (Real)0.0;
    for (int i = 0; i < 4; i++)
    {
        fDot += m_afTuple[i]*rkQ.m_afTuple[i];
    }
    return fDot;
}
//----------------------------------------------------------------------------
template <class Real>
inline Real Quaternion<Real>::Normalize ()
{
    Real fLength = Length();

    if (fLength > Math<Real>::ZERO_TOLERANCE)
    {
        Real fInvLength = ((Real)1.0)/fLength;
        m_afTuple[0] *= fInvLength;
        m_afTuple[1] *= fInvLength;
        m_afTuple[2] *= fInvLength;
        m_afTuple[3] *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        m_afTuple[0] = (Real)0.0;
        m_afTuple[1] = (Real)0.0;
        m_afTuple[2] = (Real)0.0;
        m_afTuple[3] = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Inverse () const
{
    Quaternion kInverse;

    Real fNorm = (Real)0.0;
    int i;
    for (i = 0; i < 4; i++)
    {
        fNorm += m_afTuple[i]*m_afTuple[i];
    }

    if (fNorm > (Real)0.0)
    {
        Real fInvNorm = ((Real)1.0)/fNorm;
        kInverse.m_afTuple[0] = m_afTuple[0]*fInvNorm;
        kInverse.m_afTuple[1] = -m_afTuple[1]*fInvNorm;
        kInverse.m_afTuple[2] = -m_afTuple[2]*fInvNorm;
        kInverse.m_afTuple[3] = -m_afTuple[3]*fInvNorm;
    }
    else
    {
        // return an invalid result to flag the error
        for (i = 0; i < 4; i++)
        {
            kInverse.m_afTuple[i] = (Real)0.0;
        }
    }

    return kInverse;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Conjugate () const
{
    return Quaternion(m_afTuple[0],-m_afTuple[1],-m_afTuple[2],
        -m_afTuple[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Exp () const
{
    // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
    // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

    Quaternion kResult;

    Real fAngle = Math<Real>::Sqrt(m_afTuple[1]*m_afTuple[1] +
        m_afTuple[2]*m_afTuple[2] + m_afTuple[3]*m_afTuple[3]);

    Real fSin = Math<Real>::Sin(fAngle);
    kResult.m_afTuple[0] = Math<Real>::Cos(fAngle);

    int i;

    if (Math<Real>::FAbs(fSin) >= Math<Real>::ZERO_TOLERANCE)
    {
        Real fCoeff = fSin/fAngle;
        for (i = 1; i <= 3; i++)
        {
            kResult.m_afTuple[i] = fCoeff*m_afTuple[i];
        }
    }
    else
    {
        for (i = 1; i <= 3; i++)
        {
            kResult.m_afTuple[i] = m_afTuple[i];
        }
    }

    return kResult;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::Log () const
{
    // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length, then
    // log(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use log(q) =
    // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

    Quaternion kResult;
    kResult.m_afTuple[0] = (Real)0.0;

    int i;

    if (Math<Real>::FAbs(m_afTuple[0]) < (Real)1.0)
    {
        Real fAngle = Math<Real>::ACos(m_afTuple[0]);
        Real fSin = Math<Real>::Sin(fAngle);
        if (Math<Real>::FAbs(fSin) >= Math<Real>::ZERO_TOLERANCE)
        {
            Real fCoeff = fAngle/fSin;
            for (i = 1; i <= 3; i++)
            {
                kResult.m_afTuple[i] = fCoeff*m_afTuple[i];
            }
            return kResult;
        }
    }

    for (i = 1; i <= 3; i++)
    {
        kResult.m_afTuple[i] = m_afTuple[i];
    }
    return kResult;
}
//----------------------------------------------------------------------------
template <class Real>
Vector3<Real> Quaternion<Real>::Rotate (const Vector3<Real>& rkVector)
    const
{
    // Given a vector u = (x0,y0,z0) and a unit length quaternion
    // q = <w,x,y,z>, the vector v = (x1,y1,z1) which represents the
    // rotation of u by q is v = q*u*q^{-1} where * indicates quaternion
    // multiplication and where u is treated as the quaternion <0,x0,y0,z0>.
    // Note that q^{-1} = <w,-x,-y,-z>, so no real work is required to
    // invert q.  Now
    //
    //   q*u*q^{-1} = q*<0,x0,y0,z0>*q^{-1}
    //     = q*(x0*i+y0*j+z0*k)*q^{-1}
    //     = x0*(q*i*q^{-1})+y0*(q*j*q^{-1})+z0*(q*k*q^{-1})
    //
    // As 3-vectors, q*i*q^{-1}, q*j*q^{-1}, and 2*k*q^{-1} are the columns
    // of the rotation matrix computed in Quaternion<Real>::ToRotationMatrix.
    // The vector v is obtained as the product of that rotation matrix with
    // vector u.  As such, the quaternion representation of a rotation
    // matrix requires less space than the matrix and more time to compute
    // the rotated vector.  Typical space-time tradeoff...

    Matrix3<Real> kRot;
    ToRotationMatrix(kRot);
    return kRot*rkVector;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::Slerp (Real fT, const Quaternion& rkP,
    const Quaternion& rkQ)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math<Real>::ACos(fCos);

    if (Math<Real>::FAbs(fAngle) >= Math<Real>::ZERO_TOLERANCE)
    {
        Real fSin = Math<Real>::Sin(fAngle);
        Real fInvSin = ((Real)1.0)/fSin;
        Real fTAngle = fT*fAngle;
        Real fCoeff0 = Math<Real>::Sin(fAngle - fTAngle)*fInvSin;
        Real fCoeff1 = Math<Real>::Sin(fTAngle)*fInvSin;

        // Profiling showed that the old line of code,
        //   *this = fCoeff0*rkP + fCoeff1*rkQ;
        // was using a large number of cycles, more so than the sin and cos
        // function calls.  The following inlined code is much faster.
        m_afTuple[0] = fCoeff0*rkP.m_afTuple[0] + fCoeff1*rkQ.m_afTuple[0];
        m_afTuple[1] = fCoeff0*rkP.m_afTuple[1] + fCoeff1*rkQ.m_afTuple[1];
        m_afTuple[2] = fCoeff0*rkP.m_afTuple[2] + fCoeff1*rkQ.m_afTuple[2];
        m_afTuple[3] = fCoeff0*rkP.m_afTuple[3] + fCoeff1*rkQ.m_afTuple[3];
    }
    else
    {
        // Based on the problem with the code in the previous block, inlining
        // the old code
        //   *this = rkP;
        m_afTuple[0] = rkP.m_afTuple[0];
        m_afTuple[1] = rkP.m_afTuple[1];
        m_afTuple[2] = rkP.m_afTuple[2];
        m_afTuple[3] = rkP.m_afTuple[3];
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::SlerpExtraSpins (Real fT,
    const Quaternion& rkP, const Quaternion& rkQ, int iExtraSpins)
{
    Real fCos = rkP.Dot(rkQ);
    Real fAngle = Math<Real>::ACos(fCos);

    if (Math<Real>::FAbs(fAngle) >= Math<Real>::ZERO_TOLERANCE)
    {
        Real fSin = Math<Real>::Sin(fAngle);
        Real fPhase = Math<Real>::PI*iExtraSpins*fT;
        Real fInvSin = ((Real)1.0)/fSin;
        Real fCoeff0 = Math<Real>::Sin(((Real)1.0-fT)*fAngle-fPhase)*fInvSin;
        Real fCoeff1 = Math<Real>::Sin(fT*fAngle + fPhase)*fInvSin;
        *this = fCoeff0*rkP + fCoeff1*rkQ;
    }
    else
    {
        *this = rkP;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::Intermediate (const Quaternion& rkQ0,
    const Quaternion& rkQ1, const Quaternion& rkQ2)
{
    // assert:  Q0, Q1, Q2 all unit-length
    Quaternion kQ1Inv = rkQ1.Conjugate();
    Quaternion kP0 = kQ1Inv*rkQ0;
    Quaternion kP2 = kQ1Inv*rkQ2;
    Quaternion kArg = -((Real)0.25)*(kP0.Log()+kP2.Log());
    Quaternion kA = rkQ1*kArg.Exp();
    *this = kA;

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::Squad (Real fT, const Quaternion& rkQ0,
    const Quaternion& rkA0, const Quaternion& rkA1, const Quaternion& rkQ1)
{
    Real fSlerpT = ((Real)2.0)*fT*((Real)1.0-fT);
    Quaternion kSlerpP = Slerp(fT,rkQ0,rkQ1);
    Quaternion kSlerpQ = Slerp(fT,rkA0,rkA1);
    return Slerp(fSlerpT,kSlerpP,kSlerpQ);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real>& Quaternion<Real>::Align (const Vector3<Real>& rkV1,
    const Vector3<Real>& rkV2)
{
    // If V1 and V2 are not parallel, the axis of rotation is the unit-length
    // vector U = Cross(V1,V2)/Length(Cross(V1,V2)).  The angle of rotation,
    // A, is the angle between V1 and V2.  The quaternion for the rotation is
    // q = cos(A/2) + sin(A/2)*(ux*i+uy*j+uz*k) where U = (ux,uy,uz).
    //
    // (1) Rather than extract A = acos(Dot(V1,V2)), multiply by 1/2, then
    //     compute sin(A/2) and cos(A/2), we reduce the computational costs by
    //     computing the bisector B = (V1+V2)/Length(V1+V2), so cos(A/2) =
    //     Dot(V1,B).
    //
    // (2) The rotation axis is U = Cross(V1,B)/Length(Cross(V1,B)), but
    //     Length(Cross(V1,B)) = Length(V1)*Length(B)*sin(A/2) = sin(A/2), in
    //     which case sin(A/2)*(ux*i+uy*j+uz*k) = (cx*i+cy*j+cz*k) where
    //     C = Cross(V1,B).
    //
    // If V1 = V2, then B = V1, cos(A/2) = 1, and U = (0,0,0).  If V1 = -V2,
    // then B = 0.  This can happen even if V1 is approximately -V2 using
    // floating point arithmetic, since Vector3::Normalize checks for
    // closeness to zero and returns the zero vector accordingly.  The test
    // for exactly zero is usually not recommend for floating point
    // arithmetic, but the implementation of Vector3::Normalize guarantees
    // the comparison is robust.  In this case, the A = pi and any axis
    // perpendicular to V1 may be used as the rotation axis.

    Vector3<Real> kBisector = rkV1 + rkV2;
    kBisector.Normalize();

    Real fCosHalfAngle = rkV1.Dot(kBisector);
    Vector3<Real> kCross;

    m_afTuple[0] = fCosHalfAngle;

    if (fCosHalfAngle != (Real)0.0)
    {
        kCross = rkV1.Cross(kBisector);
        m_afTuple[1] = kCross.X();
        m_afTuple[2] = kCross.Y();
        m_afTuple[3] = kCross.Z();
    }
    else
    {
        Real fInvLength;
        if (Math<Real>::FAbs(rkV1[0]) >= Math<Real>::FAbs(rkV1[1]))
        {
            // V1.x or V1.z is the largest magnitude component
            fInvLength = Math<Real>::InvSqrt(rkV1[0]*rkV1[0] +
                rkV1[2]*rkV1[2]);

            m_afTuple[1] = -rkV1[2]*fInvLength;
            m_afTuple[2] = (Real)0.0;
            m_afTuple[3] = +rkV1[0]*fInvLength;
        }
        else
        {
            // V1.y or V1.z is the largest magnitude component
            fInvLength = Math<Real>::InvSqrt(rkV1[1]*rkV1[1] +
                rkV1[2]*rkV1[2]);

            m_afTuple[1] = (Real)0.0;
            m_afTuple[2] = +rkV1[2]*fInvLength;
            m_afTuple[3] = -rkV1[1]*fInvLength;
        }
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::DecomposeTwistTimesSwing (
    const Vector3<Real>& rkV1, Quaternion& rkTwist, Quaternion& rkSwing)
{
    Vector3<Real> kV2 = Rotate(rkV1);
    rkSwing = Align(rkV1,kV2);
    rkTwist = (*this)*rkSwing.Conjugate();
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::DecomposeSwingTimesTwist (
    const Vector3<Real>& rkV1, Quaternion& rkSwing, Quaternion& rkTwist)
{
    Vector3<Real> kV2 = Rotate(rkV1);
    rkSwing = Align(rkV1,kV2);
    rkTwist = rkSwing.Conjugate()*(*this);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosest (int iAxis) const
{
    // The appropriate nonzero components will be set later.
    Quaternion kQ((Real)0,(Real)0,(Real)0,(Real)0);
    Real fP0 = m_afTuple[0];
    Real fP1 = m_afTuple[iAxis];
    Real fSqrLength = fP0*fP0 + fP1*fP1;
    if (fSqrLength > ms_fTolerance)
    {
        // A unique closest point.
        Real fInvLength = Math<Real>::InvSqrt(fSqrLength);
        kQ[0] = fP0*fInvLength;
        kQ[iAxis] = fP1*fInvLength;
    }
    else
    {
        // Infinitely many solutions, choose the one for theta = 0.
        kQ[0] = (Real)1;
        kQ[iAxis] = (Real)0;
    }
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestX () const
{
    return GetClosest(1);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestY () const
{
    return GetClosest(2);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZ () const
{
    return GetClosest(3);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestXY () const
{
    Quaternion kQ;

    Real fDet = m_afTuple[0]*m_afTuple[3] - m_afTuple[1]*m_afTuple[2];
    if(Math<Real>::FAbs(fDet) < (Real)0.5 - ms_fTolerance)
    {
        Real fDiscr = (Real)1 - ((Real)4)*fDet*fDet;
        fDiscr = Math<Real>::Sqrt(Math<Real>::FAbs(fDiscr));
        Real fA = m_afTuple[0]*m_afTuple[1] + m_afTuple[2]*m_afTuple[3];
        Real fB = m_afTuple[0]*m_afTuple[0] - m_afTuple[1]*m_afTuple[1] +
            m_afTuple[2]*m_afTuple[2] - m_afTuple[3]*m_afTuple[3];

        Real fC0, fS0, fC1, fS1, fInvLength;

        if (fB >= (Real)0)
        {
            fC0 = ((Real)0.5)*(fDiscr + fB);
            fS0 = fA;
        }
        else
        {
            fC0 = fA;
            fS0 = ((Real)0.5)*(fDiscr - fB);
        }
        fInvLength = Math<Real>::InvSqrt(fC0*fC0 + fS0*fS0);
        fC0 *= fInvLength;
        fS0 *= fInvLength;

        fC1 = m_afTuple[0]*fC0 + m_afTuple[1]*fS0;
        fS1 = m_afTuple[2]*fC0 + m_afTuple[3]*fS0;
        fInvLength = Math<Real>::InvSqrt(fC1*fC1 + fS1*fS1);
        fC1 *= fInvLength;
        fS1 *= fInvLength;

        kQ[0] = fC0*fC1;
        kQ[1] = fS0*fC1;
        kQ[2] = fC0*fS1;
        kQ[3] = fS0*fS1;
    }
    else
    {
        Real fInvLength = Math<Real>::InvSqrt(Math<Real>::FAbs(fDet));
        kQ[0] = m_afTuple[0]*fInvLength;
        kQ[1] = m_afTuple[1]*fInvLength;
        kQ[2] = (Real)0;
        kQ[3] = (Real)0;
    }

    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestYX () const
{
    Quaternion kAlt(m_afTuple[0],m_afTuple[1],m_afTuple[2],-m_afTuple[3]);
    Quaternion kQ = kAlt.GetClosestXY();
    kQ[3] = -kQ[3];
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZX () const
{
    Quaternion kAlt(m_afTuple[0],m_afTuple[1],m_afTuple[3],m_afTuple[2]);
    Quaternion kQ = kAlt.GetClosestXY();
    Real fSave = kQ[2];
    kQ[2] = kQ[3];
    kQ[3] = fSave;
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestXZ () const
{
    Quaternion kAlt(m_afTuple[0],m_afTuple[1],-m_afTuple[3],m_afTuple[2]);
    Quaternion kQ = kAlt.GetClosestXY();
    Real fSave = kQ[2];
    kQ[2] = kQ[3];
    kQ[3] = -fSave;
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestYZ () const
{
    Quaternion kAlt(m_afTuple[0],m_afTuple[2],m_afTuple[3],m_afTuple[1]);
    Quaternion kQ = kAlt.GetClosestXY();
    Real fSave = kQ[3];
    kQ[3] = kQ[2];
    kQ[2] = kQ[1];
    kQ[1] = fSave;
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZY () const
{
    Quaternion kAlt(m_afTuple[0],m_afTuple[2],m_afTuple[3],-m_afTuple[1]);
    Quaternion kQ = kAlt.GetClosestXY();
    Real fSave = kQ[3];
    kQ[3] = kQ[2];
    kQ[2] = kQ[1];
    kQ[1] = -fSave;
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorXYZ (Real& rfCx, Real& rfSx, Real& rfCy,
    Real& rfSy, Real& rfCz, Real& rfSz)
{
    Real fA = m_afTuple[0]*m_afTuple[1] - m_afTuple[2]*m_afTuple[3];
    Real fB = ((Real)0.5)*(
          m_afTuple[0]*m_afTuple[0]
        - m_afTuple[1]*m_afTuple[1]
        - m_afTuple[2]*m_afTuple[2]
        + m_afTuple[3]*m_afTuple[3]);

    Real fLength = Math<Real>::Sqrt(fA*fA + fB*fB);
    if (fLength > ms_fTolerance)
    {
        Real fInvLength = ((Real)1)/fLength;
        Real fSigma0 = fA * fInvLength;
        Real fGamma0 = fB * fInvLength;
        if (fGamma0 >= (Real)0)
        {
            rfCx = Math<Real>::Sqrt(((Real)0.5)*((Real)1 + fGamma0));
            rfSx = ((Real)0.5)*fSigma0/rfCx;
        }
        else
        {
            rfSx = Math<Real>::Sqrt(((Real)0.5)*((Real)1 - fGamma0));
            rfCx = ((Real)0.5)*fSigma0/rfSx;
        }

        Real fTmp0 = rfCx*m_afTuple[0] + rfSx*m_afTuple[1];
        Real fTmp1 = rfCx*m_afTuple[3] - rfSx*m_afTuple[2];
        fInvLength = Math<Real>::InvSqrt(fTmp0*fTmp0 + fTmp1*fTmp1);
        rfCz = fTmp0 * fInvLength;
        rfSz = fTmp1 * fInvLength;

        if(Math<Real>::FAbs(rfCz) >= Math<Real>::FAbs(rfSz))
        {
            fInvLength = ((Real)1)/rfCz;
            rfCy = fTmp0 * fInvLength;
            rfSy = (rfCx*m_afTuple[2] + rfSx*m_afTuple[3]) * fInvLength;
        }
        else
        {
            fInvLength = ((Real)1)/rfSz;
            rfCy = fTmp1 * fInvLength;
            rfSy = (rfCx*m_afTuple[1] - rfSx*m_afTuple[0]) * fInvLength;
        }
    }
    else
    {
        // Infinitely many solutions.  Choose one of them.
        if(m_afTuple[0]*m_afTuple[2] + m_afTuple[1]*m_afTuple[3] > (Real)0)
        {
            // p = (p0,p1,p0,p1)
            rfCx = (Real)1;
            rfSx = (Real)0;
            rfCy = ms_fRootHalf;
            rfSy = ms_fRootHalf;
            rfCz = ms_fRootTwo * m_afTuple[0];
            rfSz = ms_fRootTwo * m_afTuple[1];
        }
        else
        {
            // p = (p0,p1,-p0,-p1)
            rfCx = (Real)1;
            rfSx = (Real)0;
            rfCy = ms_fRootHalf;
            rfSy = -ms_fRootHalf;
            rfCz = ms_fRootTwo * m_afTuple[0];
            rfSz = -ms_fRootTwo * m_afTuple[1];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorXZY (Real& rfCx, Real& rfSx, Real& rfCz,
    Real& rfSz, Real& rfCy, Real& rfSy)
{
    Quaternion pkAlt(m_afTuple[0],m_afTuple[1],m_afTuple[3],-m_afTuple[2]);
    pkAlt.FactorXYZ(rfCx,rfSx,rfCz,rfSz,rfCy,rfSy);
    rfSy = -rfSy;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorYZX (Real& rfCy, Real& rfSy, Real& rfCz,
    Real& rfSz, Real& rfCx, Real& rfSx)
{
    Quaternion pkAlt(m_afTuple[0],-m_afTuple[2],m_afTuple[3],-m_afTuple[1]);
    pkAlt.FactorXYZ(rfCy,rfSy,rfCz,rfSz,rfCx,rfSx);
    rfSx = -rfSx;
    rfSy = -rfSy;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorYXZ (Real& rfCy, Real& rfSy, Real& rfCx,
    Real& rfSx, Real& rfCz, Real& rfSz)
{
    Quaternion pkAlt(m_afTuple[0],-m_afTuple[2],m_afTuple[1],m_afTuple[3]);
    pkAlt.FactorXYZ(rfCy,rfSy,rfCx,rfSx,rfCz,rfSz);
    rfSy = -rfSy;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorZXY (Real& rfCz, Real& rfSz, Real& rfCx,
    Real& rfSx, Real& rfCy, Real& rfSy)
{
    Quaternion pkAlt(m_afTuple[0],-m_afTuple[3],m_afTuple[1],-m_afTuple[2]);
    pkAlt.FactorXYZ(rfCz,rfSz,rfCx,rfSx,rfCy,rfSy);
    rfSy = -rfSy;
    rfSz = -rfSz;
}
//----------------------------------------------------------------------------
template <class Real>
void Quaternion<Real>::FactorZYX (Real& rfCz, Real& rfSz, Real& rfCy,
    Real& rfSy, Real& rfCx, Real& rfSx)
{
    Quaternion pkAlt(m_afTuple[0],m_afTuple[3],-m_afTuple[2],m_afTuple[1]);
    pkAlt.FactorXYZ(rfCz,rfSz,rfCy,rfSy,rfCx,rfSx);
    rfSy = -rfSy;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosest (int iAxis,
    const Constraints& rkCon) const
{
    Quaternion kQ((Real)0,(Real)0,(Real)0,(Real)0);

    Real fP0 = m_afTuple[0];
    Real fP1 = m_afTuple[iAxis];
    Real fSqrLength = fP0*fP0 + fP1*fP1;
    if (fSqrLength > ms_fTolerance)
    {
        Real fInvLength = Math<Real>::InvSqrt(fSqrLength);
        fP0 *= fInvLength;
        fP1 *= fInvLength;
        if (rkCon.IsValid(fP0,fP1))
        {
            // The maximum occurs at an interior point.
            kQ[0] = fP0;
            kQ[iAxis] = fP1;
        }
        else
        {
            // The maximum occurs at a boundary point.
            Real fCsMin = rkCon.m_fCosMinAngle;
            Real fSnMin = rkCon.m_fSinMinAngle;
            Real fDotMinAngle = fP0*fCsMin + fP1*fSnMin;
            if (fDotMinAngle < (Real)0)
            {
                fCsMin = -fCsMin;
                fSnMin = -fSnMin;
                fDotMinAngle = -fDotMinAngle;
            }

            Real fCsMax = rkCon.m_fCosMaxAngle;
            Real fSnMax = rkCon.m_fSinMaxAngle;
            Real fDotMaxAngle = fP0*fCsMax + fP1*fSnMax;
            if (fDotMaxAngle < (Real)0)
            {
                fCsMax = -fCsMax;
                fSnMax = -fSnMax;
                fDotMaxAngle = -fDotMaxAngle;
            }

            if (fDotMaxAngle >= fDotMinAngle)
            {
                kQ[0] = fCsMax;
                kQ[iAxis] = fSnMax;
            }
            else
            {
                kQ[0] = fCsMin;
                kQ[iAxis] = fSnMin;
            }
        }
    }
    else
    {
        // Infinitely many solutions, choose one that satisfies the angle
        // constraints.
        kQ[0] = rkCon.m_fCosAvrAngle;
        kQ[iAxis] = rkCon.m_fSinAvrAngle;
    }

    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestX (const Constraints& rkXCon)
    const
{
    return GetClosest(1,rkXCon);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestY (const Constraints& rkYCon)
    const
{
    return GetClosest(2,rkYCon);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZ (const Constraints& rkZCon)
    const
{
    return GetClosest(3,rkZCon);
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestXY (const Constraints& rkXCon,
    const Constraints& rkYCon) const
{
    Quaternion kQ, kTmp;
    Real fC0, fS0, fC1, fS1, fInvLength;

    Real fDet = m_afTuple[0]*m_afTuple[3] - m_afTuple[1]*m_afTuple[2];
    if (Math<Real>::FAbs(fDet) < (Real)0.5 - ms_fTolerance)
    {
        Real fDiscr = Math<Real>::Sqrt(Math<Real>::FAbs((Real)1 -
            ((Real)4)*fDet*fDet));
        Real fA = m_afTuple[0]*m_afTuple[1] + m_afTuple[2]*m_afTuple[3];
        Real fB = m_afTuple[0]*m_afTuple[0] - m_afTuple[1]*m_afTuple[1]
            + m_afTuple[2]*m_afTuple[2] - m_afTuple[3]*m_afTuple[3];

        if (fB >= (Real)0)
        {
            fC0 = ((Real)0.5)*(fDiscr + fB);
            fS0 = fA;
        }
        else
        {
            fC0 = fA;
            fS0 = ((Real)0.5)*(fDiscr - fB);
        }
        fInvLength = Math<Real>::InvSqrt(fC0*fC0 + fS0*fS0);
        fC0 *= fInvLength;
        fS0 *= fInvLength;

        fC1 = m_afTuple[0]*fC0 + m_afTuple[1]*fS0;
        fS1 = m_afTuple[2]*fC0 + m_afTuple[3]*fS0;
        fInvLength = Math<Real>::InvSqrt(fC1*fC1 + fS1*fS1);
        fC1 *= fInvLength;
        fS1 *= fInvLength;

        if (rkXCon.IsValid(fC0,fS0) && rkYCon.IsValid(fC1,fS1))
        {
            // The maximum occurs at an interior point.
            kQ[0] = fC0*fC1;
            kQ[1] = fS0*fC1;
            kQ[2] = fC0*fS1;
            kQ[3] = fS0*fS1;
        }
        else
        {
            // The maximum occurs at a boundary point.
            Quaternion kR(rkXCon.m_fCosMinAngle,rkXCon.m_fSinMinAngle,
                (Real)0,(Real)0);
            Quaternion kRInv(rkXCon.m_fCosMinAngle,-rkXCon.m_fSinMinAngle,
                (Real)0,(Real)0);
            Quaternion kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(2,rkYCon);
            Real fDotOptAngle = kProd.Dot(kTmp);
            kQ = kR*kTmp;

            kR = Quaternion(rkXCon.m_fCosMaxAngle,rkXCon.m_fSinMaxAngle,
                (Real)0,(Real)0);
            kRInv = Quaternion(rkXCon.m_fCosMaxAngle,-rkXCon.m_fSinMaxAngle,
                (Real)0,(Real)0);
            kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(2,rkYCon);
            Real fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kR*kTmp;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkYCon.m_fCosMinAngle,(Real)0,
                rkYCon.m_fSinMinAngle,(Real)0);
            kRInv = Quaternion(rkYCon.m_fCosMinAngle,(Real)0,
                -rkYCon.m_fSinMinAngle,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(1,rkXCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkYCon.m_fCosMaxAngle,(Real)0,
                rkYCon.m_fSinMaxAngle,(Real)0);
            kRInv = Quaternion(rkYCon.m_fCosMaxAngle,(Real)0,
                -rkYCon.m_fSinMaxAngle,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(1,rkXCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }
        }
    }
    else
    {
        // Infinitely many solutions, choose one that satisfies the angle
        // constraints.
        Real fMinAngle, fMaxAngle, fAngle;
        Constraints kCon;

        if (fDet > (Real)0)
        {
            fMinAngle = rkXCon.m_fMinAngle - rkYCon.m_fMaxAngle;
            fMaxAngle = rkXCon.m_fMaxAngle - rkYCon.m_fMinAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(1,kCon);

            fAngle = Math<Real>::ATan2(kTmp[1],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -= 
                    (kTmp[1] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle <= rkXCon.m_fMaxAngle - rkYCon.m_fMaxAngle)
            {
                fC1 = rkYCon.m_fCosMaxAngle;
                fS1 = rkYCon.m_fSinMaxAngle;
                fAngle = rkYCon.m_fMaxAngle + fAngle;
                fC0 = Math<Real>::Cos(fAngle);
                fS0 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC0 = rkXCon.m_fCosMaxAngle;
                fS0 = rkXCon.m_fSinMaxAngle;
                fAngle = rkXCon.m_fMaxAngle - fAngle;
                fC1 = Math<Real>::Cos(fAngle);
                fS1 = Math<Real>::Sin(fAngle);
            }
        }
        else
        {
            fMinAngle = rkXCon.m_fMinAngle + rkYCon.m_fMinAngle;
            fMaxAngle = rkXCon.m_fMaxAngle + rkYCon.m_fMaxAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(1,kCon);

            fAngle = Math<Real>::ATan2(kTmp[1],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -=
                    (kTmp[1] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle >= rkXCon.m_fMinAngle + rkYCon.m_fMaxAngle)
            {
                fC1 = rkYCon.m_fCosMaxAngle;
                fS1 = rkYCon.m_fSinMaxAngle;
                fAngle = fAngle - rkYCon.m_fMaxAngle;
                fC0 = Math<Real>::Cos(fAngle);
                fS0 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC0 = rkXCon.m_fCosMaxAngle;
                fS0 = rkXCon.m_fSinMaxAngle;
                fAngle = fAngle - rkXCon.m_fMaxAngle;
                fC1 = Math<Real>::Cos(fAngle);
                fS1 = Math<Real>::Sin(fAngle);
            }
        }

        kQ[0] = fC0*fC1;
        kQ[1] = fS0*fC1;
        kQ[2] = fC0*fS1;
        kQ[3] = fS0*fS1;
        if (Dot(kQ) < (Real)0)
        {
            kQ = -kQ;
        }
    }

    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestYX (const Constraints& rkYCon,
    const Constraints& rkXCon) const
{
    Quaternion pkAlt(m_afTuple[0],m_afTuple[1],m_afTuple[2],-m_afTuple[3]);
    Quaternion kQ = pkAlt.GetClosestXY(rkXCon,rkYCon);
    kQ[3] = -kQ[3];
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZX (const Constraints& rkZCon,
    const Constraints& rkXCon) const
{
    Quaternion kQ, kTmp;
    Real fC2, fS2, fC0, fS0, fInvLength;

    Real fDet = m_afTuple[0]*m_afTuple[2] - m_afTuple[1]*m_afTuple[3];
    if (Math<Real>::FAbs(fDet) < (Real)0.5 - ms_fTolerance)
    {
        Real fDiscr = Math<Real>::Sqrt(Math<Real>::FAbs((Real)1 -
            ((Real)4)*fDet*fDet));
        Real fA = m_afTuple[0]*m_afTuple[3] + m_afTuple[1]*m_afTuple[2];
        Real fB = m_afTuple[0]*m_afTuple[0] + m_afTuple[1]*m_afTuple[1]
            - m_afTuple[2]*m_afTuple[2] - m_afTuple[3]*m_afTuple[3];

        if (fB >= (Real)0)
        {
            fC2 = ((Real)0.5)*(fDiscr + fB);
            fS2 = fA;
        }
        else
        {
            fC2 = fA;
            fS2 = ((Real)0.5)*(fDiscr - fB);
        }
        fInvLength = Math<Real>::InvSqrt(fC2*fC2 + fS2*fS2);
        fC2 *= fInvLength;
        fS2 *= fInvLength;

        fC0 = m_afTuple[0]*fC2 + m_afTuple[3]*fS2;
        fS0 = m_afTuple[1]*fC2 + m_afTuple[2]*fS2;
        fInvLength = Math<Real>::InvSqrt(fC0*fC0 + fS0*fS0);
        fC0 *= fInvLength;
        fS0 *= fInvLength;

        if (rkZCon.IsValid(fC2,fS2) && rkXCon.IsValid(fC0,fS0))
        {
            // The maximum occurs at an interior point.
            kQ[0] = fC2*fC0;
            kQ[1] = fC2*fS0;
            kQ[2] = fS2*fS0;
            kQ[3] = fS2*fC0;
        }
        else
        {
            // The maximum occurs at a boundary point.
            Quaternion kR(rkZCon.m_fCosMinAngle,(Real)0,(Real)0,
                rkZCon.m_fSinMinAngle);
            Quaternion kRInv(rkZCon.m_fCosMinAngle,(Real)0,(Real)0,
                -rkZCon.m_fSinMinAngle);
            Quaternion kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(1,rkXCon);
            Real fDotOptAngle = kProd.Dot(kTmp);
            kQ = kR*kTmp;

            kR = Quaternion(rkZCon.m_fCosMaxAngle,(Real)0,(Real)0,
                rkZCon.m_fSinMaxAngle);
            kRInv = Quaternion(rkZCon.m_fCosMaxAngle,(Real)0,(Real)0,
                -rkZCon.m_fSinMaxAngle);
            kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(1,rkXCon);
            Real fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kR*kTmp;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkXCon.m_fCosMinAngle,rkXCon.m_fSinMinAngle,
                (Real)0,(Real)0);
            kRInv = Quaternion(rkXCon.m_fCosMinAngle,-rkXCon.m_fSinMinAngle,
                (Real)0,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(3,rkZCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkXCon.m_fCosMaxAngle,rkXCon.m_fSinMaxAngle,
                (Real)0,(Real)0);
            kRInv = Quaternion(rkXCon.m_fCosMaxAngle,-rkXCon.m_fSinMaxAngle,
                (Real)0,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(3,rkZCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }
        }
    }
    else
    {
        // Infinitely many solutions, choose one that satisfies the angle
        // constraints.
        Real fMinAngle, fMaxAngle, fAngle;
        Constraints kCon;

        if (fDet > (Real)0)
        {
            fMinAngle = rkXCon.m_fMinAngle - rkZCon.m_fMaxAngle;
            fMaxAngle = rkXCon.m_fMaxAngle - rkZCon.m_fMinAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(1,kCon);

            fAngle = Math<Real>::ATan2(kTmp[1],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -=
                    (kTmp[1] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle <= rkXCon.m_fMaxAngle - rkZCon.m_fMaxAngle)
            {
                fC2 = rkZCon.m_fCosMaxAngle;
                fS2 = rkZCon.m_fSinMaxAngle;
                fAngle = rkZCon.m_fMaxAngle + fAngle;
                fC0 = Math<Real>::Cos(fAngle);
                fS0 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC0 = rkXCon.m_fCosMaxAngle;
                fS0 = rkXCon.m_fSinMaxAngle;
                fAngle = rkXCon.m_fMaxAngle - fAngle;
                fC2 = Math<Real>::Cos(fAngle);
                fS2 = Math<Real>::Sin(fAngle);
            }
        }
        else
        {
            fMinAngle = rkXCon.m_fMinAngle + rkZCon.m_fMinAngle;
            fMaxAngle = rkXCon.m_fMaxAngle + rkZCon.m_fMaxAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(1,kCon);

            fAngle = Math<Real>::ATan2(kTmp[1],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -=
                    (kTmp[1] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle >= rkXCon.m_fMinAngle + rkZCon.m_fMaxAngle)
            {
                fC2 = rkZCon.m_fCosMaxAngle;
                fS2 = rkZCon.m_fSinMaxAngle;
                fAngle = fAngle - rkZCon.m_fMaxAngle;
                fC0 = Math<Real>::Cos(fAngle);
                fS0 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC0 = rkXCon.m_fCosMaxAngle;
                fS0 = rkXCon.m_fSinMaxAngle;
                fAngle = fAngle - rkXCon.m_fMaxAngle;
                fC2 = Math<Real>::Cos(fAngle);
                fS2 = Math<Real>::Sin(fAngle);
            }
        }

        kQ[0] = fC2*fC0;
        kQ[1] = fC2*fS0;
        kQ[2] = fS2*fS0;
        kQ[3] = fS2*fC0;
        if (Dot(kQ) < (Real)0)
        {
            kQ = -kQ;
        }
    }

    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestXZ (const Constraints& rkXCon,
    const Constraints& rkZCon) const
{
    Quaternion pkAlt(m_afTuple[0],m_afTuple[1],-m_afTuple[2],m_afTuple[3]);
    Quaternion kQ = pkAlt.GetClosestZX(rkZCon,rkXCon);
    kQ[2] = -kQ[2];
    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestZY (const Constraints& rkZCon,
    const Constraints& rkYCon) const
{
    Quaternion kQ, kTmp;
    Real fC2, fS2, fC1, fS1, fInvLength;

    Real fDet = m_afTuple[0]*m_afTuple[1] + m_afTuple[2]*m_afTuple[3];
    if (Math<Real>::FAbs(fDet) < (Real)0.5 - ms_fTolerance)
    {
        Real fDiscr = Math<Real>::Sqrt(Math<Real>::FAbs((Real)1 -
            ((Real)4)*fDet*fDet));
        Real fA = m_afTuple[0]*m_afTuple[3] - m_afTuple[1]*m_afTuple[2];
        Real fB = m_afTuple[0]*m_afTuple[0] - m_afTuple[1]*m_afTuple[1]
            + m_afTuple[2]*m_afTuple[2] - m_afTuple[3]*m_afTuple[3];

        if (fB >= (Real)0)
        {
            fC2 = ((Real)0.5)*(fDiscr + fB);
            fS2 = fA;
        }
        else
        {
            fC2 = fA;
            fS2 = ((Real)0.5)*(fDiscr - fB);
        }
        fInvLength = Math<Real>::InvSqrt(fC2*fC2 + fS2*fS2);
        fC2 *= fInvLength;
        fS2 *= fInvLength;

        fC1 = m_afTuple[0]*fC2 + m_afTuple[3]*fS2;
        fS1 = m_afTuple[2]*fC2 - m_afTuple[1]*fS2;
        fInvLength = Math<Real>::InvSqrt(fC1*fC1 + fS1*fS1);
        fC1 *= fInvLength;
        fS1 *= fInvLength;

        if (rkZCon.IsValid(fC2,fS2) && rkYCon.IsValid(fC1,fS1))
        {
            // The maximum occurs at an interior point.
            kQ[0] = fC2*fC1;
            kQ[1] = -fS2*fS1;
            kQ[2] = fC2*fS1;
            kQ[3] = fS2*fC1;
        }
        else
        {
            // The maximum occurs at a boundary point.
            Quaternion kR(rkZCon.m_fCosMinAngle,(Real)0,(Real)0,
                rkZCon.m_fSinMinAngle);
            Quaternion kRInv(rkZCon.m_fCosMinAngle,(Real)0,(Real)0,
                -rkZCon.m_fSinMinAngle);
            Quaternion kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(2,rkYCon);
            Real fDotOptAngle = kProd.Dot(kTmp);
            kQ = kR*kTmp;

            kR = Quaternion(rkZCon.m_fCosMaxAngle,(Real)0,(Real)0,
                rkZCon.m_fSinMaxAngle);
            kRInv = Quaternion(rkZCon.m_fCosMaxAngle,(Real)0,(Real)0,
                -rkZCon.m_fSinMaxAngle);
            kProd = kRInv*(*this);
            kTmp = kProd.GetClosest(2,rkYCon);
            Real fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kR*kTmp;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkYCon.m_fCosMinAngle,(Real)0,
                rkYCon.m_fSinMinAngle,(Real)0);
            kRInv = Quaternion(rkYCon.m_fCosMinAngle,(Real)0,
                -rkYCon.m_fSinMinAngle,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(3,rkZCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }

            kR = Quaternion(rkYCon.m_fCosMaxAngle,(Real)0,
                rkYCon.m_fSinMaxAngle,(Real)0);
            kRInv = Quaternion(rkYCon.m_fCosMaxAngle,(Real)0,
                -rkYCon.m_fSinMaxAngle,(Real)0);
            kProd = (*this)*kRInv;
            kTmp = kProd.GetClosest(3,rkZCon);
            fDotAngle = kProd.Dot(kTmp);
            if (fDotAngle > fDotOptAngle)
            {
                kQ = kTmp*kR;
                fDotOptAngle = fDotAngle;
            }
        }
    }
    else
    {
        // Infinitely many solutions, choose one that satisfies the angle
        // constraints.
        Real fMinAngle, fMaxAngle, fAngle;
        Constraints kCon;

        if (fDet < (Real)0)
        {
            fMinAngle = rkYCon.m_fMinAngle - rkZCon.m_fMaxAngle;
            fMaxAngle = rkYCon.m_fMaxAngle - rkZCon.m_fMinAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(2,kCon);

            fAngle = Math<Real>::ATan2(kTmp[2],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -=
                    (kTmp[2] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle <= rkYCon.m_fMaxAngle - rkZCon.m_fMaxAngle)
            {
                fC2 = rkZCon.m_fCosMaxAngle;
                fS2 = rkZCon.m_fSinMaxAngle;
                fAngle = rkZCon.m_fMaxAngle + fAngle;
                fC1 = Math<Real>::Cos(fAngle);
                fS1 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC1 = rkYCon.m_fCosMaxAngle;
                fS1 = rkYCon.m_fSinMaxAngle;
                fAngle = rkYCon.m_fMaxAngle - fAngle;
                fC2 = Math<Real>::Cos(fAngle);
                fS2 = Math<Real>::Sin(fAngle);
            }
        }
        else
        {
            fMinAngle = rkYCon.m_fMinAngle + rkZCon.m_fMinAngle;
            fMaxAngle = rkYCon.m_fMaxAngle + rkZCon.m_fMaxAngle;
            kCon.SetAngles(fMinAngle,fMaxAngle);

            kTmp = GetClosest(2,kCon);

            fAngle = Math<Real>::ATan2(kTmp[2],kTmp[0]);
            if (fAngle < fMinAngle || fAngle > fMaxAngle)
            {
                fAngle -=
                    (kTmp[2] >= (Real)0 ? Math<Real>::PI : -Math<Real>::PI);
                // assert(fMinAngle <= fAngle && fAngle <= fMaxAngle);
            }

            if (fAngle >= rkYCon.m_fMinAngle + rkZCon.m_fMaxAngle)
            {
                fC2 = rkZCon.m_fCosMaxAngle;
                fS2 = rkZCon.m_fSinMaxAngle;
                fAngle = fAngle - rkZCon.m_fMaxAngle;
                fC1 = Math<Real>::Cos(fAngle);
                fS1 = Math<Real>::Sin(fAngle);
            }
            else
            {
                fC1 = rkYCon.m_fCosMaxAngle;
                fS1 = rkYCon.m_fSinMaxAngle;
                fAngle = fAngle - rkYCon.m_fMaxAngle;
                fC2 = Math<Real>::Cos(fAngle);
                fS2 = Math<Real>::Sin(fAngle);
            }
        }

        kQ[0] = fC2*fC1;
        kQ[1] = -fS2*fS1;
        kQ[2] = fC2*fS1;
        kQ[3] = fS2*fC1;
        if (Dot(kQ) < (Real)0)
        {
            kQ = -kQ;
        }
    }

    return kQ;
}
//----------------------------------------------------------------------------
template <class Real>
Quaternion<Real> Quaternion<Real>::GetClosestYZ (const Constraints& rkYCon,
    const Constraints& rkZCon) const
{
    Quaternion pkAlt(m_afTuple[0],-m_afTuple[1],m_afTuple[2],m_afTuple[3]);
    Quaternion kQ = pkAlt.GetClosestZY(rkZCon,rkYCon);
    kQ[1] = -kQ[1];
    return kQ;
}
//----------------------------------------------------------------------------
