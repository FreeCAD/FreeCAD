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
template <class T>
TSmallUnorderedSet<T>::TSmallUnorderedSet ()
{
    m_iMaxQuantity = 1;
    m_iGrowBy = 1;
    m_iQuantity = 0;
    m_atElement = WM4_NEW T[1];
}
//----------------------------------------------------------------------------
template <class T>
TSmallUnorderedSet<T>::TSmallUnorderedSet (int iMaxQuantity, int iGrowBy)
{
    assert(iMaxQuantity > 0 && iGrowBy > 0);
    if (iMaxQuantity <= 0)
    {
        iMaxQuantity = 1;
    }
    if (iGrowBy <= 0)
    {
        iGrowBy = 1;
    }

    m_iMaxQuantity = iMaxQuantity;
    m_iGrowBy = iGrowBy;
    m_iQuantity = 0;
    m_atElement = WM4_NEW T[iMaxQuantity];
}
//----------------------------------------------------------------------------
template <class T>
TSmallUnorderedSet<T>::TSmallUnorderedSet (const TSmallUnorderedSet& rkSet)
{
    m_iMaxQuantity = rkSet.m_iMaxQuantity;
    m_iGrowBy = rkSet.m_iGrowBy;
    m_iQuantity = rkSet.m_iQuantity;
    m_atElement = WM4_NEW T[m_iMaxQuantity];
    for (int i = 0; i < m_iMaxQuantity; i++)
    {
        m_atElement[i] = rkSet.m_atElement[i];
    }
}
//----------------------------------------------------------------------------
template <class T>
TSmallUnorderedSet<T>::~TSmallUnorderedSet ()
{
    WM4_DELETE[] m_atElement;
}
//----------------------------------------------------------------------------
template <class T>
TSmallUnorderedSet<T>& TSmallUnorderedSet<T>::operator= (
    const TSmallUnorderedSet& rkSet)
{
    WM4_DELETE[] m_atElement;
    m_iMaxQuantity = rkSet.m_iMaxQuantity;
    m_iGrowBy = rkSet.m_iGrowBy;
    m_iQuantity = rkSet.m_iQuantity;
    m_atElement = WM4_NEW T[m_iMaxQuantity];
    for (int i = 0; i < m_iMaxQuantity; i++)
    {
        m_atElement[i] = rkSet.m_atElement[i];
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class T>
int TSmallUnorderedSet<T>::GetMaxQuantity () const
{
    return m_iMaxQuantity;
}
//----------------------------------------------------------------------------
template <class T>
int TSmallUnorderedSet<T>::GetGrowBy () const
{
    return m_iGrowBy;
}
//----------------------------------------------------------------------------
template <class T>
int TSmallUnorderedSet<T>::GetQuantity () const
{
    return m_iQuantity;
}
//----------------------------------------------------------------------------
template <class T>
T* TSmallUnorderedSet<T>::GetElements ()
{
    return m_atElement;
}
//----------------------------------------------------------------------------
template <class T>
const T* TSmallUnorderedSet<T>::GetElements () const
{
    return m_atElement;
}
//----------------------------------------------------------------------------
template <class T>
T& TSmallUnorderedSet<T>::operator[] (int i)
{
    assert(0 <= i && i < m_iQuantity);
    if (i < 0)
    {
        i = 0;
    }
    else if (i >= m_iQuantity)
    {
        i = m_iQuantity-1;
    }

    return m_atElement[i];
}
//----------------------------------------------------------------------------
template <class T>
const T& TSmallUnorderedSet<T>::operator[] (int i) const
{
    assert(0 <= i && i < m_iQuantity);
    if (i < 0)
    {
        i = 0;
    }
    else if (i >= m_iQuantity)
    {
        i = m_iQuantity-1;
    }

    return m_atElement[i];
}
//----------------------------------------------------------------------------
template <class T>
bool TSmallUnorderedSet<T>::Insert (const T& rkElement)
{
    for (int i = 0; i < m_iQuantity; i++)
    {
        if (rkElement == m_atElement[i])
        {
            return false;
        }
    }

    if (m_iQuantity == m_iMaxQuantity)
    {
        // array is full, resize it
        int iNewMaxQuantity = m_iMaxQuantity + m_iGrowBy;
        T* atNewElement = WM4_NEW T[iNewMaxQuantity];
        for (int i = 0; i < m_iMaxQuantity; i++)
        {
            atNewElement[i] = m_atElement[i];
        }

        WM4_DELETE[] m_atElement;
        m_atElement = atNewElement;
        m_iMaxQuantity = iNewMaxQuantity;
    }

    m_atElement[m_iQuantity++] = rkElement;
    return true;
}
//----------------------------------------------------------------------------
template <class T>
void TSmallUnorderedSet<T>::InsertNoCheck (const T& rkElement)
{
    if (m_iQuantity == m_iMaxQuantity)
    {
        // array is full, resize it
        int iNewMaxQuantity = m_iMaxQuantity + m_iGrowBy;
        T* atNewElement = WM4_NEW T[iNewMaxQuantity];
        for (int i = 0; i < m_iMaxQuantity; i++)
        {
            atNewElement[i] = m_atElement[i];
        }

        WM4_DELETE[] m_atElement;
        m_atElement = atNewElement;
        m_iMaxQuantity = iNewMaxQuantity;
    }

    m_atElement[m_iQuantity++] = rkElement;
}
//----------------------------------------------------------------------------
template <class T>
bool TSmallUnorderedSet<T>::Remove (const T& rkElement)
{
    for (int i = 0; i < m_iQuantity; i++)
    {
        if (rkElement == m_atElement[i])
        {
            // element exists, shift array to fill in empty slot
            for (int j = i+1; j < m_iQuantity; j++, i++)
            {
                m_atElement[i] = m_atElement[j];
            }

            m_atElement[m_iQuantity-1] = T();
            m_iQuantity--;
            return true;
        }
    }

    return false;
}
//----------------------------------------------------------------------------
template <class T>
bool TSmallUnorderedSet<T>::Exists (const T& rkElement)
{
    for (int i = 0; i < m_iQuantity; i++)
    {
        if (rkElement == m_atElement[i])
        {
            return true;
        }
    }

    return false;
}
//----------------------------------------------------------------------------
template <class T>
void TSmallUnorderedSet<T>::Clear ()
{
    for (int i = 0; i < m_iQuantity; i++)
    {
        m_atElement[i] = T();
    }

    m_iQuantity = 0;
}
//----------------------------------------------------------------------------
template <class T>
void TSmallUnorderedSet<T>::Clear (int iMaxQuantity, int iGrowBy)
{
    assert(iMaxQuantity > 0 && iGrowBy > 0);
    if (iMaxQuantity <= 0)
    {
        iMaxQuantity = 1;
    }
    if (iGrowBy <= 0)
    {
        iGrowBy = 1;
    }

    WM4_DELETE[] m_atElement;
    m_iMaxQuantity = iMaxQuantity;
    m_iGrowBy = iGrowBy;
    m_iQuantity = 0;
    m_atElement = WM4_NEW T[iMaxQuantity];
}
//----------------------------------------------------------------------------
} //namespace Wm4
