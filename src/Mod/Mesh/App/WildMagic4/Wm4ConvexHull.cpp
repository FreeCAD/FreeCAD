// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4ConvexHull.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
ConvexHull<Real>::ConvexHull (int iVertexQuantity, Real fEpsilon, bool bOwner,
    Query::Type eQueryType)
{
    assert(iVertexQuantity > 0 && fEpsilon >= (Real)0.0);

    m_eQueryType = eQueryType;
    m_iVertexQuantity = iVertexQuantity;
    m_iDimension = 0;
    m_iSimplexQuantity = 0;
    m_aiIndex = nullptr;
    m_fEpsilon = fEpsilon;
    m_bOwner = bOwner;
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull<Real>::~ConvexHull ()
{
    WM4_DELETE[] m_aiIndex;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull<Real>::GetQueryType () const
{
    return m_eQueryType;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull<Real>::GetVertexQuantity () const
{
    return m_iVertexQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
Real ConvexHull<Real>::GetEpsilon () const
{
    return m_fEpsilon;
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull<Real>::GetOwner () const
{
    return m_bOwner;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull<Real>::GetDimension () const
{
    return m_iDimension;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull<Real>::GetSimplexQuantity () const
{
    return m_iSimplexQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const int* ConvexHull<Real>::GetIndices () const
{
    return m_aiIndex;
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull<Real>::Load (FILE* pkIFile)
{
    WM4_DELETE[] m_aiIndex;

    // fixed-size members
    int iQueryType;
    System::Read4le(pkIFile,1,&iQueryType);
    m_eQueryType = (Query::Type)iQueryType;
    System::Read4le(pkIFile,1,&m_iVertexQuantity);
    System::Read4le(pkIFile,1,&m_iDimension);
    System::Read4le(pkIFile,1,&m_iSimplexQuantity);
    System::Read4le(pkIFile,1,&m_fEpsilon);

    // variable-size members
    int iIQuantity;
    System::Read4le(pkIFile,1,&iIQuantity);
    if (1 <= m_iDimension && m_iDimension <= 3)
    {
        assert(iIQuantity == (m_iDimension+1)*m_iSimplexQuantity);
        m_aiIndex = WM4_NEW int[iIQuantity];
        System::Read4le(pkIFile,iIQuantity,m_aiIndex);
        return true;
    }

    m_aiIndex = nullptr;
    return m_iDimension == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull<Real>::Save (FILE* pkOFile) const
{
    // fixed-size members
    int iQueryType = (int)m_eQueryType;
    System::Write4le(pkOFile,1,&iQueryType);
    System::Write4le(pkOFile,1,&m_iVertexQuantity);
    System::Write4le(pkOFile,1,&m_iDimension);
    System::Write4le(pkOFile,1,&m_iSimplexQuantity);
    System::Write4le(pkOFile,1,&m_fEpsilon);

    // The member m_bOwner is not streamed because on a Load call, this
    // object will allocate the vertices and own this memory.

    // variable-size members
    int iIQuantity;
    if (1 <= m_iDimension && m_iDimension <= 3)
    {
        iIQuantity = (m_iDimension+1)*m_iSimplexQuantity;
        System::Write4le(pkOFile,1,&iIQuantity);
        System::Write4le(pkOFile,iIQuantity,m_aiIndex);
        return true;
    }

    iIQuantity = 0;
    System::Write4le(pkOFile,1,&iIQuantity);
    return m_iDimension == 0;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class ConvexHull<float>;

template WM4_FOUNDATION_ITEM
class ConvexHull<double>;
//----------------------------------------------------------------------------
}
