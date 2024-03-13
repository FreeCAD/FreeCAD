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
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/graph/graph_concepts.hpp>

#ifndef _PreComp_
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
#include <boost_geometry.hpp>

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

using RParameters = bgi::linear<16>;

BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET(
        gp_Pnt,double,bg::cs::cartesian,X,Y,Z,SetX,SetY,SetZ)

FC_LOG_LEVEL_INIT("WireJoiner",true, true)

using namespace Part;

static inline void getEndPoints(const TopoDS_Edge &e, gp_Pnt &p1, gp_Pnt &p2) {
    p1 = BRep_Tool::Pnt(TopExp::FirstVertex(e));
    p2 = BRep_Tool::Pnt(TopExp::LastVertex(e));
}

static inline void getEndPoints(const TopoDS_Wire &wire, gp_Pnt &p1, gp_Pnt &p2) {
    BRepTools_WireExplorer xp(wire);
    p1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.CurrentVertex()));
    for(;xp.More();xp.Next());
    p2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.CurrentVertex()));
}

static void _assertCheck(int line, bool cond, const char *msg)
{
    if (!cond) {
        _FC_ERR(__FILE__, line, "Assert failed: " << msg);
        throw Base::RuntimeError("Assertion failed");
    }
}

#define assertCheck(cond)  _assertCheck(__LINE__, cond, #cond)

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
    int catchIteration;
    int iteration = 0;

    typedef bg::model::box<gp_Pnt> Box;
    
    bool checkBBox(const Bnd_Box &box)
    {
        if (box.IsVoid())
            return false;
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        return zMax - zMin <= myTol;
    }

    WireJoinerP()
    {
        auto hParam = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/WireJoiner");
        catchObject = hParam->GetASCII("ObjectName");
        catchIteration = hParam->GetInt("Iteration", 0);
    }

    bool getBBox(const TopoDS_Shape &e, Bnd_Box &bound) {
        BRepBndLib::AddOptimal(e,bound,Standard_False);
        if (bound.IsVoid()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("failed to get bound of edge");
            return false;
        }
        if (!checkBBox(bound))
            showShape(e, "invalid");
        if (bound.SquareExtent() < myTol2)
            return false;
        bound.Enlarge(myTol);
        return true;
    }

    bool getBBox(const TopoDS_Shape &e, Box &box) {
        Bnd_Box bound;
        if (!getBBox(e, bound))
            return false;
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
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
        int iStart[2]; // adjacent list index start for p1 and p2
        int iEnd[2]; // adjacent list index end
        int iteration;
        int iteration2;
        bool queryBBox;
        std::shared_ptr<WireInfo> wireInfo;
        std::shared_ptr<WireInfo> wireInfo2; // an edge can be shared by at most two tight bound wires.
        std::unique_ptr<Geometry> geo;
        Standard_Real firstParam;
        Standard_Real lastParam;
        Handle_Geom_Curve curve;
        GeomAbs_CurveType type;
        bool isLinear;

        EdgeInfo(const TopoDS_Edge &e,
                 const gp_Pnt &pt1,
                 const gp_Pnt &pt2,
                 const Box &bound,
                 bool bbox,
                 bool isLinear)
            :edge(e),p1(pt1),p2(pt2),box(bound),queryBBox(bbox),isLinear(isLinear)
        {
            curve = BRep_Tool::Curve(e, firstParam, lastParam);
            type = GeomAdaptor_Curve(curve).GetType();
            assertCheck(!curve.IsNull());
            GeomLProp_CLProps prop(curve,(firstParam+lastParam)*0.5,0,Precision::Confusion());
            mid = prop.Value();

            iteration = 0;
            reset();
        }
        Geometry *geometry() {
            if (!geo)
                geo = Geometry::fromShape(edge, /*silent*/true);
            return geo.get();
        }
        void reset() {
            wireInfo.reset();
            wireInfo2.reset();
            if (iteration >= 0)
                iteration = 0;
            iteration2 = 0;
            iStart[0] = iStart[1] = iEnd[0] = iEnd[1] = -1;
        }
        const TopoDS_Shape &shape(bool forward=true) const
        {
            if (superEdge.IsNull()) {
                if (forward)
                    return edge;
                if (edgeReversed.IsNull())
                    edgeReversed = edge.Reversed();
                return edgeReversed;
            }
            if (forward)
                return superEdge;
            if (superEdgeReversed.IsNull())
                superEdgeReversed = superEdge.Reversed();
            return superEdgeReversed;
        }
        TopoDS_Wire wire() const
        {
            auto s = shape();
            if (s.ShapeType() == TopAbs_WIRE)
                return TopoDS::Wire(s);
            return BRepBuilderAPI_MakeWire(TopoDS::Edge(s)).Wire();
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
        bool contains(const T &v)
        {
            if (!sorted) {
                if (data.size() < 30)
                    return std::find(data.begin(), data.end(), v) != data.end();
                sort();
            }
            auto it = std::lower_bound(data.begin(), data.end(), v);
            return it!=data.end() && *it == v;
        }
        bool intersects(const VectorSet<T> &other)
        {
            if (other.size() < size())
                return other.intersects(*this);
            else if (!sorted) {
                for (const auto &v : data) {
                    if (other.contains(v))
                        return true;
                }
            } else {
                other.sort();
                auto it = other.data.begin();
                for (const auto &v : data) {
                    it = std::lower_bound(it, other.data.end(), v);
                    if (it == other.data.end())
                        return false;
                    if (*it == v)
                        return true;
                }
            }
            return false;
        }
        void insert(const T &v)
        {
            if (sorted)
                data.insert(std::upper_bound(data.begin(), data.end(), v), v);
            else
                data.push_back(v);
        }
        bool insertUnique(const T &v)
        {
            if (sorted) {
                auto it = std::lower_bound(data.begin(), data.end(), v);
                if (it != data.end() && *it == v)
                    return false;
                data.insert(it, v);
                return true;
            }
            
            if (contains(v))
                return false;
            data.push_back(v);
            return true;
        }
        void erase(const T &v)
        {
            if (!sorted)
                data.erase(std::remove(data.begin(), data.end(), v), data.end());
            else {
                auto it = std::lower_bound(data.begin(), data.end(), v);
                auto itEnd = it;
                while (itEnd!=data.end() && *itEnd==v)
                    ++itEnd;
                data.erase(it, itEnd);
            }
            if (data.size() < 20)
                sorted = false;
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
        std::vector<T> data;
    };

    Handle(BRepTools_History) aHistory = new BRepTools_History;

    typedef std::list<EdgeInfo> Edges;
    Edges edges;

    std::map<EdgeInfo*, Edges::iterator> edgesTable;

    struct VertexInfo {
        Edges::iterator it;
        bool start;
        VertexInfo()
        {}
        VertexInfo(Edges::iterator it, bool start)
            :it(it),start(start)
        {}
        VertexInfo reversed() const {
            return VertexInfo(it, !start);
        }
        bool operator==(const VertexInfo &other) const {
            return it==other.it && start==other.start;
        }
        bool operator<(const VertexInfo &other) const {
            auto a = edgeInfo();
            auto b = other.edgeInfo();
            if (a < b)
                return true;
            if (a > b)
                return false;
            return start < other.start;
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
        StackInfo(size_t idx=0):iStart(idx),iEnd(idx),iCurrent(idx){}
    };

    std::vector<StackInfo> stack;
    std::vector<VertexInfo> vertexStack;
    std::vector<VertexInfo> tmpVertices;
    std::vector<VertexInfo> adjacentList;

    struct WireInfo {
        std::vector<VertexInfo> vertices;
        mutable std::vector<int> sorted;
        TopoDS_Wire wire;
        TopoDS_Face face;
        mutable Bnd_Box box;
        bool done = false;
        bool purge = false;

        void sort() const
        {
            if (sorted.size() == vertices.size())
                return;
            assertCheck(sorted.size() < vertices.size());
            sorted.reserve(vertices.size());
            for (int i=(int)sorted.size(); i<(int)vertices.size(); ++i)
                sorted.push_back(i);
            std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
                return vertices[a] < vertices[b];
            });
        }
        int find(const VertexInfo &info) const
        {
            if (vertices.size() < 20) {
                auto it = std::find(vertices.begin(), vertices.end(), info);
                if (it == vertices.end())
                    return 0;
                return it - vertices.begin() + 1;
            }
            sort();
            auto it = std::lower_bound(sorted.begin(), sorted.end(), info,
                    [&](int idx, const VertexInfo &v) {return vertices[idx]<v;});
            int res = 0;
            if (it != sorted.end() && vertices[*it] == info)
                res = *it + 1;
            return res;
        }
        int find(const EdgeInfo *info) const
        {
            if (vertices.size() < 20) {
                for (auto it=vertices.begin(); it!=vertices.end(); ++it) {
                    if (it->edgeInfo() == info)
                        return it - vertices.begin() + 1;
                }
                return 0;
            }
            sort();
            auto it = std::lower_bound(sorted.begin(), sorted.end(), info,
                    [&](int idx, const EdgeInfo *v) {return vertices[idx].edgeInfo()<v;});
            int res = 0;
            if (it != sorted.end() && vertices[*it].edgeInfo() == info)
                res = *it + 1;
            return res;
        }
        bool isSame(const WireInfo &other) const
        {
            if (this == &other)
                return true;
            if (vertices.size() != other.vertices.size())
                return false;
            if (vertices.empty())
                return true;
            int idx=find(other.vertices.front().edgeInfo()) - 1;
            if (idx < 0)
                return false;
            for (auto &v : other.vertices) {
                if (v.edgeInfo() != vertices[idx].edgeInfo())
                    return false;
                if (++idx == (int)vertices.size())
                    idx = 0;
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

    const Bnd_Box &getWireBound(const WireInfo &wireInfo)
    {
        if (wireInfo.box.IsVoid()) {
            for (auto &v : wireInfo.vertices)
                BRepBndLib::Add(v.it->shape(),wireInfo.box);
            wireInfo.box.Enlarge(myTol);
        }
        return wireInfo.box;
    }

    bool initWireInfo(WireInfo &wireInfo)
    {
        if (!wireInfo.face.IsNull())
            return true;
        getWireBound(wireInfo);
        if (wireInfo.wire.IsNull()) {
            wireData->Clear();
            for (auto &v : wireInfo.vertices)
                wireData->Add(v.it->shape(v.start));
            wireInfo.wire = makeCleanWire();
        }

        if (!BRep_Tool::IsClosed(wireInfo.wire)) {
            showShape(wireInfo.wire, "FailedToClose");
            FC_ERR("Wire not closed");
            for (auto &v : wireInfo.vertices) {
                showShape(v.edgeInfo(), v.start ? "failed" : "failed_r", iteration);
            }
            return false;
        }

        BRepBuilderAPI_MakeFace mkFace(wireInfo.wire);
        if (!mkFace.IsDone()) {
            FC_ERR("Failed to create face for wire");
            showShape(wireInfo.wire, "FailedFace");
            return false;
        }
        wireInfo.face = mkFace.Face();
        return true;
    }

    bool isInside(const WireInfo &wireInfo, gp_Pnt &pt)
    {
        if (getWireBound(wireInfo).IsOut(pt))
            return false;
        BRepClass_FaceClassifier fc(wireInfo.face, pt, myTol);
        return fc.State() == TopAbs_IN;
    }

    bool isOutside(const WireInfo &wireInfo, gp_Pnt &pt)
    {
        if (getWireBound(wireInfo).IsOut(pt))
            return false;
        BRepClass_FaceClassifier fc(wireInfo.face, pt, myTol);
        return fc.State() == TopAbs_OUT;
    }

    struct PntGetter
    {
        typedef const gp_Pnt& result_type;
        result_type operator()(const VertexInfo &v) const {
            return v.pt();
        }
    };

    bgi::rtree<VertexInfo,RParameters, PntGetter> vmap;

    struct BoxGetter
    {
        typedef const Box& result_type;
        result_type operator()(Edges::iterator it) const {
            return it->box;
        }
    };
    bgi::rtree<Edges::iterator,RParameters, BoxGetter> boxMap;

    BRep_Builder builder;
    TopoDS_Compound compound;

    std::unordered_set<TopoShape, ShapeHasher, ShapeHasher> sourceEdges;
    std::vector<TopoShape> sourceEdgeArray;
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
        if (it->queryBBox)
            boxMap.remove(it);
        vmap.remove(VertexInfo(it,true));
        vmap.remove(VertexInfo(it,false));
        return edges.erase(it);
    }

    void remove(EdgeInfo *info)
    {
        if (edgesTable.empty()) {
            for (auto it=edges.begin(); it!=edges.end(); ++it)
                edgesTable[&(*it)] = it;
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
        if (it->queryBBox)
            boxMap.insert(it);
        showShape(it->edge, "add");
    }

    int add(const TopoDS_Edge &e, bool queryBBox=false)
    {
        auto it = edges.begin();
        return add(e, queryBBox, it);
    }

    int add(const TopoDS_Edge &e, bool queryBBox, Edges::iterator &it)
    {
        Box bbox;
        if (!getBBox(e, bbox)) {
            showShape(e, "small");
            aHistory->Remove(e);
            return 0;
        }
        return add(e, queryBBox, bbox, it) ? 1 : -1;
    }

    bool add(const TopoDS_Edge &e, bool queryBBox, const Box &bbox, Edges::iterator &it)
    {
        gp_Pnt p1,p2;
        getEndPoints(e,p1,p2);
        TopoDS_Vertex v1, v2;
        TopoDS_Edge ev1, ev2;
        double tol = myTol2;
        // search for duplicate edges
        showShape(e, "addcheck");
        bool isLinear = TopoShape(e).isLinearEdge();
        std::unique_ptr<Geometry> geo;

        for (auto vit=vmap.qbegin(bgi::nearest(p1,INT_MAX));vit!=vmap.qend();++vit) {
            auto &vinfo = *vit;
            if (canShowShape())
                FC_MSG("addcheck " << vinfo.edge().HashCode(INT_MAX));
            double d1 = vinfo.pt().SquareDistance(p1);
            if (d1 >= tol)
                break;
            if (v1.IsNull()) {
                ev1 = vinfo.edge();
                v1 = vinfo.vertex();
            }
            double d2 = vinfo.ptOther().SquareDistance(p2);
            if (d2 < tol) {
                if (v2.IsNull()) {
                    ev2 = vinfo.edge();
                    v2 = vinfo.otherVertex();
                }
                if (isLinear && vinfo.edgeInfo()->isLinear) {
                    showShape(e, "duplicate");
                    aHistory->Remove(e);
                    return false;
                }
                else if (auto geoEdge = vinfo.edgeInfo()->geometry()) {
                    if (!geo)
                        geo = Geometry::fromShape(e, /*silent*/true);
                    if (geo && geo->isSame(*geoEdge, myTol, myAngularTol)) {
                        showShape(e, "duplicate");
                        aHistory->Remove(e);
                        return false;
                    }
                }
            }
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
        auto connectEdge = [&](TopoDS_Edge &e,
                              const TopoDS_Vertex &v,
                              const TopoDS_Edge &eOther,
                              const TopoDS_Vertex &vOther)
        {
            if (vOther.IsNull())
                return;
            if (v.IsSame(vOther))
                return;
            double tol = std::max(BRep_Tool::Pnt(v).Distance(BRep_Tool::Pnt(vOther)),
                                  BRep_Tool::Tolerance(vOther));
            if (tol >= BRep_Tool::Tolerance(v)) {
                ShapeFix_ShapeTolerance fix;
                fix.SetTolerance(v, std::max(tol*0.5, myTol), TopAbs_VERTEX);
            }
            BRepBuilderAPI_MakeWire mkWire(eOther);
            mkWire.Add(e);
            auto newEdge = mkWire.Edge();
            TopoDS_Vertex vFirst = TopExp::FirstVertex(newEdge);
            TopoDS_Vertex vLast = TopExp::LastVertex(newEdge);
            assertCheck(vLast.IsSame(vOther) || vFirst.IsSame(vOther));
            e = newEdge;
        };

        TopoDS_Edge edge = e;
        TopoDS_Vertex vFirst = TopExp::FirstVertex(e);
        TopoDS_Vertex vLast = TopExp::LastVertex(e);
        connectEdge(edge, vFirst, ev1, v1);
        connectEdge(edge, vLast, ev2, v2);
        if (!edge.IsSame(e)) {
            auto itSource = sourceEdges.find(e);
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
        for (TopExp_Explorer xp(shape,TopAbs_EDGE); xp.More(); xp.Next())
            add(TopoDS::Edge(xp.Current()),queryBBox);
    }

    //This algorithm tries to join connected edges into wires
    //
    //tol*tol>Precision::SquareConfusion() can be used to join points that are
    //close but do not coincide with a line segment. The close points may be
    //the results of rounding issue.
    //
    void join()
    {
        double tol = myTol2;
        while (edges.size()) {
            auto it = edges.begin();
            BRepBuilderAPI_MakeWire mkWire;
            mkWire.Add(it->edge);
            gp_Pnt pstart(it->p1),pend(it->p2);
            remove(it);

            bool done = false;
            for (int idx=0;!done&&idx<2;++idx) {
                while (edges.size()) {
                    std::vector<VertexInfo> ret;
                    ret.reserve(1);
                    const gp_Pnt &pt = idx==0?pstart:pend;
                    vmap.query(bgi::nearest(pt,1),std::back_inserter(ret));
                    assertCheck(ret.size()==1);
                    double d = ret[0].pt().SquareDistance(pt);
                    if (d > tol) break;

                    const auto &info = *ret[0].it;
                    bool start = ret[0].start;
                    if (d > Precision::SquareConfusion()) {
                        // insert a filling edge to solve the tolerance problem
                        const gp_Pnt &pt = ret[idx].pt();
                        if (idx)
                            mkWire.Add(BRepBuilderAPI_MakeEdge(pend,pt).Edge());
                        else
                            mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pstart).Edge());
                    }

                    if (idx==1 && start) {
                        pend = info.p2;
                        mkWire.Add(info.edge);
                    }else if (idx==0 && !start) {
                        pstart = info.p1;
                        mkWire.Add(info.edge);
                    }else if (idx==0 && start) {
                        pstart = info.p2;
                        mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
                    }else {
                        pend = info.p1;
                        mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
                    }
                    remove(ret[0].it);
                    if (pstart.SquareDistance(pend)<=Precision::SquareConfusion()){
                        done = true;
                        break;
                    }
                }
            }
            builder.Add(compound,mkWire.Wire());
        }
    }

    struct IntersectInfo {
        double param;
        TopoDS_Shape intersectShape;
        gp_Pnt point;
        IntersectInfo(double p, const gp_Pnt &pt, const TopoDS_Shape &s)
            :param(p), intersectShape(s), point(pt)
        {}
        bool operator<(const IntersectInfo &other) const {
            return param < other.param;
        }
    };

    void checkSelfIntersection(const EdgeInfo &info, std::set<IntersectInfo> &params)
    {
        // Early return if checking for self intersection (only for non linear spline curves)
        if (info.type <= GeomAbs_Parabola || info.isLinear)
            return;
        IntRes2d_SequenceOfIntersectionPoint points2d;
        TColgp_SequenceOfPnt points3d;
        TColStd_SequenceOfReal errors;
        TopoDS_Wire wire;
        BRepBuilderAPI_MakeWire mkWire(info.edge);
        if (!mkWire.IsDone())
            return;
        if (!BRep_Tool::IsClosed(mkWire.Wire())) {
            BRepBuilderAPI_MakeEdge mkEdge(info.p1, info.p2);
            if (!mkEdge.IsDone())
                return;
            mkWire.Add(mkEdge.Edge());
        }
        wire = mkWire.Wire();
        BRepBuilderAPI_MakeFace mkFace(wire);
        if (!mkFace.IsDone())
            return;
        TopoDS_Face face = mkFace.Face();
        ShapeAnalysis_Wire analysis(wire, face, myTol);
        analysis.CheckSelfIntersectingEdge(1, points2d, points3d);
        assertCheck(points2d.Length() == points3d.Length());
        for (int i=1; i<=points2d.Length(); ++i) {
            params.emplace(points2d(i).ParamOnFirst(), points3d(i), info.edge);
            params.emplace(points2d(i).ParamOnSecond(), points3d(i), info.edge);
        }
    }

    void checkIntersection(const EdgeInfo &info,
                           const EdgeInfo &other,
                           std::set<IntersectInfo> &params1,
                           std::set<IntersectInfo> &params2)
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
                if (extss.IsDone() && extss.NbSolution() > 0)
                if (!extss.IsDone() || extss.NbSolution()<=0 || extss.Value()>=myTol)
                    return;
                for (int i=1; i<=extss.NbSolution(); ++i) {
                    Standard_Real p;
                    auto s1 = extss.SupportOnShape1(i);
                    auto s2 = extss.SupportOnShape2(i);
                    if (s1.ShapeType() == TopAbs_EDGE) {
                        extss.ParOnEdgeS1(i,p);
                        pushIntersection(params1, p, extss.PointOnShape1(i), other.edge);
                    }
                    if (s2.ShapeType() == TopAbs_EDGE) {
                        extss.ParOnEdgeS2(i,p);
                        pushIntersection(params2, p, extss.PointOnShape2(i), info.edge);
                    }
                }
                return;
            }
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
        int idx;
        BRepBuilderAPI_MakeWire mkWire(info.edge);
        mkWire.Add(other.edge);
        if (mkWire.IsDone())
            idx = 2;
        else if (mkWire.Error() == BRepBuilderAPI_DisconnectedWire) {
            idx = 3;
            BRepBuilderAPI_MakeEdge mkEdge(info.p1, other.p1);
            if (!mkEdge.IsDone()) {
                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_WARN("Failed to build edge for checking intersection");
                return;
            }
            mkWire.Add(mkEdge.Edge());
            mkWire.Add(other.edge);
        }

        if (!mkWire.IsDone()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Failed to build wire for checking intersection");
            return;
        }
        wire = mkWire.Wire();
        if (!BRep_Tool::IsClosed(wire)) {
            gp_Pnt p1, p2;
            getEndPoints(wire, p1, p2);
            BRepBuilderAPI_MakeEdge mkEdge(p1, p2);
            if (!mkEdge.IsDone()) {
                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_WARN("Failed to build edge for checking intersection");
                return;
            }
            mkWire.Add(mkEdge.Edge());
        }

        BRepBuilderAPI_MakeFace mkFace(wire);
        if (!mkFace.IsDone()) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("Failed to build face for checking intersection");
            return;
        }
        TopoDS_Face face = mkFace.Face();
        ShapeAnalysis_Wire analysis(wire, face, myTol);
        analysis.CheckIntersectingEdges(1, idx, points2d, points3d, errors);
        assertCheck(points2d.Length() == points3d.Length());
        for (int i=1; i<=points2d.Length(); ++i) {
            pushIntersection(params1, points2d(i).ParamOnFirst(), points3d(i), other.edge);
            pushIntersection(params2, points2d(i).ParamOnSecond(), points3d(i), info.edge);
        }
    }

    void pushIntersection(std::set<IntersectInfo> &params, double param, const gp_Pnt &pt, const TopoDS_Shape &shape)
    {
        IntersectInfo info(param, pt, shape);
        auto it = params.upper_bound(info);
        if (it != params.end()) {
            if (it->point.SquareDistance(pt) < myTol2)
                return;
        }
        if (it != params.begin()) {
            auto itPrev = it;
            --itPrev;
            if (itPrev->point.SquareDistance(pt) < myTol2)
                return;
        }
        params.insert(it, info);
        return;
    }

    struct SplitInfo {
        TopoDS_Edge edge;
        TopoDS_Shape intersectShape;
        Box bbox; 
    };

    // Try splitting any edges that intersects other edge
    void splitEdges()
    {
        std::unordered_map<const EdgeInfo*, std::set<IntersectInfo>> intersects;

        int i=0;
        for (auto &info : edges)
            info.iteration = ++i;

        std::unique_ptr<Base::SequencerLauncher> seq(
                new Base::SequencerLauncher("Splitting edges", edges.size()));

        i = 0;
        for (auto it=edges.begin(); it!=edges.end();++it) {
            seq->next(true);
            ++i;
            auto &info = *it;
            auto &params = intersects[&info];
            checkSelfIntersection(info, params);

            for (auto vit=boxMap.qbegin(bgi::intersects(info.box)); vit!=boxMap.qend(); ++vit) {
                const auto &other = *(*vit);
                if (other.iteration <= i) {
                    // means the edge is before us, and we've already checked intersection
                    continue;
                }
                checkIntersection(info, other, params, intersects[&other]);
            }
        }

        i=0;
        std::vector<SplitInfo> splitted;
        for (auto it=edges.begin(); it!=edges.end(); ) {
            ++i;
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
            if (itParam->point.SquareDistance(info.p1) < myTol2)
                params.erase(itParam);
            params.emplace(info.firstParam, info.p1, TopoDS_Shape());
            itParam = params.end();
            --itParam;
            if (itParam->point.SquareDistance(info.p2) < myTol2)
                params.erase(itParam);
            params.emplace(info.lastParam, info.p2, TopoDS_Shape());

            if (params.size() <= 2) {
                ++it;
                continue;
            }

            splitted.clear();
            itParam = params.begin();
            for (auto itPrevParam=itParam++; itParam!=params.end(); ++itParam) {
                const auto &intersectShape = itParam->intersectShape.IsNull()
                    ? itPrevParam->intersectShape : itParam->intersectShape;
                if (intersectShape.IsNull())
                    break;

                // Using points cause MakeEdge failure for some reason. Using
                // parameters is better.
                //
                const gp_Pnt &p1 = itPrevParam->point;
                const gp_Pnt &p2 = itParam->point;
                const Standard_Real &param1 = itPrevParam->param;
                const Standard_Real &param2 = itParam->param;

                BRepBuilderAPI_MakeEdge mkEdge(info.curve, param1, param2);
                if (mkEdge.IsDone()) {
                    splitted.emplace_back();
                    auto &entry = splitted.back();
                    entry.edge = mkEdge.Edge();
                    entry.intersectShape = intersectShape;
                    if (getBBox(entry.edge, entry.bbox))
                        itPrevParam = itParam;
                    else
                        splitted.pop_back();
                    }
                else if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                    FC_WARN("edge split failed "
                            << std::setprecision(16)
                            << FC_XYZ(p1) << FC_XYZ(p2)
                            << ": " << mkEdge.Error());
                }
            }
            if (splitted.size() <= 1) {
                ++it;
                continue;
            }

            showShape(info.edge, "remove");
            auto removedEdge = info.edge;
            it = remove(it);
            for (const auto &v : splitted) {
                if (!add(v.edge, false, v.bbox, it))
                    continue;
                auto &newInfo = *it++;
                aHistory->AddModified(v.intersectShape, newInfo.edge);
                // if (v.intersectShape != removedEdge)
                //     aHistory->AddModified(removedEdge, newInfo.edge);
                showShape(newInfo.edge, "split");
            }
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
        for (auto it=edges.begin(); it!=edges.end(); ++it) {
            seq->next(true);
            auto &info = *it;
            if (info.iteration == iteration || info.iteration < 0)
                continue;
            info.iteration = iteration;
            // showShape(&info, "scheck");

            vertices.clear();
            vertices.emplace_back(it, true);
            edgeSet.clear();

            bool done = false;
            for (int k=0; k<2; ++k) { // search in both direction
                auto begin = k==1 ? vertices.back().reversed() : vertices.front();
                while (true) {
                    auto currentVertex = k==1 ? vertices.front() : vertices.back();
                    auto current = currentVertex.edgeInfo();
                    // showShape(current, "siter", k);
                    int idx = (currentVertex.start?1:0)^k;
                    EdgeInfo *found = nullptr;
                    for (int i=current->iStart[idx]; i<current->iEnd[idx]; ++i) {
                        const auto &v = adjacentList[i];
                        auto next = v.edgeInfo();
                        if (next->iteration < 0 // skipped
                                || next == current) // skip self (see how adjacent list is built)
                            continue;
                        if (v == begin) {
                            // closed
                            done = true;
                            break;
                        }
                        if (found // more than one branch
                            || edgeSet.contains(next)) // or, self intersect
                        {
                            // if (found) {
                            //     showShape(found, "branch_a", k);
                            //     showShape(next, "branch_b", k);
                            // } else {
                            //     showShape(next, "insect", k);
                            // }
                            found = nullptr;
                            break;
                        }
                        found = next;
                        currentVertex = v;
                    }
                    if (done || !found)
                        break;
                    // showShape(found, "snext", k);
                    if (k==1) {
                        edgeSet.insert(current);
                        vertices.push_front(currentVertex.reversed());
                    } else {
                        edgeSet.insert(found);
                        vertices.push_back(currentVertex);
                    }
                }
                if (done)
                    break;
            }

            if (vertices.size() <= 1)
                continue;

            wireData->Clear();
            Bnd_Box bbox;
            for (const auto &v : vertices) {
                auto current = v.edgeInfo();
                bbox.Add(current->box.min_corner());
                bbox.Add(current->box.max_corner());
                wireData->Add(current->shape(v.start));
                showShape(current, "edge");
                current->iteration = -1;
            }
            auto first = vertices.front().edgeInfo();
            first->superEdge = makeCleanWire(false);
            first->superEdgeReversed.Nullify();
            if (BRep_Tool::IsClosed(first->superEdge)) {
                first->iteration = -2;
                showShape(first, "super_done");
            } else {
                first->iteration = iteration;
                showShape(first, "super");
                auto &vFirst = vertices.front();
                auto &vLast = vertices.back();
                auto last = vLast.edgeInfo();
                vFirst.ptOther() = vLast.ptOther();
                int idx = vFirst.start ? 1 : 0;
                first->iStart[idx] = last->iStart[vLast.start?1:0];
                first->iEnd[idx] = last->iEnd[vLast.start?1:0];

                for (int i=first->iStart[idx];i<first->iEnd[idx];++i) {
                    auto &v = adjacentList[i];
                    if (v.it == vLast.it) {
                        v.it = vFirst.it;
                        v.start = !vFirst.start;
                    }
                }
                bbox.Enlarge(myTol);
                first->box = Box(bbox.CornerMin(), bbox.CornerMax());
            }
        }
    }

    void buildAdjacentList()
    {
        builder.MakeCompound(compound);

        for (auto &info : edges)
            info.reset();

        adjacentList.clear();

        // populate adjacent list
        for (auto &info : edges) {
            if (info.iteration == -2) {
#if OCC_VERSION_HEX >= 0x070000
                assertCheck(BRep_Tool::IsClosed(info.shape()));
#endif
                showShape(&info,"closed");
                if (!doTightBound)
                    builder.Add(compound,info.wire());
                continue;
            } else if (info.iteration < 0)
                continue;

            if (info.p1.SquareDistance(info.p2)<=myTol2) {
                if (!doTightBound)
                    builder.Add(compound,info.wire());
                info.iteration = -2;
                continue;
            }

            gp_Pnt pt[2];
            pt[0] = info.p1;
            pt[1] = info.p2;
            for (int i=0;i<2;++i) {
                if (info.iStart[i]>=0)
                    continue;
                info.iEnd[i] = info.iStart[i] = (int)adjacentList.size();

                for (auto vit=vmap.qbegin(bgi::nearest(pt[i],INT_MAX));vit!=vmap.qend();++vit) {
                    auto &vinfo = *vit;
                    if (vinfo.pt().SquareDistance(pt[i]) > myTol2)
                        break;

                    // We must push ourself too, because the adjacency
                    // information is shared among all connected edges.
                    //
                    // if (&(*vinfo.it) == &info)
                    //     continue;

                    if (vinfo.it->iteration < 0)
                        continue;

                    adjacentList.push_back(vinfo);
                    ++info.iEnd[i];
                }

                // copy the adjacent indices to all connected edges
                for (int j=info.iStart[i];j<info.iEnd[i];++j) {
                    auto &other = adjacentList[j];
                    auto &otherInfo = *other.it;
                    if (&otherInfo != &info) {
                        int k = other.start?0:1;
                        otherInfo.iStart[k] = info.iStart[i];
                        otherInfo.iEnd[k] = info.iEnd[i];
                    }
                }
            }
        }

        bool done = false;

        while (!done) {
            done = true;

            if (doMergeEdge || doTightBound)
                findSuperEdges();

            //Skip edges that are connected to only one end
            for (auto &info : edges) {
                if (info.iteration<0)
                    continue;
                for (int k=0; k<2; ++k) {
                    int i;
                    for (i=info.iStart[k]; i<info.iEnd[k]; ++i) {
                        const auto &v = adjacentList[i];
                        auto other = v.edgeInfo();
                        if (other->iteration >= 0 && other != &info)
                            break;
                    }
                    if (i == info.iEnd[k]) {
                        // If merge or tight bound, then repeat until no edges
                        // can be skipped.
                        done = !doMergeEdge & !doTightBound;
                        info.iteration = -3;
                        showShape(&info, "skip");
                        break;
                    }
                }
            }
        }
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
            if (beginInfo.iteration < 0 || beginInfo.wireInfo)
                continue;

            VertexInfo currentVertex(it, true);
            EdgeInfo *currentInfo = &beginInfo;
            showShape(currentInfo, "begin");
            stack.clear();
            vertexStack.clear();
            edgeSet.clear();

            TopoDS_Wire wire = _findClosedWires(beginVertex, currentVertex);
            if (wire.IsNull())
                continue;

            if (tightBound) {
                assert(!beginInfo.wireInfo);
                beginInfo.wireInfo.reset(new WireInfo);
                beginInfo.wireInfo->vertices.emplace_back(it, true);
                beginInfo.wireInfo->wire = wire;
            }
            for (auto &r : stack) {
                const auto &v = vertexStack[r.iCurrent];
                auto &info = *v.it;
                if (tightBound)
                    beginInfo.wireInfo->vertices.push_back(v);
                if (!info.wireInfo) {
                    info.wireInfo = beginInfo.wireInfo;
                    // showShape(&info, "visited");
                }
            }
            showShape(wire,"joined");
            if (!tightBound)
                builder.Add(compound, wire);
        }
    }

    void checkStack()
    {
#if 0
        if (stack.size() <= 1)
            return;
        std::vector<EdgeInfo*> edges;
        auto &r = stack[stack.size()-2];
        for (int i=r.iStart;i<r.iEnd;++i)
            edges.push_back(vertexStack[i].edgeInfo());
        auto &r2 = stack.back();
        for (int i=r2.iStart;i<r2.iEnd;++i) {
            auto info = vertexStack[i].edgeInfo();
            assertCheck(std::find(edges.begin(), edges.end(), info) == edges.end());
        }
#endif
    }

    void checkWireInfo(const WireInfo &wireInfo)
    {
        (void)wireInfo;
        if (FC_LOG_INSTANCE.level()<=FC_LOGLEVEL_TRACE)
            return;
        int i = 0;
        for (auto &info : edges) {
            ++i;
            if (auto w = info.wireInfo.get())
                assertCheck(w->vertices.front().edgeInfo()->wireInfo.get() == w);
        }
    }

    TopoDS_Wire _findClosedWires(VertexInfo beginVertex,
                                 VertexInfo currentVertex,
                                 std::shared_ptr<WireInfo> wireInfo = std::shared_ptr<WireInfo>(),
                                 int *idxVertex = nullptr,
                                 int *stackPos = nullptr)
    {
        Base::SequencerBase::Instance().checkAbort();
        EdgeInfo &beginInfo = *beginVertex.it;

        EdgeInfo *currentInfo = currentVertex.edgeInfo();
        int currentIdx = currentVertex.start ? 1 : 0;
        currentInfo->iteration = iteration;

        gp_Pnt pstart = beginVertex.pt();
        gp_Pnt pend = currentVertex.ptOther();

        auto stackEnd = stack.size();
        checkStack();

        // pstart and pend is the start and end vertex of the current wire
        while (true) {
            // push a new stack entry
            stack.emplace_back(vertexStack.size());
            auto &r = stack.back();
            showShape(currentInfo, "check", iteration);

            bool proceed = true;

            // The loop below is to find all edges connected to pend, and save them into stack.back()
            auto size = vertexStack.size();
            for (int i=currentInfo->iStart[currentIdx];i<currentInfo->iEnd[currentIdx];++i) {
                auto &vinfo = adjacentList[i];
                auto &info = *vinfo.it;
                if (info.iteration < 0 || currentInfo == &info)
                    continue;

                bool abort = false;
                if (!wireSet.empty() && wireSet.contains(info.wireInfo.get())) {
                    showShape(&info, "wired", iteration);
                    if (wireInfo)
                        wireInfo->purge = true;
                    abort = true;
                }

                if (edgeSet.contains(&info)) {
                    showShape(&info, "intersect", iteration);
                    // This means the current edge connects to an
                    // existing edge in the middle of the stack.
                    // skip the current edge.
                    r.iEnd = r.iStart;
                    vertexStack.resize(size);
                    break;
                }

                if (abort || currentInfo->wireInfo2) {
                    if (wireInfo)
                        wireInfo->purge = true;
                    continue;
                }

                if (info.iteration == iteration)
                    continue;
                info.iteration = iteration;

                if (wireInfo) {
                    // We may be called by findTightBound() with an existing wire
                    // to try to find a new wire by splitting the current one. So
                    // check if we've iterated to some edge in the existing wire.
                    if (int idx = wireInfo->find(vinfo)) {
                        vertexStack.push_back(adjacentList[i]);
                        r.iCurrent = r.iEnd++;
                        --idx;
                        proceed = false;
                        if (idxVertex)
                            *idxVertex = idx;
                        if (stackPos)
                            *stackPos = (int)stack.size()-2;

                        auto info = wireInfo->vertices[idx].edgeInfo();
                        showShape(info, "merge", iteration);

                        if (info != &beginInfo) {
                            while (true) {
                                if (++idx == (int)wireInfo->vertices.size())
                                    idx = 0;
                                info = wireInfo->vertices[idx].edgeInfo();
                                if (info == &beginInfo)
                                    break;
                                stack.emplace_back(vertexStack.size());
                                vertexStack.push_back(wireInfo->vertices[idx]);
                                ++stack.back().iEnd;
                                checkStack();
                            }
                        }
                        break;
                    }

                    if (wireInfo->find(VertexInfo(vinfo.it, !vinfo.start))) {
                        showShape(&info, "rintersect", iteration);
                        // Only used when exhausting tight bound.
                        wireInfo->purge = true; 
                        continue;
                    }

                    if (isOutside(*wireInfo, info.mid)) {
                        showShape(&info, "outside", iteration);
                        continue;
                    }
                }
                vertexStack.push_back(adjacentList[i]);
                ++r.iEnd;
            }
            checkStack();

            if (proceed) {
                while (true) {
                    auto &r = stack.back();
                    if (r.iCurrent<r.iEnd) {
                        // now pick one edge from stack.back(), connect it to
                        // pend, then extend pend
                        currentVertex = vertexStack[r.iCurrent];
                        pend = currentVertex.ptOther();
                        // update current edge info
                        currentInfo = currentVertex.edgeInfo();
                        showShape(currentInfo, "iterate", iteration);
                        currentIdx = currentVertex.start?1:0;
                        edgeSet.insert(currentInfo);
                        if (!wireSet.empty())
                            wireSet.insert(currentInfo->wireInfo.get());
                        break;
                    }
                    vertexStack.erase(vertexStack.begin()+r.iStart,vertexStack.end());

                    stack.pop_back();
                    if (stack.size() == stackEnd) {
                        // If stack reaches the end, it means this wire is open.
                        return TopoDS_Wire();
                    }

                    auto &lastInfo = *vertexStack[stack.back().iCurrent].it;
                    edgeSet.erase(&lastInfo);
                    wireSet.erase(lastInfo.wireInfo.get());
                    showShape(&lastInfo, "pop", iteration);
                    ++stack.back().iCurrent;
                }

                if (pstart.SquareDistance(pend) > myTol2) {
                    // if the wire is not closed yet, continue search for the
                    // next connected edge
                    continue;
                }
                if (wireInfo) {
                    if (idxVertex)
                        *idxVertex = (int)wireInfo->vertices.size();
                    if (stackPos)
                        *stackPos = (int)stack.size()-1;
                }
            }

            wireData->Clear();
            wireData->Add(beginInfo.shape(beginVertex.start));
            for (auto &r : stack) {
                const auto &v = vertexStack[r.iCurrent];
                auto &info = *v.it;
                wireData->Add(info.shape(v.start));
            }
            TopoDS_Wire wire = makeCleanWire();
            if (!BRep_Tool::IsClosed(wire)) {
                FC_WARN("failed to close some wire in iteration " << iteration);
                showShape(wire,"_FailedToClose", iteration);
                showShape(beginInfo.shape(beginVertex.start), "failed", iteration);
                for (auto &r : stack) {
                    const auto &v = vertexStack[r.iCurrent];
                    auto &info = *v.it;
                    showShape(info.shape(v.start), v.start ? "failed" : "failed_r", iteration);
                }
                assertCheck(false);
                continue;
            }
            return wire;
        }
    }

    void findTightBound()
    {
        // Assumption: all edges lies on a common manifold surface
        //
        // Definition of 'Tight Bound': a wire that cannot be splitted into
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
            if (info.iteration < 0 || !info.wireInfo)
                continue;

            ++iteration2;
            while(!info.wireInfo->done) {
                auto wireInfo = info.wireInfo;
                checkWireInfo(*wireInfo);
                const auto &wireVertices = wireInfo->vertices;
                auto beginVertex = wireVertices.front();
                auto &beginInfo = *beginVertex.it;
                initWireInfo(*wireInfo);
                showShape(wireInfo->wire, "iwire", iteration);
                for (auto &v : wireVertices)
                    v.it->iteration2 = iteration2;

                stack.clear();
                vertexStack.clear();
                edgeSet.clear();

                std::shared_ptr<WireInfo> newWire;
                gp_Pnt pstart = beginVertex.pt();

                int idxV = 0;
                while (true) {
                    int idx = wireVertices[idxV].start ? 1 : 0;
                    auto current = wireVertices[idxV].edgeInfo();
                    showShape(current, "current", iteration);

                    for (int n=current->iStart[idx]; n<current->iEnd[idx]; ++n) {
                        const auto &currentVertex = adjacentList[n];
                        auto next = currentVertex.edgeInfo();
                        if (next == current || next->iteration2 == iteration2 || next->iteration<0)
                            continue;

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
                        checkStack();

                        int idxEnd = (int)wireVertices.size();
                        int stackStart = (int)stack.size()-1;
                        int stackPos = (int)stack.size()-1;

                        TopoDS_Wire wire;
                        if (pstart.SquareDistance(currentVertex.ptOther()) > myTol2) {
                            wire = _findClosedWires(beginVertex, currentVertex, beginInfo.wireInfo, &idxEnd, &stackPos);
                            if (wire.IsNull()) {
                                vertexStack.pop_back();
                                stack.pop_back();
                                edgeSet.erase(next);
                                continue;
                            }
                        }

                        newWire.reset(new WireInfo);
                        auto &newWireVertices = newWire->vertices;
                        newWireVertices.push_back(beginVertex);
                        for (auto &r : stack) {
                            const auto &v = vertexStack[r.iCurrent];
                            newWireVertices.push_back(v);
                        }
                        if (!wire.IsNull())
                            newWire->wire = wire;
                        else if (!initWireInfo(*newWire)) {
                            newWire.reset();
                            vertexStack.pop_back();
                            stack.pop_back();
                            edgeSet.erase(next);
                            continue;
                        }
                        for (auto &v : newWire->vertices) {
                            if (v.edgeInfo()->wireInfo == wireInfo)
                                v.edgeInfo()->wireInfo = newWire;
                        }
                        beginInfo.wireInfo = newWire;
                        showShape(*newWire, "nwire", iteration);

                        std::shared_ptr<WireInfo> splitWire;
                        if (idxEnd == 0)
                            idxEnd = (int)wireVertices.size();
                        ++idxV;
                        assertCheck(idxV<=idxEnd);
                        int idxStart = idxV;
                        for (int idx=idxV; idx!=idxEnd; ++idx) {
                            auto info = wireVertices[idx].edgeInfo();
                            if (info == &beginInfo) {
                                showShape(*wireInfo, "exception", iteration, true);
                                showShape(info, "exception", iteration, true);
                                assertCheck(info != &beginInfo);
                            }
                            if (info->wireInfo == wireInfo) {
                                if (!splitWire) {
                                    idxStart = idx;
                                    splitWire.reset(new WireInfo);
                                }
                                info->wireInfo = splitWire;
                            }
                        }
                        if (splitWire) {
                            auto &splitEdges = splitWire->vertices;
                            gp_Pnt pstart, pt;
                            bool first = true;
                            for (int idx=idxStart; idx!=idxEnd; ++idx) {
                                auto &v = wireVertices[idx];
                                if (first) {
                                    first = false;
                                    pstart = v.pt();
                                } else
                                    assertCheck(pt.SquareDistance(v.pt()) < myTol2);
                                pt = v.ptOther();
                                splitEdges.push_back(v);
                            }
                            for (int i=stackPos; i>=stackStart; --i) {
                                const auto &v = vertexStack[stack[i].iCurrent];
                                assertCheck(pt.SquareDistance(v.ptOther()) < myTol2);
                                pt = v.pt();
                                // The edges in the stack are the ones to slice
                                // the wire in half. We construct a new wire
                                // that includes the original beginning edge in
                                // the loop above. And this loop contains the
                                // other half. Note that the slicing edges
                                // should run in the oppsite direction, hence reversed
                                splitEdges.push_back(v.reversed());
                            }
                            for (int idx=idxV; idx!=idxStart; ++idx) {
                                auto &v = wireVertices[idx];
                                assertCheck(pt.SquareDistance(v.pt()) < myTol2);
                                pt = v.ptOther();
                                splitEdges.push_back(v);
                            }
                            assertCheck(pt.SquareDistance(pstart) < myTol2);
                            showShape(*splitWire, "swire", iteration);
                        }

                        checkWireInfo(*newWire);
                        break;
                    }
                    if (newWire) {
                        ++iteration;
                        break;
                    }

                    if (++idxV == (int)wireVertices.size())
                        break;
                    stack.emplace_back(vertexStack.size());
                    ++stack.back().iEnd;
                    vertexStack.push_back(wireVertices[idxV]);
                    edgeSet.insert(wireVertices[idxV].edgeInfo());
                    checkStack();
                }

                if (!newWire) {
                    showShape(*beginInfo.wireInfo, "done", iteration);
                    beginInfo.wireInfo->done = true;
                    // If a wire is done, make sure all edges of this wire is
                    // marked as done. This can also prevent duplicated wires.
                    for (auto &v : beginInfo.wireInfo->vertices) {
                        auto info = v.edgeInfo();
                        if (!info->wireInfo) {
                            info->wireInfo = beginInfo.wireInfo;
                            continue;
                        }
                        else if (info->wireInfo->done)
                            continue;
                        auto otherWire = info->wireInfo;
                        auto &otherWireVertices = info->wireInfo->vertices;
                        if (info == otherWireVertices.front().edgeInfo()) {
                            // About to change the first edge of the other wireInfo.
                            // Try to find a new first edge for it.
                            tmpVertices.clear();
                            auto it = otherWireVertices.begin();
                            tmpVertices.push_back(*it);
                            for (++it;it!=otherWireVertices.end();++it) {
                                if (it->edgeInfo()->wireInfo == otherWire)
                                    break;
                                tmpVertices.push_back(*it);
                            }
                            if (tmpVertices.size() != otherWireVertices.size()) {
                                otherWireVertices.erase(otherWireVertices.begin(), it);
                                otherWireVertices.insert(otherWireVertices.end(),
                                        tmpVertices.begin(), tmpVertices.end());
                            }
                        }
                        assertCheck(info != &beginInfo);
                        info->wireInfo = beginInfo.wireInfo;
                        checkWireInfo(*otherWire);
                    }
                    checkWireInfo(*beginInfo.wireInfo);
                }
            }
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
            if (info.iteration < 0 || !info.wireInfo || !info.wireInfo->done)
                continue;
            for (auto &v : info.wireInfo->vertices) {
                auto edgeInfo = v.edgeInfo();
                if (edgeInfo->wireInfo != info.wireInfo)
                    edgeInfo->wireInfo2 = info.wireInfo;
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
                if (info.wireInfo)
                    showShape(*info.wireInfo, "iskip");
                else
                    showShape(&info, "iskip");
                continue;
            }

            if (info.wireInfo2 && info.wireInfo2->done) {
                showShape(*info.wireInfo, "idone");
                continue;
            }

            showShape(*info.wireInfo, "iwire2", iteration);
            showShape(&info, "begin2", iteration);

            int idx = info.wireInfo->find(&info);
            assertCheck(idx > 0);
            const auto &vertices = info.wireInfo->vertices;
            --idx;
            int nextIdx = idx == (int)vertices.size()-1 ? 0 : idx + 1;
            int prevIdx = idx == 0 ? (int)vertices.size()-1 : idx - 1;
            int count = prevIdx == nextIdx ? 1 : 2;
            for (int n=0; n<count && !info.wireInfo2; ++n) {
                auto check = vertices[n==0 ? nextIdx : prevIdx].edgeInfo();
                auto beginVertex = vertices[idx];
                if (n == 1)
                    beginVertex.start = !beginVertex.start;
                const gp_Pnt &pstart = beginVertex.pt();
                int vidx = beginVertex.start ? 1 : 0;

                edgeSet.clear();
                vertexStack.clear();
                stack.clear();
                stack.emplace_back();

                for (int i=info.iStart[vidx]; i<info.iEnd[vidx]; ++i) {
                    const auto &currentVertex = adjacentList[i];
                    auto next = currentVertex.edgeInfo();
                    if (next == &info
                            || next == check
                            || next->iteration<0
                            || !next->wireInfo
                            || !next->wireInfo->done
                            || next->wireInfo2)
                        continue;

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
                        if (wire.IsNull())
                            continue;
                    }

                    std::shared_ptr<WireInfo> wireInfo(new WireInfo);
                    wireInfo->vertices.push_back(beginVertex);
                    for (auto &r : stack) {
                        const auto &v = vertexStack[r.iCurrent];
                        wireInfo->vertices.push_back(v);
                    }
                    if (!wire.IsNull())
                        wireInfo->wire = wire;
                    else if (!initWireInfo(*wireInfo))
                        continue;

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

                        const auto &wireVertices = wireInfo->vertices;
                        initWireInfo(*wireInfo);
                        for (auto &v : wireVertices)
                            v.it->iteration2 = iteration2;

                        std::shared_ptr<WireInfo> newWire;

                        int idxV = 1;
                        while (true) {
                            int idx = wireVertices[idxV].start ? 1 : 0;
                            auto current = wireVertices[idxV].edgeInfo();

                            for (int n=current->iStart[idx]; n<current->iEnd[idx]; ++n) {
                                const auto &currentVertex = adjacentList[n];
                                auto next = currentVertex.edgeInfo();
                                if (next == current || next->iteration2 == iteration2 || next->iteration<0)
                                    continue;

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
                                checkStack();

                                TopoDS_Wire wire;
                                if (pstart.SquareDistance(currentVertex.ptOther()) > myTol2) {
                                    wire = _findClosedWires(beginVertex, currentVertex, wireInfo);
                                    if (wire.IsNull()) {
                                        vertexStack.pop_back();
                                        stack.pop_back();
                                        edgeSet.erase(next);
                                        wireSet.erase(next->wireInfo.get());
                                        continue;
                                    }
                                }

                                newWire.reset(new WireInfo);
                                auto &newWireVertices = newWire->vertices;
                                newWireVertices.push_back(beginVertex);
                                for (auto &r : stack) {
                                    const auto &v = vertexStack[r.iCurrent];
                                    newWireVertices.push_back(v);
                                }
                                if (!wire.IsNull())
                                    newWire->wire = wire;
                                else if (!initWireInfo(*newWire)) {
                                    newWire.reset();
                                    vertexStack.pop_back();
                                    stack.pop_back();
                                    edgeSet.erase(next);
                                    wireSet.erase(next->wireInfo.get());
                                    continue;
                                }
                                for (auto &v : newWire->vertices) {
                                    if (v.edgeInfo()->wireInfo == wireInfo)
                                        v.edgeInfo()->wireInfo = newWire;
                                }
                                showShape(*newWire, "nwire2", iteration);
                                checkWireInfo(*newWire);
                                break;
                            }

                            if (newWire) {
                                ++iteration;
                                wireInfo = newWire;
                                break;
                            }

                            if (++idxV == (int)wireVertices.size()) {
                                if (wireInfo->purge) {
                                    showShape(*wireInfo, "discard2", iteration);
                                    wireInfo.reset();
                                } else {
                                    wireInfo->done = true;
                                    showShape(*wireInfo, "done2", iteration);
                                }
                                break;
                            }
                            stack.emplace_back(vertexStack.size());
                            ++stack.back().iEnd;
                            vertexStack.push_back(wireVertices[idxV]);
                            edgeSet.insert(wireVertices[idxV].edgeInfo());
                            checkStack();
                        }
                    }

                    if (wireInfo && wireInfo->done) {
                        for (auto &v : wireInfo->vertices) {
                            auto edgeInfo = v.edgeInfo();
                            assertCheck(edgeInfo->wireInfo != nullptr);
                            if (edgeInfo->wireInfo->isSame(*wireInfo)) {
                                wireInfo = edgeInfo->wireInfo;
                                break;
                            }
                        }
                        for (auto &v : wireInfo->vertices) {
                            auto edgeInfo = v.edgeInfo();
                            if (!edgeInfo->wireInfo2 && edgeInfo->wireInfo != wireInfo)
                                edgeInfo->wireInfo2 = wireInfo;
                        }
                        assertCheck(info.wireInfo2 == wireInfo);
                        assertCheck(info.wireInfo2 != info.wireInfo);
                        showShape(*wireInfo, "exhaust");
                        break;
                    }
                }
            }
        }
        wireSet.clear();
    }

    TopoDS_Wire makeCleanWire(bool fixGap=true)
    {
        // Make a clean wire with sorted, oriented, connected, etc edges
        TopoDS_Wire result;
        std::vector<TopoShape> inputEdges;

        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE)) {
            for (int i=1; i<=wireData->NbEdges(); ++i)
                inputEdges.emplace_back(wireData->Edge(i));
        }

        ShapeFix_Wire fixer;
        fixer.SetContext(new ShapeBuild_ReShape);
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

        if (FC_LOG_INSTANCE.level()>FC_LOGLEVEL_TRACE+1) {
            FC_MSG("init:");
            for (const auto &s : sourceEdges)
                FC_MSG(s.getShape().TShape().get() << ", " << s.getShape().HashCode(INT_MAX));
            printHistory(aHistory, sourceEdges);
            printHistory(newHistory, inputEdges);
        }

        aHistory->Merge(newHistory);

        if (FC_LOG_INSTANCE.level()>FC_LOGLEVEL_TRACE+1) {
            printHistory(aHistory, sourceEdges);
            FC_MSG("final:");
            for (int i=1; i<=wireData->NbEdges(); ++i) {
                auto s = wireData->Edge(i);
                FC_MSG(s.TShape().get() << ", " << s.HashCode(INT_MAX));
            }
        }
        return result;
    }

    template<class T>
    void printHistory(Handle(BRepTools_History) hist, const T &input)
    {
        FC_MSG("\nHistory:\n");
        for (const auto &s : input) {
            for(TopTools_ListIteratorOfListOfShape it(hist->Modified(s.getShape())); it.More(); it.Next()) {
                FC_MSG(s.getShape().TShape().get() << ", " << s.getShape().HashCode(INT_MAX)
                        << " -> " << it.Value().TShape().get() << ", " << it.Value().HashCode(INT_MAX));
            }
        }
    }

    bool canShowShape(int idx=-1, bool forced=false)
    {
        if (idx < 0 || catchIteration == 0 || catchIteration > idx) {
            if (!forced && FC_LOG_INSTANCE.level()<=FC_LOGLEVEL_TRACE)
                return false;
        }
        return true;
    }
    
    void showShape(const EdgeInfo *info, const char *name, int idx=-1, bool forced=false)
    {
        if (!canShowShape(idx, forced))
            return;
        showShape(info->shape(), name, idx, forced);
    }

    void showShape(WireInfo &wireInfo, const char *name, int idx=-1, bool forced=false)
    {
        if (!canShowShape(idx, forced))
            return;
        if (wireInfo.wire.IsNull())
            initWireInfo(wireInfo);
        showShape(wireInfo.wire, name, idx, forced);
    }

    void showShape(const TopoDS_Shape &s, const char *name, int idx=-1, bool forced=false)
    {
        if (!canShowShape(idx, forced))
            return;
        std::string _name;
        if (idx >= 0) {
            _name = name;
            _name += "_";
            _name += std::to_string(idx);
            _name += "_";
            name = _name.c_str();
        }
        auto obj = Feature::create(s, name);
        FC_MSG(obj->getNameInDocument() << " " << ShapeHasher()(s));
        if (catchObject == obj->getNameInDocument())
            FC_MSG("found");
        return;
    }

    void build()
    {
        clear();
        sourceEdges.clear();
        sourceEdges.insert(sourceEdgeArray.begin(), sourceEdgeArray.end());
        for (const auto &e : sourceEdgeArray)
            add(TopoDS::Edge(e.getShape()), true);

        if (doTightBound || doSplitEdge)
            splitEdges();

        buildAdjacentList();

        if (!doTightBound && !doOutline)
            findClosedWires();
        else {
            findClosedWires(true);
            findTightBound();
            exhaustTightBound();
            bool done = !doOutline;
            while(!done) {
                ++iteration;
                done = true;
                std::unordered_map<EdgeInfo*, int> counter;
                std::unordered_set<WireInfo*> wires;
                for (auto &info : edges) {
                    if (info.iteration == -2)
                        continue;
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
                        for (auto &v : info.wireInfo2->vertices) {
                            if (++counter[v.edgeInfo()] == 2) {
                                v.edgeInfo()->iteration = -1;
                                done = false;
                                showShape(v.edgeInfo(), "removed2", iteration);
                                aHistory->Remove(info.edge);
                            }
                        }
                    }
                    if (!wires.insert(info.wireInfo.get()).second)
                        continue;
                    for (auto &v : info.wireInfo->vertices) {
                        if (++counter[v.edgeInfo()] == 2) {
                            v.edgeInfo()->iteration = -1;
                            done = false;
                            showShape(v.edgeInfo(), "removed1", iteration);
                            aHistory->Remove(info.edge);
                        }
                    }
                }
                findClosedWires(true);
                findTightBound();
            }

            builder.MakeCompound(compound);
            wireSet.clear();
            for (auto &info : edges) {
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
        if (!wireInfo || !wireInfo->done || !wireSet.insertUnique(wireInfo.get()))
            return;
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
                for (const auto &e : it->getSubShapes(TopAbs_EDGE)) {
                    if (source.findSubShapesWithSharedVertex(TopoShape(e, -1)).empty()) {
                        purge = false;
                        break;
                    }
                }
                if (purge) {
                    it = wires.erase(it);
                    touched = true;
                } else
                    ++it;
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
    :pimpl(new WireJoinerP)
{
}

WireJoiner::~WireJoiner()
{
}

void WireJoiner::addShape(const TopoShape &shape)
{
    NotDone();
    for (auto &e : shape.getSubTopoShapes(TopAbs_EDGE))
        pimpl->sourceEdgeArray.push_back(e);
}

void WireJoiner::addShape(const std::vector<TopoShape> &shapes)
{
    NotDone();
    for (const auto &shape : shapes) {
        for (auto &e : shape.getSubTopoShapes(TopAbs_EDGE))
            pimpl->sourceEdgeArray.push_back(e);
    }
}

void WireJoiner::addShape(const std::vector<TopoDS_Shape> &shapes)
{
    NotDone();
    for (const auto &shape : shapes) {
        for (TopExp_Explorer xp(shape,TopAbs_EDGE); xp.More(); xp.Next())
            pimpl->sourceEdgeArray.emplace_back(TopoDS::Edge(xp.Current()), -1);
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
#else
void WireJoiner::Build(const Message_ProgressRange&)
#endif
{
    if (IsDone())
        return;
    pimpl->build();
    if (TopoShape(pimpl->compound).countSubShapes(TopAbs_SHAPE) > 0)
        myShape = pimpl->compound;
    else
        myShape.Nullify();
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

const TopTools_ListOfShape& WireJoiner::Generated (const TopoDS_Shape& S)
{
    Build();
    return pimpl->aHistory->Generated(S);
}

const TopTools_ListOfShape& WireJoiner::Modified (const TopoDS_Shape& S)
{
    Build();
    return pimpl->aHistory->Modified(S);
}

Standard_Boolean WireJoiner::IsDeleted (const TopoDS_Shape& S)
{
    Build();
    return pimpl->aHistory->IsRemoved(S);
}
