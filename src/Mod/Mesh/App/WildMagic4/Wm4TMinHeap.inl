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

namespace Wm4
{
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
TMinHeapRecord<Generator,Real>::TMinHeapRecord ()
{
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
TMinHeapRecord<Generator,Real>::~TMinHeapRecord ()
{
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
Generator TMinHeapRecord<Generator,Real>::GetGenerator () const
{
    return m_tGenerator;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
Real TMinHeapRecord<Generator,Real>::GetValue () const
{
    return m_fValue;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
template <typename Generator, typename Real>
TMinHeap<Generator,Real>::TMinHeap (int iMaxQuantity, int iGrowBy)
{
    assert(iMaxQuantity > 0 && iGrowBy > 0);
    m_iMaxQuantity = iMaxQuantity;
    m_iGrowBy = iGrowBy;
    m_iQuantity = 0;
    m_akRecords = WM4_NEW TMinHeapRecord<Generator,Real>[m_iMaxQuantity];
    m_apkRecords = WM4_NEW TMinHeapRecord<Generator,Real>*[m_iMaxQuantity];
    for (int i = 0; i < m_iMaxQuantity; i++)
    {
        m_apkRecords[i] = &m_akRecords[i];
        m_apkRecords[i]->m_iIndex = i;
    }
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
TMinHeap<Generator,Real>::~TMinHeap ()
{
    WM4_DELETE[] m_akRecords;
    WM4_DELETE[] m_apkRecords;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
int TMinHeap<Generator,Real>::GetMaxQuantity () const
{
    return m_iMaxQuantity;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
int TMinHeap<Generator,Real>::GetGrowBy () const
{
    return m_iGrowBy;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
int TMinHeap<Generator,Real>::GetQuantity () const
{
    return m_iQuantity;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
const TMinHeapRecord<Generator,Real>* TMinHeap<Generator,Real>::GetRecord (
    int i) const
{
    if (0 <= i && i < m_iQuantity)
    {
        return m_apkRecords[i];
    }
    return 0;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
const TMinHeapRecord<Generator,Real>* TMinHeap<Generator,Real>::Insert (
    Generator tGenerator, Real fValue)
{
    // Grow the heap record array, if necessary.
    if (m_iQuantity == m_iMaxQuantity)
    {
        int iNewQuantity = m_iMaxQuantity + m_iGrowBy;

        TMinHeapRecord<Generator,Real>* akNewRecords =
            WM4_NEW TMinHeapRecord<Generator,Real>[iNewQuantity];

        TMinHeapRecord<Generator,Real>** apkNewRecords =
            WM4_NEW TMinHeapRecord<Generator,Real>*[iNewQuantity];

        // Copy the old records to the new storage.
        size_t uiSize = m_iMaxQuantity*sizeof(TMinHeapRecord<Generator,Real>);
        memcpy(akNewRecords,m_akRecords,uiSize);

        // Update the pointers to the old records.
        int i;
        for (i = 0; i < m_iMaxQuantity; i++)
        {
            int iByteOffset = (int)(m_apkRecords[i] - m_akRecords);
            apkNewRecords[i] = (TMinHeapRecord<Generator,Real>*)(
                ((char*)akNewRecords) + iByteOffset);
            apkNewRecords[i]->m_iIndex = i;
        }

        // Create the pointers for the new records.
        for (i = m_iMaxQuantity; i < iNewQuantity; i++)
        {
            apkNewRecords[i] = &akNewRecords[i];
            apkNewRecords[i]->m_iIndex = i;
        }

        WM4_DELETE[] m_akRecords;
        WM4_DELETE[] m_apkRecords;
        m_iMaxQuantity = iNewQuantity;
        m_akRecords = akNewRecords;
        m_apkRecords = apkNewRecords;
    }

    // Store the input information in the last heap record, which is the last
    // leaf in the tree.
    int iChild = m_iQuantity++;
    TMinHeapRecord<Generator,Real>* pkRecord = m_apkRecords[iChild];
    pkRecord->m_tGenerator = tGenerator;
    pkRecord->m_fValue = fValue;

    // Propagate the information toward the root of the tree until it reaches
    // its correct position, thus restoring the tree to a valid heap.
    while (iChild > 0)
    {
        int iParent = (iChild - 1)/2;
        if (m_apkRecords[iParent]->m_fValue <= fValue)
        {
            // The parent has a value smaller than or equal to the child's
            // value, so we now have a valid heap.
            break;
        }

        // The parent has a larger value than the child's value.  Swap the
        // parent and child:

        // Move the parent into the child's slot.
        m_apkRecords[iChild] = m_apkRecords[iParent];
        m_apkRecords[iChild]->m_iIndex = iChild;

        // Move the child into the parent's slot.
        m_apkRecords[iParent] = pkRecord;
        m_apkRecords[iParent]->m_iIndex = iParent;

        iChild = iParent;
    }

    return m_apkRecords[iChild];
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
void TMinHeap<Generator,Real>::Remove (Generator& rtGenerator, Real& rfValue)
{
    // Get the information from the root of the heap.
    TMinHeapRecord<Generator,Real>* pkRoot = m_apkRecords[0];
    rtGenerator = pkRoot->m_tGenerator;
    rfValue = pkRoot->m_fValue;

    // Restore the tree to a heap.  Abstractly, pkRecord is the new root of
    // the heap.  It is moved down the tree via parent-child swaps until it
    // is in a location that restores the tree to a heap.
    int iLast = --m_iQuantity;
    TMinHeapRecord<Generator,Real>* pkRecord = m_apkRecords[iLast];
    int iParent = 0, iChild = 1;
    while (iChild <= iLast)
    {
        if (iChild < iLast)
        {
            // Select the child with smallest value to be the one that is
            // swapped with the parent, if necessary.
            int iChildP1 = iChild + 1;
            if (m_apkRecords[iChild]->m_fValue >
                m_apkRecords[iChildP1]->m_fValue)
            {
                iChild = iChildP1;
            }
        }

        if (m_apkRecords[iChild]->m_fValue >= pkRecord->m_fValue)
        {
            // The tree is now a heap.
            break;
        }

        // Move the child into the parent's slot.
        m_apkRecords[iParent] = m_apkRecords[iChild];
        m_apkRecords[iParent]->m_iIndex = iParent;

        iParent = iChild;
        iChild = 2*iChild + 1;
    }

    // The previous 'last' record was moved to the root and propagated down
    // the tree to its final resting place, restoring the tree to a heap.
    // The slot m_apkRecords[iParent] is that resting place.
    m_apkRecords[iParent] = pkRecord;
    m_apkRecords[iParent]->m_iIndex = iParent;

    // The old root record must not be lost.  Attach it to the slot that
    // contained the old last record.
    m_apkRecords[iLast] = pkRoot;
    m_apkRecords[iLast]->m_iIndex = iLast;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
void TMinHeap<Generator,Real>::Update (
    const TMinHeapRecord<Generator,Real>* pkConstRecord, Real fValue)
{
    // The input is 'const' to let the caller know that only TMinHeap may
    // update the record.  This is essentially a form of mutability.
    TMinHeapRecord<Generator,Real>* pkRecord =
        (TMinHeapRecord<Generator,Real>*)pkConstRecord;

    int iParent, iChild, iChildP1, iMaxChild;

    if (fValue > pkRecord->m_fValue)
    {
        pkRecord->m_fValue = fValue;

        // The new value is larger than the old value.  Propagate it towards
        // the leaves.
        iParent = pkRecord->m_iIndex;
        iChild = 2*iParent + 1;
        while (iChild < m_iQuantity)
        {
            // At least one child exists.  Locate the one of maximum value.
            if (iChild < m_iQuantity-1)
            {
                // Two children exist.
                iChildP1 = iChild + 1;
                if (m_apkRecords[iChild]->m_fValue <=
                    m_apkRecords[iChildP1]->m_fValue)
                {
                    iMaxChild = iChild;
                }
                else
                {
                    iMaxChild = iChildP1;
                }
            }
            else
            {
                // One child exists.
                iMaxChild = iChild;
            }

            if (m_apkRecords[iMaxChild]->m_fValue >= fValue)
            {
                // The new value is in the correct place to restore the tree
                // to a heap.
                break;
            }

            // The child has a larger value than the parent's value.  Swap
            // the parent and child:

            // Move the child into the parent's slot.
            m_apkRecords[iParent] = m_apkRecords[iMaxChild];
            m_apkRecords[iParent]->m_iIndex = iParent;

            // Move the parent into the child's slot.
            m_apkRecords[iMaxChild] = pkRecord;
            m_apkRecords[iMaxChild]->m_iIndex = iMaxChild;

            iParent = iMaxChild;
            iChild = 2*iParent + 1;
        }
    }
    else if (fValue < pkRecord->m_fValue)
    {
        pkRecord->m_fValue = fValue;

        // The new weight is smaller than the old weight.  Propagate it
        // towards the root.
        iChild = pkRecord->m_iIndex;
        while (iChild > 0)
        {
            // A parent exists.
            iParent = (iChild - 1)/2;

            if (m_apkRecords[iParent]->m_fValue <= fValue)
            {
                // The new value is in the correct place to restore the tree
                // to a heap.
                break;
            }

            // The parent has a smaller value than the child's value.  Swap
            // the child and parent:

            // Move the parent into the child's slot.
            m_apkRecords[iChild] = m_apkRecords[iParent];
            m_apkRecords[iChild]->m_iIndex = iChild;

            // Move the child into the parent's slot.
            m_apkRecords[iParent] = pkRecord;
            m_apkRecords[iParent]->m_iIndex = iParent;

            iChild = iParent;
        }
    }
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
bool TMinHeap<Generator,Real>::IsValid (int iStart, int iFinal)
{
    for (int iChild = iStart; iChild <= iFinal; iChild++)
    {
        int iParent = (iChild - 1)/2;
        if (iParent > iStart)
        {
            if (m_apkRecords[iParent]->m_fValue >
                m_apkRecords[iChild]->m_fValue)
            {
                return false;
            }

            if (m_apkRecords[iParent]->m_iIndex != iParent)
            {
                return false;
            }
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
bool TMinHeap<Generator,Real>::IsValid ()
{
    return IsValid(0,m_iQuantity-1);
}
//----------------------------------------------------------------------------
template <typename Generator, typename Real>
void TMinHeap<Generator,Real>::Print (const char* acFilename)
{
    std::ofstream kOStr(acFilename);
    for (int i = 0; i < m_iQuantity; i++)
    {
        TMinHeapRecord<Generator,Real>* pkRecord = m_apkRecords[i];
        kOStr << pkRecord->m_iIndex << ": gen = " << pkRecord->m_tGenerator
              << " , val = " << pkRecord->m_fValue << std::endl;
    }
    kOStr.close();
}
//----------------------------------------------------------------------------
} //namespace Wm4
