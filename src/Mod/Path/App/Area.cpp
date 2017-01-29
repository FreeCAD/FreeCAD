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

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/range/adaptor/reversed.hpp>

#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
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
#include <BRepBndLib.hxx>
#include <BRepLib_MakeFace.hxx>
#include <Bnd_Box.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Console.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FaceMakerBullseye.h>
#include <Mod/Part/App/CrossSection.h>
#include "Area.h"
#include "../libarea/Area.h"

using namespace Path;
using namespace boost::posix_time;

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

    // Arc fitting is lossy. we shall reduce the number of unecessary fit
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

Area::Area(const AreaParams *params)
:myHaveFace(false)
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
,myShapePlane(other.myShapePlane)
,myWorkPlane(other.myWorkPlane)
,myHaveFace(other.myHaveFace)
,myHaveSolid(other.myHaveSolid)
,myShapeDone(false)
{
    if(!deep_copy || !other.isBuilt())
        return;
    if(other.myArea)
        myArea.reset(new CArea(*other.myArea));
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
    if(shape.IsNull()) {
        myWorkPlane.Nullify();
        return;
    }
    TopoDS_Shape plane;
    gp_Trsf trsf;
    findPlane(shape,plane,trsf);
    if (plane.IsNull())
        throw Base::ValueError("shape is not planar");
    myWorkPlane = plane;
    myTrsf = trsf;
    clean();
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

int Area::add(CArea &area, const TopoDS_Shape &shape, const gp_Trsf *trsf, 
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
            add(area,TopoDS::Wire(it.Current()),trsf,deflection);
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
            add(_area,wire,trsf,deflection);
        else if(to_edges) {
            for (TopExp_Explorer it(wire, TopAbs_EDGE); it.More(); it.Next())
                add(_areaOpen,BRepBuilderAPI_MakeWire(
                    TopoDS::Edge(it.Current())).Wire(),trsf,deflection,true);
        }else
            add(_areaOpen,wire,trsf,deflection);
    }

    if(!haveShape) {
        for (TopExp_Explorer it(shape, TopAbs_EDGE); it.More(); it.Next()) {
            if(plane && !isCoplanar(it.Current(),*plane)) {
                ++skipped;
                if(force_coplanar) continue;
            }
            TopoDS_Wire wire = BRepBuilderAPI_MakeWire(
                                    TopoDS::Edge(it.Current())).Wire();
            add(BRep_Tool::IsClosed(wire)?_area:_areaOpen,wire,trsf,deflection);
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

void Area::add(CArea &area, const TopoDS_Wire& wire,
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
                gp_Ax1 axis = circle.Axis();
                int dir = axis.Direction().Z()<0?-1:1;
                if(reversed) dir = -dir;
                gp_Pnt loc = axis.Location();
                if(fabs(first-last)>M_PI) {
                    // Split arc(circle) larger than half circle. Because gcode
                    // can't handle full circle?
                    gp_Pnt mid = curve.Value((last-first)*0.5+first);
                    ccurve.append(CVertex(dir,Point(mid.X(),mid.Y()),
                                Point(loc.X(),loc.Y())));
                }
                ccurve.append(CVertex(dir,Point(p.X(),p.Y()),
                            Point(loc.X(),loc.Y())));
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
            Base::Console().Warning("ccurve not closed\n");
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
    if(deleteShapes){
        myShapePlane.Nullify();
        myShapes.clear();
        myHaveFace = false;
        myHaveSolid = false;
    }
}

void Area::add(const TopoDS_Shape &shape,short op) {
#define AREA_CONVERT_OP \
    ClipperLib::ClipType Operation;\
    switch(op){\
    case OperationUnion:\
        Operation = ClipperLib::ctUnion;\
        break;\
    case OperationDifference:\
        Operation = ClipperLib::ctDifference;\
        break;\
    case OperationIntersection:\
        Operation = ClipperLib::ctIntersection;\
        break;\
    case OperationXor:\
        Operation = ClipperLib::ctXor;\
        break;\
    default:\
        throw Base::ValueError("invalid Operation");\
    }

    if(shape.IsNull())
        throw Base::ValueError("null shape");

    if(op!=OperationCompound) {
        AREA_CONVERT_OP;
        Q_UNUSED(Operation);
    }
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
#define AREA_SRC2(_param) params.PARAM_FNAME(_param)
    // Validate all enum type of parameters
    PARAM_ENUM_CHECK(AREA_SRC2,PARAM_ENUM_EXCEPT,AREA_PARAMS_CONF);
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
    mySkippedShapes += add(area,shape,&myTrsf,myParams.Deflection,
            myParams.Coplanar==CoplanarNone?NULL:&plane,
            myHaveSolid||myParams.Coplanar==CoplanarForce,&areaOpen,
            myParams.OpenMode==OpenModeEdges,myParams.Reorient);
    if(areaOpen.m_curves.size()) {
        if(&area == myArea.get() || myParams.OpenMode == OpenModeNone)
            myAreaOpen->m_curves.splice(myAreaOpen->m_curves.end(),areaOpen.m_curves);
        else
            Base::Console().Warning("open wires discarded in clipping shapes\n");
    }
}

namespace Part {
extern PartExport std::list<TopoDS_Edge> sort_Edges(double tol3d, std::list<TopoDS_Edge>& edges);
}

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
                add(*myArea,BRepBuilderAPI_MakeWire(
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
        add(*myArea,BRepBuilderAPI_MakeWire(
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

bool Area::findPlane(const TopoDS_Shape &shape,
                    TopoDS_Shape &plane, gp_Trsf &trsf) 
{
    return findPlane(shape,TopAbs_FACE,plane,trsf) ||
           findPlane(shape,TopAbs_WIRE,plane,trsf) ||
           findPlane(shape,TopAbs_EDGE,plane,trsf);
}

bool Area::findPlane(const TopoDS_Shape &shape, int type, 
                    TopoDS_Shape &dst, gp_Trsf &dst_trsf) 
{
    bool haveShape = false;
    double top_z;
    bool top_found = false;
    gp_Trsf trsf;
    for(TopExp_Explorer it(shape,(TopAbs_ShapeEnum)type); it.More(); it.Next()) {
        haveShape = true;
        const TopoDS_Shape &plane = it.Current();
        BRepLib_FindSurface planeFinder(plane,-1,Standard_True);
        if (!planeFinder.Found())
            continue;
        gp_Ax3 pos = GeomAdaptor_Surface(planeFinder.Surface()).Plane().Position();
        gp_Dir dir(pos.Direction());
        trsf.SetTransformation(pos);

        //pos.Location().Z() is always 0, why? As a walk around, use the first vertex Z
        gp_Pnt origin = pos.Location();

        for(TopExp_Explorer it(plane.Moved(trsf),TopAbs_VERTEX);it.More();) {
            origin.SetZ(BRep_Tool::Pnt(TopoDS::Vertex(it.Current())).Z());
            break;
        }

        if(fabs(dir.X())<Precision::Confusion() &&
           fabs(dir.Y())<Precision::Confusion()) 
        {
            if(top_found && top_z > origin.Z())
                continue;
            top_found = true;
            top_z = origin.Z();
        }else if(!dst.IsNull())
            continue;
        dst = plane;

        //Some how the plane returned by BRepLib_FindSurface has Z always set to 0.
        //We need to manually translate Z to its actual value
        gp_Trsf trsf2;
        trsf2.SetTranslationPart(gp_XYZ(0,0,-origin.Z()));
        dst_trsf = trsf.Multiplied(trsf2);
    }
    return haveShape;
}

std::vector<shared_ptr<Area> > Area::makeSections(
        PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
        const std::vector<double> &_heights,
        const TopoDS_Shape &_plane)
{
    TopoDS_Shape plane;
    gp_Trsf trsf;

    if(!_plane.IsNull())
        findPlane(_plane,plane,trsf);
    else
        plane = getPlane(&trsf);

    if(plane.IsNull())
        throw Base::ValueError("failed to obtain section plane");

    TopLoc_Location loc(trsf);

    Bnd_Box bounds;
    for(const Shape &s : myShapes) {
        const TopoDS_Shape &shape = s.shape.Moved(loc);
        BRepBndLib::Add(shape, bounds, Standard_False);
    }
    bounds.SetGap(0.0);
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    bool hit_bottom = false;
    std::vector<double> heights;
    if(_heights.empty()) {
        if(mode != SectionModeAbsolute && myParams.SectionOffset<0)
            throw Base::ValueError("only positive section offset is allowed in non-absolute mode");
        if(myParams.SectionCount>1 && myParams.Stepdown<Precision::Confusion())
            throw Base::ValueError("invalid stepdown");

        if(mode == SectionModeBoundBox)
            zMax -= myParams.SectionOffset;
        else if(mode == SectionModeWorkplane)
            zMax = -myParams.SectionOffset;
        else {
            gp_Pnt pt(0,0,myParams.SectionOffset);
            double z = pt.Transformed(loc).Z();
            if(z < zMax)
                zMax = z;
        }
        if(zMax <= zMin)
            throw Base::ValueError("section offset too big");

        int count = myParams.SectionCount;
        if(count<0 || count*myParams.Stepdown > zMax-zMin) {
            count = ceil((zMax-zMin)/myParams.Stepdown);
            if((count-1)*myParams.Stepdown < zMax-zMin)
                ++count;
        }
        heights.reserve(count);
        for(int i=0;i<count;++i,zMax-=myParams.Stepdown) {
            if(zMax < zMin) {
                hit_bottom = true;
                break;
            }
            heights.push_back(zMax);
        }
    }else{
        heights.reserve(_heights.size());
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
            if((zMin-z)>Precision::Confusion()) {
                hit_bottom = true;
                continue;
            }else if ((z-zMax)>Precision::Confusion())
                continue;
            heights.push_back(z);
        }
    }

    if(hit_bottom)
        heights.push_back(zMin);
    else if(heights.empty())
        heights.push_back(zMax);

    std::vector<shared_ptr<Area> > sections;
    sections.reserve(heights.size());
    for(double z : heights) {
        gp_Pln pln(gp_Pnt(0,0,z),gp_Dir(0,0,1));
        Standard_Real a,b,c,d;
        pln.Coefficients(a,b,c,d);
        BRepLib_MakeFace mkFace(pln,xMin,xMax,yMin,yMax);
        const TopoDS_Shape &face = mkFace.Face();

        shared_ptr<Area> area(new Area(&myParams));
        area->setPlane(face);
        for(const Shape &s : myShapes) {
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            for(TopExp_Explorer it(s.shape.Moved(loc), TopAbs_SOLID); it.More(); it.Next()) {
                Part::CrossSection section(a,b,c,it.Current());
                std::list<TopoDS_Wire> wires = section.slice(-d);
                if(wires.empty()) {
                    Base::Console().Warning("Section return no wires\n");
                    continue;
                }

                Part::FaceMakerBullseye mkFace;
                mkFace.setPlane(pln);
                for(const TopoDS_Wire &wire : wires)
                    mkFace.addWire(wire);
                try {
                    mkFace.Build();
                    if (mkFace.Shape().IsNull())
                        Base::Console().Warning("FaceMakerBullseye return null shape on section\n");
                    else {
                        builder.Add(comp,mkFace.Shape());
                        continue;
                    }
                }catch (Base::Exception &e){
                    Base::Console().Warning("FaceMakerBullseye failed on section: %s\n", e.what());
                }
                for(const TopoDS_Wire &wire : wires)
                    builder.Add(comp,wire);
            }

            // Make sure the compound has at least one edge
            for(TopExp_Explorer it(comp,TopAbs_EDGE);it.More();) {
                area->add(comp,s.op);
                break;
            }
        }
        if(area->myShapes.size())
            sections.push_back(area);
        else
            Base::Console().Warning("Discard empty section\n");
    }
    return std::move(sections);
}

TopoDS_Shape Area::getPlane(gp_Trsf *trsf) {
    if(!myWorkPlane.IsNull()) {
        if(trsf) *trsf = myTrsf;
        return myWorkPlane;
    }
    if(!isBuilt()) {
        myShapePlane.Nullify();
        for(const Shape &s : myShapes)
            findPlane(s.shape,myShapePlane,myTrsf);
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

#define AREA_SRC(_param) myParams.PARAM_FNAME(_param)
    PARAM_ENUM_CONVERT(AREA_SRC,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);

    if(myHaveSolid && myParams.SectionCount) {
        mySections = makeSections(myParams.SectionMode);
        return;
    }

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
                        AREA_CONVERT_OP;
                        myArea->Clip(Operation,&areaClip,SubjectFill,ClipFill);
                        areaClip.m_curves.clear();
                    }
                }
                op=s.op;
            }
            addToBuild(op==OperationUnion?*myArea:areaClip,s.shape);
            pending = true;
        }
        if(mySkippedShapes && !myHaveSolid)
            Base::Console().Warning("%s %d non coplanar shapes\n",
                myParams.Coplanar==CoplanarForce?"Skipped":"Found",mySkippedShapes);

        if(pending){
            if(myParams.OpenMode!=OpenModeNone)
                myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);
            if(op == OperationCompound)
                myArea->m_curves.splice(myArea->m_curves.end(),areaClip.m_curves);
            else{
                AREA_CONVERT_OP;
                myArea->Clip(Operation,&areaClip,SubjectFill,ClipFill);
            }
        }
        myArea->m_curves.splice(myArea->m_curves.end(),myAreaOpen->m_curves);

        //Reassemble wires after explode
        if(myParams.Explode) {
            std::list<TopoDS_Edge> edges;
            gp_Trsf trsf(myTrsf.Inverted());
            for(const auto &c : myArea->m_curves) {
                TopoDS_Wire wire = toShape(c,&trsf);
                if(wire.IsNull()) continue;
                TopExp_Explorer it(wire, TopAbs_EDGE);
                edges.push_back(TopoDS::Edge(it.Current()));
            }
            Area area(&myParams);
            area.myParams.Explode = false;
            area.myParams.Coplanar = CoplanarNone;
            area.myWorkPlane = getPlane(&area.myTrsf);
            while(edges.size()) {
                BRepBuilderAPI_MakeWire mkWire;
                for(const auto &e : Part::sort_Edges(myParams.Tolerance,edges))
                    mkWire.Add(TopoDS::Edge(e));
                area.add(mkWire.Wire(),OperationCompound);
            }
            area.build();
            myArea = std::move(area.myArea);
        }

    }catch(...) {
        clean();
        throw;
    }
}

list<TopoDS_Shape> Area::sortWires(int index, int count, const gp_Pnt *pstart, 
        gp_Pnt *_pend, PARAM_ARGS(PARAM_FARG,AREA_PARAMS_MIN_DIST))
{
    std::list<TopoDS_Shape> wires;

    build();

    gp_Pnt pend,pt;
    if(pstart) pt = *pstart;

    pt.Transform(TopLoc_Location(myTrsf));

    if(mySections.size()) {
        if(index>=(int)mySections.size())
            throw Base::ValueError("index out of bound");
        TopLoc_Location loc(myTrsf.Inverted());
        if(index<0) {
            index = 0;
            count = mySections.size();
        }
        if(count<=0 || count>(int)mySections.size())
            count = mySections.size();
        for(int i=index;i<count;++i) {
            const std::list<TopoDS_Shape> ws = 
                mySections[i]->sortWires(-1,0,&pt,&pend,
                        PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_MIN_DIST));
            for(auto &wire : ws)
                wires.push_back(wire.Moved(loc));
            pt = pend;
        }
        if(_pend)
            *_pend = pend.Transformed(loc);
        return wires;
    }

    if(!myArea || myArea->m_curves.empty()) return wires;

    CArea area(*myArea);
    Point p(pt.X(),pt.Y());
    area.ChangeStartToNearest(&p,
        PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_MIN_DIST));
    gp_Trsf trsf(myTrsf.Inverted());
    for(const CCurve &c : area.m_curves) {
        const TopoDS_Wire &wire = toShape(c,&trsf);
        if(wire.IsNull()) continue;
        wires.push_back(toShape(c,&trsf));
    }
    if(_pend) {
        gp_Pnt pend = pt;
        if(area.m_curves.size() && 
           area.m_curves.back().m_vertices.size()) 
        {
            const Point &pt = area.m_curves.back().m_vertices.back().m_p;
            pend.SetCoord(pt.x,pt.y,0.0);
        }
        *_pend = pend.Transformed(TopLoc_Location(trsf));
    }
    return wires;
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
        TopLoc_Location loc(myTrsf.Inverted());\
        if(_index<0) {\
            BRep_Builder builder;\
            TopoDS_Compound compound;\
            builder.MakeCompound(compound);\
            for(shared_ptr<Area> area : mySections){\
                const TopoDS_Shape &s = area->_op(-1, ## __VA_ARGS__);\
                if(s.IsNull()) continue;\
                builder.Add(compound,s.Moved(loc));\
            }\
            for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();)\
                return compound;\
            return TopoDS_Shape();\
        }\
        const TopoDS_Shape &shape = mySections[_index]->_op(-1, ## __VA_ARGS__);\
        if(!shape.IsNull())\
            return shape.Moved(loc);\
        return shape;\
    }\
}while(0)

