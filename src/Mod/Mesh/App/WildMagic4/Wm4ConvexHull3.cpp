// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#include "Wm4FoundationPCH.h"
#include "Wm4ConvexHull3.h"
#include "Wm4Mapper3.h"
#include "Wm4Query3Filtered.h"
#include "Wm4Query3Int64.h"
#include "Wm4Query3TInteger.h"
#include "Wm4Query3TRational.h"

namespace Wm4
{

//----------------------------------------------------------------------------
template <class Real>
ConvexHull3<Real>::ConvexHull3 (int iVertexQuantity, Vector3<Real>* akVertex,
    Real fEpsilon, bool bOwner, Query::Type eQueryType)
    :
    ConvexHull<Real>(iVertexQuantity,fEpsilon,bOwner,eQueryType),
    m_kLineOrigin(Vector3<Real>::ZERO),
    m_kLineDirection(Vector3<Real>::ZERO),
    m_kPlaneOrigin(Vector3<Real>::ZERO)
{
    assert(akVertex);
    m_akVertex = akVertex;
    m_akPlaneDirection[0] = Vector3<Real>::ZERO;
    m_akPlaneDirection[1] = Vector3<Real>::ZERO;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;

    Mapper3<Real> kMapper(m_iVertexQuantity,m_akVertex,m_fEpsilon);
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

    if (kMapper.GetDimension() == 2)
    {
        // The set is (nearly) coplanar.  The caller is responsible for
        // creating a ConvexHull2 object.
        m_iDimension = 2;
        m_kPlaneOrigin = kMapper.GetOrigin();
        m_akPlaneDirection[0] = kMapper.GetDirection(0);
        m_akPlaneDirection[1] = kMapper.GetDirection(1);
        return;
    }

    m_iDimension = 3;

    int i0 = kMapper.GetExtremeIndex(0);
    int i1 = kMapper.GetExtremeIndex(1);
    int i2 = kMapper.GetExtremeIndex(2);
    int i3 = kMapper.GetExtremeIndex(3);

    m_akSVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity];
    int i;

