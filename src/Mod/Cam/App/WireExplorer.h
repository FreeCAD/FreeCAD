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

#ifndef WIREEXPLORER_H
#define WIREEXPLORER_H

#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Pnt.hxx>
#include <TopTools_MapOfShape.hxx>
#include <map>
#include <list>
#include <vector>

struct WireExplorer_gp_PntLess
{
    bool operator()(const gp_Pnt& _Left, const gp_Pnt& _Right) const
    {
        Standard_Real x1,y1,z1,x2,y2,z2;
        _Left.Coord(x1,y1,z1);
        _Right.Coord(x2,y2,z2);
        if ( x1 != x2 )
            return x1 < x2;
        else if ( y1 != y2 )
            return y1 < y2;
        return z1 < z2;
    }
};

typedef std::vector<TopoDS_Edge> tEdgeVector;
typedef std::map<gp_Pnt,tEdgeVector,WireExplorer_gp_PntLess>  tMapPntShapes;
typedef std::pair<gp_Pnt,tEdgeVector> tMapPntShapesPair;

class WireExplorer
{
public:
    Standard_EXPORT WireExplorer(const TopoDS_Wire& wire);
    Standard_EXPORT virtual ~WireExplorer(void);

    Standard_EXPORT void Init();
    Standard_EXPORT bool More();
    Standard_EXPORT bool MoreEdge();
    Standard_EXPORT void Next();
    Standard_EXPORT const TopoDS_Edge& NextEdge();
    Standard_EXPORT const TopoDS_Edge& Current();
private:
    void Perform();
    void Perform(const TopoDS_Edge& edge);
    bool PerformEdges(gp_Pnt& point);
    bool IsValidEdge(const TopoDS_Edge& edge);
    TopoDS_Wire m_wire;

    tMapPntShapes m_vertices;

    bool m_done;
    tEdgeVector m_edges;
    tEdgeVector::const_iterator m_edgeIter;
};

#endif