TopoDS_Shape Area::getShape(int index) {
    build();
    AREA_SECTION(getShape,index);

    if(myShapeDone) return myShape;

    if(!myArea) return TopoDS_Shape();

    CAreaConfig conf(myParams);

#define AREA_MY(_param) myParams.PARAM_FNAME(_param)

    // if no offset or thicken, try pocket
    if(fabs(myParams.Offset) < Precision::Confusion() && !myParams.Thicken) {
        if(myParams.PocketMode == PocketModeNone) {
            myShape = toShape(*myArea,myParams.Fill);
            myShapeDone = true;
            return myShape;
        }
        myShape = makePocket(-1,PARAM_FIELDS(AREA_MY,AREA_PARAMS_POCKET));
        myShapeDone = true;
        return myShape;
    }

    // if no pocket, do offset or thicken
    if(myParams.PocketMode == PocketModeNone){
        myShape = makeOffset(-1,PARAM_FIELDS(AREA_MY,AREA_PARAMS_OFFSET));
        myShapeDone = true;
        return myShape;
    }

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
    for(shared_ptr<CArea> area : areas) {
        if(myParams.Thicken)
            area->Thicken(myParams.ToolRadius);
        const TopoDS_Shape &shape = toShape(*area,fill);
        builder.Add(compound,toShape(*area,fill));
    }
    // make sure the compound has at least one edge
    for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();) {
        builder.Add(compound,areaPocket.makePocket(
                    -1,PARAM_FIELDS(AREA_MY,AREA_PARAMS_POCKET)));
        myShape = compound;
        break;
    }
    myShapeDone = true;
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
            area.Thicken(myParams.ToolRadius);
            return toShape(area,FillFace);
        }
        return TopoDS_Shape();
    }
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    for(shared_ptr<CArea> area : areas) {
        short fill;
        if(myParams.Thicken && myParams.ToolRadius>Precision::Confusion()) {
            area->Thicken(myParams.ToolRadius);
            fill = FillFace;
        }else if(areas.size()==1)
            fill = myParams.Fill;
        else
            fill = FillNone;
        const TopoDS_Shape &shape = toShape(*area,fill);
        if(shape.IsNull()) continue;
        builder.Add(compound,shape);
    }
    for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();)
        return compound;
    return TopoDS_Shape();
}

