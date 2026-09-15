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
#include "Wm4System.h"

namespace Wm4
{

class WM4_FOUNDATION_ITEM VEManifoldMesh
{
public:
    // vertex data types
    class Vertex;
    typedef Vertex* VPtr;
    typedef const Vertex* VCPtr;
    typedef VPtr (*VCreator)(int);
    typedef std::map<int,Vertex*> VMap;
    typedef VMap::iterator VMapIterator;
    typedef VMap::const_iterator VMapCIterator;

    // edge data types
    class Edge;
    typedef Edge* EPtr;
    typedef const Edge* ECPtr;
    typedef EPtr (*ECreator)(int,int);
    typedef std::map<std::pair<int,int>,Edge*> EMap;
    typedef EMap::iterator EMapIterator;
    typedef EMap::const_iterator EMapCIterator;

    // vertex object
    class WM4_FOUNDATION_ITEM Vertex
    {
    public:
        Vertex (int iV);
        virtual ~Vertex ();

        int V;
        EPtr E[2];
    };

    // edge object
    class WM4_FOUNDATION_ITEM Edge
    {
    public:
        Edge (int iV0, int iV1);
        virtual ~Edge ();

        // vertices, listed as a directed edge <V[0],V[1]>
        int V[2];

        // adjacent edges
        //   E[0] points to edge sharing V[0]
        //   E[1] points to edge sharing V[1]
        EPtr E[2];
    };


    // construction and destruction
    VEManifoldMesh (VCreator oVCreator = nullptr, ECreator oECreator = nullptr);
    virtual ~VEManifoldMesh ();

    // member access
    const VMap& GetVertices () const;
    const EMap& GetEdges () const;

    // mesh manipulation
    EPtr InsertEdge (int iV0, int iV1);
    bool RemoveEdge (int iV0, int iV1);

    // manifold mesh is closed if each vertex is shared twice
    bool IsClosed () const;

    void Print (const char* acFilename);

protected:
    // vertices
    static VPtr CreateVertex (int iV0);
    VCreator m_oVCreator;
    VMap m_kVMap;

    // edges
    static EPtr CreateEdge (int iV0, int iV1);
    ECreator m_oECreator;
    EMap m_kEMap;
};

}

#include "Wm4VEManifoldMesh.inl"