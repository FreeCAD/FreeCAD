/***************************************************************************
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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

#ifndef _DrawProjectSplit_h_
#define _DrawProjectSplit_h_


#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/FeaturePython.h>
#include <Base/Vector3D.h>
#include <Base/BoundBox.h>

class gp_Pnt;
class gp_Ax2;

namespace TechDrawGeometry
{
class GeometryObject;
class Vertex;
class BaseGeom;
}

namespace TechDraw
{
struct splitPoint {
    int i;
    Base::Vector3d v;
    double param;
};

class edgeSortItem
{
public:
    edgeSortItem() {
        startAngle = endAngle = 0.0;
        idx = 0;
    }
    ~edgeSortItem() {}

    Base::Vector3d start;
    Base::Vector3d end;
    double startAngle;
    double endAngle;
    unsigned int idx;

    static bool edgeLess(const edgeSortItem& e1, const edgeSortItem& e2);
    static bool edgeEqual(const edgeSortItem& e1, const edgeSortItem& e2);
    std::string dump(void);
};
class TechDrawExport DrawProjectSplit
{
public:
    DrawProjectSplit();
    ~DrawProjectSplit();

public:
    static std::vector<TopoDS_Edge> getEdgesForWalker(TopoDS_Shape shape, double scale, Base::Vector3d direction);
    static TechDrawGeometry::GeometryObject*  buildGeometryObject(TopoDS_Shape shape, const gp_Ax2& viewAxis);

    static bool isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds = false);
    static std::vector<TopoDS_Edge> splitEdges(std::vector<TopoDS_Edge> orig, std::vector<splitPoint> splits);
    static std::vector<TopoDS_Edge> split1Edge(TopoDS_Edge e, std::vector<splitPoint> splitPoints);

    static std::vector<splitPoint> sortSplits(std::vector<splitPoint>& s, bool ascend);
    static bool splitCompare(const splitPoint& p1, const splitPoint& p2);
    static bool splitEqual(const splitPoint& p1, const splitPoint& p2);
    static std::vector<TopoDS_Edge> removeDuplicateEdges(std::vector<TopoDS_Edge>& inEdges);
    static std::vector<edgeSortItem> sortEdges(std::vector<edgeSortItem>& e, bool ascend);

protected:
    static std::vector<TopoDS_Edge> getEdges(TechDrawGeometry::GeometryObject* geometryObject);


private:

};

typedef App::FeaturePythonT<DrawProjectSplit> DrawProjectSplitPython;

} //namespace TechDraw

#endif  // #ifndef _DrawProjectSplit_h_
