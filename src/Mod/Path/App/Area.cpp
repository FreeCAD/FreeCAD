/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef _PreComp_
#endif

#include <boost/version.hpp>
#include <boost/config.hpp>
#if defined(BOOST_MSVC) && (BOOST_VERSION == 105500)
// for fixing issue https://svn.boost.org/trac/boost/ticket/9332
#   include "boost_fix/intrusive/detail/memory_util.hpp"
#   include "boost_fix/container/detail/memory_util.hpp"
#endif
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <GeomAbs_JoinType.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <gp_Circ.hxx>
#include <gp_GTrsf.hxx>
#include <Standard_Version.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <BRepBndLib.hxx>
#include <BRepLib_MakeFace.hxx>
#include <Bnd_Box.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <HLRBRep.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRAlgo_Projector.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FaceMakerBullseye.h>
#include <Mod/Part/App/CrossSection.h>
#include "Area.h"
#include "../libarea/Area.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bgi::linear<16> RParameters;

BOOST_GEOMETRY_REGISTER_POINT_3D_GET_SET(
        gp_Pnt,double,bg::cs::cartesian,X,Y,Z,SetX,SetY,SetZ)

using namespace Path;

CAreaParams::CAreaParams()
    :PARAM_INIT(PARAM_FNAME,AREA_PARAMS_CAREA)
{}

AreaParams::AreaParams()
    :PARAM_INIT(PARAM_FNAME,AREA_PARAMS_AREA)
{}

CAreaConfig::CAreaConfig(const CAreaParams &p, bool noFitArcs)
{
#define AREA_CONF_SAVE_AND_APPLY(_param) \
    PARAM_FNAME(_param) = BOOST_PP_CAT(CArea::get_,PARAM_FARG(_param))();\
    BOOST_PP_CAT(CArea::set_,PARAM_FARG(_param))(p.PARAM_FNAME(_param));

    PARAM_FOREACH(AREA_CONF_SAVE_AND_APPLY,AREA_PARAMS_CAREA)

    // Arc fitting is lossy. We shall reduce the number of unnecessary fit
    if(noFitArcs)
        CArea::set_fit_arcs(false);

}

CAreaConfig::~CAreaConfig() {

#define AREA_CONF_RESTORE(_param) \
    BOOST_PP_CAT(CArea::set_,PARAM_FARG(_param))(PARAM_FNAME(_param));

    PARAM_FOREACH(AREA_CONF_RESTORE,AREA_PARAMS_CAREA)
}

//////////////////////////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(Path::Area, Base::BaseClass);

bool Area::s_aborting;

Area::Area(const AreaParams *params)
:myParams(s_params)
,myHaveFace(false)
,myHaveSolid(false)
,myShapeDone(false)
{
    if(params)
        setParams(*params);
}

Area::Area(const Area &other, bool deep_copy)
:Base::BaseClass(other)
,myShapes(other.myShapes)
,myTrsf(other.myTrsf)
,myParams(other.myParams)
,myWorkPlane(other.myWorkPlane)
,myHaveFace(other.myHaveFace)
,myHaveSolid(other.myHaveSolid)
,myShapeDone(false)
{
    if(!deep_copy || !other.isBuilt())
        return;
    if(other.myArea)
        myArea.reset(new CArea(*other.myArea));
    myShapePlane = other.myShapePlane;
    myShape = other.myShape;
    myShapeDone = other.myShapeDone;
    mySections.reserve(other.mySections.size());
    for(shared_ptr<Area> area:mySections)
        mySections.push_back(make_shared<Area>(*area,true));
}

Area::~Area() {
    clean();
}

void Area::setPlane(const TopoDS_Shape &shape) {
    clean();
    if(shape.IsNull()) {
        myWorkPlane.Nullify();
        return;
    }
    gp_Trsf trsf;
    TopoDS_Shape plane = findPlane(shape,trsf);
    if (plane.IsNull())
        throw Base::ValueError("shape is not planar");
    myWorkPlane = plane;
    myTrsf = trsf;
}

bool Area::isCoplanar(const TopoDS_Shape &s1, const TopoDS_Shape &s2) {
    if(s1.IsNull() || s2.IsNull()) return false;
    if(s1.IsEqual(s2)) return true;
    TopoDS_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    builder.Add(comp,s1);
    builder.Add(comp,s2);
    BRepLib_FindSurface planeFinder(comp,-1,Standard_True);
    return planeFinder.Found();
}

int Area::addShape(CArea &area, const TopoDS_Shape &shape, const gp_Trsf *trsf, 
                double deflection, const TopoDS_Shape *plane, bool force_coplanar,
                CArea *areaOpen, bool to_edges, bool reorient) 
{
    bool haveShape = false;
    int skipped = 0;
    for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
        haveShape = true;
        const TopoDS_Face &face = TopoDS::Face(it.Current());
        if(plane && !isCoplanar(face,*plane)) {
            ++skipped;
            if(force_coplanar) continue;
        }
        for (TopExp_Explorer it(face, TopAbs_WIRE); it.More(); it.Next())
            addWire(area,TopoDS::Wire(it.Current()),trsf,deflection);
    }

    if(haveShape) return skipped;

    CArea _area;
    CArea _areaOpen;

    for (TopExp_Explorer it(shape, TopAbs_WIRE); it.More(); it.Next()) {
        haveShape = true;
        const TopoDS_Wire &wire = TopoDS::Wire(it.Current());
        if(plane && !isCoplanar(wire,*plane)) {
            ++skipped;
            if(force_coplanar) continue;
        }
        if(BRep_Tool::IsClosed(wire))
            addWire(_area,wire,trsf,deflection);
        else if(to_edges) {
            for (TopExp_Explorer it(wire, TopAbs_EDGE); it.More(); it.Next())
                addWire(_areaOpen,BRepBuilderAPI_MakeWire(
                    TopoDS::Edge(it.Current())).Wire(),trsf,deflection,true);
        }else
            addWire(_areaOpen,wire,trsf,deflection);
    }

    if(!haveShape) {
        for (TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next()) {
            if(plane && !isCoplanar(it.Current(),*plane)) {
                ++skipped;
                if(force_coplanar) continue;
            }
            TopoDS_Wire wire = BRepBuilderAPI_MakeWire(
                                    TopoDS::Edge(it.Current())).Wire();
            addWire(BRep_Tool::IsClosed(wire)?_area:_areaOpen,wire,trsf,deflection);
        }
    }

    if(reorient)
        _area.Reorder();
    area.m_curves.splice(area.m_curves.end(),_area.m_curves);
    if(areaOpen)
        areaOpen->m_curves.splice(areaOpen->m_curves.end(),_areaOpen.m_curves);
    else
        area.m_curves.splice(area.m_curves.end(),_areaOpen.m_curves);
    return skipped;
}

void Area::addWire(CArea &area, const TopoDS_Wire& wire,
        const gp_Trsf *trsf, double deflection, bool to_edges) 
{
    CCurve ccurve;
    BRepTools_WireExplorer xp(trsf?TopoDS::Wire(
                wire.Moved(TopLoc_Location(*trsf))):wire);

    gp_Pnt p = BRep_Tool::Pnt(xp.CurrentVertex());
    ccurve.append(CVertex(Point(p.X(),p.Y())));

    for (;xp.More();xp.Next()) {
        const TopoDS_Edge &edge = TopoDS::Edge(xp.Current());
        BRepAdaptor_Curve curve(edge);
        bool reversed = (xp.Current().Orientation()==TopAbs_REVERSED);

        p = curve.Value(reversed?curve.FirstParameter():curve.LastParameter());

        switch (curve.GetType()) {
        case GeomAbs_Line: {
            ccurve.append(CVertex(Point(p.X(),p.Y())));
            if(to_edges) {
                area.append(ccurve);
                ccurve.m_vertices.pop_front();
            }
            break;
        } case GeomAbs_Circle:{
            if(!to_edges) {
                double first = curve.FirstParameter();
                double last = curve.LastParameter();
                gp_Circ circle = curve.Circle();
                gp_Dir dir = circle.Axis().Direction();
                gp_Pnt center = circle.Location();
                int type = dir.Z()<0?-1:1;
                if(reversed) type = -type;
                if(fabs(first-last)>M_PI) {
                    // Split arc(circle) larger than half circle. Because gcode
                    // can't handle full circle?
                    gp_Pnt mid = curve.Value((last-first)*0.5+first);
                    ccurve.append(CVertex(type,Point(mid.X(),mid.Y()),
                                Point(center.X(),center.Y())));
                }
                ccurve.append(CVertex(type,Point(p.X(),p.Y()),
                            Point(center.X(),center.Y())));
                break;
            }
            //fall through
        } default: {
            // Discretize all other type of curves
            GCPnts_QuasiUniformDeflection discretizer(curve, deflection, 
                    curve.FirstParameter(), curve.LastParameter());
            if (discretizer.IsDone () && discretizer.NbPoints () > 1) {
                int nbPoints = discretizer.NbPoints ();
                //strangly OCC discretizer points are one-based, not zero-based, why?
                if(reversed) {
                    for (int i=nbPoints-1; i>=1; --i) {
                        gp_Pnt pt = discretizer.Value (i);
                        ccurve.append(CVertex(Point(pt.X(),pt.Y())));
                        if(to_edges) {
                            area.append(ccurve);
                            ccurve.m_vertices.pop_front();
                        }
                    }
                }else{
                    for (int i=2; i<=nbPoints; i++) {
                        gp_Pnt pt = discretizer.Value (i);
                        ccurve.append(CVertex(Point(pt.X(),pt.Y())));
                        if(to_edges) {
                            area.append(ccurve);
                            ccurve.m_vertices.pop_front();
                        }
                    }
                }

            }else
                Standard_Failure::Raise("Curve discretization failed");
        }}
    }
    if(!to_edges) {
        if(BRep_Tool::IsClosed(wire) && !ccurve.IsClosed()) {
            AREA_WARN("ccurve not closed");
            ccurve.append(ccurve.m_vertices.front());
        }
        area.append(ccurve);
    }
}


void Area::clean(bool deleteShapes) {
    myShapeDone = false;
    mySections.clear();
    myShape.Nullify();
    myArea.reset();
    myAreaOpen.reset();
    myShapePlane.Nullify();
    if(deleteShapes){
        myShapes.clear();
        myHaveFace = false;
        myHaveSolid = false;
    }
}

static inline ClipperLib::ClipType toClipperOp(short op) {
    switch(op){
    case Area::OperationUnion:
        return ClipperLib::ctUnion;
        break;
    case Area::OperationDifference:
        return ClipperLib::ctDifference;
        break;
    case Area::OperationIntersection:
        return ClipperLib::ctIntersection;
        break;
    case Area::OperationXor:
        return ClipperLib::ctXor;
        break;
    default:
        throw Base::ValueError("invalid Operation");
    }
}

void Area::add(const TopoDS_Shape &shape,short op) {

    if(shape.IsNull())
        throw Base::ValueError("null shape");

    if(op!=OperationCompound) 
        toClipperOp(op);

    bool haveSolid = false;
    for(TopExp_Explorer it(shape, TopAbs_SOLID);it.More();) {
        haveSolid = true;
        break;
    }
    //TODO: shall we support Shells?
    if((!haveSolid && myHaveSolid) ||
        (haveSolid && !myHaveSolid && !myShapes.empty()))
        throw Base::ValueError("mixing solid and planar shapes is not allowed");

    myHaveSolid = haveSolid;

    clean();
    if(op!=OperationCompound && myShapes.empty())
        op = OperationUnion;
    myShapes.push_back(Shape(op,shape));
}


void Area::setParams(const AreaParams &params) {
#define AREA_SRC(_param) params.PARAM_FNAME(_param)
    // Validate all enum type of parameters
    PARAM_ENUM_CHECK(AREA_SRC,PARAM_ENUM_EXCEPT,AREA_PARAMS_CONF);
    if(params!=myParams) {
        clean();
        myParams = params;
    }
}

