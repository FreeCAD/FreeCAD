/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
// a class to handle changes to dimension reference geometry

#ifndef GEOMETRYMATCHER_H
#define GEOMETRYMATCHER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <DrawViewDimension.h>

namespace Part
{
class TopoShape;
}

namespace TechDraw {

class TechDrawExport GeometryMatcher {
public:
    GeometryMatcher() {}
    explicit GeometryMatcher(DrawViewDimension* dim) { m_dimension = dim; }
    ~GeometryMatcher() = default;

    bool compareGeometry(Part::TopoShape geom1,  Part::TopoShape geom2);
    bool comparePoints(TopoDS_Shape& shape1,  TopoDS_Shape& shape2);
    bool compareEdges(TopoDS_Shape& shape1,  TopoDS_Shape& shape2);

    bool compareLines(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareCircles(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareEllipses(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareBSplines(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareDifferent(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareCircleArcs(TopoDS_Edge& edge1, TopoDS_Edge& edge2);
    bool compareEllipseArcs(TopoDS_Edge& edge1, TopoDS_Edge& edge2);

private:
    bool compareEndPoints(TopoDS_Edge& edge1, TopoDS_Edge& edge2);

    DrawViewDimension* m_dimension;
};

} //end namespace TechDraw
#endif

