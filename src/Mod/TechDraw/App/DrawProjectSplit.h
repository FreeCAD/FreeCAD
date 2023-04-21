/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DrawProjectSplit_h_
#define DrawProjectSplit_h_

#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <App/FeaturePython.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawUtil.h"
#include "Geometry.h"


class gp_Pnt;
class gp_Ax2;

namespace TechDraw
{
class GeometryObject;
using GeometryObjectPtr = std::shared_ptr<GeometryObject>;
class Vertex;
class BaseGeom;
}

namespace TechDraw
{

//magic number for finding parameter of point on curve
#define PARAM_MAX_DIST 0.000001

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
    ~edgeSortItem()  = default;

    Base::Vector3d start;
    Base::Vector3d end;
    double startAngle;
    double endAngle;
    unsigned int idx;

    static bool edgeLess(const edgeSortItem& e1, const edgeSortItem& e2);
    static bool edgeEqual(const edgeSortItem& e1, const edgeSortItem& e2);
    std::string dump();
};

using vertexMap = std::map<Base::Vector3d, int, DrawUtil::vectorLessType>;

class edgeVectorEntry {
public:
    edgeVectorEntry(TopoDS_Edge e, bool flag) {
        edge = e;
        validFlag = flag;
    }
    ~edgeVectorEntry() = default;

    TopoDS_Edge edge;
    bool validFlag;
};

class TechDrawExport DrawProjectSplit
{
public:
    DrawProjectSplit();
    ~DrawProjectSplit();

public:
    static std::vector<TopoDS_Edge> getEdgesForWalker(TopoDS_Shape shape, double scale, Base::Vector3d direction);
    static TechDraw::GeometryObjectPtr  buildGeometryObject(TopoDS_Shape shape, const gp_Ax2& viewAxis);

    static bool isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds = false);
    static std::vector<TopoDS_Edge> splitEdges(std::vector<TopoDS_Edge> orig, std::vector<splitPoint> splits);
    static std::vector<TopoDS_Edge> split1Edge(TopoDS_Edge e, std::vector<splitPoint> splitPoints);

    static std::vector<splitPoint> sortSplits(std::vector<splitPoint>& s, bool ascend);
    static bool splitCompare(const splitPoint& p1, const splitPoint& p2);
    static bool splitEqual(const splitPoint& p1, const splitPoint& p2);
    static std::vector<TopoDS_Edge> removeDuplicateEdges(std::vector<TopoDS_Edge>& inEdges);
    static std::vector<edgeSortItem> sortEdges(std::vector<edgeSortItem>& e, bool ascend);


    //routines for revised face finding approach
    static std::vector<TopoDS_Edge> scrubEdges(const std::vector<BaseGeomPtr> &origEdges,
                                               std::vector<TopoDS_Edge>& closedEdges);
    static std::vector<TopoDS_Edge> scrubEdges(std::vector<TopoDS_Edge>& origEdges,
                                               std::vector<TopoDS_Edge>& closedEdges);
    static vertexMap                getUniqueVertexes(std::vector<TopoDS_Edge> inEdges);
    static std::vector<TopoDS_Edge> pruneUnconnected(vertexMap verts,
                                                     std::vector<TopoDS_Edge> edges);
    static std::vector<TopoDS_Edge> removeOverlapEdges(const std::vector<TopoDS_Edge>& inEdges);
    static std::vector<TopoDS_Edge> splitIntersectingEdges(std::vector<TopoDS_Edge>& inEdges);

    static bool                     sameEndPoints(const TopoDS_Edge& e1,
                                                  const TopoDS_Edge& e2);
    static int                      isSubset(const TopoDS_Edge &e0,
                                             const TopoDS_Edge &e1);
    static std::vector<TopoDS_Edge> fuseEdges(const TopoDS_Edge& e0,
                                              const TopoDS_Edge& e1);
    static bool                     boxesIntersect(const TopoDS_Edge& e0,
                                                   const TopoDS_Edge& e1);
    static void dumpVertexMap(vertexMap verts);

protected:
    static std::vector<TopoDS_Edge> getEdges(TechDraw::GeometryObject* geometryObject);


private:

};

using DrawProjectSplitPython = App::FeaturePythonT<DrawProjectSplit>;

} //namespace TechDraw

#endif  // #ifndef DrawProjectSplit_h_
