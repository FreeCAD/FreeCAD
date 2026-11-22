// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "Wm4FoundationLIB.h"

// The class TKEY is either native data or is class data that has the
// following member functions:
//   TKEY::TKEY ()
//   TKEY& TKEY::operator= (const TKEY&)
//   bool TKEY::operator== (const TKEY&) const
//   bool TKEY::operator!= (const TKEY&) const
//   TKEY::operator unsigned int () const
// The implicit conversion to unsigned int is used to select a hash table
// index for the T object.  The return value need not be within the range of
// hash table indices.  THashTable will use modular arithmetic to make this
// happen.
//
// The class TVALUE is either native data or is class data that has the
// following member functions:
//   TVALUE::TVALUE ()
//   TVALUE& TVALUE::operator= (const TVALUE&)

#include "Wm4System.h"

namespace Wm4
{

template <class TKEY, class TVALUE>
class THashTable
{
public:
    // construction and destruction
    THashTable (int iTableSize);
    ~THashTable ();

    // element access
    int GetQuantity () const;

    // insert a key-value pair into the hash table
    bool Insert (const TKEY& rtKey, const TVALUE& rtValue);

    // search for a key and returns it value (null, if key does not exist)
    TVALUE* Find (const TKEY& rtKey) const;

    // remove key-value pairs from the hash table
    bool Remove (const TKEY& rtKey);
    void RemoveAll ();

    // linear traversal of table
    TVALUE* GetFirst (TKEY* ptKey) const;
    TVALUE* GetNext (TKEY* ptKey) const;

    // user-specified key-to-index construction
    int (*UserHashFunction)(const TKEY&);

private:
    class HashItem
    {
    public:
        TKEY m_tKey;
        TVALUE m_tValue;
        HashItem* m_pkNext;
    };

    // Default key-to-index construction (override by user-specified when
    // requested).
    int HashFunction (const TKEY& rtKey) const;

    // hash table
    int m_iTableSize;
    int m_iQuantity;
    HashItem** m_apkTable;

    // iterator for traversal
    mutable int m_iIndex;
    mutable HashItem* m_pkItem;
};

}

#include "Wm4THashTable.inl"