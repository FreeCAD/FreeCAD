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
#include "Wm4Intersector1.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Intersector1<Real>::Intersector1 (Real fU0, Real fU1, Real fV0, Real fV1)
{
    assert(fU0 <= fU1 && fV0 <= fV1);
    m_afU[0] = fU0;
    m_afU[1] = fU1;
    m_afV[0] = fV0;
    m_afV[1] = fV1;
    m_fFirstTime = (Real)0.0;
    m_fLastTime = (Real)0.0;
    m_iQuantity = 0;
}
//----------------------------------------------------------------------------
template <class Real>
Intersector1<Real>::Intersector1 (Real afU[2], Real afV[2])
{
    assert(afU[0] <= afU[1] && afV[0] <= afV[1]);
    for (int i = 0; i < 2; i++)
    {
        m_afU[i] = afU[i];
        m_afV[i] = afV[i];
    }
    m_fFirstTime = (Real)0.0;
    m_fLastTime = (Real)0.0;
    m_iQuantity = 0;
}
//----------------------------------------------------------------------------
template <class Real>
Intersector1<Real>::~Intersector1 ()
{
}
//----------------------------------------------------------------------------
template <class Real>
Real Intersector1<Real>::GetU (int i) const
{
    assert(0 <= i && i < 2);
    return m_afU[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real Intersector1<Real>::GetV (int i) const
{
    assert(0 <= i && i < 2);
    return m_afV[i];
}
//----------------------------------------------------------------------------
template <class Real>
bool Intersector1<Real>::Test ()
{
    return m_afU[0] <= m_afV[1] && m_afU[1] >= m_afV[0];
}
//----------------------------------------------------------------------------
template <class Real>
bool Intersector1<Real>::Find ()
{
    if (m_afU[1] < m_afV[0] || m_afU[0] > m_afV[1])
    {
        m_iQuantity = 0;
    }
    else if (m_afU[1] > m_afV[0])
    {
        if (m_afU[0] < m_afV[1])
        {
            m_iQuantity = 2;
            m_afOverlap[0] = (m_afU[0] < m_afV[0] ? m_afV[0] : m_afU[0]);
            m_afOverlap[1] = (m_afU[1] > m_afV[1] ? m_afV[1] : m_afU[1]);
            if (m_afOverlap[0] == m_afOverlap[1])
            {
                m_iQuantity = 1;
            }
        }
        else  // m_afU[0] == m_afV[1]
        {
            m_iQuantity = 1;
            m_afOverlap[0] = m_afU[0];
        }
    }
    else  // m_afU[1] == m_afV[0]
    {
        m_iQuantity = 1;
        m_afOverlap[0] = m_afU[1];
    }

    return m_iQuantity > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Intersector1<Real>::Test (Real fTMax, Real fSpeedU, Real fSpeedV)
{
    Real fDiffSpeed, fInvDiffSpeed, fDiffPos;

    if (m_afU[1] < m_afV[0])
    {
        // [u0,u1] initially to the left of [v0,v1]
        fDiffSpeed = fSpeedU - fSpeedV;
        if (fDiffSpeed > (Real)0.0)
        {
            // the intervals must move towards each other
            fDiffPos = m_afV[0] - m_afU[1];
            if (fDiffPos <= fTMax*fDiffSpeed)
            {
                // the intervals intersect within the specified time
                fInvDiffSpeed = ((Real)1.0)/fDiffSpeed;
                m_fFirstTime = fDiffPos*fInvDiffSpeed;
                m_fLastTime = (m_afV[1] - m_afU[0])*fInvDiffSpeed;
                return true;
            }
        }
    }
    else if (m_afU[0] > m_afV[1])
    {
        // [u0,u1] initially to the right of [v0,v1]
        fDiffSpeed = fSpeedV - fSpeedU;
        if ( fDiffSpeed > (Real)0.0 )
        {
            // the intervals must move towards each other
            fDiffPos = m_afU[0] - m_afV[1];
            if (fDiffPos <= fTMax*fDiffSpeed)
            {
                // the intervals intersect within the specified time
                fInvDiffSpeed = ((Real)1.0)/fDiffSpeed;
                m_fFirstTime = fDiffPos*fInvDiffSpeed;
                m_fLastTime = (m_afU[1] - m_afV[0])*fInvDiffSpeed;
                return true;
            }
        }
    }
    else
    {
        // the intervals are initially intersecting
        m_fFirstTime = 0.0f;
        if (fSpeedV > fSpeedU)
        {
            m_fLastTime = (m_afU[1] - m_afV[0])/(fSpeedV - fSpeedU);
        }
        else if (fSpeedV < fSpeedU)
        {
            m_fLastTime = (m_afV[1] - m_afU[0])/(fSpeedU - fSpeedV);
        }
        else
        {
            m_fLastTime = Math<Real>::MAX_REAL;
        }

        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Intersector1<Real>::Find (Real fTMax, Real fSpeedU, Real fSpeedV)
{
    Real fDiffSpeed, fInvDiffSpeed, fDiffPos;

    if (m_afU[1] < m_afV[0])
    {
        // [u0,u1] initially to the left of [v0,v1]
        fDiffSpeed = fSpeedU - fSpeedV;
        if (fDiffSpeed > (Real)0.0)
        {
            // the intervals must move towards each other
            fDiffPos = m_afV[0] - m_afU[1];
            if (fDiffPos <= fTMax*fDiffSpeed)
            {
                // the intervals intersect within the specified time
                fInvDiffSpeed = ((Real)1.0)/fDiffSpeed;
                m_fFirstTime = fDiffPos*fInvDiffSpeed;
                m_fLastTime = (m_afV[1] - m_afU[0])*fInvDiffSpeed;
                m_iQuantity = 1;
                m_afOverlap[0] = m_afU[0] + m_fFirstTime*fSpeedU;
                return true;
            }
        }
    }
    else if (m_afU[0] > m_afV[1])
    {
        // [u0,u1] initially to the right of [v0,v1]
        fDiffSpeed = fSpeedV - fSpeedU;
        if (fDiffSpeed > (Real)0.0)
        {
            // the intervals must move towards each other
            fDiffPos = m_afU[0] - m_afV[1];
            if (fDiffPos <= fTMax*fDiffSpeed)
            {
                // the intervals intersect within the specified time
                fInvDiffSpeed = ((Real)1.0)/fDiffSpeed;
                m_fFirstTime = fDiffPos*fInvDiffSpeed;
                m_fLastTime = (m_afU[1] - m_afV[0])*fInvDiffSpeed;
                m_iQuantity = 1;
                m_afOverlap[0] = m_afV[1] + m_fFirstTime*fSpeedV;
                return true;
            }
        }
    }
    else
    {
        // the intervals are initially intersecting
        m_fFirstTime = 0.0f;
        if (fSpeedV > fSpeedU)
        {
            m_fLastTime = (m_afU[1] - m_afV[0])/(fSpeedV - fSpeedU);
        }
        else if (fSpeedV < fSpeedU)
        {
            m_fLastTime = (m_afV[1] - m_afU[0])/(fSpeedU - fSpeedV);
        }
        else
        {
            m_fLastTime = Math<Real>::MAX_REAL;
        }

        if (m_afU[1] > m_afV[0])
        {
            if (m_afU[0] < m_afV[1])
            {
                m_iQuantity = 2;
                m_afOverlap[0] = (m_afU[0] < m_afV[0] ? m_afV[0] : m_afU[0]);
                m_afOverlap[1] = (m_afU[1] > m_afV[1] ? m_afV[1] : m_afU[1]);
            }
            else  // m_afU[0] == m_afV[1]
            {
                m_iQuantity = 1;
                m_afOverlap[0] = m_afU[0];
            }
        }
        else  // m_afU[1] == m_afV[0]
        {
            m_iQuantity = 1;
            m_afOverlap[0] = m_afU[1];
        }
        return true;
    }

    m_iQuantity = 0;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real Intersector1<Real>::GetFirstTime () const
{
    return m_fFirstTime;
}
//----------------------------------------------------------------------------
template <class Real>
Real Intersector1<Real>::GetLastTime () const
{
    return m_fLastTime;
}
//----------------------------------------------------------------------------
template <class Real>
int Intersector1<Real>::GetQuantity () const
{
    return m_iQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
Real Intersector1<Real>::GetOverlap (int i) const
{
    assert(0 <= i && i < m_iQuantity);
    return m_afOverlap[i];
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class Intersector1<float>;

template WM4_FOUNDATION_ITEM
class Intersector1<double>;
//----------------------------------------------------------------------------
}
