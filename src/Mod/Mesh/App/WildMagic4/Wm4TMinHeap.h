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
// Version: 4.0.2 (2006/10/15)

#pragma once

#include "Wm4System.h"

namespace Wm4
{

template <typename Generator, typename Real> class TMinHeap;

template <typename Generator, typename Real>
class TMinHeapRecord
{
public:
    TMinHeapRecord ();
    ~TMinHeapRecord ();

    Generator GetGenerator () const;
    Real GetValue () const;

private:
    friend class TMinHeap<Generator,Real>;

    Generator m_tGenerator;
    Real m_fValue;
    int m_iIndex;
};

template <typename Generator, typename Real>
class TMinHeap
{
public:
    TMinHeap (int iMaxQuantity, int iGrowBy);
    ~TMinHeap ();

    // Member access.
    int GetMaxQuantity () const;
    int GetGrowBy () const;
    int GetQuantity () const;
    const TMinHeapRecord<Generator,Real>* GetRecord (int i) const;

    // Insert into the heap the number fValue that corresponds to the object
    // identified by iGenerator.  The return value is a pointer to the heap
    // record storing the information.
    const TMinHeapRecord<Generator,Real>* Insert (Generator tGenerator,
        Real fValue);

    // Remove the root of the heap.  The root contains the minimum value of
    // all heap elements.  The root information is returned by the function's
    // output parameters.
    void Remove (Generator& rtGenerator, Real& rfValue);

    // The value of a heap record must be modified through this function call.
    // The side effect is that the heap must be updated accordingly to
    // accommodate the new value.
    void Update (const TMinHeapRecord<Generator,Real>* pkConstRecord,
        Real fValue);

    // Support for debugging.  The first two functions check if the array of
    // records really do form a heap.  The last function prints the heap
    // to a file.
    bool IsValid (int iStart, int iFinal);
    bool IsValid ();
    void Print (const char* acFilename);

private:
    // The actual record storage, allocated in one large chunk.
    int m_iMaxQuantity, m_iGrowBy, m_iQuantity;
    TMinHeapRecord<Generator,Real>* m_akRecords;

    // Pointers to the records in storage.  The two-level system avoids the
    // large number of allocations and deallocations that would occur if each
    // element of m_apkRecord were to be allocated/deallocated individually.
    TMinHeapRecord<Generator,Real>** m_apkRecords;
};

}

#include "Wm4TMinHeap.inl"