void Area::addToBuild(CArea &area, const TopoDS_Shape &shape) {
    if(myParams.Fill==FillAuto && !myHaveFace) {
        TopExp_Explorer it(shape, TopAbs_FACE);
        myHaveFace = it.More();
    }
    TopoDS_Shape plane = getPlane();
    CArea areaOpen;
    mySkippedShapes += addShape(area,shape,&myTrsf,myParams.Deflection,
            myParams.Coplanar==CoplanarNone?NULL:&plane,
            myHaveSolid||myParams.Coplanar==CoplanarForce,&areaOpen,
            myParams.OpenMode==OpenModeEdges,myParams.Reorient);
    if(areaOpen.m_curves.size()) {
        if(&area == myArea.get() || myParams.OpenMode == OpenModeNone)
            myAreaOpen->m_curves.splice(myAreaOpen->m_curves.end(),areaOpen.m_curves);
        else
            AREA_WARN("open wires discarded in clipping shapes");
    }
}

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


struct WireJoiner {

    typedef bg::model::box<gp_Pnt> Box;

    static bool getBBox(const TopoDS_Edge &e, Box &box) {
        Bnd_Box bound;
        BRepBndLib::Add(e,bound);
        bound.SetGap(0.1);
        if (bound.IsVoid()) {
            if(Area::TraceEnabled())
                AREA_WARN("failed to get bound of edge");
            return false;
        }
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bound.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        box = Box(gp_Pnt(xMin,yMin,zMin), gp_Pnt(xMax,yMax,zMax));
        return true;
    }

    struct EdgeInfo {
        TopoDS_Edge edge;
        gp_Pnt p1;
        gp_Pnt p2;
        Box box;
        int iteration;
        bool used;
        bool hasBox;
        EdgeInfo(const TopoDS_Edge &e, bool bbox)
            :edge(e),iteration(0) ,used(false),hasBox(false)
        {
            getEndPoints(e,p1,p2);
            if(bbox) hasBox= getBBox(e,box);
        }
        EdgeInfo(const TopoDS_Edge &e, const gp_Pnt &pt1, 
                const gp_Pnt &pt2, bool bbox)
            :edge(e),p1(pt1),p2(pt2),iteration(0)
            ,used(false),hasBox(false)
        {
            if(bbox) hasBox= getBBox(e,box);
        }
    };

    typedef std::list<EdgeInfo> Edges;
    Edges edges;

    struct EdgeValue {
        Edges::iterator it;
        bool start;
        EdgeValue(Edges::iterator _it, bool _start)
            :it(_it),start(_start)
        {}
        bool operator==(const EdgeValue &other) const {
            return it==other.it && start==other.start;
        }
        const gp_Pnt &pt() const {
            return start?it->p1:it->p2;
        }
        const gp_Pnt &ptOther() const {
            return start?it->p2:it->p1;
        }
    };

    struct PntGetter
    {
        typedef const gp_Pnt& result_type;
        result_type operator()(const EdgeValue &v) const { 
            return v.pt(); 
        }
    };

    bgi::rtree<EdgeValue,RParameters, PntGetter> vmap;

    struct BoxGetter
    {
        typedef const Box& result_type;
        result_type operator()(Edges::iterator it) const { 
            return it->box;
        }
    };
    bgi::rtree<Edges::iterator,RParameters, BoxGetter> boxMap;

    BRep_Builder builder;
    TopoDS_Compound comp;

    WireJoiner() {
        builder.MakeCompound(comp);
    }

    void remove(Edges::iterator it) {
        if(it->hasBox)
            boxMap.remove(it);
        vmap.remove(EdgeValue(it,true));
        vmap.remove(EdgeValue(it,false));
        edges.erase(it);
    }

    void add(Edges::iterator it) {
        vmap.insert(EdgeValue(it,true));
        vmap.insert(EdgeValue(it,false));
        if(it->hasBox) 
            boxMap.insert(it);
    }

    void add(const TopoDS_Edge &e, bool bbox=false) {
        if(BRep_Tool::IsClosed(e)){
            BRepBuilderAPI_MakeWire mkWire;
            mkWire.Add(e);
            const TopoDS_Wire &wire = mkWire.Wire();
            if(bbox && Area::getWireDirection(wire)>0)
                builder.Add(comp,wire.Reversed());
            else
                builder.Add(comp,wire);
            return;
        }
        gp_Pnt p1,p2;
        getEndPoints(e,p1,p2);
        if(p1.SquareDistance(p2) < Precision::SquareConfusion())
            return;
        edges.emplace_front(e,p1,p2,bbox);
        add(edges.begin());
    }

    void add(const TopoDS_Shape &shape, bool bbox=false) {
        for(TopExp_Explorer xp(shape,TopAbs_EDGE); xp.More(); xp.Next())
            add(TopoDS::Edge(xp.Current()),bbox);
    }

    //This algorithm tries to join connected edges into wires
    //
    //tol>Precision::SquareConfusion() is used to join points that are close
    //but do not coincide with a line segment. The close points my the results
    //of rounding issue.
    //
    void join(double tol) {
        while(edges.size()) {
            auto it = edges.begin();
            BRepBuilderAPI_MakeWire mkWire;
            mkWire.Add(it->edge);
            gp_Pnt pstart(it->p1),pend(it->p2);
            remove(it);

            bool done = false;
            for(int idx=0;!done&&idx<2;++idx) {
                while(edges.size()) {
                    std::vector<EdgeValue> ret;
                    ret.reserve(1);
                    const gp_Pnt &pt = idx?pstart:pend;
                    vmap.query(bgi::nearest(pt,1),std::back_inserter(ret));
                    assert(ret.size()==1);
                    double d = ret[0].pt().SquareDistance(pt);
                    if(d > tol) break;

                    const auto &info = *ret[0].it;
                    bool start = ret[0].start;
                    if(d > Precision::SquareConfusion()) {
                        // insert a filling edge to solve the tolerance problem
                        const gp_Pnt &pt = ret[idx].pt();
                        if(idx)
                            mkWire.Add(BRepBuilderAPI_MakeEdge(pend,pt).Edge());
                        else
                            mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pstart).Edge());
                    }

                    if(idx==1 && start) {
                        pend = info.p2;
                        mkWire.Add(info.edge);
                    }else if(idx==0 && !start) {
                        pstart = info.p1;
                        mkWire.Add(info.edge);
                    }else if(idx==0 && start) {
                        pstart = info.p2;
                        mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
                    }else {
                        pend = info.p1;
                        mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
                    }
                    remove(ret[0].it);
                    if(pstart.SquareDistance(pend)<=Precision::SquareConfusion()){
                        done = true;
                        break;
                    }
                }
            }
            builder.Add(comp,mkWire.Wire());
        }
    }

    // split any edges that are intersected by othe edge's end point in the middle
    void splitEdges() {
        for(auto it=edges.begin();it!=edges.end();) {
            const auto &info = *it;
            if(!info.hasBox) {
                ++it;
                continue;
            }

            gp_Pnt pstart(info.p1), pend(info.p2);

            gp_Pnt pt;
            bool intersects = false;

            for(auto vit=boxMap.qbegin(bgi::intersects(info.box));
                !intersects && vit!=boxMap.qend();
                ++vit)
            {
                const auto &other = *(*vit);
                if(info.edge.IsSame(other.edge)) continue;

                for(int i=0; i<2; ++i) {
                    const gp_Pnt &p = i?other.p1:other.p2;
                    if(pstart.SquareDistance(p)<=Precision::SquareConfusion() ||
                       pend.SquareDistance(p)<=Precision::SquareConfusion())
                        continue;

                    BRepExtrema_DistShapeShape extss(
                            BRepBuilderAPI_MakeVertex(p),info.edge);
                    if(extss.IsDone() && extss.NbSolution()) {
                        const gp_Pnt &pp = extss.PointOnShape2(1);
                        if(pp.SquareDistance(p)<=Precision::SquareConfusion()) {
                            pt = pp;
                            intersects = true;
                            break;
                        }
                    }else if(Area::TraceEnabled())
                        AREA_WARN("BRepExtrema_DistShapeShape failed");
                }
            }

            if(!intersects) {
                ++it;
                continue;
            }

            Standard_Real first,last;
            Handle_Geom_Curve curve = BRep_Tool::Curve(it->edge, first, last);
            bool reversed = pstart.SquareDistance(curve->Value(last))<=
                Precision::SquareConfusion();

            BRepBuilderAPI_MakeEdge mkEdge1,mkEdge2;
            if(reversed) {
                mkEdge1.Init(curve, pt, pstart);
                mkEdge2.Init(curve, pend, pt);
            }else{
                mkEdge1.Init(curve, pstart, pt);
                mkEdge2.Init(curve, pt, pend);
            }
            if(!mkEdge1.IsDone() || !mkEdge2.IsDone()) {
                if(Area::TraceEnabled())
                    AREA_WARN((reversed?"reversed ":"")<<"edge split failed "<<
                            AREA_XYZ(pstart)<<", " << AREA_XYZ(pt)<< ", "<<AREA_XYZ(pend)<< 
                            ", "<<", err: " << mkEdge1.Error() << ", " << mkEdge2.Error());
                ++it;
                continue;
            }

            Edges::iterator itNext=it;
            ++itNext;
            remove(it);
            add(it=edges.emplace(itNext,mkEdge1.Edge(),true));
            add(edges.emplace(itNext,mkEdge2.Edge(),true));
        }
    }

    struct StackInfo {
        std::deque<EdgeValue> ret;
        int idx;
        StackInfo():idx(0){}
    };

    // This algorithm tries to find a set of closed wires that includes as many
    // edges (added by calling add() ) as possible. One edge may be included
    // in more than one closed wires if it connects to more than one edges.
    int findClosedWires() {
        std::map<int,TopoDS_Edge> edgesToVisit;
        for(const auto &info : edges) 
            edgesToVisit[info.edge.HashCode(INT_MAX)] = info.edge;

        std::deque<StackInfo> stack;
        int skips = 0;

        for(int iteration=1;edgesToVisit.size();++iteration) {
            auto it = edgesToVisit.begin();
            TopoDS_Edge e = it->second;
            edgesToVisit.erase(it);
            gp_Pnt pstart,pend;
            getEndPoints(e,pstart,pend);
            bool skip_me = true;
            stack.clear();

            while(true) {
                stack.emplace_back();
                auto &r = stack.back();

                for(auto vit=vmap.qbegin(bgi::nearest(pend,INT_MAX));
                    vit!=vmap.qend();++vit)
                {
                    if(vit->pt().SquareDistance(pend) > Precision::SquareConfusion())
                        break;

                    auto &info = *vit->it;

                    // use 'iteration' to make sure we don't visit an edge twice.
                    if(skip_me && info.edge.IsSame(e)) {
                        skip_me = false;
                        info.iteration = iteration;
                        info.used = true;
                        continue;
                    }
                    if(info.iteration!=iteration) {
                        info.iteration = iteration;
                        r.ret.push_back(*vit);
                    }
                }
                while(stack.size()) {
                    auto &r = stack.back();
                    if(r.idx<(int)r.ret.size()) {
                        pend = r.ret[r.idx].ptOther();
                        break;
                    }
                    for(auto &v : r.ret)
                        --v.it->iteration;
                    stack.pop_back();
                    if(stack.size()) 
                        ++stack.back().idx;
                }
                if(stack.empty()) {
                    ++skips;
                    break;
                }
                if(pstart.SquareDistance(pend) > Precision::SquareConfusion()) 
                    continue;

                BRepBuilderAPI_MakeWire mkWire;
                mkWire.Add(e);
                for(auto &r : stack) {
                    const auto &v = r.ret[r.idx];
                    auto &info = *v.it;
                    if(!info.used) {
                        info.used = true;
                        auto it = edgesToVisit.find(info.edge.HashCode(INT_MAX));
                        if(it!=edgesToVisit.end())
                            edgesToVisit.erase(it);
                    }
                    if(v.start)
                        mkWire.Add(info.edge);
                    else
                        mkWire.Add(TopoDS::Edge(info.edge.Reversed()));
                }
                const TopoDS_Wire &wire = mkWire.Wire();
                if(Area::getWireDirection(wire)>0)
                    builder.Add(comp,wire.Reversed());
                else
                    builder.Add(comp,wire);
                break;
            }
        }
        return skips;
    }
};

