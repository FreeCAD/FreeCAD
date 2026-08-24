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

// The class TVALUE is either native data or is class data that has the
// following member functions:
//   TVALUE::TVALUE ()
//   TVALUE& TVALUE::operator= (const TVALUE&)

#include "Wm4System.h"

namespace Wm4
{

template <class TVALUE>
class TStringHashTable
{
public:
    // construction and destruction
    TStringHashTable (int iTableSize);
    ~TStringHashTable ();

    // element access
    int GetQuantity () const;

    // insert a key-value pair into the hash table
    bool Insert (const std::string& rkKey, const TVALUE& rtValue);

    // search for a key and returns it value (null, if key does not exist)
    TVALUE* Find (const std::string& rkKey) const;

    // remove key-value pairs from the hash table
    bool Remove (const std::string& rkKey);
    void RemoveAll ();

    // linear traversal of table
    TVALUE* GetFirst (std::string* pkKey) const;
    TVALUE* GetNext (std::string* pkKey) const;

private:
    class HashItem
    {
    public:
        HashItem () : m_kKey("") { /**/ }

        std::string m_kKey;
        TVALUE m_tValue;
        HashItem* m_pkNext;
    };

    // key-to-index construction
    int HashFunction (const std::string& rkKey) const;

    // hash table
    int m_iTableSize;
    int m_iQuantity;
    HashItem** m_apkTable;

    // iterator for traversal
    mutable int m_iIndex;
    mutable HashItem* m_pkItem;
};

}

#include "Wm4TStringHashTable.inl"