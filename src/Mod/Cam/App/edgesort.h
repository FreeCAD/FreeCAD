/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
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

#ifndef EDGESORT_H
#define EDGESORT_H

#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>
#include <Base/BoundBox.h>
#include <list>
#include <map>

struct Edgesort_gp_Pnt_Less
{
    bool operator()(const gp_Pnt & _Left, const gp_Pnt & _Right) const
    {
        Standard_Real x1,y1,z1,x2,y2,z2;
        _Left.Coord(x1,y1,z1);
        _Right.Coord(x2,y2,z2);
        if ( fabs(x1- x2) > 0.2 )
            return x1 < x2;
        else if ( fabs(y1 -y2) > 0.2 )
            return y1 < y2;
        else if ( fabs(z1 -z2) > 0.2 )
            return z1 < z2;
        return false;
    }
};

struct EdgeSortBoundBox_Less
{
    bool operator()(const Base::BoundBox3f& _Left, const Base::BoundBox3f& _Right) const
    {
        if (_Left.IsInBox(_Right)) return false;

        return true;
    }
};

typedef std::vector<TopoDS_Edge> tEdgeVector;
typedef std::map<gp_Pnt,tEdgeVector,Edgesort_gp_Pnt_Less>  tMapPntEdge;
typedef std::pair<gp_Pnt,tEdgeVector> tMapPntEdgePair;
typedef std::map<Base::BoundBox3f,std::vector<TopoDS_Edge>,EdgeSortBoundBox_Less> tEdgeBBoxMap;
typedef std::pair<Base::BoundBox3f,std::vector<TopoDS_Edge> >tEdgeBBoxPair;

class Edgesort
{
public:
    Standard_EXPORT Edgesort(const TopoDS_Shape& aShape);
    Standard_EXPORT virtual ~Edgesort(void);

    Standard_EXPORT void Init();
    Standard_EXPORT void ReInit(const TopoDS_Shape& aShape);
    Standard_EXPORT bool More();
    Standard_EXPORT bool MoreEdge();
    Standard_EXPORT void Next();
    Standard_EXPORT const TopoDS_Edge& NextEdge();
    Standard_EXPORT const TopoDS_Edge& Current();
    Standard_EXPORT TopoDS_Shape GetDesiredCutShape(int desiredIndex);
private:
    void Perform();
    void Perform(const TopoDS_Edge& edge);
    bool PerformEdges(gp_Pnt& point);
    bool IsValidEdge(const TopoDS_Edge& edge);
    Base::BoundBox3f getBoundingBox(std::vector<TopoDS_Edge>& aList);

    TopoDS_Shape m_shape;

    tMapPntEdge m_vertices;
    tEdgeBBoxMap m_EdgeBBoxMap;
    bool m_done;
    bool m_asecondwire;
    bool m_whichedgelist;
    tEdgeVector m_edges;
    //BoundingBox m_edges, m_edges2
    tEdgeVector::const_iterator m_edgeIter;

};

#endif