void Area::explode(const TopoDS_Shape &shape) {
    const TopoDS_Shape &plane = getPlane();
    bool haveShape = false;
    for(TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
        haveShape = true;
        if(myParams.Coplanar!=CoplanarNone && !isCoplanar(it.Current(),plane)){
            ++mySkippedShapes;
            if(myParams.Coplanar == CoplanarForce)
                continue;
        }
        for(TopExp_Explorer itw(it.Current(), TopAbs_WIRE); itw.More(); itw.Next()) {
            for(BRepTools_WireExplorer xp(TopoDS::Wire(itw.Current()));xp.More();xp.Next())
                addWire(*myArea,BRepBuilderAPI_MakeWire(
                            TopoDS::Edge(xp.Current())).Wire(),&myTrsf,myParams.Deflection,true);
        }
    }
    if(haveShape) return;
    for(TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next()) {
        if(myParams.Coplanar!=CoplanarNone && !isCoplanar(it.Current(),plane)){
            ++mySkippedShapes;
            if(myParams.Coplanar == CoplanarForce)
                continue;
        }
        addWire(*myArea,BRepBuilderAPI_MakeWire(
                    TopoDS::Edge(it.Current())).Wire(),&myTrsf,myParams.Deflection,true);
    }
}

#if 0
static void show(const TopoDS_Shape &shape, const char *name) {
    App::Document *pcDoc = App::GetApplication().getActiveDocument(); 	 
    if (!pcDoc)
        pcDoc = App::GetApplication().newDocument();
    Part::Feature *pcFeature = (Part::Feature *)pcDoc->addObject("Part::Feature", name);
    // copy the data
    //TopoShape* shape = new MeshObject(*pShape->getTopoShapeObjectPtr());
    pcFeature->Shape.setValue(shape);
    //pcDoc->recompute();
}
#endif

template<class Func>
static int foreachSubshape(const TopoDS_Shape &shape, Func func, int type=TopAbs_FACE) {
    bool haveShape = false;
    switch(type) {
    case TopAbs_FACE:
        for(TopExp_Explorer it(shape,TopAbs_FACE); it.More(); it.Next()) {
            haveShape = true;
            func(it.Current(),TopAbs_FACE);
        }
        if(haveShape) return TopAbs_FACE;
        //fall through
    case TopAbs_WIRE:
        for(TopExp_Explorer it(shape,TopAbs_WIRE); it.More(); it.Next()) {
            haveShape = true;
            func(it.Current(),TopAbs_WIRE);
        }
        if(haveShape) return TopAbs_WIRE;
        //fall through
    default:
        for(TopExp_Explorer it(shape,TopAbs_EDGE); it.More(); it.Next()) {
            haveShape = true;
            func(it.Current(),TopAbs_EDGE);
        }
    }
    return haveShape?TopAbs_EDGE:-1;
}

struct FindPlane {
    TopoDS_Shape &myPlaneShape;
    gp_Trsf &myTrsf;
    double &myZ;
    FindPlane(TopoDS_Shape &s, gp_Trsf &t, double &z)
        :myPlaneShape(s),myTrsf(t),myZ(z)
    {}
    void operator()(const TopoDS_Shape &shape, int) {
        gp_Trsf trsf;

        BRepLib_FindSurface finder(shape.Located(TopLoc_Location()),-1,Standard_True);
        if (!finder.Found()) 
            return;

        gp_Ax3 pos = GeomAdaptor_Surface(finder.Surface()).Plane().Position();
        // TODO: It seemed that FindSurface disregard shape's
        // transformation SOMETIME, so we have to transformed the found
        // plane manually. Need to figure out WHY!
        pos.Transform(shape.Location().Transformation());

        // We only use right hand coordinate, hence gp_Ax2 instead of gp_Ax3
        // This means that no matter what the work plane face oriented, we 
        // will treat it as face upward in a right hand coordinate system.
        if(!pos.Direct())
            pos = gp_Ax3(pos.Ax2());

        gp_Dir dir(pos.Direction());

        trsf.SetTransformation(pos);

        if(fabs(dir.X())<Precision::Confusion() &&
            fabs(dir.Y())<Precision::Confusion()) 
        {
            TopExp_Explorer it(shape,TopAbs_VERTEX);
            const auto &pt = BRep_Tool::Pnt(TopoDS::Vertex(it.Current()));
            if(!myPlaneShape.IsNull() && myZ > pt.Z())
                return;
            myZ = pt.Z();
        }else if(!myPlaneShape.IsNull())
            return;
        myPlaneShape = shape;
        myTrsf = trsf;
    }
};

TopoDS_Shape Area::findPlane(const TopoDS_Shape &shape, gp_Trsf &trsf) 
{
    TopoDS_Shape plane;
    double top_z;
    foreachSubshape(shape,FindPlane(plane,trsf,top_z));
    return plane;
}

std::list<TopoDS_Wire> Area::project(const TopoDS_Shape &solid)
{
    TIME_INIT2(t,t1);
    Handle_HLRBRep_Algo brep_hlr = NULL;
    try {
        brep_hlr = new HLRBRep_Algo();
        brep_hlr->Add(solid, 0);
        HLRAlgo_Projector projector(gp_Ax2(gp_Pnt(),gp_Dir(0,0,1)));
        brep_hlr->Projector(projector);
        brep_hlr->Update();
        brep_hlr->Hide();
    } catch (...) {
        AREA_ERR("error occurred while projecting shape");
    }
    TIME_PRINT(t1,"HLRBrep_Algo");
    WireJoiner joiner;
    try {
#define ADD_HLR_SHAPE(_name) \
        shape = hlrToShape._name##Compound();\
        if(!shape.IsNull()){\
            BRepLib::BuildCurves3d(shape);\
            joiner.add(shape,true);\
        }
        TopoDS_Shape shape;
        HLRBRep_HLRToShape hlrToShape(brep_hlr);
        ADD_HLR_SHAPE(V)
        // ADD_HLR_SHAPE(H)
        ADD_HLR_SHAPE(OutLineV);
        // ADD_HLR_SHAPE(OutLineH);
    }
    catch (...) {
        AREA_ERR("error occurred while extracting edges");
    }
    TIME_PRINT(t1,"WireJoiner init");
    joiner.splitEdges();
    TIME_PRINT(t1,"WireJoiner splitEdges");
    // for(const auto &v : joiner.edges) {
    //     joiner.builder.Add(joiner.comp,BRepBuilderAPI_MakeWire(v.edge).Wire());
    // }

    int skips = joiner.findClosedWires();
    TIME_PRINT(t1,"WireJoiner findClosedWires");
    if(skips) AREA_WARN("skipped " << skips << " open edges");

    std::list<TopoDS_Wire> wires;

    Area area(&myParams);
    area.myParams.Explode = false;
    area.myParams.FitArcs = false;
    area.myParams.Reorient = false;
    area.myParams.Coplanar = CoplanarNone;
    area.add(joiner.comp, OperationUnion);
    TopoDS_Shape shape = area.getShape();

    TIME_PRINT(t1,"Clipper wire union");

    if(shape.IsNull()) {
        AREA_ERR("poject failed");
    }else{
        for(TopExp_Explorer xp(shape, TopAbs_WIRE); xp.More(); xp.Next())
            wires.push_back(TopoDS::Wire(xp.Current()));
    }

    TIME_PRINT(t,"project total");
    return std::move(wires);
}

std::vector<shared_ptr<Area> > Area::makeSections(
        PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
        const std::vector<double> &_heights,
        const TopoDS_Shape &section_plane)
{
    TopoDS_Shape plane;
    gp_Trsf trsf;

    if(!section_plane.IsNull())
        plane = findPlane(section_plane,trsf);
    else
        plane = getPlane(&trsf);

    if(plane.IsNull())
        throw Base::ValueError("failed to obtain section plane");

    TIME_INIT2(t,t1);

    TopLoc_Location loc(trsf);

    Bnd_Box bounds;
    for(const Shape &s : myShapes) {
        const TopoDS_Shape &shape = s.shape.Moved(loc);
        BRepBndLib::Add(shape, bounds, Standard_False);
    }
    bounds.SetGap(0.0);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    AREA_TRACE("section bounds X("<<xMin<<','<<xMax<<"), Y("<<
            yMin<<','<<yMax<<"), Z("<<zMin<<','<<zMax<<')');

    std::vector<double> heights;
    double tolerance = 0.0;
    if(_heights.empty()) {
        double z;
        double d = fabs(myParams.Stepdown);
        if(myParams.SectionCount>1 && d<Precision::Confusion())
            throw Base::ValueError("invalid stepdown");

        if(mode == SectionModeBoundBox) {
            if(myParams.Stepdown > 0.0)
                z = zMax-myParams.SectionOffset;
            else
                z = zMin+myParams.SectionOffset;
        }else if(mode == SectionModeWorkplane){
            // Because we've transformed the shapes using the work plane so
            // that the work plane is aligned with xy0 plane, the starting Z
            // value shall be 0 minus the given section offset. Note the
            // section offset is relative to the starting Z
            if(myParams.Stepdown > 0.0)
                z = -myParams.SectionOffset;
            else
                z = myParams.SectionOffset;
        } else {
            gp_Pnt pt(0,0,myParams.SectionOffset);
            z = pt.Transformed(loc).Z();
        }
        if(z > zMax)
            z = zMax;
        else if(z < zMin)
            z = zMin;
        double dz;
        if(myParams.Stepdown>0.0) {
            dz = z - zMin;
            tolerance = myParams.SectionTolerance;
        }else{
            dz = zMax - z;
            tolerance = -myParams.SectionTolerance;
        }
        int count = myParams.SectionCount;
        if(count<0 || count*d > dz) 
            count = floor(dz/d)+1;
        heights.reserve(count);
        for(int i=0;i<count;++i,z-=myParams.Stepdown) {
            double height = z-tolerance;
            if(z-zMin<myParams.SectionTolerance){
                height = zMin+myParams.SectionTolerance;
                AREA_WARN("hit bottom " <<z<<','<<zMin<<','<<height);
                heights.push_back(height);
                if(myParams.Stepdown>0.0) break;
            }else if(zMax-z<myParams.SectionTolerance) {
                height = zMax-myParams.SectionTolerance;
                AREA_WARN("hit top " <<z<<','<<zMax<<','<<height);
                heights.push_back(height);
                if(myParams.Stepdown<0.0) break;
            }else
                heights.push_back(height);
        }
    }else{
        heights.reserve(_heights.size());

        bool hitMax = false, hitMin = false;
        for(double z : _heights) {
            switch(mode) {
            case SectionModeAbsolute: {
                gp_Pnt pt(0,0,z);
                z = pt.Transformed(loc).Z();
                break;
            }case SectionModeBoundBox:
                z = zMax - z;
                break;
            case SectionModeWorkplane:
                z = -z;
                break;
            default: 
                throw Base::ValueError("invalid section mode");
            }
            if(z-zMin<myParams.SectionTolerance) {
                if(hitMin) continue;
                hitMin = true;
                double zNew = zMin+myParams.SectionTolerance;
                AREA_WARN("hit bottom " <<z<<','<<zMin<<','<<zNew);
                z = zNew;
            }else if (zMax-z<myParams.SectionTolerance) {
                if(hitMax) continue;
                double zNew = zMax-myParams.SectionTolerance;
                AREA_WARN("hit top " <<z<<','<<zMax<<','<<zNew);
                z = zNew;
            }
            heights.push_back(z);
        }
    }

    if(heights.empty())
        throw Base::ValueError("no sections");

    std::vector<shared_ptr<Area> > sections;
    sections.reserve(heights.size());
    tolerance *= 2.0;
    bool can_retry = fabs(tolerance)>Precision::Confusion();
    TopLoc_Location locInverse(loc.Inverted());

    std::vector<Shape> projectedShapes;
    if(project) projectedShapes.reserve(myShapes.size());

    bool aborted = false;
    for(double z : heights) {
        bool retried = !can_retry;
        while(true) {
            gp_Pln pln(gp_Pnt(0,0,z),gp_Dir(0,0,1));
            Standard_Real a,b,c,d;
            pln.Coefficients(a,b,c,d);
            BRepLib_MakeFace mkFace(pln,xMin,xMax,yMin,yMax);
            const TopoDS_Shape &face = mkFace.Face();

            shared_ptr<Area> area(new Area(&myParams));
            area->setPlane(face.Moved(locInverse));

            if(projectedShapes.size()) {
                for(const auto &s : projectedShapes) {
                    gp_Trsf t;
                    t.SetTranslation(gp_Vec(0,0,-d));
                    TopLoc_Location wloc(t);
                    area->add(s.shape.Moved(wloc).Moved(locInverse),s.op);
                }
                sections.push_back(area);
                break;
            }else{
                for(auto it=myShapes.begin();it!=myShapes.end();++it) {
                    const auto &s = *it;
                    BRep_Builder builder;
                    TopoDS_Compound comp;
                    builder.MakeCompound(comp);

                    for(TopExp_Explorer xp(s.shape.Moved(loc), TopAbs_SOLID); xp.More(); xp.Next()) {
                        std::list<TopoDS_Wire> wires;
                        if(project) {
                            wires = this->project(xp.Current());
                        }else{
                            Part::CrossSection section(a,b,c,xp.Current());
                            wires = section.slice(-d);
                        }

                        if(wires.empty()) {
                            AREA_LOG("Section returns no wires");
                            continue;
                        }

                        if(myParams.Fill != FillNone) {
                            Part::FaceMakerBullseye mkFace;
                            mkFace.setPlane(pln);
                            for(const TopoDS_Wire &wire : wires)
                                mkFace.addWire(wire);
                            try {
                                mkFace.Build();
                                if (mkFace.Shape().IsNull())
                                    AREA_WARN("FaceMakerBullseye return null shape on section");
                                else {
                                    builder.Add(comp,mkFace.Shape());
                                    continue;
                                }
                            }catch (Base::Exception &e){
                                AREA_WARN("FaceMakerBullseye failed on section: " << e.what());
                            }
                        }
                        for(const TopoDS_Wire &wire : wires)
                            builder.Add(comp,wire);
                    }

                    // Make sure the compound has at least one edge
                    TopExp_Explorer xp(comp,TopAbs_EDGE);
                    if(xp.More()) {
                        if(project){
                            projectedShapes.push_back(Shape(s.op,comp));

                            gp_Trsf t;
                            t.SetTranslation(gp_Vec(0,0,-d));
                            TopLoc_Location wloc(t);
                            area->add(comp.Moved(wloc).Moved(locInverse),s.op);
                        }else
                            area->add(comp.Moved(locInverse),s.op);
                    }else if(area->myShapes.empty()){
                        auto itNext = it;
                        if(++itNext != myShapes.end() &&
                            (itNext->op==OperationIntersection ||
                            itNext->op==OperationDifference))
                        {
                            break;
                        }
                    }
                }
                if(area->myShapes.size()){
                    sections.push_back(area);
                    break;
                }
                if(project) {
                    AREA_ERR("empty projection");
                    aborted = true;
                }
                if(retried) {
                    AREA_WARN("Discard empty section");
                    break;
                }else{
                    AREA_TRACE("retry section " <<z<<"->"<<z+tolerance);
                    z += tolerance;
                    retried = true;
                }
            }
            TIME_PRINT(t1,"makeSection " << z);
        }
        if(aborted) break;
    }
    TIME_PRINT(t,"makeSection count: " << sections.size()<<", total");
    return std::move(sections);
}

