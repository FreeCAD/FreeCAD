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
#include <GCPnts_UniformAbscissa.hxx>
#include <BRepBndLib.hxx>
#include <BRepLib_MakeFace.hxx>
#include <Bnd_Box.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>

#include <Base/Exception.h>
#include <Base/Tools.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FaceMakerBullseye.h>
#include <Mod/Part/App/CrossSection.h>
#include "Area.h"
#include "../libarea/Area.h"

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
            AREA_WARN("open wires discarded in clipping shapes");
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
    TopoDS_Shape &myShape;
    gp_Trsf &myTrsf;
    double &myZ;
    FindPlane(TopoDS_Shape &s, gp_Trsf &t, double &z)
        :myShape(s),myTrsf(t),myZ(z)
    {}
    void operator()(const TopoDS_Shape &shape, int) {
        gp_Trsf trsf;
        BRepLib_FindSurface finder(shape,-1,Standard_True);
        if (!finder.Found()) 
            return;

        gp_Ax3 pos = GeomAdaptor_Surface(finder.Surface()).Plane().Position();

        //force plane to be right handed
        if(!pos.Direct())
            pos = gp_Ax3(pos.Ax2());
        gp_Dir dir(pos.Direction());

        trsf.SetTransformation(pos);
        if(fabs(dir.X())<Precision::Confusion() &&
            fabs(dir.Y())<Precision::Confusion()) 
        {
            gp_Pnt origin = pos.Location();

            // Probably another OCC bug, sometimes pos.Location().Z() for XY
            // plane is stuck at zero, even though the plane is at above. So we
            // double check the first vertex Z value
            TopExp_Explorer it(shape,TopAbs_VERTEX);
            double z = BRep_Tool::Pnt(TopoDS::Vertex(it.Current())).Z();
            if(fabs(origin.Z()-z)>Precision::Confusion()) {
                AREA_WARN("XY plane has wrong Z height "<<origin.Z()<<", "<<z);
                gp_Trsf trsf2;
                trsf2.SetTranslationPart(gp_XYZ(0,0,origin.Z()-z));
                trsf.Multiply(trsf2);
            }
            gp_Pnt pt = origin.Transformed(TopLoc_Location(trsf));
            if(fabs(pt.X()) > Precision::Confusion() ||
            fabs(pt.Y()) > Precision::Confusion() ||
            fabs(pt.Z()) > Precision::Confusion()) {
                AREA_WARN("wrong transformation "<<AREA_PT(pt));
            }
            if(!myShape.IsNull() && myZ > z)
                return;
            myZ = z;
        }else if(!myShape.IsNull())
            return;
        myShape = shape;
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
                    AREA_LOG("Section returns no wires");
                    continue;
                }

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
            AREA_WARN("Discard empty section");
        TIME_PRINT(t1,"makeSection " << z);
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

#define AREA_SRC(_param) myParams.PARAM_FNAME(_param)
    PARAM_ENUM_CONVERT(AREA_SRC,PARAM_FNAME,PARAM_ENUM_EXCEPT,AREA_PARAMS_CLIPPER_FILL);

    if(myHaveSolid && myParams.SectionCount) {
        mySections = makeSections(myParams.SectionMode);
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
            AREA_WARN((myParams.Coplanar==CoplanarForce?"Skipped ":"Found ")<<
                mySkippedShapes<<" non coplanar shapes");

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

        TIME_TRACE(t,"prepare");

    }catch(...) {
        clean();
        throw;
    }
}

list<TopoDS_Shape> Area::sortWires(int index, int count, const gp_Pnt *pstart, 
        gp_Pnt *_pend, PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SORT))
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
                mySections[i]->sortWires(0,0,&pt,&pend,
                        PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_SORT));
            for(auto &wire : ws)
                wires.push_back(wire.Moved(loc));
            pt = pend;
        }
        if(_pend)
            *_pend = pend.Transformed(loc);
        return std::move(wires);
    }

    if(!myArea || myArea->m_curves.empty()) return wires;

    CArea area(*myArea);
    Point p(pt.X(),pt.Y());
    area.ChangeStartToNearest(&p, PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_MIN_DIST));
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
    return std::move(wires);
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

    TIME_PRINT(t,"makePocket");

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
                AREA_WARN("Arc correction: "<<r<<", "<<r2<<", center"<<
                        AREA_PT(center)<<"->"<<AREA_PT(newCenter));
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
        AREA_WARN("warning: patch open wire type " << 
            c.m_vertices.back().m_type<<endl<<AREA_PT(p1)<<endl<<
            AREA_PT(p2)<<endl<<AREA_PT(pt)<<endl<<AREA_PT(pstart));
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
    return TopoDS_Shape();
}

struct WireInfo {
    TopoDS_Wire wire;
    gp_Pnt pend;
    gp_Pnt pstart;
};

