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
#include "Wm4Delaunay3.h"
#include "Wm4DelPolyhedronFace.h"
#include "Wm4Mapper3.h"
#include "Wm4ETManifoldMesh.h"
#include "Wm4Delaunay2.h"
#include "Wm4Query3Filtered.h"
#include "Wm4Query3Int64.h"
#include "Wm4Query3TInteger.h"
#include "Wm4Query3TRational.h"

// Indexing for the vertices of the triangle opposite a vertex.  The triangle
// opposite vertex j is
//   <gs_aaiIndex[j][0],gs_aaiIndex[j][1],gs_aaiIndex[j][2]>
// and is listed in counterclockwise order when viewed from outside the
// tetrahedron.
static const int gs_aaiIndex[4][3] = { {1,2,3}, {0,3,2}, {0,1,3}, {0,2,1} };

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Delaunay3<Real>::Delaunay3 (int iVertexQuantity, Vector3<Real>* akVertex,
    Real fEpsilon, bool bOwner, Query::Type eQueryType)
    :
    Delaunay<Real>(iVertexQuantity,fEpsilon,bOwner,eQueryType),
    m_kLineOrigin(Vector3<Real>::ZERO),
    m_kLineDirection(Vector3<Real>::ZERO),
    m_kPlaneOrigin(Vector3<Real>::ZERO)
{
    assert(akVertex);
    m_akVertex = akVertex;
    m_iUniqueVertexQuantity = 0;
    m_akPlaneDirection[0] = Vector3<Real>::ZERO;
    m_akPlaneDirection[1] = Vector3<Real>::ZERO;
    m_akSVertex = nullptr;
    m_pkQuery = nullptr;
    m_iPathLast = -1;
    m_aiPath = nullptr;
    m_iLastFaceV0 = -1;
    m_iLastFaceV1 = -1;
    m_iLastFaceV2 = -1;
    m_iLastFaceOpposite = -1;
    m_iLastFaceOppositeIndex = -1;

    Mapper3<Real> kMapper(m_iVertexQuantity,m_akVertex,m_fEpsilon);
    if (kMapper.GetDimension() == 0)
    {
        // The values of m_iDimension, m_aiIndex, and m_aiAdjacent were
        // already initialized by the Delaunay base class.
        return;
    }

    int i;
    if (kMapper.GetDimension() == 1)
    {
        // The set is (nearly) collinear.  The caller is responsible for
        // creating a Delaunay1 object.
        m_iDimension = 1;
        m_kLineOrigin = kMapper.GetOrigin();
        m_kLineDirection = kMapper.GetDirection(0);
        return;
    }

    if (kMapper.GetDimension() == 2)
    {
        // The set is (nearly) coplanar.  The caller is responsible for
        // creating a Delaunay2 object.
        m_iDimension = 2;
        m_kPlaneOrigin = kMapper.GetOrigin();
        m_akPlaneDirection[0] = kMapper.GetDirection(0);
        m_akPlaneDirection[1] = kMapper.GetDirection(1);
        return;
    }

    m_iDimension = 3;

    // Allocate storage for the input vertices and the supertetrahedron
    // vertices.
    m_akSVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity+4];

    if (eQueryType != Query::QT_RATIONAL && eQueryType != Query::QT_FILTERED)
    {
        // Transform the vertices to the cube [0,1]^3.
        m_kMin = kMapper.GetMin();
        m_fScale = ((Real)1.0)/kMapper.GetMaxRange();
        for (i = 0; i < m_iVertexQuantity; i++)
        {
            m_akSVertex[i] = (m_akVertex[i] - m_kMin)*m_fScale;
        }

        // Construct the supertetrahedron to contain [0,1]^3.
        m_aiSV[0] = m_iVertexQuantity++;
        m_aiSV[1] = m_iVertexQuantity++;
        m_aiSV[2] = m_iVertexQuantity++;
        m_aiSV[3] = m_iVertexQuantity++;
        m_akSVertex[m_aiSV[0]] = Vector3<Real>((Real)-1.0,(Real)-1.0,
            (Real)-1.0);
        m_akSVertex[m_aiSV[1]] = Vector3<Real>((Real)+6.0,(Real)-1.0,
            (Real)-1.0);
        m_akSVertex[m_aiSV[2]] = Vector3<Real>((Real)-1.0,(Real)+6.0,
            (Real)-1.0);
        m_akSVertex[m_aiSV[3]] = Vector3<Real>((Real)-1.0,(Real)-1.0,
            (Real)+6.0);

        Real fExpand;
        if (eQueryType == Query::QT_INT64)
        {
            // Scale the vertices to the cube [0,2^{10}]^3 to allow use of
            // 64-bit integers for tetrahedralization.
            fExpand = (Real)(1 << 10);
            m_pkQuery = WM4_NEW Query3Int64<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else if (eQueryType == Query::QT_INTEGER)
        {
            // Scale the vertices to the cube [0,2^{20}]^3 to get more
            // precision for TInteger than for 64-bit integers for
            // tetrahedralization.
            fExpand = (Real)(1 << 20);
            m_pkQuery = WM4_NEW Query3TInteger<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else // eQueryType == Query::QT_REAL
        {
            // No scaling for floating point.
            fExpand = (Real)1.0;
            m_pkQuery = WM4_NEW Query3<Real>(m_iVertexQuantity,m_akSVertex);
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
        m_kMin = Vector3<Real>::ZERO;
        m_fScale = (Real)1.0;
        size_t uiSize = m_iVertexQuantity*sizeof(Vector3<Real>);
        System::Memcpy(m_akSVertex,uiSize,m_akVertex,uiSize);

        // Construct the supertriangle to contain [min,max].
        Vector3<Real> kMin = kMapper.GetMin();
        Vector3<Real> kMax = kMapper.GetMax();
        Vector3<Real> kDelta = kMax - kMin;
        Vector3<Real> kSMin = kMin - kDelta;
        Vector3<Real> kSMax = kMax + ((Real)5.0)*kDelta;
        m_aiSV[0] = m_iVertexQuantity++;
        m_aiSV[1] = m_iVertexQuantity++;
        m_aiSV[2] = m_iVertexQuantity++;
        m_aiSV[3] = m_iVertexQuantity++;
        m_akSVertex[m_aiSV[0]] = kSMin;
        m_akSVertex[m_aiSV[1]] = Vector3<Real>(kSMax[0],kSMin[1],kSMin[2]);
        m_akSVertex[m_aiSV[2]] = Vector3<Real>(kSMin[0],kSMax[1],kSMin[2]);
        m_akSVertex[m_aiSV[3]] = Vector3<Real>(kSMin[0],kSMin[1],kSMax[2]);

        if (eQueryType == Query::QT_RATIONAL)
        {
            m_pkQuery = WM4_NEW Query3TRational<Real>(m_iVertexQuantity,
                m_akSVertex);
        }
        else // eQueryType == Query::QT_FILTERED
        {
            m_pkQuery = WM4_NEW Query3Filtered<Real>(m_iVertexQuantity,
                m_akSVertex,fEpsilon);
        }
    }

    DelTetrahedron<Real>* pkTetra = WM4_NEW DelTetrahedron<Real>(m_aiSV[0],
        m_aiSV[1],m_aiSV[2],m_aiSV[3]);
    m_kTetrahedron.insert(pkTetra);

    // Incrementally update the tetrahedralization.  The set of processed
    // points is maintained to eliminate duplicates, either in the original
    // input points or in the points obtained by snap rounding.
    std::set<Vector3<Real> > kProcessed;
    for (i = 0; i < m_iVertexQuantity-4; i++)
    {
        if (kProcessed.find(m_akSVertex[i]) == kProcessed.end())
        {
            Update(i);
            kProcessed.insert(m_akSVertex[i]);
        }
    }
    m_iUniqueVertexQuantity = (int)kProcessed.size();

    // Remove tetrahedra sharing a vertex of the supertetrahedron.
    RemoveTetrahedra();

    // Assign integer values to the tetrahedra for use by the caller.
    std::map<DelTetrahedron<Real>*,int> kPermute;
    typename std::set<DelTetrahedron<Real>*>::iterator pkTIter =
        m_kTetrahedron.begin();
    for (i = 0; pkTIter != m_kTetrahedron.end(); pkTIter++)
    {
        pkTetra = *pkTIter;
        kPermute[pkTetra] = i++;
    }
    kPermute[nullptr] = -1;

    // Put Delaunay tetrahedra into an array (vertices and adjacency info).
    m_iSimplexQuantity = (int)m_kTetrahedron.size();
    if (m_iSimplexQuantity > 0)
    {
        m_aiIndex = WM4_NEW int[4*m_iSimplexQuantity];
        m_aiAdjacent = WM4_NEW int[4*m_iSimplexQuantity];
        i = 0;
        pkTIter = m_kTetrahedron.begin();
        for (/**/; pkTIter != m_kTetrahedron.end(); pkTIter++)
        {
            pkTetra = *pkTIter;
            m_aiIndex[i] = pkTetra->V[0];
            m_aiAdjacent[i++] = kPermute[pkTetra->A[0]];
            m_aiIndex[i] = pkTetra->V[1];
            m_aiAdjacent[i++] = kPermute[pkTetra->A[1]];
            m_aiIndex[i] = pkTetra->V[2];
            m_aiAdjacent[i++] = kPermute[pkTetra->A[2]];
            m_aiIndex[i] = pkTetra->V[3];
            m_aiAdjacent[i++] = kPermute[pkTetra->A[3]];
        }
        assert(i == 4*m_iSimplexQuantity);

        m_iPathLast = -1;
        m_aiPath = WM4_NEW int[m_iSimplexQuantity+1];
    }

    // Restore the vertex count to the original (discards the vertices of the
    // supertetrahedron).
    m_iVertexQuantity -= 4;

    pkTIter = m_kTetrahedron.begin();
    for (/**/; pkTIter != m_kTetrahedron.end(); ++pkTIter)
    {
        WM4_DELETE *pkTIter;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay3<Real>::~Delaunay3 ()
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
const Vector3<Real>* Delaunay3<Real>::GetVertices () const
{
    return m_akVertex;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay3<Real>::GetUniqueVertexQuantity () const
{
    return m_iUniqueVertexQuantity;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Delaunay3<Real>::GetLineOrigin () const
{
    return m_kLineOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Delaunay3<Real>::GetLineDirection () const
{
    return m_kLineDirection;
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay1<Real>* Delaunay3<Real>::GetDelaunay1 () const
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

    return WM4_NEW Delaunay1<Real>(m_iVertexQuantity,afProjection,m_fEpsilon,
        true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Delaunay3<Real>::GetPlaneOrigin () const
{
    return m_kPlaneOrigin;
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& Delaunay3<Real>::GetPlaneDirection (int i) const
{
    assert(0 <= i && i < 2);
    return m_akPlaneDirection[i];
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay2<Real>* Delaunay3<Real>::GetDelaunay2 () const
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

    return WM4_NEW Delaunay2<Real>(m_iVertexQuantity,akProjection,m_fEpsilon,
        true,m_eQueryType);
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::GetHull (int& riTQuantity, int*& raiIndex) const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return false;
    }

    riTQuantity = 0;
    raiIndex = nullptr;

    // Count the number of triangles that are not shared by two tetrahedra.
    int i, iAdjQuantity = 4*m_iSimplexQuantity;
    for (i = 0; i < iAdjQuantity; i++)
    {
        if (m_aiAdjacent[i] == -1)
        {
            riTQuantity++;
        }
    }
    assert(riTQuantity > 0);
    if (riTQuantity == 0)
    {
        return false;
    }

    // Enumerate the triangles.
    raiIndex = WM4_NEW int[3*riTQuantity];
    int* piIndex = raiIndex;
    for (i = 0; i < iAdjQuantity; i++)
    {
        if (m_aiAdjacent[i] == -1)
        {
            int iTetra = i/4, iFace = i%4;
            for (int j = 0; j < 4; j++)
            {
                if (j != iFace)
                {
                    *piIndex++ = m_aiIndex[4*iTetra+j];
                }
            }
            if ((iFace % 2) == 0)
            {
                int iSave = *(piIndex-1);
                *(piIndex-1) = *(piIndex-2);
                *(piIndex-2) = iSave;
            }
        }
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay3<Real>::GetContainingTetrahedron (const Vector3<Real>& rkP)
    const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return -1;
    }

    // convert to scaled coordinates
    Vector3<Real> kXFrmP = (rkP - m_kMin)*m_fScale;

    // start at first tetrahedron in mesh
    int iIndex = (m_iPathLast >= 0 ? m_aiPath[m_iPathLast] : 0);
    m_iPathLast = -1;
    m_iLastFaceV0 = -1;
    m_iLastFaceV1 = -1;
    m_iLastFaceV2 = -1;
    m_iLastFaceOpposite = -1;
    m_iLastFaceOppositeIndex = -1;

    // use tetrahedron faces as binary separating planes
    for (int i = 0; i < m_iSimplexQuantity; i++)
    {
        m_aiPath[++m_iPathLast] = iIndex;

        int* aiV = &m_aiIndex[4*iIndex];

        // <V1,V2,V3> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(kXFrmP,aiV[1],aiV[2],aiV[3]) > 0)
        {
            iIndex = m_aiAdjacent[4*iIndex];
            if (iIndex == -1)
            {
                m_iLastFaceV0 = aiV[1];
                m_iLastFaceV1 = aiV[2];
                m_iLastFaceV2 = aiV[3];
                m_iLastFaceOpposite = aiV[0];
                m_iLastFaceOppositeIndex = 0;
                return -1;
            }
            continue;
        }

        // <V0,V3,V2> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(kXFrmP,aiV[0],aiV[2],aiV[3]) < 0)
        {
            iIndex = m_aiAdjacent[4*iIndex+1];
            if (iIndex == -1)
            {
                m_iLastFaceV0 = aiV[0];
                m_iLastFaceV1 = aiV[2];
                m_iLastFaceV2 = aiV[3];
                m_iLastFaceOpposite = aiV[1];
                m_iLastFaceOppositeIndex = 1;
                return -1;
            }
            continue;
        }

        // <V0,V1,V3> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(kXFrmP,aiV[0],aiV[1],aiV[3]) > 0)
        {
            iIndex = m_aiAdjacent[4*iIndex+2];
            if (iIndex == -1)
            {
                m_iLastFaceV0 = aiV[0];
                m_iLastFaceV1 = aiV[1];
                m_iLastFaceV2 = aiV[3];
                m_iLastFaceOpposite = aiV[2];
                m_iLastFaceOppositeIndex = 2;
                return -1;
            }
            continue;
        }

        // <V0,V2,V1> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(kXFrmP,aiV[0],aiV[1],aiV[2]) < 0)
        {
            iIndex = m_aiAdjacent[4*iIndex+3];
            if (iIndex == -1)
            {
                m_iLastFaceV0 = aiV[0];
                m_iLastFaceV1 = aiV[1];
                m_iLastFaceV2 = aiV[2];
                m_iLastFaceOpposite = aiV[3];
                m_iLastFaceOppositeIndex = 3;
                return -1;
            }
            continue;
        }

        m_iLastFaceV0 = -1;
        m_iLastFaceV1 = -1;
        m_iLastFaceV2 = -1;
        m_iLastFaceOppositeIndex = -1;
        return iIndex;
    }

    return -1;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay3<Real>::GetPathLast () const
{
    return m_iPathLast;
}
//----------------------------------------------------------------------------
template <class Real>
const int* Delaunay3<Real>::GetPath () const
{
    return m_aiPath;
}
//----------------------------------------------------------------------------
template <class Real>
int Delaunay3<Real>::GetLastFace (int& riV0, int& riV1, int& riV2,
    int& riV3) const
{
    riV0 = m_iLastFaceV0;
    riV1 = m_iLastFaceV1;
    riV2 = m_iLastFaceV2;
    riV3 = m_iLastFaceOpposite;
    return m_iLastFaceOppositeIndex;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::GetVertexSet (int i, Vector3<Real> akV[4]) const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        akV[0] = m_akVertex[m_aiIndex[4*i  ]];
        akV[1] = m_akVertex[m_aiIndex[4*i+1]];
        akV[2] = m_akVertex[m_aiIndex[4*i+2]];
        akV[3] = m_akVertex[m_aiIndex[4*i+3]];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::GetIndexSet (int i, int aiIndex[4]) const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiIndex[0] = m_aiIndex[4*i  ];
        aiIndex[1] = m_aiIndex[4*i+1];
        aiIndex[2] = m_aiIndex[4*i+2];
        aiIndex[3] = m_aiIndex[4*i+3];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::GetAdjacentSet (int i, int aiAdjacent[4]) const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        aiAdjacent[0] = m_aiAdjacent[4*i  ];
        aiAdjacent[1] = m_aiAdjacent[4*i+1];
        aiAdjacent[2] = m_aiAdjacent[4*i+2];
        aiAdjacent[3] = m_aiAdjacent[4*i+3];
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::GetBarycentricSet (int i, const Vector3<Real>& rkP,
    Real afBary[4]) const
{
    assert(m_iDimension == 3);
    if (m_iDimension != 3)
    {
        return false;
    }

    if (0 <= i && i < m_iSimplexQuantity)
    {
        Vector3<Real> kV0 = m_akVertex[m_aiIndex[4*i  ]];
        Vector3<Real> kV1 = m_akVertex[m_aiIndex[4*i+1]];
        Vector3<Real> kV2 = m_akVertex[m_aiIndex[4*i+2]];
        Vector3<Real> kV3 = m_akVertex[m_aiIndex[4*i+3]];
        rkP.GetBarycentrics(kV0,kV1,kV2,kV3,afBary);
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------
template <class Real>
void Delaunay3<Real>::Update (int i)
{
    // Locate the tetrahedron containing vertex i.
    DelTetrahedron<Real>* pkTetra = GetContainingTetrahedron(i);

    // Locate and remove the tetrahedra forming the insertion polyhedron.
    std::stack<DelTetrahedron<Real>*> kStack;
    ETManifoldMesh kPolyhedron(nullptr,DelPolyhedronFace<Real>::TCreator);
    kStack.push(pkTetra);
    pkTetra->OnStack = true;
    int j, iV0, iV1, iV2;
    DelPolyhedronFace<Real>* pkFace;
    while (!kStack.empty())
    {
        pkTetra = kStack.top();
        kStack.pop();
        pkTetra->OnStack = false;
        for (j = 0; j < 4; j++)
        {
            DelTetrahedron<Real>* pkAdj = pkTetra->A[j];
            if (pkAdj)
            {
                // Detach tetrahedron and adjacent tetrahedron from each
                // other.
                int iNullIndex = pkTetra->DetachFrom(j,pkAdj);

                if (pkAdj->IsInsertionComponent(i,pkTetra,m_pkQuery,m_aiSV))
                {
                    if (!pkAdj->OnStack)
                    {
                        // Adjacent triangle inside insertion polyhedron.
                        kStack.push(pkAdj);
                        pkAdj->OnStack = true;
                    }
                }
                else
                {
                    // Adjacent tetrahedron outside insertion polyhedron.
                    iV0 = pkTetra->V[gs_aaiIndex[j][0]];
                    iV1 = pkTetra->V[gs_aaiIndex[j][1]];
                    iV2 = pkTetra->V[gs_aaiIndex[j][2]];
                    pkFace = (DelPolyhedronFace<Real>*)
                        kPolyhedron.InsertTriangle(iV0,iV1,iV2);
                    pkFace->NullIndex = iNullIndex;
                    pkFace->Tetra = pkAdj;
                }
            }
            else
            {
                // The tetrahedron is in the insertion polyhedron, but the
                // adjacent one does not exist.  This means one of two things:
                // (1) We are at a face of the supertetrahedron, and that
                //     face is part of the insertion polyhedron.
                // (2) We are at a face that was recently shared by the
                //     tetrahedron and the adjacent, but we detached those
                //     tetrahedra from each other.  These faces should be
                //     ignored.

                iV0 = pkTetra->V[gs_aaiIndex[j][0]];
                if (IsSupervertex(iV0))
                {
                    iV1 = pkTetra->V[gs_aaiIndex[j][1]];
                    if (IsSupervertex(iV1))
                    {
                        iV2 = pkTetra->V[gs_aaiIndex[j][2]];
                        if (IsSupervertex(iV2))
                        {
                            pkFace = (DelPolyhedronFace<Real>*)
                                kPolyhedron.InsertTriangle(iV0,iV1,iV2);
                            pkFace->NullIndex = -1;
                            pkFace->Tetra = nullptr;
                        }
                    }
                }
            }
        }
        m_kTetrahedron.erase(pkTetra);
        WM4_DELETE pkTetra;
    }

    // Insert the new tetrahedra formed by the input point and the faces of
    // the insertion polyhedron.
    const ETManifoldMesh::TMap& rkTMap = kPolyhedron.GetTriangles();
    assert(rkTMap.size() >= 4 && kPolyhedron.IsClosed());
    typename ETManifoldMesh::TMapCIterator pkTIter;
    for (pkTIter = rkTMap.begin(); pkTIter != rkTMap.end(); pkTIter++)
    {
        pkFace = (DelPolyhedronFace<Real>*)pkTIter->second;

        // Create and insert the new tetrahedron.
        pkTetra = WM4_NEW DelTetrahedron<Real>(i,pkFace->V[0],pkFace->V[1],
            pkFace->V[2]);
        m_kTetrahedron.insert(pkTetra);

        // Establish the adjacency links across the polyhedron face.
        pkTetra->A[0] = pkFace->Tetra;
        if (pkFace->Tetra)
        {
            pkFace->Tetra->A[pkFace->NullIndex] = pkTetra;
        }

        // Update the faces's tetrahedron pointer to point to the newly
        // created tetrahedron.  This information is used later to establish
        // the links between the new tetrahedra.
        pkFace->Tetra = pkTetra;
    }

    // Establish the adjacency links between the new tetrahedra.
    DelPolyhedronFace<Real>* pkAdjFace;
    for (pkTIter = rkTMap.begin(); pkTIter != rkTMap.end(); pkTIter++)
    {
        pkFace = (DelPolyhedronFace<Real>*)pkTIter->second;

        pkAdjFace = (DelPolyhedronFace<Real>*)pkFace->T[0];
        pkFace->Tetra->A[3] = pkAdjFace->Tetra;
        assert(SharesFace(3,pkFace->Tetra,pkAdjFace->Tetra));

        pkAdjFace = (DelPolyhedronFace<Real>*)pkFace->T[1];
        pkFace->Tetra->A[1] = pkAdjFace->Tetra;
        assert(SharesFace(1,pkFace->Tetra,pkAdjFace->Tetra));

        pkAdjFace = (DelPolyhedronFace<Real>*)pkFace->T[2];
        pkFace->Tetra->A[2] = pkAdjFace->Tetra;
        assert(SharesFace(2,pkFace->Tetra,pkAdjFace->Tetra));
    }
}
//----------------------------------------------------------------------------
template <class Real>
DelTetrahedron<Real>* Delaunay3<Real>::GetContainingTetrahedron (int i) const
{
    // Locate which tetrahedron in the current mesh contains vertex i.  By
    // construction, there must be such a tetrahedron (the vertex cannot be
    // outside the supertetrahedron).

    DelTetrahedron<Real>* pkTetra = *m_kTetrahedron.begin();
    int iTQuantity = (int)m_kTetrahedron.size();
    for (int iT = 0; iT < iTQuantity; iT++)
    {
        int* aiV = pkTetra->V;

        // <V1,V2,V3> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(i,aiV[1],aiV[2],aiV[3]) > 0)
        {
            pkTetra = pkTetra->A[0];
            if (!pkTetra)
            {
                break;
            }
            continue;
        }

        // <V0,V3,V2> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(i,aiV[0],aiV[2],aiV[3]) < 0)
        {
            pkTetra = pkTetra->A[1];
            if (!pkTetra)
            {
                break;
            }
            continue;
        }

        // <V0,V1,V3> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(i,aiV[0],aiV[1],aiV[3]) > 0)
        {
            pkTetra = pkTetra->A[2];
            if (!pkTetra)
            {
                break;
            }
            continue;
        }

        // <V0,V2,V1> counterclockwise when viewed outside tetrahedron
        if (m_pkQuery->ToPlane(i,aiV[0],aiV[1],aiV[2]) < 0)
        {
            pkTetra = pkTetra->A[3];
            if (!pkTetra)
            {
                break;
            }
            continue;
        }

        return pkTetra;
    }

    assert(false);
    return nullptr;
}
//----------------------------------------------------------------------------
template <class Real>
void Delaunay3<Real>::RemoveTetrahedra ()
{
    // Identify those triangles sharing a vertex of the supertetrahedron.
    std::set<DelTetrahedron<Real>*> kRemoveTetra;
    DelTetrahedron<Real>* pkTetra;
    typename std::set<DelTetrahedron<Real>*>::iterator pkTIter =
        m_kTetrahedron.begin();
    for (/**/; pkTIter != m_kTetrahedron.end(); pkTIter++)
    {
        pkTetra = *pkTIter;
        for (int j = 0; j < 4; j++)
        {
            if (IsSupervertex(pkTetra->V[j]))
            {
                kRemoveTetra.insert(pkTetra);
                break;
            }
        }
    }

    // Remove the tetrahedra from the mesh.
    pkTIter = kRemoveTetra.begin();
    for (/**/; pkTIter != kRemoveTetra.end(); pkTIter++)
    {
        pkTetra = *pkTIter;
        for (int j = 0; j < 4; j++)
        {
            // Break the links with adjacent tetrahedra.
            DelTetrahedron<Real>* pkAdj = pkTetra->A[j];
            if (pkAdj)
            {
                for (int k = 0; k < 4; k++)
                {
                    if (pkAdj->A[k] == pkTetra)
                    {
                        pkAdj->A[k] = nullptr;
                        break;
                    }
                }
            }
        }
        m_kTetrahedron.erase(pkTetra);
        WM4_DELETE pkTetra;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool Delaunay3<Real>::IsSupervertex (int i) const
{
    for (int j = 0; j < 4; j++)
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
bool Delaunay3<Real>::SharesFace (int i, DelTetrahedron<Real>* pkFace,
    DelTetrahedron<Real>* pkAdj)
{
    int aiF[3], iCount = 0, j;
    for (j = 0; j < 4; j++)
    {
        if (j != i)
        {
            aiF[iCount++] = pkFace->V[j];
        }
    }

    for (i = 0; i < 4; i++)
    {
        if (pkAdj->V[i] != aiF[0] &&
            pkAdj->V[i] != aiF[1] &&
            pkAdj->V[i] != aiF[2])
        {
            break;
        }
    }
    if (i == 4)
    {
        return false;
    }

    int aiA[3];
    for (j = 0, iCount = 0; j < 4; j++)
    {
        if (j != i)
        {
            aiA[iCount++] = pkAdj->V[j];
        }
    }

    if (aiF[0] > aiF[1])
    {
        j = aiF[0];
        aiF[0] = aiF[1];
        aiF[1] = j;
    }
    if (aiF[1] > aiF[2])
    {
        j = aiF[1];
        aiF[1] = aiF[2];
        aiF[2] = j;
    }
    if (aiF[0] > aiF[1])
    {
        j = aiF[0];
        aiF[0] = aiF[1];
        aiF[1] = j;
    }

    if (aiA[0] > aiA[1])
    {
        j = aiA[0];
        aiA[0] = aiA[1];
        aiA[1] = j;
    }
    if (aiA[1] > aiA[2])
    {
        j = aiA[1];
        aiA[1] = aiA[2];
        aiA[2] = j;
    }
    if (aiA[0] > aiA[1])
    {
        j = aiA[0];
        aiA[0] = aiA[1];
        aiA[1] = j;
    }

    if (aiA[0] != aiF[0] || aiA[1] != aiF[1] || aiA[2] != aiF[2])
    {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Delaunay3<Real>::Delaunay3 (const char* acFilename)
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
bool Delaunay3<Real>::Load (const char* acFilename)
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
    m_akVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity];
    m_akSVertex = WM4_NEW Vector3<Real>[m_iVertexQuantity+4];
    m_aiPath = WM4_NEW int[m_iSimplexQuantity+1];

    System::Read4le(pkIFile,1,&m_iUniqueVertexQuantity);
    System::Read4le(pkIFile,4,m_aiSV);
    System::Read4le(pkIFile,1,&m_iPathLast);
    System::Read4le(pkIFile,1,&m_iLastFaceV0);
    System::Read4le(pkIFile,1,&m_iLastFaceV1);
    System::Read4le(pkIFile,1,&m_iLastFaceV2);
    System::Read4le(pkIFile,1,&m_iLastFaceOpposite);
    System::Read4le(pkIFile,1,&m_iLastFaceOppositeIndex);
    System::Read4le(pkIFile,m_iSimplexQuantity+1,m_aiPath);

    size_t uiSize = sizeof(Real);
    int iVQ = 3*m_iVertexQuantity, iSVQ = 3*(m_iVertexQuantity + 4);
    if (uiSize == 4)
    {
        System::Read4le(pkIFile,iVQ,m_akVertex);
        System::Read4le(pkIFile,iSVQ,m_akSVertex);
        System::Read4le(pkIFile,3,(Real*)m_kMin);
        System::Read4le(pkIFile,1,&m_fScale);
        System::Read4le(pkIFile,3,(Real*)m_kLineOrigin);
        System::Read4le(pkIFile,3,(Real*)m_kLineDirection);
        System::Read4le(pkIFile,3,(Real*)m_kPlaneOrigin);
        System::Read4le(pkIFile,3,(Real*)m_akPlaneDirection[0]);
        System::Read4le(pkIFile,3,(Real*)m_akPlaneDirection[1]);
    }
    else // iSize == 8
    {
        System::Read8le(pkIFile,iVQ,m_akVertex);
        System::Read8le(pkIFile,iSVQ,m_akSVertex);
        System::Read8le(pkIFile,3,(Real*)m_kMin);
        System::Read8le(pkIFile,1,&m_fScale);
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
bool Delaunay3<Real>::Save (const char* acFilename) const
{
    FILE* pkOFile = System::Fopen(acFilename,"wb");
    if (!pkOFile)
    {
        return false;
    }

    Delaunay<Real>::Save(pkOFile);

    System::Write4le(pkOFile,1,&m_iUniqueVertexQuantity);
    System::Write4le(pkOFile,4,m_aiSV);
    System::Write4le(pkOFile,1,&m_iPathLast);
    System::Write4le(pkOFile,1,&m_iLastFaceV0);
    System::Write4le(pkOFile,1,&m_iLastFaceV1);
    System::Write4le(pkOFile,1,&m_iLastFaceV2);
    System::Write4le(pkOFile,1,&m_iLastFaceOpposite);
    System::Write4le(pkOFile,1,&m_iLastFaceOppositeIndex);
    System::Write4le(pkOFile,m_iSimplexQuantity+1,m_aiPath);

    size_t uiSize = sizeof(Real);
    int iVQ = 3*m_iVertexQuantity, iSVQ = 3*(m_iVertexQuantity + 4);
    if (uiSize == 4)
    {
        System::Write4le(pkOFile,iVQ,m_akVertex);
        System::Write4le(pkOFile,iSVQ,m_akSVertex);
        System::Write4le(pkOFile,3,(const Real*)m_kMin);
        System::Write4le(pkOFile,1,&m_fScale);
        System::Write4le(pkOFile,3,(const Real*)m_kLineOrigin);
        System::Write4le(pkOFile,3,(const Real*)m_kLineDirection);
        System::Write4le(pkOFile,3,(const Real*)m_kPlaneOrigin);
        System::Write4le(pkOFile,3,(const Real*)m_akPlaneDirection[0]);
        System::Write4le(pkOFile,3,(const Real*)m_akPlaneDirection[1]);
    }
    else // iSize == 8
    {
        System::Write8le(pkOFile,iVQ,m_akVertex);
        System::Write8le(pkOFile,iSVQ,m_akSVertex);
        System::Write8le(pkOFile,3,(const Real*)m_kMin);
        System::Write8le(pkOFile,1,&m_fScale);
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
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class Delaunay3<float>;

template WM4_FOUNDATION_ITEM
class Delaunay3<double>;
//----------------------------------------------------------------------------
}