TopoDS_Shape Area::getPlane(gp_Trsf *trsf) {
    if(!myWorkPlane.IsNull()) {
        if(trsf) *trsf = myTrsf;
        return myWorkPlane;
    }
    if(myShapePlane.IsNull()) {
        if(myShapes.empty())
            throw Base::ValueError("no shape added");
        double top_z;
        for(auto &s : myShapes)
            foreachSubshape(s.shape,FindPlane(myShapePlane,myTrsf,top_z));
        if(myShapePlane.IsNull())
            throw Base::ValueError("shapes are not planar");
    }
    if(trsf) *trsf = myTrsf;
    return myShapePlane;
}

bool Area::isBuilt() const { 
    return (myArea || mySections.size());
}


void Area::build() {
    if(isBuilt()) return;

    if(myShapes.empty())
        throw Base::ValueError("no shape added");

#define AREA_MY(_param) myParams.PARAM_FNAME(_param)
    PARAM_ENUM_CONVERT(AREA_MY,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);

    if(myHaveSolid && myParams.SectionCount) {
        mySections = makeSections(PARAM_FIELDS(AREA_MY,AREA_PARAMS_SECTION_EXTRA));
        return;
    }

    TIME_INIT(t);
    getPlane();

    try {
        myArea.reset(new CArea());
        myAreaOpen.reset(new CArea());

        CAreaConfig conf(myParams);
        CArea areaClip;

        mySkippedShapes = 0;
        short op = OperationUnion;
        bool pending = false;
        bool exploding = myParams.Explode;
        for(const Shape &s : myShapes) {
            if(exploding) {
                exploding = false;
                explode(s.shape);
                continue;
            }else if(op!=s.op) {
                if(myParams.OpenMode!=OpenModeNone)
                    myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
                pending = false;
                if(areaClip.m_curves.size()) {
                    if(op == OperationCompound)
                        myArea->m_curves.splice(myArea->m_curves.end(),areaClip.m_curves);
                    else{
                        myArea->Clip(toClipperOp(op),&areaClip,SubjectFill,ClipFill);
                        areaClip.m_curves.clear();
                    }
                }
                op=s.op;
            }
            addToBuild(op==OperationUnion?*myArea:areaClip,s.shape);
            pending = true;
        }
        if(mySkippedShapes && !myHaveSolid)
            AREA_WARN((myParams.Coplanar==CoplanarForce?"Skipped ":"Found ")<<
                mySkippedShapes<<" non coplanar shapes");

        if(pending){
            if(myParams.OpenMode!=OpenModeNone)
                myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
            if(op == OperationCompound)
                myArea->m_curves.splice(myArea->m_curves.end(),areaClip.m_curves);
            else{
                myArea->Clip(toClipperOp(op),&areaClip,SubjectFill,ClipFill);
            }
        }
        myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);

        //Reassemble wires after explode
        if(myParams.Explode) {
            WireJoiner joiner;
            gp_Trsf trsf(myTrsf.Inverted());
            for(const auto &c : myArea->m_curves) {
                TopoDS_Wire wire = toShape(c,&trsf);
                if(!wire.IsNull()) 
                    joiner.add(wire);
            }
            joiner.join(Precision::Confusion());

            Area area(&myParams);
            area.myParams.Explode = false;
            area.myParams.Coplanar = CoplanarNone;
            area.myWorkPlane = getPlane(&area.myTrsf);
            area.add(joiner.comp,OperationCompound);
            area.build();
            myArea = std::move(area.myArea);
        }

        TIME_TRACE(t,"prepare");

    }catch(...) {
        clean();
        throw;
    }
}

TopoDS_Shape Area::toShape(CArea &area, short fill) {
    gp_Trsf trsf(myTrsf.Inverted());
    bool bFill;
    switch(fill){
    case Area::FillAuto:
        bFill = myHaveFace;
        break;
    case Area::FillFace:
        bFill = true;
        break;
    default:
        bFill = false;
    }
    if(myParams.FitArcs) {
        if(&area == myArea.get()) {
            CArea copy(area);
            copy.FitArcs();
            return toShape(copy,bFill,&trsf);
        }
        area.FitArcs();
    }
    return toShape(area,bFill,&trsf);
}


#define AREA_SECTION(_op,_index,...) do {\
    if(mySections.size()) {\
        if(_index>=(int)mySections.size())\
            return TopoDS_Shape();\
        if(_index<0) {\
            BRep_Builder builder;\
            TopoDS_Compound compound;\
            builder.MakeCompound(compound);\
            for(shared_ptr<Area> area : mySections){\
                const TopoDS_Shape &s = area->_op(_index, ## __VA_ARGS__);\
                if(s.IsNull()) continue;\
                builder.Add(compound,s);\
            }\
            for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();)\
                return compound;\
            return TopoDS_Shape();\
        }\
        return mySections[_index]->_op(_index, ## __VA_ARGS__);\
    }\
}while(0)

TopoDS_Shape Area::getShape(int index) {
    build();
    AREA_SECTION(getShape,index);

    if(myShapeDone) return myShape;

    if(!myArea) return TopoDS_Shape();

    CAreaConfig conf(myParams);

    // if no offset or thicken, try pocket
    if(fabs(myParams.Offset) < Precision::Confusion() && !myParams.Thicken) {
        if(myParams.PocketMode == PocketModeNone) {
            myShape = toShape(*myArea,myParams.Fill);
            myShapeDone = true;
            return myShape;
        }
        myShape = makePocket(index,PARAM_FIELDS(AREA_MY,AREA_PARAMS_POCKET));
        myShapeDone = true;
        return myShape;
    }

    // if no pocket, do offset or thicken
    if(myParams.PocketMode == PocketModeNone){
        myShape = makeOffset(index,PARAM_FIELDS(AREA_MY,AREA_PARAMS_OFFSET));
        myShapeDone = true;
        return myShape;
    }

    TIME_INIT(t);

    // do offset first, then pocket the inner most offseted shape
    std::list<shared_ptr<CArea> > areas;
    makeOffset(areas,PARAM_FIELDS(AREA_MY,AREA_PARAMS_OFFSET));

    if(areas.empty()) 
        areas.push_back(make_shared<CArea>(*myArea));

    Area areaPocket(&myParams);
    bool front = true;
    if(areas.size()>1) {
        double step = myParams.Stepover;
        if(fabs(step)<Precision::Confusion())
            step = myParams.Offset;
        front = step>0;
    }

    // for pocketing, we discard the outer most offset wire in order to achieve
    // the effect of offseting shape first than pocket, where the actual offset
    // path is not wanted. For extra outline profiling, add extra_offset
    if(front) {
        areaPocket.add(toShape(*areas.front(),myParams.Fill));
        areas.pop_back();
    }else{
        areaPocket.add(toShape(*areas.back(),myParams.Fill));
        areas.pop_front();
    }

    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    short fill = myParams.Thicken?FillFace:FillNone;
    TIME_INIT(t2);
    DURATION_INIT(d);
    for(shared_ptr<CArea> area : areas) {
        if(myParams.Thicken){
            area->Thicken(myParams.ToolRadius);
            DURATION_PLUS(d,t2);
        }
        const TopoDS_Shape &shape = toShape(*area,fill);
        if(shape.IsNull()) continue;
        builder.Add(compound,shape);
    }
    if(myParams.Thicken) 
        DURATION_PRINT(d,"Thicken");

    // make sure the compound has at least one edge
    for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();) {
        builder.Add(compound,areaPocket.makePocket(
                    -1,PARAM_FIELDS(AREA_MY,AREA_PARAMS_POCKET)));
        myShape = compound;
        break;
    }
    myShapeDone = true;
    TIME_PRINT(t,"total");
    return myShape;
}

TopoDS_Shape Area::makeOffset(int index,PARAM_ARGS(PARAM_FARG,AREA_PARAMS_OFFSET)) {
    build();
    AREA_SECTION(makeOffset,index,PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_OFFSET));

    std::list<shared_ptr<CArea> > areas;
    makeOffset(areas,PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_OFFSET));
    if(areas.empty()) {
        if(myParams.Thicken && myParams.ToolRadius>Precision::Confusion()) {
            CArea area(*myArea);
            TIME_INIT(t);
            area.Thicken(myParams.ToolRadius);
            TIME_PRINT(t,"Thicken");
            return toShape(area,FillFace);
        }
        return TopoDS_Shape();
    }
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    TIME_INIT(t);
    DURATION_INIT(d);
    for(shared_ptr<CArea> area : areas) {
        short fill;
        if(myParams.Thicken && myParams.ToolRadius>Precision::Confusion()) {
            area->Thicken(myParams.ToolRadius);
            DURATION_PLUS(d,t);
            fill = FillFace;
        }else if(areas.size()==1)
            fill = myParams.Fill;
        else
            fill = FillNone;
        const TopoDS_Shape &shape = toShape(*area,fill);
        if(shape.IsNull()) continue;
        builder.Add(compound,shape);
    }
    if(myParams.Thicken && myParams.ToolRadius>Precision::Confusion())
        DURATION_PRINT(d,"Thicken");
    for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();)
        return compound;
    return TopoDS_Shape();
}

