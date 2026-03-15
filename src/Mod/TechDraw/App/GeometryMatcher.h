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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <DrawViewDimension.h>

namespace Part
{
class TopoShape;
}

namespace TechDraw
{

class TechDrawExport GeometryMatcher
{
public:
    GeometryMatcher() = default;

    bool compareGeometry(const Part::TopoShape& geom1, const Part::TopoShape& geom2);

    double getPointTolerance() const
    {
        return m_pointTolerance;
    }
    void setPointTolerance(double tol)
    {
        m_pointTolerance = tol;
    }

private:
    static bool comparePoints(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    static bool compareEdges(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    static bool compareFaces(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);

    static bool compareLines(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareCircles(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareEllipses(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareBSplines(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareDifferent(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);

    static bool compareCircleArcs(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareEllipseArcs(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    static bool compareEndPoints(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);

    double m_pointTolerance {EWTOLERANCE};
};

}  // end namespace TechDraw