struct GetWires {
    std::list<WireInfo> &wires;
    GetWires(std::list<WireInfo> &ws)
        :wires(ws)
    {}
    void operator()(const TopoDS_Shape &shape, int type) {
        WireInfo info;
        if(type == TopAbs_WIRE)
            info.wire = TopoDS::Wire(shape);
        else
            info.wire = BRepBuilderAPI_MakeWire(TopoDS::Edge(shape)).Wire();

        BRepTools_WireExplorer xp(info.wire);
        info.pstart = BRep_Tool::Pnt(xp.CurrentVertex());
        for(;xp.More();xp.Next());
        info.pend = BRep_Tool::Pnt(xp.CurrentVertex());
        wires.push_back(info);
    }
};

struct ShapeInfo{
    gp_Pln myPln;
    std::list<WireInfo> myWires;
    TopoDS_Shape myShape;
    gp_Pnt myBestPt;
    std::list<WireInfo>::iterator myBestWire;
    TopoDS_Shape mySupport;
    bool mySupportEdge;
    bool myPlanar;
    bool myRebase;
    bool myStart;

    ShapeInfo(BRepLib_FindSurface &finder, const TopoDS_Shape &_shape)
        :myPln(GeomAdaptor_Surface(finder.Surface()).Plane())
        ,myShape(_shape)
        ,myPlanar(true)
    {}
    ShapeInfo(const TopoDS_Shape &_shape)
        :myShape(_shape)
        ,myPlanar(false)
    {}
    double nearest(const gp_Pnt &pt) {
        if(myWires.empty()) 
            foreachSubshape(myShape,GetWires(myWires),TopAbs_WIRE);
        TopoDS_Shape v = BRepBuilderAPI_MakeVertex(pt);
        bool first = true;
        double best_d=1e20;
        myBestWire = myWires.begin();
        for(auto it=myWires.begin();it!=myWires.end();++it) {
            const TopoDS_Shape &wire = it->wire;
            TopoDS_Shape support;
            bool support_edge;
            double d = 0;
            gp_Pnt p;
            bool done = false;
            bool is_start = false;
            if(BRep_Tool::IsClosed(wire)) {
                BRepExtrema_DistShapeShape extss(v,wire);
                if(extss.IsDone() && extss.NbSolution()) {
                    d = extss.Value();
                    p = extss.PointOnShape2(1);
                    support = extss.SupportOnShape2(1);
                    support_edge = extss.SupportTypeShape2(1)==BRepExtrema_IsOnEdge;
                    done = true;
                }else
                    AREA_WARN("BRepExtrema_DistShapeShape failed");
            }
            if(!done){
                double d1 = pt.Distance(it->pstart);
                double d2 = pt.Distance(it->pend);
                if(d1<d2) {
                    d = d1;
                    p = it->pstart;
                    is_start = true;
                }else{
                    d = d2;
                    p = it->pend;
                    is_start = false;
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
        AREA_TRACE("rebase wire");
        BRepBuilderAPI_MakeWire mkWire;
        TopoDS_Shape estart;
        TopoDS_Edge eend;

        if(min_dist < Precision::Confusion())
            min_dist = Precision::Confusion();

        for(int state=0;state<3;++state) {
            BRepTools_WireExplorer xp(TopoDS::Wire(myBestWire->wire));
            pend = BRep_Tool::Pnt(xp.CurrentVertex());

            //checking the case of bestpoint == wire start
            if(state==0 && !mySupportEdge && pend.Distance(myBestPt)<Precision::Confusion()) {
                AREA_TRACE("keep start");
                pend = myBestWire->pend;
                return myBestWire->wire;
            }

            gp_Pnt pt;
            for(;xp.More();xp.Next(),pend=pt) {
                //state==2 means we are in second pass. estart marks the new start of the wire.
                //so seeing estart means we're done
                if(state==2 && estart.IsEqual(xp.Current()))
                    break;

                BRepAdaptor_Curve curve(xp.Current());
                bool reversed = (xp.Current().Orientation()==TopAbs_REVERSED);
                pt = curve.Value(reversed?curve.FirstParameter():curve.LastParameter());
                //state!=0 means we've found the new start of wire, now just keep adding new edges
                if(state) {
                    mkWire.Add(xp.Current());
                    pend = pt;
                    continue;
                }
                //state==0 means we are looking for the new start
                if(mySupportEdge) {
                    //if best point is on some edge, break the edge in half
                    if(xp.Current().IsEqual(mySupport)) {
                        double d1 = pend.Distance(myBestPt);
                        double d2 = pt.Distance(myBestPt);

                        if(d1>min_dist && d2>min_dist) {
                            BRepBuilderAPI_MakeEdge mkEdge1,mkEdge2;
                            if(reversed) {
                                mkEdge1.Init(curve.Curve().Curve(), myBestPt, myBestPt);
                                mkEdge2.Init(curve.Curve().Curve(), pt, myBestPt);
                            }else{
                                mkEdge1.Init(curve.Curve().Curve(), pend, myBestPt);
                                mkEdge2.Init(curve.Curve().Curve(), myBestPt, pt);
                            }
                            if(mkEdge1.IsDone() && mkEdge2.IsDone()) {
                                if(reversed) {
                                    eend = TopoDS::Edge(mkEdge1.Edge().Reversed());
                                    mkWire.Add(TopoDS::Edge(mkEdge2.Edge().Reversed()));
                                }else{
                                    eend = mkEdge1.Edge();
                                    mkWire.Add(mkEdge2.Edge());
                                }
                                estart = mySupport;
                                state = 1;
                                AREA_TRACE("edge broken "<<AREA_PT(pend)<<", " << AREA_PT(myBestPt)
                                        << ", " << AREA_PT(pt) << ", " << d1 << ", "  << d2);
                                continue;
                            }
                            AREA_WARN("edge break failed "<<AREA_PT(pend)<<", " << AREA_PT(myBestPt)
                                    << ", " << AREA_PT(pt) << ", " << d1 << ", " << d2);
                        }

                        if(d1<d2) {
                            AREA_TRACE("break edge->start");
                            estart = xp.Current();
                            state = 1;
                            mkWire.Add(xp.Current());
                        }else{
                            AREA_TRACE("break edge->end");
                            mySupportEdge = false;
                            myBestPt = pt;
                            continue;
                        }
                    }
                }else if(myBestPt.Distance(pend)<Precision::Confusion()){
                    AREA_TRACE("break vertex");
                    //if best point is on some vertex
                    estart = xp.Current();
                    state = 1;
                    mkWire.Add(xp.Current());
                }
            }
        }
        if(!eend.IsNull())
            mkWire.Add(eend);
        if(mkWire.IsDone())
            return mkWire.Wire();
        AREA_WARN("wire rebase failed");
        pend = myBestWire->pend;
        return myBestWire->wire;
    }

    std::list<TopoDS_Shape> sortWires3D(gp_Pnt &pend,double min_dist) {
        std::list<TopoDS_Shape> wires;
        while(true) {
            AREA_TRACE("3D sort pt " << AREA_PT(myBestPt));
            if(myRebase) {
                AREA_TRACE("3D sort rebase");
                pend = myBestPt;
                wires.push_back(rebaseWire(pend,min_dist));
            }else if(!myStart){
                AREA_TRACE("3D sort reverse");
                wires.push_back(myBestWire->wire.Reversed());
                pend = myBestWire->pstart;
            }else{
                wires.push_back(myBestWire->wire);
                pend = myBestWire->pend;
            }
            AREA_TRACE("3D sort end " << AREA_PT(pend));
            myWires.erase(myBestWire);
            if(myWires.empty()) break;
            nearest(pend);
        }
        return std::move(wires);
    }
};

struct ShapeInfoBuilder {
    std::list<ShapeInfo> &myList;

    ShapeInfoBuilder(std::list<ShapeInfo> &list)
        :myList(list)
    {}

    void operator()(const TopoDS_Shape &shape, int) {
        BRepLib_FindSurface finder(shape,-1,Standard_True);
        if(finder.Found())
            myList.push_back(ShapeInfo(finder,shape));
        else
            myList.push_back(ShapeInfo(shape));
    }
};


std::list<TopoDS_Shape> Area::sortWires(const std::list<TopoDS_Shape> &shapes, 
    const AreaParams *params, const gp_Pnt *_pstart, gp_Pnt *_pend, 
    PARAM_ARGS(PARAM_FARG,AREA_PARAMS_SORT))
{
    std::list<TopoDS_Shape> wires;

    if(shapes.empty()) return wires;

    if(sort_mode == SortModeNone) {
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
        return std::move(wires);
    }

    std::list<ShapeInfo> shape_list;

    TIME_INIT2(t,t1);
#define SORT_WIRE_TIME(_msg) \
    TIME_PRINT(t1,"sortWires "<< _msg)

    if(sort_mode == SortMode3D) {
        for(auto &shape : shapes)
            shape_list.push_back(ShapeInfo(shape));
    }else{
        //first pass, find plane of each shape
        for(auto &shape : shapes) {
            //explode the shape
            foreachSubshape(shape,ShapeInfoBuilder(shape_list));
        }

        if(shape_list.empty()) 
            return wires;

        SORT_WIRE_TIME("plan finding");
    }
 
    Bnd_Box bounds;
    gp_Pnt pstart,pend;
    if(_pstart) 
        pstart = *_pstart;
    bool use_bound = fabs(pstart.X())<Precision::Confusion() &&
                     fabs(pstart.Y())<Precision::Confusion() &&
                     fabs(pstart.Z())<Precision::Confusion();

    if(use_bound || sort_mode == SortMode2D5) {
        //Second stage, group shape by its plane, and find overall boundary
        for(auto itNext=shape_list.begin(),it=itNext;it!=shape_list.end();it=itNext) {
            ++itNext;
            if(use_bound)
                BRepBndLib::Add(it->myShape, bounds, Standard_False);
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
        SORT_WIRE_TIME("plane merging");
    }

    Area area(params);
    //We have done planar checking here, so disable Area planar check
    area.myParams.Coplanar = Area::CoplanarNone;

    DURATION_INIT2(td1,td2);

    if(use_bound) {
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        pstart.SetCoord(xMax,yMax,zMax);
    }
    bool has_2d5=false,has_3d=false;
    while(shape_list.size()) {
        AREA_TRACE("start " << shape_list.size() << ' ' << AREA_PT(pstart));
        double best_d;
        auto best_it = shape_list.begin();
        bool first = true;
        for(auto it=best_it;it!=shape_list.end();++it) {
            double d;
            gp_Pnt pt;
            if(it->myPlanar){
                d = it->myPln.Distance(pstart);
#define AREA_TIME_2D5 \
                DURATION_PLUS(td1,t1);\
                has_2d5=true

                AREA_TIME_2D5;
            }else{
                d = it->nearest(pstart);
#define AREA_TIME_3D \
                DURATION_PLUS(td2,t1);\
                has_3d=true

                AREA_TIME_3D;
            }
            if(first || d<best_d) {
                first = false;
                best_it = it;
                best_d = d;
            }
        }
        if(best_it->myPlanar) {
            area.clean(true);
            area.myWorkPlane = best_it->myShape;
            area.myTrsf.SetTransformation(best_it->myPln.Position());
            area.add(best_it->myShape,Area::OperationCompound);
            wires.splice(wires.end(),area.sortWires(
                0,-1,&pstart,&pend, PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_SORT)));
            AREA_TIME_2D5;
        }else{
            wires.splice(wires.end(),best_it->sortWires3D(pend,min_dist));
            AREA_TIME_3D;
        }
        pstart = pend;
        shape_list.erase(best_it);
    }
    if(_pend) *_pend = pend;

    if(has_2d5) DURATION_PRINT(td1,"sortWires 2D5");
    if(has_3d) DURATION_PRINT(td2,"sortWires 3D");
    TIME_PRINT(t,"sortWires total");
    return std::move(wires);
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
        const AreaParams *_params, const gp_Pnt *pstart, gp_Pnt *pend, 
        PARAM_ARGS(PARAM_FARG,AREA_PARAMS_PATH))
{
    std::list<TopoDS_Shape> wires;

    AreaParams params;
    if(_params) params =*_params;
    wires = sortWires(shapes,&params,pstart,pend,
            PARAM_FIELDS(PARAM_FARG,AREA_PARAMS_SORT));
    
    if(threshold < Precision::Confusion())
        threshold = Precision::Confusion();
    gp_Pnt plast,p;
    if(pstart) plast = *pstart;
    bool first = true;
    for(const TopoDS_Shape &wire : wires) {
        BRepTools_WireExplorer xp(TopoDS::Wire(wire));
        p = BRep_Tool::Pnt(xp.CurrentVertex());
        if(first||p.Distance(plast)>threshold)
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
                if(segmentation > Precision::Confusion()) {
                    GCPnts_UniformAbscissa discretizer(curve, segmentation, 
                            curve.FirstParameter(), curve.LastParameter());
                    if (discretizer.IsDone () && discretizer.NbPoints () > 2) {
                        int nbPoints = discretizer.NbPoints ();
                        if(reversed) {
                            for (int i=nbPoints-1; i>=1; --i) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addCommand(path,pt);
                            }
                        }else{
                            for (int i=2; i<=nbPoints; i++) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addCommand(path,pt);
                            }
                        }
                        break;
                    }
                }
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
                if(segmentation > Precision::Confusion()) {
                    GCPnts_UniformAbscissa discretizer(curve, segmentation, 
                            curve.FirstParameter(), curve.LastParameter());
                    if (discretizer.IsDone () && discretizer.NbPoints () > 2) {
                        int nbPoints = discretizer.NbPoints ();
                        if(reversed) {
                            for (int i=nbPoints-1; i>=1; --i) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addCommand(path,plast,pt,center,clockwise);
                                plast = pt;
                            }
                        }else{
                            for (int i=2; i<=nbPoints; i++) {
                                gp_Pnt pt = curve.Value(discretizer.Parameter(i));
                                addCommand(path,plast,pt,center,clockwise);
                                plast = pt;
                            }
                        }
                        break;
                    }
                }
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
                GCPnts_QuasiUniformDeflection discretizer(curve, params.Deflection, 
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
