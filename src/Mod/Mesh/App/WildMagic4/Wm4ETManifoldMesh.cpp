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
#include "Wm4ETManifoldMesh.h"
using namespace Wm4;

//----------------------------------------------------------------------------
ETManifoldMesh::ETManifoldMesh (ECreator oECreator, TCreator oTCreator)
{
    m_oECreator = (oECreator ? oECreator : CreateEdge);
    m_oTCreator = (oTCreator ? oTCreator : CreateTriangle);
}
//----------------------------------------------------------------------------
ETManifoldMesh::~ETManifoldMesh ()
{
    EMap::iterator pkEIter;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        WM4_DELETE pkEIter->second;
    }

    TMap::iterator pkTIter;
    for (pkTIter = m_kTMap.begin(); pkTIter != m_kTMap.end(); pkTIter++)
    {
        WM4_DELETE pkTIter->second;
    }
}
//----------------------------------------------------------------------------
ETManifoldMesh::EPtr ETManifoldMesh::CreateEdge (int iV0, int iV1)
{
    return WM4_NEW Edge(iV0,iV1);
}
//----------------------------------------------------------------------------
ETManifoldMesh::TPtr ETManifoldMesh::CreateTriangle (int iV0, int iV1,
    int iV2)
{
    return WM4_NEW Triangle(iV0,iV1,iV2);
}
//----------------------------------------------------------------------------
ETManifoldMesh::TPtr ETManifoldMesh::InsertTriangle (int iV0, int iV1,
    int iV2)
{
    TriangleKey kTKey(iV0,iV1,iV2);
    TMapIterator pkTIter = m_kTMap.find(kTKey);
    if (pkTIter != m_kTMap.end())
    {
        // triangle already exists
        return nullptr;
    }

    // add new triangle
    TPtr pkTriangle = m_oTCreator(iV0,iV1,iV2);
    m_kTMap[kTKey] = pkTriangle;

    // add edges to mesh
    for (int i0 = 2, i1 = 0; i1 < 3; i0 = i1++)
    {
        EdgeKey kEKey(pkTriangle->V[i0],pkTriangle->V[i1]);
        EPtr pkEdge;
        EMapIterator pkEIter = m_kEMap.find(kEKey);
        if (pkEIter == m_kEMap.end())
        {
            // first time edge encountered
            pkEdge = m_oECreator(pkTriangle->V[i0],pkTriangle->V[i1]);
            m_kEMap[kEKey] = pkEdge;

            // update edge and triangle
            pkEdge->T[0] = pkTriangle;
            pkTriangle->E[i0] = pkEdge;
        }
        else
        {
            // second time edge encountered
            pkEdge = pkEIter->second;
            assert(pkEdge);

            // update edge
            if (pkEdge->T[1])
            {
                assert(false);  // mesh must be manifold
                return nullptr;
            }
            pkEdge->T[1] = pkTriangle;

            // update adjacent triangles
            TPtr pkAdjacent = pkEdge->T[0];
            assert(pkAdjacent);
            for (int i = 0; i < 3; i++)
            {
                if (pkAdjacent->E[i] == pkEdge)
                {
                    pkAdjacent->T[i] = pkTriangle;
                    break;
                }
            }

            // update triangle
            pkTriangle->E[i0] = pkEdge;
            pkTriangle->T[i0] = pkAdjacent;
        }
    }

    return pkTriangle;
}
//----------------------------------------------------------------------------
bool ETManifoldMesh::RemoveTriangle (int iV0, int iV1, int iV2)
{
    TriangleKey kTKey(iV0,iV1,iV2);
    TMapIterator pkTIter = m_kTMap.find(kTKey);
    if (pkTIter == m_kTMap.end())
    {
        // triangle does not exist
        return false;
    }

    TPtr pkTriangle = pkTIter->second;
    for (int i = 0; i < 3; i++)
    {
        // inform edges you are going away
        Edge* pkEdge = pkTriangle->E[i];
        assert(pkEdge);
        if (pkEdge->T[0] == pkTriangle)
        {
            // one-triangle edges always have pointer in slot zero
            pkEdge->T[0] = pkEdge->T[1];
            pkEdge->T[1] = nullptr;
        }
        else if (pkEdge->T[1] == pkTriangle)
        {
            pkEdge->T[1] = nullptr;
        }
        else
        {
            assert(false);
            return false;
        }

        // remove edge if you had the last reference to it
        if (!pkEdge->T[0] && !pkEdge->T[1])
        {
            EdgeKey kEKey(pkEdge->V[0],pkEdge->V[1]);
            m_kEMap.erase(kEKey);
            WM4_DELETE pkEdge;
        }

        // inform adjacent triangles you are going away
        TPtr pkAdjacent = pkTriangle->T[i];
        if (pkAdjacent)
        {
            for (int j = 0; j < 3; j++)
            {
                if (pkAdjacent->T[j] == pkTriangle)
                {
                    pkAdjacent->T[j] = nullptr;
                    break;
                }
            }
        }
    }

    m_kTMap.erase(kTKey);
    WM4_DELETE pkTriangle;
    return true;
}
//----------------------------------------------------------------------------
bool ETManifoldMesh::IsClosed () const
{
    EMapCIterator pkEIter;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        const Edge* pkEdge = pkEIter->second;
        if (!pkEdge->T[0] || !pkEdge->T[1])
        {
            return false;
        }
    }
    return true;
}
//----------------------------------------------------------------------------
void ETManifoldMesh::Print (const char* acFilename)
{
    std::ofstream kOStr(acFilename);
    if (!kOStr)
    {
        return;
    }

    // assign unique indices to the edges
    std::map<EPtr,int> kEIndex;
    kEIndex[nullptr] = 0;
    int i = 1;
    EMapIterator pkEIter;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        if (pkEIter->second)
        {
            kEIndex[pkEIter->second] = i++;
        }
    }

    // assign unique indices to the triangles
    std::map<TPtr,int> kTIndex;
    kTIndex[nullptr] = 0;
    i = 1;
    TMapIterator pkTIter;
    for (pkTIter = m_kTMap.begin(); pkTIter != m_kTMap.end(); pkTIter++)
    {
        if (pkTIter->second)
        {
            kTIndex[pkTIter->second] = i++;
        }
    }

    // print edges
    kOStr << "edge quantity = " << (int)m_kEMap.size() << std::endl;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        const Edge& rkEdge = *pkEIter->second;
        kOStr << 'e' << kEIndex[pkEIter->second] << " <"
              << 'v' << rkEdge.V[0] << ",v" << rkEdge.V[1] << "; ";
        if (rkEdge.T[0])
        {
            kOStr << 't' << kTIndex[rkEdge.T[0]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkEdge.T[1])
        {
            kOStr << 't' << kTIndex[rkEdge.T[1]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << '>' << std::endl;
    }
    kOStr << std::endl;

    // print triangles
    kOStr << "triangle quantity = " << (int)m_kTMap.size() << std::endl;
    for (pkTIter = m_kTMap.begin(); pkTIter != m_kTMap.end(); pkTIter++)
    {
        const Triangle& rkTriangle = *pkTIter->second;
        kOStr << 't' << kTIndex[pkTIter->second] << " <"
              << 'v' << rkTriangle.V[0] << ",v" << rkTriangle.V[1] << ",v"
              << rkTriangle.V[2] << "; ";
        if (rkTriangle.E[0])
        {
            kOStr << 'e' << kEIndex[rkTriangle.E[0]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkTriangle.E[1])
        {
            kOStr << 'e' << kEIndex[rkTriangle.E[1]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkTriangle.E[2])
        {
            kOStr << 'e' << kEIndex[rkTriangle.E[2]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << "; ";

        if (rkTriangle.T[0])
        {
            kOStr << 't' << kTIndex[rkTriangle.T[0]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkTriangle.T[1])
        {
            kOStr << 't' << kTIndex[rkTriangle.T[1]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkTriangle.T[2])
        {
            kOStr << 't' << kTIndex[rkTriangle.T[2]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << '>' << std::endl;
    }
    kOStr << std::endl;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ETManifoldMesh::Edge
//----------------------------------------------------------------------------
ETManifoldMesh::Edge::Edge (int iV0, int iV1)
{
    V[0] = iV0;
    V[1] = iV1;
    T[0] = nullptr;
    T[1] = nullptr;
}
//----------------------------------------------------------------------------
ETManifoldMesh::Edge::~Edge ()
{
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ETManifoldMesh::Triangle
//----------------------------------------------------------------------------
ETManifoldMesh::Triangle::Triangle (int iV0, int iV1, int iV2)
{
    V[0] = iV0;
    V[1] = iV1;
    V[2] = iV2;

    for (int i = 0; i < 3; i++)
    {
        E[i] = nullptr;
        T[i] = nullptr;
    }
}
//----------------------------------------------------------------------------
ETManifoldMesh::Triangle::~Triangle ()
{
}
//----------------------------------------------------------------------------
