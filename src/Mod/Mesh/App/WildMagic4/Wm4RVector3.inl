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
template <int ISIZE>
RVector3<ISIZE>::RVector3 ()
{
    // the vector is uninitialized
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector3<ISIZE>::RVector3 (const RVector3& rkV) : TRVector<3,ISIZE>()
{
    m_akTuple[0] = rkV.m_akTuple[0];
    m_akTuple[1] = rkV.m_akTuple[1];
    m_akTuple[2] = rkV.m_akTuple[2];
}
//----------------------------------------------------------------------------
#ifndef WM4_USING_VC70
template <int ISIZE>
RVector3<ISIZE>::RVector3 (const TRVector<3,ISIZE>& rkV)
{
    m_akTuple[0] = rkV[0];
    m_akTuple[1] = rkV[1];
    m_akTuple[2] = rkV[2];
}
#endif
//----------------------------------------------------------------------------
template <int ISIZE>
RVector3<ISIZE>::RVector3 (const TRational<ISIZE>& rkX,
    const TRational<ISIZE>& rkY, const TRational<ISIZE>& rkZ)
{
    m_akTuple[0] = rkX;
    m_akTuple[1] = rkY;
    m_akTuple[2] = rkZ;
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector3<ISIZE>& RVector3<ISIZE>::operator= (const RVector3& rkV)
{
    m_akTuple[0] = rkV.m_akTuple[0];
    m_akTuple[1] = rkV.m_akTuple[1];
    m_akTuple[2] = rkV.m_akTuple[2];
    return *this;
}
//----------------------------------------------------------------------------
#ifndef WM4_USING_VC70
template <int ISIZE>
RVector3<ISIZE>& RVector3<ISIZE>::operator= (const TRVector<3,ISIZE>& rkV)
{
    m_akTuple[0] = rkV[0];
    m_akTuple[1] = rkV[1];
    m_akTuple[2] = rkV[2];
    return *this;
}
#endif
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector3<ISIZE>::X () const
{
    return m_akTuple[0];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE>& RVector3<ISIZE>::X ()
{
    return m_akTuple[0];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector3<ISIZE>::Y () const
{
    return m_akTuple[1];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE>& RVector3<ISIZE>::Y ()
{
    return m_akTuple[1];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector3<ISIZE>::Z () const
{
    return m_akTuple[2];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE>& RVector3<ISIZE>::Z ()
{
    return m_akTuple[2];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector3<ISIZE>::Dot (const RVector3& rkV) const
{
    return m_akTuple[0]*rkV.m_akTuple[0] + m_akTuple[1]*rkV.m_akTuple[1] +
        m_akTuple[2]*rkV.m_akTuple[2];
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector3<ISIZE> RVector3<ISIZE>::Cross (const RVector3& rkV) const
{
    return RVector3<ISIZE>(
        m_akTuple[1]*rkV.m_akTuple[2] - m_akTuple[2]*rkV.m_akTuple[1],
        m_akTuple[2]*rkV.m_akTuple[0] - m_akTuple[0]*rkV.m_akTuple[2],
        m_akTuple[0]*rkV.m_akTuple[1] - m_akTuple[1]*rkV.m_akTuple[0]);
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector3<ISIZE>::TripleScalar (const RVector3& rkU,
    const RVector3& rkV) const
{
    return Dot(rkU.Cross(rkV));
}
//----------------------------------------------------------------------------
} //namespace Wm4