void Area::makeOffset(list<shared_ptr<CArea> > &areas,
                      PARAM_ARGS(PARAM_FARG,AREA_PARAMS_OFFSET))
{
    if(fabs(offset)<Precision::Confusion())
        return;

    TIME_INIT2(t,t1);

    long count = 1;
    if(extra_pass) {
        if(fabs(stepover)<Precision::Confusion())
            stepover = offset;
        if(extra_pass > 0) {
            count += extra_pass;
        }else{
            if(stepover>0 || offset>0)
                throw Base::ValueError("invalid extra count");
            // In this case, we loop until no outputs from clipper
            count=-1;
        }
    }

    PARAM_ENUM_CONVERT(AREA_MY,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_OFFSET_CONF);
#ifdef AREA_OFFSET_ALGO
    PARAM_ENUM_CONVERT(AREA_MY,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);
#endif
    
    for(int i=0;count<0||i<count;++i,offset+=stepover) {
        areas.push_back(make_shared<CArea>());
        CArea &area = *areas.back();
        CArea areaOpen;
#ifdef AREA_OFFSET_ALGO
        if(myParams.Algo == Area::Algolibarea) {
            for(const CCurve &c : myArea->m_curves) {
                if(c.IsClosed())
                    area.append(c);
                else
                    areaOpen.append(c);
            }
        }else
#endif
            area = *myArea;

#ifdef AREA_OFFSET_ALGO
        switch(myParams.Algo){
        case Area::Algolibarea:
            // libarea somehow fails offset without Reorder, but ClipperOffset
            // works okay. Don't know why
            area.Reorder();
            area.Offset(-offset);
            if(areaOpen.m_curves.size()) {
                areaOpen.Thicken(offset);
                area.Clip(ClipperLib::ctUnion,&areaOpen,SubjectFill,ClipFill);
            }
            break;
        case Area::AlgoClipperOffset:
#endif
            area.OffsetWithClipper(offset,JoinType,EndType,
                    myParams.MiterLimit,myParams.RoundPreceision);
#ifdef AREA_OFFSET_ALGO
            break;
        }
#endif
        if(count>1)
            TIME_PRINT(t1,"makeOffset " << i << '/' << count);
        if(area.m_curves.empty())
            return;
    }
    TIME_PRINT(t,"makeOffset count: " << count);
}

TopoDS_Shape Area::makePocket(int index, PARAM_ARGS(PARAM_FARG,AREA_PARAMS_POCKET)) {
    if(tool_radius < Precision::Confusion()) 
        throw Base::ValueError("tool radius too small");

    if(stepover == 0.0)
        stepover = tool_radius;

    if(stepover < Precision::Confusion())
        throw Base::ValueError("stepover too small");

    if(mode == Area::PocketModeNone)
        return TopoDS_Shape();

    build();
    AREA_SECTION(makePocket,index,PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_POCKET));

    TIME_INIT(t);
    bool done = false;

    if(index>=0) {
        if(fabs(angle_shift) >= Precision::Confusion())
            angle += index*angle_shift;

        if(fabs(shift)>=Precision::Confusion())
            shift *= index;
    }

    if(angle<-360.0) 
        angle += ceil(fabs(angle)/360.0)*360.0;
    else if(angle>360.0)
        angle -= floor(angle/360.0)*360.0;
    else if(angle<0.0)
        angle += 360.0;

    if(shift<-stepover)
        shift += ceil(fabs(shift)/stepover)*stepover;
    else if(shift>stepover)
        shift -= floor(shift/stepover)*stepover;
    else if(shift<0.0)
        shift += stepover;

    CAreaConfig conf(myParams);

    CArea out;
    PocketMode pm;

    switch(mode) {
    case Area::PocketModeZigZag:
        pm = ZigZagPocketMode;
        break;
    case Area::PocketModeSpiral:
        pm = SpiralPocketMode;
        break;
    case Area::PocketModeOffset: {
        PARAM_DECLARE_INIT(PARAM_FNAME,AREA_PARAMS_OFFSET);
        Offset = -tool_radius-extra_offset-shift;
        ExtraPass = -1;
        Stepover = -stepover;
        return makeOffset(index,PARAM_FIELDS(PARAM_FNAME,AREA_PARAMS_OFFSET));
    }case Area::PocketModeZigZagOffset:
        pm = ZigZagThenSingleOffsetPocketMode;
        break;
    case Area::PocketModeLine:
    case Area::PocketModeGrid:
    case Area::PocketModeTriangle:{
        CBox2D box;
        myArea->GetBox(box);
        if(!box.m_valid)
            throw Base::ValueError("failed to get bound box");
        double angles[4];
        int count=1;
        angles[0] = 0.0;
        if(mode == Area::PocketModeGrid){
            angles[1]=90.0;
            count=2;
            if(shift<Precision::Confusion()){
                count=4;
                angles[2]=180.0;
                angles[3]=270.0;
            }
        }else if(mode == Area::PocketModeTriangle) {
            count=3;
            angles[1]=120;
            angles[2]=240;
        }else
            shift = 0.0; //Line pattern does not support shift
        Point center(box.Centre());
        double r = box.Radius()+stepover;
        int steps = (int)ceil(r*2.0/stepover);
        for(int i=0;i<count;++i) {
            double a = angle + angles[i];
            if(a>360.0) a-=360.0;
            double offset = -r+shift;
            for(int j=0;j<steps;++j,offset+=stepover) {
                Point p1(-r,offset),p2(r,offset);
                if(a > Precision::Confusion()) {
                    double r = a*M_PI/180.0;
                    p1.Rotate(r);
                    p2.Rotate(r);
                }
                out.m_curves.emplace_back();
                CCurve &curve = out.m_curves.back();
                curve.m_vertices.emplace_back(p1+center);
                curve.m_vertices.emplace_back(p2+center);
            }
        }
        PARAM_ENUM_CONVERT(AREA_MY,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);
        out.Clip(toClipperOp(OperationIntersection),myArea.get(), SubjectFill,ClipFill);
        done = true;
        break;
    }default:
        throw Base::ValueError("unknown poket mode");
    }

    if(!done) {
        CAreaPocketParams params(
                tool_radius,extra_offset,stepover,from_center,pm,angle);
        CArea in(*myArea),out;
        // MakePcoketToolPath internally uses libarea Offset which somehow demands
        // reorder before input, otherwise nothing is shown.
        in.Reorder();
        in.MakePocketToolpath(out.m_curves,params);
    }

    TIME_PRINT(t,"makePocket");

    if(myParams.Thicken){
        TIME_INIT(t);
        out.Thicken(tool_radius);
        TIME_PRINT(t,"thicken");
        return toShape(out,FillFace);
    }else
        return toShape(out,FillNone);
}

static inline bool IsLeft(const gp_Pnt &a, const gp_Pnt &b, const gp_Pnt &c) {
    return ((b.X() - a.X())*(c.Y() - a.Y()) - (b.Y() - a.Y())*(c.X() - a.X())) > 0;
}

TopoDS_Wire Area::toShape(const CCurve &c, const gp_Trsf *trsf) {
    BRepBuilderAPI_MakeWire mkWire;
    gp_Pnt pstart,pt;
    bool first = true;
    for(const CVertex &v : c.m_vertices){
        if(first){
            first = false;
            pstart = pt = gp_Pnt(v.m_p.x,v.m_p.y,0);
            continue;
        }
        gp_Pnt pnext(v.m_p.x,v.m_p.y,0);
        if(pnext.SquareDistance(pt)<Precision::SquareConfusion())
            continue;
        if(v.m_type == 0) {
            mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pnext).Edge());
        } else {
            gp_Pnt center(v.m_c.x,v.m_c.y,0);
            double r = center.Distance(pt);
            double r2 = center.Distance(pnext);
            if(fabs(r-r2) > Precision::Confusion()) {
                double d = pt.Distance(pnext);
                double q = sqrt(r*r - d*d*0.25);
                double x = (pt.X()+pnext.X())*0.5;
                double y = (pt.Y()+pnext.Y())*0.5;
                double dx = q*(pt.Y()-pnext.Y())/d;
                double dy = q*(pnext.X()-pt.X())/d;
                gp_Pnt newCenter(x + dx, y + dy,0);
                if(IsLeft(pt,pnext,center) != IsLeft(pt,pnext,newCenter)) {
                    newCenter.SetX(x - dx);
                    newCenter.SetY(y - dy);
                }
                AREA_WARN("Arc correction: "<<r<<", "<<r2<<", center"<<
                        AREA_XYZ(center)<<"->"<<AREA_XYZ(newCenter));
                center = newCenter;
            }
            gp_Ax2 axis(center, gp_Dir(0,0,v.m_type));
            mkWire.Add(BRepBuilderAPI_MakeEdge(gp_Circ(axis,r),pt,pnext).Edge());
        }
        pt = pnext;
    }
    if(!mkWire.IsDone()) {
        AREA_WARN("failed to make wire " << mkWire.Error());
        return TopoDS_Wire();
    }

    if(c.IsClosed() && !BRep_Tool::IsClosed(mkWire.Wire())){
        // This should never happen after changing libarea's
        // Point::tolerance to be the same as Precision::Confusion().  
        // Just leave it here in case.
        BRepAdaptor_Curve curve(mkWire.Edge());
        gp_Pnt p1(curve.Value(curve.FirstParameter()));
        gp_Pnt p2(curve.Value(curve.LastParameter()));
        AREA_WARN("warning: patch open wire type " << 
            c.m_vertices.back().m_type<<endl<<AREA_XYZ(p1)<<endl<<
            AREA_XYZ(p2)<<endl<<AREA_XYZ(pt)<<endl<<AREA_XYZ(pstart));
        mkWire.Add(BRepBuilderAPI_MakeEdge(pt,pstart).Edge());
    }
    if(trsf)
        return TopoDS::Wire(mkWire.Wire().Moved(TopLoc_Location(*trsf)));
    else
        return mkWire.Wire();
}

TopoDS_Shape Area::toShape(const CArea &area, bool fill, const gp_Trsf *trsf) {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    for(const CCurve &c : area.m_curves) {
        const TopoDS_Wire &wire = toShape(c,trsf);
        if(!wire.IsNull())
            builder.Add(compound,wire);
    }
    TopExp_Explorer xp(compound,TopAbs_EDGE);
    if(!xp.More()) return TopoDS_Shape();
    if(fill) {
        try{
            TIME_INIT(t);
            Part::FaceMakerBullseye mkFace;
            if(trsf)
                mkFace.setPlane(gp_Pln().Transformed(*trsf));
            for(TopExp_Explorer it(compound, TopAbs_WIRE); it.More(); it.Next())
                mkFace.addWire(TopoDS::Wire(it.Current()));
            mkFace.Build();
            if (mkFace.Shape().IsNull())
                AREA_WARN("FaceMakerBullseye returns null shape");
            TIME_PRINT(t,"makeFace");
            return mkFace.Shape();
        }catch (Base::Exception &e){
            AREA_WARN("FaceMakerBullseye failed: "<<e.what());
        }
    }
    return compound;
}

struct WireInfo {
    TopoDS_Wire wire;
    std::deque<gp_Pnt> points;
    bool isClosed;

    inline const gp_Pnt &pstart() const{ 
        return points.front(); 
    }
    inline const gp_Pnt &pend() const{ 
        return isClosed?pstart():points.back(); 
    }
};

typedef std::list<WireInfo> Wires;
typedef std::pair<Wires::iterator,size_t> RValue;

struct RGetter
{
    typedef const gp_Pnt& result_type;
    result_type operator()(const RValue &v) const { return v.first->points[v.second]; }
};
typedef bgi::rtree<RValue,RParameters,RGetter> RTree;

struct ShapeParams {
    double abscissa;
    int k;
    short orientation;
    short direction;
#ifdef AREA_TIME_ENABLE
    TIME_DURATION qd; //rtree query duration
    TIME_DURATION bd; //rtree build duration
    TIME_DURATION rd; //rtree remove duration
    TIME_DURATION xd; //BRepExtrema_DistShapeShape duration
#endif

