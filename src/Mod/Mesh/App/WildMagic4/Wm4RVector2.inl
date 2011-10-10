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
RVector2<ISIZE>::RVector2 ()
{
    // the vector is uninitialized
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector2<ISIZE>::RVector2 (const RVector2& rkV)
{
    m_akTuple[0] = rkV.m_akTuple[0];
    m_akTuple[1] = rkV.m_akTuple[1];
}
//----------------------------------------------------------------------------
#ifndef WM4_USING_VC70
template <int ISIZE>
RVector2<ISIZE>::RVector2 (const TRVector<2,ISIZE>& rkV)
{
    m_akTuple[0] = rkV[0];
    m_akTuple[1] = rkV[1];
}
#endif
//----------------------------------------------------------------------------
template <int ISIZE>
RVector2<ISIZE>::RVector2 (const TRational<ISIZE>& rkX,
    const TRational<ISIZE>& rkY)
{
    m_akTuple[0] = rkX;
    m_akTuple[1] = rkY;
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector2<ISIZE>& RVector2<ISIZE>::operator= (const RVector2& rkV)
{
    m_akTuple[0] = rkV.m_akTuple[0];
    m_akTuple[1] = rkV.m_akTuple[1];
    return *this;
}
//----------------------------------------------------------------------------
#ifndef WM4_USING_VC70
template <int ISIZE>
RVector2<ISIZE>& RVector2<ISIZE>::operator= (const TRVector<2,ISIZE>& rkV)
{
    m_akTuple[0] = rkV[0];
    m_akTuple[1] = rkV[1];
    return *this;
}
#endif
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector2<ISIZE>::X () const
{
    return m_akTuple[0];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE>& RVector2<ISIZE>::X ()
{
    return m_akTuple[0];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector2<ISIZE>::Y () const
{
    return m_akTuple[1];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE>& RVector2<ISIZE>::Y ()
{
    return m_akTuple[1];
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector2<ISIZE>::Dot (const RVector2& rkV) const
{
    return m_akTuple[0]*rkV.m_akTuple[0] + m_akTuple[1]*rkV.m_akTuple[1];
}
//----------------------------------------------------------------------------
template <int ISIZE>
RVector2<ISIZE> RVector2<ISIZE>::Perp () const
{
    return RVector2<ISIZE>(m_akTuple[1],-m_akTuple[0]);
}
//----------------------------------------------------------------------------
template <int ISIZE>
TRational<ISIZE> RVector2<ISIZE>::DotPerp (const RVector2& rkV) const
{
    return m_akTuple[0]*rkV.m_akTuple[1] - m_akTuple[1]*rkV.m_akTuple[0];
}
//----------------------------------------------------------------------------
} //namespace Wm4
