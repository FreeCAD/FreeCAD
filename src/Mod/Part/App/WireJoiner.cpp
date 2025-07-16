/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#include <boost/core/ignore_unused.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/graph/graph_concepts.hpp>

#ifndef _PreComp_
# include <limits>
# include <BRepLib.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <BRepTools.hxx>
# include <BRepTools_WireExplorer.hxx>
# include <gp_Pln.hxx>
# include <GeomAdaptor_Curve.hxx>
# include <GeomLProp_CLProps.hxx>
# include <GProp_GProps.hxx>
# include <ShapeAnalysis_Wire.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <ShapeExtend_WireData.hxx>
# include <ShapeFix_Wire.hxx>
# include <ShapeFix_Shape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_HSequenceOfShape.hxx>
#endif

#include <BRepTools_History.hxx>
#include <ShapeBuild_ReShape.hxx>

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <boost/geometry.hpp>
#include <utility>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Sequencer.h>
#include <Base/Parameter.h>
#include <App/Application.h>

#include "WireJoiner.h"

#include "Geometry.h"
#include "PartFeature.h"
#include "TopoShapeOpCode.h"
#include "TopoShapeMapper.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

const size_t RParametersNumber = 16UL;
using RParameters = bgi::linear<RParametersNumber>;

BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET(
        gp_Pnt,double,bg::cs::cartesian,X,Y,Z,SetX,SetY,SetZ)

FC_LOG_LEVEL_INIT("WireJoiner",true, true)

using namespace Part;

static inline void getEndPoints(const TopoDS_Edge &eForEndPoints, gp_Pnt &p1, gp_Pnt &p2) {
    p1 = BRep_Tool::Pnt(TopExp::FirstVertex(eForEndPoints));
    p2 = BRep_Tool::Pnt(TopExp::LastVertex(eForEndPoints));
}

static inline void getEndPoints(const TopoDS_Wire &wire, gp_Pnt &p1, gp_Pnt &p2) {
    BRepTools_WireExplorer xp(wire);
    p1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.CurrentVertex()));
    for(;xp.More();xp.Next()) {};
    p2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.CurrentVertex()));
}

// Originally here there was the definition of the precompiler macro assertCheck() and of the method
// _assertCheck(), that have been replaced with the already defined precompiler macro assert().
// See
// https://github.com/realthunder/FreeCAD/blob/6f15849be2505f98927e75d0e8352185e14e7b72/src/Mod/Part/App/WireJoiner.cpp#L107
// for reference and https://github.com/FreeCAD/FreeCAD/pull/12535/files#r1526647457 for the
// discussion about replacing it

class WireJoiner::WireJoinerP {
public:
    double myTol = Precision::Confusion();
    double myTol2 = myTol * myTol;
    double myAngularTol = Precision::Angular();
    bool doSplitEdge = true;
    bool doMergeEdge = true;
    bool doOutline = false;
    bool doTightBound = true;

    std::string catchObject;
    int catchIteration {};
    int iteration = 0;

    using Box = bg::model::box<gp_Pnt>;

    bool checkBBox(const Bnd_Box &box) const
    {
        if (box.IsVoid()) {
            return false;
        }
        Standard_Real xMin = Standard_Real();
        Standard_Real yMin = Standard_Real();
        Standard_Real zMin = Standard_Real();
        Standard_Real xMax = Standard_Real();
        Standard_Real yMax = Standard_Real();
        Standard_Real zMax = Standard_Real();
        box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        return zMax - zMin <= myTol;
    }

    WireJoinerP()
        : catchObject(App::GetApplication()
                          .GetParameterGroupByPath("User parameter:BaseApp/Preferences/WireJoiner")
                          ->GetASCII("ObjectName"))
        , catchIteration(static_cast<int>(
              App::GetApplication()
                  .GetParameterGroupByPath("User parameter:BaseApp/Preferences/WireJoiner")
                  ->GetInt("Iteration", 0)))
    {}

