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

#include "Wm4FoundationPCH.h"
#include "Wm4Delaunay2.h"
#include "Wm4DelPolygonEdge.h"
#include "Wm4Mapper2.h"
#include "Wm4VEManifoldMesh.h"
#include "Wm4Query2Filtered.h"
#include "Wm4Query2Int64.h"
#include "Wm4Query2TInteger.h"
#include "Wm4Query2TRational.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Delaunay2<Real>::Delaunay2 (int iVertexQuantity, Vector2<Real>* akVertex,
    Real fEpsilon, bool bOwner, Query::Type eQueryType)
    :
    Delaunay<Real>(iVertexQuantity,fEpsilon,bOwner,eQueryType),
    m_kLineOrigin(Vector2<Real>::ZERO),
    m_kLineDirection(Vector2<Real>::ZERO)
{
    assert(akVertex);
    m_akVertex = akVertex;
    m_iUniqueVertexQuantity = 0;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;
    m_iPathLast = -1;
    m_aiPath = nullptr;
    m_iLastEdgeV0 = -1;
    m_iLastEdgeV1 = -1;
    m_iLastEdgeOpposite = -1;
    m_iLastEdgeOppositeIndex = -1;

    Mapper2<Real> kMapper(m_iVertexQuantity,m_akVertex,m_fEpsilon);
    if (kMapper.GetDimension() == 0)
    {
        // The values of m_iDimension, m_aiIndex, and m_aiAdjacent were
        // already initialized by the Delaunay base class.
        return;
    }

    if (kMapper.GetDimension() == 1)
    {
        // The set is (nearly) collinear.  The caller is responsible for
        // creating a Delaunay1 object.
        m_iDimension = 1;
        m_kLineOrigin = kMapper.GetOrigin();
        m_kLineDirection = kMapper.GetDirection(0);
        return;
    }

    m_iDimension = 2;

    // Allocate storage for the input vertices and the supertriangle
    // vertices.
    m_akSVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity+3];
    int i;

    if (eQueryType != Query::QT_RATIONAL && eQueryType != Query::QT_FILTERED)
    {
        // Transform the vertices to the square [0,1]^2.
        m_kMin = kMapper.GetMin();
        m_fScale = ((Real)1.0)/kMapper.GetMaxRange();
        for (i = 0; i < m_iVertexQuantity; i++)
        {
            m_akSVertex[i] = (m_akVertex[i] - m_kMin)*m_fScale;
        }

        // Construct the supertriangle to contain [0,1]^2.
        m_aiSV[0] = m_iVertexQuantity++;
        m_aiSV[1] = m_iVertexQuantity++;
        m_aiSV[2] = m_iVertexQuantity++;
        m_akSVertex[m_aiSV[0]] = Vector2<Real>((Real)-1.0,(Real)-1.0);
        m_akSVertex[m_aiSV[1]] = Vector2<Real>((Real)+4.0,(Real)-1.0);
        m_akSVertex[m_aiSV[2]] = Vector2<Real>((Real)-1.0,(Real)+4.0);

        Real fExpand;
        if (eQueryType == Query::QT_INT64)
        {
            // Scale the vertices to the square [0,2^{16}]^2 to allow use of
            // 64-bit integers for triangulation.
            fExpand = (Real)(1 << 16);
            m_pkQuery = WM4_NEW Query2Int64<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else if (eQueryType == Query::QT_INTEGER)
        {
            // Scale the vertices to the square [0,2^{20}]^2 to get more
            // precision for TInteger than for 64-bit integers for
            // triangulation.
            fExpand = (Real)(1 << 20);
            m_pkQuery = WM4_NEW Query2TInteger<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else // eQueryType == Query::QT_REAL
        {
            // No scaling for floating point.
            fExpand = (Real)1.0;
            m_pkQuery = WM4_NEW Query2<Real>(m_iVertexQuantity,m_akSVertex);
        }

        m_fScale *= fExpand;
        for (i = 0; i < m_iVertexQuantity; i++)
        {
            m_akSVertex[i] *= fExpand;
        }
    }
    else
    {
        // No transformation needed for exact rational arithmetic.
        m_kMin = Vector2<Real>::ZERO;
        m_fScale = (Real)1.0;
        size_t uiSize = m_iVertexQuantity*sizeof(Vector2<Real>);
        System::Memcpy(m_akSVertex,uiSize,m_akVertex,uiSize);

        // Construct the supertriangle to contain [min,max].
        Vector2<Real> kMin = kMapper.GetMin();
        Vector2<Real> kMax = kMapper.GetMax();
        Vector2<Real> kDelta = kMax - kMin;
        Vector2<Real> kSMin = kMin - kDelta;
        Vector2<Real> kSMax = kMax + kDelta*((Real)3.0);
        m_aiSV[0] = m_iVertexQuantity++;
        m_aiSV[1] = m_iVertexQuantity++;
        m_aiSV[2] = m_iVertexQuantity++;
        m_akSVertex[m_aiSV[0]] = kSMin;
        m_akSVertex[m_aiSV[1]] = Vector2<Real>(kSMax[0],kSMin[1]);
        m_akSVertex[m_aiSV[2]] = Vector2<Real>(kSMin[0],kSMax[1]);

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

    DelTriangle<Real>* pkTri = WM4_NEW DelTriangle<Real>(m_aiSV[0],m_aiSV[1],
        m_aiSV[2]);
    m_kTriangle.insert(pkTri);

    // Incrementally update the triangulation.  The set of processed points
    // is maintained to eliminate duplicates, either in the original input
    // points or in the points obtained by snap rounding.
    std::set<Vector2<Real> > kProcessed;
    for (i = 0; i < m_iVertexQuantity-3; i++)
    {
        if (kProcessed.find(m_akSVertex[i]) == kProcessed.end())
        {
            Update(i);
            kProcessed.insert(m_akSVertex[i]);
        }
    }
    m_iUniqueVertexQuantity = (int)kProcessed.size();

    // Remove triangles sharing a vertex of the supertriangle.
    RemoveTriangles();

    // Assign integer values to the triangles for use by the caller.
    std::map<DelTriangle<Real>*,int> kPermute;
    typename std::set<DelTriangle<Real>*>::iterator pkTIter =
        m_kTriangle.begin();
    for (i = 0; pkTIter != m_kTriangle.end(); pkTIter++)
    {
        pkTri = *pkTIter;
        kPermute[pkTri] = i++;
    }
    kPermute[nullptr] = -1;

    // Put Delaunay triangles into an array (vertices and adjacency info).
    m_iSimplexQuantity = (int)m_kTriangle.size();
    if (m_iSimplexQuantity > 0)
    {
        m_aiIndex = WM4_NEW int[3*m_iSimplexQuantity];
        m_aiAdjacent = WM4_NEW int[3*m_iSimplexQuantity];
        i = 0;
        pkTIter = m_kTriangle.begin();
        for (/**/; pkTIter != m_kTriangle.end(); pkTIter++)
        {
            pkTri = *pkTIter;
            m_aiIndex[i] = pkTri->V[0];
            m_aiAdjacent[i++] = kPermute[pkTri->A[0]];
            m_aiIndex[i] = pkTri->V[1];
            m_aiAdjacent[i++] = kPermute[pkTri->A[1]];
            m_aiIndex[i] = pkTri->V[2];
            m_aiAdjacent[i++] = kPermute[pkTri->A[2]];
        }
        assert(i == 3*m_iSimplexQuantity);

        m_iPathLast = -1;
        m_aiPath = WM4_NEW int[m_iSimplexQuantity+1];
    }

    // Restore the vertex count to the original (discards the vertices of the
    // supertriangle).
    m_iVertexQuantity -= 3;

    pkTIter = m_kTriangle.begin();
    for (/**/; pkTIter != m_kTriangle.end(); ++pkTIter)
    {
        WM4_DELETE *pkTIter;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay2<Real>::~Delaunay2 ()
{
    WM4_DELETE m_pkQuery;
    WM4_DELETE[] m_akSVertex;
    WM4_DELETE[] m_aiPath;
    if (m_bOwner)
    {
        WM4_DELETE[] m_akVertex;
    }
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>* Delaunay2<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay2<Real>::GetUniqueVertexQuantity () const
{
    return m_iUniqueVertexQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>& Delaunay2<Real>::GetLineOrigin () const
{
    return m_kLineOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector2<Real>& Delaunay2<Real>::GetLineDirection () const
{
    return m_kLineDirection;
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay1<Real>* Delaunay2<Real>::GetDelaunay1 () const
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

    return WM4_NEW Delaunay1<Real>(m_iVertexQuantity,afProjection,m_fEpsilon,
        true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::GetHull (int& riEQuantity, int*& raiIndex)
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return false;
    }

    riEQuantity = 0;
    raiIndex = nullptr;

    // Count the number of edges that are not shared by two triangles.
    int i, iAdjQuantity = 3*m_iSimplexQuantity;
    for (i = 0; i < iAdjQuantity; i++)
    {
        if (m_aiAdjacent[i] == -1)
        {
            riEQuantity++;
        }
    }
    assert(riEQuantity > 0);
    if (riEQuantity == 0)
    {
        return false;
    }

    // Enumerate the edges.
    raiIndex = WM4_NEW int[2*riEQuantity];
    int* piIndex = raiIndex;
    for (i = 0; i < iAdjQuantity; i++)
    {
        if (m_aiAdjacent[i] == -1)
        {
            int iTri = i/3, j = i%3;
            *piIndex++ = m_aiIndex[3*iTri+j];
            *piIndex++ = m_aiIndex[3*iTri+((j+1)%3)];
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay2<Real>::GetContainingTriangle (const Vector2<Real>& rkP) const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return -1;
    }

    // convert to scaled coordinates
    Vector2<Real> kXFrmP = (rkP - m_kMin)*m_fScale;

    // start at first triangle in mesh
    int iIndex = (m_iPathLast >= 0 ? m_aiPath[m_iPathLast] : 0);
    m_iPathLast = -1;
    m_iLastEdgeV0 = -1;
    m_iLastEdgeV1 = -1;
    m_iLastEdgeOpposite = -1;
    m_iLastEdgeOppositeIndex = -1;

    // use triangle edges as binary separating lines
    for (int i = 0; i < m_iSimplexQuantity; i++)
    {
        m_aiPath[++m_iPathLast] = iIndex;

        int* aiV = &m_aiIndex[3*iIndex];

        if (m_pkQuery->ToLine(kXFrmP,aiV[0],aiV[1]) > 0)
        {
            iIndex = m_aiAdjacent[3*iIndex];
            if (iIndex == -1)
            {
                m_iLastEdgeV0 = aiV[0];
                m_iLastEdgeV1 = aiV[1];
                m_iLastEdgeOpposite = aiV[2];
                m_iLastEdgeOppositeIndex = 2;
                return -1;
            }
            continue;
        }

        if (m_pkQuery->ToLine(kXFrmP,aiV[1],aiV[2]) > 0)
        {
            iIndex = m_aiAdjacent[3*iIndex+1];
            if (iIndex == -1)
            {
                m_iLastEdgeV0 = aiV[1];
                m_iLastEdgeV1 = aiV[2];
                m_iLastEdgeOpposite = aiV[0];
                m_iLastEdgeOppositeIndex = 0;
                return -1;
            }
            continue;
        }

        if (m_pkQuery->ToLine(kXFrmP,aiV[2],aiV[0]) > 0)
        {
            iIndex = m_aiAdjacent[3*iIndex+2];
            if (iIndex == -1)
            {
                m_iLastEdgeV0 = aiV[2];
                m_iLastEdgeV1 = aiV[0];
                m_iLastEdgeOpposite = aiV[1];
                m_iLastEdgeOppositeIndex = 1;
                return -1;
            }
            continue;
        }

        m_iLastEdgeV0 = -1;
        m_iLastEdgeV1 = -1;
        m_iLastEdgeOpposite = -1;
        m_iLastEdgeOppositeIndex = -1;
        return iIndex;
    }

    return -1;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay2<Real>::GetPathLast () const
{
    return m_iPathLast;
}
//----------------------------------------------------------------------------
template <class Real>
const int* Delaunay2<Real>::GetPath () const
{
    return m_aiPath;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay2<Real>::GetLastEdge (int& riV0, int& riV1, int& riV2) const
{
    riV0 = m_iLastEdgeV0;
    riV1 = m_iLastEdgeV1;
    riV2 = m_iLastEdgeOpposite;
    return m_iLastEdgeOppositeIndex;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::GetVertexSet (int i, Vector2<Real> akV[3]) const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        akV[0] = m_akVertex[m_aiIndex[3*i  ]];
        akV[1] = m_akVertex[m_aiIndex[3*i+1]];
        akV[2] = m_akVertex[m_aiIndex[3*i+2]];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::GetIndexSet (int i, int aiIndex[3]) const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiIndex[0] = m_aiIndex[3*i  ];
        aiIndex[1] = m_aiIndex[3*i+1];
        aiIndex[2] = m_aiIndex[3*i+2];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::GetAdjacentSet (int i, int aiAdjacent[3]) const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiAdjacent[0] = m_aiAdjacent[3*i  ];
        aiAdjacent[1] = m_aiAdjacent[3*i+1];
        aiAdjacent[2] = m_aiAdjacent[3*i+2];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::GetBarycentricSet (int i, const Vector2<Real>& rkP,
    Real afBary[3]) const
{
    assert(m_iDimension == 2);
    if (m_iDimension != 2)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        Vector2<Real> kV0 = m_akVertex[m_aiIndex[3*i  ]];
        Vector2<Real> kV1 = m_akVertex[m_aiIndex[3*i+1]];
        Vector2<Real> kV2 = m_akVertex[m_aiIndex[3*i+2]];
        rkP.GetBarycentrics(kV0,kV1,kV2,afBary);
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
void Delaunay2<Real>::Update (int i)
{
    // Locate the triangle containing vertex i.
    DelTriangle<Real>* pkTri = GetContainingTriangle(i);

    // Locate and remove the triangles forming the insertion polygon.
    std::stack<DelTriangle<Real>*> kStack;
    VEManifoldMesh kPolygon(nullptr,DelPolygonEdge<Real>::ECreator);
    kStack.push(pkTri);
    pkTri->OnStack = true;
    int j, iV0, iV1;
    DelPolygonEdge<Real>* pkEdge;
    while (!kStack.empty())
    {
        pkTri = kStack.top();
        kStack.pop();
        pkTri->OnStack = false;
        for (j = 0; j < 3; j++)
        {
            DelTriangle<Real>* pkAdj = pkTri->A[j];
            if (pkAdj)
            {
                // Detach triangle and adjacent triangle from each other.
                int iNullIndex = pkTri->DetachFrom(j,pkAdj);

                if (pkAdj->IsInsertionComponent(i,pkTri,m_pkQuery,m_aiSV))
                {
                    if (!pkAdj->OnStack)
                    {
                        // Adjacent triangle inside insertion polygon.
                        kStack.push(pkAdj);
                        pkAdj->OnStack = true;
                    }
                }
                else
                {
                    // Adjacent triangle outside insertion polygon.
                    iV0 = pkTri->V[j];
                    iV1 = pkTri->V[(j+1)%3];
                    pkEdge = (DelPolygonEdge<Real>*)kPolygon.InsertEdge(iV0,
                        iV1);
                    pkEdge->NullIndex = iNullIndex;
                    pkEdge->Tri = pkAdj;
                }
            }
            else
            {
                // The triangle is in the insertion polygon, but the adjacent
                // one does not exist.  This means one of two things:
                // (1) We are at an edge of the supertriangle, and that edge
                //     is part of the insertion polygon.
                // (2) We are at an edge that was recently shared by the
                //     triangle and the adjacent, but we detached those
                //     triangles from each other.  These edges should be
                //     ignored.
                iV0 = pkTri->V[j];
                if (IsSupervertex(iV0))
                {
                    iV1 = pkTri->V[(j+1)%3];
                    if (IsSupervertex(iV1))
                    {
                        pkEdge = (DelPolygonEdge<Real>*)kPolygon.InsertEdge(
                            iV0,iV1);
                        pkEdge->NullIndex = -1;
                        pkEdge->Tri = nullptr;
                    }
                }
            }
        }
        m_kTriangle.erase(pkTri);
        WM4_DELETE pkTri;
    }

    // Insert the new triangles formed by the input point and the edges of
    // the insertion polygon.
    const VEManifoldMesh::EMap& rkEMap = kPolygon.GetEdges();
    assert(rkEMap.size() >= 3 && kPolygon.IsClosed());
    typename VEManifoldMesh::EMapCIterator pkEIter;
    for (pkEIter = rkEMap.begin(); pkEIter != rkEMap.end(); pkEIter++)
    {
        pkEdge = (DelPolygonEdge<Real>*)pkEIter->second;

        // Create and insert the new triangle.
        pkTri = WM4_NEW DelTriangle<Real>(i,pkEdge->V[0],pkEdge->V[1]);
        m_kTriangle.insert(pkTri);

        // Establish the adjacency links across the polygon edge.
        pkTri->A[1] = pkEdge->Tri;
        if (pkEdge->Tri)
        {
            pkEdge->Tri->A[pkEdge->NullIndex] = pkTri;
        }

        // Update the edge's triangle pointer to point to the newly created
        // triangle.  This information is used later to establish the links
        // between the new triangles.
        pkEdge->Tri = pkTri;
    }

    // Establish the adjacency links between the new triangles.
    DelPolygonEdge<Real>* pkAdjEdge;
    for (pkEIter = rkEMap.begin(); pkEIter != rkEMap.end(); pkEIter++)
    {
        pkEdge = (DelPolygonEdge<Real>*)pkEIter->second;
        pkAdjEdge = (DelPolygonEdge<Real>*)pkEdge->E[0];
        pkEdge->Tri->A[0] = pkAdjEdge->Tri;
        pkAdjEdge = (DelPolygonEdge<Real>*)pkEdge->E[1];
        pkEdge->Tri->A[2] = pkAdjEdge->Tri;
    }
}
//----------------------------------------------------------------------------
template <class Real>
DelTriangle<Real>* Delaunay2<Real>::GetContainingTriangle (int i) const
{
    // Locate which triangle in the current mesh contains vertex i.  By
    // construction, there must be such a triangle (the vertex cannot be
    // outside the supertriangle).

    DelTriangle<Real>* pkTri = *m_kTriangle.begin();
    int iTQuantity = (int)m_kTriangle.size();
    for (int iT = 0; iT < iTQuantity; iT++)
    {
        int* aiV = pkTri->V;

        if (m_pkQuery->ToLine(i,aiV[0],aiV[1]) > 0)
        {
            pkTri = pkTri->A[0];
            if (!pkTri)
            {
                break;
            }
            continue;
        }

        if (m_pkQuery->ToLine(i,aiV[1],aiV[2]) > 0)
        {
            pkTri = pkTri->A[1];
            if (!pkTri)
            {
                break;
            }
            continue;
        }

        if (m_pkQuery->ToLine(i,aiV[2],aiV[0]) > 0)
        {
            pkTri = pkTri->A[2];
            if (!pkTri)
            {
                break;
            }
            continue;
        }

        return pkTri;
    }

    assert(false);
    return nullptr;
}
//----------------------------------------------------------------------------
template <class Real>
void Delaunay2<Real>::RemoveTriangles ()
{
    // Identify those triangles sharing a vertex of the supertriangle.
    std::set<DelTriangle<Real>*> kRemoveTri;
    DelTriangle<Real>* pkTri;
    typename std::set<DelTriangle<Real>*>::iterator pkTIter =
        m_kTriangle.begin();
    for (/**/; pkTIter != m_kTriangle.end(); pkTIter++)
    {
        pkTri = *pkTIter;
        for (int j = 0; j < 3; j++)
        {
            if (IsSupervertex(pkTri->V[j]))
            {
                kRemoveTri.insert(pkTri);
                break;
            }
        }
    }

    // Remove the triangles from the mesh.
    pkTIter = kRemoveTri.begin();
    for (/**/; pkTIter != kRemoveTri.end(); pkTIter++)
    {
        pkTri = *pkTIter;
        for (int j = 0; j < 3; j++)
        {
            // Break the links with adjacent triangles.
            DelTriangle<Real>* pkAdj = pkTri->A[j];
            if (pkAdj)
            {
                for (int k = 0; k < 3; k++)
                {
                    if (pkAdj->A[k] == pkTri)
                    {
                        pkAdj->A[k] = nullptr;
                        break;
                    }
                }
            }
        }
        m_kTriangle.erase(pkTri);
        WM4_DELETE pkTri;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::IsSupervertex (int i) const
{
    for (int j = 0; j < 3; j++)
    {
        if (i == m_aiSV[j])
        {
            return true;
        }
    }
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay2<Real>::Delaunay2 (const char* acFilename)
    :
    Delaunay<Real>(0,(Real)0.0,false,Query::QT_REAL)
{
    m_akVertex = nullptr;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;
    m_aiPath = nullptr;
    bool bLoaded = Load(acFilename);
    assert(bLoaded);
    (void)bLoaded;  // avoid warning in Release build
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay2<Real>::Load (const char* acFilename)
{
    FILE* pkIFile = System::Fopen(acFilename,"rb");
    if (!pkIFile)
    {
        return false;
    }

    Delaunay<Real>::Load(pkIFile);

    WM4_DELETE m_pkQuery;
    WM4_DELETE[] m_akSVertex;
    WM4_DELETE[] m_aiPath;
    if (m_bOwner)
    {
        WM4_DELETE[] m_akVertex;
    }

    m_bOwner = true;
    m_akVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity];
    m_akSVertex = WM4_NEW Vector2<Real>[m_iVertexQuantity+3];
    m_aiPath = WM4_NEW int[m_iSimplexQuantity+1];

    System::Read4le(pkIFile,1,&m_iUniqueVertexQuantity);
    System::Read4le(pkIFile,3,m_aiSV);
    System::Read4le(pkIFile,1,&m_iPathLast);
    System::Read4le(pkIFile,1,&m_iLastEdgeV0);
    System::Read4le(pkIFile,1,&m_iLastEdgeV1);
    System::Read4le(pkIFile,1,&m_iLastEdgeOpposite);
    System::Read4le(pkIFile,1,&m_iLastEdgeOppositeIndex);
    System::Read4le(pkIFile,m_iSimplexQuantity+1,m_aiPath);

    size_t uiSize = sizeof(Real);
    int iVQ = 2*m_iVertexQuantity, iSVQ = 2*(m_iVertexQuantity + 3);
    if (uiSize == 4)
    {
        System::Read4le(pkIFile,iVQ,m_akVertex);
        System::Read4le(pkIFile,iSVQ,m_akSVertex);
        System::Read4le(pkIFile,2,(Real*)m_kMin);
        System::Read4le(pkIFile,1,&m_fScale);
        System::Read4le(pkIFile,2,(Real*)m_kLineOrigin);
        System::Read4le(pkIFile,2,(Real*)m_kLineDirection);
    }
    else // iSize == 8
    {
        System::Read8le(pkIFile,iVQ,m_akVertex);
        System::Read8le(pkIFile,iSVQ,m_akSVertex);
        System::Read8le(pkIFile,2,(Real*)m_kMin);
        System::Read8le(pkIFile,1,&m_fScale);
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
bool Delaunay2<Real>::Save (const char* acFilename) const
{
    FILE* pkOFile = System::Fopen(acFilename,"wb");
    if (!pkOFile)
    {
        return false;
    }

    Delaunay<Real>::Save(pkOFile);

    System::Write4le(pkOFile,1,&m_iUniqueVertexQuantity);
    System::Write4le(pkOFile,3,m_aiSV);
    System::Write4le(pkOFile,1,&m_iPathLast);
    System::Write4le(pkOFile,1,&m_iLastEdgeV0);
    System::Write4le(pkOFile,1,&m_iLastEdgeV1);
    System::Write4le(pkOFile,1,&m_iLastEdgeOpposite);
    System::Write4le(pkOFile,1,&m_iLastEdgeOppositeIndex);
    System::Write4le(pkOFile,m_iSimplexQuantity+1,m_aiPath);

    size_t uiSize = sizeof(Real);
    int iVQ = 2*m_iVertexQuantity, iSVQ = 2*(m_iVertexQuantity + 3);
    if (uiSize == 4)
    {
        System::Write4le(pkOFile,iVQ,m_akVertex);
        System::Write4le(pkOFile,iSVQ,m_akSVertex);
        System::Write4le(pkOFile,2,(const Real*)m_kMin);
        System::Write4le(pkOFile,1,&m_fScale);
        System::Write4le(pkOFile,2,(const Real*)m_kLineOrigin);
        System::Write4le(pkOFile,2,(const Real*)m_kLineDirection);
    }
    else // iSize == 8
    {
        System::Write8le(pkOFile,iVQ,m_akVertex);
        System::Write8le(pkOFile,iSVQ,m_akSVertex);
        System::Write8le(pkOFile,2,(const Real*)m_kMin);
        System::Write8le(pkOFile,1,&m_fScale);
        System::Write8le(pkOFile,2,(const Real*)m_kLineOrigin);
        System::Write8le(pkOFile,2,(const Real*)m_kLineDirection);
    }

    System::Fclose(pkOFile);
    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class Delaunay2<float>;

template WM4_FOUNDATION_ITEM
class Delaunay2<double>;
//----------------------------------------------------------------------------
}
