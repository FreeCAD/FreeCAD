/***************************************************************************
 *   Copyright (c) 2010                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <TopExp_Explorer.hxx>
# include <TopAbs_ShapeEnum.hxx>
# include <BRep_Tool.hxx>
# include <TopExp.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Compound.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <GCPnts_QuasiUniformDeflection.hxx>
# include <BRep_Builder.hxx>
#endif

#include "edgecluster.h"


using namespace Part;

Edgecluster::Edgecluster(const std::vector<TopoDS_Edge>& unsorted_edges)
        :m_unsortededges(unsorted_edges),m_done(false)
{
    m_edges.clear();
    m_vertices.clear();
    m_final_cluster.clear();
}

Edgecluster::~Edgecluster(void)
{
}

tEdgeClusterVector Edgecluster::GetClusters()
{
    Perform();
    return m_final_cluster;
}

void Edgecluster::Perform()
{
    if ( m_unsortededges.empty() )
        return;

    //adds all the vertices to a map, and store the associated edges
    Standard_Integer nbEdges = 0;
    Standard_Integer nbNonEdges = 0;
    std::vector<TopoDS_Edge>::iterator aVectorIt;
    for (aVectorIt = m_unsortededges.begin();aVectorIt != m_unsortededges.end();++aVectorIt)
    {
        if (IsValidEdge(*aVectorIt))
        {
            Perform(*aVectorIt);
            nbEdges++;
        }
        else
        {
            nbNonEdges++;
        }
    }

    //now, iterate through the edges to sort and cluster them

    do
    {
        m_edges.clear();
        //Lets start with a vertice that only has one edge (that means start or end point of the merged edges!)
        tMapPntEdge::iterator iter; 
        bool closed = true;
        for(iter=m_vertices.begin();iter!=m_vertices.end();++iter)
        {
            if (iter->second.size()==1)
            {
                closed = false;
                break;
            }
        }
        if(closed)
            iter = m_vertices.begin();
        const gp_Pnt& firstPoint = iter->first;
        gp_Pnt currentPoint = firstPoint;
        Standard_Boolean toContinue;
        do
        {
            toContinue = PerformEdges(currentPoint);
        }
        while (toContinue == Standard_True);
        //Store the current adjacent edges as a cluster
        m_final_cluster.push_back(m_edges);
        //and continue now with the still existing edges in the m_vertices
    }
    while (!m_vertices.empty());

    m_done = true;
}

bool Edgecluster::PerformEdges(gp_Pnt& point)
{
    tMapPntEdge::iterator iter = m_vertices.find(point);
    if ( iter == m_vertices.end() )
        return false;

    tEdgeVector& edges = iter->second;

    tEdgeVector::iterator edgeIt = edges.begin();

    //no more edges. pb
    if ( edgeIt == edges.end() )
    {
        //Delete also the current vertex
        m_vertices.erase(iter);

        return false;
    }

    TopoDS_Edge theEdge = *edgeIt;

    //we are storing the edge, so remove it from the vertex association
    edges.erase(edgeIt);

    //if no more edges, remove the vertex
    if ( edges.empty() )
        m_vertices.erase(iter);


    TopoDS_Vertex V1,V2;
    TopExp::Vertices(theEdge,V1,V2);
    gp_Pnt P1 = BRep_Tool::Pnt(V1);
    gp_Pnt P2 = BRep_Tool::Pnt(V2);
    if ( theEdge.Orientation() == TopAbs_REVERSED )
    {
        //switch the points
        gp_Pnt tmpP = P1;
        P1 = P2;
        P2 = tmpP;
    }

    gp_Pnt nextPoint;
    if ( P2.IsEqual(point,0.2) )
    {
        //need to reverse the edge

        theEdge.Reverse();
        nextPoint = P1;
    }
    else
    {
        nextPoint = P2;
    }


    //need to erase the edge from the second point
    iter = m_vertices.find(nextPoint);
    if ( iter != m_vertices.end() )
    {
        tEdgeVector& nextEdges = iter->second;
        for ( edgeIt = nextEdges.begin() ; edgeIt != nextEdges.end(); ++edgeIt )
        {
            if ( theEdge.IsSame(*edgeIt) )
            {
                nextEdges.erase(edgeIt);
                break;
            }
        }
    }

    //put the edge at the end of the list
    m_edges.push_back(theEdge);

    //Update the point for the next do-while loop
    point = nextPoint;
    return true;

}

void Edgecluster::Perform(const TopoDS_Edge& edge)
{
    if ( edge.IsNull() )
        return;
    TopoDS_Vertex V1,V2;
    TopExp::Vertices(edge,V1,V2);
    gp_Pnt P1 = BRep_Tool::Pnt(V1);
    gp_Pnt P2 = BRep_Tool::Pnt(V2);

    tEdgeVector emptyList;

    std::pair<tMapPntEdge::iterator,bool> iter = m_vertices.insert(tMapPntEdgePair(P1,emptyList));
    iter.first->second.push_back(edge);
    iter = m_vertices.insert(tMapPntEdgePair(P2,emptyList));
    iter.first->second.push_back(edge);
}

#include <BRepAdaptor_Curve.hxx>

bool Edgecluster::IsValidEdge(const TopoDS_Edge& edge)
{
    if ( edge.IsNull() )
        return false;
    if ( BRep_Tool::Degenerated(edge) )
        return false;

    BRepAdaptor_Curve bac(edge);

    Standard_Real fparam = bac.FirstParameter();
    Standard_Real lparam = bac.LastParameter();

    gp_Pnt fpoint = bac.Value(fparam);
    gp_Pnt lpoint = bac.Value(lparam);

    //do not test the distance first last in case of a full circle edge (fpoint == lastpoint)
    //if ( fpoint.IsEqual(lpoint,1e-5 ) )
    //      return false;

    gp_Pnt mpoint = bac.Value((fparam+lparam)*0.5);

    Standard_Real dist = mpoint.Distance(lpoint);
    if ( dist <= 1e-5 )
        return false;
    dist = mpoint.Distance(fpoint);
    if ( dist <= 1e-5 )
        return false;

    return true;
}