void Area::makeOffset(list<shared_ptr<CArea> > &areas,
                      PARAM_ARGS(PARAM_FARG,AREA_PARAMS_OFFSET))
{
    if(fabs(offset)<Precision::Confusion())
        return;

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

    PARAM_ENUM_CONVERT(AREA_SRC,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_OFFSET_CONF);
#ifdef AREA_OFFSET_ALGO
    PARAM_ENUM_CONVERT(AREA_SRC,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);
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

        if(area.m_curves.empty())
            return;
    }
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
        Offset = -tool_radius-extra_offset;
        ExtraPass = -1;
        Stepover = -stepover;
        return makeOffset(index,PARAM_FIELDS(PARAM_FNAME,AREA_PARAMS_OFFSET));
    }case Area::PocketModeZigZagOffset:
	    pm = ZigZagThenSingleOffsetPocketMode;
        break;
    default:
        throw Base::ValueError("unknown poket mode");
    }

    CAreaConfig conf(myParams);
    CAreaPocketParams params(
            tool_radius,extra_offset,stepover,from_center,pm,zig_angle);
    CArea in(*myArea),out;
    // MakePcoketToolPath internally uses libarea Offset which somehow demands
    // reorder before input, otherwise nothing is shown.
    in.Reorder();
    in.MakePocketToolpath(out.m_curves,params);

    if(myParams.Thicken){
        out.Thicken(tool_radius);
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
        if(pnext.Distance(pt)<Precision::Confusion())
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
                Base::Console().Warning(
                        "Arc correction: %lf,%lf, center(%lf,%lf)->(%lf,%lf)\n", 
                        r,r2,center.X(),center.Y(),newCenter.X(),newCenter.Y());
                center = newCenter;
            }
            gp_Ax2 axis(center, gp_Dir(0,0,v.m_type));
            mkWire.Add(BRepBuilderAPI_MakeEdge(gp_Circ(axis,r),pt,pnext).Edge());
        }
        pt = pnext;
    }
    if(!mkWire.IsDone())
        return TopoDS_Wire();

    if(c.IsClosed() && !BRep_Tool::IsClosed(mkWire.Wire())){
        // This should never happen after changing libarea's
        // Point::tolerance to be the same as Precision::Confusion().  
        // Just leave it here in case.
        BRepAdaptor_Curve curve(mkWire.Edge());
        gp_Pnt p1(curve.Value(curve.FirstParameter()));
        gp_Pnt p2(curve.Value(curve.LastParameter()));
        std::stringstream str;
        str<< "warning: patch open wire type " << 
            c.m_vertices.back().m_type << endl << 
            '(' << p1.X() << ',' << p1.Y() << ')' << endl << 
            '(' << p2.X() << ',' << p2.Y() << ')' << endl << 
            '(' << pt.X() << ',' << pt.Y() << ')' << endl <<
            '(' << pstart.X() << ',' <<pstart.Y() <<')' <<endl;
        Base::Console().Warning(str.str().c_str());
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
    for(TopExp_Explorer it(compound,TopAbs_EDGE);it.More();) {
        if(fill) {
            try{
                Part::FaceMakerBullseye mkFace;
                if(trsf)
                    mkFace.setPlane(gp_Pln().Transformed(*trsf));
                for(TopExp_Explorer it(compound, TopAbs_WIRE); it.More(); it.Next())
                    mkFace.addWire(TopoDS::Wire(it.Current()));
                mkFace.Build();
                if (mkFace.Shape().IsNull())
                    Base::Console().Warning("FaceMakerBullseye returns null shape\n");
                return mkFace.Shape();
            }catch (Base::Exception &e){
                Base::Console().Warning("FaceMakerBullseye failed: %s\n", e.what());
            }
        }
        return compound;
    }
    return TopoDS_Shape();
}