    bool getBBox(const TopoDS_Shape &eForBBox, Bnd_Box &bound) {
        BRepBndLib::AddOptimal(eForBBox,bound,Standard_False);
        if (bound.IsVoid()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("failed to get bound of edge");
            }
            return false;
        }
        if (!checkBBox(bound)) {
            showShape(eForBBox, "invalid");
        }
        if (bound.SquareExtent() < myTol2) {
            return false;
        }
        bound.Enlarge(myTol);
        return true;
    }

    bool getBBox(const TopoDS_Shape &eForBBox, Box &box) {
        Bnd_Box bound;
        if (!getBBox(eForBBox, bound)) {
            return false;
        }
        Standard_Real xMin = Standard_Real();
        Standard_Real yMin = Standard_Real();
        Standard_Real zMin = Standard_Real();
        Standard_Real xMax = Standard_Real();
        Standard_Real yMax = Standard_Real();
        Standard_Real zMax = Standard_Real();
        bound.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        box = Box(gp_Pnt(xMin,yMin,zMin), gp_Pnt(xMax,yMax,zMax));
        return true;
    }

    struct WireInfo;
    struct EdgeSet;

    struct EdgeInfo {
        TopoDS_Edge edge;
        TopoDS_Wire superEdge;
        mutable TopoDS_Shape edgeReversed;
        mutable TopoDS_Shape superEdgeReversed;
        gp_Pnt p1;
        gp_Pnt p2;
        gp_Pnt mid;
        Box box;
        std::array<int, 2> iStart {};  // adjacent list index start for p1 and p2
        std::array<int, 2> iEnd {};    // adjacent list index end
        int iteration {};
        int iteration2 {};
        bool queryBBox;
        std::shared_ptr<WireInfo> wireInfo {};
        std::shared_ptr<WireInfo> wireInfo2 {}; // an edge can be shared by at most two tight bound wires.
        std::unique_ptr<Geometry> geo {};
        Standard_Real firstParam {};
        Standard_Real lastParam {};
        Handle_Geom_Curve curve;
        GeomAbs_CurveType type {};
        bool isLinear;

        EdgeInfo(const TopoDS_Edge& eForInfo,
                 const gp_Pnt& pt1,
                 const gp_Pnt& pt2,
                 const Box& bound,
                 bool bbox,
                 bool isLinear)
            : edge(eForInfo)
            , p1(pt1)
            , p2(pt2)
            , box(bound)
            , queryBBox(bbox)
            , isLinear(isLinear)
        {
            curve = BRep_Tool::Curve(eForInfo, firstParam, lastParam);
            type = GeomAdaptor_Curve(curve).GetType();

            // Originally here there was a call to the precompiler macro assertCheck(), which has
            // been replaced with the precompiler macro assert()

            assert(!curve.IsNull());
            const double halving {0.5};
            GeomLProp_CLProps prop(curve,(firstParam+lastParam)*halving,0,Precision::Confusion());
            mid = prop.Value();

            reset();
        }
        Geometry *geometry() {
            if (!geo) {
                geo = Geometry::fromShape(edge, /*silent*/ true);
            }
            return geo.get();
        }
        void reset() {
            wireInfo.reset();
            wireInfo2.reset();
            if (iteration >= 0) {
                iteration = 0;
            }
            iteration2 = 0;
            iStart[0] = iStart[1] = iEnd[0] = iEnd[1] = -1;
        }
        const TopoDS_Shape &shape(bool forward=true) const
        {
            if (superEdge.IsNull()) {
                if (forward) {
                    return edge;
                }
                if (edgeReversed.IsNull()) {
                    edgeReversed = edge.Reversed();
                }
                return edgeReversed;
            }
            if (forward) {
                return superEdge;
            }
            if (superEdgeReversed.IsNull()) {
                superEdgeReversed = superEdge.Reversed();
            }
            return superEdgeReversed;
        }
        TopoDS_Wire wire() const
        {
            auto sForWire = shape();
            if (sForWire.ShapeType() == TopAbs_WIRE) {
                return TopoDS::Wire(sForWire);
            }
            return BRepBuilderAPI_MakeWire(TopoDS::Edge(sForWire)).Wire();
        }
    };

    template<class T>
    struct VectorSet {
        void sort()
        {
            if (!sorted) {
                sorted = true;
                std::sort(data.begin(), data.end());
            }
        }
        bool contains(const T &vForContains)
        {
            if (!sorted) {
                constexpr static size_t dataSizeMax = 30;
                if (data.size() < dataSizeMax) {
                    return std::ranges::find(data, vForContains) != data.end();
                }
                sort();
            }
            auto it = std::ranges::lower_bound(data, vForContains);
            return it!=data.end() && *it == vForContains;
        }
        bool intersects(const VectorSet<T> &other)
        {
            if (other.size() < size()) {
                return other.intersects(*this);
            }
            if (!sorted) {
                for (const auto &vector : data) {
                    if (other.contains(vector)) {
                        return true;
                    }
                }
            }
            else {
                other.sort();
                auto it = other.data.begin();
                for (const auto &vertex : data) {
                    it = std::lower_bound(it, other.data.end(), vertex);
                    if (it == other.data.end()) {
                        return false;
                    }
                    if (*it == vertex) {
                        return true;
                    }
                }
            }
            return false;
        }
        void insert(const T &vToInsert)
        {
            if (sorted) {
                data.insert(std::ranges::upper_bound(data, vToInsert), vToInsert);
            }
            else {
                data.push_back(vToInsert);
            }
        }
        bool insertUnique(const T &vToInsertUnique)
        {
            if (sorted) {
                auto it = std::ranges::lower_bound(data, vToInsertUnique);
                bool insert = !(it != data.end() && *it == vToInsertUnique);
                if (insert) {
                    data.insert(it, vToInsertUnique);
                }
                return insert;
            }

            if (contains(vToInsertUnique)) {
                return false;
            }
            data.push_back(vToInsertUnique);
            return true;
        }
        void erase(const T &vToErase)
        {
            if (!sorted) {
                data.erase(std::remove(data.begin(), data.end(), vToErase), data.end());
            }
            else {
                auto it = std::ranges::lower_bound(data, vToErase);
                auto itEnd = it;
                while (itEnd != data.end() && *itEnd == vToErase) {
                    ++itEnd;
                }
                data.erase(it, itEnd);
            }
            const size_t dataSizeMax = 20;
            if (data.size() < dataSizeMax) {
                sorted = false;
            }
        }
        void clear()
        {
            data.clear();
            sorted = false;
        }
        std::size_t size() const
        {
            return data.size();
        }
        bool empty() const
        {
            return data.empty();
        }
    private:
        bool sorted = false;
        std::vector<T> data {};
    };

    Handle(BRepTools_History) aHistory = new BRepTools_History;

    using Edges = std::list<EdgeInfo>;
    Edges edges {};

    std::map<EdgeInfo*, Edges::iterator> edgesTable {};

    struct VertexInfo {
        Edges::iterator it {};
        bool start {};
        VertexInfo() = default;
        VertexInfo(Edges::iterator it, bool start)
            :it(it),start(start)
        {}
        VertexInfo reversed() const {
            return {it, !start};
        }
        bool operator==(const VertexInfo &other) const {
            return it==other.it && start==other.start;
        }
        bool operator<(const VertexInfo &other) const {
            auto thisInfo = edgeInfo();
            auto otherInfo = other.edgeInfo();
            if (thisInfo < otherInfo) {
                return true;
            }
            if (thisInfo > otherInfo) {
                return false;
            }
            return static_cast<int>(start) < static_cast<int>(other.start);
        }
        const gp_Pnt &pt() const {
            return start?it->p1:it->p2;
        }
        gp_Pnt &pt() {
            return start?it->p1:it->p2;
        }
        const gp_Pnt &ptOther() const {
            return start?it->p2:it->p1;
        }
        gp_Pnt &ptOther() {
            return start?it->p2:it->p1;
        }
        TopoDS_Vertex vertex() const {
            return start ? TopExp::FirstVertex(edge()) : TopExp::LastVertex(edge());
        }
        TopoDS_Vertex otherVertex() const {
            return start ? TopExp::LastVertex(edge()) : TopExp::FirstVertex(edge());
        }
        EdgeInfo *edgeInfo() const {
            return &(*it);
        }
        const TopoDS_Edge &edge() const {
            return it->edge;
        }
    };

    struct StackInfo {
        size_t iStart;
        size_t iEnd;
        size_t iCurrent;
        explicit StackInfo(size_t idx = 0)
            : iStart(idx)
            , iEnd(idx)
            , iCurrent(idx)
        {}
    };

    std::vector<StackInfo> stack {};
    std::vector<VertexInfo> vertexStack {};
    std::vector<VertexInfo> tmpVertices {};
    std::vector<VertexInfo> adjacentList {};

    struct WireInfo {
        std::vector<VertexInfo> vertices {};
        mutable std::vector<int> sorted {};
        TopoDS_Wire wire;
        TopoDS_Face face;
        mutable Bnd_Box box;
        bool done = false;
        bool purge = false;

        void sort() const
        {
            if (sorted.size() == vertices.size()) {
                return;
            }

            // Originally here there was a call to the precompiler macro assertCheck(), which has
            // been replaced with the precompiler macro assert()

            assert(sorted.size() < vertices.size());
            sorted.reserve(vertices.size());
            for (int i = (int)sorted.size(); i < (int)vertices.size(); ++i) {
                sorted.push_back(i);
            }
            std::sort(sorted.begin(), sorted.end(), [&](int vA, int vB) {
                return vertices[vA] < vertices[vB];
            });
        }
        int find(const VertexInfo &info) const
        {
            const size_t verticesSizeMax = 20;
            if (vertices.size() < verticesSizeMax) {
                const auto it = std::ranges::find(vertices, info);
                if (it == vertices.end()) {
                    return 0;
                }
                return (static_cast<int>(it - vertices.begin()) + 1);
            }
            sort();
            const auto it = std::lower_bound(sorted.begin(), sorted.end(), info,
                    [&](const int idx, const VertexInfo &vertex) {return vertices[idx]<vertex;});
            int res = 0;
            if (it != sorted.end() && vertices[*it] == info) {
                res = *it + 1;
            }
            return res;
        }
        int find(const EdgeInfo *info) const
        {
            const size_t verticesSizeMax = 20;
            if (vertices.size() < verticesSizeMax) {
                for (auto it=vertices.begin(); it!=vertices.end(); ++it) {
                    if (it->edgeInfo() == info) {
                        return (static_cast<int>(it - vertices.begin()) + 1);
                    }
                }
                return 0;
            }
            sort();
            const auto it = std::lower_bound(sorted.begin(), sorted.end(), info,
                    [&](int idx, const EdgeInfo *vertex) {return vertices[idx].edgeInfo()<vertex;});
            int res = 0;
            if (it != sorted.end() && vertices[*it].edgeInfo() == info) {
                res = *it + 1;
            }
            return res;
        }
        bool isSame(const WireInfo &other) const
        {
            if (this == &other) {
                return true;
            }
            if (vertices.size() != other.vertices.size()) {
                return false;
            }
            if (vertices.empty()) {
                return true;
            }
            int idx=find(other.vertices.front().edgeInfo()) - 1;
            if (idx < 0) {
                return false;
            }
            for (auto &vertex : other.vertices) {
                if (vertex.edgeInfo() != vertices[idx].edgeInfo()) {
                    return false;
                }
                if (++idx == (int)vertices.size()) {
                    idx = 0;
                }
            }
            return true;
        }
    };

    struct EdgeSet: VectorSet<EdgeInfo*> {
    };
    EdgeSet edgeSet;

    struct WireSet: VectorSet<WireInfo*> {
    };
    WireSet wireSet;

    const Bnd_Box &getWireBound(const WireInfo &wireInfo) const
    {
        if (wireInfo.box.IsVoid()) {
            for (auto& vertex : wireInfo.vertices) {
                BRepBndLib::Add(vertex.it->shape(), wireInfo.box);
            }
            wireInfo.box.Enlarge(myTol);
        }
        return wireInfo.box;
    }

    // This method was originally part of WireJoinerP::initWireInfo(), split to reduce cognitive
    // complexity
    bool initWireInfoWireClosed(const WireInfo& wireInfo)
    {
        if (!BRep_Tool::IsClosed(wireInfo.wire)) {
            showShape(wireInfo.wire, "FailedToClose");
            FC_ERR("Wire not closed");
            for (auto& vertex : wireInfo.vertices) {
                showShape(vertex.edgeInfo(), vertex.start ? "failed" : "failed_r", iteration);
            }
            return false;
        }
        return true;
    }

    // This method was originally part of WireJoinerP::initWireInfo(), split to reduce cognitive
    // complexity
    bool initWireInfoFaceDone(WireInfo& wireInfo)
    {
        BRepBuilderAPI_MakeFace mkFace(wireInfo.wire);
        if (!mkFace.IsDone()) {
            FC_ERR("Failed to create face for wire");
            showShape(wireInfo.wire, "FailedFace");
            return false;
        }
        wireInfo.face = mkFace.Face();
        return true;
    }

    bool initWireInfo(WireInfo &wireInfo)
    {
        if (!wireInfo.face.IsNull()) {
            return true;
        }
        getWireBound(wireInfo);
        if (wireInfo.wire.IsNull()) {
            wireData->Clear();
            for (auto& vertex : wireInfo.vertices) {
                wireData->Add(vertex.it->shape(vertex.start));
            }
            wireInfo.wire = makeCleanWire();
        }

        if (!initWireInfoWireClosed(wireInfo)) {
            return false;
        }

        if (!initWireInfoFaceDone(wireInfo)) {
            return false;
        }

        return true;
    }

    bool isInside(const WireInfo &wireInfo, gp_Pnt &pt) const
    {
        if (getWireBound(wireInfo).IsOut(pt)) {
            return false;
        }
        BRepClass_FaceClassifier fc(wireInfo.face, pt, myTol);
        return fc.State() == TopAbs_IN;
    }

    bool isOutside(const WireInfo &wireInfo, gp_Pnt &pt) const
    {
        if (getWireBound(wireInfo).IsOut(pt)) {
            return false;
        }
        BRepClass_FaceClassifier fc(wireInfo.face, pt, myTol);
        return fc.State() == TopAbs_OUT;
    }

    struct PntGetter
    {
        using result_type = const gp_Pnt&;
        result_type operator()(const VertexInfo &vInfo) const {
            return vInfo.pt();
        }
    };

    bgi::rtree<VertexInfo, RParameters, PntGetter> vmap {};

    struct BoxGetter
    {
        using result_type = const Box&;
        result_type operator()(Edges::iterator it) const {
            return it->box;
        }
    };
    bgi::rtree<Edges::iterator, RParameters, BoxGetter> boxMap {};

    BRep_Builder builder;
    TopoDS_Compound compound;

    std::unordered_set<TopoShape, ShapeHasher, ShapeHasher> sourceEdges {};
    std::vector<TopoShape> sourceEdgeArray {};
    TopoDS_Compound openWireCompound;

    Handle(ShapeExtend_WireData) wireData = new ShapeExtend_WireData();

    void clear()
    {
        aHistory->Clear();
        iteration = 0;
        boxMap.clear();
        vmap.clear();
        edges.clear();
        edgeSet.clear();
        wireSet.clear();
        adjacentList.clear();
        stack.clear();
        tmpVertices.clear();
        vertexStack.clear();
        builder.MakeCompound(compound);
        openWireCompound.Nullify();
    }

    Edges::iterator remove(Edges::iterator it)
    {
        if (it->queryBBox) {
            boxMap.remove(it);
        }
        vmap.remove(VertexInfo(it,true));
        vmap.remove(VertexInfo(it,false));
        return edges.erase(it);
    }

    void remove(EdgeInfo *info)
    {
        if (edgesTable.empty()) {
            for (auto it = edges.begin(); it != edges.end(); ++it) {
                edgesTable[&(*it)] = it;
            }
        }
        auto it = edgesTable.find(info);
        if (it != edgesTable.end()) {
            remove(it->second);
            edgesTable.erase(it);
        }
    }

    void add(Edges::iterator it)
    {
        vmap.insert(VertexInfo(it,true));
        vmap.insert(VertexInfo(it,false));
        if (it->queryBBox) {
            boxMap.insert(it);
        }
        showShape(it->edge, "add");
    }

    int add(const TopoDS_Edge &eToAdd, bool queryBBox=false)
    {
        auto it = edges.begin();
        return add(eToAdd, queryBBox, it);
    }

    int add(const TopoDS_Edge &eToAdd, bool queryBBox, Edges::iterator &it)
    {
        Box bbox;
        if (!getBBox(eToAdd, bbox)) {
            showShape(eToAdd, "small");
            aHistory->Remove(eToAdd);
            return 0;
        }
        return add(eToAdd, queryBBox, bbox, it) ? 1 : -1;
    }

    // This method was originally part of WireJoinerP::add(), split to reduce cognitive complexity
    bool addNoDuplicates(const TopoDS_Edge& eToAdd,
                         TopoDS_Vertex& v2,
                         TopoDS_Edge& ev2,
                         const bool isLinear,
                         const VertexInfo& vinfo,
                         std::unique_ptr<Geometry>& geo)
    {
        if (v2.IsNull()) {
            ev2 = vinfo.edge();
            v2 = vinfo.otherVertex();
        }
        if (isLinear && vinfo.edgeInfo()->isLinear) {
            showShape(eToAdd, "duplicate");
            aHistory->Remove(eToAdd);
            return false;
        }
        if (auto geoEdge = vinfo.edgeInfo()->geometry()) {
            if (!geo) {
                geo = Geometry::fromShape(eToAdd, /*silent*/ true);
            }
            if (geo && geo->isSame(*geoEdge, myTol, myAngularTol)) {
                showShape(eToAdd, "duplicate");
                aHistory->Remove(eToAdd);
                return false;
            }
        }
        return true;
    }

    // This method was originally part of WireJoinerP::add(), split to reduce cognitive complexity
    bool addValidEdges(const TopoDS_Edge& eToAdd,
                       const gp_Pnt p1,
                       const double tol,
                       TopoDS_Vertex& v1,
                       TopoDS_Edge& ev1,
                       const gp_Pnt p2,
                       TopoDS_Vertex& v2,
                       TopoDS_Edge& ev2,
                       const bool isLinear)
    {
        std::unique_ptr<Geometry> geo;
        constexpr int max = std::numeric_limits<int>::max();
        for (auto vit = vmap.qbegin(bgi::nearest(p1, max)); vit != vmap.qend(); ++vit) {
            auto& vinfo = *vit;
            if (canShowShape()) {
#if OCC_VERSION_HEX < 0x070800
                FC_MSG("addcheck " << vinfo.edge().HashCode(max));
#else
                FC_MSG("addcheck " << std::hash<TopoDS_Edge> {}(vinfo.edge()));
#endif
            }
            double d1 = vinfo.pt().SquareDistance(p1);
            if (d1 >= tol) {
                break;
            }
            if (v1.IsNull()) {
                ev1 = vinfo.edge();
                v1 = vinfo.vertex();
            }
            double d2 = vinfo.ptOther().SquareDistance(p2);
            if (d2 < tol) {
                if (!addNoDuplicates(eToAdd, v2, ev2, isLinear, vinfo, geo)){
                    return false;
                }
            }
        }
        return true;
    }

    bool add(const TopoDS_Edge &eToAdd, bool queryBBox, const Box &bbox, Edges::iterator &it)
    {
        gp_Pnt p1 = gp_Pnt();
        gp_Pnt p2 = gp_Pnt();
        getEndPoints(eToAdd,p1,p2);
        TopoDS_Vertex v1 = TopoDS_Vertex();
        TopoDS_Vertex v2 = TopoDS_Vertex();
        TopoDS_Edge ev1 = TopoDS_Edge();
        TopoDS_Edge ev2 = TopoDS_Edge();
        double tol = myTol2;
        // search for duplicate edges
        showShape(eToAdd, "addcheck");
        bool isLinear = TopoShape(eToAdd).isLinearEdge();

        if (!addValidEdges(eToAdd, p1, tol, v1, ev1, p2, v2, ev2, isLinear)){
            return false;
        }

        if (v2.IsNull()) {
            for (auto vit=vmap.qbegin(bgi::nearest(p2,1));vit!=vmap.qend();++vit) {
                auto &vinfo = *vit;
                double d1 = vinfo.pt().SquareDistance(p2);
                if (d1 < tol) {
                    v2 = vit->vertex();
                    ev2 = vit->edge();
                }
            }
        }

        // Make sure coincident vertices are actually the same TopoDS_Vertex,
        // which is crucial for the OCC internal shape hierarchy structure. We
        // achieve this by making a temp wire and let OCC do the hard work of
        // replacing the vertex.
        auto connectEdge = [&](TopoDS_Edge& eCurrent,
                               const TopoDS_Vertex& vCurrent,
                               const TopoDS_Edge& eOther,
                               const TopoDS_Vertex& vOther) {
            if (vOther.IsNull()) {
                return;
            }
            if (vCurrent.IsSame(vOther)) {
                return;
            }
            double tol = std::max(BRep_Tool::Pnt(vCurrent).Distance(BRep_Tool::Pnt(vOther)),
                                  BRep_Tool::Tolerance(vOther));
            if (tol >= BRep_Tool::Tolerance(vCurrent)) {
                ShapeFix_ShapeTolerance fix;
                const double halving {0.5};
                fix.SetTolerance(vCurrent, std::max(tol*halving, myTol), TopAbs_VERTEX);
            }
            BRepBuilderAPI_MakeWire mkWire(eOther);
            mkWire.Add(eCurrent);
            auto newEdge = mkWire.Edge();
            TopoDS_Vertex vFirst = TopExp::FirstVertex(newEdge);
            TopoDS_Vertex vLast = TopExp::LastVertex(newEdge);

            // Originally here there was a call to the precompiler macro assertCheck(), which has
            // been replaced with the precompiler macro assert()

            assert(vLast.IsSame(vOther) || vFirst.IsSame(vOther));
            eCurrent = newEdge;
        };

        TopoDS_Edge edge = eToAdd;
        TopoDS_Vertex vFirst = TopExp::FirstVertex(eToAdd);
        TopoDS_Vertex vLast = TopExp::LastVertex(eToAdd);
        connectEdge(edge, vFirst, ev1, v1);
        connectEdge(edge, vLast, ev2, v2);
        if (!edge.IsSame(eToAdd)) {
            auto itSource = sourceEdges.find(eToAdd);
            if (itSource != sourceEdges.end()) {
                TopoShape newEdge = *itSource;
                newEdge.setShape(edge, false);
                sourceEdges.erase(itSource);
                sourceEdges.insert(newEdge);
            }
            getEndPoints(edge,p1,p2);
            // Shall we also update bbox?
        }
        it = edges.emplace(it,edge,p1,p2,bbox,queryBBox,isLinear);
        add(it);
        return true;
    }

    void add(const TopoDS_Shape &shape, bool queryBBox=false)
    {
        for (TopExp_Explorer xp(shape, TopAbs_EDGE); xp.More(); xp.Next()) {
            add(TopoDS::Edge(xp.Current()), queryBBox);
        }
    }

    // This method was originally part of WireJoinerP::join(), split to reduce cognitive complexity
    void joinMakeWire(const int idx,
                      BRepBuilderAPI_MakeWire& mkWire,
                      const Edges::iterator it,
                      bool& done)
    {
        double tol = myTol2;
        gp_Pnt pstart(it->p1);
        gp_Pnt pend(it->p2);
        while (!edges.empty()) {
            std::vector<VertexInfo> ret;
            ret.reserve(1);
            const gp_Pnt& pt = idx == 0 ? pstart : pend;
            vmap.query(bgi::nearest(pt, 1), std::back_inserter(ret));

            // Originally here there was a call to the precompiler macro assertCheck(),
            // which has been replaced with the precompiler macro assert()

            assert(ret.size() == 1);
            double dist = ret[0].pt().SquareDistance(pt);
            if (dist > tol) {
                break;
            }

            const auto& info = *ret[0].it;
            bool start = ret[0].start;
            if (dist > Precision::SquareConfusion()) {
                // insert a filling edge to solve the tolerance problem
                const gp_Pnt& pt = ret[idx].pt();
                if (idx != 0) {
                    mkWire.Add(BRepBuilderAPI_MakeEdge(pend, pt).Edge());
                }
                else {
                    mkWire.Add(BRepBuilderAPI_MakeEdge(pt, pstart).Edge());
                }
            }

            if (idx == 1 && start) {
                pend = info.p2;
                mkWire.Add(info.edge);
            }
            else if (idx == 0 && !start) {
                pstart = info.p1;
                mkWire.Add(info.edge);
            }
            else if (idx == 0 && start) {
                pstart = info.p2;
                mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
            }
            else {
                pend = info.p1;
                mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
            }
            remove(ret[0].it);
            if (pstart.SquareDistance(pend) <= Precision::SquareConfusion()) {
                done = true;
                break;
            }
        }
    }

    //This algorithm tries to join connected edges into wires
    //
    //tol*tol>Precision::SquareConfusion() can be used to join points that are
    //close but do not coincide with a line segment. The close points may be
    //the results of rounding issue.
    //
    void join()
    {
        while (!edges.empty()) {
            auto it = edges.begin();
            BRepBuilderAPI_MakeWire mkWire;
            mkWire.Add(it->edge);
            remove(it);

            bool done = false;
            for (int idx=0;!done&&idx<2;++idx) {
                joinMakeWire(idx, mkWire, it, done);
            }

            builder.Add(compound,mkWire.Wire());
        }
    }

    struct IntersectInfo {
        double param;
        TopoDS_Shape intersectShape;
        gp_Pnt point;
        IntersectInfo(double pToIntersect, const gp_Pnt& pt, TopoDS_Shape sToIntersect)
            : param(pToIntersect)
            , intersectShape(std::move(sToIntersect))
            , point(pt)
        {}
        bool operator<(const IntersectInfo &other) const {
            return param < other.param;
        }
    };

    void checkSelfIntersection(const EdgeInfo &info, std::set<IntersectInfo> &params) const
    {
        // Early return if checking for self intersection (only for non linear spline curves)
        if (info.type <= GeomAbs_Parabola || info.isLinear) {
            return;
        }
        IntRes2d_SequenceOfIntersectionPoint points2d;
        TColgp_SequenceOfPnt points3d;
        TColStd_SequenceOfReal errors;
        TopoDS_Wire wire;
        BRepBuilderAPI_MakeWire mkWire(info.edge);
        if (!mkWire.IsDone()) {
            return;
        }
        if (!BRep_Tool::IsClosed(mkWire.Wire())) {
            BRepBuilderAPI_MakeEdge mkEdge(info.p1, info.p2);
            if (!mkEdge.IsDone()) {
                return;
            }
            mkWire.Add(mkEdge.Edge());
        }
        wire = mkWire.Wire();
        BRepBuilderAPI_MakeFace mkFace(wire);
        if (!mkFace.IsDone()) {
            return;
        }
        const TopoDS_Face& face = mkFace.Face();
        ShapeAnalysis_Wire analysis(wire, face, myTol);
        analysis.CheckSelfIntersectingEdge(1, points2d, points3d);

        // Originally here there was a call to the precompiler macro assertCheck(), which has been
        // replaced with the precompiler macro assert()

        assert(points2d.Length() == points3d.Length());
        for (int i=1; i<=points2d.Length(); ++i) {
            params.emplace(points2d(i).ParamOnFirst(), points3d(i), info.edge);
            params.emplace(points2d(i).ParamOnSecond(), points3d(i), info.edge);
        }
    }

    // This method was originally part of WireJoinerP::checkIntersection(), split to reduce
    // cognitive complexity
    bool checkIntersectionPlanar(const EdgeInfo& info,
                                 const EdgeInfo& other,
                                 std::set<IntersectInfo>& params1,
                                 std::set<IntersectInfo>& params2)
    {
        gp_Pln pln;
        bool planar = TopoShape(info.edge).findPlane(pln);
        if (!planar) {
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            builder.Add(comp, info.edge);
            builder.Add(comp, other.edge);
            planar = TopoShape(comp).findPlane(pln);
            if (!planar) {
                BRepExtrema_DistShapeShape extss(info.edge, other.edge);
                extss.Perform();
                if (extss.IsDone() && extss.NbSolution() > 0) {
                    if (!extss.IsDone() || extss.NbSolution() <= 0 || extss.Value() >= myTol) {
                        return false;
                    }
                }
                for (int i = 1; i <= extss.NbSolution(); ++i) {
                    Standard_Real par = Standard_Real();
                    auto s1 = extss.SupportOnShape1(i);
                    auto s2 = extss.SupportOnShape2(i);
                    if (s1.ShapeType() == TopAbs_EDGE) {
                        extss.ParOnEdgeS1(i, par);
                        pushIntersection(params1, par, extss.PointOnShape1(i), other.edge);
                    }
                    if (s2.ShapeType() == TopAbs_EDGE) {
                        extss.ParOnEdgeS2(i, par);
                        pushIntersection(params2, par, extss.PointOnShape2(i), info.edge);
                    }
                }
                return false;
            }
        }

        return true;
    }

    // This method was originally part of WireJoinerP::checkIntersection(), split to reduce
    // cognitive complexity
    static bool checkIntersectionEdgeDone(const BRepBuilderAPI_MakeEdge& mkEdge)
    {
        if (!mkEdge.IsDone()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("Failed to build edge for checking intersection");
            }
            return false;
        }
        return true;
    }

    // This method was originally part of WireJoinerP::checkIntersection(), split to reduce
    // cognitive complexity
    static bool checkIntersectionWireDone(const BRepBuilderAPI_MakeWire& mkWire)
    {
        if (!mkWire.IsDone()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("Failed to build wire for checking intersection");
            }
            return false;
        }
        return true;
    }

    // This method was originally part of WireJoinerP::checkIntersection(), split to reduce
    // cognitive complexity
    static bool checkIntersectionMakeWire(const EdgeInfo& info,
                                          const EdgeInfo& other,
                                          int& idx,
                                          TopoDS_Wire& wire)
    {
        BRepBuilderAPI_MakeWire mkWire(info.edge);
        mkWire.Add(other.edge);
        if (mkWire.IsDone()) {
            idx = 2;
        }
        else if (mkWire.Error() == BRepBuilderAPI_DisconnectedWire) {
            idx = 3;
            BRepBuilderAPI_MakeEdge mkEdge(info.p1, other.p1);

            if (!checkIntersectionEdgeDone(mkEdge)) {
                return false;
            }

            mkWire.Add(mkEdge.Edge());
            mkWire.Add(other.edge);
        }

        if (!checkIntersectionWireDone(mkWire)) {
            return false;
        }

        wire = mkWire.Wire();
        if (!BRep_Tool::IsClosed(wire)) {
            gp_Pnt p1 = gp_Pnt();
            gp_Pnt p2 = gp_Pnt();
            getEndPoints(wire, p1, p2);
            BRepBuilderAPI_MakeEdge mkEdge(p1, p2);

            if (!checkIntersectionEdgeDone(mkEdge)) {
                return false;
            }

            mkWire.Add(mkEdge.Edge());
        }
        return true;
    }

    // This method was originally part of WireJoinerP::checkIntersection(), split to reduce
    // cognitive complexity
    static bool checkIntersectionFaceDone(const BRepBuilderAPI_MakeFace& mkFace)
    {
        if (!mkFace.IsDone()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("Failed to build face for checking intersection");
            }
            return false;
        }
        return true;
    }

    void checkIntersection(const EdgeInfo &info,
                           const EdgeInfo &other,
                           std::set<IntersectInfo> &params1,
                           std::set<IntersectInfo> &params2)
    {
        if(!checkIntersectionPlanar(info, other, params1, params2)){
            return;
        }

        // BRepExtrema_DistShapeShape has trouble finding all solutions for a
        // spline curve. ShapeAnalysis_Wire is better. Besides, it can check
        // for self intersection. It's slightly more troublesome to use, as it
        // requires to build a face for the wire, so we only use it for planar
        // cases.

        IntRes2d_SequenceOfIntersectionPoint points2d;
        TColgp_SequenceOfPnt points3d;
        TColStd_SequenceOfReal errors;
        TopoDS_Wire wire;
        int idx = 0;

        if (!checkIntersectionMakeWire(info, other, idx, wire)){
            return;
        }

        BRepBuilderAPI_MakeFace mkFace(wire);
        if (!checkIntersectionFaceDone(mkFace)) {
            return;
        }

        const TopoDS_Face& face = mkFace.Face();
        ShapeAnalysis_Wire analysis(wire, face, myTol);
        analysis.CheckIntersectingEdges(1, idx, points2d, points3d, errors);

        // Originally here there was a call to the precompiler macro assertCheck(), which has been
        // replaced with the precompiler macro assert()

        assert(points2d.Length() == points3d.Length());
        for (int i=1; i<=points2d.Length(); ++i) {
            pushIntersection(params1, points2d(i).ParamOnFirst(), points3d(i), other.edge);
            pushIntersection(params2, points2d(i).ParamOnSecond(), points3d(i), info.edge);
        }
    }

    void pushIntersection(std::set<IntersectInfo>& params,
                          double param,
                          const gp_Pnt& pt,
                          const TopoDS_Shape& shape)
    {
        IntersectInfo info(param, pt, shape);
        auto it = params.upper_bound(info);
        if (it != params.end()) {
            if (it->point.SquareDistance(pt) < myTol2) {
                return;
            }
        }
        if (it != params.begin()) {
            auto itPrev = it;
            --itPrev;
            if (itPrev->point.SquareDistance(pt) < myTol2) {
                return;
            }
        }
        params.insert(it, info);
    }

    struct SplitInfo {
        TopoDS_Edge edge;
        TopoDS_Shape intersectShape;
        Box bbox;
    };

    // This method was originally part of WireJoinerP::splitEdges(), split to reduce cognitive
    // complexity
    void splitEdgesMakeEdge(const std::set<IntersectInfo>::iterator& itParam,
                            const EdgeInfo& info,
                            std::vector<SplitInfo>& splits,
                            std::set<IntersectInfo>::iterator& itPrevParam,
                            const TopoDS_Shape& intersectShape)
    {
        // Using points cause MakeEdge failure for some reason. Using
        // parameters is better.
        //
        const gp_Pnt& p1 = itPrevParam->point;
        const gp_Pnt& p2 = itParam->point;
        const Standard_Real& param1 = itPrevParam->param;
        const Standard_Real& param2 = itParam->param;

        BRepBuilderAPI_MakeEdge mkEdge(info.curve, param1, param2);
        if (mkEdge.IsDone()) {
            splits.emplace_back();
            auto& entry = splits.back();
            entry.edge = mkEdge.Edge();
            entry.intersectShape = intersectShape;
            if (getBBox(entry.edge, entry.bbox)) {
                itPrevParam = itParam;
            }
            else {
                splits.pop_back();
            }
        }
        else if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("edge split failed " << std::setprecision(16) << FC_XYZ(p1) << FC_XYZ(p2)
                                         << ": " << mkEdge.Error());
        }
    }

    // This method was originally part of WireJoinerP::splitEdges(), split to reduce cognitive
    // complexity
    void splitEdgesMakeEdges(std::set<IntersectInfo>::iterator& itParam,
                             const std::set<IntersectInfo>& params,
                             const EdgeInfo& info,
                             std::vector<SplitInfo>& splits)
    {
        for (auto itPrevParam = itParam++; itParam != params.end(); ++itParam) {
            const auto& intersectShape = itParam->intersectShape.IsNull()
                ? itPrevParam->intersectShape
                : itParam->intersectShape;
            if (intersectShape.IsNull()) {
                break;
            }

            splitEdgesMakeEdge(itParam, info, splits, itPrevParam, intersectShape);
        }
    }

    // Try splitting any edges that intersects other edge
    void splitEdges()
    {
        std::unordered_map<const EdgeInfo*, std::set<IntersectInfo>> intersects;

        int idx=0;
        for (auto& info : edges) {
            info.iteration = ++idx;
        }

        std::unique_ptr<Base::SequencerLauncher> seq(
                new Base::SequencerLauncher("Splitting edges", edges.size()));

        idx = 0;
        for (auto& info : edges) {
            seq->next(true);
            ++idx;
            auto &params = intersects[&info];
            checkSelfIntersection(info, params);

            for (auto vit=boxMap.qbegin(bgi::intersects(info.box)); vit!=boxMap.qend(); ++vit) {
                const auto &other = *(*vit);
                if (other.iteration <= idx) {
                    // means the edge is before us, and we've already checked intersection
                    continue;
                }
                checkIntersection(info, other, params, intersects[&other]);
            }
        }

        idx=0;
        std::vector<SplitInfo> splits;
        for (auto it=edges.begin(); it!=edges.end(); ) {
            ++idx;
            auto iter = intersects.find(&(*it));
            if (iter == intersects.end()) {
                ++it;
                continue;
            }
            auto &info = *it;
            auto &params = iter->second;
            if (params.empty()) {
                ++it;
                continue;
            }

            auto itParam = params.begin();
            if (itParam->point.SquareDistance(info.p1) < myTol2) {
                params.erase(itParam);
            }
            params.emplace(info.firstParam, info.p1, TopoDS_Shape());
            itParam = params.end();
            --itParam;
            if (itParam->point.SquareDistance(info.p2) < myTol2) {
                params.erase(itParam);
            }
            params.emplace(info.lastParam, info.p2, TopoDS_Shape());

            if (params.size() <= 2) {
                ++it;
                continue;
            }

            splits.clear();
            itParam = params.begin();

            splitEdgesMakeEdges(itParam, params, info, splits);

            if (splits.size() <= 1) {
                ++it;
                continue;
            }

            showShape(info.edge, "remove");
            auto removedEdge = info.edge;
            it = remove(it);
            for (const auto& split : splits) {
                if (!add(split.edge, false, split.bbox, it)) {
                    continue;
                }
                auto &newInfo = *it++;
                aHistory->AddModified(split.intersectShape, newInfo.edge);
                // if (v.intersectShape != removedEdge)
                //     aHistory->AddModified(removedEdge, newInfo.edge);
                showShape(newInfo.edge, "split");
            }
        }
    }

    // This method was originally part of WireJoinerP::findSuperEdges(), split to reduce cognitive
    // complexity
    void findSuperEdgeFromAdjacent(std::deque<VertexInfo>& vertices, const int direction)
    {
        bool done = false;
        auto begin = direction == 1 ? vertices.back().reversed() : vertices.front();
        while (true) {
            auto currentVertex = direction == 1 ? vertices.front() : vertices.back();
            auto current = currentVertex.edgeInfo();
            // showShape(current, "siter", k);
            const int idx = (currentVertex.start ? 1 : 0) ^ direction;
            EdgeInfo* found = nullptr;
            for (int i = current->iStart[idx]; i < current->iEnd[idx]; ++i) {
                const auto& vertex = adjacentList[i];
                auto next = vertex.edgeInfo();
                if (next->iteration < 0    // skipped
                    || next == current) {  // skip self (see how adjacent list is built)
                    continue;
                }
                if (vertex == begin) {
                    // closed
                    done = true;
                    break;
                }
                if (found                       // more than one branch
                    || edgeSet.contains(next))  // or, self intersect
                {

                    // Originally here there were some lines of code that have been removed
                    // as them are commented out.
                    // See
                    // https://github.com/realthunder/FreeCAD/blob/6f15849be2505f98927e75d0e8352185e14e7b72/src/Mod/Part/App/WireJoiner.cpp#L1141
                    // for reference.

                    found = nullptr;
                    break;
                }
                found = next;
                currentVertex = vertex;
            }
            if (done || !found) {
                break;
            }
            // showShape(found, "snext", k);
            if (direction == 1) {
                edgeSet.insert(current);
                vertices.push_front(currentVertex.reversed());
            }
            else {
                edgeSet.insert(found);
                vertices.push_back(currentVertex);
            }
        }
    }

    // This method was originally part of WireJoinerP::findSuperEdges(), split to reduce cognitive
    // complexity
    void findSuperEdge(std::deque<VertexInfo>& vertices, const Edges::iterator it)
    {
        vertices.clear();
        vertices.emplace_back(it, true);
        edgeSet.clear();

        for (int direction = 0; direction < 2; ++direction) {  // search in both direction
            findSuperEdgeFromAdjacent(vertices, direction);
        }
    }

    // This method was originally part of WireJoinerP::findSuperEdges(), split to reduce cognitive
    // complexity
    void findSuperEdgesUpdateFirst(std::deque<VertexInfo> vertices)
    {
        Bnd_Box bbox;
        for (const auto& vertex : vertices) {
            auto current = vertex.edgeInfo();
            bbox.Add(current->box.min_corner());
            bbox.Add(current->box.max_corner());
            wireData->Add(current->shape(vertex.start));
            showShape(current, "edge");
            current->iteration = -1;
        }

        auto first = vertices.front().edgeInfo();
        first->superEdge = makeCleanWire(false);
        first->superEdgeReversed.Nullify();
        if (BRep_Tool::IsClosed(first->superEdge)) {
            first->iteration = -2;
            showShape(first, "super_done");
        }
        else {
            first->iteration = iteration;
            showShape(first, "super");
            auto& vFirst = vertices.front();
            auto& vLast = vertices.back();
            auto last = vLast.edgeInfo();
            vFirst.ptOther() = vLast.ptOther();
            const int idx = vFirst.start ? 1 : 0;
            first->iStart[idx] = last->iStart[vLast.start ? 1 : 0];
            first->iEnd[idx] = last->iEnd[vLast.start ? 1 : 0];

            for (int i = first->iStart[idx]; i < first->iEnd[idx]; ++i) {
                auto& vertex = adjacentList[i];
                if (vertex.it == vLast.it) {
                    vertex.it = vFirst.it;
                    vertex.start = !vFirst.start;
                }
            }
            bbox.Enlarge(myTol);
            first->box = Box(bbox.CornerMin(), bbox.CornerMax());
        }
    }

    void findSuperEdges()
    {
        std::unique_ptr<Base::SequencerLauncher> seq(
            new Base::SequencerLauncher("Combining edges", edges.size()));

        std::deque<VertexInfo> vertices;

        ++iteration;

        // Join edges (let's call it super edge) that are connected to only one
        // other edges (count == 2 counts this and the other edge) on one of
        // its vertices to save traverse time.
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            seq->next(true);
            auto& info = *it;
            if (info.iteration == iteration || info.iteration < 0) {
                continue;
            }
            info.iteration = iteration;
            // showShape(&info, "scheck");

            findSuperEdge(vertices, it);

            if (vertices.size() <= 1) {
                continue;
            }

            wireData->Clear();

            findSuperEdgesUpdateFirst(vertices);
        }
    }

    void buildAdjacentListPopulate()
    {
        // populate adjacent list
        for (auto& info : edges) {
            if (info.iteration == -2) {

                // Originally there was the following precompiler directive around assertCheck():
                // #if OCC_VERSION_HEX >= 0x070000
                // The precompiler directive has been removed as the minimum OCCT version supported
                // is 7.3.0 and the precompiler macro has been replaced with assert()

                assert(BRep_Tool::IsClosed(info.shape()));

                showShape(&info, "closed");
                if (!doTightBound) {
                    builder.Add(compound, info.wire());
                }
                continue;
            }

            if (info.iteration < 0) {
                continue;
            }

            if (info.p1.SquareDistance(info.p2) <= myTol2) {
                if (!doTightBound) {
                    builder.Add(compound, info.wire());
                }
                info.iteration = -2;
                continue;
            }

            std::array<gp_Pnt, 2> pt {};
            pt[0] = info.p1;
            pt[1] = info.p2;
            for (int i = 0; i < 2; ++i) {
                const int ic = i;
                if (info.iStart[ic] >= 0) {
                    continue;
                }
                info.iEnd[ic] = info.iStart[ic] = (int)adjacentList.size();

                constexpr int max = std::numeric_limits<int>::max();
                for (auto vit = vmap.qbegin(bgi::nearest(pt[ic], max)); vit != vmap.qend();
                     ++vit) {
                    auto& vinfo = *vit;
                    if (vinfo.pt().SquareDistance(pt[ic]) > myTol2) {
                        break;
                    }

                    // We must push ourself too, because the adjacency
                    // information is shared among all connected edges.
                    //
                    // if (&(*vinfo.it) == &info)
                    //     continue;

                    if (vinfo.it->iteration < 0) {
                        continue;
                    }

                    adjacentList.push_back(vinfo);
                    ++info.iEnd[ic];
                }

                // copy the adjacent indices to all connected edges
                for (int j = info.iStart[ic]; j < info.iEnd[ic]; ++j) {
                    auto& other = adjacentList[j];
                    auto& otherInfo = *other.it;
                    if (&otherInfo != &info) {
                        const int kc = other.start ? 0 : 1;
                        otherInfo.iStart[kc] = info.iStart[ic];
                        otherInfo.iEnd[kc] = info.iEnd[ic];
                    }
                }
            }
        }
    }

    void buildAdjacentListSkipEdges()
    {
        bool done = false;
        while (!done) {
            done = true;

            if (doMergeEdge || doTightBound) {
                findSuperEdges();
            }

            // Skip edges that are connected to only one end
            for (auto& info : edges) {
                if (info.iteration < 0) {
                    continue;
                }
                for (int k = 0; k < 2; ++k) {
                    const int kc = k;
                    int idx = 0;
                    for (idx = info.iStart[kc]; idx < info.iEnd[kc]; ++idx) {
                        const auto& vertex = adjacentList[idx];
                        auto other = vertex.edgeInfo();
                        if (other->iteration >= 0 && other != &info) {
                            break;
                        }
                    }
                    if (idx == info.iEnd[kc]) {
                        // If merge or tight bound, then repeat until no edges
                        // can be skipped.
                        done = !doMergeEdge && !doTightBound;
                        info.iteration = -3;
                        showShape(&info, "skip");
                        break;
                    }
                }
            }
        }
    }

    void buildAdjacentList()
    {
        builder.MakeCompound(compound);

        for (auto& info : edges) {
            info.reset();
        }

        adjacentList.clear();

        buildAdjacentListPopulate();

        buildAdjacentListSkipEdges();
    }

    // This algorithm tries to find a set of closed wires that includes as many
    // edges (added by calling add() ) as possible. One edge may be included
    // in more than one closed wires if it connects to more than one edges.
    void findClosedWires(bool tightBound=false)
    {
        std::unique_ptr<Base::SequencerLauncher> seq(
                new Base::SequencerLauncher("Finding wires", edges.size()));

        for (auto &info : edges) {
            info.wireInfo.reset();
            info.wireInfo2.reset();
        }

        for (auto it=edges.begin(); it!=edges.end(); ++it) {
            VertexInfo beginVertex(it, true);
            auto &beginInfo = *it;
            seq->next(true);
            ++iteration;
            if (beginInfo.iteration < 0 || beginInfo.wireInfo) {
                continue;
            }

            VertexInfo currentVertex(it, true);
            EdgeInfo *currentInfo = &beginInfo;
            showShape(currentInfo, "begin");
            stack.clear();
            vertexStack.clear();
            edgeSet.clear();

            TopoDS_Wire wire = _findClosedWires(beginVertex, currentVertex);
            if (wire.IsNull()) {
                continue;
            }

            if (tightBound) {

                // Originally here there was a call to the precompiler macro assertCheck(), which
                // has been replaced with the precompiler macro assert()

                assert(!beginInfo.wireInfo);
                beginInfo.wireInfo.reset(new WireInfo());
                beginInfo.wireInfo->vertices.emplace_back(it, true);
                beginInfo.wireInfo->wire = wire;
            }
            for (auto &entry : stack) {
                const auto &vertex = vertexStack[entry.iCurrent];
                auto &info = *vertex.it;
                if (tightBound) {
                    beginInfo.wireInfo->vertices.push_back(vertex);
                }
                if (!info.wireInfo) {
                    info.wireInfo = beginInfo.wireInfo;
                    // showShape(&info, "visited");
                }
            }
            showShape(wire,"joined");
            if (!tightBound) {
                builder.Add(compound, wire);
            }
        }
    }

    // Originally here there was the definition of the method checkStack(), which does nothing and
    // therefore has been removed. See
    // https://github.com/realthunder/FreeCAD/blob/6f15849be2505f98927e75d0e8352185e14e7b72/src/Mod/Part/App/WireJoiner.cpp#L1366
    // for reference

    void checkWireInfo(const WireInfo &wireInfo)
    {
        (void)wireInfo;
        if (FC_LOG_INSTANCE.level() <= FC_LOGLEVEL_TRACE) {
            return;
        }
        for (auto &info : edges) {
            if (auto wire = info.wireInfo.get()) {
                boost::ignore_unused(wire);

                // Originally here there was a call to the precompiler macro assertCheck(), which
                // has been replaced with the precompiler macro assert()

                assert(wire->vertices.front().edgeInfo()->wireInfo.get() == wire);
            }
        }
    }

    // This method was originally part of WireJoinerP::_findClosedWires(), split to reduce cognitive
    // complexity
    void _findClosedWiresBeginEdge(const std::shared_ptr<WireInfo>& wireInfo,
                                   const EdgeInfo& beginInfo,
                                   int& idx)
    {
        auto info = wireInfo->vertices[idx].edgeInfo();
        showShape(info, "merge", iteration);

        if (info != &beginInfo) {
            while (true) {
                if (++idx == (int)wireInfo->vertices.size()) {
                    idx = 0;
                }

                info = wireInfo->vertices[idx].edgeInfo();
                if (info == &beginInfo) {
                    break;
                }
                stack.emplace_back(vertexStack.size());
                vertexStack.push_back(wireInfo->vertices[idx]);
                ++stack.back().iEnd;

                // Originally here there was a call to the method checkStack(),
                // which does nothing and therefore has been removed.
            }
        }
    }

    // This method was originally part of WireJoinerP::_findClosedWires(), split to reduce cognitive
    // complexity
    int _findClosedWiresWithExisting(int* idxVertex,
                                     const std::shared_ptr<WireInfo>& wireInfo,
                                     int* const stackPos,
                                     bool& proceed,
                                     const VertexInfo& vinfo,
                                     const int ic,
                                     StackInfo& stackBack,
                                     const EdgeInfo& beginInfo,
                                     EdgeInfo& info)
    {
        if (wireInfo) {
            // We may be called by findTightBound() with an existing wire
            // to try to find a new wire by splitting the current one. So
            // check if we've iterated to some edge in the existing wire.

            int idx = wireInfo->find(vinfo);

            if (idx != 0) {
                vertexStack.push_back(adjacentList[ic]);
                stackBack.iCurrent = stackBack.iEnd++;
                --idx;
                proceed = false;
                if (idxVertex) {
                    *idxVertex = idx;
                }
                if (stackPos) {
                    *stackPos = (int)stack.size() - 2;
                }

                _findClosedWiresBeginEdge(wireInfo, beginInfo, idx);

                return 1;
            }

            if (wireInfo->find(VertexInfo(vinfo.it, !vinfo.start)) != 0) {
                showShape(&info, "rintersect", iteration);
                // Only used when exhausting tight bound.
                wireInfo->purge = true;
                return 2;
            }

            if (isOutside(*wireInfo, info.mid)) {
                showShape(&info, "outside", iteration);
                return 2;
            }
        }
        return 0;
    }

    // This method was originally part of WireJoinerP::_findClosedWires(), split to reduce cognitive
    // complexity
    void _findClosedWiresUpdateStack(int* idxVertex,
                                     const std::shared_ptr<WireInfo>& wireInfo,
                                     int* stackPos,
                                     const EdgeInfo* currentInfo,
                                     const int currentIdx,
                                     bool& proceed,
                                     const EdgeInfo& beginInfo)
    {
        auto& stackBack = stack.back();

        // The loop below is to find all edges connected to pend, and save them into stack.back()
        auto size = vertexStack.size();
        for (int i = currentInfo->iStart[currentIdx]; i < currentInfo->iEnd[currentIdx]; ++i) {
            auto& vinfo = adjacentList[i];
            auto& info = *vinfo.it;
            if (info.iteration < 0 || currentInfo == &info) {
                continue;
            }

            bool abort = false;
            if (!wireSet.empty() && wireSet.contains(info.wireInfo.get())) {
                showShape(&info, "wired", iteration);
                if (wireInfo) {
                    wireInfo->purge = true;
                }
                abort = true;
            }

            if (edgeSet.contains(&info)) {
                showShape(&info, "intersect", iteration);
                // This means the current edge connects to an
                // existing edge in the middle of the stack.
                // skip the current edge.
                stackBack.iEnd = stackBack.iStart;
                vertexStack.resize(size);
                break;
            }

            if (abort || currentInfo->wireInfo2) {
                if (wireInfo) {
                    wireInfo->purge = true;
                }
                continue;
            }

            if (info.iteration == iteration) {
                continue;
            }
            info.iteration = iteration;

            int exists = _findClosedWiresWithExisting(idxVertex,
                                                      wireInfo,
                                                      stackPos,
                                                      proceed,
                                                      vinfo,
                                                      i,
                                                      stackBack,
                                                      beginInfo,
                                                      info);

            if (exists == 1) {
                break;
            }

            if (exists == 2) {
                continue;
            }

            vertexStack.push_back(adjacentList[i]);
            ++stackBack.iEnd;
        }
    }

    // This method was originally part of WireJoinerP::_findClosedWires(), split to reduce cognitive
    // complexity
    bool _findClosedWiresUpdateEdges(VertexInfo& currentVertex,
                                     gp_Pnt& pend,
                                     EdgeInfo* currentInfo,
                                     int& currentIdx,
                                     const size_t stackEnd)
    {
        while (true) {
            auto& stackBack = stack.back();
            if (stackBack.iCurrent < stackBack.iEnd) {
                // now pick one edge from stack.back(), connect it to
                // pend, then extend pend
                currentVertex = vertexStack[stackBack.iCurrent];
                pend = currentVertex.ptOther();
                // update current edge info
                currentInfo = currentVertex.edgeInfo();
                showShape(currentInfo, "iterate", iteration);
                currentIdx = currentVertex.start ? 1 : 0;
                edgeSet.insert(currentInfo);
                if (!wireSet.empty()) {
                    wireSet.insert(currentInfo->wireInfo.get());
                }
                break;
            }
            vertexStack.erase(vertexStack.begin() + static_cast<long>(stackBack.iStart), vertexStack.end());

            stack.pop_back();
            if (stack.size() == stackEnd) {
                // If stack reaches the end, it means this wire is open.
                return true;
            }

            auto& lastInfo = *vertexStack[stack.back().iCurrent].it;
            edgeSet.erase(&lastInfo);
            wireSet.erase(lastInfo.wireInfo.get());
            showShape(&lastInfo, "pop", iteration);
            ++stack.back().iCurrent;
        }
        return false;
    }

    // This method was originally part of WireJoinerP::_findClosedWires(), split to reduce cognitive
    // complexity
    bool _findClosedWiresIsClosed(const VertexInfo& beginVertex,
                                  const TopoDS_Wire& wire,
                                  const EdgeInfo& beginInfo)
    {
        if (!BRep_Tool::IsClosed(wire)) {
            FC_WARN("failed to close some wire in iteration " << iteration);
            showShape(wire, "_FailedToClose", iteration);
            showShape(beginInfo.shape(beginVertex.start), "failed", iteration);
            for (auto& entry : stack) {
                const auto& vertex = vertexStack[entry.iCurrent];
                auto& info = *vertex.it;
                showShape(info.shape(vertex.start), vertex.start ? "failed" : "failed_r", iteration);
            }

            // Originally here there was a call to the precompiler macro assertCheck(), which
            // has been replaced with the precompiler macro assert()

            assert(false);
            return false;
        }
        return true;
    }

    TopoDS_Wire _findClosedWires(VertexInfo beginVertex,
                                 VertexInfo currentVertex,
                                 int *idxVertex = nullptr,
                                 const std::shared_ptr<WireInfo>& wireInfo = std::shared_ptr<WireInfo>(),
                                 int* stackPos = nullptr)
    {
        Base::SequencerBase::Instance().checkAbort();
        EdgeInfo &beginInfo = *beginVertex.it;

        EdgeInfo *currentInfo = currentVertex.edgeInfo();
        int currentIdx = currentVertex.start ? 1 : 0;
        currentInfo->iteration = iteration;

        gp_Pnt pstart = beginVertex.pt();
        gp_Pnt pend = currentVertex.ptOther();

        auto stackEnd = stack.size();

        // Originally here there was a call to the method checkStack(), which does nothing and
        // therefore has been removed.

        // pstart and pend is the start and end vertex of the current wire
        while (true) {
            // push a new stack entry
            stack.emplace_back(vertexStack.size());
            showShape(currentInfo, "check", iteration);

            bool proceed = true;

            _findClosedWiresUpdateStack(idxVertex,
                                        wireInfo,
                                        stackPos,
                                        currentInfo,
                                        currentIdx,
                                        proceed,
                                        beginInfo);

            // Originally here there was a call to the method checkStack(), which does nothing and
            // therefore has been removed.

            if (proceed) {
                if (_findClosedWiresUpdateEdges(currentVertex,
                                                pend,
                                                currentInfo,
                                                currentIdx,
                                                stackEnd)) {
                    return {};
                }

                if (pstart.SquareDistance(pend) > myTol2) {
                    // if the wire is not closed yet, continue search for the
                    // next connected edge
                    continue;
                }
                if (wireInfo) {
                    if (idxVertex) {
                        *idxVertex = (int)wireInfo->vertices.size();
                    }
                    if (stackPos) {
                        *stackPos = (int)stack.size() - 1;
                    }
                }
            }

            wireData->Clear();
            wireData->Add(beginInfo.shape(beginVertex.start));
            for (auto &entry : stack) {
                const auto &vertex = vertexStack[entry.iCurrent];
                auto &info = *vertex.it;
                wireData->Add(info.shape(vertex.start));
            }
            TopoDS_Wire wire = makeCleanWire();
            if (!_findClosedWiresIsClosed(beginVertex, wire, beginInfo)) {
                continue;
            }
            return wire;
        }
    }

    // This method was originally part of WireJoinerP::findTightBound(), split to reduce cognitive
    // complexity
    void findTightBoundSplitWire(const std::shared_ptr<WireInfo>& wireInfo,
                                 const EdgeInfo& beginInfo,
                                 const std::vector<VertexInfo>& wireVertices,
                                 std::shared_ptr<WireInfo>& splitWire,
                                 const int idxV,
                                 int& idxStart,
                                 const int idxEnd)
    {
        for (int idx = idxV; idx != idxEnd; ++idx) {
            auto info = wireVertices[idx].edgeInfo();
            if (info == &beginInfo) {
                showShape(*wireInfo, "exception", iteration, true);
                showShape(info, "exception", iteration, true);

                // Originally here there was a call to the precompiler macro
                // assertCheck(), which has been replaced with the precompiler macro
                // assert()

                assert(info != &beginInfo);
            }
            if (info->wireInfo == wireInfo) {
                if (!splitWire) {
                    idxStart = idx;
                    splitWire.reset(new WireInfo());
                }
                info->wireInfo = splitWire;
            }
        }
    }

    // This method was originally part of WireJoinerP::findTightBound(), split to reduce cognitive
    // complexity
    void findTightBoundWithSplit(const std::vector<VertexInfo>& wireVertices,
                                 const int idxV,
                                 const std::shared_ptr<WireInfo>& splitWire,
                                 const int idxStart,
                                 const int idxEnd,
                                 const int stackPos,
                                 const int stackStart)
    {
        auto& splitEdges = splitWire->vertices;
        gp_Pnt pstart;
        gp_Pnt pt;
        bool first = true;
        for (int idx = idxStart; idx != idxEnd; ++idx) {
            auto& vertex = wireVertices[idx];
            if (first) {
                first = false;
                pstart = vertex.pt();
            }
            else {

                // Originally here there was a call to the precompiler macro
                // assertCheck(), which has been replaced with the precompiler
                // macro assert()

                assert(pt.SquareDistance(vertex.pt()) < myTol2);
            }
            pt = vertex.ptOther();
            splitEdges.push_back(vertex);
        }
        for (int i = stackPos; i >= stackStart; --i) {
            const auto& vertex = vertexStack[stack[i].iCurrent];

            // Originally here there was a call to the precompiler macro
            // assertCheck(), which has been replaced with the precompiler macro
            // assert()

            assert(pt.SquareDistance(vertex.ptOther()) < myTol2);
            pt = vertex.pt();
            // The edges in the stack are the ones to slice
            // the wire in half. We construct a new wire
            // that includes the original beginning edge in
            // the loop above. And this loop contains the
            // other half. Note that the slicing edges
            // should run in the oppsite direction, hence reversed
            splitEdges.push_back(vertex.reversed());
        }
        for (int idx = idxV; idx != idxStart; ++idx) {
            auto& vertex = wireVertices[idx];

            // Originally here there was a call to the precompiler macro
            // assertCheck(), which has been replaced with the precompiler macro
            // assert()

            assert(pt.SquareDistance(vertex.pt()) < myTol2);
            pt = vertex.ptOther();
            splitEdges.push_back(vertex);
        }

        // Originally here there was a call to the precompiler macro
        // assertCheck(), which has been replaced with the precompiler macro
        // assert()

        assert(pt.SquareDistance(pstart) < myTol2);
        showShape(*splitWire, "swire", iteration);
    }

    // This method was originally part of WireJoinerP::findTightBound(), split to reduce cognitive
    // complexity
    void findTightBoundByVertices(EdgeInfo& beginInfo,
                                  const std::vector<VertexInfo>& wireVertices,
                                  int& idxV,
                                  const int iteration2,
                                  const gp_Pnt& pstart,
                                  const std::shared_ptr<WireInfo>& wireInfo,
                                  const VertexInfo& beginVertex,
                                  std::shared_ptr<WireInfo>& newWire)
    {
        const int idx = wireVertices[idxV].start ? 1 : 0;

        auto current = wireVertices[idxV].edgeInfo();
        showShape(current, "current", iteration);

        for (int vertex = current->iStart[idx]; vertex < current->iEnd[idx]; ++vertex) {
            const auto& currentVertex = adjacentList[vertex];
            auto next = currentVertex.edgeInfo();
            if (next == current || next->iteration2 == iteration2 || next->iteration < 0) {
                continue;
            }

            showShape(next, "tcheck", iteration);

            if (!isInside(*wireInfo, next->mid)) {
                showShape(next, "ninside", iteration);
                next->iteration2 = iteration2;
                continue;
            }

            edgeSet.insert(next);
            stack.emplace_back(vertexStack.size());
            ++stack.back().iEnd;
            vertexStack.push_back(currentVertex);

            // Originally here there was a call to the method checkStack(), which does
            // nothing and therefore has been removed.

            int idxEnd = (int)wireVertices.size();
            int stackStart = (int)stack.size() - 1;
            int stackPos = (int)stack.size() - 1;

            TopoDS_Wire wire;
            if (pstart.SquareDistance(currentVertex.ptOther()) > myTol2) {
                wire = _findClosedWires(beginVertex,
                                        currentVertex,
                                        &idxEnd,
                                        beginInfo.wireInfo,
                                        &stackPos);
                if (wire.IsNull()) {
                    vertexStack.pop_back();
                    stack.pop_back();
                    edgeSet.erase(next);
                    continue;
                }
            }

            newWire.reset(new WireInfo());
            auto& newWireVertices = newWire->vertices;
            newWireVertices.push_back(beginVertex);
            for (auto& entry : stack) {
                const auto& vertex = vertexStack[entry.iCurrent];
                newWireVertices.push_back(vertex);
            }
            if (!wire.IsNull()) {
                newWire->wire = wire;
            }
            else if (!initWireInfo(*newWire)) {
                newWire.reset();
                vertexStack.pop_back();
                stack.pop_back();
                edgeSet.erase(next);
                continue;
            }
            for (auto& vertex : newWire->vertices) {
                if (vertex.edgeInfo()->wireInfo == wireInfo) {
                    vertex.edgeInfo()->wireInfo = newWire;
                }
            }
            beginInfo.wireInfo = newWire;
            showShape(*newWire, "nwire", iteration);

            std::shared_ptr<WireInfo> splitWire;
            if (idxEnd == 0) {
                idxEnd = (int)wireVertices.size();
            }
            ++idxV;

            // Originally here there was a call to the precompiler macro assertCheck(),
            // which has been replaced with the precompiler macro assert()

            assert(idxV <= idxEnd);
            int idxStart = idxV;

            findTightBoundSplitWire(wireInfo,
                                    beginInfo,
                                    wireVertices,
                                    splitWire,
                                    idxV,
                                    idxStart,
                                    idxEnd);

            if (splitWire) {
                findTightBoundWithSplit(wireVertices,
                                        idxV,
                                        splitWire,
                                        idxStart,
                                        idxEnd,
                                        stackPos,
                                        stackStart);
            }

            checkWireInfo(*newWire);
            break;
        }
    }

    // This method was originally part of WireJoinerP::findTightBound(), split to reduce cognitive
    // complexity
    void findTightBoundUpdateVertices(EdgeInfo& beginInfo)
    {
        showShape(*beginInfo.wireInfo, "done", iteration);
        beginInfo.wireInfo->done = true;
        // If a wire is done, make sure all edges of this wire is
        // marked as done. This can also prevent duplicated wires.
        for (auto& vertex : beginInfo.wireInfo->vertices) {
            auto info = vertex.edgeInfo();
            if (!info->wireInfo) {
                info->wireInfo = beginInfo.wireInfo;
                continue;
            }
            if (info->wireInfo->done) {
                continue;
            }
            auto otherWire = info->wireInfo;
            auto& otherWireVertices = info->wireInfo->vertices;
            if (info == otherWireVertices.front().edgeInfo()) {
                // About to change the first edge of the other wireInfo.
                // Try to find a new first edge for it.
                tmpVertices.clear();
                auto it = otherWireVertices.begin();
                tmpVertices.push_back(*it);
                for (++it; it != otherWireVertices.end(); ++it) {
                    if (it->edgeInfo()->wireInfo == otherWire) {
                        break;
                    }
                    tmpVertices.push_back(*it);
                }
                if (tmpVertices.size() != otherWireVertices.size()) {
                    otherWireVertices.erase(otherWireVertices.begin(), it);
                    otherWireVertices.insert(otherWireVertices.end(),
                                             tmpVertices.begin(),
                                             tmpVertices.end());
                }
            }

            // Originally here there was a call to the precompiler macro assertCheck(),
            // which has been replaced with the precompiler macro assert()

            assert(info != &beginInfo);
            info->wireInfo = beginInfo.wireInfo;
            checkWireInfo(*otherWire);
        }
        checkWireInfo(*beginInfo.wireInfo);
    }

    void findTightBound()
    {
        // Assumption: all edges lies on a common manifold surface
        //
        // Definition of 'Tight Bound': a wire that cannot be split into
        // smaller wires by any intersecting edges internal to the wire.
        //
        // The idea of the searching algorithm is simple. The initial condition
        // here is that we've found a closed wire for each edge. To find the
        // tight bound, for each wire, check wire edge branches (using the
        // adjacent list built earlier), and split the wire whenever possible.

        std::unique_ptr<Base::SequencerLauncher> seq(
                new Base::SequencerLauncher("Finding tight bound", edges.size()));

        int iteration2 = iteration;
        for (auto &info : edges) {
            ++iteration;
            seq->next(true);
            if (info.iteration < 0 || !info.wireInfo) {
                continue;
            }

            ++iteration2;
            while(!info.wireInfo->done) {
                auto wireInfo = info.wireInfo;
                checkWireInfo(*wireInfo);
                const auto &wireVertices = wireInfo->vertices;
                auto beginVertex = wireVertices.front();
                auto &beginInfo = *beginVertex.it;
                initWireInfo(*wireInfo);
                showShape(wireInfo->wire, "iwire", iteration);
                for (auto& vertex : wireVertices) {
                    vertex.it->iteration2 = iteration2;
                }

                stack.clear();
                vertexStack.clear();
                edgeSet.clear();

                std::shared_ptr<WireInfo> newWire;
                gp_Pnt pstart = beginVertex.pt();

                int idxV = 0;
                while (true) {
                    findTightBoundByVertices(beginInfo,
                                             wireVertices,
                                             idxV,
                                             iteration2,
                                             pstart,
                                             wireInfo,
                                             beginVertex,
                                             newWire);

                    if (newWire) {
                        ++iteration;
                        break;
                    }

                    if (++idxV == (int)wireVertices.size()) {
                        break;
                    }

                    stack.emplace_back(vertexStack.size());
                    ++stack.back().iEnd;
                    vertexStack.push_back(wireVertices[idxV]);
                    edgeSet.insert(wireVertices[idxV].edgeInfo());

                    // Originally here there was a call to the method checkStack(), which does
                    // nothing and therefore has been removed.
                }

                if (!newWire) {
                    findTightBoundUpdateVertices(beginInfo);
                }
            }
        }
    }

    // This method was originally part of WireJoinerP::exhaustTightBound(), split to reduce cognitive
    // complexity
    void exhaustTightBoundUpdateVertex(const int iteration2,
                                       const VertexInfo& beginVertex,
                                       const int idxV,
                                       const gp_Pnt& pstart,
                                       const std::vector<VertexInfo>& wireVertices,
                                       std::shared_ptr<WireInfo>& newWire,
                                       const std::shared_ptr<WireInfo>& wireInfo)
    {
        const int idx = wireVertices[idxV].start ? 1 : 0;
        auto current = wireVertices[idxV].edgeInfo();

        for (int vertex = current->iStart[idx]; vertex < current->iEnd[idx]; ++vertex) {
            const auto& currentVertex = adjacentList[vertex];
            auto next = currentVertex.edgeInfo();
            if (next == current || next->iteration2 == iteration2 || next->iteration < 0) {
                continue;
            }

            showShape(next, "check2", iteration);

            if (!isInside(*wireInfo, next->mid)) {
                showShape(next, "ninside2", iteration);
                next->iteration2 = iteration2;
                continue;
            }

            edgeSet.insert(next);
            stack.emplace_back(vertexStack.size());
            ++stack.back().iEnd;
            vertexStack.push_back(currentVertex);

            // Originally here there a call to the method checkStack(), which
            // does nothing and therefore has been removed.

            TopoDS_Wire wire;
            if (pstart.SquareDistance(currentVertex.ptOther()) > myTol2) {
                wire = _findClosedWires(beginVertex, currentVertex, nullptr, wireInfo);
                if (wire.IsNull()) {
                    vertexStack.pop_back();
                    stack.pop_back();
                    edgeSet.erase(next);
                    wireSet.erase(next->wireInfo.get());
                    continue;
                }
            }

            newWire.reset(new WireInfo());
            auto& newWireVertices = newWire->vertices;
            newWireVertices.push_back(beginVertex);
            for (auto& entry : stack) {
                const auto& vertex = vertexStack[entry.iCurrent];
                newWireVertices.push_back(vertex);
            }
            if (!wire.IsNull()) {
                newWire->wire = wire;
            }
            else if (!initWireInfo(*newWire)) {
                newWire.reset();
                vertexStack.pop_back();
                stack.pop_back();
                edgeSet.erase(next);
                wireSet.erase(next->wireInfo.get());
                continue;
            }
            for (auto& vertex : newWire->vertices) {
                if (vertex.edgeInfo()->wireInfo == wireInfo) {
                    vertex.edgeInfo()->wireInfo = newWire;
                }
            }
            showShape(*newWire, "nwire2", iteration);
            checkWireInfo(*newWire);
            break;
        }
    }

    // This method was originally part of WireJoinerP::exhaustTightBound(), split to reduce cognitive
    // complexity
    void exhaustTightBoundUpdateEdge(const int iteration2,
                                     const VertexInfo& beginVertex,
                                     const std::vector<VertexInfo>& wireVertices,
                                     const gp_Pnt& pstart,
                                     std::shared_ptr<WireInfo>& wireInfo)
    {
        std::shared_ptr<WireInfo> newWire;

        int idxV = 1;
        while (true) {
            exhaustTightBoundUpdateVertex(iteration2,
                                          beginVertex,
                                          idxV,
                                          pstart,
                                          wireVertices,
                                          newWire,
                                          wireInfo);

            if (newWire) {
                ++iteration;
                wireInfo = newWire;
                break;
            }

            if (++idxV == (int)wireVertices.size()) {
                if (wireInfo->purge) {
                    showShape(*wireInfo, "discard2", iteration);
                    wireInfo.reset();
                }
                else {
                    wireInfo->done = true;
                    showShape(*wireInfo, "done2", iteration);
                }
                break;
            }
            stack.emplace_back(vertexStack.size());
            ++stack.back().iEnd;
            vertexStack.push_back(wireVertices[idxV]);
            edgeSet.insert(wireVertices[idxV].edgeInfo());

            // Originally here there a call to the method checkStack(), which does
            // nothing and therefore has been removed.
        }
    }

    // This method was originally part of WireJoinerP::exhaustTightBound(), split to reduce cognitive
    // complexity
    void exhaustTightBoundWithAdjacent(const EdgeInfo& info,
                                       int& iteration2,
                                       const VertexInfo beginVertex,
                                       const EdgeInfo* check)
    {
        const gp_Pnt& pstart = beginVertex.pt();
        const int vidx = beginVertex.start ? 1 : 0;

        edgeSet.clear();
        vertexStack.clear();
        stack.clear();
        stack.emplace_back();
        for (int i = info.iStart[vidx]; i < info.iEnd[vidx]; ++i) {
            const auto& currentVertex = adjacentList[i];
            auto next = currentVertex.edgeInfo();
            if (next == &info || next == check || next->iteration < 0 || !next->wireInfo
                || !next->wireInfo->done || next->wireInfo2) {
                continue;
            }

            showShape(next, "n2", iteration);

            stack.clear();
            stack.emplace_back();
            ++stack.back().iEnd;
            vertexStack.clear();
            vertexStack.push_back(currentVertex);

            edgeSet.clear();
            edgeSet.insert(next);
            wireSet.clear();
            wireSet.insert(next->wireInfo.get());

            TopoDS_Wire wire;
            if (pstart.SquareDistance(currentVertex.ptOther()) > myTol2) {
                wire = _findClosedWires(beginVertex, currentVertex);
                if (wire.IsNull()) {
                    continue;
                }
            }

            std::shared_ptr<WireInfo> wireInfo(new WireInfo());
            wireInfo->vertices.push_back(beginVertex);
            for (auto& entry : stack) {
                const auto& vertex = vertexStack[entry.iCurrent];
                wireInfo->vertices.push_back(vertex);
            }
            if (!wire.IsNull()) {
                wireInfo->wire = wire;
            }
            else if (!initWireInfo(*wireInfo)) {
                continue;
            }

            showShape(*wireInfo, "nw2", iteration);

            ++iteration;
            ++iteration2;

            while (wireInfo && !wireInfo->done) {
                showShape(next, "next2", iteration);

                vertexStack.resize(1);
                stack.resize(1);
                edgeSet.clear();
                edgeSet.insert(next);
                wireSet.clear();
                wireSet.insert(next->wireInfo.get());

                const auto& wireVertices = wireInfo->vertices;
                initWireInfo(*wireInfo);
                for (auto& vertex : wireVertices) {
                    vertex.it->iteration2 = iteration2;
                }

                exhaustTightBoundUpdateEdge(iteration2,
                                            beginVertex,
                                            wireVertices,
                                            pstart,
                                            wireInfo);
            }

            if (wireInfo && wireInfo->done) {
                for (auto& vertex : wireInfo->vertices) {
                    auto edgeInfo = vertex.edgeInfo();

                    // Originally here there was a call to the precompiler macro
                    // assertCheck(), which has been replaced with the precompiler macro
                    // assert()

                    assert(edgeInfo->wireInfo != nullptr);
                    if (edgeInfo->wireInfo->isSame(*wireInfo)) {
                        wireInfo = edgeInfo->wireInfo;
                        break;
                    }
                }
                for (auto& vertex : wireInfo->vertices) {
                    auto edgeInfo = vertex.edgeInfo();
                    if (!edgeInfo->wireInfo2 && edgeInfo->wireInfo != wireInfo) {
                        edgeInfo->wireInfo2 = wireInfo;
                    }
                }

                // Originally here there were two calls to the precompiler macro
                // assertCheck(), which have been replaced with the precompiler macro
                // assert()

                assert(info.wireInfo2 == wireInfo);
                assert(info.wireInfo2 != info.wireInfo);
                showShape(*wireInfo, "exhaust");
                break;
            }
        }
    }

    // This method was originally part of WireJoinerP::exhaustTightBound(), split to reduce cognitive
    // complexity
    void exhaustTightBoundUpdateWire(const EdgeInfo& info, int& iteration2)
    {

        showShape(*info.wireInfo, "iwire2", iteration);
        showShape(&info, "begin2", iteration);

        int idx = info.wireInfo->find(&info);

        // Originally here there was a call to the precompiler macro assertCheck(), which has
        // been replaced with the precompiler macro assert()

        assert(idx > 0);
        const auto& vertices = info.wireInfo->vertices;
        --idx;
        int nextIdx = idx == (int)vertices.size() - 1 ? 0 : idx + 1;
        int prevIdx = idx == 0 ? (int)vertices.size() - 1 : idx - 1;
        int count = prevIdx == nextIdx ? 1 : 2;
        for (int idxV = 0; idxV < count && !info.wireInfo2; ++idxV) {
            auto check = vertices[idxV == 0 ? nextIdx : prevIdx].edgeInfo();
            auto beginVertex = vertices[idx];
            if (idxV == 1) {
                beginVertex.start = !beginVertex.start;
            }

            exhaustTightBoundWithAdjacent(info, iteration2, beginVertex, check);
        }
    }

    void exhaustTightBound()
    {
        // findTightBound() function will find a tight bound wire for each
        // edge. Now we try to find all possible tight bound wires, relying on
        // the important fact that an edge can be shared by at most two tight
        // bound wires.

        std::unique_ptr<Base::SequencerLauncher> seq(
                new Base::SequencerLauncher("Exhaust tight bound", edges.size()));

        for (auto &info : edges) {
            if (info.iteration < 0 || !info.wireInfo || !info.wireInfo->done) {
                continue;
            }
            for (auto &vertex : info.wireInfo->vertices) {
                auto edgeInfo = vertex.edgeInfo();
                if (edgeInfo->wireInfo != info.wireInfo) {
                    edgeInfo->wireInfo2 = info.wireInfo;
                }
            }
        }

        int iteration2 = iteration;
        for (auto &info : edges) {
            ++iteration;
            seq->next(true);
            if (info.iteration < 0
                    || !info.wireInfo
                    || !info.wireInfo->done)
            {
                if (info.wireInfo) {
                    showShape(*info.wireInfo, "iskip");
                }
                else {
                    showShape(&info, "iskip");
                }
                continue;
            }

            if (info.wireInfo2 && info.wireInfo2->done) {
                showShape(*info.wireInfo, "idone");
                continue;
            }

            exhaustTightBoundUpdateWire(info, iteration2);

        }
        wireSet.clear();
    }

    // This method was originally part of WireJoinerP::makeCleanWire(), split to reduce cognitive
    // complexity
    void printHistoryInit(const Handle_BRepTools_History& newHistory,
                          const std::vector<TopoShape>& inputEdges)
    {
        FC_MSG("init:");
        for (const auto& shape : sourceEdges) {
#if OCC_VERSION_HEX < 0x070800
            constexpr int max = std::numeric_limits<int>::max();
            FC_MSG(shape.getShape().TShape().get() << ", " << shape.getShape().HashCode(max));
#else
            FC_MSG(shape.getShape().TShape().get()
                   << ", " << std::hash<TopoDS_Shape> {}(shape.getShape()));
#endif
        }
        printHistory(aHistory, sourceEdges);
        printHistory(newHistory, inputEdges);
    }

    // This method was originally part of WireJoinerP::makeCleanWire(), split to reduce cognitive
    // complexity
    void printHistoryFinal()
    {
        printHistory(aHistory, sourceEdges);
        FC_MSG("final:");
        for (int i = 1; i <= wireData->NbEdges(); ++i) {
            auto shape = wireData->Edge(i);
#if OCC_VERSION_HEX < 0x070800
            constexpr int max = std::numeric_limits<int>::max();
            FC_MSG(shape.TShape().get() << ", " << shape.HashCode(max));
#else
            FC_MSG(shape.TShape().get() << ", " << std::hash<TopoDS_Edge> {}(shape));
#endif
        }
    }

    TopoDS_Wire makeCleanWire(bool fixGap=true)
    {
        // Make a clean wire with sorted, oriented, connected, etc edges
        TopoDS_Wire result;
        std::vector<TopoShape> inputEdges;

        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE)) {
            for (int i = 1; i <= wireData->NbEdges(); ++i) {
                inputEdges.emplace_back(wireData->Edge(i));
            }
        }

        ShapeFix_Wire fixer;
        Handle(ShapeBuild_ReShape) reshape = new ShapeBuild_ReShape();
        fixer.SetContext(reshape);
        fixer.Load(wireData);
        fixer.SetMaxTolerance(myTol);
        fixer.ClosedWireMode() = Standard_True;
        fixer.Perform();
        // fixer.FixReorder();
        // fixer.FixConnected();

        if (fixGap) {
            // Gap fixing may change vertex, but we need all concident vertexes
            // to be the same one.
            //
            // fixer.FixGap3d(1, Standard_True);
        }

        fixer.FixClosed();

        result = fixer.Wire();
        auto newHistory = fixer.Context()->History();


        if (FC_LOG_INSTANCE.level() > FC_LOGLEVEL_TRACE + 1) {
            printHistoryInit(newHistory, inputEdges);
        }

        aHistory->Merge(newHistory);


        if (FC_LOG_INSTANCE.level() > FC_LOGLEVEL_TRACE + 1) {
            printHistoryFinal();
        }

        return result;
    }

    // This method was originally part of WireJoinerP::printHistory(), split to reduce cognitive
    // complexity
    template<class T>
    void printHistoryOfShape(const Handle(BRepTools_History)& hist, const T& shape)
    {
        for (TopTools_ListIteratorOfListOfShape it(hist->Modified(shape.getShape())); it.More();
             it.Next()) {
#if OCC_VERSION_HEX < 0x070800
                constexpr int max = std::numeric_limits<int>::max();
                FC_MSG(shape.getShape().TShape().get()
                   << ", " << shape.getShape().HashCode(max) << " -> "
                   << it.Value().TShape().get() << ", " << it.Value().HashCode(max));
#else
            FC_MSG(shape.getShape().TShape().get()
                   << ", " << std::hash<TopoDS_Shape> {}(shape.getShape()) << " -> "
                   << it.Value().TShape().get() << ", " << std::hash<TopoDS_Shape> {}(it.Value()));
#endif
        }
    }

    template<class T>
    void printHistory(Handle(BRepTools_History) hist, const T &input)
    {
        FC_MSG("\nHistory:\n");
        for (const auto& shape : input) {
            printHistoryOfShape(hist, shape);
        }
    }

    bool canShowShape(int idx=-1, bool forced=false) const
    {
        if (idx < 0 || catchIteration == 0 || catchIteration > idx) {
            if (!forced && FC_LOG_INSTANCE.level() <= FC_LOGLEVEL_TRACE) {
                return false;
            }
        }
        return true;
    }

    void showShape(const EdgeInfo* info, const char* name, int idx = -1, bool forced = false) const
    {
        if (!canShowShape(idx, forced)) {
            return;
        }
        showShape(info->shape(), name, idx, forced);
    }

    void showShape(WireInfo &wireInfo, const char *name, int idx=-1, bool forced=false)
    {
        if (!canShowShape(idx, forced)) {
            return;
        }
        if (wireInfo.wire.IsNull()) {
            initWireInfo(wireInfo);
        }
        showShape(wireInfo.wire, name, idx, forced);
    }

    void showShape(const TopoDS_Shape& sToShow,
                   const char* name,
                   int idx = -1,
                   bool forced = false) const
    {
        if (!canShowShape(idx, forced)) {
            return;
        }
        std::string _name;
        if (idx >= 0) {
            _name = name;
            _name += "_";
            _name += std::to_string(idx);
            _name += "_";
            name = _name.c_str();
        }
        auto obj = Feature::create(sToShow, name);
        FC_MSG(obj->getNameInDocument() << " " << ShapeHasher()(sToShow));
        if (catchObject == obj->getNameInDocument()) {
            FC_MSG("found");
        }
    }

    // This method was originally part of WireJoinerP::build(), split to reduce cognitive complexity
    void buildClosedWire()
    {
        findClosedWires(true);
        findTightBound();
        exhaustTightBound();
        bool done = !doOutline;
        while (!done) {
            ++iteration;
            done = true;
            std::unordered_map<EdgeInfo*, int> counter;
            std::unordered_set<WireInfo*> wires;
            for (auto& info : edges) {
                if (info.iteration == -2) {
                    continue;
                }
                if (info.iteration < 0 || !info.wireInfo || !info.wireInfo->done) {
                    if (info.iteration >= 0) {
                        info.iteration = -1;
                        done = false;
                        showShape(&info, "removed", iteration);
                        aHistory->Remove(info.edge);
                    }
                    continue;
                }
                if (info.wireInfo2 && wires.insert(info.wireInfo2.get()).second) {
                    for (auto& vertex : info.wireInfo2->vertices) {
                        if (++counter[vertex.edgeInfo()] == 2) {
                            vertex.edgeInfo()->iteration = -1;
                            done = false;
                            showShape(vertex.edgeInfo(), "removed2", iteration);
                            aHistory->Remove(info.edge);
                        }
                    }
                }
                if (!wires.insert(info.wireInfo.get()).second) {
                    continue;
                }
                for (auto& vertex : info.wireInfo->vertices) {
                    if (++counter[vertex.edgeInfo()] == 2) {
                        vertex.edgeInfo()->iteration = -1;
                        done = false;
                        showShape(vertex.edgeInfo(), "removed1", iteration);
                        aHistory->Remove(info.edge);
                    }
                }
            }
            findClosedWires(true);
            findTightBound();
        }

        builder.MakeCompound(compound);
        wireSet.clear();
        for (auto& info : edges) {
            if (info.iteration == -2) {
                if (!info.wireInfo) {
                    builder.Add(compound, info.wire());
                    continue;
                }
                addWire(info.wireInfo);
                addWire(info.wireInfo2);
            }
            else if (info.iteration >= 0) {
                addWire(info.wireInfo2);
                addWire(info.wireInfo);
            }
        }
        wireSet.clear();
    }

    void build()
    {
        clear();
        sourceEdges.clear();
        sourceEdges.insert(sourceEdgeArray.begin(), sourceEdgeArray.end());
        for (const auto& edge : sourceEdgeArray) {
            add(TopoDS::Edge(edge.getShape()), true);
        }

        if (doTightBound || doSplitEdge) {
            splitEdges();
        }

        buildAdjacentList();

        if (!doTightBound && !doOutline) {
            findClosedWires();
        }
        else {
            buildClosedWire();
        }

        // TODO: We choose to put open wires in a separated shape from the final
        // result shape, so the history may contains some entries that are not
        // presented in the final result, which will cause warning message when
        // generating topo naming in TopoShape::makESHAPE(). We've lowered log
        // message level to suppress the warning for the moment. The right way
        // to solve the problem is to reconstruct the history and filter out
        // those entries.

        bool hasOpenEdge = false;
        for (const auto &info : edges) {
            if (info.iteration == -3 || (!info.wireInfo && info.iteration>=0)) {
                if (!hasOpenEdge) {
                    hasOpenEdge = true;
                    builder.MakeCompound(openWireCompound);
                }
                builder.Add(openWireCompound, info.wire());
            }
        }
    }

    void addWire(std::shared_ptr<WireInfo> &wireInfo)
    {
        if (!wireInfo || !wireInfo->done || !wireSet.insertUnique(wireInfo.get())) {
            return;
        }
        initWireInfo(*wireInfo);
        builder.Add(compound, wireInfo->wire);
    }

    bool getOpenWires(TopoShape &shape, const char *op, bool noOriginal) {
        if (openWireCompound.IsNull()) {
            shape.setShape(TopoShape());
            return false;
        }
        auto comp = openWireCompound;
        if (noOriginal) {
            TopoShape source(-1);
            source.makeElementCompound(sourceEdgeArray);
            auto wires = TopoShape(openWireCompound, -1).getSubTopoShapes(TopAbs_WIRE);
            bool touched = false;
            for (auto it=wires.begin(); it!=wires.end();) {
                bool purge = true;
                for (const auto &edge : it->getSubShapes(TopAbs_EDGE)) {
                    if (source.findSubShapesWithSharedVertex(TopoShape(edge, -1)).empty()) {
                        purge = false;
                        break;
                    }
                }
                if (purge) {
                    it = wires.erase(it);
                    touched = true;
                }
                else {
                    ++it;
                }
            }
            if (touched) {
                if (wires.empty()) {
                    shape.setShape(TopoShape());
                    return false;
                }
                comp = TopoDS::Compound(TopoShape(-1).makeElementCompound(wires).getShape());
            }
        }
        shape.makeShapeWithElementMap(comp,
                        MapperHistory(aHistory),
                        {sourceEdges.begin(), sourceEdges.end()},
                        op);
        return true;
    }

    bool getResultWires(TopoShape &shape, const char *op) {
        // As compound is created by various calls to builder.MakeCompound() it looks that the
        // following condition is always false.
        // Probably it may be needed to add something like compound.Nullify() as done for
        // openWireCompound in WireJoiner::WireJoinerP::clear()
        if (compound.IsNull()) {
            shape.setShape(TopoShape());
            return false;
        }
        shape.makeShapeWithElementMap(compound,
                        MapperHistory(aHistory),
                        {sourceEdges.begin(), sourceEdges.end()},
                        op);
        return true;
    }
};