    if (eQueryType != Query::QT_RATIONAL && eQueryType != Query::QT_FILTERED)
    {
        // Transform the vertices to the cube [0,1]^3.
        Vector3<Real> kMin = kMapper.GetMin();
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
            m_pkQuery = WM4_NEW Query3Int64<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else if (eQueryType == Query::QT_INTEGER)
        {
            // Scale the vertices to the square [0,2^{24}]^2 to allow use of
            // TInteger.
            fExpand = (Real)(1 << 24);
            m_pkQuery = WM4_NEW Query3TInteger<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else  // eQueryType == Query::QT_REAL
        {
            // No scaling for floating point.
            fExpand = (Real)1.0;
            m_pkQuery = WM4_NEW Query3<Real>(m_iVertexQuantity,m_akSVertex);
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
        size_t uiSize = m_iVertexQuantity*sizeof(Vector3<Real>);
        System::Memcpy(m_akSVertex,uiSize,m_akVertex,uiSize);

        if (eQueryType == Query::QT_RATIONAL)
        {
            m_pkQuery = WM4_NEW Query3TRational<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else // eQueryType == Query::QT_FILTERED
        {
            m_pkQuery = WM4_NEW Query3Filtered<Real>(m_iVertexQuantity,
                m_akSVertex,m_fEpsilon);
        }
    }

    Triangle* pkT0;
    Triangle* pkT1;
    Triangle* pkT2;
    Triangle* pkT3;

    if (kMapper.GetExtremeCCW())
    {
        pkT0 = WM4_NEW Triangle(i0,i1,i3);
        pkT1 = WM4_NEW Triangle(i0,i2,i1);
        pkT2 = WM4_NEW Triangle(i0,i3,i2);
        pkT3 = WM4_NEW Triangle(i1,i2,i3);
        pkT0->AttachTo(pkT1,pkT3,pkT2);
        pkT1->AttachTo(pkT2,pkT3,pkT0);
        pkT2->AttachTo(pkT0,pkT3,pkT1);
        pkT3->AttachTo(pkT1,pkT2,pkT0);
    }
    else
    {
        pkT0 = WM4_NEW Triangle(i0,i3,i1);
        pkT1 = WM4_NEW Triangle(i0,i1,i2);
        pkT2 = WM4_NEW Triangle(i0,i2,i3);
        pkT3 = WM4_NEW Triangle(i1,i3,i2);
        pkT0->AttachTo(pkT2,pkT3,pkT1);
        pkT1->AttachTo(pkT0,pkT3,pkT2);
        pkT2->AttachTo(pkT1,pkT3,pkT0);
        pkT3->AttachTo(pkT0,pkT2,pkT1);
    }

    m_kHull.clear();
    m_kHull.insert(pkT0);
    m_kHull.insert(pkT1);
    m_kHull.insert(pkT2);
    m_kHull.insert(pkT3);

    for (i = 0; i < m_iVertexQuantity; i++)
    {
        if (!Update(i))
        {
            DeleteHull();
            return;
        }
    }

    ExtractIndices();
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull3<Real>::~ConvexHull3 ()
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
const Vector3<Real>& ConvexHull3<Real>::GetLineOrigin () const
{
    return m_kLineOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& ConvexHull3<Real>::GetLineDirection () const
{
    return m_kLineDirection;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& ConvexHull3<Real>::GetPlaneOrigin () const
{
    return m_kPlaneOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& ConvexHull3<Real>::GetPlaneDirection (int i) const
{
    assert(0 <= i && i < 2);
    return m_akPlaneDirection[i];
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull1<Real>* ConvexHull3<Real>::GetConvexHull1 () const
{
    assert(m_iDimension == 1);
    if (m_iDimension != 1)
    {
        return nullptr;
    }

    Real* afProjection = WM4_NEW Real[m_iVertexQuantity];
    for (int i = 0; i < m_iVertexQuantity; i++)
    {
        Vector3<Real> kDiff = m_akVertex[i] - m_kLineOrigin;
        afProjection[i] = m_kLineDirection.Dot(kDiff);
    }

    return WM4_NEW ConvexHull1<Real>(m_iVertexQuantity,afProjection,
        m_fEpsilon,true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull2<Real>* ConvexHull3<Real>::GetConvexHull2 () const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return nullptr;
    }

    Vector2<Real>* akProjection = WM4_NEW Vector2<Real>[m_iVertexQuantity];
    for (int i = 0; i < m_iVertexQuantity; i++)
    {
        Vector3<Real> kDiff = m_akVertex[i] - m_kPlaneOrigin;
        akProjection[i][0] = m_akPlaneDirection[0].Dot(kDiff);
        akProjection[i][1] = m_akPlaneDirection[1].Dot(kDiff);
    }

    return WM4_NEW ConvexHull2<Real>(m_iVertexQuantity,akProjection,
        m_fEpsilon,true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull3<Real>::Update (int i)
{
    // Locate a triangle visible to the input point (if possible).
    Triangle* pkVisible = nullptr;
    Triangle* pkTri;
    typename std::set<Triangle*>::iterator pkIter;
    for (pkIter = m_kHull.begin(); pkIter != m_kHull.end(); pkIter++)
    {
        pkTri = *pkIter;
        if (pkTri->GetSign(i,m_pkQuery) > 0)
        {
            pkVisible = pkTri;
            break;
        }
    }

    if (!pkVisible)
    {
        // The point is inside the current hull; nothing to do.
        return true;
    }

    // Locate and remove the visible triangles.
    std::stack<Triangle*> kVisible;
    std::map<int,TerminatorData> kTerminator;
    kVisible.push(pkVisible);
    pkVisible->OnStack = true;
    int j, iV0, iV1;
    while (!kVisible.empty())
    {
        pkTri = kVisible.top();
        kVisible.pop();
        pkTri->OnStack = false;
        for (j = 0; j < 3; j++)
        {
            Triangle* pkAdj = pkTri->A[j];
            if (pkAdj)
            {
                // Detach triangle and adjacent triangle from each other.
                int iNullIndex = pkTri->DetachFrom(j,pkAdj);

                if (pkAdj->GetSign(i,m_pkQuery) > 0)
                {
                    if (!pkAdj->OnStack)
                    {
                        // Adjacent triangle is visible.
                        kVisible.push(pkAdj);
                        pkAdj->OnStack = true;
                    }
                }
                else
                {
                    // Adjacent triangle is invisible.
                    iV0 = pkTri->V[j];
                    iV1 = pkTri->V[(j+1)%3];
                    kTerminator[iV0] = TerminatorData(iV0,iV1,iNullIndex,
                        pkAdj);
                }
            }
        }
        m_kHull.erase(pkTri);
        WM4_DELETE pkTri;
    }

    // Insert the new edges formed by the input point and the terminator
    // between visible and invisible triangles.
    int iSize = (int)kTerminator.size();
    assert(iSize >= 3);
    typename std::map<int,TerminatorData>::iterator pkEdge =
        kTerminator.begin();
    iV0 = pkEdge->second.V[0];
    iV1 = pkEdge->second.V[1];
    pkTri = WM4_NEW Triangle(i,iV0,iV1);
    m_kHull.insert(pkTri);

    // save information for linking first/last inserted new triangles
    int iSaveV0 = pkEdge->second.V[0];
    Triangle* pkSaveTri = pkTri;

    // establish adjacency links across terminator edge
    pkTri->A[1] = pkEdge->second.Tri;
    pkEdge->second.Tri->A[pkEdge->second.NullIndex] = pkTri;
    for (j = 1; j < iSize; j++)
    {
        pkEdge = kTerminator.find(iV1);
        assert(pkEdge != kTerminator.end());
        iV0 = iV1;
        iV1 = pkEdge->second.V[1];
        Triangle* pkNext = WM4_NEW Triangle(i,iV0,iV1);
        m_kHull.insert(pkNext);

        // establish adjacency links across terminator edge
        pkNext->A[1] = pkEdge->second.Tri;
        pkEdge->second.Tri->A[pkEdge->second.NullIndex] = pkNext;

        // establish adjacency links with previously inserted triangle
        pkNext->A[0] = pkTri;
        pkTri->A[2] = pkNext;

        pkTri = pkNext;
    }
    assert(iV1 == iSaveV0);
    (void)iSaveV0;  // avoid warning in Release build

    // establish adjacency links between first/last triangles
    pkSaveTri->A[0] = pkTri;
    pkTri->A[2] = pkSaveTri;

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull3<Real>::ExtractIndices ()
{
    int iTQuantity = (int)m_kHull.size();
    m_iSimplexQuantity = iTQuantity;
    m_aiIndex = WM4_NEW int[3*m_iSimplexQuantity];

    typename std::set<Triangle*>::iterator pkIter;
    int i = 0;
    for (pkIter = m_kHull.begin(); pkIter != m_kHull.end(); pkIter++)
    {
        Triangle* pkTri = *pkIter;
        for (int j = 0; j < 3; j++)
        {
            m_aiIndex[i++] = pkTri->V[j];
        }
        WM4_DELETE pkTri;
    }
    m_kHull.clear();
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull3<Real>::DeleteHull ()
{
    typename std::set<Triangle*>::iterator pkIter;
    for (pkIter = m_kHull.begin(); pkIter != m_kHull.end(); pkIter++)
    {
        Triangle* pkTri = *pkIter;
        WM4_DELETE pkTri;
    }
    m_kHull.clear();
}
//----------------------------------------------------------------------------
template <class Real>
ConvexHull3<Real>::ConvexHull3 (const char* acFilename)
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
bool ConvexHull3<Real>::Load (const char* acFilename)
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
    m_akVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity];
    m_akSVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity+4];

    size_t uiSize = sizeof(Real);
    int iVQ = 3*m_iVertexQuantity;
    if (uiSize == 4)
    {
        System::Read4le(pkIFile,iVQ,m_akVertex);
        System::Read4le(pkIFile,iVQ,m_akSVertex);
        System::Read4le(pkIFile,3,(Real*)m_kLineOrigin);
        System::Read4le(pkIFile,3,(Real*)m_kLineDirection);
        System::Read4le(pkIFile,3,(Real*)m_kPlaneOrigin);
        System::Read4le(pkIFile,3,(Real*)m_akPlaneDirection[0]);
        System::Read4le(pkIFile,3,(Real*)m_akPlaneDirection[1]);
    }
    else // iSize == 8
    {
        System::Read8le(pkIFile,iVQ,m_akVertex);
        System::Read8le(pkIFile,iVQ,m_akSVertex);
        System::Read8le(pkIFile,3,(Real*)m_kLineOrigin);
        System::Read8le(pkIFile,3,(Real*)m_kLineDirection);
        System::Read8le(pkIFile,3,(Real*)m_kPlaneOrigin);
        System::Read8le(pkIFile,3,(Real*)m_akPlaneDirection[0]);
        System::Read8le(pkIFile,3,(Real*)m_akPlaneDirection[1]);
    }

    System::Fclose(pkIFile);

    switch (m_eQueryType)
    {
    case Query::QT_INT64:
        m_pkQuery = WM4_NEW Query3Int64<Real>(m_iVertexQuantity,m_akSVertex);
        break;
    case Query::QT_INTEGER:
        m_pkQuery = WM4_NEW Query3TInteger<Real>(m_iVertexQuantity,
            m_akSVertex);
        break;
    case Query::QT_RATIONAL:
        m_pkQuery = WM4_NEW Query3TRational<Real>(m_iVertexQuantity,
            m_akSVertex);
        break;
    case Query::QT_REAL:
        m_pkQuery = WM4_NEW Query3<Real>(m_iVertexQuantity,m_akSVertex);
        break;
    case Query::QT_FILTERED:
        m_pkQuery = WM4_NEW Query3Filtered<Real>(m_iVertexQuantity,
            m_akSVertex,m_fEpsilon);
        break;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool ConvexHull3<Real>::Save (const char* acFilename) const
{
    FILE* pkOFile = System::Fopen(acFilename,"wb");
    if (!pkOFile)
    {
        return false;
    }

    ConvexHull<Real>::Save(pkOFile);

    size_t uiSize = sizeof(Real);
    int iVQ = 3*m_iVertexQuantity;
    if (uiSize == 4)
    {
        System::Write4le(pkOFile,iVQ,m_akVertex);
        System::Write4le(pkOFile,iVQ,m_akSVertex);
        System::Write4le(pkOFile,3,(const Real*)m_kLineOrigin);
        System::Write4le(pkOFile,3,(const Real*)m_kLineDirection);
        System::Write4le(pkOFile,3,(const Real*)m_kPlaneOrigin);
        System::Write4le(pkOFile,3,(const Real*)m_akPlaneDirection[0]);
        System::Write4le(pkOFile,3,(const Real*)m_akPlaneDirection[1]);
    }
    else // iSize == 8
    {
        System::Write8le(pkOFile,iVQ,m_akVertex);
        System::Write8le(pkOFile,iVQ,m_akSVertex);
        System::Write8le(pkOFile,3,(const Real*)m_kLineOrigin);
        System::Write8le(pkOFile,3,(const Real*)m_kLineDirection);
        System::Write8le(pkOFile,3,(const Real*)m_kPlaneOrigin);
        System::Write8le(pkOFile,3,(const Real*)m_akPlaneDirection[0]);
        System::Write8le(pkOFile,3,(const Real*)m_akPlaneDirection[1]);
    }

    System::Fclose(pkOFile);
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ConvexHull3::Triangle
//----------------------------------------------------------------------------
template <class Real>
ConvexHull3<Real>::Triangle::Triangle (int iV0, int iV1, int iV2)
{
    V[0] = iV0;
    V[1] = iV1;
    V[2] = iV2;
    A[0] = nullptr;
    A[1] = nullptr;
    A[2] = nullptr;
    Sign = 0;
    Time = -1;
    OnStack = false;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull3<Real>::Triangle::GetSign (int i, const Query3<Real>* pkQuery)
{
    if (i != Time)
    {
        Time = i;
        Sign = pkQuery->ToPlane(i,V[0],V[1],V[2]);
    }

    return Sign;
}
//----------------------------------------------------------------------------
template <class Real>
void ConvexHull3<Real>::Triangle::AttachTo (Triangle* pkAdj0,
    Triangle* pkAdj1, Triangle* pkAdj2)
{
    // assert:  The input adjacent triangles are correctly ordered.
    A[0] = pkAdj0;
    A[1] = pkAdj1;
    A[2] = pkAdj2;
}
//----------------------------------------------------------------------------
template <class Real>
int ConvexHull3<Real>::Triangle::DetachFrom (int iAdj, Triangle* pkAdj)
{
    assert(0 <= iAdj && iAdj < 3 && A[iAdj] == pkAdj);
    A[iAdj] = nullptr;
    for (int i = 0; i < 3; i++)
    {
        if (pkAdj->A[i] == this)
        {
            pkAdj->A[i] = nullptr;
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class ConvexHull3<float>;

template WM4_FOUNDATION_ITEM
class ConvexHull3<double>;
//----------------------------------------------------------------------------
}
