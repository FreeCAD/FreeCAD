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
template <class TVALUE>
TStringHashTable<TVALUE>::TStringHashTable (int iTableSize)
{
    assert(iTableSize > 0);

    m_iTableSize = iTableSize;
    m_iQuantity = 0;
    m_iIndex = 0;
    m_pkItem = 0;
    m_apkTable = WM4_NEW HashItem*[m_iTableSize];
    memset(m_apkTable,0,m_iTableSize*sizeof(HashItem*));
}
//----------------------------------------------------------------------------
template <class TVALUE>
TStringHashTable<TVALUE>::~TStringHashTable ()
{
    RemoveAll();
    WM4_DELETE[] m_apkTable;
}
//----------------------------------------------------------------------------
template <class TVALUE>
int TStringHashTable<TVALUE>::GetQuantity () const
{
    return m_iQuantity;
}
//----------------------------------------------------------------------------
template <class TVALUE>
bool TStringHashTable<TVALUE>::Insert (const std::string& rkKey,
    const TVALUE& rtValue)
{
    // find hash table entry for given key
    int iIndex = HashFunction(rkKey);
    HashItem* pkItem = m_apkTable[iIndex];

    // search for item in list associated with key
    while (pkItem)
    {
        if (rkKey == pkItem->m_kKey)
        {
            // item already in hash table
            return false;
        }
        pkItem = pkItem->m_pkNext;
    }

    // add item to beginning of list
    pkItem = WM4_NEW HashItem;
    pkItem->m_kKey = rkKey;
    pkItem->m_tValue = rtValue;
    pkItem->m_pkNext = m_apkTable[iIndex];
    m_apkTable[iIndex] = pkItem;
    m_iQuantity++;

    return true;
}
//----------------------------------------------------------------------------
template <class TVALUE>
TVALUE* TStringHashTable<TVALUE>::Find (const std::string& rkKey) const
{
    // find hash table entry for given key
    int iIndex = HashFunction(rkKey);
    HashItem* pkItem = m_apkTable[iIndex];

    // search for item in list associated with key
    while (pkItem)
    {
        if (rkKey == pkItem->m_kKey)
        {
            // item is in hash table
            return &pkItem->m_tValue;
        }
        pkItem = pkItem->m_pkNext;
    }

    return 0;
}
//----------------------------------------------------------------------------
template <class TVALUE>
bool TStringHashTable<TVALUE>::Remove (const std::string& rkKey)
{
    // find hash table entry for given key
    int iIndex = HashFunction(rkKey);
    HashItem* pkItem = m_apkTable[iIndex];

    if (!pkItem)
    {
        return false;
    }

    if (rkKey == pkItem->m_kKey)
    {
        // item is at front of list, strip it off
        HashItem* pkSave = pkItem;
        m_apkTable[iIndex] = pkItem->m_pkNext;
        WM4_DELETE pkSave;
        m_iQuantity--;
        return true;
    }

    // search for item in list
    HashItem* pkPrev = pkItem;
    HashItem* pkCurr = pkItem->m_pkNext;
    while (pkCurr && rkKey != pkCurr->m_kKey)
    {
        pkPrev = pkCurr;
        pkCurr = pkCurr->m_pkNext;
    }

    if (pkCurr)
    {
        // found the item
        pkPrev->m_pkNext = pkCurr->m_pkNext;
        WM4_DELETE pkCurr;
        m_iQuantity--;
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class TVALUE>
void TStringHashTable<TVALUE>::RemoveAll ()
{
    if (m_iQuantity > 0)
    {
        for (int iIndex = 0; iIndex < m_iTableSize; iIndex++)
        {
            while (m_apkTable[iIndex])
            {
                HashItem* pkSave = m_apkTable[iIndex];
                m_apkTable[iIndex] = m_apkTable[iIndex]->m_pkNext;
                WM4_DELETE pkSave;
                if (--m_iQuantity == 0)
                {
                    return;
                }
            }
        }
    }
}
//----------------------------------------------------------------------------
template <class TVALUE>
TVALUE* TStringHashTable<TVALUE>::GetFirst (std::string* pkKey) const
{
    if (m_iQuantity > 0)
    {
        for (m_iIndex = 0; m_iIndex < m_iTableSize; m_iIndex++)
        {
            if (m_apkTable[m_iIndex])
            {
                m_pkItem = m_apkTable[m_iIndex];
                *pkKey = m_pkItem->m_kKey;
                return &m_pkItem->m_tValue;
            }
        }
    }

    return 0;
}
//----------------------------------------------------------------------------
template <class TVALUE>
TVALUE* TStringHashTable<TVALUE>::GetNext (std::string* pkKey) const
{
    if (m_iQuantity > 0)
    {
        m_pkItem = m_pkItem->m_pkNext;
        if (m_pkItem)
        {
            *pkKey = m_pkItem->m_kKey;
            return &m_pkItem->m_tValue;
        }
        
        for (m_iIndex++; m_iIndex < m_iTableSize; m_iIndex++)
        {
            if (m_apkTable[m_iIndex])
            {
                m_pkItem = m_apkTable[m_iIndex];
                *pkKey = m_pkItem->m_kKey;
                return &m_pkItem->m_tValue;
            }
        }
    }

    return 0;
}
//----------------------------------------------------------------------------
template <class TVALUE>
int TStringHashTable<TVALUE>::HashFunction (const std::string& rkKey) const
{
    int iHiKey = 0, iLoKey = 0;
    for (int i = 0; i < (int)rkKey.length(); i++)
    {
        if (i & 1)
        {
            iHiKey += (int)rkKey[i];
        }
        else
        {
            iLoKey += (int)rkKey[i];
        }
    }
    return abs(iLoKey * iHiKey) % m_iTableSize;
}
//----------------------------------------------------------------------------
} //namespace Wm4
