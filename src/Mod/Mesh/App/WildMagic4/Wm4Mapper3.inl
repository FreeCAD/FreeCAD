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
Mapper3<Real>::Mapper3 (int iVQuantity, const Vector3<Real>* akVertex,
    Real fEpsilon)
{
    assert(iVQuantity > 0 && akVertex && fEpsilon >= (Real)0.0);
    m_bExtremeCCW = false;

    // Compute the axis-aligned bounding box for the input points.
    m_kMin = akVertex[0];
    m_kMax = m_kMin;

    int aiIMin[3], aiIMax[3], i, j;
    for (j = 0; j < 3; j++)
    {
        aiIMin[j] = 0;
        aiIMax[j] = 0;
    }

    for (i = 1; i < iVQuantity; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (akVertex[i][j] < m_kMin[j])
            {
                m_kMin[j] = akVertex[i][j];
                aiIMin[j] = i;
            }
            else if (akVertex[i][j] > m_kMax[j])
            {
                m_kMax[j] = akVertex[i][j];
                aiIMax[j] = i;
            }
        }
    }

    // Determine the maximum range for the bounding box.
    Vector3<Real> kRange = m_kMax - m_kMin;
    m_fMaxRange = kRange[0];
    m_aiExtreme[0] = aiIMin[0];
    m_aiExtreme[1] = aiIMax[0];
    if (kRange[1] > m_fMaxRange)
    {
        m_fMaxRange = kRange[1];
        m_aiExtreme[0] = aiIMin[1];
        m_aiExtreme[1] = aiIMax[1];
    }
    if (kRange[2] > m_fMaxRange)
    {
        m_fMaxRange = kRange[2];
        m_aiExtreme[0] = aiIMin[2];
        m_aiExtreme[1] = aiIMax[2];
    }
    m_kOrigin = akVertex[m_aiExtreme[0]];

    // Test if the point set is (nearly) a point.
    if (m_fMaxRange < fEpsilon)
    {
        m_iDimension = 0;
        m_aiExtreme[1] = m_aiExtreme[0];
        m_aiExtreme[2] = m_aiExtreme[0];
        m_aiExtreme[3] = m_aiExtreme[0];
        m_akDirection[0] = Vector3<Real>::ZERO;
        m_akDirection[1] = Vector3<Real>::ZERO;
        m_akDirection[2] = Vector3<Real>::ZERO;
        return;
    }

    // Test if the point set is (nearly) a line segment.
    m_akDirection[0] = akVertex[m_aiExtreme[1]] - m_kOrigin;
    m_akDirection[0].Normalize();
    Real fLMax = (Real)0.0, fL, fDot;
    m_aiExtreme[2] = m_aiExtreme[0];
    for (i = 0; i < iVQuantity; i++)
    {
        Vector3<Real> kDiff = akVertex[i] - m_kOrigin;
        fDot = m_akDirection[0].Dot(kDiff);
        Vector3<Real> kProj = kDiff - fDot*m_akDirection[0];
        fL = kProj.Length();
        if (fL > fLMax)
        {
            fLMax = fL;
            m_aiExtreme[2] = i;
        }
    }

    if (fLMax < fEpsilon*m_fMaxRange)
    {
        m_iDimension = 1;
        m_aiExtreme[2] = m_aiExtreme[1];
        m_aiExtreme[3] = m_aiExtreme[1];
        m_akDirection[1] = Vector3<Real>::ZERO;
        m_akDirection[2] = Vector3<Real>::ZERO;
        return;
    }

    // Test if the point set is (nearly) a planar polygon.
    m_akDirection[1] = akVertex[m_aiExtreme[2]] - m_kOrigin;
    fDot = m_akDirection[0].Dot(m_akDirection[1]);
    m_akDirection[1] -= fDot*m_akDirection[0];
    m_akDirection[1].Normalize();
    m_akDirection[2] = m_akDirection[0].Cross(m_akDirection[1]);
    fLMax = (Real)0.0;
    Real fMaxSign = (Real)0.0, fSign;
    m_aiExtreme[3] = m_aiExtreme[0];
    for (i = 0; i < iVQuantity; i++)
    {
        Vector3<Real> kDiff = akVertex[i] - m_kOrigin;
        fL = m_akDirection[2].Dot(kDiff);
        fSign = Math<Real>::Sign(fL);
        fL = Math<Real>::FAbs(fL);
        if (fL > fLMax)
        {
            fLMax = fL;
            fMaxSign = fSign;
            m_aiExtreme[3] = i;
        }
    }

    if (fLMax < fEpsilon*m_fMaxRange)
    {
        m_iDimension = 2;
        m_aiExtreme[3] = m_aiExtreme[2];
        return;
    }

    m_iDimension = 3;
    m_bExtremeCCW = (fMaxSign > (Real)0.0 ? true : false);
}
//----------------------------------------------------------------------------
template <class Real>
Mapper3<Real>::~Mapper3 ()
{
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Mapper3<Real>::GetMin () const
{
    return m_kMin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Mapper3<Real>::GetMax () const
{
    return m_kMax;
}
//----------------------------------------------------------------------------
template <class Real>
Real Mapper3<Real>::GetMaxRange () const
{
    return m_fMaxRange;
}
//----------------------------------------------------------------------------
template <class Real>
int Mapper3<Real>::GetDimension () const
{
    return m_iDimension;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Mapper3<Real>::GetOrigin () const
{
    return m_kOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Mapper3<Real>::GetDirection (int i) const
{
    assert(0 <= i && i < 3);
    return m_akDirection[i];
}
//----------------------------------------------------------------------------
template <class Real>
int Mapper3<Real>::GetExtremeIndex (int i) const
{
    assert(0 <= i && i < 4);
    return m_aiExtreme[i];
}
//----------------------------------------------------------------------------
template <class Real>
bool Mapper3<Real>::GetExtremeCCW () const
{
    return m_bExtremeCCW;
}
//----------------------------------------------------------------------------
} //namespace Wm4