std::list<TopoDS_Shape> Area::sortWires(const std::list<TopoDS_Shape> &shapes, 
    const AreaParams *params, const gp_Pnt *_pstart, gp_Pnt *_pend, 
    PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SORT_WIRES))
{
    std::list<TopoDS_Shape> wires;

    //Heristic sorting by shape's vertex Z value. For performance's sake, we don't
    //perform any planar checking here
    std::multimap<double,TopoDS_Shape> shape_map;

    for (auto &shape : shapes) {
        std::list<TopoDS_Shape> subshapes;
        if(!explode) 
            subshapes.push_back(shape);
        else{
            bool haveShape=false;
            for(TopExp_Explorer it(shape,TopAbs_WIRE);it.More();it.Next()) {
                haveShape=true;
                subshapes.push_back(it.Current());
            }
            if(!haveShape) {
                for(TopExp_Explorer it(shape,TopAbs_EDGE);it.More();it.Next())
                    subshapes.push_back(it.Current());
            }
        }
        //Order the shapes by its vertex Z value.
        for(auto &s : subshapes) {
            bool first=true;
            double z=0.0;
            for(TopExp_Explorer it(s,TopAbs_VERTEX);it.More();) {
                gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(it.Current()));
                if(first || z < p.Z()) {
                    first = false;
                    z = p.Z();
                }
                if(!top_z) break;
            }
            shape_map.insert(std::pair<double,TopoDS_Shape>(z,s));
        }
    }
    if(!shape_map.size()) 
        return wires;

    Area area(params);
    //We'll do planar checking here, so disable Area planar check
    area.myParams.Coplanar = Area::CoplanarNone;

    gp_Pnt pstart,pend;
    if(_pstart) pstart = *_pstart;
    TopoDS_Shape plane = shape_map.rbegin()->second;
    area.setPlane(plane);
    for(auto &item : boost::adaptors::reverse(shape_map)) {
        //do planar checking, and sort wires grouped by plane
        if(!Area::isCoplanar(plane,item.second)) {
            wires.splice(wires.end(),area.sortWires(
                -1,0,&pstart,&pend, PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_MIN_DIST)));
            pstart = pend;
            area.clean(true);
            plane = item.second;
            area.setPlane(plane);
        }
        area.add(item.second,Area::OperationCompound);
    }
    wires.splice(wires.end(),area.sortWires(
        -1,0,&pstart,&pend, PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_MIN_DIST)));
    if(_pend) *_pend = pend;
    return wires;
}

