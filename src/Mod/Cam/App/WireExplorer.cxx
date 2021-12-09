/***************************************************************************
 *   Copyright (c) 2007 Stephane Routelous <stephane.routelous@exotk.org>  *
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

#include "WireExplorer.h"
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopoDS_Vertex.hxx>
#include <Precision.hxx>


WireExplorer::WireExplorer(const TopoDS_Wire& wire)
:m_wire(wire),m_done(false)
{
}

WireExplorer::~WireExplorer(void)
{
}

void WireExplorer::Init()
{
    if ( !m_done )
		Perform();

    m_edgeIter = m_edges.begin();
}

bool WireExplorer::More()
{
       return m_edgeIter != m_edges.end();
}

bool WireExplorer::MoreEdge()
{
       return (m_edgeIter+1) != m_edges.end();
}

void WireExplorer::Next()
{
       ++m_edgeIter;
}

const TopoDS_Edge& WireExplorer::Current()
{
       return *m_edgeIter;
}

const TopoDS_Edge& WireExplorer::NextEdge()
{
	return *(m_edgeIter+1);
}

void WireExplorer::Perform()
{
       if ( m_wire.IsNull() )
               return;

       //adds all the vertices to a map, and store the associated edges
       TopExp_Explorer explorer;
       Standard_Integer nbEdges = 0;
       Standard_Integer nbNonEdges = 0;
       for ( explorer.Init(m_wire,TopAbs_EDGE) ; explorer.More() ; explorer.Next() )
       {
               const TopoDS_Edge& currentEdge = TopoDS::Edge(explorer.Current());
               if (IsValidEdge(currentEdge))
               {
                       Perform(currentEdge);
                       nbEdges++;
               }
               else
               {
                       nbNonEdges++;
               }
       }

       //now, iterate through the edge to sort them

       //take the first entry in the map
       tMapPntShapes::iterator iter = m_vertices.begin();
       const gp_Pnt& firstPoint = iter->first;

       gp_Pnt currentPoint = firstPoint;
       Standard_Boolean toContinue;
       do
       {
               toContinue = PerformEdges(currentPoint);
       }
       while (toContinue == Standard_True);

       m_done = true;

}

bool WireExplorer::PerformEdges(gp_Pnt& point)
{
       tMapPntShapes::iterator iter = m_vertices.find(point);
       if ( iter == m_vertices.end() )
               return false;

       tEdgeVector& edges = iter->second;

       tEdgeVector::iterator edgeIt = edges.begin();

       //no more edges. pb
       if ( edgeIt == edges.end() )
               return false;

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
       if ( P2.IsEqual(point,Precision::Confusion()) )
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
               bool somethingRemoved = false;
               for ( edgeIt = nextEdges.begin() ; edgeIt != nextEdges.end(); ++edgeIt )
               {
                       if ( theEdge.IsSame(*edgeIt) )
                       {
                               nextEdges.erase(edgeIt);
                               somethingRemoved = true;
                               break;
                       }
               }
       }

       //put the edge at the end of the list
       m_edges.push_back(theEdge);

       point = nextPoint;
       return true;

}

void WireExplorer::Perform(const TopoDS_Edge& edge)
{
       if ( edge.IsNull() )
               return;
       TopoDS_Vertex V1,V2;
       TopExp::Vertices(edge,V1,V2);
       gp_Pnt P1 = BRep_Tool::Pnt(V1);
       gp_Pnt P2 = BRep_Tool::Pnt(V2);

       tEdgeVector emptyList;
       std::pair<tMapPntShapes::iterator,bool> iter = m_vertices.insert(tMapPntShapesPair(P1,emptyList));
       iter.first->second.push_back(edge);
       iter = m_vertices.insert(tMapPntShapesPair(P2,emptyList));
       iter.first->second.push_back(edge);
}

#include <BRepAdaptor_Curve.hxx>

bool WireExplorer::IsValidEdge(const TopoDS_Edge& edge)
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

