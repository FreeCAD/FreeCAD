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
#include "Wm4Intersector.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real, class TVector>
Intersector<Real,TVector>::Intersector ()
{
    m_fContactTime = (Real)0.0;
    m_iIntersectionType = IT_EMPTY;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
Intersector<Real,TVector>::~Intersector ()
{
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
Real Intersector<Real,TVector>::GetContactTime () const
{
    return m_fContactTime;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
int Intersector<Real,TVector>::GetIntersectionType () const
{
    return m_iIntersectionType;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Test ()
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Find ()
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Test (Real, const TVector&, const TVector&)
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real, class TVector>
bool Intersector<Real,TVector>::Find (Real, const TVector&, const TVector&)
{
    // stub for derived class
    assert(false);
    return false;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class Intersector<float,Vector2f>;

template WM4_FOUNDATION_ITEM
class Intersector<float,Vector3f>;

template WM4_FOUNDATION_ITEM
class Intersector<double,Vector2d>;

template WM4_FOUNDATION_ITEM
class Intersector<double,Vector3d>;
//----------------------------------------------------------------------------
}
