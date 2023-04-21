// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4ConvexHull2.h"
#include "Wm4Mapper2.h"
#include "Wm4Query2Filtered.h"
#include "Wm4Query2Int64.h"
#include "Wm4Query2TInteger.h"
#include "Wm4Query2TRational.h"

namespace Wm4
{

//----------------------------------------------------------------------------
template <class Real>
ConvexHull2<Real>::ConvexHull2 (int iVertexQuantity, Vector2<Real>* akVertex,
    Real fEpsilon, bool bOwner, Query::Type eQueryType)
    :
    ConvexHull<Real>(iVertexQuantity,fEpsilon,bOwner,eQueryType),
    m_kLineOrigin(Vector2<Real>::ZERO),
    m_kLineDirection(Vector2<Real>::ZERO)
{
    assert(akVertex);
    m_akVertex = akVertex;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;

    Mapper2<Real> kMapper(m_iVertexQuantity,m_akVertex,m_fEpsilon);
    if (kMapper.GetDimension() == 0)
    {
        // The values of m_iDimension, m_aiIndex, and m_aiAdjacent were
        // already initialized by the ConvexHull base class.
        return;
    }

    if (kMapper.GetDimension() == 1)
    {
        // The set is (nearly) collinear.  The caller is responsible for
        // creating a ConvexHull1 object.
        m_iDimension = 1;
        m_kLineOrigin = kMapper.GetOrigin();
        m_kLineDirection = kMapper.GetDirection(0);
        return;
    }

    m_iDimension = 2;

    int i0 = kMapper.GetExtremeIndex(0);
    int i1 = kMapper.GetExtremeIndex(1);
    int i2 = kMapper.GetExtremeIndex(2);

    m_akSVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity];
    int i;

