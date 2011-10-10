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
LinComp<Real>::LinComp ()
{
    m_iType = CT_EMPTY;
    m_fMin = Math<Real>::MAX_REAL;
    m_fMax = -Math<Real>::MAX_REAL;
}
//----------------------------------------------------------------------------
template <class Real>
LinComp<Real>::~LinComp ()
{
}
//----------------------------------------------------------------------------
template <class Real>
LinComp<Real>& LinComp<Real>::operator= (const LinComp& rkComponent)
{
    m_iType = rkComponent.m_iType;
    m_fMin = rkComponent.m_fMin;
    m_fMax = rkComponent.m_fMax;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
int LinComp<Real>::GetType () const
{
    return m_iType;
}
//----------------------------------------------------------------------------
template <class Real>
Real LinComp<Real>::GetMin () const
{
    return m_fMin;
}
//----------------------------------------------------------------------------
template <class Real>
Real LinComp<Real>::GetMax () const
{
    return m_fMax;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinComp<Real>::Contains (Real fParam) const
{
    return m_fMin <= fParam && fParam <= m_fMax;
}
//----------------------------------------------------------------------------
template <class Real>
void LinComp<Real>::SetInterval (Real fMin, Real fMax)
{
    m_iType = GetTypeFromInterval(fMin,fMax);
    m_fMin = fMin;
    m_fMax = fMax;
}
//----------------------------------------------------------------------------
template <class Real>
int LinComp<Real>::GetTypeFromInterval (Real fMin, Real fMax)
{
    if (fMin < fMax)
    {
        if (fMax == Math<Real>::MAX_REAL)
        {
            if (fMin == -Math<Real>::MAX_REAL)
            {
                return CT_LINE;
            }
            else
            {
                return CT_RAY;
            }
        }
        else
        {
            if (fMin == -Math<Real>::MAX_REAL)
            {
                return CT_RAY;
            }
            else
            {
                return CT_SEGMENT;
            }
        }
    }
    else if (fMin == fMax)
    {
        if (fMin != -Math<Real>::MAX_REAL && fMax != Math<Real>::MAX_REAL)
        {
            return CT_POINT;
        }
    }

    return CT_EMPTY;
}
//----------------------------------------------------------------------------
template <class Real>
bool LinComp<Real>::IsCanonical () const
{
    if (m_iType == CT_RAY)
    {
        return m_fMin == (Real)0.0 && m_fMax == Math<Real>::MAX_REAL;
    }

    if (m_iType == CT_SEGMENT)
    {
        return m_fMin == -m_fMax;
    }

    if (m_iType == CT_POINT)
    {
        return m_fMin == (Real)0.0; 
    }

    if (m_iType == CT_EMPTY)
    {
        return m_fMin == Math<Real>::MAX_REAL
            && m_fMax == -Math<Real>::MAX_REAL;
    }

    // m_iType == CT_LINE
    return true;
}
//----------------------------------------------------------------------------
} //namespace Wm4
