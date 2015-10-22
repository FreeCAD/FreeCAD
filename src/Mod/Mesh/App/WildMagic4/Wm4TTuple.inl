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
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>::TTuple ()
{
    // Uninitialized for native data.  Initialized for class data as long as
    // TYPE's default constructor initializes its own data.
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>::~TTuple ()
{
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>::TTuple (const TTuple& rkT)
{
    for (int i = 0; i < DIMENSION; i++)
    {
        m_atTuple[i] = rkT.m_atTuple[i];
    }
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>::operator const TYPE* () const
{
    return m_atTuple;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>::operator TYPE* ()
{
    return m_atTuple;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TYPE TTuple<DIMENSION,TYPE>::operator[] (int i) const
{
    assert(0 <= i && i < DIMENSION);
    return m_atTuple[i];
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TYPE& TTuple<DIMENSION,TYPE>::operator[] (int i)
{
    assert(0 <= i && i < DIMENSION);
    return m_atTuple[i];
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
TTuple<DIMENSION,TYPE>& TTuple<DIMENSION,TYPE>::operator= (const TTuple& rkT)
{
    for (int i = 0; i < DIMENSION; i++)
    {
        m_atTuple[i] = rkT.m_atTuple[i];
    }
    return *this;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator== (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) == 0;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator!= (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) != 0;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator< (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) < 0;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator<= (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) <= 0;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator> (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) > 0;
}
//----------------------------------------------------------------------------
template <int DIMENSION, class TYPE>
bool TTuple<DIMENSION,TYPE>::operator>= (const TTuple& rkT) const
{
    const size_t uiSize = DIMENSION*sizeof(TYPE);
    return memcmp(m_atTuple,rkT.m_atTuple,uiSize) >= 0;
}
//----------------------------------------------------------------------------
} //namespace Wm4