WireJoiner::WireJoiner()
    :pimpl(new WireJoinerP())
{
}

WireJoiner::~WireJoiner() = default;

void WireJoiner::addShape(const TopoShape &shape)
{
    NotDone();
    for (auto& edge : shape.getSubTopoShapes(TopAbs_EDGE)) {
        pimpl->sourceEdgeArray.push_back(edge);
    }
}

void WireJoiner::addShape(const std::vector<TopoShape> &shapes)
{
    NotDone();
    for (const auto &shape : shapes) {
        for (auto& edge : shape.getSubTopoShapes(TopAbs_EDGE)) {
            pimpl->sourceEdgeArray.push_back(edge);
        }
    }
}

void WireJoiner::addShape(const std::vector<TopoDS_Shape> &shapes)
{
    NotDone();
    for (const auto &shape : shapes) {
        for (TopExp_Explorer xp(shape, TopAbs_EDGE); xp.More(); xp.Next()) {
            pimpl->sourceEdgeArray.emplace_back(TopoDS::Edge(xp.Current()), -1);
        }
    }
}

void WireJoiner::setOutline(bool enable)
{
    if (enable != pimpl->doOutline) {
        NotDone();
        pimpl->doOutline = enable;
    }
}

void WireJoiner::setTightBound(bool enable)
{
    if (enable != pimpl->doTightBound) {
        NotDone();
        pimpl->doTightBound = enable;
    }
}