    ShapeParams(double _a, int _k, short o, short d)
        :abscissa(_a),k(_k),orientation(o),direction(d)
#ifdef AREA_TIME_ENABLE
        ,qd(0),bd(0),rd(0),xd(0)
#endif
    {}
};

bool operator<(const Wires::iterator &a, const Wires::iterator &b) {
    return &(*a) < &(*b);
}
typedef std::map<Wires::iterator,size_t> RResults;

struct GetWires {
    Wires &wires;
    RTree &rtree;
    ShapeParams &params;
    GetWires(std::list<WireInfo> &ws, RTree &rt, ShapeParams &rp)
        :wires(ws),rtree(rt),params(rp)
    {}
    void operator()(const TopoDS_Shape &shape, int type) {
        wires.push_back(WireInfo());
        WireInfo &info = wires.back();
        if(type == TopAbs_WIRE)
            info.wire = TopoDS::Wire(shape);
        else
            info.wire = BRepBuilderAPI_MakeWire(TopoDS::Edge(shape)).Wire();
        info.isClosed = BRep_Tool::IsClosed(info.wire);

        if(info.isClosed && params.orientation != Area::OrientationNone){
            int dir =Area::getWireDirection(info.wire);
            if((dir>0&&params.orientation==Area::OrientationCW) || 
               (dir<0&&params.orientation==Area::OrientationCCW))
                info.wire.Reverse();
        }

        TIME_INIT(t);
        if(params.abscissa<Precision::Confusion() || !info.isClosed) {
            gp_Pnt p1,p2;
            getEndPoints(info.wire,p1,p2);
            if(!info.isClosed && params.direction!=Area::DirectionNone) {
                bool reverse = false;
                switch(params.direction) {
                case Area::DirectionXPositive:
                    reverse = p1.X()>p2.X();
                    break;
                case Area::DirectionXNegative:
                    reverse = p1.X()<p2.X();
                    break;
                case Area::DirectionYPositive:
                    reverse = p1.Y()>p2.Y();
                    break;
                case Area::DirectionYNegative:
                    reverse = p1.Y()<p2.Y();
                    break;
                case Area::DirectionZPositive:
                    reverse = p1.Z()>p2.Z();
                    break;
                case Area::DirectionZNegative:
                    reverse = p1.Z()<p2.Z();
                    break;
                }
                if(reverse) {
                    info.wire.Reverse();
                    std::swap(p1,p2);
                }
            }
            // We don't add in-between vertices of an open wire, because we
            // haven't implemented open wire breaking yet.
            info.points.push_back(p1);
            if(params.direction!=Area::DirectionNone)
                info.points.push_back(p2);
        } else {
            // For closed wires, we are can easily rebase the wire, so we
            // discretize the wires to spatial index it in order to accelerate
            // nearest point searching
            for(BRepTools_WireExplorer xp(info.wire);xp.More();xp.Next()) {

                // push the head point
                info.points.push_back(BRep_Tool::Pnt(xp.CurrentVertex()));

                BRepAdaptor_Curve curve(xp.Current());
                GCPnts_UniformAbscissa discretizer(curve, params.abscissa, 
                        curve.FirstParameter(), curve.LastParameter());
                if (discretizer.IsDone()) {
                    int nbPoints = discretizer.NbPoints();
                    // OCC discretizer uses one-based index, so index one is
                    // the first point.  The tail point is added later, and the
                    // head point is the tail of the previous edge. So We can
                    // exclude the head and tail points, which is convenient
                    // since we don't need to check the orientation of the
                    // edge.
                    for (int i=2; i<nbPoints; i++) 
                        info.points.push_back(curve.Value(discretizer.Parameter(i)));
                }else
                    AREA_WARN("discretizer failed");
            }
            // no need to push the final tail point, since it's a closed wire
            // info.points.push_back(BRep_Tool::Pnt(xp.CurrentVertex()));
        }
        auto it = wires.end();
        --it;
        for(size_t i=0,count=info.points.size();i<count;++i)
            rtree.insert(RValue(it,i));
        DURATION_PLUS(params.bd,t);
    }
};

struct ShapeInfo{
    gp_Pln myPln;
    Wires myWires;
    RTree myRTree;
    TopoDS_Shape myShape;
    gp_Pnt myBestPt;
    gp_Pnt myStartPt;
    Wires::iterator myBestWire;
    TopoDS_Shape mySupport;
    ShapeParams &myParams;
    Standard_Real myBestParameter;
    bool mySupportEdge;
    bool myPlanar;
    bool myRebase;
    bool myStart;

    ShapeInfo(BRepLib_FindSurface &finder, const TopoDS_Shape &shape, ShapeParams &params)
        :myPln(GeomAdaptor_Surface(finder.Surface()).Plane())
        ,myShape(shape),myStartPt(1e20,1e20,1e20),myParams(params),myPlanar(true)
    {}

    ShapeInfo(const TopoDS_Shape &shape, ShapeParams &params)
        :myShape(shape),myStartPt(1e20,1e20,1e20),myParams(params),myPlanar(false)
    {}
    double nearest(const gp_Pnt &pt) {
        myStartPt = pt;

        if(myWires.empty()) 
            foreachSubshape(myShape,GetWires(myWires,myRTree,myParams),TopAbs_WIRE);
        
        // Now find the ture nearest point among the wires returned. Currently
        // only closed wire has a ture nearest point, using OCC's
        // BRepExtrema_DistShapeShape. We don't do this on open wires, because
        // we haven't implemented wire breaking on open wire yet, and I doubt
        // its usefulness.

        RResults ret;
        {
            TIME_INIT(t); 
            myRTree.query(bgi::nearest(pt,myParams.k),bgi::inserter(ret));
            DURATION_PLUS(myParams.qd,t);
        }

        TopoDS_Shape v = BRepBuilderAPI_MakeVertex(pt);
        bool first = true;
        double best_d=1e20;
        myBestWire = myWires.begin();
        for(auto r : ret) {
            Wires::iterator it = r.first;
            const TopoDS_Shape &wire = it->wire;
            TopoDS_Shape support;
            bool support_edge;
            double d = 0;
            gp_Pnt p;
            bool done = false;
            bool is_start = false;
            if(BRep_Tool::IsClosed(wire)) {
                TIME_INIT(t);
                BRepExtrema_DistShapeShape extss(v,wire);
                if(extss.IsDone() && extss.NbSolution()) {
                    d = extss.Value();
                    p = extss.PointOnShape2(1);
                    support = extss.SupportOnShape2(1);
                    support_edge = extss.SupportTypeShape2(1)==BRepExtrema_IsOnEdge;
                    if(support_edge) 
                        extss.ParOnEdgeS2(1,myBestParameter);
                    done = true;
                }else
                    AREA_WARN("BRepExtrema_DistShapeShape failed");
                DURATION_PLUS(myParams.xd,t);
            }
            if(!done){
                double d1 = pt.SquareDistance(it->pstart());
                if(myParams.direction==Area::DirectionNone) {
                    d = d1;
                    p = it->pstart();
                    is_start = true;
                }else{
                    double d2 = pt.SquareDistance(it->pend());
                    if(d1<d2) {
                        d = d1;
                        p = it->pstart();
                        is_start = true;
                    }else{
                        d = d2;
                        p = it->pend();
                        is_start = false;
                    }
                }
            }
            if(!first && d>=best_d) continue;
            first = false;
            myBestPt = p;
            myBestWire = it;
            best_d = d;
            myRebase = done;
            myStart = is_start;
            if(done) {
                mySupport = support;
                mySupportEdge = support_edge;
            }
        }
        return best_d;
    }

    //Assumes nearest() has been called. Rebased the best wire
    //to begin with the best point. Currently only works with closed wire
    TopoDS_Shape rebaseWire(gp_Pnt &pend, double min_dist) {
        min_dist *= min_dist;
        BRepBuilderAPI_MakeWire mkWire;
        TopoDS_Shape estart;
        TopoDS_Edge eend;

        for(int state=0;state<3;++state) {
            BRepTools_WireExplorer xp(TopoDS::Wire(myBestWire->wire));
            gp_Pnt pprev(BRep_Tool::Pnt(xp.CurrentVertex()));

            //checking the case of bestpoint == wire start
            if(state==0 && !mySupportEdge && 
               pprev.SquareDistance(myBestPt)<Precision::SquareConfusion()) {
                pend = myBestWire->pend();
                return myBestWire->wire;
            }

            gp_Pnt pt;
            for(;xp.More();xp.Next(),pprev=pt) {
                const auto &edge = xp.Current();

                //state==2 means we are in second pass. estart marks the new
                //start of the wire.  so seeing estart means we're done
                if(state==2 && estart.IsEqual(edge))
                    break;

                // Edge split not working if using BRepAdaptor_Curve.
                // BRepBuilderAPI_MakeEdge always fails with
                // PointProjectionFailed. Why??

                Standard_Real first,last;
                Handle_Geom_Curve curve = BRep_Tool::Curve(edge, first, last);
                pt = curve->Value(last);
                bool reversed;
                if(pprev.SquareDistance(pt)<Precision::SquareConfusion()) {
                    reversed = true;
                    pt = curve->Value(first);
                }else
                    reversed = false;

                //state!=0 means we've found the new start of wire, now just
                //keep adding new edges
                if(state) {
                    mkWire.Add(edge);
                    continue;
                }
                //state==0 means we are looking for the new start
                if(mySupportEdge) {
                    //if best point is on some edge, split the edge in half
                    if(edge.IsEqual(mySupport)) {
                        double d1 = pprev.SquareDistance(myBestPt);
                        double d2 = pt.SquareDistance(myBestPt);

                        if(d1>min_dist && d2>min_dist) {
                            BRepBuilderAPI_MakeEdge mkEdge1,mkEdge2;
                            if(reversed) {
                                mkEdge1.Init(curve, myBestPt, pprev);
                                mkEdge2.Init(curve, pt, myBestPt);
                            }else{
                                mkEdge1.Init(curve, pprev, myBestPt);
                                mkEdge2.Init(curve, myBestPt, pt);
                            }
                            // Using parameter is not working, why?
                            // if(reversed) {
                            //     mkEdge1.Init(curve, myBestParameter, last);
                            //     mkEdge2.Init(curve, first, myBestParameter);
                            // }else{
                            //     mkEdge1.Init(curve, first, myBestParameter);
                            //     mkEdge2.Init(curve, myBestParameter, last);
                            // }
                            if(mkEdge1.IsDone() && mkEdge2.IsDone()) {
                                if(reversed) {
                                    eend = TopoDS::Edge(mkEdge1.Edge().Reversed());
                                    mkWire.Add(TopoDS::Edge(mkEdge2.Edge().Reversed()));
                                }else{
                                    eend = mkEdge1.Edge();
                                    mkWire.Add(mkEdge2.Edge());
                                }
                                pend = myBestPt;
                                estart = mySupport;
                                state = 1;
                                // AREA_TRACE((reversed?"reversed ":"")<<"edge split "<<AREA_XYZ(pprev)<<", " <<
                                //         AREA_XYZ(myBestPt)<< ", "<<AREA_XYZ(pt)<<", "<<d1<<", "<<d2 <<", ("<<
                                //         first<<", " << myBestParameter << ", " << last<<')');
                                continue;
                            }
                            AREA_WARN((reversed?"reversed ":"")<<"edge split failed "<<AREA_XYZ(pprev)<<", " << 
                                    AREA_XYZ(myBestPt)<< ", "<<AREA_XYZ(pt)<<", "<<d1<<", "<<d2<<", err: " <<
                                    mkEdge1.Error() << ", " << mkEdge2.Error());
                        }

                        if(d1<d2) {
                            pend = pprev;
                            // AREA_TRACE("split edge->start");
                            estart = edge;
                            state = 1;
                            mkWire.Add(edge);
                        }else{
                            // AREA_TRACE("split edge->end");
                            mySupportEdge = false;
                            myBestPt = pt;
                            continue;
                        }
                    }
                }else if(myBestPt.SquareDistance(pprev)<Precision::SquareConfusion()){
                    pend = pprev;
                    // AREA_TRACE("break vertex");
                    //if best point is on some vertex
                    estart = edge;
                    state = 1;
                    mkWire.Add(edge);
                }
            }

            if(state==0) {
                AREA_WARN("edge break point not found");
                pend = myBestWire->pend();
                return myBestWire->wire;
            }
        }
        if(!eend.IsNull())
            mkWire.Add(eend);
        if(mkWire.IsDone())
            return mkWire.Wire();
        AREA_WARN("wire rebase failed");
        pend = myBestWire->pend();
        return myBestWire->wire;
    }