    if (eQueryType != Query::QT_RATIONAL && eQueryType != Query::QT_FILTERED)
    {
        // Transform the vertices to the square [0,1]^2.
        Vector2<Real> kMin = kMapper.GetMin();
        Real fScale = ((Real)1.0)/kMapper.GetMaxRange();
        for (i = 0; i < m_iVertexQuantity; i++)
        {
            m_akSVertex[i] = (m_akVertex[i] - kMin)*fScale;
        }

        Real fExpand;
        if (eQueryType == Query::QT_INT64)
        {
            // Scale the vertices to the square [0,2^{20}]^2 to allow use of
            // 64-bit integers.
            fExpand = (Real)(1 << 20);
            m_pkQuery = WM4_NEW Query2Int64<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else if (eQueryType == Query::QT_INTEGER)
        {
            // Scale the vertices to the square [0,2^{24}]^2 to allow use of
            // TInteger.
            fExpand = (Real)(1 << 24);
            m_pkQuery = WM4_NEW Query2TInteger<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else  // eQueryType == Query::QT_REAL
        {
            // No scaling for floating point.
            fExpand = (Real)1.0;
            m_pkQuery = WM4_NEW Query2<Real>(m_iVertexQuantity,m_akSVertex);
        }

        for (i = 0; i < m_iVertexQuantity; i++)
        {
            m_akSVertex[i] *= fExpand;
        }
    }
    else
    {
        // No transformation needed for exact rational arithmetic or filtered
        // predicates.
        size_t uiSize = m_iVertexQuantity*sizeof(Vector2<Real>);
        System::Memcpy(m_akSVertex,uiSize,m_akVertex,uiSize);

        if (eQueryType == Query::QT_RATIONAL)
        {
            m_pkQuery = WM4_NEW Query2TRational<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else // eQueryType == Query::QT_FILTERED
        {
            m_pkQuery = WM4_NEW Query2Filtered<Real>(m_iVertexQuantity,
                m_akSVertex,m_fEpsilon);
        }
    }

    Edge* pkE0;
    Edge* pkE1;
    Edge* pkE2;

    if (kMapper.GetExtremeCCW())
    {
        pkE0 = WM4_NEW Edge(i0,i1);
        pkE1 = WM4_NEW Edge(i1,i2);
        pkE2 = WM4_NEW Edge(i2,i0);
    }
    else
    {
        pkE0 = WM4_NEW Edge(i0,i2);
        pkE1 = WM4_NEW Edge(i2,i1);
        pkE2 = WM4_NEW Edge(i1,i0);
    }

    pkE0->Insert(pkE2,pkE1);
    pkE1->Insert(pkE0,pkE2);
    pkE2->Insert(pkE1,pkE0);

    Edge* pkHull = pkE0;
    for (i = 0; i < m_iVertexQuantity; i++)
    {
        if (!Update(pkHull,i))
        {
            pkHull->DeleteAll();
            return;
        }
    }

    pkHull->GetIndices(m_iSimplexQuantity,m_aiIndex);
    pkHull->DeleteAll();
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull2<Real>::~ConvexHull2 ()
{
    if (m_bOwner)
    {
        WM4_DELETE[] m_akVertex;
    }
    WM4_DELETE[] m_akSVertex;
    WM4_DELETE m_pkQuery;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>& ConvexHull2<Real>::GetLineOrigin () const
{
    return m_kLineOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>& ConvexHull2<Real>::GetLineDirection () const
{
    return m_kLineDirection;
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull1<Real>* ConvexHull2<Real>::GetConvexHull1 () const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return nullptr;
    }

    Real* afProjection = WM4_NEW Real[m_iVertexQuantity];
    for (int i = 0; i < m_iVertexQuantity; i++)
    {
        Vector2<Real> kDiff = m_akVertex[i] - m_kLineOrigin;
        afProjection[i] = m_kLineDirection.Dot(kDiff);
    }

    return WM4_NEW ConvexHull1<Real>(m_iVertexQuantity,afProjection,
        m_fEpsilon,true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull2<Real>::Update (Edge*& rpkHull, int i)
{
    // Locate an edge visible to the input point (if possible).
    Edge* pkVisible = nullptr;
    Edge* pkCurrent = rpkHull;
    do
    {
        if (pkCurrent->GetSign(i,m_pkQuery) > 0)
        {
            pkVisible = pkCurrent;
            break;
        }

        pkCurrent = pkCurrent->A[1];
    }
    while (pkCurrent != rpkHull);

    if (!pkVisible)
    {
        // The point is inside the current hull; nothing to do.
        return true;
    }

    // Remove the visible edges.
    Edge* pkAdj0 = pkVisible->A[0];
    assert(pkAdj0);
    if (!pkAdj0)
    {
        return false;
    }

    Edge* pkAdj1 = pkVisible->A[1];
    assert(pkAdj1);
    if (!pkAdj1)
    {
        return false;
    }

    pkVisible->DeleteSelf();

    while (pkAdj0->GetSign(i,m_pkQuery) > 0)
    {
        rpkHull = pkAdj0;
        pkAdj0 = pkAdj0->A[0];
        assert(pkAdj0);
        if (!pkAdj0)
        {
            return false;
        }

        pkAdj0->A[1]->DeleteSelf();
    }

    while (pkAdj1->GetSign(i,m_pkQuery) > 0)
    {
        rpkHull = pkAdj1;
        pkAdj1 = pkAdj1->A[1];
        assert(pkAdj1);
        if (!pkAdj1)
        {
            return false;
        }

        pkAdj1->A[0]->DeleteSelf();
    }

    // Insert the new edges formed by the input point and the end points of
    // the polyline of invisible edges.
    Edge* pkEdge0 = WM4_NEW Edge(pkAdj0->V[1],i);
    Edge* pkEdge1 = WM4_NEW Edge(i,pkAdj1->V[0]);
    pkEdge0->Insert(pkAdj0,pkEdge1);
    pkEdge1->Insert(pkEdge0,pkAdj1);
    rpkHull = pkEdge0;

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull2<Real>::ConvexHull2 (const char* acFilename)
    :
    ConvexHull<Real>(0,(Real)0.0,false,Query::QT_REAL)
{
    m_akVertex = nullptr;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;
    bool bLoaded = Load(acFilename);
    assert(bLoaded);
    (void)bLoaded;  // avoid warning in Release build
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull2<Real>::Load (const char* acFilename)
{
    FILE* pkIFile = System::Fopen(acFilename,"rb");
    if (!pkIFile)
    {
        return false;
    }

    ConvexHull<Real>::Load(pkIFile);

    WM4_DELETE m_pkQuery;
    WM4_DELETE[] m_akSVertex;
    if (m_bOwner)
    {
        WM4_DELETE[] m_akVertex;
    }

    m_bOwner = true;
    m_akVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity];
    m_akSVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity];

    size_t uiSize = sizeof(Real);
    int iVQ = 2*m_iVertexQuantity;
    if (uiSize == 4)
    {
        System::Read4le(pkIFile,iVQ,m_akVertex);
        System::Read4le(pkIFile,iVQ,m_akSVertex);
        System::Read4le(pkIFile,2,(Real*)m_kLineOrigin);
        System::Read4le(pkIFile,2,(Real*)m_kLineDirection);
    }
    else // iSize == 8
    {
        System::Read8le(pkIFile,iVQ,m_akVertex);
        System::Read8le(pkIFile,iVQ,m_akSVertex);
        System::Read8le(pkIFile,2,(Real*)m_kLineOrigin);
        System::Read8le(pkIFile,2,(Real*)m_kLineDirection);
    }

    System::Fclose(pkIFile);

    switch (m_eQueryType)
    {
    case Query::QT_INT64:
        m_pkQuery = WM4_NEW Query2Int64<Real>(m_iVertexQuantity,m_akSVertex);
        break;
    case Query::QT_INTEGER:
        m_pkQuery = WM4_NEW Query2TInteger<Real>(m_iVertexQuantity,
            m_akSVertex);
        break;
    case Query::QT_RATIONAL:
        m_pkQuery = WM4_NEW Query2TRational<Real>(m_iVertexQuantity,
            m_akSVertex);
        break;
    case Query::QT_REAL:
        m_pkQuery = WM4_NEW Query2<Real>(m_iVertexQuantity,m_akSVertex);
        break;
    case Query::QT_FILTERED:
        m_pkQuery = WM4_NEW Query2Filtered<Real>(m_iVertexQuantity,
            m_akSVertex,m_fEpsilon);
        break;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull2<Real>::Save (const char* acFilename) const
{
    FILE* pkOFile = System::Fopen(acFilename,"wb");
    if (!pkOFile)
    {
        return false;
    }

    ConvexHull<Real>::Save(pkOFile);

    size_t uiSize = sizeof(Real);
    int iVQ = 2*m_iVertexQuantity;
    if (uiSize == 4)
    {
        System::Write4le(pkOFile,iVQ,m_akVertex);
        System::Write4le(pkOFile,iVQ,m_akSVertex);
        System::Write4le(pkOFile,2,(const Real*)m_kLineOrigin);
        System::Write4le(pkOFile,2,(const Real*)m_kLineDirection);
    }
    else // iSize == 8
    {
        System::Write8le(pkOFile,iVQ,m_akVertex);
        System::Write8le(pkOFile,iVQ,m_akSVertex);
        System::Write8le(pkOFile,2,(const Real*)m_kLineOrigin);
        System::Write8le(pkOFile,2,(const Real*)m_kLineDirection);
    }

    System::Fclose(pkOFile);
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ConvexHull2::Edge
//----------------------------------------------------------------------------
template <class Real>
ConvexHull2<Real>::Edge::Edge (int iV0, int iV1)
{
    V[0] = iV0;
    V[1] = iV1;
    A[0] = nullptr;
    A[1] = nullptr;
    Sign = 0;
    Time = -1;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull2<Real>::Edge::GetSign (int i, const Query2<Real>* pkQuery)
{
    if (i != Time)
    {
        Time = i;
        Sign = pkQuery->ToLine(i,V[0],V[1]);
    }

    return Sign;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull2<Real>::Edge::Insert (Edge* pkAdj0, Edge* pkAdj1)
{
    pkAdj0->A[1] = this;
    pkAdj1->A[0] = this;
    A[0] = pkAdj0;
    A[1] = pkAdj1;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull2<Real>::Edge::DeleteSelf ()
{
    if (A[0])
    {
        A[0]->A[1] = nullptr;
    }

    if (A[1])
    {
        A[1]->A[0] = nullptr;
    }

    WM4_DELETE this;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull2<Real>::Edge::DeleteAll ()
{
    Edge* pkAdj = A[1];
    while (pkAdj && pkAdj != this)
    {
        Edge* pkSave = pkAdj->A[1];
        WM4_DELETE pkAdj;
        pkAdj = pkSave;
    }

    assert(pkAdj == this);
    WM4_DELETE this;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull2<Real>::Edge::GetIndices (int& riHQuantity, int*& raiHIndex)
{
    // Count the number of edge vertices and allocate the index array.
    riHQuantity = 0;
    Edge* pkCurrent = this;
    do
    {
        riHQuantity++;
        pkCurrent = pkCurrent->A[1];
    }
    while (pkCurrent != this);
    raiHIndex = WM4_NEW int[riHQuantity];

    // Fill the index array.
    riHQuantity = 0;
    pkCurrent = this;
    do
    {
        raiHIndex[riHQuantity++] = pkCurrent->V[0];
        pkCurrent = pkCurrent->A[1];
    }
    while (pkCurrent != this);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class ConvexHull2<float>;

template WM4_FOUNDATION_ITEM
class ConvexHull2<double>;
//----------------------------------------------------------------------------
}