void WireJoiner::setSplitEdges(bool enable)
{
    if (enable != pimpl->doSplitEdge) {
        NotDone();
        pimpl->doSplitEdge = enable;
    }
}

void WireJoiner::setMergeEdges(bool enable)
{
    if (enable != pimpl->doSplitEdge) {
        NotDone();
        pimpl->doMergeEdge = enable;
    }
}

void WireJoiner::setTolerance(double tol, double atol)
{
    if (tol >= 0 && tol != pimpl->myTol) {
        NotDone();
        pimpl->myTol = tol;
        pimpl->myTol2 = tol * tol;
    }
    if (atol >= 0 && atol != pimpl->myAngularTol) {
        NotDone();
        pimpl->myAngularTol = atol;
    }
}

#if OCC_VERSION_HEX < 0x070600
void WireJoiner::Build()
{
#else
void WireJoiner::Build(const Message_ProgressRange& theRange)
{
    (void)theRange;
#endif
    if (IsDone()) {
        return;
    }
    pimpl->build();
    if (TopoShape(pimpl->compound).countSubShapes(TopAbs_SHAPE) > 0) {
        myShape = pimpl->compound;
    }
    else {
        myShape.Nullify();
    }
    Done();
}

bool WireJoiner::getOpenWires(TopoShape &shape, const char *op, bool noOriginal)
{
    Build();
    return pimpl->getOpenWires(shape, op, noOriginal);
}

bool WireJoiner::getResultWires(TopoShape &shape, const char *op)
{
    Build();
    return pimpl->getResultWires(shape, op);
}

const TopTools_ListOfShape& WireJoiner::Generated (const TopoDS_Shape& SThatGenerates)
{
    Build();
    return pimpl->aHistory->Generated(SThatGenerates);
}

const TopTools_ListOfShape& WireJoiner::Modified (const TopoDS_Shape& SThatModifies)
{
    Build();
    return pimpl->aHistory->Modified(SThatModifies);
}

Standard_Boolean WireJoiner::IsDeleted (const TopoDS_Shape& SDeleted)
{
    Build();
    return pimpl->aHistory->IsRemoved(SDeleted);
}
