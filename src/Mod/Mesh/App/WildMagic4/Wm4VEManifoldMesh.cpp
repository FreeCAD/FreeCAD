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
#include "Wm4VEManifoldMesh.h"
using namespace Wm4;

//----------------------------------------------------------------------------
VEManifoldMesh::VEManifoldMesh (VCreator oVCreator, ECreator oECreator)
{
    m_oVCreator = (oVCreator ? oVCreator : CreateVertex);
    m_oECreator = (oECreator ? oECreator : CreateEdge);
}
//----------------------------------------------------------------------------
VEManifoldMesh::~VEManifoldMesh ()
{
    VMap::iterator pkVIter;
    for (pkVIter = m_kVMap.begin(); pkVIter != m_kVMap.end(); pkVIter++)
    {
        WM4_DELETE pkVIter->second;
    }

    EMap::iterator pkEIter;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        WM4_DELETE pkEIter->second;
    }
}
//----------------------------------------------------------------------------
VEManifoldMesh::VPtr VEManifoldMesh::CreateVertex (int iV)
{
    return WM4_NEW Vertex(iV);
}
//----------------------------------------------------------------------------
VEManifoldMesh::EPtr VEManifoldMesh::CreateEdge (int iV0, int iV1)
{
    return WM4_NEW Edge(iV0,iV1);
}
//----------------------------------------------------------------------------
VEManifoldMesh::EPtr VEManifoldMesh::InsertEdge (int iV0, int iV1)
{
    std::pair<int,int> kEKey(iV0,iV1);
    EMapIterator pkEIter = m_kEMap.find(kEKey);
    if (pkEIter != m_kEMap.end())
    {
        // edge already exists
        return nullptr;
    }

    // add new edge
    EPtr pkEdge = m_oECreator(iV0,iV1);
    m_kEMap[kEKey] = pkEdge;

    // add vertices to mesh
    for (int i = 0; i < 2; i++)
    {
        int iV = pkEdge->V[i];
        VPtr pkVertex;
        VMapIterator pkVIter = m_kVMap.find(iV);
        if (pkVIter == m_kVMap.end())
        {
            // first time vertex encountered
            pkVertex = m_oVCreator(iV);
            m_kVMap[iV] = pkVertex;

            // update vertex
            pkVertex->E[0] = pkEdge;
        }
        else
        {
            // second time vertex encountered
            pkVertex = pkVIter->second;
            assert(pkVertex);

            // update vertex
            if (pkVertex->E[1])
            {
                assert(false);  // mesh must be manifold
                return nullptr;
            }
            pkVertex->E[1] = pkEdge;

            // update adjacent edge
            EPtr pkAdjacent = pkVertex->E[0];
            assert(pkAdjacent);
            for (int j = 0; j < 2; j++)
            {
                if (pkAdjacent->V[j] == iV)
                {
                    pkAdjacent->E[j] = pkEdge;
                    break;
                }
            }

            // update edge
            pkEdge->E[i] = pkAdjacent;
        }
    }

    return pkEdge;
}
//----------------------------------------------------------------------------
bool VEManifoldMesh::RemoveEdge (int iV0, int iV1)
{
    std::pair<int,int> kEKey(iV0,iV1);
    EMapIterator pkEIter = m_kEMap.find(kEKey);
    if (pkEIter == m_kEMap.end())
    {
        // edge does not exist
        return false;
    }

    EPtr pkEdge = pkEIter->second;
    for (int i = 0; i < 2; i++)
    {
        // inform vertices you are going away
        VMapIterator pkVIter = m_kVMap.find(pkEdge->V[i]);
        assert(pkVIter != m_kVMap.end());
        Vertex* pkVertex = pkVIter->second;
        assert(pkVertex);
        if (pkVertex->E[0] == pkEdge)
        {
            // one-edge vertices always have pointer in slot zero
            pkVertex->E[0] = pkVertex->E[1];
            pkVertex->E[1] = nullptr;
        }
        else if (pkVertex->E[1] == pkEdge)
        {
            pkVertex->E[1] = nullptr;
        }
        else
        {
            assert(false);
            return false;
        }

        // remove vertex if you had the last reference to it
        if (!pkVertex->E[0] && !pkVertex->E[1])
        {
            m_kVMap.erase(pkVertex->V);
            WM4_DELETE pkVertex;
        }

        // inform adjacent edges you are going away
        EPtr pkAdjacent = pkEdge->E[i];
        if (pkAdjacent)
        {
            for (int j = 0; j < 2; j++)
            {
                if (pkAdjacent->E[j] == pkEdge)
                {
                    pkAdjacent->E[j] = nullptr;
                    break;
                }
            }
        }
    }

    m_kEMap.erase(kEKey);
    WM4_DELETE pkEdge;
    return true;
}
//----------------------------------------------------------------------------
bool VEManifoldMesh::IsClosed () const
{
    VMapCIterator pkVIter;
    for (pkVIter = m_kVMap.begin(); pkVIter != m_kVMap.end(); pkVIter++)
    {
        const Vertex* pkVertex = pkVIter->second;
        if (!pkVertex->E[0] || !pkVertex->E[1])
        {
            return false;
        }
    }
    return true;
}
//----------------------------------------------------------------------------
void VEManifoldMesh::Print (const char* acFilename)
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

    // print vertices
    kOStr << "vertex quantity = " << (int)m_kVMap.size() << std::endl;
    VMapIterator pkVIter;
    for (pkVIter = m_kVMap.begin(); pkVIter != m_kVMap.end(); pkVIter++)
    {
        const Vertex& rkVertex = *pkVIter->second;
        kOStr << 'v' << rkVertex.V << " <";
        if (rkVertex.E[0])
        {
            kOStr << 'e' << kEIndex[rkVertex.E[0]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkVertex.E[1])
        {
            kOStr << 'e'  << kEIndex[rkVertex.E[1]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << '>' << std::endl;
    }

    // print edges
    kOStr << "edge quantity = " << (int)m_kEMap.size() << std::endl;
    for (pkEIter = m_kEMap.begin(); pkEIter != m_kEMap.end(); pkEIter++)
    {
        const Edge& rkEdge = *pkEIter->second;
        kOStr << 'e' << kEIndex[pkEIter->second] << " <"
              << 'v' << rkEdge.V[0] << ",v" << rkEdge.V[1] << "; ";
        if (rkEdge.E[0])
        {
            kOStr << 'e' << kEIndex[rkEdge.E[0]];
        }
        else
        {
            kOStr << '*';
        }
        kOStr << ',';
        if (rkEdge.E[1])
        {
            kOStr << 'e' << kEIndex[rkEdge.E[1]];
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
// VEManifoldMesh::Vertex
//----------------------------------------------------------------------------
VEManifoldMesh::Vertex::Vertex (int iV)
{
    V = iV;
    E[0] = nullptr;
    E[1] = nullptr;
}
//----------------------------------------------------------------------------
VEManifoldMesh::Vertex::~Vertex ()
{
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// VEManifoldMesh::Edge
//----------------------------------------------------------------------------
VEManifoldMesh::Edge::Edge (int iV0, int iV1)
{
    V[0] = iV0;
    V[1] = iV1;
    E[0] = nullptr;
    E[1] = nullptr;
}
//----------------------------------------------------------------------------
VEManifoldMesh::Edge::~Edge ()
{
}
//----------------------------------------------------------------------------