    std::list<TopoDS_Shape> sortWires(const gp_Pnt &pstart, gp_Pnt &pend,double min_dist) {

        if(pstart.SquareDistance(myStartPt)>Precision::SquareConfusion())
            nearest(pstart);

        std::list<TopoDS_Shape> wires;
        if(min_dist < 0.01)
            min_dist = 0.01;
        while(true) {
            if(myRebase) {
                pend = myBestPt;
                wires.push_back(rebaseWire(pend,min_dist));
            }else if(!myStart){
                wires.push_back(myBestWire->wire.Reversed());
                pend = myBestWire->pstart();
            }else{
                wires.push_back(myBestWire->wire);
                pend = myBestWire->pend();
            }
            TIME_INIT(t);
            for(size_t i=0,count=myBestWire->points.size();i<count;++i)
                myRTree.remove(RValue(myBestWire,i));
            DURATION_PLUS(myParams.rd,t);
            myWires.erase(myBestWire);
            if(myWires.empty()) break;
            nearest(pend);
        }
        return std::move(wires);
    }
};

struct ShapeInfoBuilder {
    std::list<ShapeInfo> &myList;
    gp_Trsf &myTrsf;
    short *myArcPlane;
    bool &myArcPlaneFound;
    ShapeParams &myParams;

    ShapeInfoBuilder(bool &plane_found, short *arc_plane, gp_Trsf &trsf, 
            std::list<ShapeInfo> &list, ShapeParams &params)
        :myList(list) ,myTrsf(trsf) ,myArcPlane(arc_plane)
        ,myArcPlaneFound(plane_found), myParams(params)
    {}

    void operator()(const TopoDS_Shape &shape, int type) {
        BRepLib_FindSurface finder(shape,-1,Standard_True);
        if(!finder.Found()) {
            myList.push_back(ShapeInfo(shape,myParams));
            return;
        }
        myList.push_back(ShapeInfo(finder,shape,myParams));
        if(myArcPlane==NULL || myArcPlaneFound ||
           *myArcPlane==Area::ArcPlaneNone || 
           *myArcPlane==Area::ArcPlaneVariable)
            return;

        if(type == TopAbs_EDGE) {
            BRepAdaptor_Curve curve(TopoDS::Edge(shape));
            if(curve.GetType()!=GeomAbs_Circle) return;
        }else{
            bool found = false;
            for(TopExp_Explorer it(shape,TopAbs_EDGE);it.More();it.Next()) {
                BRepAdaptor_Curve curve(TopoDS::Edge(it.Current()));
                if(curve.GetType()==GeomAbs_Circle) {
                    found = true;
                    break;
                }
            }
            if(!found) return;
        }
        gp_Ax3 pos = myList.back().myPln.Position();
        if(!pos.Direct()) pos = gp_Ax3(pos.Ax2());
        const gp_Dir &dir = pos.Direction();
        gp_Ax3 dstPos;
        bool x0 = fabs(dir.X())<Precision::Confusion();
        bool y0 = fabs(dir.Y())<Precision::Confusion();
        bool z0 = fabs(dir.Z())<Precision::Confusion();
        switch(*myArcPlane) {
        case Area::ArcPlaneAuto: {
            if(x0&&y0){
                AREA_TRACE("found arc plane XY");
                *myArcPlane = Area::ArcPlaneXY;
            } else if(x0&&z0) {
                AREA_TRACE("found arc plane ZX");
                *myArcPlane = Area::ArcPlaneZX;
            } else if(z0&&y0) {
                AREA_TRACE("found arc plane YZ");
                *myArcPlane = Area::ArcPlaneYZ;
            } else {
                *myArcPlane = Area::ArcPlaneXY;
                dstPos = gp_Ax3(pos.Location(),gp_Dir(0,0,1));
                break;
            }
            myArcPlaneFound = true;
            return;
        }case Area::ArcPlaneXY:
            if(x0&&y0) {myArcPlaneFound=true;return;}
            dstPos = gp_Ax3(pos.Location(),gp_Dir(0,0,1));
            break;
        case Area::ArcPlaneZX:
            if(x0&&z0) {myArcPlaneFound=true;return;}
            dstPos = gp_Ax3(pos.Location(),gp_Dir(0,1,0));
            break;
        case Area::ArcPlaneYZ:
            if(z0&&y0) {myArcPlaneFound=true;return;}
            dstPos = gp_Ax3(pos.Location(),gp_Dir(1,0,0));
            break;
        default:
            return;
        }
        AREA_WARN("force arc plane " << AREA_XYZ(dir) << 
                " to " << AREA_XYZ(dstPos.Direction()));
        myTrsf.SetTransformation(pos);
        gp_Trsf trsf;
        trsf.SetTransformation(dstPos);
        myTrsf.PreMultiply(trsf.Inverted());
        myArcPlaneFound = true;
    }
};

struct WireOrienter {
    std::list<TopoDS_Shape> &wires;
    short orientation;
    short direction;

    WireOrienter(std::list<TopoDS_Shape> &ws, short o, short d)
        :wires(ws),orientation(o),direction(d)
    {}

    void operator()(const TopoDS_Shape &shape, int type) {
        if(type == TopAbs_WIRE)
            wires.push_back(shape);
        else
            wires.push_back(BRepBuilderAPI_MakeWire(TopoDS::Edge(shape)).Wire());

        TopoDS_Shape &wire = wires.back();

        if(BRep_Tool::IsClosed(wire)) {
            if(orientation!=Area::OrientationNone) {
                int dir = Area::getWireDirection(wire);
                if((dir>0&&orientation==Area::OrientationCW) || 
                (dir<0&&orientation==Area::OrientationCCW))
                    wire.Reverse();
            }
        }else if(direction!=Area::DirectionNone) {
            gp_Pnt p1,p2;
            getEndPoints(TopoDS::Wire(wire),p1,p2);
            bool reverse = false;
            switch(direction) {
            case Area::DirectionXPositive:
                reverse = p1.X()>p2.X();
                break;
            case Area::DirectionXNegative:
                reverse = p1.X()<p2.X();
                break;
            case Area::DirectionYPositive:
                reverse = p1.Y()>p2.Y();
                break;
            case Area::DirectionYNegative:
                reverse = p1.Y()<p2.Y();
                break;
            case Area::DirectionZPositive:
                reverse = p1.Z()>p2.Z();
                break;
            case Area::DirectionZNegative:
                reverse = p1.Z()<p2.Z();
                break;
            }
            if(reverse)
                wire.Reverse();
        }
    }
};

std::list<TopoDS_Shape> Area::sortWires(const std::list<TopoDS_Shape> &shapes, 
    const gp_Pnt *_pstart, gp_Pnt *_pend, short *arc_plane, 
    PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SORT))
{
    std::list<TopoDS_Shape> wires;

    if(shapes.empty()) return wires;

    if(sort_mode == SortModeNone) {
        for(auto &shape : shapes) {
            if(!shape.IsNull())
                foreachSubshape(shape,
                    WireOrienter(wires,orientation,direction), TopAbs_WIRE);
        }
        return std::move(wires);
    }

    ShapeParams rparams(abscissa,nearest_k>0?nearest_k:1,orientation,direction);
    std::list<ShapeInfo> shape_list;

    TIME_INIT2(t,t1);

    gp_Trsf trsf;
    bool arcPlaneFound = false;

    if(sort_mode == SortMode3D) {
        TopoDS_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for(auto &shape : shapes) {
            if(!shape.IsNull())
                builder.Add(comp,shape);
        }
        TopExp_Explorer xp(comp,TopAbs_EDGE);
        if(xp.More()) 
            shape_list.push_back(ShapeInfo(comp,rparams));
    }else{
        //first pass, find plane of each shape
        for(auto &shape : shapes) {
            //explode the shape
            if(!shape.IsNull()){
                foreachSubshape(shape,ShapeInfoBuilder(
                    arcPlaneFound,arc_plane,trsf,shape_list,rparams));
            }
        }
        TIME_PRINT(t1,"plane finding");
    }

    if(shape_list.empty()) 
        return wires;
 
    Bnd_Box bounds;
    gp_Pnt pstart,pend;
    if(_pstart) 
        pstart = *_pstart;
    bool use_bound = fabs(pstart.X())<Precision::Confusion() &&
                     fabs(pstart.Y())<Precision::Confusion() &&
                     fabs(pstart.Z())<Precision::Confusion();

    if(use_bound || sort_mode == SortMode2D5) {
        //Second stage, group shape by its plane, and find overall boundary

        if(arcPlaneFound || use_bound) {
            for(auto &info : shape_list) {
                if(arcPlaneFound) {
                    info.myShape.Move(trsf);
                    if(info.myPlanar) info.myPln.Transform(trsf);
                }
                if(use_bound)
                    BRepBndLib::Add(info.myShape, bounds, Standard_False);
            }
        }
        
        for(auto itNext=shape_list.begin(),it=itNext;it!=shape_list.end();it=itNext) {
            ++itNext;
            if(!it->myPlanar) continue;
            TopoDS_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            bool empty = true;
            for(auto itNext3=itNext,itNext2=itNext;itNext2!=shape_list.end();itNext2=itNext3) {
                ++itNext3;
                if(!itNext2->myPlanar || 
                   !it->myPln.Position().IsCoplanar(itNext2->myPln.Position(),
                       Precision::Confusion(),Precision::Confusion())) 
                    continue;
                if(itNext == itNext2) ++itNext;
                builder.Add(comp,itNext2->myShape);
                shape_list.erase(itNext2);
                empty = false;
            }
            if(!empty) {
                builder.Add(comp,it->myShape);
                it->myShape = comp;
            }
        }
        TIME_PRINT(t,"plane merging");
    }

    DURATION_INIT(td);

    if(use_bound) {
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        AREA_TRACE("bound (" << xMin<<", "<<xMax<<"), ("<<
                yMin<<", "<<yMax<<"), ("<<zMin<<", "<<zMax<<')');
        pstart.SetCoord(xMax,yMax,zMax);
    }
    while(shape_list.size()) {
        AREA_TRACE("sorting " << shape_list.size() << ' ' << AREA_XYZ(pstart));
        double best_d;
        auto best_it = shape_list.begin();
        bool first = true;
        for(auto it=best_it;it!=shape_list.end();++it) {
            double d;
            gp_Pnt pt;
            if(it->myPlanar)
                d = it->myPln.SquareDistance(pstart);
            else
                d = it->nearest(pstart);
            if(first || d<best_d) {
                first = false;
                best_it = it;
                best_d = d;
            }
        }
        wires.splice(wires.end(),best_it->sortWires(pstart,pend,min_dist));
        pstart = pend;
        shape_list.erase(best_it);
    }
    if(_pend) *_pend = pend;
    DURATION_PRINT(rparams.bd,"rtree build");
    DURATION_PRINT(rparams.qd,"rtree query");
    DURATION_PRINT(rparams.rd,"rtree clean");
    DURATION_PRINT(rparams.xd,"BRepExtrema");
    TIME_PRINT(t,"sortWires total");
    return std::move(wires);
}

static inline void addParameter(bool verbose, Command &cmd, const char *name,
        double last, double next, bool relative=false)
{
    double d = next-last;
    if(verbose || fabs(d)>Precision::Confusion())
        cmd.Parameters[name] = relative?d:next;
}

static inline void addGCode(bool verbose, Toolpath &path, const gp_Pnt &last, 
        const gp_Pnt &next, const char *name) 
{
    Command cmd;
    cmd.Name = name;
    addParameter(verbose,cmd,"X",last.X(),next.X());
    addParameter(verbose,cmd,"Y",last.Y(),next.Y());
    addParameter(verbose,cmd,"Z",last.Z(),next.Z());
    path.addCommand(cmd);
    return;
}

static inline void addG1(bool verbose,Toolpath &path, const gp_Pnt &last, 
        const gp_Pnt &next, double f, double &last_f) 
{
    addGCode(verbose,path,last,next,"G1");
    if(f>Precision::Confusion()) {
        Command *cmd = path.getCommands().back();
        addParameter(verbose,*cmd,"F",last_f,f);
        last_f = f;
    }
    return;
}