static void addCommand(Toolpath &path, const gp_Pnt &p, 
        bool g0=false, double g0height=0.0, double clearance=0.0)
{
    Command cmd;
    cmd.Name = g0?"G0":"G1";
    if(g0 && fabs(g0height)>Precision::Confusion()) {
        cmd.Parameters["Z"] = g0height;
        path.addCommand(cmd);
        cmd.Parameters["X"] = p.X();
        cmd.Parameters["Y"] = p.Y();
        path.addCommand(cmd);
        if(fabs(clearance)>Precision::Confusion()) {
            cmd.Parameters["Z"] = p.Z()+clearance;
            path.addCommand(cmd);
            cmd.Name = "G1";
        }
    }else
        cmd.Parameters["X"] = p.X();
        cmd.Parameters["Y"] = p.Y();
    cmd.Parameters["Z"] = p.Z();
    path.addCommand(cmd);
}

static void addCommand(Toolpath &path, 
        const gp_Pnt &pstart, const gp_Pnt &pend, 
        const gp_Pnt &center, bool clockwise) 
{
    Command cmd;
    cmd.Name = clockwise?"G2":"G3";
    cmd.Parameters["I"] = center.X()-pstart.X();
    cmd.Parameters["J"] = center.Y()-pstart.Y();
    cmd.Parameters["K"] = center.Z()-pstart.Z();
    cmd.Parameters["X"] = pend.X();
    cmd.Parameters["Y"] = pend.Y();
    cmd.Parameters["Z"] = pend.Z();
    path.addCommand(cmd);
}

void Area::toPath(Toolpath &path, const std::list<TopoDS_Shape> &shapes,
            const gp_Pnt *pstart, PARAM_ARGS(PARAM_FARG,AREA_PARAMS_PATH))
{
    std::list<TopoDS_Shape> wires;
    if(sort) 
        wires = sortWires(shapes,NULL,pstart);
    else{
        for(auto &shape : shapes) {
            if (shape.IsNull())
                continue;
            bool haveShape=false;
            for(TopExp_Explorer it(shape,TopAbs_WIRE);it.More();it.Next()) {
                haveShape=true;
                wires.push_back(it.Current());
            }
            if(haveShape) continue;
            for(TopExp_Explorer it(shape,TopAbs_EDGE);it.More();it.Next())
                wires.push_back(BRepBuilderAPI_MakeWire(TopoDS::Edge(it.Current())).Wire());
        }
    }
    
    if(threshold < Precision::Confusion())
        threshold = Precision::Confusion();
    gp_Pnt plast,p;
    if(pstart) plast = *pstart;
    bool first = true;
    for(const TopoDS_Shape &wire : wires) {
        BRepTools_WireExplorer xp(TopoDS::Wire(wire));
        p = BRep_Tool::Pnt(xp.CurrentVertex());
        if(first||(p.Z()>=plast.Z()&&p.Distance(plast)>threshold))
            addCommand(path,p,true,height,clearance);
        else
            addCommand(path,p);
        plast = p;
        first = false;
        for(;xp.More();xp.Next(),plast=p) {
            BRepAdaptor_Curve curve(xp.Current());
            bool reversed = (xp.Current().Orientation()==TopAbs_REVERSED);
            p = curve.Value(reversed?curve.FirstParameter():curve.LastParameter());

            switch (curve.GetType()) {
            case GeomAbs_Line: {
                addCommand(path,p);
                break;
            } case GeomAbs_Circle:{
                double first = curve.FirstParameter();
                double last = curve.LastParameter();
                gp_Circ circle = curve.Circle();
                gp_Ax1 axis = circle.Axis();
                bool clockwise = axis.Direction().Z()<0;
                if(reversed) clockwise = !clockwise;
                gp_Pnt center = axis.Location();
                if(fabs(first-last)>M_PI) {
                    // Split arc(circle) larger than half circle. 
                    gp_Pnt mid = curve.Value((last-first)*0.5+first);
                    addCommand(path,plast,mid,center,clockwise);
                    plast = mid;
                }
                addCommand(path,plast,p,center,clockwise);
                break;
            } default: {
                // Discretize all other type of curves
                GCPnts_QuasiUniformDeflection discretizer(curve, deflection, 
                        curve.FirstParameter(), curve.LastParameter());
                if (discretizer.IsDone () && discretizer.NbPoints () > 1) {
                    int nbPoints = discretizer.NbPoints ();
                    if(reversed) {
                        for (int i=nbPoints-1; i>=1; --i) {
                            gp_Pnt pt = discretizer.Value (i);
                            addCommand(path,pt);
                        }
                    }else{
                        for (int i=2; i<=nbPoints; i++) {
                            gp_Pnt pt = discretizer.Value (i);
                            addCommand(path,pt);
                        }
                    }
                }else
                    Standard_Failure::Raise("Curve discretization failed");
            }}
        }
    }
}