typedef Standard_Real (gp_Pnt::*AxisGetter)() const;
typedef void (gp_Pnt::*AxisSetter)(Standard_Real);

static void addG0(bool verbose, Toolpath &path, 
        gp_Pnt last, const gp_Pnt &next, 
        AxisGetter getter, AxisSetter setter,
        double retraction, double clearance, 
        double f, double &last_f)
{
    if(!getter || retraction-(last.*getter)() < Precision::Confusion()) {
        addGCode(verbose,path,last,next,"G0");
        return;
    }
    gp_Pnt pt(last);
    (pt.*setter)(retraction);
    addGCode(verbose,path,last,pt,"G0");
    last = pt;
    pt = next;
    (pt.*setter)(retraction);
    addGCode(verbose,path,last,pt,"G0");
    if(clearance>Precision::Confusion() && 
       clearance+(next.*getter)() < retraction)
    {
        last = pt;
        pt = next;
        (pt.*setter)((next.*getter)()+clearance);
        addGCode(verbose,path,last,pt,"G0");
        addG1(verbose,path,pt,next,f,last_f);
    }else
        addGCode(verbose,path,pt,next,"G0");
}

static void addGArc(bool verbose,bool abs_center, Toolpath &path, 
        const gp_Pnt &pstart, const gp_Pnt &pend, const gp_Pnt &center, 
        bool clockwise, double f, double &last_f) 
{
    Command cmd;
    cmd.Name = clockwise?"G2":"G3";
    if(abs_center) {
        addParameter(verbose,cmd,"I",0.0,center.X());
        addParameter(verbose,cmd,"J",0.0,center.Y());
        addParameter(verbose,cmd,"K",0.0,center.Z());
    }else{
        addParameter(verbose,cmd,"I",pstart.X(),center.X(),true);
        addParameter(verbose,cmd,"J",pstart.Y(),center.Y(),true);
        addParameter(verbose,cmd,"K",pstart.Z(),center.Z(),true);
    }
    addParameter(verbose,cmd,"X",pstart.X(),pend.X());
    addParameter(verbose,cmd,"Y",pstart.Y(),pend.Y());
    addParameter(verbose,cmd,"Z",pstart.Z(),pend.Z());
    if(f>Precision::Confusion()) {
        addParameter(verbose,cmd,"F",last_f,f);
        last_f = f;
    }
    path.addCommand(cmd);
}

static inline void addGCode(Toolpath &path, const char *name) {
    Command cmd;
    cmd.Name = name;
    path.addCommand(cmd);
}

int Area::getWireDirection(const TopoDS_Shape &shape, const gp_Pln *pln) {
    gp_Dir dir;
    if(pln) 
        dir = pln->Axis().Direction();
    else{
        BRepLib_FindSurface finder(shape,-1,Standard_True);
        if(!finder.Found()) return 0;
        dir = GeomAdaptor_Surface(finder.Surface()).Plane().Axis().Direction();
    }
    const TopoDS_Wire &wire = TopoDS::Wire(shape);
    //make a test face
    BRepBuilderAPI_MakeFace mkFace(wire, /*onlyplane=*/Standard_True);
    if(!mkFace.IsDone()) return 0;
    TopoDS_Face tmpFace = mkFace.Face();
    //compare face surface normal with our plane's one
    BRepAdaptor_Surface surf(tmpFace);
    bool normal_co = surf.Plane().Axis().Direction().Dot(dir) > 0;

    //unlikely, but just in case OCC decided to reverse our wire for the face...  take that into account!
    TopoDS_Iterator it(tmpFace, /*CumOri=*/Standard_False);
    normal_co ^= it.Value().Orientation() != wire.Orientation();

    return normal_co ? 1 : -1;
}

void Area::toPath(Toolpath &path, const std::list<TopoDS_Shape> &shapes,
        const gp_Pnt *pstart, gp_Pnt *pend, PARAM_ARGS(PARAM_FARG,AREA_PARAMS_PATH))
{
    std::list<TopoDS_Shape> wires;

    wires = sortWires(shapes,pstart,pend,
            PARAM_REF(PARAM_FARG,AREA_PARAMS_ARC_PLANE), 
            PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_SORT));

    // absolute mode
    addGCode(path,"G90");
    if(abs_center)
        addGCode(path,"G90.1"); // absolute center for arc move

    short currentArcPlane = arc_plane;
    if(arc_plane==ArcPlaneZX)
        addGCode(path,"G18");
    else if(arc_plane==ArcPlaneYZ)
        addGCode(path,"G19");
    else {
        currentArcPlane=ArcPlaneXY;
        addGCode(path,"G17");
    }
    
    threshold = fabs(threshold);
    if(threshold < Precision::Confusion())
        threshold = Precision::Confusion();
    clearance = fabs(clearance);

    AxisGetter getter = &gp_Pnt::Z;
    AxisSetter setter = &gp_Pnt::SetZ;
    retraction = fabs(retraction);
    if(retraction>Precision::Confusion()) {
        switch(retract_axis) {
        case RetractAxisX:
            getter = &gp_Pnt::X;
            setter = &gp_Pnt::SetX;
            break;
        case RetractAxisY:
            getter = &gp_Pnt::Y;
            setter = &gp_Pnt::SetY;
            break;
        }
    }

    gp_Pnt plast,p;
    if(pstart) plast = *pstart;
    bool first = true;
    bool arcWarned = false;
    double cur_f = 0.0; // current feed rate
    double nf = fabs(feedrate); // user specified normal move feed rate
    double vf = fabs(feedrate_v); // user specified vertical move feed rate
    if(vf < Precision::Confusion()) vf = nf;

    for(const TopoDS_Shape &wire : wires) {

        BRepTools_WireExplorer xp(TopoDS::Wire(wire));
        p = BRep_Tool::Pnt(xp.CurrentVertex());

        gp_Pnt pTmp(p),plastTmp(plast);
        // Assuming the stepdown direction is the same as retraction direction.
        // We don't want to count step down distance in stepdown direction,
        // because it is always safe to go in that direction in feed move
        // without getting bumped.
        (pTmp.*setter)(0.0);
        (plastTmp.*setter)(0.0);

        if(first||pTmp.SquareDistance(plastTmp)>threshold)
            addG0(verbose,path,plast,p,getter,setter,retraction,clearance,vf,cur_f);
        else
            addG1(verbose,path,plast,p,vf,cur_f);
        plast = p;
        first = false;
        for(;xp.More();xp.Next(),plast=p) {
            const auto &edge = xp.Current();
            BRepAdaptor_Curve curve(edge);
            bool reversed = (edge.Orientation()==TopAbs_REVERSED);
            p = curve.Value(reversed?curve.FirstParameter():curve.LastParameter());

            switch (curve.GetType()) {
            case GeomAbs_Line: {
                if(segmentation > Precision::Confusion()) {
                    GCPnts_UniformAbscissa discretizer(curve, segmentation, 
                            curve.FirstParameter(), curve.LastParameter());
                    if (discretizer.IsDone () && discretizer.NbPoints () > 2) {
                        int nbPoints = discretizer.NbPoints ();
                        if(reversed) {
                            for (int i=nbPoints-1; i>=1; --i) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addG1(verbose,path,plast,pt,nf,cur_f);
                                plast = pt;
                            }
                        }else{
                            for (int i=2; i<=nbPoints; i++) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addG1(verbose,path,plast,pt,nf,cur_f);
                                plast = pt;
                            }
                        }
                        break;
                    }
                }
                addG1(verbose,path,plast,p,nf,cur_f);
                break;
            } case GeomAbs_Circle:{
                const gp_Circ &circle = curve.Circle();
                const gp_Dir &dir = circle.Axis().Direction();
                short arcPlane = ArcPlaneNone;
                bool clockwise;
                const char *cmd;
                if(fabs(dir.X())<Precision::Confusion() &&
                   fabs(dir.Y())<Precision::Confusion()) {
                    clockwise = dir.Z()<0;
                    arcPlane = ArcPlaneXY;
                    cmd = "G17";
                }else if(fabs(dir.Z())<Precision::Confusion() &&
                         fabs(dir.X())<Precision::Confusion()){
                    clockwise = dir.Y()<0;
                    arcPlane = ArcPlaneZX;
                    cmd = "G18";
                }else if(fabs(dir.Y())<Precision::Confusion() &&
                         fabs(dir.Z())<Precision::Confusion()){
                    clockwise = dir.X()<0;
                    arcPlane = ArcPlaneYZ;
                    cmd = "G19";
                }

                if(arcPlane!=ArcPlaneNone &&
                   (arcPlane==currentArcPlane || arc_plane==ArcPlaneVariable))
                {
                    if(arcPlane!=currentArcPlane)
                        addGCode(path,cmd);

                    if(reversed) clockwise = !clockwise;
                    gp_Pnt center = circle.Location();

                    double first = curve.FirstParameter();
                    double last = curve.LastParameter();
                    if(segmentation > Precision::Confusion()) {
                        GCPnts_UniformAbscissa discretizer(curve,segmentation,first,last);
                        if (discretizer.IsDone () && discretizer.NbPoints () > 2) {
                            int nbPoints = discretizer.NbPoints ();
                            if(reversed) {
                                for (int i=nbPoints-1; i>=1; --i) {
                                    gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                    addGArc(verbose,abs_center,path,plast,pt,center,clockwise,nf,cur_f);
                                    plast = pt;
                                }
                            }else{
                                for (int i=2; i<=nbPoints; i++) {
                                    gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                    addGArc(verbose,abs_center,path,plast,pt,center,clockwise,nf,cur_f);
                                    plast = pt;
                                }
                            }
                            break;
                        }
                    }

                    if(fabs(first-last)>M_PI) {
                        // Split arc(circle) larger than half circle. 
                        gp_Pnt mid = curve.Value((last-first)*0.5+first);
                        addGArc(verbose,abs_center,path,plast,mid,center,clockwise,nf,cur_f);
                        plast = mid;
                    }
                    addGArc(verbose,abs_center,path,plast,p,center,clockwise,nf,cur_f);
                    break;
                }

                if(!arcWarned){
                    arcWarned = true;
                    AREA_WARN("arc plane not aligned, force discretization");
                }
                AREA_TRACE("arc discretize " << AREA_XYZ(dir));
                //fall through

            } default: {
                // Discretize all other type of curves
                GCPnts_QuasiUniformDeflection discretizer(curve, deflection, 
                        curve.FirstParameter(), curve.LastParameter());
                if (discretizer.IsDone () && discretizer.NbPoints () > 1) {
                    int nbPoints = discretizer.NbPoints ();
                    if(reversed) {
                        for (int i=nbPoints-1; i>=1; --i) {
                            gp_Pnt pt = discretizer.Value (i);
                            addG1(verbose,path,plast,pt,nf,cur_f);
                            plast = pt;
                        }
                    }else{
                        for (int i=2; i<=nbPoints; i++) {
                            gp_Pnt pt = discretizer.Value (i);
                            addG1(verbose,path,plast,pt,nf,cur_f);
                            plast = pt;
                        }
                    }
                }else
                    Standard_Failure::Raise("Curve discretization failed");
            }}
        }
    }
}

void Area::abort(bool aborting) {
    s_aborting = aborting;
}

bool Area::aborting() {
    return s_aborting;
}

AreaStaticParams::AreaStaticParams()
    :PARAM_INIT(PARAM_FNAME,AREA_PARAMS_EXTRA_CONF)
{}
    
AreaStaticParams Area::s_params;

void Area::setDefaultParams(const AreaStaticParams &params){
    s_params = params;
}

const AreaStaticParams &Area::getDefaultParams() {
    return s_params;
}

#define AREA_LOG_CHECK_DEFINE(_1,_2,_elem) \
bool Area::BOOST_PP_CAT(_elem,Enabled)() {\
    return s_params.LogLevel >= BOOST_PP_CAT(LogLevel,_elem);\
}
BOOST_PP_SEQ_FOR_EACH(AREA_LOG_CHECK_DEFINE,_,AREA_PARAM_LOG_LEVEL)
